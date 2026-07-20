# ============================================================
# 项目配置
# ============================================================
cmake_minimum_required(VERSION 3.8)

set(TARGET_NAME chric_protobuf_api_service_tangpa)
project(${TARGET_NAME}
    VERSION 1.0.0
    DESCRIPTION "CHRIC框架的TangpaProtobuf定义"
    LANGUAGES CXX
)

# ============================================================
# 编译器设置
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# 环境配置
# ============================================================
# 包含framework环境配置
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../cmake/Env.cmake)

# ============================================================
# 依赖管理
# ============================================================
find_package(ament_cmake REQUIRED)
find_package(chric_protobuf_ros2 REQUIRED)
find_package(chric_protobuf_common REQUIRED)
find_package(chric_protobuf_tangpa_ros2_interfaces REQUIRED)

# ============================================================
# 路径配置  
# ============================================================
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(SUBMODULE_NAME "api_service/tangpa")
set(SUBMODULE_SOURCE_DIR "${SOURCE_DIR}")
set(SUBMODULE_INCLUDE_DIR "${INCLUDE_DIR}/${SUBMODULE_NAME}")

# ============================================================
# 源文件收集
# ============================================================
file(GLOB_RECURSE SUBMODULE_SOURCES 
    "${SUBMODULE_SOURCE_DIR}/*.cc"
    "${SUBMODULE_SOURCE_DIR}/*.cpp"
)

# ============================================================
# 库目标配置
# ============================================================
if(SUBMODULE_SOURCES)
    # 创建库
    add_library(${TARGET_NAME} SHARED ${SUBMODULE_SOURCES})
    
    # 目标属性
    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${TARGET_NAME}"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        POSITION_INDEPENDENT_CODE ON
    )
    
    # 包含目录
    target_include_directories(${TARGET_NAME} 
        PUBLIC 
            $<BUILD_INTERFACE:${INCLUDE_DIR}>
            $<BUILD_INTERFACE:${SUBMODULE_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../base/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${GRPC_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${PROTOBUF_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include/common/idl/protobuf>
            $<INSTALL_INTERFACE:include/common/idl/protobuf/${SUBMODULE_NAME}>
    )

    # 添加ROS2依赖
    ament_target_dependencies(${TARGET_NAME}
        chric_protobuf_ros2
        chric_protobuf_common
        chric_protobuf_tangpa_ros2_interfaces
    )

    # 链接目录
    target_link_directories(${TARGET_NAME}
        PRIVATE
            ${GRPC_LIB_DIR}
            ${PROTOBUF_LIB_DIR}
    )
    
    # 链接库
    target_link_libraries(${TARGET_NAME}
        ${GRPC_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
    )
    
    message(STATUS "Created Protobuf library: ${TARGET_NAME}")
    
    # ============================================================
    # 安装配置
    # ============================================================
    # 安装库
    install(TARGETS ${TARGET_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/common/idl/protobuf
    )
    
    # 安装头文件
    install(DIRECTORY ${SUBMODULE_INCLUDE_DIR}/
        DESTINATION include/common/idl/protobuf/${SUBMODULE_NAME}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # 安装生成的protobuf头文件
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/${SUBMODULE_NAME}/
        DESTINATION include/common/idl/protobuf/${SUBMODULE_NAME}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # 安装base头文件
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../base/include/
        DESTINATION include/common/idl/protobuf/${SUBMODULE_NAME}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # 安装目标导出文件
    install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE chric::
        DESTINATION lib/cmake/${PROJECT_NAME}
    )
    
    # ============================================================
    # ROS2导出配置
    # ============================================================
    ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
    ament_export_dependencies(ament_cmake)
    ament_export_include_directories(include/common/idl/protobuf)
    ament_export_include_directories(include/common/idl/protobuf/${SUBMODULE_NAME})
        
else()
    message(WARNING "No source files found for api_service_tangpa protobuf module")
endif()

# ============================================================
# ROS2包设置
# ============================================================
ament_package()