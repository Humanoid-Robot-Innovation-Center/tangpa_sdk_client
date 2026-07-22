# 一、接口定义

## 1.1 连接管理

`ControlApiClient` 和 `SystemApiClient` 的连接管理接口一致，但相互独立，都需要建立各自的连接才能使用业务接口。

```cpp
Status Connect(const std::string& server_address, int port);
Status Connect(const std::string& target);
Status Connect();
void Disconnect();
bool IsConnected() const;
```

**参数：**

| 函数                       | 说明                                                                     |
| ------------------------ | ---------------------------------------------------------------------- |
| `Connect(address, port)` | 指定地址和端口连接                                                              |
| `Connect(target)`        | 指定完整目标字符串连接（如 `"192.168.0.1:50051"`）                                   |
| `Connect()`              | 读取 `config/config.yaml` 中 `grpc_client.connection.default_target` 自动连接 |
| `Disconnect()`           | 断开连接                                                                   |

**返回值：**

| 函数            | 类型       | 说明                     |
| ------------- | -------- | ---------------------- |
| `Connect`     | `Status` | 成功返回 OK，失败返回错误状态       |
| `IsConnected` | `bool`   | `true` 已连接，`false` 未连接 |

**备注：** 已连接时重复调用 `Connect` 会返回错误，需先调用 `Disconnect()`。

***

## 1.2 ControlApiClient 业务接口

### 1.2.1 GetImuData

```cpp
uint32_t GetImuData(Imu& imu_status);
```

**功能：** 获取一次 IMU 数据快照。

**参数：**

| 参数           | 类型    | 说明                                                                                           |
| ------------ | ----- | -------------------------------------------------------------------------------------------- |
| `imu_status` | `Imu` | 输出参数，IMU 数据，参见 [sensor\_msgs/Imu](https://docs.ros2.org/latest/api/sensor_msgs/msg/Imu.html) |

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 网络通信 5 秒。

**注意事项：** 获取 IMU 故障反馈请使用 `GetDiagnosticData` 接口。

**返回码：**

| 场景                | 返回码                         | 值            |
| ----------------- | --------------------------- | ------------ |
| IMU 子系统错误，未获取到数据  | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 接口层通信错误，返回数据序列化失败 | `ERROR_SERIALIZE_FAILED`    | `0x00000003` |
| 接口层服务端异常          | `ERROR_FAILED`              | `0x7FFFFFFF` |
| 成功                | `STATUS_OK`                 | `0x0`        |

***

### 1.2.2 MotorCalibration

```cpp
uint32_t MotorCalibration(const MotorCalibrationRequest& request, MotorCalibrationResponse& response);
```

**功能：** 对指定电机执行标定。

**参数：**

| 参数        | 类型                        | 说明   |
| --------- | ------------------------- | ---- |
| `request` | `MotorCalibrationRequest` | 输入参数 |

**request 字段：**

| 字段         | 类型       | 说明    |
| ---------- | -------- | ----- |
| `motor_id` | `uint32` | 电机 ID |

| 参数         | 类型                         | 说明   |
| ---------- | -------------------------- | ---- |
| `response` | `MotorCalibrationResponse` | 输出参数 |

**response 字段：**

| 字段         | 类型       | 说明                                                  |
| ---------- | -------- | --------------------------------------------------- |
| `state`    | `string` | 标定状态：idle / running / succeeded / failed / rejected |
| `motor_id` | `uint32` | 电机 ID                                               |
| `message`  | `string` | 进度、失败或拒绝原因                                          |

**state 状态说明：**

| state       | 是否最终状态 | 含义                           |
| ----------- | ------ | ---------------------------- |
| `idle`      | 否      | 节点已启动，等待请求                   |
| `running`   | 否      | 正在标定                         |
| `succeeded` | 是      | 5 个标定帧均确认成功                  |
| `failed`    | 是      | 标定已执行，但 CAN 初始化异常或至少一个标定帧未确认 |
| `rejected`  | 是      | 请求未执行                        |

**常见拒绝原因（rejected 时 message 内容）：**

| message 内容                  | 含义                |
| --------------------------- | ----------------- |
| 硬件自检未通过，禁止电机标定              | 本次开机自检未通过，无法标定    |
| 电机控制或另一标定任务正在占用 CAN，总线标定被拒绝 | 电机控制正在运行，或已有标定在执行 |
| 已有标定任务正在执行                  | 当前节点正处理另一请求       |
| 不支持的电机 ID                   | data 不在支持范围内      |

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 服务超时 5 秒，网络通信超时 30 秒。

**返回码：**

| 场景                              | 返回码                        | 值            |
| ------------------------------- | -------------------------- | ------------ |
| 接口层通信错误，参数解析失败                  | `ERROR_INVALID_PARAMETER`  | `0x00200006` |
| 接口层通信错误，参数反序列化失败                | `ERROR_DESERIALIZE_FAILED` | `0x00000004` |
| 系统接口层服务端错误，响应序列化失败              | `ERROR_SERIALIZE_FAILED`   | `0x00000003` |
| 系统接口层服务端超时（有/无缓存）               | `ERROR_TIMEOUT`            | `0x00000001` |
| 系统接口层服务端，异常                     | `ERROR_FAILED`             | `0x7FFFFFFF` |
| 收到子系统 succeeded/failed/rejected | `STATUS_OK`                | `0x0`        |

**注意事项：**

- 支持的电机 ID：1、2、3、4、11、12、13、14、15、16、17、18、19、20。
- 调用前，机器人本次开机硬件自检必须通过。
- 标定与机器人电机控制互斥，不能同时执行。
- 同一时间仅允许一个标定任务。

***

### 1.2.3 SendMotorControl

```cpp
uint32_t SendMotorControl(const Cmotorcommand& command);
```

**功能：** 向机器人发送电机控制指令。

**参数：**

| 参数        | 类型              | 说明   |
| --------- | --------------- | ---- |
| `command` | `Cmotorcommand` | 输入参数 |

**command 字段：**

| 字段              | 类型                | 说明                                           |
| --------------- | ----------------- | -------------------------------------------- |
| `motor_ids`     | `repeated uint32` | 电机 ID 数组                                     |
| `control_modes` | `repeated uint32` | 控制模式数组：0 = 位置控制, 1 = 速度控制, 2 = 力矩控制，3=力位混合模式 |
| `positions`     | `repeated float`  | 位置值数组，用于位置控制模式                               |
| `speeds`        | `repeated float`  | 速度值数组，用于速度控制模式                               |
| `torques`       | `repeated float`  | 力矩数组，用于力位混合模式（力矩控制）                          |
| `kps`           | `repeated float`  | Kp 值数组，用于力位混合模式                              |
| `kds`           | `repeated float`  | Kd 值数组，用于力位混合模式                              |

**返回值：** `uint32_t`，返回码。

**约束：**
- `control_modes` 取值范围：`[0, 3]`
- `motor_ids` 不允许重复
- 所有数组（`motor_ids`、`control_modes`、`positions`、`speeds`、`torques`、`kps`、`kds`）长度必须一致

**注意事项：**

- 调用前，机器人本次开机硬件自检必须通过。
- 标定与机器人电机控制互斥，不能同时执行。

***

### 1.2.4 SendJoyControl

```cpp
uint32_t SendJoyControl(const Joy& command);
```

**功能：** 向机器人发送手柄控制指令。

**参数：**

| 参数        | 类型    | 说明                                                                                    |
| --------- | ----- | ------------------------------------------------------------------------------------- |
| `command` | `Joy` | 输入参数，参见 [sensor\_msgs/Joy](https://docs.ros2.org/latest/api/sensor_msgs/msg/Joy.html) |

**返回值：** `uint32_t`，返回码。

***

### 1.2.5 SubscribeImu

```cpp
uint64_t SubscribeImu(const std::function<void(const std::shared_ptr<const Imu>&)>& callback);
```

**功能：** 订阅 IMU 数据流。

**参数：**

| 参数         | 类型                                                       | 说明                                                                                      |
| ---------- | -------------------------------------------------------- | --------------------------------------------------------------------------------------- |
| `callback` | `std::function<void(const std::shared_ptr<const Imu>&)>` | 回调函数，参数参见 [sensor\_msgs/Imu](https://docs.ros2.org/latest/api/sensor_msgs/msg/Imu.html) |

**返回值：** `uint64_t`。`> 0` 为订阅 ID（用于取消订阅），`0` 表示订阅失败。

**超时：** 约 5 秒。

***

### 1.2.6 UnsubscribeImu

```cpp
uint32_t UnsubscribeImu(uint64_t subscription_id);
```

**功能：** 取消 IMU 数据订阅。

**参数：**

| 参数                | 类型         | 说明                        |
| ----------------- | ---------- | ------------------------- |
| `subscription_id` | `uint64_t` | 由 `SubscribeImu` 返回的订阅 ID |

**返回值：** `uint32_t`，返回码。

**超时：** 约 5 秒。

***

### 1.2.7 SubscribeJoy

```cpp
uint64_t SubscribeJoy(const std::function<void(const std::shared_ptr<const Joy>&)>& callback);
```

**功能：** 订阅手柄数据流。

**参数：**

| 参数         | 类型                                                       | 说明                                                                                      |
| ---------- | -------------------------------------------------------- | --------------------------------------------------------------------------------------- |
| `callback` | `std::function<void(const std::shared_ptr<const Joy>&)>` | 回调函数，参数参见 [sensor\_msgs/Joy](https://docs.ros2.org/latest/api/sensor_msgs/msg/Joy.html) |

**返回值：** `uint64_t`。`> 0` 为订阅 ID（用于取消订阅），`0` 表示订阅失败。

**超时：** 约 5 秒。

**注意事项：** 获取手柄故障反馈，请使用 `GetDiagnosticData` 接口。

***

### 1.2.8 UnsubscribeJoy

```cpp
uint32_t UnsubscribeJoy(uint64_t subscription_id);
```

**功能：** 取消手柄数据订阅。

**参数：**

| 参数                | 类型         | 说明                        |
| ----------------- | ---------- | ------------------------- |
| `subscription_id` | `uint64_t` | 由 `SubscribeJoy` 返回的订阅 ID |

**返回值：** `uint32_t`，返回码。

**超时：** 约 5 秒。

***

### 1.2.9 SubscribeJointState

```cpp
uint64_t SubscribeJointState(const std::function<void(const std::shared_ptr<const JointState>&)>& callback);
```

**功能：** 订阅关节状态数据流。

**参数：**

| 参数         | 类型                                                              | 说明                                                                                                    |
| ---------- | --------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `callback` | `std::function<void(const std::shared_ptr<const JointState>&)>` | 回调函数，参数参见 [sensor\_msgs/JointState](https://docs.ros2.org/latest/api/sensor_msgs/msg/JointState.html) |

**返回值：** `uint64_t`。`> 0` 为订阅 ID（用于取消订阅），`0` 表示订阅失败。

**超时：** 约 5 秒。

***

### 1.2.10 UnsubscribeJointState

```cpp
uint32_t UnsubscribeJointState(uint64_t subscription_id);
```

**功能：** 取消关节状态数据订阅。

**参数：**

| 参数                | 类型         | 说明                               |
| ----------------- | ---------- | -------------------------------- |
| `subscription_id` | `uint64_t` | 由 `SubscribeJointState` 返回的订阅 ID |

**返回值：** `uint32_t`，返回码。

**超时：** 约 5 秒。

***

### 1.2.11 SetUWBConfigurator

```cpp
uint32_t SetUWBConfigurator(const UWBRequestHardware& request);
```

**功能：** 配置 UWB 硬件参数。

**参数：**

| 参数        | 类型                   | 说明   |
| --------- | -------------------- | ---- |
| `request` | `UWBRequestHardware` | 输入参数 |

**request 字段：**

| 字段              | 类型       | 说明                                                                                  |
| --------------- | -------- | ----------------------------------------------------------------------------------- |
| `target_device` | `string` | 目标设备，取值范围：`/dev/uwb_tag` 或 `/dev/uwb`，机器人默认使用 `/dev/uwb_tag`                        |
| `target_role`   | `int32`  | 核心参数：需要写入硬件的自身 Role ID，取值范围 0-255，机器人默认使用 `1`                                       |
| `baud`          | `int32`  | 暂未使用。波特率，必须为 9600、19200、38400、57600、115200、230400、460800、921600 之一，机器人默认使用 `921600` |

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 服务超时 5 秒，网络通信超时 30 秒。

**返回码：**

| 场景                           | 返回码                         | 值            |
| ---------------------------- | --------------------------- | ------------ |
| 接口层通信错误，参数解析失败               | `ERROR_INVALID_PARAMETER`   | `0x00200006` |
| 接口层通信错误，参数反序列化失败             | `ERROR_DESERIALIZE_FAILED`  | `0x00000004` |
| 子系统错误，configurator client 超时 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| configurator 子系统错误，参数设置失败    | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| trigger client 子系统错误，超时      | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 子系统错误，trigger 执行失败           | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端，异常                  | `ERROR_FAILED`              | `0x7FFFFFFF` |
| 成功                           | `STATUS_OK`                 | `0x0`        |

***

### 1.2.12 SetUWBConfigManager

```cpp
uint32_t SetUWBConfigManager(const UWBRequestConfig& request);
```

**功能：** 配置 UWB 跟随参数。

**参数：**

| 参数        | 类型                 | 说明   |
| --------- | ------------------ | ---- |
| `request` | `UWBRequestConfig` | 输入参数 |

**request 字段：**

| 字段                | 类型       | 说明                                        |
| ----------------- | -------- | ----------------------------------------- |
| `target_role`     | `int32`  | 目标 ID，取值范围 0-255：机器人将根据此 ID 筛选 UWB 数据进行跟随 |
| `target_distance` | `double` | 目标距离，单位：米，必须大于 0，机器人与跟随目标之间保持的距离          |

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 服务超时 5 秒，网络通信超时 30 秒。

**返回码：**

| 场景                     | 返回码                         | 值            |
| ---------------------- | --------------------------- | ------------ |
| 接口层通信错误，参数解析失败         | `ERROR_INVALID_PARAMETER`   | `0x00200006` |
| 接口层通信错误，参数反序列化失败       | `ERROR_DESERIALIZE_FAILED`  | `0x00000004` |
| 子系统错误，config client 超时 | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 子系统错误，config 参数设置失败    | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端，异常            | `ERROR_FAILED`              | `0x7FFFFFFF` |
| 成功                     | `STATUS_OK`                 | `0x0`        |

***

### 1.2.13 SubscribeUWBData

```cpp
uint64_t SubscribeUWBData(const std::function<void(const std::shared_ptr<const LinktrackNodeframe7>&)>& callback);
```

**功能：** 订阅 UWB 定位数据。

**参数：**

| 参数         | 类型                                                                       | 说明   |
| ---------- | ------------------------------------------------------------------------ | ---- |
| `callback` | `std::function<void(const std::shared_ptr<const LinktrackNodeframe7>&)>` | 回调函数 |

**callback 参数（LinktrackNodeframe7）字段：**

| 字段            | 类型                        | 说明            |
| ------------- | ------------------------- | ------------- |
| `role`        | `uint32`                  | 本帧发送者 / 本设备角色 |
| `id`          | `uint32`                  | 本帧发送者的 ID     |
| `local_time`  | `uint32`                  | 设备本地时间戳       |
| `system_time` | `uint32`                  | 系统时间戳         |
| `voltage`     | `float`                   | 设备电压          |
| `nodes`       | `repeated LinktrackNode7` | 多个节点测量结果      |

**nodes 子字段（LinktrackNode7）：**

| 字段        | 类型       | 说明      |
| --------- | -------- | ------- |
| `role`    | `uint32` | 节点角色    |
| `id`      | `uint32` | 节点 ID   |
| `dis`     | `float`  | 距离      |
| `angle0`  | `float`  | 方位角     |
| `angle1`  | `float`  | 俯仰角     |
| `fp_rssi` | `float`  | 首径信号强度  |
| `rx_rssi` | `float`  | 接收总信号强度 |

**返回值：** `uint64_t`。`> 0` 为订阅 ID（用于取消订阅），`0` 表示订阅失败。

**超时：** 约 5 秒。

**备注：** 获取 UWB 硬件故障反馈请使用 `GetDiagnosticData` 接口。

***

### 1.2.14 UnsubscribeUWBData

```cpp
uint32_t UnsubscribeUWBData(uint64_t subscription_id);
```

**功能：** 取消 UWB 定位数据订阅。

**参数：**

| 参数                | 类型         | 说明                            |
| ----------------- | ---------- | ----------------------------- |
| `subscription_id` | `uint64_t` | 由 `SubscribeUWBData` 返回的订阅 ID |

**返回值：** `uint32_t`，返回码。

**超时：** 约 5 秒。

***

## 1.3 SystemApiClient 业务接口

### 1.3.1 GetDiagnosticData

```cpp
uint32_t GetDiagnosticData(DiagnosticMessage& diagnostic_message);
```

**功能：** 获取诊断数据。

**参数：**

| 参数                   | 类型                  | 说明   |
| -------------------- | ------------------- | ---- |
| `diagnostic_message` | `DiagnosticMessage` | 输出参数 |

**diagnostic\_message 字段：**

| 字段         | 类型                | 说明                                                                                                                      |
| ---------- | ----------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `messages` | `DiagnosticArray` | 诊断结果数组，参见 [diagnostic\_msgs/DiagnosticArray](https://docs.ros2.org/latest/api/diagnostic_msgs/msg/DiagnosticArray.html) |
| `result`   | `int32`           | 执行结果                                                                                                                    |

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 网络通信 5 秒。

**返回码：**

| 场景               | 返回码                         | 值            |
| ---------------- | --------------------------- | ------------ |
| 子系统错误，未获取到数据     | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端错误，序列化失败 | `ERROR_SERIALIZE_FAILED`    | `0x00000003` |
| 系统接口层服务端异常       | `ERROR_FAILED`              | `0x7FFFFFFF` |
| 成功               | `STATUS_OK`                 | `0x0`        |

***

### 1.3.2 GetRobotStatus

```cpp
uint32_t GetRobotStatus(RobotStatus& robot_status);
```

**功能：** 获取机器人状态。

**参数：**

| 参数             | 类型            | 说明   |
| -------------- | ------------- | ---- |
| `robot_status` | `RobotStatus` | 输出参数 |

**robot\_status 字段：**

| 字段            | 类型                | 说明                    |
| ------------- | ----------------- | --------------------- |
| `state_id`    | `int64`           | 机器人当前状态码，见下表。         |
| `state_desc`  | `string`          | 机器人当前状态描述             |
| `battery`     | `float`           | 机器人当前归一化电量，范围：\[0, 1] |
| `error_codes` | `repeated string` | 机器人当前错误状态码列表          |

**state\_id 对照表：**

| state\_id | state\_desc                 | 状态名称  | 详细说明                             |
| --------- | --------------------------- | ----- | -------------------------------- |
| 0         | PREPARING                   | 准备中   | 自检已通过，系统正在等待关节状态、电机指令等运行条件       |
| 1         | MOTORS\_READY               | 电机已就绪 | 底层驱动已连接，关节状态活跃，但运动控制命令尚未进入       |
| 2         | MANUAL\_OP                  | 可操作状态 | 运动控制已启动，可通过手柄或脚本进行控制             |
| 3         | FOLLOWING                   | 跟随状态  | 进入 UWB 自动跟随模式                    |
| 4         | ERROR / SELF\_CHECK\_FAILED | 错误状态  | 发生致命硬件错误或硬件自检失败                  |
| 5         | LOW\_BATTERY                | 低电量状态 | 电量低于 15%，机器人进入低电量状态              |
| 6         | SELF\_CHECKING              | 自检中   | 硬件自检未通过前的启动保护状态。此时不能进入 PREPARING |

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 网络通信 5 秒。

**返回码：**

| 场景                      | 返回码                         | 值            |
| ----------------------- | --------------------------- | ------------ |
| 子系统错误，未获取到数据            | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端错误，序列化失败        | `ERROR_SERIALIZE_FAILED`    | `0x00000003` |
| 系统接口层服务端异常（含 JSON 解析异常） | `ERROR_FAILED`              | `0x7FFFFFFF` |
| 成功                      | `STATUS_OK`                 | `0x0`        |

***

### 1.3.3 GetBatteryPercentage

```cpp
uint32_t GetBatteryPercentage(BatteryPercentage& battery_percentage);
```

**功能：** 获取电池电量信息。

**参数：**

| 参数                   | 类型                  | 说明   |
| -------------------- | ------------------- | ---- |
| `battery_percentage` | `BatteryPercentage` | 输出参数 |

**battery\_percentage 字段：**

| 字段     | 类型       | 说明                           |
| ------ | -------- | ---------------------------- |
| `info` | `string` | 电池电量信息，当前为归一化电池电量，范围：\[0, 1] |

battery\_percentage 示例："0.52"、"0.75"

**返回值：** `uint32_t`，返回码，见下表。

**超时：** 网络通信 5 秒。

**返回码：**

| 场景               | 返回码                         | 值            |
| ---------------- | --------------------------- | ------------ |
| 子系统错误，未获取到数据     | `ERROR_SUBSYSTEM_NOT_READY` | `0x00200007` |
| 系统接口层服务端错误，序列化失败 | `ERROR_SERIALIZE_FAILED`    | `0x00000003` |
| 系统接口层服务端异常       | `ERROR_FAILED`              | `0x7FFFFFFF` |
| 成功               | `STATUS_OK`                 | `0x0`        |

# 二、接口使用示例

## 2.1 获取诊断数据

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

## 2.2 发送电机控制命令

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

## 2.3 订阅关节状态

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

***

