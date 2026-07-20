/**
 * @file simple_config_manager.h
 * @brief 极简配置管理器 - 加载 YAML 配置文件
 * @author 软件部门基础设施团队
 * @date 2025-11-17
 */

#ifndef HUMANOID_ROBOT_SIMPLE_CONFIG_MANAGER_H
#define HUMANOID_ROBOT_SIMPLE_CONFIG_MANAGER_H

#include "config_node.h"
#include <string>

namespace humanoid_robot
{
    namespace framework
    {
        namespace common
        {

            /**
             * @brief 配置管理器 - 极简版本
             *
             * 提供静态方法从 YAML 文件或字符串加载配置，返回配置树。
             * 所有配置值以字符串形式存储，类型转换由使用者完成。
             *
             * 使用示例：
             * @code
             * // 加载配置文件
             * auto config = ConfigManager::LoadFromFile("config.yaml");
             *
             * // 访问配置（所有值都是字符串）
             * std::string host = config["server"]["host"];
             * std::string port_str = config["server"]["port"];
             *
             * // 使用者自己转换类型
             * int port = std::stoi(port_str);
             *
             * // 访问数组
             * auto languages = config["supported_languages"];
             * for (const auto& lang : languages) {
             *     std::string lang_str = lang;
             *     std::cout << lang_str << std::endl;
             * }
             * @endcode
             */
            class ConfigManager
            {
            public:
                /**
                 * @brief 从 YAML 文件加载配置
                 * @param file_path 配置文件路径
                 * @return 配置树根节点（失败返回空节点）
                 *
                 * @note 如果文件不存在或解析失败，会输出错误信息到 stderr 并返回空节点
                 */
                static ConfigNode LoadFromFile(const std::string &file_path);

                /**
                 * @brief 从字符串加载配置
                 * @param yaml_content YAML 内容字符串
                 * @return 配置树根节点（失败返回空节点）
                 *
                 * @note 如果解析失败，会输出错误信息到 stderr 并返回空节点
                 */
                static ConfigNode LoadFromString(const std::string &yaml_content);
            };

        } // namespace common
    } // namespace framework
} // namespace humanoid_robot

#endif // HUMANOID_ROBOT_SIMPLE_CONFIG_MANAGER_H
