# tangpa_sdk_client CMake 配置文件
# 用法: set(CMAKE_PREFIX_PATH "/path/to/tangpa_sdk_client") 然后 find_package(tangpa_sdk_client REQUIRED)

# 防止重复加载
if(TARGET tangpa_sdk_client::all)
  return()
endif()

# 从本文件路径推算 SDK 根目录
# lib/cmake/tangpa_sdk_client/  ->  lib/  ->  tangpa_sdk_client/
get_filename_component(_SDK_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(_SDK_LIB_DIR  "${_SDK_CMAKE_DIR}/../.." ABSOLUTE)
get_filename_component(_SDK_ROOT     "${_SDK_LIB_DIR}/.." ABSOLUTE)

# 平台检测
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
  set(_ARCH "aarch64")
else()
  set(_ARCH "x86_64")
endif()

set(_SDK_ARCH_LIB_DIR            "${_SDK_LIB_DIR}/${_ARCH}")
set(_SDK_THIRD_PARTY_LIB_DIR     "${_SDK_ROOT}/third_party/lib/${_ARCH}")
set(_SDK_THIRD_PARTY_INCLUDE_DIR "${_SDK_ROOT}/third_party/include")

# ===================== 第三方 IMPORTED 目标 =====================

# -- gRPC + protobuf + abseil（聚合为一个目标，避免 70+ 个独立 target） --
file(GLOB _GRPC_ALL_LIBS "${_SDK_THIRD_PARTY_LIB_DIR}/libgrpc*.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libgpr.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libprotobuf*.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libabsl_*.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libupb_*.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libre2.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libaddress_sorting.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libutf8_range*.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libutf8_validity.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libssl.so*"
                          "${_SDK_THIRD_PARTY_LIB_DIR}/libcrypto.so*")

add_library(tangpa_sdk_client::grpc INTERFACE IMPORTED)
set_target_properties(tangpa_sdk_client::grpc PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_THIRD_PARTY_INCLUDE_DIR}/grpc"
  INTERFACE_LINK_LIBRARIES "${_GRPC_ALL_LIBS}"
)

# -- yaml-cpp --
add_library(tangpa_sdk_client::yaml_cpp SHARED IMPORTED)
set_target_properties(tangpa_sdk_client::yaml_cpp PROPERTIES
  IMPORTED_LOCATION "${_SDK_THIRD_PARTY_LIB_DIR}/libyaml-cpp.so"
)

# -- glog --
add_library(tangpa_sdk_client::glog SHARED IMPORTED)
set_target_properties(tangpa_sdk_client::glog PROPERTIES
  IMPORTED_LOCATION "${_SDK_THIRD_PARTY_LIB_DIR}/libglog.so"
)

# -- zenoh (tangpa 专用) --
add_library(tangpa_sdk_client::zenoh INTERFACE IMPORTED)
set_target_properties(tangpa_sdk_client::zenoh PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_THIRD_PARTY_INCLUDE_DIR}/zenoh"
  INTERFACE_LINK_LIBRARIES "${_SDK_THIRD_PARTY_LIB_DIR}/libzenohc.so"
)

# -- pthread / dl --
find_library(_PTHREAD_LIB pthread REQUIRED)
find_library(_DL_LIB dl REQUIRED)

# ===================== chric_* IMPORTED 目标 =====================

function(_tangpa_imported_lib NAME)
  add_library(tangpa_sdk_client::${NAME} SHARED IMPORTED)
  set_target_properties(tangpa_sdk_client::${NAME} PROPERTIES
    IMPORTED_LOCATION "${_SDK_ARCH_LIB_DIR}/lib${NAME}.so"
  )
endfunction()

_tangpa_imported_lib(chric_sdk_client)
_tangpa_imported_lib(chric_sdk_common)
_tangpa_imported_lib(chric_tangpa_sdk_module_api)
_tangpa_imported_lib(chric_protobuf_ros2)
_tangpa_imported_lib(chric_protobuf_api_service_common)
_tangpa_imported_lib(chric_protobuf_api_service_tangpa)
_tangpa_imported_lib(chric_protobuf_tangpa_ros2_interfaces)
_tangpa_imported_lib(chric_protobuf_common)
_tangpa_imported_lib(chric_config_manager)
_tangpa_imported_lib(chric_common_log)

# ===================== 设置头文件路径和传递依赖 =====================

# SDK 公共头文件（chric 命名空间）
set(_SDK_INCLUDE_DIRS
  "${_SDK_ROOT}/include"
  "${_SDK_ROOT}/include/chric"
  "${_SDK_ROOT}/include/chric/common/idl/protobuf"
)

# 运行时库搜索路径（注入用户二进制 RPATH，无需 LD_LIBRARY_PATH）
set(_SDK_RPATH "${_SDK_ARCH_LIB_DIR}:${_SDK_THIRD_PARTY_LIB_DIR}")

set_target_properties(tangpa_sdk_client::chric_sdk_client PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::chric_sdk_common;tangpa_sdk_client::grpc;tangpa_sdk_client::yaml_cpp;tangpa_sdk_client::glog"
)

set_target_properties(tangpa_sdk_client::chric_sdk_common PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
)

set_target_properties(tangpa_sdk_client::chric_tangpa_sdk_module_api PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::chric_sdk_client;tangpa_sdk_client::chric_protobuf_api_service_tangpa;tangpa_sdk_client::chric_protobuf_tangpa_ros2_interfaces;tangpa_sdk_client::chric_protobuf_common;tangpa_sdk_client::chric_protobuf_ros2;tangpa_sdk_client::chric_protobuf_api_service_common;tangpa_sdk_client::chric_config_manager;tangpa_sdk_client::chric_common_log;tangpa_sdk_client::zenoh;${_PTHREAD_LIB};${_DL_LIB}"
)

set_target_properties(tangpa_sdk_client::chric_protobuf_ros2 PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::grpc"
)
set_target_properties(tangpa_sdk_client::chric_protobuf_api_service_common PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::grpc"
)
set_target_properties(tangpa_sdk_client::chric_protobuf_api_service_tangpa PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::grpc"
)
set_target_properties(tangpa_sdk_client::chric_protobuf_tangpa_ros2_interfaces PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::grpc"
)
set_target_properties(tangpa_sdk_client::chric_protobuf_common PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::grpc"
)
set_target_properties(tangpa_sdk_client::chric_config_manager PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::yaml_cpp;tangpa_sdk_client::grpc"
)
set_target_properties(tangpa_sdk_client::chric_common_log PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${_SDK_INCLUDE_DIRS}"
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::glog"
)

# ===================== 便捷聚合目标 =====================
add_library(tangpa_sdk_client::all INTERFACE IMPORTED)
set_target_properties(tangpa_sdk_client::all PROPERTIES
  INTERFACE_LINK_LIBRARIES "tangpa_sdk_client::chric_tangpa_sdk_module_api"
)

# 清理内部变量
unset(_SDK_CMAKE_DIR)
unset(_SDK_LIB_DIR)
unset(_SDK_ROOT)
unset(_SDK_ARCH_LIB_DIR)
unset(_SDK_THIRD_PARTY_LIB_DIR)
unset(_SDK_THIRD_PARTY_INCLUDE_DIR)
unset(_SDK_INCLUDE_DIRS)
unset(_SDK_RPATH)
unset(_GRPC_ALL_LIBS)
unset(_ARCH)
unset(_PTHREAD_LIB)
unset(_DL_LIB)
