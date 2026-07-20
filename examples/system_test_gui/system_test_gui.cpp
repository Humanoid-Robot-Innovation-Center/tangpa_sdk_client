#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <thread>

#include "chric/robot/client/interfaces_client.h"
#include "chric/robot/tangpa/tangpa_system_api.h"

using namespace humanoid_robot::sdk_client::robot::tangpa_system_api;
using namespace humanoid_robot::sdk_client::robot;

void TestGetDiagnosticData(std::unique_ptr<SystemApiClient>& client) {
  std::cout << "\n--- GetDiagnosticData ---" << std::endl;
  DiagnosticMessage msg;
  uint32_t status = client->GetDiagnosticData(msg);
  if (status == 0) {
    std::cout << "  result: " << msg.result() << std::endl;
    const auto& diag = msg.messages();
    std::cout << "  诊断项数量: " << diag.status_size() << std::endl;
    for (int i = 0; i < std::min(5, diag.status_size()); ++i) {
      const auto& s = diag.status(i);
      std::cout << "  [" << i << "] " << s.name()
                << " (level=" << s.level() << "): " << s.message() << std::endl;
    }
    if (diag.status_size() > 5)
      std::cout << "  ... 共 " << diag.status_size() << " 项" << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}

void TestGetRobotStatus(std::unique_ptr<SystemApiClient>& client) {
  std::cout << "\n--- GetRobotStatus ---" << std::endl;
  RobotStatus robot_status;
  uint32_t status = client->GetRobotStatus(robot_status);
  if (status == 0) {
    std::cout << "  state_id: " << robot_status.state_id() << std::endl;
    std::cout << "  state_desc: " << robot_status.state_desc() << std::endl;
    std::cout << "  battery: " << robot_status.battery() << "%" << std::endl;
    std::cout << "  error_codes: " << robot_status.error_codes_size() << " 个" << std::endl;
    for (int i = 0; i < robot_status.error_codes_size(); ++i)
      std::cout << "    [" << i << "] " << robot_status.error_codes(i) << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}

void TestGetBatteryPercentage(std::unique_ptr<SystemApiClient>& client) {
  std::cout << "\n--- GetBatteryPercentage ---" << std::endl;
  BatteryPercentage battery;
  uint32_t status = client->GetBatteryPercentage(battery);
  if (status == 0) {
    std::cout << "  info: " << battery.info() << std::endl;
  } else {
    std::cerr << "  失败, 状态码: " << status << std::endl;
  }
}

void ClearInputBuffer() {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void PrintMenu() {
  std::cout << "\n=====================================" << std::endl;
  std::cout << "        System API 测试菜单          " << std::endl;
  std::cout << "=====================================" << std::endl;
  std::cout << "  1 - GetDiagnosticData     获取诊断数据" << std::endl;
  std::cout << "  2 - GetRobotStatus        获取机器人状态" << std::endl;
  std::cout << "  3 - GetBatteryPercentage  获取电池电量" << std::endl;
  std::cout << "  0 - 退出" << std::endl;
  std::cout << "=====================================" << std::endl;
  std::cout << "请输入编号: ";
}

int main(int argc, char* argv[]) {
  // std::string server_url = "localhost:50051";
  // if (argc > 1) server_url = argv[1];
  // std::cout << "System API 交互式测试 | Server: " << server_url << std::endl;

  auto client = std::make_unique<SystemApiClient>();
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
      case 1: TestGetDiagnosticData(client); break;
      case 2: TestGetRobotStatus(client); break;
      case 3: TestGetBatteryPercentage(client); break;
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
