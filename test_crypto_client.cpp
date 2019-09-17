#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include <cassert>
#include <cstdlib>
#include <memory>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include  <openssl/bio.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

DEFINE_string(cert, "", "Certificate PEM");
DEFINE_string(key, "", "Key PEM");
DEFINE_string(ca, "", "Certificate Authority");
DEFINE_string(ipv4, "", "IPv4 address of server");
DEFINE_int32(port, -1, "Port");
DEFINE_string(message, "", "Message");

namespace
{
bool InitCtx(SSL_CTX **outCtx)
{
    const SSL_METHOD *method;

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    method = TLSv1_2_client_method();
    *outCtx = SSL_CTX_new(method);
    if (*outCtx == nullptr)
    {
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

bool LoadClientCerts(SSL_CTX *ctx, const char *certPath, const char *keyPath, const char *caPath)
{
    if (SSL_CTX_load_verify_locations(ctx, caPath, nullptr) != 1)
    {
        LOG(ERROR) << "Failed to load client CA file: " << caPath;
        return false;
    }

    if (SSL_CTX_use_certificate_file(ctx, certPath, SSL_FILETYPE_PEM) != 1)
    {
        LOG(ERROR) << "Failed to load client cert " << certPath;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, keyPath, SSL_FILETYPE_PEM) != 1)
    {
        LOG(ERROR) << "Failed to load private key: " << keyPath;
        return false;
    }


    if (SSL_CTX_check_private_key(ctx) != 1)
    {
        LOG(ERROR) << "Private key does not agree with cert";
        return false;
    }

/*
    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
*/
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, 1);

    return true;
}

bool ConnectServer(const char *serverIpv4, uint16_t port, uint32_t *outServer)
{
    assert(outServer);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    inet_pton(AF_INET, serverIpv4, &(addr.sin_addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    int sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        LOG(ERROR) << "Failed to initialize socket";
        return false;
    }

    if (connect(
          sd,
          reinterpret_cast<struct sockaddr*>(&addr),
          sizeof(addr)) < 0)
    {
        LOG(ERROR) << "Failed to connect server";
        close(sd);
        return false;
    }

    *outServer = sd;
    return true;
}

bool ShowCerts(SSL *ssl)
{
    X509 *cert;
    std::unique_ptr<char[]> x509Line;

    cert = SSL_get_peer_certificate(ssl);
    if (cert == nullptr)
    {
        LOG(ERROR) << "Failed to get peer certificate";
        return false;
    }

    LOG(ERROR) << "Server TLS certificate...";
    x509Line.reset(X509_NAME_oneline(X509_get_subject_name(cert), 0, 0));
    LOG(ERROR) << "- Subject name: " << x509Line.get();
    x509Line.reset(X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0));
    LOG(ERROR) << "- Issuer: " << x509Line.get();
    X509_free(cert);

    return true;
}

} // anonymous namespace

int main(int argc, char **argv)
{
    google::ParseCommandLineFlags(&argc, &argv, false);
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging(argv[0]);

    /* Initializing OpenSSL */
    ERR_load_BIO_strings();

    LOG(INFO) << "Cert: " << FLAGS_cert;
    LOG(INFO) << "Key: " << FLAGS_key;
    LOG(INFO) << "CA: " << FLAGS_ca;
    LOG(INFO) << "IPv4: " << FLAGS_ipv4;
    LOG(INFO) << "Port: " << FLAGS_port;
    LOG(INFO) << "Message: " << FLAGS_message;

    SSL_CTX *ctx = nullptr;
    SSL *ssl = nullptr;
    uint32_t serverFd = -1;
    char message[100];
    int sslReadResult = -1;

    LOG(INFO) << "Before InitCtx";

    if (!InitCtx(&ctx))
    {
        LOG(ERROR) << "Failed to initilize TLSv1.2 context";
        goto error;
    }

    LOG(INFO) << "Before LoadClientCerts";

    if (!LoadClientCerts(ctx, FLAGS_cert.c_str(), FLAGS_key.c_str(), FLAGS_ca.c_str()))
    {
        LOG(ERROR) << "Failed to load client certs";
        goto error;
    }

    LOG(INFO) << "Before ConnectServer";

    if (!ConnectServer(FLAGS_message.c_str(), FLAGS_port, &serverFd))
    {
        LOG(ERROR) << "Failed to connect server";
        goto error;
    }

    LOG(INFO) << "Client succeeded in connect() to server";

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, serverFd);
    if (SSL_connect(ssl) != 1)
    {
        LOG(ERROR) << "Failed to perform SSL handshake";
        ERR_print_errors_fp(stderr);
        goto error;
    }

    LOG(INFO) << "Client succeeded in SSL_connect()";

    /*
    if (!ShowCerts(ssl))
    {
        LOG(ERROR) << "Failed to print peer cert";
        goto error;
    }

    LOG(INFO) << "Client succeeded in ShowCerts()";
    */

    if (SSL_write(ssl, FLAGS_message.c_str(), strlen(FLAGS_message.c_str())) < 1)
    {
        LOG(ERROR) << "Failed to send message";
        goto error;
    }

    LOG(INFO) << "Client succeeded in write()";

    memset(message, 0, sizeof(message));
    sslReadResult = SSL_read(ssl, message, sizeof(message));
    if (sslReadResult < 1)
    {
        LOG(ERROR) << "Failed to read message";
        goto error;
    }

    LOG(INFO) << "Server message: " << message;
    return EXIT_SUCCESS;

error:
    SSL_free(ssl);
    close(serverFd);
    SSL_CTX_free(ctx);
    return EXIT_FAILURE;
}
