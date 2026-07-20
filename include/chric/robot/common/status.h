/**
 * @file status.h
 * @brief 状态封装类
 *
 * 基于CHRIC SDK的统一状态封装模块
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 提供统一的错误状态封装，支持std::error_code和自定义消息
 *
 * @warning
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#ifndef CHRIC_SDK_COMMON_STATUS
#define CHRIC_SDK_COMMON_STATUS
#include <string>
#include <system_error>

namespace humanoid_robot {
namespace sdk_client {
namespace common {
class [[nodiscard]] Status {
public:
  Status() = default;

  ~Status() = default;

  Status(const std::error_code &code, std::string message = "");

  operator bool() const;

  std::error_code code() const { return m_code; }

  // Check if this matches a certain error enum.
  template <typename ENUM> bool Is() const {
    static_assert(std::is_error_code_enum<ENUM>::value,
                  "Must check against an error code enum");
    return std::error_code(ENUM{}).category() == m_code.category();
  }

  const std::string &message() const { return m_message; }

  // Extend a ::bosdyn::common::Status with a new message.
  ::humanoid_robot::sdk_client::common::Status Chain(std::string message) const;

  // Transform a ::bosdyn::common::Status into a new code.
  ::humanoid_robot::sdk_client::common::Status Chain(std::error_code code,
                                                    std::string message) const;

  std::string DebugString() const;

  // Used to explicitly ignore any error present to silence nodiscard warnings.
  inline void IgnoreError() const {}

private:
  std::error_code m_code;
  std::string m_message;
};
} // namespace common
} // namespace sdk_client
} // namespace humanoid_robot
#endif // CHRIC_SDK_COMMON_STATUS