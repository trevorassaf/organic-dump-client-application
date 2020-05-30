#ifndef ORGANICDUMP_CLIENT_SOILMOISTUREMONITORINGCLIENT_H
#define ORGANICDUMP_CLIENT_SOILMOISTUREMONITORINGCLIENT_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "I2c/Devices/Ads1115AnalogToDigitalConverter/Ads1115.h"
#include "I2c/Devices/Ads1115AnalogToDigitalConverter/Ads1115Channel.h"
#include "I2c/I2cClient.h"

#include "Client.h"

namespace organicdump
{

class SoilMoistureMonitoringClient
{
public:
  SoilMoistureMonitoringClient(
      std::string ipv4,
      int32_t port,
      std::string cert_file,
      std::string key_file,
      std::string ca_file,
      std::chrono::seconds retry_connect_server_period,
      std::chrono::seconds measurement_period,
      std::unordered_map<I2c::Ads1115Channel, size_t> channel_to_sensor_table);
  SoilMoistureMonitoringClient(SoilMoistureMonitoringClient &&other);
  SoilMoistureMonitoringClient &operator=(SoilMoistureMonitoringClient &&other);
  ~SoilMoistureMonitoringClient();
  bool Run();

private:
  bool MonitorSoilMoisture(size_t *out_consecutive_failed_connections);
  bool MeasureSoilMoisture(Client *client, I2c::I2cClient *i2c);
  void StealResources(SoilMoistureMonitoringClient *other);

private:
  SoilMoistureMonitoringClient(const SoilMoistureMonitoringClient &other) = delete;
  SoilMoistureMonitoringClient &operator=(const SoilMoistureMonitoringClient &other) = delete;

private:
  std::string ipv4_;
  int32_t port_;
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
  std::chrono::seconds retry_connect_server_period_;
  std::chrono::seconds measurement_period_;
  std::unordered_map<I2c::Ads1115Channel, size_t> channel_to_sensor_table_;
};

} // namespace organicdump

#endif // ORGANICDUMP_CLIENT_SOILMOISTUREMONITORINGCLIENT_H
