#ifndef ORGANICDUMP_CLICONFIG_H
#define ORGANICDUMP_CLICONFIG_H

#include <cstdint>

#include <gflags/gflags.h>
#include <glog/logging.h>

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
      std::string ca_file);

  const std::string& GetIpv4() const;
  int32_t GetPort() const;
  const std::string& GetCertFile() const;
  const std::string& GetKeyFile() const;
  const std::string& GetCaFile() const;
  

private:
  std::string ipv4_;
  int32_t port_;
  std::string cert_file_;
  std::string key_file_;
  std::string ca_file_;
};

}; // namespace organicdump

#endif // ORGANICDUMP_CLICONFIG_H
