/**
 * @file success_condition.h
 * @brief 成功条件枚举
 *
 * 基于CHRIC SDK的成功条件枚举模块
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 定义Success错误条件，集成std::error_condition体系
 *
 * @warning
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#include <system_error>

enum class SuccessCondition { Success = 0 };

namespace std {
template <> struct is_error_condition_enum<SuccessCondition> : true_type {};
} // namespace std

std::error_condition make_error_condition(SuccessCondition);