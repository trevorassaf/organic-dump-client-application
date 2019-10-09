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

DEFINE_string(cert, "", "Certificate PEM");
DEFINE_string(key, "", "Key PEM");
DEFINE_string(ca, "", "Certificate Authority");
DEFINE_string(ipv4, "", "IPv4 address of server");
DEFINE_int32(port, -1, "Port");
DEFINE_string(message, "", "Message");

using network::TlsClientFactory;
using network::TlsClient;
using network::TlsConnection;

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

    /* Initializing OpenSSL */
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

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

    if (!SendTlsMessage(&cxn, FLAGS_message))
    {
        LOG(ERROR) << "Failed to send message to TLS server: " << FLAGS_message;
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Sent message to TLS server!";

    uint8_t read_buffer[256];
    std::string server_message;
    if (!ReadTlsMessage(
          &cxn,
          read_buffer,
          sizeof(read_buffer),
          &server_message))
    {
        LOG(ERROR) << "Failed to read TLS message from server: " << server_message;
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Received message from TLS server: " << server_message;

    return EXIT_SUCCESS;
}
