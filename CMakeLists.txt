cmake_minimum_required(VERSION 3.1)

add_executable(test_crypto_client test_crypto_client.cpp)
target_link_libraries(test_crypto_client gflags::gflags)
target_link_libraries(test_crypto_client glog::glog)
target_link_libraries(test_crypto_client ssl crypto)
