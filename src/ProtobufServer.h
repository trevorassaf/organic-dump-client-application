#ifndef ORGANICDUMP_SERVER_PROTOBUFSERVER_H
#define ORGANICDUMP_SERVER_PROTOBUFSERVER_H

#include "organic_dump.pb.h"

#include "Fd.h"
#include "OrganicDumpProtoMessage.h"
#include "TlsConnection.h"

namespace organicdump
{

class ProtobufServer
{
public:
  ProtobufServer();
  ProtobufServer(network::TlsConnection cxn);
  ProtobufServer(ProtobufServer&& other);
  ProtobufServer &operator=(ProtobufServer&& other);
  ~ProtobufServer();
  bool Read(OrganicDumpProtoMessage *out_msg, bool *out_cxn_closed=nullptr);
  bool Write(OrganicDumpProtoMessage *msg, bool *out_cxn_closed=nullptr);
  const network::Fd &GetFd() const;

private:
  void CloseResources();
  void StealResources(ProtobufServer *other);

private:
  ProtobufServer(const ProtobufServer& other);
  ProtobufServer &operator=(const ProtobufServer& other);

private:
  bool is_initialized_;
  network::TlsConnection cxn_;
};

} // namespace organicdump

#endif // ORGANICDUMP_SERVER_PROTOBUFSERVER_H
