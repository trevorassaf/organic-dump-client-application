#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

#include  <openssl/bio.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Client.h"
#include "CliConfig.h"

#include "organic_dump.pb.h"

#include "I2c/I2cException.h"
#include "I2c/I2cClient.h"
#include "I2c/RpiI2cContext.h"
#include "System/RpiSystemContext.h"

namespace
{
using organicdump::Client;
using organicdump::CliConfig;

using I2c::I2cException;
using I2c::I2cClient;
using I2c::RpiI2cContext;
using System::RpiSystemContext;

void InitLibraries(const char *app_name)
{
  google::InitGoogleLogging(app_name);

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ERR_load_BIO_strings();
}

bool PerformServerAction(
    const CliConfig &config,
    Client *client)
{
  assert(client);

  LOG(ERROR) << "Performing server action: "
             << organicdump_proto::MessageType_Name(config.GetServerAction());

  size_t id;
  switch (config.GetServerAction())
  {
    case organicdump_proto::MessageType::REGISTER_RPI:
      assert(config.HasName());
      assert(config.HasLocation());

      return client->SendRegisterRpi(
          config.GetName(),
          config.GetLocation(),
          &id);

    case organicdump_proto::MessageType::REGISTER_SOIL_MOISTURE_SENSOR:
      assert(config.HasName());
      assert(config.HasLocation());
      assert(config.HasFloor());
      assert(config.HasCeiling());

      return client->SendRegisterSoilMoistureSensor(
          config.GetName(),
          config.GetLocation(),
          config.GetFloor(),
          config.GetCeiling(),
          &id);

    case organicdump_proto::MessageType::UPDATE_PERIPHERAL_OWNERSHIP:
      assert(config.HasId());
      assert(config.HasParentId());

      return client->SetPeripheralParent(
          config.GetId(),
          config.GetParentId());

    case organicdump_proto::MessageType::SEND_SOIL_MOISTURE_MEASUREMENT:
      assert(config.HasId());
      assert(config.HasMeasurement());

      return client->SendSoilMoistureMeasurement(
          config.GetId(),
          config.GetMeasurement());

    // TODO(bozkurtus): add more server action handlers
    default:
      LOG(ERROR) << "Unsupported server action";
      return false;
  }
}

} // anonymous namespace

int main(int argc, char **argv)
{
  // Run I2C Program to query soil moisture sensor
  RpiSystemContext rpiSystemContext;
  RpiI2cContext rpiI2cContext{&rpiSystemContext};
  
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

  if (!PerformServerAction(config, &client))
  {
    LOG(ERROR) << "Failed to perform server action";
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}
