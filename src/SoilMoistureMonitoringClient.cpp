#include "SoilMoistureMonitoringClient.h"

#include <thread>
#include <unordered_map>
#include <utility>

#include <glog/logging.h>

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
using I2c::Ads1115;
using I2c::I2cClient;
using I2c::RpiI2cContext;
using System::RpiSystemContext;

constexpr size_t I2C_ADC_SLAVE_ID = 0x49;
} // namespace

namespace organicdump
{

SoilMoistureMonitoringClient::SoilMoistureMonitoringClient(
    std::string ipv4,
    int32_t port,
    std::string cert_file,
    std::string key_file,
    std::string ca_file,
    std::chrono::seconds retry_connect_server_period,
    std::chrono::seconds measurement_period,
    std::unordered_map<Ads1115Channel, size_t> channel_to_sensor_table)
  : ipv4_{ipv4},
    port_{port},
    cert_file_{cert_file},
    key_file_{key_file},
    ca_file_{ca_file},
    retry_connect_server_period_{retry_connect_server_period},
    measurement_period_{measurement_period},
    channel_to_sensor_table_{std::move(channel_to_sensor_table)} {}

SoilMoistureMonitoringClient::SoilMoistureMonitoringClient(SoilMoistureMonitoringClient &&other)
{
  StealResources(&other);
}

SoilMoistureMonitoringClient &SoilMoistureMonitoringClient::operator=(
    SoilMoistureMonitoringClient &&other)
{
  if (this != &other)
  {
    StealResources(&other);
  }
  return *this;
}

SoilMoistureMonitoringClient::~SoilMoistureMonitoringClient() {}

bool SoilMoistureMonitoringClient::Run()
{
  size_t consecutive_failed_connections = 0;
  while (true)
  {
    // Encapsulate Client in its own scope so that it gets destroyed before sleeping. It is
    // potentially harmful to keep the TCP connection alive for an extended period of time
    // because it produces "address in use" errors on the server.
    {
      Client client;
      if (Client::Create(
            ipv4_,
            port_,
            cert_file_,
            key_file_,
            ca_file_,
            &client))
      {
        LOG(INFO) << "Successfully connected to server: " << ipv4_ << ":" << port_;
        consecutive_failed_connections = 0;

        if (!MonitorSoilMoisture(&client)) {
          LOG(ERROR) << "Encountered error when monitoring soil moisture. Retrying connection...";
        }
      }
      else
      {
        LOG(ERROR) << "Failed to connect server. Num consecutive failures: "
                   << ++consecutive_failed_connections;
      }
    }

    LOG(INFO) << "Sleeping for " << retry_connect_server_period_.count() << " seconds";
    std::this_thread::sleep_for(retry_connect_server_period_);
  }

  return true;
}

bool SoilMoistureMonitoringClient::MonitorSoilMoisture(Client *client) {
  assert(client);

  RpiSystemContext rpiSystemContext;
  RpiI2cContext rpiI2cContext{&rpiSystemContext};
  I2cClient *i2c = rpiI2cContext.GetBus1I2cClient();
  i2c->SetSlave(I2C_ADC_SLAVE_ID);
  size_t consecutive_successful_readings = 0;

  while (true)
  {
    uint16_t soil_moisture_reading;
    if (!MeasureSoilMoisture(client, i2c))
    {
      LOG(ERROR) << "Failed to take soil moisture sensor reading";
      return false;
    }
    else
    {
      LOG(ERROR) << "Num successful soil moisture readings: "
                 << ++consecutive_successful_readings;
    }

    LOG(INFO) << "Sleeping for " << measurement_period_.count() << " seconds";
    std::this_thread::sleep_for(measurement_period_);
  }

  return true;
}

bool SoilMoistureMonitoringClient::MeasureSoilMoisture(
    Client *client,
    I2cClient *i2c)
{
  assert(client);
  assert(i2c);

  Ads1115 ads1115{i2c};
  uint16_t reading;

  for (const auto &entry : channel_to_sensor_table_)
  {
    if (!ads1115.Read(entry.first, &reading))
    {
      LOG(ERROR) << "Failed to read channel " << static_cast<int>(entry.first);
      return false;
    }

    if (!client->SendSoilMoistureMeasurement(entry.second, reading))
    {
      LOG(ERROR) << "Failed to upload soil moisture sensor reading for sensor "
                 << entry.second;
      return false;
    }
  }

  return true;
}

void SoilMoistureMonitoringClient::StealResources(SoilMoistureMonitoringClient *other) {
  ipv4_ = std::move(other->ipv4_);
  port_ = std::move(other->port_);
  cert_file_ = std::move(other->cert_file_);
  key_file_ = std::move(other->key_file_);
  ca_file_ = std::move(other->ca_file_);
  retry_connect_server_period_ = std::move(other->retry_connect_server_period_);
  measurement_period_ = std::move(other->measurement_period_);
  channel_to_sensor_table_ = std::move(other->channel_to_sensor_table_);
}

} // namespace organicdump
