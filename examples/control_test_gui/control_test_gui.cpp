#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

#include "chric/robot/client/interfaces_client.h"
#include "chric/robot/tangpa/tangpa_control_api.h"
#include "chric/robot/tangpa/tangpa_system_api.h"

#include "../tools/proto_text_parser.hpp"

using namespace std::chrono_literals;
using namespace humanoid_robot::sdk_client::robot::tangpa_control_api;
using namespace humanoid_robot::sdk_client::robot::tangpa_system_api;
using namespace humanoid_robot::sdk_client::robot;

// ============================================================================
// 回调函数
// ============================================================================

void OnIMUCallback(const std::shared_ptr<const Imu>& imu) {
  std::cout << "\n[IMU] 时间戳: " << imu->header().stamp().sec() << "."
            << imu->header().stamp().nanosec() << std::endl;
  const auto& ori = imu->orientation();
  std::cout << "  姿态: x=" << ori.x() << " y=" << ori.y()
            << " z=" << ori.z() << " w=" << ori.w() << std::endl;
  const auto& ang = imu->angular_velocity();
  std::cout << "  角速度: x=" << ang.x() << " y=" << ang.y()
            << " z=" << ang.z() << " rad/s" << std::endl;
  const auto& lin = imu->linear_acceleration();
  std::cout << "  线加速度: x=" << lin.x() << " y=" << lin.y()
            << " z=" << lin.z() << " m/s²" << std::endl;
}

void OnJoyCallback(const std::shared_ptr<const Joy>& joy) {
  std::cout << "[Joy] axes: ";
  for (int i = 0; i < joy->axes_size(); ++i)
    std::cout << joy->axes(i) << " ";
  std::cout << "buttons: ";
  for (int i = 0; i < joy->buttons_size(); ++i)
    std::cout << joy->buttons(i) << " ";
  std::cout << std::endl;
}

void OnJointStateCallback(const std::shared_ptr<const JointState>& state) {
  std::cout << "\n[JointState] 关节数量: " << state->name_size() << std::endl;
  for (int i = 0; i < std::min(5, state->name_size()); ++i) {
    std::cout << "  " << state->name(i) << ": pos=" << state->position(i)
              << " vel=" << state->velocity(i)
              << " eff=" << state->effort(i) << std::endl;
  }
  if (state->name_size() > 5)
    std::cout << "  ... 共 " << state->name_size() << " 个关节" << std::endl;
}

void OnUWBStreamCallback(const std::shared_ptr<const LinktrackNodeframe7>& frame) {
  std::cout << "\n[UWB] role=" << frame->role() << " id=" << frame->id()
            << " voltage=" << frame->voltage() << "V"
            << " nodes=" << frame->nodes_size() << std::endl;
  for (int i = 0; i < frame->nodes_size(); ++i) {
    const auto& node = frame->nodes(i);
    std::cout << "  节点 " << i + 1 << ": role=" << node.role()
              << " id=" << node.id() << " dis=" << node.dis() << "m"
              << std::endl;
  }
}

// ============================================================================
// 测试函数
// ============================================================================

void TestGetImuData(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- GetImuData ---" << std::endl;
  Imu imu;
  uint32_t status = client->GetImuData(imu);
  if (status == 0) {
    const auto& ori = imu.orientation();
    std::cout << "  姿态: x=" << ori.x() << " y=" << ori.y()
              << " z=" << ori.z() << " w=" << ori.w() << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}

/*
void TestMotorControlRPC(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- MotorControl (RPC) ---" << std::endl;
  Cmotorcommand req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "motor_control.pb.txt", req))
    return;

  JointState resp;
  uint32_t status = client->MotorControl(req, resp);
  if (status == 0) {
    std::cout << "  返回关节数量: " << resp.name_size() << std::endl;
    for (int i = 0; i < resp.name_size(); ++i)
      std::cout << "  " << resp.name(i) << ": pos=" << resp.position(i)
                << " vel=" << resp.velocity(i)
                << " eff=" << resp.effort(i) << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}

void TestJoyControlRPC(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- JoyControl (RPC) ---" << std::endl;
  Joy req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "joy_control.pb.txt", req))
    return;

  JoyControllerResponse resp;
  uint32_t status = client->JoyControl(req, resp);
  if (status == 0) {
    const auto& diag = resp.results();
    std::cout << "  诊断状态数量: " << diag.status_size() << std::endl;
    for (int i = 0; i < diag.status_size(); ++i)
      std::cout << "  [" << i << "] " << diag.status(i).name()
                << ": " << diag.status(i).message() << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}
*/

void TestSendMotorControl(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SendMotorControl (流式发布) ---" << std::endl;
  Cmotorcommand req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "motor_control_stream.pb.txt", req))
    return;

  uint32_t status = client->SendMotorControl(req);
  std::cout << "  状态码: " << status << (status == 0 ? " [OK]" : " [FAIL]") << std::endl;
}

void TestSendJoyControl(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SendJoyControl (流式发布) ---" << std::endl;
  Joy req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "joy_control.pb.txt", req))
    return;

  uint32_t status = client->SendJoyControl(req);
  std::cout << "  状态码: " << status << (status == 0 ? " [OK]" : " [FAIL]") << std::endl;
}

void TestSubscribeImu(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SubscribeImu (订阅5秒) ---" << std::endl;
  uint64_t sub_id = client->SubscribeImu(OnIMUCallback);
  if (sub_id == 0) {
    std::cerr << "  订阅失败" << std::endl;
    return;
  }
  std::cout << "  订阅ID: " << sub_id << ", 等待数据..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  client->UnsubscribeImu(sub_id);
  std::cout << "  已取消订阅" << std::endl;
}

void TestSubscribeJoy(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SubscribeJoy (订阅5秒) ---" << std::endl;
  uint64_t sub_id = client->SubscribeJoy(OnJoyCallback);
  if (sub_id == 0) {
    std::cerr << "  订阅失败" << std::endl;
    return;
  }
  std::cout << "  订阅ID: " << sub_id << ", 等待数据..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  client->UnsubscribeJoy(sub_id);
  std::cout << "  已取消订阅" << std::endl;
}

void TestSubscribeJointState(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SubscribeJointState (订阅5秒) ---" << std::endl;
  uint64_t sub_id = client->SubscribeJointState(OnJointStateCallback);
  if (sub_id == 0) {
    std::cerr << "  订阅失败" << std::endl;
    return;
  }
  std::cout << "  订阅ID: " << sub_id << ", 等待数据..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  client->UnsubscribeJointState(sub_id);
  std::cout << "  已取消订阅" << std::endl;
}

void TestSetUWBConfigurator(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SetUWBConfigurator ---" << std::endl;
  UWBRequestHardware req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "uwb_hardware.pb.txt", req))
    return;

  uint32_t status = client->SetUWBConfigurator(req);
  std::cout << "  状态码: " << status << (status == 0 ? " [OK]" : " [FAIL]") << std::endl;
}

void TestSetUWBConfigManager(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SetUWBConfigManager ---" << std::endl;
  UWBRequestConfig req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "uwb_config.pb.txt", req))
    return;

  uint32_t status = client->SetUWBConfigManager(req);
  std::cout << "  状态码: " << status << (status == 0 ? " [OK]" : " [FAIL]") << std::endl;
}

void TestSubscribeUWBData(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- SubscribeUWBData (订阅5秒) ---" << std::endl;
  uint64_t sub_id = client->SubscribeUWBData(OnUWBStreamCallback);
  if (sub_id == 0) {
    std::cerr << "  订阅失败" << std::endl;
    return;
  }
  std::cout << "  订阅ID: " << sub_id << ", 等待数据..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  client->UnsubscribeUWBData(sub_id);
  std::cout << "  已取消订阅" << std::endl;
}

void TestMotorCalibration(std::unique_ptr<ControlApiClient>& client) {
  std::cout << "\n--- MotorCalibration ---" << std::endl;
  MotorCalibrationRequest req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "motor_calibration.pb.txt", req))
    return;

  MotorCalibrationResponse resp;
  uint32_t status = client->MotorCalibration(req, resp);
  if (status == 0) {
    std::cout << "  state: " << resp.state() << " motor_id: " << resp.motor_id()
              << " message: " << resp.message() << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}

// ============================================================================
// 主菜单
// ============================================================================

void ClearInputBuffer() {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void PrintMenu() {
  std::cout << "\n=====================================" << std::endl;
  std::cout << "        Control API 测试菜单         " << std::endl;
  std::cout << "=====================================" << std::endl;
  std::cout << "  1  - GetImuData           获取IMU数据" << std::endl;
  std::cout << "  2  - MotorControl         RPC电机控制" << std::endl;
  std::cout << "  3  - JoyControl           RPC手柄控制" << std::endl;
  std::cout << "  4  - SendMotorControl     流式电机控制" << std::endl;
  std::cout << "  5  - SendJoyControl       流式手柄控制" << std::endl;
  std::cout << "  6  - SubscribeImu         订阅IMU (5s)" << std::endl;
  std::cout << "  7  - SubscribeJoy         订阅手柄 (5s)" << std::endl;
  std::cout << "  8  - SubscribeJointState  订阅关节状态 (5s)" << std::endl;
  std::cout << "  9  - SetUWBConfigurator   UWB硬件配置" << std::endl;
  std::cout << "  10 - SetUWBConfigManager  UWB跟随配置" << std::endl;
  std::cout << "  11 - SubscribeUWBData     订阅UWB数据 (5s)" << std::endl;
  std::cout << "  12 - MotorCalibration     电机标定" << std::endl;
  std::cout << "  0  - 退出" << std::endl;
  std::cout << "=====================================" << std::endl;
  std::cout << "请输入编号: ";
}

int main(int argc, char* argv[]) {
  // std::string server_url = "localhost:50051";
  // if (argc > 1) server_url = argv[1];
  // std::cout << "Control API 交互式测试 | Server: " << server_url << std::endl;

  auto client = std::make_unique<ControlApiClient>();
  // auto conn = client->Connect(server_url);
  auto conn = client->Connect();
  if (!conn) {
    std::cerr << "连接失败: " << conn.message() << std::endl;
    return 1;
  }
  std::cout << "[OK] 已连接" << std::endl;

  int choice = -1;
  while (true) {
    PrintMenu();
    if (!(std::cin >> choice)) {
      ClearInputBuffer();
      std::cerr << "请输入数字" << std::endl;
      continue;
    }

    switch (choice) {
      case 0: goto Exit;
      case 1:  TestGetImuData(client); break;
      case 2:  /* TestMotorControlRPC(client); */ break;
      case 3:  /* TestJoyControlRPC(client); */ break;
      case 4:  TestSendMotorControl(client); break;
      case 5:  TestSendJoyControl(client); break;
      case 6:  TestSubscribeImu(client); break;
      case 7:  TestSubscribeJoy(client); break;
      case 8:  TestSubscribeJointState(client); break;
      case 9:  TestSetUWBConfigurator(client); break;
      case 10: TestSetUWBConfigManager(client); break;
      case 11: TestSubscribeUWBData(client); break;
      case 12: TestMotorCalibration(client); break;
      default: std::cerr << "无效选择" << std::endl; break;
    }

    std::cout << "\n按回车继续..." << std::endl;
    ClearInputBuffer();
    std::cin.get();
  }

Exit:
  client->Disconnect();
  std::cout << "\n程序退出" << std::endl;
  return 0;
}
