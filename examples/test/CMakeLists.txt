cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CHRIC_SDKClient_test tangpa_api_test.cpp)
target_link_libraries(CHRIC_SDKClient_test PRIVATE tangpa_sdk_client::chric_tangpa_sdk_module_api)

add_executable(CHRIC_SDKClient_benchmark tangpa_api_benchmark.cpp)
target_link_libraries(CHRIC_SDKClient_benchmark PRIVATE tangpa_sdk_client::chric_tangpa_sdk_module_api)
target_include_directories(CHRIC_SDKClient_benchmark PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(CHRIC_SDKClient_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set_target_properties(CHRIC_SDKClient_benchmark PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tangpa_api_benchmark")

# 复制 benchmark 的 data/ 到二进制旁，使 pb.txt 可被运行时发现
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR}/bin/tangpa_api_benchmark)

# 复制 config 到各二进制同级目录
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}/bin)
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}/bin/tangpa_api_benchmark)
