#ifndef PB_EXPORT_H
#define PB_EXPORT_H

// PB 模块导出符号定义
// 此文件由 batch_generate.sh 自动生成，请勿手动修改
// 模板：base/pb_export.h.in

// Windows 平台
#ifdef _WIN32
#ifdef chric_protobuf_api_service_common_EXPORTS
#define chric_protobuf_api_service_common __declspec(dllexport)
#else
#define chric_protobuf_api_service_common __declspec(dllimport)
#endif
#ifdef chric_protobuf_api_service_konka_EXPORTS
#define chric_protobuf_api_service_konka __declspec(dllexport)
#else
#define chric_protobuf_api_service_konka __declspec(dllimport)
#endif
#ifdef chric_protobuf_api_service_tangpa_EXPORTS
#define chric_protobuf_api_service_tangpa __declspec(dllexport)
#else
#define chric_protobuf_api_service_tangpa __declspec(dllimport)
#endif
#ifdef chric_protobuf_common_EXPORTS
#define chric_protobuf_common __declspec(dllexport)
#else
#define chric_protobuf_common __declspec(dllimport)
#endif
#ifdef chric_protobuf_communication_EXPORTS
#define chric_protobuf_communication __declspec(dllexport)
#else
#define chric_protobuf_communication __declspec(dllimport)
#endif
#ifdef chric_protobuf_konka_ros2_interfaces_EXPORTS
#define chric_protobuf_konka_ros2_interfaces __declspec(dllexport)
#else
#define chric_protobuf_konka_ros2_interfaces __declspec(dllimport)
#endif
#ifdef chric_protobuf_ros2_EXPORTS
#define chric_protobuf_ros2 __declspec(dllexport)
#else
#define chric_protobuf_ros2 __declspec(dllimport)
#endif
#ifdef chric_protobuf_tangpa_ros2_interfaces_EXPORTS
#define chric_protobuf_tangpa_ros2_interfaces __declspec(dllexport)
#else
#define chric_protobuf_tangpa_ros2_interfaces __declspec(dllimport)
#endif


#else
// Linux/Unix 平台
#if __GNUC__ >= 4
#ifdef chric_protobuf_api_service_common_EXPORTS
#define chric_protobuf_api_service_common __attribute__((visibility("default")))
#else
#define chric_protobuf_api_service_common __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_api_service_konka_EXPORTS
#define chric_protobuf_api_service_konka __attribute__((visibility("default")))
#else
#define chric_protobuf_api_service_konka __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_api_service_tangpa_EXPORTS
#define chric_protobuf_api_service_tangpa __attribute__((visibility("default")))
#else
#define chric_protobuf_api_service_tangpa __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_common_EXPORTS
#define chric_protobuf_common __attribute__((visibility("default")))
#else
#define chric_protobuf_common __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_communication_EXPORTS
#define chric_protobuf_communication __attribute__((visibility("default")))
#else
#define chric_protobuf_communication __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_konka_ros2_interfaces_EXPORTS
#define chric_protobuf_konka_ros2_interfaces __attribute__((visibility("default")))
#else
#define chric_protobuf_konka_ros2_interfaces __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_ros2_EXPORTS
#define chric_protobuf_ros2 __attribute__((visibility("default")))
#else
#define chric_protobuf_ros2 __attribute__((visibility("default")))
#endif
#ifdef chric_protobuf_tangpa_ros2_interfaces_EXPORTS
#define chric_protobuf_tangpa_ros2_interfaces __attribute__((visibility("default")))
#else
#define chric_protobuf_tangpa_ros2_interfaces __attribute__((visibility("default")))
#endif


#else
#ifndef chric_protobuf_api_service_common
#define chric_protobuf_api_service_common
#endif
#ifndef chric_protobuf_api_service_konka
#define chric_protobuf_api_service_konka
#endif
#ifndef chric_protobuf_api_service_tangpa
#define chric_protobuf_api_service_tangpa
#endif
#ifndef chric_protobuf_common
#define chric_protobuf_common
#endif
#ifndef chric_protobuf_communication
#define chric_protobuf_communication
#endif
#ifndef chric_protobuf_konka_ros2_interfaces
#define chric_protobuf_konka_ros2_interfaces
#endif
#ifndef chric_protobuf_ros2
#define chric_protobuf_ros2
#endif
#ifndef chric_protobuf_tangpa_ros2_interfaces
#define chric_protobuf_tangpa_ros2_interfaces
#endif

#endif
#endif

// 通用宏
#define DECLARE_PB_EXPORT(MODULE) MODULE##_API

#endif  // PB_EXPORT_H
