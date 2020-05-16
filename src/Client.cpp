#include "Client.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "NetworkUtilities.h"
#include "OrganicDumpProtoMessage.h"
#include "ProtobufServer.h"
#include "TlsClient.h"
#include "TlsClientFactory.h"
#include "TlsConnection.h"

namespace
{
using organicdump_proto::BasicResponse;
using organicdump_proto::ClientType;
using organicdump_proto::ErrorCode;
using organicdump_proto::Hello;
using organicdump_proto::MessageType;
using organicdump_proto::PeripheralMeta;
using organicdump_proto::RegisterRpi;
using organicdump_proto::RegisterSoilMoistureSensor;
using organicdump_proto::UpdatePeripheralOwnership;
using network::WaitPolicy;
using network::TlsClient;
using network::TlsClientFactory;
using network::TlsConnection;
} // namespace

namespace organicdump
{

bool Client::Create(
    std::string ipv4,
    int32_t port,
    std::string cert_file,
    std::string key_file,
    std::string ca_file,
    Client *out_client)
{
  assert(out_client);

  TlsClientFactory client_factory;
  TlsClient tls_client;
  if (!client_factory.Create(
          std::move(ipv4),
          port,
          std::move(cert_file),
          std::move(key_file),
          std::move(ca_file),
          WaitPolicy::BLOCKING,
          &tls_client))
  {
    LOG(ERROR) << "Failed to initialize TLS client";
    return false;
  }

  TlsConnection cxn;
  if (!tls_client.Connect(&cxn))
  {
    LOG(ERROR) << "Failed to connect to server";
    return false;
  }

  ProtobufServer server_proxy{std::move(cxn)};
  Client client{std::move(server_proxy)};

  if (!client.SendHello())
  {
    LOG(ERROR) << "Failed to send hello to server";
    return false;
  }

  *out_client = std::move(client);

  return true;
}

Client::Client() : is_initialized_{false} {}

Client::Client(ProtobufServer server)
  : is_initialized_{true},
    server_{std::move(server)} {}

Client::Client(Client &&other)
{
  StealResources(&other);
}

Client &Client::operator=(Client &&other)
{
  if (this != &other) {
    StealResources(&other);
  }
  return *this;
}

Client::~Client()
{
  CloseResources();
}

bool Client::SendRegisterRpi(
    std::string name,
    std::string location,
    size_t *out_rpi_id)
{
  assert(out_rpi_id);

  LOG(ERROR) << "bozkurtus -- Client::RegisterRpi() -- call";

  organicdump_proto::RegisterRpi register_rpi_msg;
  register_rpi_msg.set_name(std::move(name));
  register_rpi_msg.set_location(std::move(location));

  OrganicDumpProtoMessage msg{std::move(register_rpi_msg)};

  if (!server_.Write(&msg))
  {
    LOG(ERROR) << "Failed to send RegisterRpi message to server";
    return false;
  }

  if (!HandleBasicResponse(out_rpi_id))
  {
    LOG(ERROR) << "Failed on BASIC_RESPONSE for REGISTER_RPI";
    return false;
  }

  LOG(ERROR) << "bozkurtus -- Client::RegisterRpi() -- end";
  return true;
}

bool Client::SendRegisterSoilMoistureSensor(
    std::string name,
    std::string location,
    double floor,
    double ceiling,
    size_t *out_peripheral_id)
{
  assert(out_peripheral_id);

  LOG(INFO) << "Register soil moisture sensor: name=" << name;

  PeripheralMeta meta;
  meta.set_name(name);
  meta.set_location(location);

  RegisterSoilMoistureSensor register_sensor_req;
  register_sensor_req.set_allocated_meta(&meta);
  register_sensor_req.set_floor(floor);
  register_sensor_req.set_ceil(ceiling);
  OrganicDumpProtoMessage req_msg{std::move(register_sensor_req)};

  if (!server_.Write(&req_msg))
  {
    LOG(ERROR) << "Failed to send REGISTER_SOIL_MOISTURE_SENSOR  message to server";
    return false;
  }

  if (!HandleBasicResponse(out_peripheral_id))
  {
    LOG(ERROR) << "Failed on BASIC_RESPONSE for REGISTER_SOIL_MOISTURE_SENSOR";
    return false;
  }

  return true;
}

bool Client::SetPeripheralParent(size_t peripheral_id, size_t rpi_id)
{
  UpdatePeripheralOwnership update_req;
  update_req.set_peripheral_id(peripheral_id);
  update_req.set_rpi_id(rpi_id);
  update_req.set_orphan_peripheral(false);
  OrganicDumpProtoMessage msg{std::move(update_req)};

  if (!server_.Write(&msg))
  {
    LOG(ERROR) << "Failed to send UPDATE_PERIPHERAL_OWNERSHIP message";
    return false;
  }

  if (!HandleBasicResponse())
  {
    LOG(ERROR) << "Failed to read BASIC_RESPONSE for UPDATE_PERIPHERAL_OWNERSHIP";
    return false;
  }

  return true;
}

bool Client::SendSoilMoistureMeasurement(size_t sensor_id, double measurement)
{
  LOG(INFO) << "Soil Moisture Measurement: sensor_id="
            << sensor_id << ", measurement=" << measurement;

  organicdump_proto::SendSoilMoistureMeasurement req;
  req.set_sensor_id(sensor_id);
  req.set_value(measurement);
  OrganicDumpProtoMessage msg{std::move(req)};

  if (!server_.Write(&msg))
  {
    LOG(ERROR) << "Failed to send SEND_SOIL_MOISTURE_MEASUREMENT message";
    return false;
  }

  size_t measurement_id;
  if (!HandleBasicResponse(&measurement_id))
  {
    LOG(ERROR) << "Failed to read BASIC_RESPONSE for UPDATE_PERIPHERAL_OWNERSHIP";
    return false;
  }

  LOG(INFO) << "Id of soil moisture measurement is " << measurement_id;
  return true;
}

bool Client::SendHello()
{
  LOG(ERROR) << "bozkurtus -- Client::SendHello() -- call";

  Hello hello_msg;
  hello_msg.set_type(ClientType::CONTROL);
  OrganicDumpProtoMessage msg{std::move(hello_msg)};

  if (!server_.Write(&msg)) {
    LOG(ERROR) << "Failed to send HELLO message to server";
    return false;
  }

  LOG(ERROR) << "bozkurtus -- Client::SendHello() -- end";
  return true;
}

bool Client::HandleBasicResponse(
    size_t *out_id,
    organicdump_proto::ErrorCode *out_error_code,
    std::string *out_error_string)
{
  OrganicDumpProtoMessage resp;
  if (!server_.Read(&resp))
  {
    LOG(ERROR) << "Failed to read message from server. Expecting BASIC_RESPONSE";
    return false;
  }

  if (resp.type != MessageType::BASIC_RESPONSE)
  {
    LOG(ERROR) << "Received unexpected message type "
               << organicdump_proto::MessageType_Name(resp.type);
    return false;
  }

  const BasicResponse &basic_response = resp.basic_response;
  if (out_id && !basic_response.has_id())
  {
    LOG(ERROR) << "BASIC_RESPONSE message is missing its |id| field";
    return false;
  }

  LOG(INFO) << "Received basic response with ID: " << basic_response.id();

  if (out_id)
  {
    *out_id = basic_response.id();
  }

  if (out_error_code)
  {
    *out_error_code = basic_response.code();
  }

  if (out_error_string && basic_response.has_message())
  {
    *out_error_string = basic_response.message();
  }

  return true;
}

void Client::CloseResources()
{
  is_initialized_ = false;
  server_ = ProtobufServer{};
}

void Client::StealResources(Client *other)
{
  assert(other);
  is_initialized_ = other->is_initialized_;
  other->is_initialized_ = false;
  server_ = std::move(other->server_);
}

} // namespace organicdump
