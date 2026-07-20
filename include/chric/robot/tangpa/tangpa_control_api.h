/**
 * @file tangpa_control_api.h
 * @brief TangPa机器人控制API
 *
 * TangPa人形机器人的控制接口，包括紧急停止、运动控制、IMU数据获取、UWB配置管理等功能。
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 提供急停、运动控制、IMU数据、UWB配置管理等功能
 * @warning 控制命令应在确保安全的环境下执行
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */
#ifndef CHRIC_SDK_TANGPA_CONTROL_API
#define CHRIC_SDK_TANGPA_CONTROL_API

#include <functional>
#include <memory>
#include <string>

#include "api_service/common/service.pb.h"
#include "api_service/common/status.pb.h"
#include "api_service/tangpa/control.pb.h"
#include "robot/client/interfaces_client.h"
#include "ros2/sensor_msgs/Imu.pb.h"
#include "ros2/sensor_msgs/JointState.pb.h"
#include "ros2/sensor_msgs/Joy.pb.h"
#include "tangpa_ros2_interfaces/control/cmotor_interface/Cmotorcommand.pb.h"
#include "tangpa_ros2_interfaces/control/nlink_parser/LinktrackNodeframe7.pb.h"

namespace humanoid_robot {
namespace sdk_client {
namespace robot {
namespace tangpa_control_api {

using Imu = humanoid_robot::PB::ros2::sensor_msgs::Imu;
using Joy = humanoid_robot::PB::ros2::sensor_msgs::Joy;
using Cmotorcommand = humanoid_robot::PB::tangpa_ros2_interfaces::control::
    cmotor_interface::Cmotorcommand;
using JointState = humanoid_robot::PB::ros2::sensor_msgs::JointState;
using JoyControllerResponse =
    humanoid_robot::PB::api_service::tangpa::control::JoyControllerResponse;
using UWBRequestConfig =
    humanoid_robot::PB::api_service::tangpa::control::UWBRequestConfig;
using UWBRequestHardware =
    humanoid_robot::PB::api_service::tangpa::control::UWBRequestHardware;
using MotorCalibrationRequest =
    humanoid_robot::PB::api_service::tangpa::control::MotorCalibrationRequest;
using MotorCalibrationResponse =
    humanoid_robot::PB::api_service::tangpa::control::MotorCalibrationResponse;
using LinktrackNodeframe7 =
    humanoid_robot::PB::tangpa_ros2_interfaces::control::nlink_parser::
        LinktrackNodeframe7;
using CommonStatus = humanoid_robot::PB::api_service::common::CommonStatus;
using ControlCommandCode =
    humanoid_robot::PB::api_service::common::ControlCommandCode;

/**
 * @brief TangPa机器人控制API客户端
 *
 * 提供与 TangPa 机器人控制相关的 RPC 接口，包括急停、运动控制、IMU数据获取、UWB配置管理等功能。
 * 
 * @warning 非线程安全，建议在单线程环境下使用
 */
class ControlApiClient : public InterfacesClient {
 public:
  ControlApiClient();
  ~ControlApiClient();

  /**
   * @brief 获取 IMU 数据
   * @param imu_status IMU 数据输出
   * @return uint32_t 状态码（0 表示成功）
   * @note 网络通信超时时间5秒
   */
  uint32_t GetImuData(Imu& imu_status);

  /**
   * @brief 电机标定（RPC 请求-响应）
   * @param motor_calibration_request 电机标定请求（motor_id）
   * @param motor_calibration_response 标定响应（state, motor_id, message）
   * @return uint32_t 状态码（0 表示成功）
   * @note 服务超时时间5秒，网络通信超时时间30秒
   * @warning 标定与机器人电机控制互斥，不能同时执行
  */
  uint32_t MotorCalibration(
      const MotorCalibrationRequest& motor_calibration_request,
      MotorCalibrationResponse& motor_calibration_response);

  /**
   * @brief 配置机器人 UWB 硬件信息
   * @param uwb_request_hardware UWB 硬件请求（target_device, target_role, baud）
   * @return uint32_t 状态码（0 表示成功）
   * @note 服务超时时间5秒，网络通信超时时间30秒
   */
  uint32_t SetUWBConfigurator(
      const UWBRequestHardware& uwb_request_hardware);

  /**
   * @brief 配置 UWB 跟随信息
   * @param uwb_request_config UWB 配置请求（target_role, target_distance）
   * @return uint32_t 状态码（0 表示成功）
   * @note 服务超时时间5秒，网络通信超时时间30秒
   */
  uint32_t SetUWBConfigManager(
      const UWBRequestConfig& uwb_request_config);

  /**
   * @brief 订阅 UWB 数据流 (zenoh/nlink_linktrack_nodeframe7)
   * @param callback LinktrackNodeframe7 数据回调，参数为 shared_ptr<const LinktrackNodeframe7>
   * @return uint64_t 订阅 ID（大于 0 表示成功），用于取消订阅
   */
  uint64_t SubscribeUWBData(
      const std::function<void(const std::shared_ptr<const LinktrackNodeframe7>&)>& callback);
  /**
   * @brief 取消 UWB 数据订阅
   * @param subscription_id 订阅 ID
   * @return uint32_t 状态码（0 表示成功）
   */
  uint32_t UnsubscribeUWBData(uint64_t subscription_id);

  /**
   * @brief 订阅 IMU 数据流
   * @param callback IMU 数据回调，参数为 shared_ptr<const Imu>
   * @return uint64_t 订阅 ID（大于 0 表示成功），用于取消订阅
   */
  uint64_t SubscribeImu(
      const std::function<void(const std::shared_ptr<const Imu>&)>& callback);
  /**
   * @brief 取消 IMU 数据订阅
   * @param subscription_id 订阅 ID
   * @return uint32_t 状态码（0 表示成功）
   */
  uint32_t UnsubscribeImu(uint64_t subscription_id);

  /**
   * @brief 订阅手柄控制数据流
   * @param callback 手柄数据回调，参数为 shared_ptr<const Joy>
   * @return uint64_t 订阅 ID（大于 0 表示成功），用于取消订阅
   */
  uint64_t SubscribeJoy(
      const std::function<void(const std::shared_ptr<const Joy>&)>& callback);
  /**
   * @brief 取消手柄数据订阅
   * @param subscription_id 订阅 ID
   * @return uint32_t 状态码（0 表示成功）
   */
  uint32_t UnsubscribeJoy(uint64_t subscription_id);

  /**
   * @brief 订阅关节状态数据流
   * @param callback 关节状态数据回调，参数为 shared_ptr<const JointState>
   * @return uint64_t 订阅 ID（大于 0 表示成功），用于取消订阅
   */
  uint64_t SubscribeJointState(
      const std::function<void(const std::shared_ptr<const JointState>&)>& callback);
  /**
   * @brief 取消关节状态订阅
   * @param subscription_id 订阅 ID
   * @return uint32_t 状态码（0 表示成功）
   */
  uint32_t UnsubscribeJointState(uint64_t subscription_id);

  /**
   * @brief 发送电机控制指令（单向发布）
   * @param motor_command 电机控制指令
   * @return uint32_t 状态码（0 表示成功）
   */
  uint32_t SendMotorControl(const Cmotorcommand& motor_command);

  /**
   * @brief 发送手柄控制指令（单向发布）
   * @param joy_command 手柄控制指令
   * @return uint32_t 状态码（0 表示成功）
   */
  uint32_t SendJoyControl(const Joy& joy_command);

 private:
  void Initialize() override;
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace tangpa_control_api
}  // namespace robot
}  // namespace sdk_client
}  // namespace humanoid_robot
#endif  // CHRIC_SDK_TANGPA_CONTROL_API
