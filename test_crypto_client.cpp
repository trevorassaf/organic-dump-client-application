#include <cassert>
#include <cstdlib>
#include <memory>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <TlsClientFactory.h>
#include <TlsClient.h>
#include <TlsConnection.h>
#include <TlsUtilities.h>

#include <test.pb.h>

DEFINE_string(cert, "", "Certificate PEM");
DEFINE_string(key, "", "Key PEM");
DEFINE_string(ca, "", "Certificate Authority");
DEFINE_string(ipv4, "", "IPv4 address of server");
DEFINE_int32(port, -1, "Port");
DEFINE_string(message, "", "Message");

using network::TlsClientFactory;
using network::TlsClient;
using network::TlsConnection;
using network::ProtobufMessageHeader;

int main(int argc, char **argv)
{
    google::ParseCommandLineFlags(&argc, &argv, false);
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging(argv[0]);

    LOG(INFO) << "Cert: " << FLAGS_cert;
    LOG(INFO) << "Key: " << FLAGS_key;
    LOG(INFO) << "CA: " << FLAGS_ca;
    LOG(INFO) << "IPv4: " << FLAGS_ipv4;
    LOG(INFO) << "Port: " << FLAGS_port;
    LOG(INFO) << "Message: " << FLAGS_message;

    network::InitOpenSslForProcess();
    TlsClient client;
    TlsClientFactory factory;

    if (!factory.Create(
        FLAGS_ipv4,
        FLAGS_port,
        FLAGS_cert,
        FLAGS_key,
        FLAGS_ca,
        &client))
    {
        LOG(ERROR) << "Failed to create TlsClient";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "After factory.Create()";

    TlsConnection cxn;
    if (!client.Connect(&cxn))
    {
        LOG(ERROR) << "TlsClient failed to connect to server";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "client.Connect()";

    test_message::MessageType basic_str_type = test_message::MessageType::BASIC_STRING;
    test_message::BasicStringMsg basic_str;
    basic_str.set_str(FLAGS_message);

    LOG(INFO) << "BasicStringMsg.str: " << basic_str.str();

    bool cxn_closed = false;
    if (!SendTlsProtobufMessage(
            &cxn,
            static_cast<uint8_t>(basic_str_type),
            &basic_str,
            &cxn_closed))
    {
        LOG(ERROR) << "Failed to send TLS protobuf message";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Sent message to TLS server!";

    ProtobufMessageHeader header;
    if (!ReadTlsProtobufMessageHeader(&cxn, &header))
    {
        LOG(ERROR) << "Failed to read TLS protobuf header";
        return EXIT_FAILURE;
    }

    test_message::BasicStringMsg msg;
    auto buffer = std::make_unique<uint8_t[]>(header.size);

    if (!ReadTlsProtobufMessageBody(
            &cxn,
            buffer.get(),
            header.size,
            &msg)) 
    {
        LOG(ERROR) << "Failed to read TLS protobuf message body";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Message from server: " << msg.str();

    return EXIT_SUCCESS;
}
