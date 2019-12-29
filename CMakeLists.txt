cmake_minimum_required(VERSION 3.1)

include_directories(
  ../../organic-dump-network/repo
  ../../organic-dump-protocols/repo
  ../../../external/protobuf/repo/src
  ../../../external/boringssl/repo/include
)

add_executable(test_crypto_client test_crypto_client.cpp)
target_link_libraries(test_crypto_client gflags::gflags)
target_link_libraries(test_crypto_client glog::glog)
target_link_libraries(test_crypto_client ssl crypto)
target_link_libraries(test_crypto_client organic_dump_network)
target_link_libraries(test_crypto_client test_proto)
