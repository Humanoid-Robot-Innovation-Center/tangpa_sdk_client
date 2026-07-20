/**
 * @file error_code.h
 * @brief 错误码定义
 *
 * 基于CHRIC SDK的统一错误码定义模块
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 定义了SDK统一的错误码体系，包括网络层和服务层的错误码
 *
 * @warning
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#ifndef CHRIC_SDK_COMMON_ERROR_CODE
#define CHRIC_SDK_COMMON_ERROR_CODE
#include "marco.h"
using namespace humanoid_robot::sdk_client::common;

namespace humanoid_robot {
namespace sdk_client {
namespace common {
constexpr error_t SUCCESS = 0;

constexpr error_t INTERFACE_BASE = -100000;
constexpr error_t INTERFACE_NET_BASE = INTERFACE_BASE - 1000;
constexpr error_t INTERFACE_SERVICE_BASE = INTERFACE_BASE - 2000;

constexpr error_t INTERFACE_CONNECT_FAILED = INTERFACE_NET_BASE - 1;
constexpr error_t INTERFACE_SEND_FAILED = INTERFACE_NET_BASE - 2;

} // namespace common
} // namespace sdk_client
} // namespace humanoid_robot
#endif // CHRIC_SDK_COMMON_ERROR_CODE
