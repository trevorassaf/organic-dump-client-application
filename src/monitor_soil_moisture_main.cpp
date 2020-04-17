#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

#include  <openssl/bio.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Client.h"
#include "CliConfig.h"

#include "organic_dump.pb.h"

#include "I2c/Devices/Ads1115AnalogToDigitalConverter/Ads1115.h"
#include "I2c/Devices/Ads1115AnalogToDigitalConverter/Ads1115Channel.h"
#include "I2c/I2cException.h"
#include "I2c/I2cClient.h"
#include "I2c/RpiI2cContext.h"
#include "System/RpiSystemContext.h"

namespace
{
using organicdump::Client;
using organicdump::CliConfig;

using I2c::Ads1115;
using I2c::Ads1115Channel;
using I2c::I2cException;
using I2c::I2cClient;
using I2c::RpiI2cContext;
using System::RpiSystemContext;

using organicdump::Client;

const std::chrono::seconds SAMPLE_PERIOD{600};

constexpr size_t I2C_ADC_SLAVE_ID = 0x49;

const std::unordered_map<Ads1115Channel, size_t> kChannelToClientTable =
{
  {Ads1115Channel::CHANNEL_0, 1},
  {Ads1115Channel::CHANNEL_1, 2},
  {Ads1115Channel::CHANNEL_2, 3},
  {Ads1115Channel::CHANNEL_3, 4},
};

void InitLibraries(const char *app_name)
{
  google::InitGoogleLogging(app_name);

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
}

bool MeasureSoilMoisture(
    Client *client,
    I2cClient *i2c,
    uint16_t *out_reading)
{
  assert(client);
  assert(i2c);
  assert(out_reading);

  Ads1115 ads1115{i2c};
  uint16_t reading;

  for (const auto &entry : kChannelToClientTable)
  {
    if (!ads1115.Read(entry.first, &reading))
    {
      LOG(ERROR) << "Failed to read channel " << static_cast<int>(entry.first);
      return false;
    }

    if (!client->SendSoilMoistureMeasurement(
              entry.second,
              reading))
    {
      LOG(ERROR) << "Failed to upload soil moisture sensor reading for sensor "
                 << entry.second;
      return false;
    }
  }

  return true;
}

} // anonymous namespace

int main(int argc, char **argv)
{
  CliConfig config;
  if (!CliConfig::Parse(argc, argv, &config))
  {
      LOG(ERROR) << "Failed to parse CLI flags";
      return EXIT_FAILURE;
  }

  InitLibraries(argv[0]);

  Client client;
  if (!Client::Create(
          config.GetIpv4(),
          config.GetPort(),
          config.GetCertFile(),
          config.GetKeyFile(),
          config.GetCaFile(),
          &client))
  {
    LOG(ERROR) << "Failed to create client";
    return EXIT_FAILURE;
  }

  RpiSystemContext rpiSystemContext;
  RpiI2cContext rpiI2cContext{&rpiSystemContext};
  I2cClient *i2c = rpiI2cContext.GetBus1I2cClient();
  i2c->SetSlave(I2C_ADC_SLAVE_ID);

  while (true)
  {
    uint16_t soil_moisture_reading;
    if (!MeasureSoilMoisture(
            &client,
            i2c,
            &soil_moisture_reading))
    {
      LOG(ERROR) << "Failed to take soil moisture sensor reading";
      return EXIT_FAILURE;
    }

    std::this_thread::sleep_for(SAMPLE_PERIOD);
  }

  return EXIT_SUCCESS;
}
