#ifndef ORGANICDUMP_CLICONFIG_H
#define ORGANICDUMP_CLICONFIG_H

#include <cstdint>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "organic_dump.pb.h"

namespace organicdump
{

class CliConfig
{
public:
  static bool Parse(int argc, char **argv, CliConfig *out_config);

public:
  CliConfig();
  CliConfig(
      std::string ipv4,
      int32_t port,
      std::string cert_file,
      std::string key_file,
      std::string ca_file,
      bool has_action,
      organicdump_proto::MessageType server_action,
      std::string name,
      std::string location,
      int id,
      int parent_id,
      double floor,
      double ceiling,
      double measurement);

  const std::string& GetIpv4() const;
  int32_t GetPort() const;
  const std::string& GetCertFile() const;
  const std::string& GetKeyFile() const;
  const std::string& GetCaFile() const;
  bool HasAction() const;
  organicdump_proto::MessageType GetServerAction() const;
  bool HasName() const;
  const std::string &GetName() const;
  bool HasLocation() const;
  const std::string &GetLocation() const;
  bool HasId() const;
  size_t GetId() const;
  bool HasParentId() const;
  size_t GetParentId() const;
  bool HasFloor() const;
  double GetFloor() const;
  bool HasCeiling() const;
  double GetCeiling() const;
  bool HasMeasurement() const;
  double GetMeasurement() const;

private:
  std::string ipv4_;
  int32_t port_;
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
  bool has_action_;
  organicdump_proto::MessageType server_action_;
  std::string name_;
  std::string location_;
  int id_;
  int parent_id_;
  double floor_;
  double ceiling_;
  double measurement_;
};

}; // namespace organicdump

#endif // ORGANICDUMP_CLICONFIG_H
