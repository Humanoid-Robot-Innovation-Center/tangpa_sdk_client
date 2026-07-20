# Tangpa SDK 使用教程

## 1. 概述

Tangpa SDK 提供 `ControlApiClient` 和 `SystemApiClient` 两个客户端类，用于与 Tangpa 机器人进行通信和控制。

| 客户端 | 职责 |
|--------|------|
| `ControlApiClient` | 控制指令、数据订阅、UWB 配置 |
| `SystemApiClient` | 系统诊断、机器人状态、电池电量 |

## 2. 快速开始

### 2.1 CMake 工程配置

SDK 提供了 CMake 包配置文件，通过 `find_package` 即可引入。

**项目目录结构：**

```
my_project/
  CMakeLists.txt        # 顶层 CMake
  main.cpp              # 源代码
```

**CMakeLists.txt：**

```cmake
cmake_minimum_required(VERSION 3.8)
project("my_project" VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 将 SDK 根目录加入搜索路径（指向 SDK 包的顶级目录）
list(APPEND CMAKE_PREFIX_PATH "/path/to/tangpa_sdk_client")
find_package(tangpa_sdk_client REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE tangpa_sdk_client::chric_tangpa_sdk_module_api)
```

**构建：**

```bash
cd my_project
mkdir build && cd build
cmake ..
make -j$(nproc)
```

> **关于运行时库路径**：SDK 的所有 .so 均内置 `$ORIGIN` 相对 RPATH，无需设置 `LD_LIBRARY_PATH`。将 SDK 目录整体移动到任意位置均可正常运行。

### 2.2 就地构建（SDK 自带的 CMake 模板）

SDK 包内已包含 CMake 模板项目，可直接在此基础上开发：

```cmake
# 指向 SDK 自己
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}")
find_package(tangpa_sdk_client REQUIRED)
```

就地构建时，CMake 会为二进制注入 SDK 库的绝对路径 RUNPATH。如果不需要，请在 `find_package` 之前设置：

```cmake
set(CMAKE_SKIP_BUILD_RPATH ON)
```

### 2.3 示例代码

```cpp
#include "chric/robot/tangpa/tangpa_control_api.h"
#include "chric/robot/tangpa/tangpa_system_api.h"

using namespace humanoid_robot::sdk_client::robot;

int main() {
    auto control_client = std::make_unique<tangpa_control_api::ControlApiClient>();
    auto system_client = std::make_unique<tangpa_system_api::SystemApiClient>();

    Status status = control_client->Connect("localhost", 50051);
    if (!status) {
        std::cerr << "Failed to connect: " << status.message() << std::endl;
        return 1;
    }
    system_client->Connect("localhost", 50051);

    // 订阅 IMU 数据
    uint64_t sub_id = control_client->SubscribeImu(
        [](const std::shared_ptr<const tangpa_control_api::Imu>& imu) {
            std::cout << "Angular velocity: " << imu->angular_velocity().x() << std::endl;
        });

    std::this_thread::sleep_for(std::chrono::seconds(10));

    control_client->UnsubscribeImu(sub_id);
    control_client->Disconnect();

    return 0;
}
```

> SDK 提供了完整的示例程序，位于 `examples/` 目录。运行方法请参考 [7. 运行示例](#7-运行示例)。

## 3. 配置说明

SDK 通过 `config/config.yaml` 管理配置。`InterfacesClient` 构造时会自动加载并初始化日志系统。

### 3.1 gRPC 客户端

```yaml
grpc_client:
  connection:
    default_target: "192.168.1.100:50051"
    channel_ready_timeout_ms: 5000    # 通道就绪超时 (ms)
  channel:
    max_receive_mb: 100               # 最大接收消息大小 (MB)
    max_send_mb: 100                  # 最大发送消息大小 (MB)
```

### 3.2 日志

```yaml
logging:
  log_level: 3                        # 0=FATAL, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG
  log_dir: "SDK-Client/logs"          # 日志目录（相对可执行程序路径或绝对路径）
  max_file_size_mb: 3                 # 单文件最大大小 (MB)
  max_files_per_level: 10             # 每级别最多保留文件数
  enable_async: true                  # true=异步写, false=同步写
  enable_console_output: true         # true=控制台输出
  enable_simple_output: true          # true=简洁格式, false=详细格式（含文件、行号、函数名）
```

日志按级别分目录输出，文件命名格式为 `log_<pid>_<日期>_<时间>_<纳秒>.txt`：

```
SDK-Client/logs/
  fatal/
  error/
  warning/
  info/
  debug/
```

**编程方式控制（在创建任何 SDK 对象之前调用）：**

```cpp
#include "chric/common/Log/wlog.hpp"

// 使用自定义配置文件
WLogInitWithConfig("path/to/my_config.yaml");

// 或使用默认配置（DEBUG 级别，自动生成日志目录）
WLogInit();
```

## 4. ControlApiClient API

### 4.1 连接管理

```cpp
Status Connect(const std::string& server_address, int port);
Status Connect(const std::string& target);
Status Connect();
void Disconnect();
bool IsConnected() const;
```

| 重载 | 说明 |
|------|------|
| `Connect(address, port)` | 指定地址和端口连接 |
| `Connect(target)` | 指定完整目标字符串连接（如 `"192.168.0.1:50051"`） |
| `Connect()` | 读取 `config/config.yaml` 中 `grpc_client.connection.default_target` 自动连接 |
| `Disconnect()` | 断开连接 |

> **注意：** 已连接时重复调用 `Connect` 会返回错误，需先调用 `Disconnect()`。

```cpp
uint32_t GetImuData(Imu& imu_status);
```

获取一次 IMU 数据快照。`imu_status` 为 [sensor_msgs/Imu](https://docs.ros2.org/latest/api/sensor_msgs/msg/Imu.html)（[proto](data/proto/ros2/sensor_msgs/Imu.proto)）。网络通信超时 5 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| IMU 子系统错误，未获取到数据 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 接口层通信错误，返回数据序列化失败 | `ERROR_SERIALIZE_FAILED` | `0x00000003` |
| 接口层服务端异常 | `ERROR_FAILED` | `0x7FFFFFFF` |
| 成功 | `STATUS_OK` | `0x0` |

### 4.3 控制接口（gRPC）

#### MotorCalibration

```cpp
uint32_t MotorCalibration(const MotorCalibrationRequest& request, MotorCalibrationResponse& response);
```

对指定电机（`motor_id`）执行标定。`request` 为 `MotorCalibrationRequest`（[proto](data/proto/api_service/tangpa/proto/control.proto)），包含 motor_id；`response` 为 `MotorCalibrationResponse`（[proto](data/proto/api_service/tangpa/proto/control.proto)），含 state、motor_id、message。服务超时 5 秒，网络通信超时 30 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| 接口层通信错误，参数解析失败 | `ERROR_INVALID_PARAMETER` | `0x00200006` |
| 接口层通信错误，参数反序列化失败 | `ERROR_DESERIALIZE_FAILED` | `0x00000004` |
| 系统接口层服务端错误，响应序列化失败 | `ERROR_SERIALIZE_FAILED` | `0x00000003` |
| 系统接口层服务端超时（有/无缓存） | `ERROR_TIMEOUT` | `0x00000001` |
| 系统接口层服务端，异常 | `ERROR_FAILED` | `0x7FFFFFFF` |
| 收到子系统 succeeded/failed/rejected | `STATUS_OK` | `0x0` |

### 4.4 控制接口（发布）

```cpp
uint32_t SendMotorControl(const Cmotorcommand& command);
uint32_t SendJoyControl(const Joy& command);
```

非阻塞，直接返回。适用高频控制场景，无需超时。

### 4.5 订阅接口

```cpp
uint64_t SubscribeImu(const std::function<void(const std::shared_ptr<const Imu>&)>& callback);
uint64_t SubscribeJoy(const std::function<void(const std::shared_ptr<const Joy>&)>& callback);
uint64_t SubscribeJointState(const std::function<void(const std::shared_ptr<const JointState>&)>& callback);
```

| 回调参数类型 | proto |
|-------------|-------|
| `Imu` | [Imu.proto](data/proto/ros2/sensor_msgs/Imu.proto) |
| `Joy` | [Joy.proto](data/proto/ros2/sensor_msgs/Joy.proto) |
| `JointState` | [JointState.proto](data/proto/ros2/sensor_msgs/JointState.proto) |

| 返回值 | 说明 |
|--------|------|
| `> 0` | 订阅 ID，用于取消订阅 |
| `0` | 订阅失败 |

```cpp
uint32_t UnsubscribeImu(uint64_t subscription_id);
uint32_t UnsubscribeJoy(uint64_t subscription_id);
uint32_t UnsubscribeJointState(uint64_t subscription_id);
```

SDK 自动管理数据流：首次订阅启用服务端发布，最后取消时停止发布。订阅/取消约 5 秒。

### 4.6 UWB 配置与数据订阅

#### SetUWBConfigurator

```cpp
uint32_t SetUWBConfigurator(const UWBRequestHardware& request);
```

配置 UWB 硬件参数。`request` 为 `UWBRequestHardware`（[proto](data/proto/api_service/tangpa/proto/control.proto)），含 target_device、target_role、baud。服务超时 5 秒，网络通信超时 30 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| 接口层通信错误，参数解析失败 | `ERROR_INVALID_PARAMETER` | `0x00200006` |
| 接口层通信错误，参数反序列化失败 | `ERROR_DESERIALIZE_FAILED` | `0x00000004` |
| 子系统错误，configurator client 超时 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| configurator 子系统错误，参数设置失败 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| trigger client 子系统错误，超时 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 子系统错误，trigger 执行失败 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端，异常 | `ERROR_FAILED` | `0x7FFFFFFF` |
| 成功 | `STATUS_OK` | `0x0` |

#### SetUWBConfigManager

```cpp
uint32_t SetUWBConfigManager(const UWBRequestConfig& request);
```

配置 UWB 跟随参数。`request` 为 `UWBRequestConfig`（[proto](data/proto/api_service/tangpa/proto/control.proto)），含 target_role、target_distance。服务超时 5 秒，网络通信超时 30 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| 接口层通信错误，参数解析失败 | `ERROR_INVALID_PARAMETER` | `0x00200006` |
| 接口层通信错误，参数反序列化失败 | `ERROR_DESERIALIZE_FAILED` | `0x00000004` |
| 子系统错误，config client 超时 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 子系统错误，config 参数设置失败 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端，异常 | `ERROR_FAILED` | `0x7FFFFFFF` |
| 成功 | `STATUS_OK` | `0x0` |

#### SubscribeUWBData

```cpp
uint64_t SubscribeUWBData(const std::function<void(const std::shared_ptr<const LinktrackNodeframe7>&)>& callback);
```

订阅 UWB 定位数据。`callback` 参数为 `LinktrackNodeframe7`（[proto](data/proto/tangpa_ros2_interfaces/control/nlink_parser/LinktrackNodeframe7.proto)），含 role、id、nodes 等字段。通过 zenoh 频道 `zenoh/nlink_linktrack_nodeframe7` 接收数据。SDK 自动管理数据流：首次订阅启用服务端发布，最后取消时停止发布。约 5 秒。

| 返回值 | 说明 |
|--------|------|
| `> 0` | 订阅 ID，用于取消订阅 |
| `0` | 订阅失败 |

#### UnsubscribeUWBData

```cpp
uint32_t UnsubscribeUWBData(uint64_t subscription_id);
```

取消 UWB 定位数据订阅。约 5 秒。

## 5. SystemApiClient API

### 5.1 连接管理

```cpp
Status Connect(const std::string& server_address, int port);
Status Connect(const std::string& target);
Status Connect();
void Disconnect();
bool IsConnected() const;
```

| 重载 | 说明 |
|------|------|
| `Connect(address, port)` | 指定地址和端口连接 |
| `Connect(target)` | 指定完整目标字符串连接（如 `"192.168.0.1:50051"`） |
| `Connect()` | 读取 `config/config.yaml` 中 `grpc_client.connection.default_target` 自动连接 |
| `Disconnect()` | 断开连接 |

> **注意：** 已连接时重复调用 `Connect` 会返回错误，需先调用 `Disconnect()`。

### 5.2 数据获取

#### GetDiagnosticData

```cpp
uint32_t GetDiagnosticData(DiagnosticMessage& diagnostic_message);
```

获取诊断数据。`diagnostic_message` 为 `DiagnosticMessage`（[proto](data/proto/api_service/tangpa/proto/system.proto)），含 messages（DiagnosticArray）和 result（int32）。网络通信超时 5 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| 子系统错误，未获取到数据 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端错误，序列化失败 | `ERROR_SERIALIZE_FAILED` | `0x00000003` |
| 系统接口层服务端异常 | `ERROR_FAILED` | `0x7FFFFFFF` |
| 成功 | `STATUS_OK` | `0x0` |

#### GetRobotStatus

```cpp
uint32_t GetRobotStatus(RobotStatus& robot_status);
```

获取机器人状态。`robot_status` 为 `RobotStatus`（[proto](data/proto/api_service/tangpa/proto/system.proto)），含 state_id、state_desc、battery、error_codes。网络通信超时 5 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| 子系统错误，未获取到数据 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端错误，序列化失败 | `ERROR_SERIALIZE_FAILED` | `0x00000003` |
| 系统接口层服务端异常（含 JSON 解析异常） | `ERROR_FAILED` | `0x7FFFFFFF` |
| 成功 | `STATUS_OK` | `0x0` |

#### GetBatteryPercentage

```cpp
uint32_t GetBatteryPercentage(BatteryPercentage& battery_percentage);
```

获取电池电量百分比。`battery_percentage` 为 `BatteryPercentage`（[proto](data/proto/api_service/tangpa/proto/system.proto)），含 info（电池信息）。网络通信超时 5 秒。

**返回码：**

| 场景 | 返回码 | 值 |
|------|--------|-----|
| 子系统错误，未获取到数据 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端错误，序列化失败 | `ERROR_SERIALIZE_FAILED` | `0x00000003` |
| 系统接口层服务端异常 | `ERROR_FAILED` | `0x7FFFFFFF` |
| 成功 | `STATUS_OK` | `0x0` |

---

## 6. 示例代码

### 6.1 获取诊断数据

```cpp
auto system_client = std::make_unique<tangpa_system_api::SystemApiClient>();
system_client->Connect("localhost", 50051);

tangpa_system_api::DiagnosticMessage msg;
uint32_t result = system_client->GetDiagnosticData(msg);
if (result == 0) {
    std::cout << "Diagnostic: " << msg.DebugString() << std::endl;
}
system_client->Disconnect();
```

### 6.2 发送电机控制命令

```cpp
auto client = std::make_unique<tangpa_control_api::ControlApiClient>();
client->Connect("localhost", 50051);

tangpa_control_api::Cmotorcommand cmd;
cmd.add_motor_ids(1);
cmd.add_control_modes(0);
cmd.add_positions(1.57f);
cmd.add_speeds(0.5f);
cmd.add_torques(10.0f);
cmd.add_kps(100.0f);
cmd.add_kds(5.0f);

uint32_t result = client->SendMotorControl(cmd);
client->Disconnect();
```

### 6.3 订阅关节状态

```cpp
auto client = std::make_unique<tangpa_control_api::ControlApiClient>();
client->Connect("localhost", 50051);

uint64_t sub_id = client->SubscribeJointState(
    [](const std::shared_ptr<const tangpa_control_api::JointState>& state) {
        for (int i = 0; i < state->name_size(); ++i) {
            std::cout << state->name(i) << ": " << state->position(i) << std::endl;
        }
    });

std::this_thread::sleep_for(std::chrono::seconds(10));
client->UnsubscribeJointState(sub_id);
client->Disconnect();
```

## 7. 运行示例

SDK 包内 `examples/` 目录提供了可编译运行的示例代码。

**构建：**

```bash
cd tangpa_sdk_client
mkdir build && cd build
cmake ..
make -j$(nproc)
```

构建完成后二进制在 `bin/` 目录下。有 data 文件的示例（如 benchmark）会输出到独立子目录，同时自动复制其 `data/` 文件夹。CMake 会自动将 `config/` 复制到构建目录，无需手动操作。

**运行：**

```bash
# 无 data 依赖的示例（平铺在 bin/ 下）
./bin/CHRIC_SDKClient_test
./bin/system_test_gui

# 有 data 依赖的示例（独立子目录）
./bin/tangpa_api_benchmark/CHRIC_SDKClient_benchmark
./bin/control_test_gui/control_test_gui
```

| 示例 | 二进制 | 运行路径 |
|------|--------|----------|
| 功能测试 | `CHRIC_SDKClient_test` | `./bin/CHRIC_SDKClient_test` |
| 性能基准 | `CHRIC_SDKClient_benchmark` | `./bin/tangpa_api_benchmark/CHRIC_SDKClient_benchmark` |
| 控制API交互测试 | `control_test_gui` | `./bin/control_test_gui/control_test_gui` |
| 系统API交互测试 | `system_test_gui` | `./bin/system_test_gui` |

> 运行前请确保已根据实际环境修改 `config/config.yaml` 中的 `default_target` 地址和端口。

## 8. 类型速查

| 类型 | 命名空间 | proto |
|------|----------|-------|
| `Imu` | `tangpa_control_api` | [Imu.proto](data/proto/ros2/sensor_msgs/Imu.proto) |
| `Joy` | `tangpa_control_api` | [Joy.proto](data/proto/ros2/sensor_msgs/Joy.proto) |
| `JointState` | `tangpa_control_api` | [JointState.proto](data/proto/ros2/sensor_msgs/JointState.proto) |
| `Cmotorcommand` | `tangpa_control_api` | [Cmotorcommand.proto](data/proto/tangpa_ros2_interfaces/control/cmotor_interface/Cmotorcommand.proto) |
| `JoyControllerResponse` | `tangpa_control_api` | [control.proto](data/proto/api_service/tangpa/proto/control.proto) |
| `UWBRequestHardware` | `tangpa_control_api` | [control.proto](data/proto/api_service/tangpa/proto/control.proto) |
| `UWBRequestConfig` | `tangpa_control_api` | [control.proto](data/proto/api_service/tangpa/proto/control.proto) |
| `LinktrackNodeframe7` | `tangpa_control_api` | [LinktrackNodeframe7.proto](data/proto/tangpa_ros2_interfaces/control/nlink_parser/LinktrackNodeframe7.proto) |
| `MotorCalibrationRequest` | `tangpa_control_api` | [control.proto](data/proto/api_service/tangpa/proto/control.proto) |
| `MotorCalibrationResponse` | `tangpa_control_api` | [control.proto](data/proto/api_service/tangpa/proto/control.proto) |
| `DiagnosticMessage` | `tangpa_system_api` | [system.proto](data/proto/api_service/tangpa/proto/system.proto) |
| `RobotStatus` | `tangpa_system_api` | [system.proto](data/proto/api_service/tangpa/proto/system.proto) |
| `BatteryPercentage` | `tangpa_system_api` | [system.proto](data/proto/api_service/tangpa/proto/system.proto) |

## 9. 常见问题

### 连接失败

1. 检查机器人服务器是否运行
2. 检查地址和端口是否正确
3. 检查防火墙设置

### 订阅无回调

1. 检查服务端是否已启用对应数据流
2. 检查网络连接是否正常
