/**
 * @file marco.h
 * @brief 通用宏定义与类型别名
 *
 * 基于CHRIC SDK的通用宏定义与类型别名模块
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 提供了IN/OUT参数标注宏、error_t类型、IP/PORT别名等基础定义
 *
 * @warning
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#ifndef CHRIC_SDK_COMMON_MARCO
#define CHRIC_SDK_COMMON_MARCO
#include <functional>
#include <string>
#define IN
#define OUT

namespace humanoid_robot {
namespace sdk_client {
namespace common {
using error_t = int;
using IP = std::string;
using PORT = int;
} // namespace common
} // namespace sdk_client
} // namespace humanoid_robot

#endif // CHRIC_SDK_COMMON_MARCO