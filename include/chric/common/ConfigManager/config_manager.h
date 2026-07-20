/**
 * @file config_manager.h
 * @brief 通用配置管理模块 - 公共头文件
 * @author Humanoid Robot Team
 * @version 1.0.0
 * @date 2025-08-08
 * 
 * 这是一个通用的配置管理模块，支持：
 * - YAML 配置文件解析
 * - 环境变量覆盖
 * - 配置验证
 * - 热重载
 * - 线程安全
 * - 类型安全的配置访问
 */

#ifndef HUMANOID_ROBOT_CONFIG_MANAGER_H
#define HUMANOID_ROBOT_CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <atomic>

// 平台兼容性宏定义
#ifdef _WIN32
    #ifdef CONFIG_MANAGER_EXPORTS
        #define CONFIG_MANAGER_API __declspec(dllexport)
    #else
        #define CONFIG_MANAGER_API __declspec(dllimport)
    #endif
#else
    #define CONFIG_MANAGER_API __attribute__((visibility("default")))
#endif

namespace humanoid_robot {
namespace common {

/**
 * @brief 配置值类型枚举
 */
enum class ConfigValueType {
    STRING,
    INTEGER,
    DOUBLE,
    BOOLEAN,
    ARRAY,
    OBJECT
};

/**
 * @brief 配置错误类型
 */
enum class ConfigError {
    SUCCESS = 0,
    FILE_NOT_FOUND,
    PARSE_ERROR,
    TYPE_MISMATCH,
    KEY_NOT_FOUND,
    VALIDATION_FAILED,
    PERMISSION_DENIED,
    UNKNOWN_ERROR
};

/**
 * @brief 配置变更回调函数类型
 * @param key 变更的配置键
 * @param old_value 旧值
 * @param new_value 新值
 */
using ConfigChangeCallback = std::function<void(const std::string& key, 
                                               const std::string& old_value,
                                               const std::string& new_value)>;

/**
 * @brief 配置验证器函数类型
 * @param key 配置键
 * @param value 配置值
 * @return 验证是否通过
 */
using ConfigValidator = std::function<bool(const std::string& key, const std::string& value)>;

/**
 * @brief 配置统计信息
 */
struct CONFIG_MANAGER_API ConfigStats {
    size_t total_keys;              ///< 总配置项数量
    size_t file_load_count;         ///< 文件加载次数
    size_t env_override_count;      ///< 环境变量覆盖次数
    size_t validation_error_count;  ///< 验证错误次数
    std::string last_load_time;     ///< 最后加载时间
    std::string config_file_path;   ///< 配置文件路径
};

/**
 * @brief 配置管理器接口类
 * 
 * 这是一个抽象接口类，定义了配置管理的基本操作。
 * 实际实现在 ConfigManagerImpl 类中。
 */
class CONFIG_MANAGER_API IConfigManager {
public:
    virtual ~IConfigManager() = default;
    
    // 基础配置加载和保存
    virtual ConfigError LoadFromFile(const std::string& config_file) = 0;
    virtual ConfigError LoadFromString(const std::string& yaml_content) = 0;
    virtual ConfigError SaveToFile(const std::string& config_file) const = 0;
    virtual ConfigError Reload() = 0;
    
    // 配置项访问 - 基础类型
    virtual std::optional<std::string> GetString(const std::string& key, 
                                                 const std::string& default_value = "") const = 0;
    virtual std::optional<int> GetInt(const std::string& key, int default_value = 0) const = 0;
    virtual std::optional<double> GetDouble(const std::string& key, double default_value = 0.0) const = 0;
    virtual std::optional<bool> GetBool(const std::string& key, bool default_value = false) const = 0;
    
    // 配置项访问 - 复合类型
    virtual std::optional<std::vector<std::string>> GetStringArray(const std::string& key) const = 0;
    virtual std::optional<std::vector<int>> GetIntArray(const std::string& key) const = 0;
    virtual std::optional<std::map<std::string, std::string>> GetStringMap(const std::string& key) const = 0;
    
    // 配置项设置
    virtual ConfigError SetString(const std::string& key, const std::string& value) = 0;
    virtual ConfigError SetInt(const std::string& key, int value) = 0;
    virtual ConfigError SetDouble(const std::string& key, double value) = 0;
    virtual ConfigError SetBool(const std::string& key, bool value) = 0;
    
    // 配置管理
    virtual bool HasKey(const std::string& key) const = 0;
    virtual ConfigError RemoveKey(const std::string& key) = 0;
    virtual std::vector<std::string> GetAllKeys() const = 0;
    virtual ConfigError Clear() = 0;
    
    // 环境变量集成
    virtual ConfigError LoadEnvironmentVariables(const std::string& prefix = "HUMANOID_ROBOT_") = 0;
    virtual void SetEnvironmentOverride(bool enable) = 0;
    
    // 验证和回调
    virtual void RegisterValidator(const std::string& key_pattern, ConfigValidator validator) = 0;
    virtual void RegisterChangeCallback(const std::string& key_pattern, ConfigChangeCallback callback) = 0;
    virtual ConfigError ValidateAll() const = 0;
    
    // 热重载
    virtual void EnableAutoReload(bool enable, int check_interval_seconds = 5) = 0;
    virtual bool IsAutoReloadEnabled() const = 0;
    
    // 统计和调试
    virtual ConfigStats GetStats() const = 0;
    virtual std::string GetConfigSummary() const = 0;
    virtual void SetLogLevel(int level) = 0; // 0=Silent, 1=Error, 2=Warning, 3=Info, 4=Debug
    
    // 线程安全
    virtual void Lock() const = 0;
    virtual void Unlock() const = 0;
};

/**
 * @brief 配置管理器工厂类
 * 
 * 提供创建配置管理器实例的静态方法。
 */
class CONFIG_MANAGER_API ConfigManagerFactory {
public:
    /**
     * @brief 创建配置管理器实例
     * @return 配置管理器智能指针
     */
    static std::unique_ptr<IConfigManager> Create();
    
    /**
     * @brief 创建单例配置管理器
     * @return 配置管理器引用
     */
    static IConfigManager& GetInstance();
    
    /**
     * @brief 销毁单例实例
     */
    static void DestroyInstance();
};

/**
 * @brief 配置管理器辅助类
 * 
 * 提供一些便利的工具函数。
 */
class CONFIG_MANAGER_API ConfigHelper {
public:
    /**
     * @brief 将配置错误转换为字符串
     * @param error 错误码
     * @return 错误描述字符串
     */
    static std::string ErrorToString(ConfigError error);
    
    /**
     * @brief 验证配置文件格式
     * @param config_file 配置文件路径
     * @return 验证结果
     */
    static ConfigError ValidateConfigFile(const std::string& config_file);
    
    /**
     * @brief 创建默认配置模板
     * @param template_file 模板文件路径
     * @param module_name 模块名称
     * @return 创建结果
     */
    static ConfigError CreateDefaultTemplate(const std::string& template_file, 
                                            const std::string& module_name);
    
    /**
     * @brief 合并两个配置文件
     * @param base_config 基础配置文件
     * @param override_config 覆盖配置文件
     * @param output_config 输出配置文件
     * @return 合并结果
     */
    static ConfigError MergeConfigs(const std::string& base_config,
                                   const std::string& override_config,
                                   const std::string& output_config);
    
    /**
     * @brief 加密敏感配置值
     * @param plaintext 明文
     * @param key 加密密钥
     * @return 加密后的字符串
     */
    static std::string EncryptValue(const std::string& plaintext, const std::string& key);
    
    /**
     * @brief 解密敏感配置值
     * @param ciphertext 密文
     * @param key 解密密钥
     * @return 解密后的字符串
     */
    static std::string DecryptValue(const std::string& ciphertext, const std::string& key);
};

/**
 * @brief RAII 配置锁
 * 
 * 自动管理配置管理器的锁定和解锁。
 */
class CONFIG_MANAGER_API ConfigLock {
public:
    explicit ConfigLock(const IConfigManager& config_manager);
    ~ConfigLock();
    
    ConfigLock(const ConfigLock&) = delete;
    ConfigLock& operator=(const ConfigLock&) = delete;
    
private:
    const IConfigManager& config_manager_;
};

// 便利宏定义
#define CONFIG_MANAGER ConfigManagerFactory::GetInstance()
#define CONFIG_LOCK(manager) ConfigLock __config_lock__(manager)

} // namespace common
} // namespace humanoid_robot

// C 风格接口（用于C语言或其他语言绑定）
extern "C" {
    CONFIG_MANAGER_API void* config_manager_create();
    CONFIG_MANAGER_API void config_manager_destroy(void* manager);
    CONFIG_MANAGER_API int config_manager_load_file(void* manager, const char* config_file);
    CONFIG_MANAGER_API const char* config_manager_get_string(void* manager, const char* key, const char* default_value);
    CONFIG_MANAGER_API int config_manager_get_int(void* manager, const char* key, int default_value);
    CONFIG_MANAGER_API double config_manager_get_double(void* manager, const char* key, double default_value);
    CONFIG_MANAGER_API int config_manager_get_bool(void* manager, const char* key, int default_value);
    CONFIG_MANAGER_API int config_manager_set_string(void* manager, const char* key, const char* value);
    CONFIG_MANAGER_API const char* config_manager_error_to_string(int error);
}

#endif // HUMANOID_ROBOT_CONFIG_MANAGER_H
