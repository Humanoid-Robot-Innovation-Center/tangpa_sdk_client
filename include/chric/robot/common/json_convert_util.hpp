/**
 * @file json_convert_util.hpp
 * @brief Protobuf与JSON转换工具
 *
 * 基于CHRIC SDK的Protobuf与JSON转换工具模块
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 提供统一的JSON序列化/反序列化选项配置
 *
 * @warning
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#ifndef CHRIC_SDK_COMMON_JSON_CONVERT_UTIL
#define CHRIC_SDK_COMMON_JSON_CONVERT_UTIL

#include "google/protobuf/util/json_util.h"
namespace humanoid_robot {
namespace sdk_client {
namespace common {
inline const google::protobuf::util::JsonPrintOptions &GetJsonPrintOptions() {
  static const google::protobuf::util::JsonPrintOptions json_print_options =
      []() {
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true; // 格式化输出（和ROS2一致）
        options.always_print_fields_with_no_presence =
            false; // 省略默认值（和ROS2一致）
        options.always_print_enums_as_ints = true; // 枚举输出数字（核心对齐项）
        options.preserve_proto_field_names =
            true; // 保留下划线字段名（核心对齐项）
        options.unquote_int64_if_possible = true; // int64输出数字（和ROS2一致）
        return options;
      }();
  return json_print_options;
}

inline const google::protobuf::util::JsonParseOptions &GetJsonParseOptions() {
  static const google::protobuf::util::JsonParseOptions json_parse_options =
      []() {
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = true;
        return options;
      }();
  return json_parse_options;
}
} // namespace common
} // namespace sdk_client
} // namespace humanoid_robot

#endif  // CHRIC_SDK_COMMON_JSON_CONVERT_UTIL