#include <gflags/gflags.h>
#include <glog/logging.h>

#include  <openssl/bio.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>

#include <iostream>

DEFINE_bool(big_menu, true, "Include 'advanced' options in the menu listing");
DEFINE_string(languages, "english,french,german",
              "comma-separated list of languages to offer in the 'lang' menu");

int main(int argc, char **argv)
{
    google::ParseCommandLineFlags(&argc, &argv, false);

    std::cout << "Hello cmake deps" << std::endl;
    std::cout << "big_menu: " << (int)FLAGS_big_menu << std::endl;
    std::cout << "languages: " << FLAGS_languages << std::endl;

    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Test google logging" << std::endl;

    /* Initializing OpenSSL */
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    return 0;
}
