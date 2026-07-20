#include "chric/robot/client/interfaces_client.h"
#include "api_service/common/interfaces_request_response.pb.h"
#include "common/variant.pb.h"

#include "chric/robot/tangpa/tangpa_control_api.h"
#include "chric/robot/tangpa/tangpa_system_api.h"
#include <iostream>
#include <thread>

using namespace humanoid_robot::sdk_client::robot::tangpa_control_api;
using namespace humanoid_robot::sdk_client::robot::tangpa_system_api;
using namespace humanoid_robot::PB::api_service::common;
using namespace std;


// UWB数据回调函数 
void OnUWBStreamResult(const std::shared_ptr<const LinktrackNodeframe7>& frame)
{
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();

    cout << "\n==================================================" << endl;
    cout << "[⏰ 时间戳] " << ms << " ms" << endl;
    cout << "[📥 收到 UWB 数据]" << endl;

    cout << "├─ 角色 role: " << frame->role() << endl;
    cout << "├─ 设备 ID: " << frame->id() << endl;
    cout << "├─ 本地时间: " << frame->local_time() << endl;
    cout << "├─ 系统时间: " << frame->system_time() << endl;
    cout << "├─ 电压: " << frame->voltage() << " V" << endl;
    cout << "└─ 节点数量: " << frame->nodes_size() << endl;

    // 打印所有节点
    for (int i = 0; i < frame->nodes_size(); ++i) {
        const auto& node = frame->nodes(i);
        cout << "\n  === 节点 " << i+1 << " ===" << endl;
        cout << "  角色 role: " << node.role() << endl;
        cout << "  ID: " << node.id() << endl;
        cout << "  距离: " << node.dis() << " m" << endl;
        cout << "  角度0: " << node.angle0() << " °" << endl;
        cout << "  角度1: " << node.angle1() << " °" << endl;
        cout << "  FP RSSI: " << node.fp_rssi() << endl;
        cout << "  RX RSSI: " << node.rx_rssi() << endl;
    }

    cout << "==================================================\n" << endl;
}

static void test_get_imu_data(std::unique_ptr<ControlApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "        测试：获取IMU数据" << endl;
    cout << "=====================================" << endl;

    humanoid_robot::PB::ros2::sensor_msgs::Imu imu_status;
    // 修复：改为成员函数调用
    uint32_t status = client->GetImuData(imu_status);

    cout << "[✅] IMU数据获取完成，状态码: " << status << endl;
    if (status == 0) {  // 假设 0 表示成功，根据实际 TangPaStatus 调整
        // 打印 Header
        const auto& header = imu_status.header();
        cout << "├─ 序列号 seq: " << header.seq() << endl;
        cout << "├─ 坐标系 frame_id: " << header.frame_id() << endl;

        // 打印姿态 (四元数)
        const auto& ori = imu_status.orientation();
        cout << "├─ 姿态 orientation: x=" << ori.x() 
             << ", y=" << ori.y() 
             << ", z=" << ori.z() 
             << ", w=" << ori.w() << endl;

        // 打印角速度
        const auto& angular = imu_status.angular_velocity();
        cout << "├─ 角速度 angular_velocity: x=" << angular.x() 
             << ", y=" << angular.y() 
             << ", z=" << angular.z() << endl;

        // 打印线加速度
        const auto& linear = imu_status.linear_acceleration();
        cout << "└─ 线加速度 linear_acceleration: x=" << linear.x() 
             << ", y=" << linear.y() 
             << ", z=" << linear.z() << endl;
    } else {
        cerr << "[❌] 获取IMU数据失败" << endl;
    }
    this_thread::sleep_for(chrono::milliseconds(500));
}

static void test_get_diagnostic_data(std::unique_ptr<SystemApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "      测试：获取机器人诊断数据" << endl;
    cout << "=====================================" << endl;

    DiagnosticMessage diagnostic_msg;
    // 修复：改为成员函数调用
    uint32_t status = client->GetDiagnosticData(diagnostic_msg);

    cout << "[✅] 诊断数据获取完成，状态码: " << status << endl;

    if (status == 0) {
        cout << "├─ 执行结果 result: " << diagnostic_msg.result() << endl;

        const auto& diag_array = diagnostic_msg.messages();

        const auto& header = diag_array.header();
        cout << "├─ 时间戳: " << header.stamp().sec() << "." << header.stamp().nanosec() << endl;
        cout << "├─ frame_id: " << header.frame_id() << endl;

        int status_cnt = diag_array.status_size();
        cout << "└─ 诊断状态数量: " << status_cnt << endl;

        for (int i = 0; i < status_cnt; ++i) {
            const auto& stat = diag_array.status(i);
            cout << "\n===== 诊断项 " << i+1 << " =====" << endl;
            cout << "  级别(0=OK,1=WARN,2=ERROR): " << stat.level() << endl;
            cout << "  名称: " << stat.name() << endl;
            cout << "  信息: " << stat.message() << endl;
            cout << "  硬件ID: " << stat.hardware_id() << endl;

            // 打印 KeyValue 信息
            for (int j = 0; j < stat.values_size(); ++j) {
                const auto& kv = stat.values(j);
                cout << "    " << kv.key() << " = " << kv.value() << endl;
            }
        }
    } else {
        cerr << "[❌] 获取诊断数据失败" << endl;
    }

    this_thread::sleep_for(chrono::milliseconds(500));
}

static void test_get_robot_status(std::unique_ptr<SystemApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "      测试：获取机器人整体状态" << endl;
    cout << "=====================================" << endl;

    RobotStatus robot_status;
    // 修复：改为成员函数调用
    uint32_t status = client->GetRobotStatus(robot_status);

    cout << "[✅] 机器人状态获取完成，状态码: " << status << endl;
    // if (status == 0) {
    //     cout << "├─ 运行模式: " << robot_status.run_mode() << endl;
    //     cout << "├─ 工作状态: " << robot_status.work_state() << endl;
    //     cout << "├─ 电机使能: " << (robot_status.motor_enable() ? "已使能" : "未使能") << endl;
    //     cout << "└─ 控制模式: " << robot_status.control_mode() << endl;
    // } else {
    //     cerr << "[❌] 获取机器人状态失败" << endl;
    // }
    this_thread::sleep_for(chrono::milliseconds(500));
}

/*
static void test_joy_control(std::unique_ptr<ControlApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "        测试：手柄遥控控制" << endl;
    cout << "=====================================" << endl;

    Joy joy_req;

    auto* header = joy_req.mutable_header();
    header->mutable_stamp()->set_sec(static_cast<uint32_t>(time(nullptr)));
    header->mutable_stamp()->set_nanosec(0);
    header->set_frame_id("base_link");

    joy_req.add_axes(0.5f);   // 前向
    joy_req.add_axes(0.0f);   // 侧移
    joy_req.add_axes(0.0f);   // 旋转

    joy_req.add_buttons(1);   // 使能键按下
    joy_req.add_buttons(0);   // 其他按键

    JoyControllerResponse joy_resp;
    // 修复：改为成员函数调用
    uint32_t status = client->JoyControl(joy_req, joy_resp);

    cout << "[✅] 手柄控制完成，状态码: " << status << endl;

    if (status == 0) {
        const auto& diagnostic_array = joy_resp.results();
        cout << "├─ 诊断状态数量: " << diagnostic_array.status_size() << endl;

        for (int i = 0; i < diagnostic_array.status_size(); ++i) {
            const auto& status_msg = diagnostic_array.status(i);
            
            cout << "└── 状态 [" << i << "]:" << endl;
            cout << "       级别: " << DiagnosticStatus_Level_Name(status_msg.level()) << endl;
            cout << "       名称: " << status_msg.name() << endl;
            cout << "       信息: " << status_msg.message() << endl;
            cout << "       硬件ID: " << status_msg.hardware_id() << endl;
        }
    } else {
        cerr << "[❌] 手柄控制失败" << endl;
    }

    this_thread::sleep_for(chrono::milliseconds(500));
}
*/
/*
static void test_motor_control(std::unique_ptr<ControlApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "        测试：单电机控制" << endl;
    cout << "=====================================" << endl;

    Cmotorcommand motor_req;

    motor_req.add_motor_ids(1);                  // 电机 ID = 1
    motor_req.add_control_modes(1);              // 控制模式：1=位置控制
    motor_req.add_positions(100.0f);             // 目标位置
    motor_req.add_speeds(50.0f);                 // 目标速度
    motor_req.add_torques(10.0f);                // 目标力矩
    motor_req.add_kps(10.0f);                    // KP
    motor_req.add_kds(1.0f);                     // KD

    // 修复：改为成员函数调用
    JointState motor_resp;
    uint32_t status = client->MotorControl(motor_req, motor_resp);

    cout << "[✅] 电机控制完成，状态码: " << status << endl;

    if (status == 0) {
        // 打印返回的关节数据（服务端返回的是 JointState）
        cout << "├─ 返回关节数量：" << motor_resp.name_size() << endl;

        // 遍历数据（单电机就一个）
        for (int i = 0; i < motor_resp.name_size(); ++i) {
            cout << "├─ 关节名称: " << motor_resp.name(i) << endl;
            cout << "├─ 当前位置: " << motor_resp.position(i) << endl;
            cout << "├─ 当前速度: " << motor_resp.velocity(i) << endl;
            cout << "└─ 当前力矩: " << motor_resp.effort(i) << endl;
        }
    } else {
        cerr << "[❌] 电机控制失败" << endl;
    }

    this_thread::sleep_for(chrono::milliseconds(1000));
}
*/

static void test_get_battery_percentage(std::unique_ptr<SystemApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "      测试：获取电池电量" << endl;
    cout << "=====================================" << endl;

    BatteryPercentage battery;
    // 修复：改为成员函数调用
    uint32_t status = client->GetBatteryPercentage(battery);

    cout << "[✅] 电池电量获取完成，状态码: " << status << endl;
    // if (status == 0) {
    //     cout << "├─ 电量百分比: " << battery.percentage() << " %" << endl;
    //     cout << "├─ 电压: " << battery.voltage() << " V" << endl;
    //     cout << "├─ 电流: " << battery.current() << " A" << endl;
    //     cout << "└─ 温度: " << battery.temperature() << " ℃" << endl;
    // } else {
    //     cerr << "[❌] 获取电池电量失败" << endl;
    // }
    this_thread::sleep_for(chrono::milliseconds(500));
}


static void test_set_uwb_configurator(std::unique_ptr<ControlApiClient>& client)
{
    cout << "\n=====================================" << endl;
    cout << "    测试：UWB 硬件参数配置" << endl;
    cout << "=====================================" << endl;

    UWBRequestHardware uwb_hw_req;
    uwb_hw_req.set_target_device("/dev/uwb_tag");  // 串口设备
    uwb_hw_req.set_target_role(1);                 // 角色
    uwb_hw_req.set_baud(921600);                   // 波特率

    uint32_t status = client->SetUWBConfigurator(uwb_hw_req);
    cout << "[✅] UWB 硬件配置完成，状态码: " << status << endl;

    uint64_t sub_id = client->SubscribeUWBData(OnUWBStreamResult);
    cout << "[✅] UWB 数据订阅已启动，订阅ID: " << sub_id << endl;
    this_thread::sleep_for(chrono::seconds(3));  // 等待接收3秒数据
    client->UnsubscribeUWBData(sub_id);
}


int main() {

    // 修复：创建 ControlApiClient 实例（继承自 InterfacesClient）
    unique_ptr<ControlApiClient> client = make_unique<ControlApiClient>();
    auto conn_status = client->Connect("localhost:50051");

    if (!conn_status) {
            cerr << "[❌] 连接服务端失败: " << conn_status.message() << endl;
            return 1;
        }
        cout << "[✅] 成功连接机器人服务端！" << endl;
        this_thread::sleep_for(chrono::milliseconds(500));
    
    unique_ptr<SystemApiClient> system_client = make_unique<SystemApiClient>();
    auto sys_conn_status = system_client->Connect("localhost:50051");
    if (!sys_conn_status) {
        cerr << "[❌] SystemApiClient 连接服务端失败: " << sys_conn_status.message() << endl;
        return 1;
    }
    // Zenoh 会话由 SDK 构造时自动初始化，无需手动管理
    
    uint64_t imu_sub_id = client->SubscribeImu([](const std::shared_ptr<const Imu>& imu_result) {
        cout << "[📥 收到 IMU 流式数据]" << endl;

        // Header
        cout << "├─ 时间戳: " << imu_result->header().stamp().sec() << "." 
            << imu_result->header().stamp().nanosec() << endl;
        cout << "├─ Frame ID: " << imu_result->header().frame_id() << endl;

        // 姿态四元数
        cout << "├─ 姿态四元数: w=" << imu_result->orientation().w() 
            << "  x=" << imu_result->orientation().x()
            << "  y=" << imu_result->orientation().y()
            << "  z=" << imu_result->orientation().z() << endl;

        // 角速度
        cout << "├─ 角速度: x=" << imu_result->angular_velocity().x() 
            << "  y=" << imu_result->angular_velocity().y()
            << "  z=" << imu_result->angular_velocity().z() << " rad/s" << endl;

        // 线加速度
        cout << "└─ 线加速度: x=" << imu_result->linear_acceleration().x() 
            << "  y=" << imu_result->linear_acceleration().y()
            << "  z=" << imu_result->linear_acceleration().z() << " m/s²" << endl;
        cout << "==================================================\n" << endl;
    });
    
    uint64_t joy_sub_id = client->SubscribeJoy([](const std::shared_ptr<const Joy>& joy_result) {
        cout << "[📥 收到 Joy 手柄流式数据]" << endl;

        // Header
        cout << "├─ 时间戳: " << joy_result->header().stamp().sec() << "." 
            << joy_result->header().stamp().nanosec() << endl;
        cout << "├─ Frame ID: " << joy_result->header().frame_id() << endl;

        // 摇杆 axes
        cout << "├─ 摇杆 axes 数量: " << joy_result->axes_size() << endl;
        for (int i = 0; i < joy_result->axes_size(); ++i) {
            cout << "│   ├─ axes[" << i << "] = " << joy_result->axes(i) << endl;
        }

        // 按钮 buttons
        cout << "├─ 按钮 buttons 数量: " << joy_result->buttons_size() << endl;
        for (int i = 0; i < joy_result->buttons_size(); ++i) {
            cout << "│   ├─ buttons[" << i << "] = " << joy_result->buttons(i) << endl;
        }

        cout << "==================================================\n" << endl;
    });

    uint64_t joint_state_sub_id = client->SubscribeJointState([](const std::shared_ptr<const JointState>& joint_result) {
        cout << "[📥 收到 关节状态 流式数据]" << endl;

        // Header
        cout << "├─ 时间戳: " << joint_result->header().stamp().sec() << "." 
            << joint_result->header().stamp().nanosec() << endl;
        cout << "├─ Frame ID: " << joint_result->header().frame_id() << endl;
        cout << "├─ 关节数量: " << joint_result->name_size() << endl;

        // 遍历关节
        for (int i = 0; i < joint_result->name_size(); ++i) {
            cout << "\n  --- 关节 " << i + 1 << " ---" << endl;
            cout << "  名称: " << joint_result->name(i) << endl;
            cout << "  位置: " << joint_result->position(i) << " rad" << endl;
            cout << "  速度: " << joint_result->velocity(i) << " rad/s" << endl;
            cout << "  力矩: " << joint_result->effort(i) << " Nm" << endl;
        }

        cout << "==================================================\n" << endl;
    });

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 修复：SendMotorControl 参数类型应为 Cmotorcommand，不是 cmotor_interface::msg::Cmotorcommand
    Cmotorcommand motor_cmd;
    motor_cmd.add_motor_ids(0);
    motor_cmd.add_motor_ids(1);
    motor_cmd.add_motor_ids(2);
    motor_cmd.add_control_modes(1);  // 位置模式
    motor_cmd.add_control_modes(1);
    motor_cmd.add_control_modes(1);
    motor_cmd.add_positions(0.0f);
    motor_cmd.add_positions(0.5f);
    motor_cmd.add_positions(-0.5f);
    motor_cmd.add_kps(100.0f);
    motor_cmd.add_kps(100.0f);
    motor_cmd.add_kps(100.0f);
    motor_cmd.add_kds(10.0f);
    motor_cmd.add_kds(10.0f);
    motor_cmd.add_kds(10.0f);
    
    client->SendMotorControl(motor_cmd);
    
    // MotorCalibration 测试
    MotorCalibrationRequest calib_req;
    calib_req.set_motor_id(1);
    MotorCalibrationResponse calib_resp;
    uint32_t calib_status = client->MotorCalibration(calib_req, calib_resp);
    cout << "[✅] 电机标定完成，状态码: " << calib_status << endl;
    if (calib_status == 0) {
        cout << "├─ state: " << calib_resp.state() << endl;
        cout << "├─ motor_id: " << calib_resp.motor_id() << endl;
        cout << "└─ message: " << calib_resp.message() << endl;
    }

    client->UnsubscribeImu(imu_sub_id);
    client->UnsubscribeJoy(joy_sub_id);
    client->UnsubscribeJointState(joint_state_sub_id);
    
    // Zenoh 会话由 SDK 析构时自动关闭，无需手动管理
    return 0;
}