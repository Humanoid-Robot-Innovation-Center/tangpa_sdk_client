/**
 * @file tangpa_system_api.h
 * @brief TangPa机器人系统API
 *
 * TangPa人形机器人的系统信息接口，包括硬件信息查询等功能。
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 提供系统硬件信息、设备信息查询功能
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */
#ifndef CHRIC_SDK_TANGPA_SYSTEM_API
#define CHRIC_SDK_TANGPA_SYSTEM_API

#include <memory>

#include "api_service/common/service.pb.h"
#include "api_service/common/status.pb.h"
#include "api_service/tangpa/system.pb.h"
#include "robot/client/interfaces_client.h"

namespace humanoid_robot {
namespace sdk_client {
namespace robot {
namespace tangpa_system_api {

using DiagnosticMessage =
    humanoid_robot::PB::api_service::tangpa::system::DiagnosticMessage;
using RobotStatus =
    humanoid_robot::PB::api_service::tangpa::system::RobotStatus;
using BatteryPercentage =
    humanoid_robot::PB::api_service::tangpa::system::BatteryPercentage;
using CommonStatus = humanoid_robot::PB::api_service::common::CommonStatus;
using SystemCommandCode =
    humanoid_robot::PB::api_service::common::SystemCommandCode;
/**
 * @brief TangPa机器人系统API客户端
 *
 * 提供与 TangPa 机器人系统相关的 RPC 接口，包括诊断数据获取、机器人状态查询、电池电量查询等功能。
 * 
 * @warning 非线程安全，建议在单线程环境下使用
 */
class SystemApiClient : public InterfacesClient {
 public:
  SystemApiClient();
  ~SystemApiClient();

  /**
   * @brief 获取诊断数据
   * @param diagnostic_message 诊断消息输出
   * @return uint32_t 状态码（0 表示成功）
   * @note 网络通信超时时间5秒
   */
  uint32_t GetDiagnosticData(DiagnosticMessage& diagnostic_message);

  /**
   * @brief 获取机器人状态
   * @param robot_status 机器人状态输出
   * @return uint32_t 状态码（0 表示成功）
   * @note 网络通信超时时间5秒
   */
  uint32_t GetRobotStatus(RobotStatus& robot_status);

  /**
   * @brief 获取电池电量
   * @param battery_percentage 电池电量输出
   * @return uint32_t 状态码（0 表示成功）
   * @note 网络通信超时时间5秒
   */
  uint32_t GetBatteryPercentage(BatteryPercentage& battery_percentage);

 private:
  void Initialize() override {}
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace tangpa_system_api
}  // namespace robot
}  // namespace sdk_client
}  // namespace humanoid_robot
#endif  // CHRIC_SDK_TANGPA_SYSTEM_API
