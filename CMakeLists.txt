cmake_minimum_required(VERSION 3.1)

include_directories(
  ../../organic-dump-network/repo
  ../../organic-dump-protocols/repo
  ../../gpio14/repo/include
  ../../../external/protobuf/repo/src
  ../../../external/boringssl/repo/include
  ../../../external/jsoncpp/repo/include
)

add_executable(test_crypto_client examples/test_crypto_client.cpp)
target_link_libraries(test_crypto_client gflags::gflags)
target_link_libraries(test_crypto_client glog::glog)
target_link_libraries(test_crypto_client ssl crypto)
target_link_libraries(test_crypto_client organic_dump_network)
target_link_libraries(test_crypto_client gpio14)
target_link_libraries(test_crypto_client test_proto)

add_executable(organic_dump_client
  src/main.cpp
  src/Client.cpp
  src/CliConfig.cpp
  src/ProtobufServer.cpp)

target_link_libraries(organic_dump_client gflags::gflags)
target_link_libraries(organic_dump_client glog::glog)
target_link_libraries(organic_dump_client ssl crypto)
target_link_libraries(organic_dump_client organic_dump_network)
target_link_libraries(organic_dump_client organic_dump_proto)
target_link_libraries(organic_dump_client gpio14)

add_executable(organic_dump_pot_monitor_client
  src/monitor_soil_moisture_main.cpp
  src/Client.cpp
  src/CliConfig.cpp
  src/ProtobufServer.cpp)

target_link_libraries(organic_dump_pot_monitor_client gflags::gflags)
target_link_libraries(organic_dump_pot_monitor_client glog::glog)
target_link_libraries(organic_dump_pot_monitor_client ssl crypto)
target_link_libraries(organic_dump_pot_monitor_client organic_dump_network)
target_link_libraries(organic_dump_pot_monitor_client organic_dump_proto)
target_link_libraries(organic_dump_pot_monitor_client gpio14)
target_link_libraries(organic_dump_pot_monitor_client jsoncpp_lib)
