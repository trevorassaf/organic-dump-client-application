#ifndef ORGANICDUMP_CLIENT_CLIENT_H
#define ORGANICDUMP_CLIENT_CLIENT_H

#include <cstdint>
#include <memory>
#include <string>

#include "ProtobufServer.h"
#include "TlsClient.h"

namespace organicdump
{

class Client
{
public:
  static bool Create(
      std::string ipv4,
      int32_t port,
      std::string cert_file,
      std::string key_file,
      std::string ca_file,
      Client *out_client);

public:
  Client();
  Client(ProtobufServer server);
  Client(Client &&other);
  Client &operator=(Client &&other);
  ~Client();

  bool SendRegisterRpi(
      std::string name,
      std::string location,
      size_t *out_rpi_id);
  bool SendRegisterSoilMoistureSensor(
      std::string name,
      std::string location,
      double floor,
      double ceiling,
      size_t *out_peripheral_id);
  bool SetPeripheralParent(size_t peripheral_id, size_t rpi_id);
  bool SendSoilMoistureMeasurement(size_t sensor_id, double measurement);

private:
  bool SendHello();
  bool HandleBasicResponse(
      size_t *out_id=nullptr,
      organicdump_proto::ErrorCode *out_error_code=nullptr,
      std::string *out_error_string=nullptr);
  void CloseResources();
  void StealResources(Client *other);

private:
  Client(const Client &other) = delete;
  Client &operator=(const Client &other) = delete;

private:
  bool is_initialized_;
  ProtobufServer server_;
};

} // namespace organicdump

#endif // ORGANICDUMP_CLIENT_CLIENT_H
