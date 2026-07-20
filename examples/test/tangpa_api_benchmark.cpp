#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "chric/robot/client/interfaces_client.h"
#include "chric/robot/tangpa/tangpa_control_api.h"
#include "chric/robot/tangpa/tangpa_system_api.h"

#include "../tools/proto_text_parser.hpp"

using namespace std;
using namespace humanoid_robot::sdk_client::robot::tangpa_control_api;
using namespace humanoid_robot::sdk_client::robot::tangpa_system_api;
using namespace humanoid_robot::sdk_client::robot;

struct LatencyStats {
  std::vector<double> latencies;
  std::atomic<uint64_t> count{0};
  std::atomic<bool> running{true};
  std::atomic<bool> paused{false};
  std::mutex mutex;

  void reset() {
    std::lock_guard<std::mutex> lock(mutex);
    latencies.clear();
    count = 0;
    running = true;
    paused = false;
  }

  void add(double latency_ms) {
    if (paused) return;
    std::lock_guard<std::mutex> lock(mutex);
    latencies.push_back(latency_ms);
    count++;
  }

  void stop() { running = false; }

  void pause() { paused = true; }

  void resume() { paused = false; }

  void printStats(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    if (latencies.empty()) {
      cout << "[!] " << name << " 无数据" << endl;
      return;
    }

    sort(latencies.begin(), latencies.end());
    double min_val = latencies.front();
    double max_val = latencies.back();
    double sum = 0.0;
    for (double v : latencies) sum += v;
    double avg = sum / latencies.size();

    int p50_idx = static_cast<int>(0.50 * latencies.size());
    int p95_idx = static_cast<int>(0.95 * latencies.size());
    int p99_idx = static_cast<int>(0.99 * latencies.size());
    p50_idx = min(p50_idx, static_cast<int>(latencies.size()) - 1);
    p95_idx = min(p95_idx, static_cast<int>(latencies.size()) - 1);
    p99_idx = min(p99_idx, static_cast<int>(latencies.size()) - 1);

    double p50 = latencies[p50_idx];
    double p95 = latencies[p95_idx];
    double p99 = latencies[p99_idx];

    cout << fixed << setprecision(3);
    cout << "\n╔======================================╗" << endl;
    cout << "║          " << name << " 统计结果          ║" << endl;
    cout << "╠======================================╣" << endl;
    cout << "║ 总帧数: " << setw(15) << count << " ║" << endl;
    cout << "║ 帧率: " << setw(17) << count / 5.0 << " Hz ║" << endl;
    cout << "║ 最小间隔: " << setw(12) << min_val << " ms ║" << endl;
    cout << "║ 最大间隔: " << setw(12) << max_val << " ms ║" << endl;
    cout << "║ 平均间隔: " << setw(12) << avg << " ms ║" << endl;
    cout << "║ P50 间隔: " << setw(13) << p50 << " ms ║" << endl;
    cout << "║ P95 间隔: " << setw(13) << p95 << " ms ║" << endl;
    cout << "║ P99 间隔: " << setw(13) << p99 << " ms ║" << endl;
    cout << "╚======================================╝" << endl;
  }

  size_t getCount() const { return count.load(); }
};

LatencyStats imu_stats;
LatencyStats joy_stats;
LatencyStats joint_stats;

// 用于计算接收间隔的上一帧本地时间（纳秒）
std::atomic<uint64_t> last_imu_ns{0};
std::atomic<uint64_t> last_joy_ns{0};
std::atomic<uint64_t> last_joint_ns{0};

void OnIMUBenchmark(const std::shared_ptr<const Imu>& imu_result) {
  if (!imu_stats.running) return;
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();

  uint64_t prev = last_imu_ns.load();
  if (prev != 0) {
    double interval_ms = (now_ns - prev) / 1e6;
    imu_stats.add(interval_ms);
  }
  last_imu_ns.store(now_ns);
}

void OnJoyBenchmark(const std::shared_ptr<const Joy>& joy_result) {
  if (!joy_stats.running) return;
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();

  uint64_t prev = last_joy_ns.load();
  if (prev != 0) {
    double interval_ms = (now_ns - prev) / 1e6;
    joy_stats.add(interval_ms);
  }
  last_joy_ns.store(now_ns);
}

void OnJointBenchmark(const std::shared_ptr<const JointState>& joint_result) {
  if (!joint_stats.running) return;
  auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();

  uint64_t prev = last_joint_ns.load();
  if (prev != 0) {
    double interval_ms = (now_ns - prev) / 1e6;
    joint_stats.add(interval_ms);
  }
  last_joint_ns.store(now_ns);
}

std::atomic<uint64_t> last_uwb_system_time{0};

void OnUWBBenchmark(const std::shared_ptr<const LinktrackNodeframe7>& frame, LatencyStats* stats) {
  if (!stats->running.load()) return;

  uint64_t current_time = frame->system_time();

  uint64_t prev_time = last_uwb_system_time.load();
  if (prev_time != 0 && current_time > prev_time) {
    double interval_ms = (current_time - prev_time) / 1000.0;
    stats->add(interval_ms);
  }

  last_uwb_system_time.store(current_time);
}

void clearScreen() { cout << "\033[2J\033[H"; }

void printMenu() {
  clearScreen();
  cout << "============= TANGPA API 性能测试工具 ===================" << endl;
  cout << endl;
  cout << "------------------------ 订阅接口 ------------------------" << endl;
  cout << "    [1] IMU        - 惯性测量单元订阅" << endl;
  cout << "    [2] Joy        - 手柄控制器订阅" << endl;
  cout << "    [3] JointState - 关节状态订阅" << endl;
  cout << "    [4] UWBSub     - UWB数据订阅" << endl;
  cout << endl;
  cout << "------------------------ 请求接口 ------------------------" << endl;
  cout << "    [5] UWB        - UWB 硬件配置" << endl;
  cout << "    [6] UWBConfig  - UWB 配置管理" << endl;
  cout << "    [7] GetIMU     - 获取IMU数据" << endl;
  cout << "    [8] GetDiag    - 获取诊断数据" << endl;
  cout << "    [9] GetRobot   - 获取机器人状态" << endl;
  cout << "   [10] GetBattery - 获取电池电量" << endl;
  // cout << "   [11] JoyCtrl    - 手柄控制指令(RPC)" << endl;
  // cout << "   [12] MotorCtrl  - 电机控制(RPC)" << endl;
  cout << endl;
  cout << "------------------------ 发布接口 ------------------------" << endl;
  cout << "   [13] JoySend    - 手柄控制指令(Zenoh)" << endl;
  cout << "   [14] MotorSend  - 电机控制(Zenoh)" << endl;
  cout << "   [15] MotorCalib - 电机标定(RPC)" << endl;
  cout << endl;
  cout << "------------------------ 其他 ------------------------" << endl;
  cout << endl;
  cout << "   [A] All         - 测试所有接口" << endl;
  cout << endl;
  cout << "===================================================================="
       << endl;
  cout << "    [0] 退出程序" << endl;
  cout << "\n请输入选项 [0-15, A]: ";
}

void printTestProgress(int seconds, const string& interface_name) {
  cout << "\r⏳ 测试进行中 [" << interface_name << "] ";
  for (int i = 0; i < seconds; ++i) {
    cout << "█";
  }
  for (int i = seconds; i < 5; ++i) {
    cout << "░";
  }
  cout << " " << seconds << "/5 秒" << flush;
}

void TestSingleInterface(ControlApiClient& client, const string& interface_name,
                         const string& display_name) {
  uint64_t sub_id = 0;

  if (interface_name == "imu") {
    imu_stats.reset();
    last_imu_ns = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    sub_id = client.SubscribeImu(OnIMUBenchmark);
    auto t2 = std::chrono::high_resolution_clock::now();
    cout << "⏱ SubscribeImu 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
  } else if (interface_name == "joy") {
    joy_stats.reset();
    last_joy_ns = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    sub_id = client.SubscribeJoy(OnJoyBenchmark);
    auto t2 = std::chrono::high_resolution_clock::now();
    cout << "⏱ SubscribeJoy 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
  } else if (interface_name == "joint") {
    joint_stats.reset();
    last_joint_ns = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    sub_id = client.SubscribeJointState(OnJointBenchmark);
    auto t2 = std::chrono::high_resolution_clock::now();
    cout << "⏱ SubscribeJointState 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
  }

  if (sub_id == 0) {
    cout << "\n❌ " << display_name << " 订阅失败" << endl;
    return;
  }

  cout << "\n✅ " << display_name << " 订阅成功, ID: " << sub_id << endl;

  for (int i = 1; i <= 5; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    printTestProgress(i, interface_name);
  }
  cout << endl;

  if (interface_name == "imu") {
    imu_stats.stop();
    auto t1 = std::chrono::high_resolution_clock::now();
    client.UnsubscribeImu(sub_id);
    auto t2 = std::chrono::high_resolution_clock::now();
    cout << "⏱ UnsubscribeImu 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
    imu_stats.printStats(display_name);
  } else if (interface_name == "joy") {
    joy_stats.stop();
    auto t1 = std::chrono::high_resolution_clock::now();
    client.UnsubscribeJoy(sub_id);
    auto t2 = std::chrono::high_resolution_clock::now();
    cout << "⏱ UnsubscribeJoy 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
    joy_stats.printStats(display_name);
  } else if (interface_name == "joint") {
    joint_stats.stop();
    auto t1 = std::chrono::high_resolution_clock::now();
    client.UnsubscribeJointState(sub_id);
    auto t2 = std::chrono::high_resolution_clock::now();
    cout << "⏱ UnsubscribeJointState 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
    joint_stats.printStats(display_name);
  }
}

void TestMotorControl(ControlApiClient& client) {
  const int TEST_DURATION_SEC = 5;
  const float CONTROL_FREQ = 500.0f;  // 帧率 Hz

  Cmotorcommand motor_template;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "motor_control_stream.pb.txt",
                              motor_template)) {
    return;
  }
  const int MOTOR_NUM = motor_template.motor_ids_size();

  // 每帧间隔时间（微秒）
  const int64_t FRAME_INTERVAL_US =
      static_cast<int64_t>(1000000.0f / CONTROL_FREQ);

  auto start_time = chrono::steady_clock::now();
  auto next_send_time = start_time;
  uint64_t frame_count = 0;
  std::vector<double> send_latencies;

  cout << "\n✅ 开始电机控制测试..." << endl;
  cout << "    帧率: " << CONTROL_FREQ << " Hz" << endl;
  cout << "    电机数: " << MOTOR_NUM << endl;
  cout << "    每帧间隔: " << FRAME_INTERVAL_US << " us" << endl;

  int64_t total_us = TEST_DURATION_SEC * 1000000LL;
  auto test_end_time = start_time + chrono::microseconds(total_us);

  int last_progress = 0;

  while (chrono::steady_clock::now() < test_end_time) {
    Cmotorcommand req;
    auto now = chrono::steady_clock::now();
    float t =
        chrono::duration_cast<chrono::microseconds>(now - start_time).count() /
        1000000.0f;
    float pos = sin(t * 2.0f) * 0.5f;

    for (int k = 0; k < MOTOR_NUM; ++k) {
      req.add_motor_ids(motor_template.motor_ids(k));
      req.add_control_modes(motor_template.control_modes(k));
      req.add_positions(pos);
      req.add_speeds(0.0f);
      req.add_torques(0.0f);
      req.add_kps(motor_template.kps(k));
      req.add_kds(motor_template.kds(k));
    }

    auto send_start = chrono::steady_clock::now();
    client.SendMotorControl(req);
    auto send_end = chrono::steady_clock::now();

    double send_ms =
        chrono::duration_cast<chrono::nanoseconds>(send_end - send_start)
            .count() /
        1000000.0;
    send_latencies.push_back(send_ms);
    frame_count++;

    // 计算下一帧应该发送的时间
    next_send_time += chrono::microseconds(FRAME_INTERVAL_US);

    // 如果还没到下一帧时间，就睡眠等待
    auto current_time = chrono::steady_clock::now();
    if (current_time < next_send_time) {
      std::this_thread::sleep_until(next_send_time);
    } else {
      // 如果已经过了下一帧时间，说明发送速度不够快
      // 更新下一次发送时间为当前时间 + 一个帧间隔
      next_send_time = current_time + chrono::microseconds(FRAME_INTERVAL_US);
    }

    // 更新进度显示
    auto elapsed =
        chrono::duration_cast<std::chrono::seconds>(current_time - start_time)
            .count();
    if (elapsed > last_progress) {
      last_progress = static_cast<int>(elapsed);
      printTestProgress(last_progress, "MotorCtrl");
    }
  }
  cout << endl;

  if (send_latencies.empty()) {
    cout << "[!] MotorControl 无数据" << endl;
    return;
  }

  sort(send_latencies.begin(), send_latencies.end());
  double min_val = send_latencies.front();
  double max_val = send_latencies.back();
  double sum = 0.0;
  for (double v : send_latencies) sum += v;
  double avg = sum / send_latencies.size();

  int p50_idx = static_cast<int>(0.50 * send_latencies.size());
  int p95_idx = static_cast<int>(0.95 * send_latencies.size());
  int p99_idx = static_cast<int>(0.99 * send_latencies.size());
  p50_idx = min(p50_idx, static_cast<int>(send_latencies.size()) - 1);
  p95_idx = min(p95_idx, static_cast<int>(send_latencies.size()) - 1);
  p99_idx = min(p99_idx, static_cast<int>(send_latencies.size()) - 1);

  double p50 = send_latencies[p50_idx];
  double p95 = send_latencies[p95_idx];
  double p99 = send_latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║      MotorControl 统计结果           ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总帧数: " << setw(15) << frame_count << " ║" << endl;
  cout << "║ 目标帧率: " << setw(13) << CONTROL_FREQ << " Hz ║" << endl;
  cout << "║ 实际帧率: " << setw(13) << frame_count / 5.0 << " Hz ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;
}

void TestSendJoyControl(ControlApiClient& client) {
  const int TEST_DURATION_SEC = 5;
  const float CONTROL_FREQ = 100.0f;
  const int64_t FRAME_INTERVAL_US =
      static_cast<int64_t>(1000000.0f / CONTROL_FREQ);

  Joy joy_template;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "joy_control.pb.txt",
                              joy_template)) {
    return;
  }

  auto start_time = chrono::steady_clock::now();
  auto next_send_time = start_time;
  uint64_t frame_count = 0;
  std::vector<double> send_latencies;

  cout << "\n✅ 开始 SendJoyControl 测试..." << endl;
  cout << "    帧率: " << CONTROL_FREQ << " Hz" << endl;
  cout << "    每帧间隔: " << FRAME_INTERVAL_US << " us" << endl;

  int64_t total_us = TEST_DURATION_SEC * 1000000LL;
  auto test_end_time = start_time + chrono::microseconds(total_us);
  int last_progress = 0;

  while (chrono::steady_clock::now() < test_end_time) {
    Joy joy_req;

    auto* header = joy_req.mutable_header();
    auto now = chrono::steady_clock::now();
    uint32_t sec =
        chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();
    uint32_t nsec =
        chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch())
            .count() %
        1000000000;
    header->mutable_stamp()->set_sec(sec);
    header->mutable_stamp()->set_nanosec(nsec);
    header->set_frame_id(joy_template.header().frame_id());

    float t =
        chrono::duration_cast<chrono::microseconds>(now - start_time).count() /
        1000000.0f;
    joy_req.add_axes(sin(t * 2.0f) * 0.5f);
    for (int i = 1; i < joy_template.axes_size(); ++i) {
      joy_req.add_axes(joy_template.axes(i));
    }
    for (int i = 0; i < joy_template.buttons_size(); ++i) {
      joy_req.add_buttons(joy_template.buttons(i));
    }

    auto send_start = chrono::steady_clock::now();
    client.SendJoyControl(joy_req);
    auto send_end = chrono::steady_clock::now();

    double send_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(send_end - send_start)
            .count() /
        1000000.0;
    send_latencies.push_back(send_ms);
    frame_count++;

    next_send_time += chrono::microseconds(FRAME_INTERVAL_US);

    auto current_time = chrono::steady_clock::now();
    if (current_time < next_send_time) {
      std::this_thread::sleep_until(next_send_time);
    } else {
      next_send_time = current_time + chrono::microseconds(FRAME_INTERVAL_US);
    }

    auto elapsed =
        chrono::duration_cast<std::chrono::seconds>(current_time - start_time)
            .count();
    if (elapsed > last_progress) {
      last_progress = static_cast<int>(elapsed);
      printTestProgress(last_progress, "SendJoy");
    }
  }
  cout << endl;

  if (send_latencies.empty()) {
    cout << "[!] SendJoyControl 无数据" << endl;
    return;
  }

  sort(send_latencies.begin(), send_latencies.end());
  double min_val = send_latencies.front();
  double max_val = send_latencies.back();
  double sum = 0.0;
  for (double v : send_latencies) sum += v;
  double avg = sum / send_latencies.size();

  int p50_idx = static_cast<int>(0.50 * send_latencies.size());
  int p95_idx = static_cast<int>(0.95 * send_latencies.size());
  int p99_idx = static_cast<int>(0.99 * send_latencies.size());
  p50_idx = min(p50_idx, static_cast<int>(send_latencies.size()) - 1);
  p95_idx = min(p95_idx, static_cast<int>(send_latencies.size()) - 1);
  p99_idx = min(p99_idx, static_cast<int>(send_latencies.size()) - 1);

  double p50 = send_latencies[p50_idx];
  double p95 = send_latencies[p95_idx];
  double p99 = send_latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║    SendJoyControl 统计结果           ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总帧数: " << setw(15) << frame_count << " ║" << endl;
  cout << "║ 目标帧率: " << setw(13) << CONTROL_FREQ << " Hz ║" << endl;
  cout << "║ 实际帧率: " << setw(13) << frame_count / 5.0 << " Hz ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;
}

void TestGetImuData(ControlApiClient& client) {
  const int TEST_COUNT = 100;
  std::vector<double> latencies;
  uint32_t success_count = 0;
  Imu imu_status;

  cout << "\n✅ 开始获取IMU数据测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  for (int i = 1; i <= TEST_COUNT; ++i) {
    auto start = chrono::steady_clock::now();
    uint32_t status = client.GetImuData(imu_status);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] GetImuData 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║      GetImuData 统计结果             ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;

  cout << "\n📋 最后一次获取的数据:" << endl;
  cout << imu_status.DebugString() << endl;
}

void TestGetDiagnosticData(SystemApiClient& client) {
  const int TEST_COUNT = 100;
  std::vector<double> latencies;
  uint32_t success_count = 0;
  DiagnosticMessage diagnostic_msg;

  cout << "\n✅ 开始获取诊断数据测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  for (int i = 1; i <= TEST_COUNT; ++i) {
    auto start = chrono::steady_clock::now();
    uint32_t status = client.GetDiagnosticData(diagnostic_msg);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] GetDiagnosticData 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║    GetDiagnosticData 统计结果        ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;

  cout << "\n📋 最后一次获取的数据:" << endl;
  cout << diagnostic_msg.DebugString() << endl;
}

void TestGetRobotStatus(SystemApiClient& client) {
  const int TEST_COUNT = 100;
  std::vector<double> latencies;
  uint32_t success_count = 0;
  RobotStatus robot_status;

  cout << "\n✅ 开始获取机器人状态测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  for (int i = 1; i <= TEST_COUNT; ++i) {
    auto start = chrono::steady_clock::now();
    uint32_t status = client.GetRobotStatus(robot_status);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] GetRobotStatus 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║    GetRobotStatus 统计结果           ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;

  cout << "\n📋 最后一次获取的数据:" << endl;
  cout << robot_status.DebugString() << endl;
}

void TestGetBatteryPercentage(SystemApiClient& client) {
  const int TEST_COUNT = 100;
  std::vector<double> latencies;
  uint32_t success_count = 0;
  BatteryPercentage battery;

  cout << "\n✅ 开始获取电池电量测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  for (int i = 1; i <= TEST_COUNT; ++i) {
    auto start = chrono::steady_clock::now();
    uint32_t status = client.GetBatteryPercentage(battery);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] GetBatteryPercentage 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║  GetBatteryPercentage 统计结果       ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;

  cout << "\n📋 最后一次获取的数据:" << endl;
  cout << battery.DebugString() << endl;
}

// void TestJoyControl(ControlApiClient& client) {
/*
void TestJoyControl(ControlApiClient& client) {
  const int TEST_COUNT = 50;
  std::vector<double> latencies;
  uint32_t success_count = 0;

  cout << "\n✅ 开始手柄控制测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  Joy joy_template;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "joy_control.pb.txt",
                              joy_template)) {
    return;
  }

  for (int i = 1; i <= TEST_COUNT; ++i) {
    Joy joy_req;
    joy_req.CopyFrom(joy_template);

    auto* header = joy_req.mutable_header();
    header->mutable_stamp()->set_sec(static_cast<uint32_t>(time(nullptr)));
    header->mutable_stamp()->set_nanosec(0);

    JoyControllerResponse joy_resp;

    auto start = chrono::steady_clock::now();
    uint32_t status = client.JoyControl(joy_req, joy_resp);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] JoyControl 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║      JoyControl 统计结果             ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;
}
*/

/*
void TestCmotorcommand(ControlApiClient& client) {
  const int TEST_COUNT = 50;
  std::vector<double> latencies;
  uint32_t success_count = 0;

  cout << "\n✅ 开始单电机控制测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  Cmotorcommand motor_template;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "motor_control.pb.txt",
                              motor_template)) {
    return;
  }

  for (int i = 1; i <= TEST_COUNT; ++i) {
    Cmotorcommand motor_req;
    motor_req.CopyFrom(motor_template);

    JointState motor_resp;

    auto start = chrono::steady_clock::now();
    uint32_t status = client.MotorControl(motor_req, motor_resp);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] MotorControl 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║    MotorControl (RPC) 统计结果       ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;
}
*/

void TestMotorCalibrationRPC(ControlApiClient& client) {
  const int TEST_COUNT = 50;
  std::vector<double> latencies;
  uint32_t success_count = 0;

  cout << "\n✅ 开始电机标定测试..." << endl;
  cout << "    测试次数: " << TEST_COUNT << endl;

  MotorCalibrationRequest motor_template;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "motor_calibration.pb.txt",
                              motor_template)) {
    return;
  }

  for (int i = 1; i <= TEST_COUNT; ++i) {
    MotorCalibrationRequest motor_req;
    motor_req.CopyFrom(motor_template);

    MotorCalibrationResponse motor_resp;

    auto start = chrono::steady_clock::now();
    uint32_t status = client.MotorCalibration(motor_req, motor_resp);
    auto end = chrono::steady_clock::now();

    double latency_ms =
        chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() /
        1000000.0;
    latencies.push_back(latency_ms);

    if (status == 0) {
      success_count++;
    }

    cout << "\r⏳ 进度: " << i << "/" << TEST_COUNT << flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  cout << endl;

  if (latencies.empty()) {
    cout << "[!] MotorCalibration 无数据" << endl;
    return;
  }

  sort(latencies.begin(), latencies.end());
  double min_val = latencies.front();
  double max_val = latencies.back();
  double sum = 0.0;
  for (double v : latencies) sum += v;
  double avg = sum / latencies.size();

  int p50_idx = static_cast<int>(0.50 * latencies.size());
  int p95_idx = static_cast<int>(0.95 * latencies.size());
  int p99_idx = static_cast<int>(0.99 * latencies.size());

  double p50 = latencies[p50_idx];
  double p95 = latencies[p95_idx];
  double p99 = latencies[p99_idx];

  cout << fixed << setprecision(3);
  cout << "\n╔======================================╗" << endl;
  cout << "║  MotorCalibration (RPC) 统计结果     ║" << endl;
  cout << "╠======================================╣" << endl;
  cout << "║ 总请求数: " << setw(13) << TEST_COUNT << " ║" << endl;
  cout << "║ 成功数: " << setw(15) << success_count << " ║" << endl;
  cout << "║ 成功率: " << setw(14) << (success_count * 100.0 / TEST_COUNT)
       << "% ║" << endl;
  cout << "║ 最小延迟: " << setw(12) << min_val << " ms ║" << endl;
  cout << "║ 最大延迟: " << setw(12) << max_val << " ms ║" << endl;
  cout << "║ 平均延迟: " << setw(12) << avg << " ms ║" << endl;
  cout << "║ P50 延迟: " << setw(13) << p50 << " ms ║" << endl;
  cout << "║ P95 延迟: " << setw(13) << p95 << " ms ║" << endl;
  cout << "║ P99 延迟: " << setw(13) << p99 << " ms ║" << endl;
  cout << "╚======================================╝" << endl;
}

void TestSetUWBConfigurator(ControlApiClient& client) {
  cout << "\n✅ 开始 UWB 硬件配置测试..." << endl;

  UWBRequestHardware uwb_hw_req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "uwb_hardware.pb.txt",
                              uwb_hw_req)) {
    return;
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  uint32_t status = client.SetUWBConfigurator(uwb_hw_req);
  auto t2 = std::chrono::high_resolution_clock::now();
  cout << "⏱ SetUWBConfigurator 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
  if (status == 0) {
    cout << "✅ UWB 硬件配置成功" << endl;
  } else {
    cout << "[❌] UWB 硬件配置失败，状态码: " << status << endl;
  }
}

void TestSetUWBConfigManager(ControlApiClient& client) {
  cout << "\n✅ 开始 UWB 配置管理测试..." << endl;

  UWBRequestConfig uwb_cfg_req;
  if (!ReadProtoFromTextFile(std::string(kDataDir) + "uwb_config.pb.txt",
                              uwb_cfg_req)) {
    return;
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  uint32_t status = client.SetUWBConfigManager(uwb_cfg_req);
  auto t2 = std::chrono::high_resolution_clock::now();
  cout << "⏱ SetUWBConfigManager 耗时: " << std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms" << endl;
  if (status == 0) {
    cout << "✅ UWB 配置管理成功" << endl;
  } else {
    cout << "[❌] UWB 配置管理失败，状态码: " << status << endl;
  }
}

void TestSubscribeUWBData(ControlApiClient& client) {
  const int TEST_DURATION_SEC = 5;

  auto uwb_stats = std::make_unique<LatencyStats>();
  uwb_stats->reset();

  cout << "\n✅ 开始 UWB 数据订阅测试..." << endl;
  cout << "    测试时长: " << TEST_DURATION_SEC << " 秒" << endl;

  last_uwb_system_time.store(0);

  uint64_t sub_id = client.SubscribeUWBData(
      [uwb_stats_ptr = uwb_stats.get()](const std::shared_ptr<const LinktrackNodeframe7>& result) {
        OnUWBBenchmark(result, uwb_stats_ptr);
      });
  if (sub_id == 0) {
    cout << "\n[❌] UWB 数据订阅失败" << endl;
    return;
  }
  cout << "✅ UWB 数据订阅成功, ID: " << sub_id << endl;

  for (int i = 1; i <= TEST_DURATION_SEC; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    printTestProgress(i, "UWB");
  }
  cout << endl;

  uwb_stats->stop();
  client.UnsubscribeUWBData(sub_id);

  uwb_stats->printStats("UWB 数据订阅");
}

void TestAllInterfaces(ControlApiClient& client, SystemApiClient& system_client) {
  cout << "\n🚀 开始测试所有接口..." << endl;

  // 订阅接口测试
  TestSingleInterface(client, "imu", "IMU");
  TestSingleInterface(client, "joy", "Joy");
  TestSingleInterface(client, "joint", "JointState");

  // 请求接口测试
  TestGetImuData(client);
  TestGetDiagnosticData(system_client);
  TestGetRobotStatus(system_client);
  TestGetBatteryPercentage(system_client);
  // TestJoyControl(client);
  // TestCmotorcommand(client);
  TestMotorCalibrationRPC(client);

  // 发布接口测试（Zenoh）
  TestSendJoyControl(client);
  TestMotorControl(client);

  // UWB 接口测试
  TestSetUWBConfigurator(client);
  TestSetUWBConfigManager(client);
  TestSubscribeUWBData(client);

  cout << "\n🎉 所有接口测试完成！" << endl;
}

int main() {
  cout << "===== 正在初始化 TANGPA API 性能测试工具 =====" << endl;

  try {
    std::unique_ptr<ControlApiClient> tangpa_client;
    std::unique_ptr<SystemApiClient> system_client;
    std::string server_address = "localhost:50051";

    tangpa_client = std::make_unique<ControlApiClient>();
    system_client = std::make_unique<SystemApiClient>();
    if (!tangpa_client->Connect(server_address)) {
      cerr << "[ERROR] 连接服务端失败！" << endl;
      return -1;
    }
    if (!system_client->Connect(server_address)) {
      cerr << "[ERROR] SystemApiClient 连接服务端失败！" << endl;
      return -1;
    }
    cout << "[INFO] 服务端连接成功！" << endl;
    cout << "[INFO] 按 Enter 键继续..." << endl;
    cin.get();

    string input;
    while (true) {
      printMenu();

      cin >> input;
      cin.ignore();

      if (input == "0") {
        cout << "\n👋 感谢使用，再见！" << endl;
        return 0;
      } else if (input == "1") {
        TestSingleInterface(*tangpa_client, "imu", "IMU");
      } else if (input == "2") {
        TestSingleInterface(*tangpa_client, "joy", "Joy");
      } else if (input == "3") {
        TestSingleInterface(*tangpa_client, "joint", "JointState");
      } else if (input == "4") {
        TestSubscribeUWBData(*tangpa_client);
      } else if (input == "5") {
        TestSetUWBConfigurator(*tangpa_client);
      } else if (input == "6") {
        TestSetUWBConfigManager(*tangpa_client);
      } else if (input == "7") {
        TestGetImuData(*tangpa_client);
      } else if (input == "8") {
        TestGetDiagnosticData(*system_client);
      } else if (input == "9") {
        TestGetRobotStatus(*system_client);
      } else if (input == "10") {
        TestGetBatteryPercentage(*system_client);
      } else if (input == "11") {
        // TestJoyControl(*tangpa_client);
      } else if (input == "12") {
        // TestCmotorcommand(*tangpa_client);
      } else if (input == "13") {
        TestSendJoyControl(*tangpa_client);
      } else if (input == "14") {
        TestMotorControl(*tangpa_client);
      } else if (input == "15") {
        TestMotorCalibrationRPC(*tangpa_client);
      } else if (input == "A" || input == "a") {
        TestAllInterfaces(*tangpa_client, *system_client);
      } else {
        cout << "\n❌ 无效选项，请输入 0-15 或 A/a" << endl;
        continue;
      }

      cout << "\n按 Enter 键返回主菜单..." << endl;
      cin.get();
    }

  } catch (const exception& e) {
    cerr << "\n[💥] 程序异常: " << e.what() << endl;
    return 1;
  }

  return 0;
}