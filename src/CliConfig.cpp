#include "CliConfig.h"

#include <fstream>
#include <iostream>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

namespace
{

using organicdump_proto::MessageType;

constexpr int UNSET_CLI_INT = -1;
constexpr double UNSET_CLI_DOUBLE = -1E6;

const std::unordered_map<std::string, MessageType> SERVER_ACTION_MAP =
{
  {"register_rpi", MessageType::REGISTER_RPI},
  {"register_soil_moisture_sensor", MessageType::REGISTER_SOIL_MOISTURE_SENSOR},
  {"set_peripheral_ownership", MessageType::UPDATE_PERIPHERAL_OWNERSHIP},
  {"send_soil_moisture_measurement", MessageType::SEND_SOIL_MOISTURE_MEASUREMENT},
};

bool FailUnsetCliInt(const char *param, int32_t port)
{
    if (port == UNSET_CLI_INT)
    {
        LOG(ERROR) << "--" << param << " must be set";
        return false;
    }
    return true;
}

bool CheckFileExists(const char *param, const std::string &file_name)
{
    std::ifstream file{file_name};
    if (!file.is_open())
    {
        LOG(ERROR) << "--" << param << " must specify valid file: " << file_name;
        return false;
    }
    return true;
}

bool CheckNonEmptyString(const char *param, const std::string &value) {
  if (value.empty()) {
    LOG(ERROR) << "CLI param --" << param << " cannot be empty";
    return false;
  }

  return true;
}

bool IsCliIntSet(int value)
{
  return value != UNSET_CLI_INT;
}

bool IsCliDoubleSet(double value)
{
  return value != UNSET_CLI_DOUBLE;
}

bool ParseAction(const std::string &action_str, MessageType *out_action)
{
  assert(out_action);

  if (SERVER_ACTION_MAP.count(action_str) == 0)
  {
    LOG(ERROR) << "Unknown server action: " << action_str;
    return false;
  }

  *out_action = SERVER_ACTION_MAP.at(action_str);
  return true;
}

DEFINE_string(ipv4, "", "Ipv4 address");
DEFINE_int32(port, UNSET_CLI_INT, "Port");
DEFINE_string(cert, "", "Certificate file");
DEFINE_string(key, "", "Private key file");
DEFINE_string(ca, "", "CA file");
DEFINE_string(action, "", "Server action");
DEFINE_string(name, "", "Entity name");
DEFINE_string(location, "", "Entity location");
DEFINE_int32(id, UNSET_CLI_INT, "Entity id");
DEFINE_int32(parent_id, UNSET_CLI_INT, "Parent entity id");
DEFINE_double(floor, UNSET_CLI_DOUBLE, "Floor of sensor");
DEFINE_double(ceiling, UNSET_CLI_DOUBLE, "Ceiling of sensor");
DEFINE_double(measurement, UNSET_CLI_DOUBLE, "Measurement for sensors");

DEFINE_validator(ipv4, CheckNonEmptyString);
DEFINE_validator(port, FailUnsetCliInt);
DEFINE_validator(cert, CheckFileExists);
DEFINE_validator(key, CheckFileExists);
DEFINE_validator(ca, CheckFileExists);
} // namespace

namespace organicdump
{

bool CliConfig::Parse(int argc, char **argv, CliConfig *out_config)
{
  std::cout << "bozkutus -- CliConfig::Parse() -- call" << std::endl;

  // Force glog to write to stderr
  FLAGS_logtostderr = 1;
  google::ParseCommandLineFlags(&argc, &argv, false);

  MessageType action;
  bool has_action = FLAGS_action != "";
  if (has_action && !ParseAction(FLAGS_action, &action))
  {
    LOG(ERROR) << "Failed to parse action";
    return false;
  }

  *out_config = CliConfig{
      FLAGS_ipv4,
      FLAGS_port,
      FLAGS_cert,
      FLAGS_key,
      FLAGS_ca,
      has_action,
      action,
      FLAGS_name,
      FLAGS_location,
      FLAGS_id,
      FLAGS_parent_id,
      FLAGS_floor,
      FLAGS_ceiling,
      FLAGS_measurement};

  return true; 
}

CliConfig::CliConfig() {}

CliConfig::CliConfig(
    std::string ipv4,
    int32_t port,
    std::string cert_file,
    std::string key_file,
    std::string ca_file,
    bool has_action,
    MessageType server_action,
    std::string name,
    std::string location,
    int id,
    int parent_id,
    double floor,
    double ceiling,
    double measurement)
  : ipv4_{std::move(ipv4)},
    port_{port},
    cert_file_{std::move(cert_file)},
    key_file_{std::move(key_file)},
    ca_file_{std::move(ca_file)},
    has_action_{has_action},
    server_action_{server_action},
    name_{std::move(name)},
    location_{std::move(location)},
    id_{id},
    parent_id_{parent_id},
    floor_{floor},
    ceiling_{ceiling},
    measurement_{measurement}
{}

const std::string& CliConfig::GetIpv4() const
{
  return ipv4_;
}

int32_t CliConfig::GetPort() const
{
  return port_;
}

const std::string& CliConfig::GetCertFile() const
{
  return cert_file_;
}

const std::string& CliConfig::GetKeyFile() const
{
  return key_file_;
}

const std::string& CliConfig::GetCaFile() const
{
  return ca_file_;
}

bool CliConfig::HasAction() const
{
  return has_action_;
}

MessageType CliConfig::GetServerAction() const
{
  return server_action_;
}

bool CliConfig::HasName() const
{
  return !name_.empty();
}

const std::string &CliConfig::GetName() const
{
  return name_;
}

bool CliConfig::HasLocation() const
{
  return !location_.empty();
}

const std::string &CliConfig::GetLocation() const
{
  return location_;
}

bool CliConfig::HasId() const
{
  return IsCliIntSet(id_);
}

size_t CliConfig::GetId() const
{
  assert(IsCliIntSet(id_));
  return static_cast<size_t>(id_);
}

bool CliConfig::HasParentId() const
{
  return IsCliIntSet(parent_id_);
}

size_t CliConfig::GetParentId() const
{
  assert(IsCliIntSet(parent_id_));
  return static_cast<size_t>(parent_id_);
}

bool CliConfig::HasFloor() const
{
  return IsCliDoubleSet(floor_);
}

double CliConfig::GetFloor() const
{
  return floor_;
}

bool CliConfig::HasCeiling() const
{
  return IsCliDoubleSet(ceiling_);
}

double CliConfig::GetCeiling() const
{
  return ceiling_;
}

bool CliConfig::HasMeasurement() const
{
  return IsCliDoubleSet(measurement_);
}

double CliConfig::GetMeasurement() const
{
  return measurement_;
}

}; // namespace organicdump

