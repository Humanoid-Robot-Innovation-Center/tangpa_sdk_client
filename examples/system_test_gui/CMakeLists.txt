cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(tangpa_sdk_client REQUIRED)

add_executable(system_test_gui system_test_gui.cpp)
target_link_libraries(system_test_gui PRIVATE tangpa_sdk_client::chric_tangpa_sdk_module_api)

set_target_properties(system_test_gui PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 复制 config 到二进制同级目录
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}/bin)
