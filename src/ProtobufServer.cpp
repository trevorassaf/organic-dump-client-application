#include "ProtobufServer.h"

#include "organic_dump.pb.h"

#include "Fd.h"
#include "OrganicDumpProtoMessage.h"
#include "TlsConnection.h"

namespace
{
using network::Fd;
using network::TlsConnection;
} // namespace

namespace organicdump
{

ProtobufServer::ProtobufServer() : is_initialized_{false} {}

ProtobufServer::ProtobufServer(TlsConnection cxn)
  : is_initialized_{true},
    cxn_{std::move(cxn)} {}

ProtobufServer::ProtobufServer(ProtobufServer&& other)
{
  StealResources(&other);
}

ProtobufServer &ProtobufServer::operator=(ProtobufServer&& other)
{
  if (this != &other)
  {
    CloseResources();
    StealResources(&other);
  }
  return *this;
}

ProtobufServer::~ProtobufServer()
{
  CloseResources();
}

bool ProtobufServer::Read(OrganicDumpProtoMessage *out_msg, bool *out_cxn_closed)
{
  assert(out_msg);

  bool cxn_closed = false;
  if (!ReadTlsProtobufMessage(
        &cxn_,
        out_msg,
        &cxn_closed))
  {
    LOG(ERROR) << "Failed to read TLS protobuf message";
    return false;
  }

  if (out_cxn_closed)
  {
    *out_cxn_closed = cxn_closed;
  }

  return true;
}

bool ProtobufServer::Write(OrganicDumpProtoMessage *msg, bool *out_cxn_closed)
{
  assert(msg);

  bool cxn_closed = false;
  if (!SendTlsProtobufMessage(
        &cxn_,
        msg,
        &cxn_closed))
  {
    LOG(ERROR) << "Failed to write TLS protobuf message";
    return false;
  }

  if (out_cxn_closed)
  {
    *out_cxn_closed = cxn_closed;
  }

  return true;
}

const Fd &ProtobufServer::GetFd() const
{
  return cxn_.GetFd();
}

void ProtobufServer::CloseResources()
{
  if (!is_initialized_)
  {
    return;
  }

  is_initialized_ = false;
  cxn_ = TlsConnection{};
}

void ProtobufServer::StealResources(ProtobufServer *other)
{
  assert(other);
  is_initialized_ = other->is_initialized_;
  other->is_initialized_ = false;
  cxn_ = std::move(other->cxn_);
}

} // namespace organicdump
