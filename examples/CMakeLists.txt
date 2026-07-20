cmake_minimum_required(VERSION 3.8)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include_directories(
#     ${CMAKE_SOURCE_DIR}/include
#     ${CMAKE_SOURCE_DIR}/include/protobuf
# )

# link_directories(${CMAKE_SOURCE_DIR}/lib)
add_subdirectory(test)
add_subdirectory(control_test_gui)
add_subdirectory(system_test_gui)

