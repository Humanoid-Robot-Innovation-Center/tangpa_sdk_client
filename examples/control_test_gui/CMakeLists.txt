cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(tangpa_sdk_client REQUIRED)

add_executable(control_test_gui control_test_gui.cpp)
target_link_libraries(control_test_gui PRIVATE tangpa_sdk_client::chric_tangpa_sdk_module_api)
target_include_directories(control_test_gui PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(control_test_gui PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/control_test_gui")

# 复制 data/ 到二进制旁，使 pb.txt 可被运行时发现
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR}/bin/control_test_gui)

# 复制 config 到二进制同级目录
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR}/bin/control_test_gui)
