cmake_minimum_required(VERSION 3.8)
project("tangpa_sdk_client" VERSION 1.0.0.0 DESCRIPTION "Client SDK - Tangpa Robot Client Examples" LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDK config 在 lib/cmake/tangpa_sdk_client/ 下，指向 CMAKE_SOURCE_DIR 即可自动发现
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}")
find_package(tangpa_sdk_client REQUIRED)

add_subdirectory(examples)
