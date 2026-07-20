#ifndef WLOG_HPP
#define WLOG_HPP

#include <string>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <cstring>

// 日志等级定义
// 0=FATAL(致命), 1=ERROR(错误), 2=WARN(警告), 3=INFO(信息), 4=DEBUG(调试)
// 日志级别通过配置文件运行时设置，不再使用编译时宏控制

#define WLOG_LEVEL_FATAL 0 // 致命错误，程序即将崩溃
#define WLOG_LEVEL_ERROR 1 // 错误，功能失败但程序继续
#define WLOG_LEVEL_WARN 2  // 警告，潜在问题
#define WLOG_LEVEL_INFO 3  // 信息，重要的业务流程
#define WLOG_LEVEL_DEBUG 4 // 调试，详细的诊断信息

// 路径剪切声明
const char *StripPathPrefix(const char *file);

const char *GetCurrentTimeString();
// 获取进程执行路径
std::string GetExeDir();

// 日志宏声明
// 日志级别过滤在运行时通过配置文件控制，宏直接调用WLogWrite函数
void WLogWrite(int level, const char *file, int line, const char *func, const char *format, ...);

#define WLOG_DEBUG(format, ...) \
    WLogWrite(WLOG_LEVEL_DEBUG, StripPathPrefix(__FILE__), __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

#define WLOG_INFO(format, ...) \
    WLogWrite(WLOG_LEVEL_INFO, StripPathPrefix(__FILE__), __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

#define WLOG_WARN(format, ...) \
    WLogWrite(WLOG_LEVEL_WARN, StripPathPrefix(__FILE__), __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

#define WLOG_ERROR(format, ...) \
    WLogWrite(WLOG_LEVEL_ERROR, StripPathPrefix(__FILE__), __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

#define WLOG_FATAL(format, ...) \
    WLogWrite(WLOG_LEVEL_FATAL, StripPathPrefix(__FILE__), __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

// 日志系统初始化与配置
void WLogInit(); // 使用默认配置初始化（默认级别4=DEBUG）
// 使用配置文件初始化（推荐：通过YAML配置log_level）
// 例：WLogInitWithConfig("config/software_config.yaml")
void WLogInitWithConfig(const std::string &config_file_path);
void WLogSetPath(const std::string &path); // 设置日志目录路径
void WLogStop();                           // 停止日志系统，刷新缓冲区
// 安装崩溃信号处理器（SIGSEGV/SIGFPE/SIGBUS）
// 崩溃时在程序工作目录的 crash/ 子目录创建记录文件，然后恢复默认处理器以生成 coredump
// 注：SIGABRT（主动终止/abort）不在处理范围内
void WLogInstallCrashHandler();

// 开启 coredump 文件生成（解除 ulimit -c 限制）
// 在 WLogInit() 内部自动调用，无需手动调用
void WLogEnableCoredump();

// 日志文件管理
void CheckAndCreateLogFile(int level, int32_t buffer_size = 0);
std::string GetLogFilePath(int level, bool force_new = false);

#endif // WLOG_HPP