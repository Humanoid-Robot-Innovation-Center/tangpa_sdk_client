/**
 * @file interfaces_client.h
 * @brief gRPC客户端通信接口
 *
 * 提供与机器人SDK服务端的gRPC通信封装，包括连接管理、命令发送、查询、订阅等核心通信功能。
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 提供同步和异步接口、支持Send/Query/Subscribe/Unsubscribe等操作
 *
 * @warning 连接状态检查应在调用其他方法前确认
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#ifndef CHRIC_SDK_CLIENT_INTERFACES
#define CHRIC_SDK_CLIENT_INTERFACES

#include <grpcpp/grpcpp.h>

#include <functional>
#include <future>
#include <memory>
#include <string>

#include "api_service/common/interfaces_grpc.grpc.pb.h"
#include "api_service/common/interfaces_request_response.pb.h"
#include "robot/common/status.h"

namespace humanoid_robot {
namespace sdk_client {
namespace robot {
using Status = humanoid_robot::sdk_client::common::Status;

// Forward declarations for async operation results
template <typename T>
using AsyncResult = std::future<Status>;

template <typename T>
using AsyncCallback = std::function<void(const Status&, const T&)>;

/**
 * InterfacesClient - gRPC client for InterfaceService
 *
 * This client provides both synchronous and asynchronous methods for all
 * InterfaceService operations defined in interfaces_grpc.proto
 */
class InterfacesClient : public std::enable_shared_from_this<InterfacesClient> {
 public:
  // Constructor and destructor
  InterfacesClient();
  virtual ~InterfacesClient();

  // Connection management
  Status Connect(const std::string& server_address, int port);
  Status Connect(const std::string& target);
  Status Connect();
  void Disconnect();
  bool IsConnected() const;

 protected:
  /**
   * Initialize - pure virtual, called automatically by each derived class's constructor.
   * Derived classes must override this to perform custom initialization.
   */
  virtual void Initialize() = 0;

 public:

  /**
   * Send a message
   * @param request The send request
   * @param response The send response (output)
   * @param timeout_ms Timeout in milliseconds (default: 5000)
   * @return Status of the operation
   */
  Status Send(const humanoid_robot::PB::api_service::common::SendRequest& request,
              humanoid_robot::PB::api_service::common::SendResponse& response,
              std::unique_ptr<grpc::ClientContext>& context,
              int64_t timeout_ms = 5000);

  Status SendStream(
      const humanoid_robot::PB::api_service::common::SendRequest& request,
      std::unique_ptr<grpc::ClientReader<humanoid_robot::PB::api_service::common::SendResponse>>& reader,
      std::unique_ptr<grpc::ClientContext>& context, int64_t timeout_ms);

  Status SendBatch(
    std::unique_ptr<grpc::ClientWriter<::humanoid_robot::PB::api_service::common::SendRequest>>& writer,
    std::unique_ptr<grpc::ClientContext>& context,
    ::humanoid_robot::PB::api_service::common::SendResponse& response,
    int64_t timeout_ms);

  /**
   * Unsubscribe from a subscription
   * @param request The unsubscribe request
   * @param response The unsubscribe response (output)
   * @param timeout_ms Timeout in milliseconds (default: 5000)
   * @return Status of the operation
   */
  Status Unsubscribe(
      const humanoid_robot::PB::api_service::common::UnsubscribeRequest& request,
      humanoid_robot::PB::api_service::common::UnsubscribeResponse& response,
      int64_t timeout_ms = 5000);

  // =================================================================
  // Asynchronous Methods
  // =================================================================

  /**
   * Async send - returns immediately with a future
   */
  // AsyncResult<humanoid_robot::PB::api_service::common::SendResponse> SendAsync(
  //     const humanoid_robot::PB::api_service::common::SendRequest &request,
  //     int64_t timeout_ms = 5000);

  // /**
  //  * Async send with callback
  //  */
  // void SendAsync(
  //     const humanoid_robot::PB::api_service::common::SendRequest &request,
  //     AsyncCallback<humanoid_robot::PB::api_service::common::SendResponse> callback,
  //     int64_t timeout_ms = 5000);

  /**
   * Async query - returns immediately with a future
   */
  AsyncResult<humanoid_robot::PB::api_service::common::QueryResponse> QueryAsync(
      const humanoid_robot::PB::api_service::common::QueryRequest& request,
      int64_t timeout_ms = 5000);

  /**
   * Async query with callback
   */
  void QueryAsync(
      const humanoid_robot::PB::api_service::common::QueryRequest& request,
      AsyncCallback<humanoid_robot::PB::api_service::common::QueryResponse> callback,
      int64_t timeout_ms = 5000);

  // =================================================================
  // Streaming Methods
  // =================================================================

  /**
   * Subscribe to events (streaming response) - Legacy方式
   * @param request The subscription request
   * @param callback Called for each received event
   * @param timeout_ms Timeout for the subscription (0 = no timeout)
   * @return Status of the operation (returns when stream ends or errors)
   */
  Status Subscribe(
      const humanoid_robot::PB::api_service::common::SubscribeRequest& request,
      humanoid_robot::PB::api_service::common::SubscribeResponse& response,
      int64_t timeout_ms = 0);

  // =================================================================
  // Utility Methods
  // =================================================================

  /**
   * Get the current gRPC channel state
   */
  grpc_connectivity_state GetChannelState(bool try_to_connect = false);

  /**
   * Wait for the channel to be ready
   * @param timeout_ms Maximum time to wait
   * @return True if channel became ready within timeout
   */
  bool WaitForChannelReady(int64_t timeout_ms = 5000);

 private:
  // Private implementation details
  class Impl;
  std::unique_ptr<Impl> impl_;

  // Disable copy and assignment
  InterfacesClient(const InterfacesClient&) = delete;
  InterfacesClient& operator=(const InterfacesClient&) = delete;
};

// =================================================================
// Convenience Functions
// =================================================================

/**
 * Create a quick interfaces client connected to a server
 * @param server_address Server address (e.g., "localhost")
 * @param port Server port
 * @param client Output client instance
 * @return Status of the connection
 */
Status CreateInterfacesClientLegacy(const std::string& server_address, int port,
                                    std::unique_ptr<InterfacesClient>& client);

}  // namespace robot

// Minimal concrete implementation for basic connectivity testing
namespace robot {
class BasicClient : public InterfacesClient {
 protected:
  void Initialize() override {}
};
}  // namespace robot

// Factory functions for creating clients
namespace factory {
using Status = humanoid_robot::sdk_client::common::Status;
/**
 * Create and connect interfaces client (推荐用于异步操作)
 * @param server_address Server address (e.g., "localhost")
 * @param port Server port (e.g., 50051)
 * @param client Output client instance (shared_ptr for async safety)
 * @return Status of the connection
 */
Status CreateInterfacesClient(const std::string& server_address, int port,
                              std::shared_ptr<robot::InterfacesClient>& client);

/**
 * Create a quick interfaces client with target string (推荐用于异步操作)
 * @param target Target string (e.g., "localhost:50051")
 * @param client Output client instance (shared_ptr for async safety)
 * @return Status of the connection
 */
Status CreateInterfacesClient(const std::string& target,
                              std::shared_ptr<robot::InterfacesClient>& client);
}  // namespace factory
}  // namespace sdk_client
}  // namespace humanoid_robot

#endif  // CHRIC_SDK_CLIENT_INTERFACES
