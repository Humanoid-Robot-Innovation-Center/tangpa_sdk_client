/**
 * @file thread_pool.h
 * @brief 线程池实现
 *
 * 基于CHRIC SDK的线程池实现模块
 *
 * @author CHRIC SDK Team
 * @version 1.0.0
 * @date 2026-07-13
 *
 * @copyright Copyright (c) 2026 Chengdu Humanoid Robot Innovation Center Co., Ltd. All rights reserved.
 *
 * @note 支持任务提交、结果获取、线程池大小配置
 *
 * @warning
 *
 * 变更日志:
 * - v1.0.0 (2026-07-13): 初始版本
 */

#ifndef CHRIC_SDK_COMMON_THREAD_POOL
#define CHRIC_SDK_COMMON_THREAD_POOL

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace humanoid_robot {
namespace sdk_client {
namespace common {

/**
 * @brief 简单的线程池实现
 */
class ThreadPool {
 public:
  explicit ThreadPool(size_t num_threads = 4);
  ~ThreadPool();

  // 禁止拷贝
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  /**
   * @brief 提交任务到线程池
   * @param func 要执行的函数
   * @return Future 对象，可以获取任务结果
   */
  template <typename F, typename... Args>
  auto Submit(F&& func, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type>;

  /**
   * @brief 获取线程池大小
   */
  size_t Size() const { return workers_.size(); }

 private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable condition_;
  bool stop_;
};

// 模板实现
template <typename F, typename... Args>
auto ThreadPool::Submit(F&& func, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
  using return_type = typename std::invoke_result<F, Args...>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<F>(func), std::forward<Args>(args)...));

  std::future<return_type> result = task->get_future();

  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (stop_) {
      throw std::runtime_error("Submit task to stopped ThreadPool");
    }
    tasks_.emplace([task]() { (*task)(); });
  }

  condition_.notify_one();
  return result;
}

}  // namespace common
}  // namespace sdk_client
}  // namespace humanoid_robot

#endif  // CHRIC_SDK_COMMON_THREAD_POOL