#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>
#include <thread>
#include <utility>

#include  <openssl/bio.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <json/json.h>

#include "Client.h"
#include "CliConfig.h"
#include "SoilMoistureMonitoringClient.h"

#include "organic_dump.pb.h"

#include "I2c/Devices/Ads1115AnalogToDigitalConverter/Ads1115.h"
#include "I2c/Devices/Ads1115AnalogToDigitalConverter/Ads1115Channel.h"
#include "I2c/I2cException.h"
#include "I2c/I2cClient.h"
#include "I2c/RpiI2cContext.h"
#include "System/RpiSystemContext.h"

namespace
{
using I2c::Ads1115Channel;
using organicdump::Client;
using organicdump::CliConfig;
using organicdump::SoilMoistureMonitoringClient;

constexpr size_t SOIL_MOISTURE_SENSOR_COUNT = 3;
constexpr const char *RPI_ID_JSON_NAME = "rpi-id";
constexpr const char *SOIL_MOISTURE_SENSOR_IDS = "soil-moisture-sensor-ids";

void InitLibraries(const char *app_name)
{
  google::InitGoogleLogging(app_name);
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
}

bool ParseSoilMoistureSensorIds(
    const std::string &config_path,
    size_t *raspberry_pi_id,
    std::vector<size_t> *out_soil_moisture_ids) {
  assert(out_soil_moisture_ids);

  std::ifstream json_file{config_path};
  if (!json_file.is_open()) {
    LOG(ERROR) << "Failed to open json config file";
    return false;
  }

  std::string json_file_str(
      (std::istreambuf_iterator<char>(json_file)),
      std::istreambuf_iterator<char>());

  Json::Value root;
  Json::Reader reader;
  reader.parse(json_file_str, root);

  *raspberry_pi_id = root[RPI_ID_JSON_NAME].asUInt64();
  Json::Value json_soil_moisture_sensor_ids = root[SOIL_MOISTURE_SENSOR_IDS];
  out_soil_moisture_ids->clear();
  for (size_t i = 0; i < json_soil_moisture_sensor_ids.size(); i++)
  {
    out_soil_moisture_ids->push_back(
        json_soil_moisture_sensor_ids[static_cast<int>(i)].asUInt64());
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

  assert(config.HasConfigFile());

  size_t raspberry_pi_id;
  std::vector<size_t> soil_moisture_ids;

  if (!ParseSoilMoistureSensorIds(
        config.GetConfigFile(),
        &raspberry_pi_id,
        &soil_moisture_ids))
  {
    LOG(ERROR) << "Failed to parse soil moisture sensor ids";
    return false;
  }

  LOG(ERROR) << "Raspberry Pi Id: " << raspberry_pi_id;
  for (size_t i = 0; i < soil_moisture_ids.size(); ++i)
  {
    LOG(ERROR) << "Soil moisture sensor[" << i << "]: " << soil_moisture_ids.at(i);
  }

  assert(soil_moisture_ids.size() == SOIL_MOISTURE_SENSOR_COUNT);
  std::unordered_map<Ads1115Channel, size_t> channel_to_sensor_table =
  {
    {Ads1115Channel::CHANNEL_0, soil_moisture_ids.at(0)},
    {Ads1115Channel::CHANNEL_1, soil_moisture_ids.at(1)},
    {Ads1115Channel::CHANNEL_2, soil_moisture_ids.at(2)},
  };

  SoilMoistureMonitoringClient client{
      config.GetIpv4(),
      config.GetPort(),
      config.GetCertFile(),
      config.GetKeyFile(),
      config.GetCaFile(),
      config.GetRetryConnectServerPeriod(),
      config.GetMeasurementPeriod(),
      std::move(channel_to_sensor_table)};

  if (!client.Run())
  {
    LOG(ERROR) << "Encountered failure while running monitoring client";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
