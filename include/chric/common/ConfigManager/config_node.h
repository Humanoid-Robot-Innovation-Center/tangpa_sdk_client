/**
 * @file config_node.h
 * @brief 配置节点定义 - 树形结构存储配置
 * @author 软件部门基础设施团队
 * @date 2025-11-17
 */

#ifndef HUMANOID_ROBOT_CONFIG_NODE_H
#define HUMANOID_ROBOT_CONFIG_NODE_H

#include <string>
#include <map>
#include <vector>

namespace humanoid_robot
{
    namespace framework
    {
        namespace common
        {

            /**
             * @brief 配置节点类型
             */
            enum class ConfigNodeType
            {
                STRING, ///< 字符串值
                ARRAY,  ///< 数组
                OBJECT  ///< 对象（嵌套结构）
            };

            /**
             * @brief 配置节点 - 存储配置值（树形结构）
             *
             * 支持三种类型：
             * - STRING: 标量值（所有值都转换为字符串）
             * - ARRAY: 数组（可迭代）
             * - OBJECT: 对象（键值对，支持嵌套）
             *
             * 使用示例：
             * @code
             * ConfigNode config = ConfigManager::LoadFromFile("config.yaml");
             * std::string host = config["server"]["host"];
             * auto languages = config["languages"];
             * for (const auto& lang : languages) {
             *     std::cout << static_cast<std::string>(lang) << std::endl;
             * }
             * @endcode
             */
            class ConfigNode
            {
            public:
                ConfigNodeType type;                            ///< 节点类型
                std::string string_value;                       ///< 字符串值（STRING 类型）
                std::vector<ConfigNode> array_value;            ///< 数组值（ARRAY 类型）
                std::map<std::string, ConfigNode> object_value; ///< 对象值（OBJECT 类型）

                /**
                 * @brief 默认构造函数 - 创建空字符串节点
                 */
                ConfigNode() : type(ConfigNodeType::STRING) {}

                /**
                 * @brief 字符串构造函数
                 * @param value 字符串值
                 */
                explicit ConfigNode(const std::string &value)
                    : type(ConfigNodeType::STRING), string_value(value) {}

                // ========== 类型转换 ==========

                /**
                 * @brief 隐式转换为字符串
                 * @return 字符串值（如果不是 STRING 类型，返回空字符串）
                 */
                operator std::string() const
                {
                    return (type == ConfigNodeType::STRING) ? string_value : "";
                }

                // ========== 访问操作符 ==========

                /**
                 * @brief 通过键访问对象成员（非 const）
                 * @param key 键名
                 * @return 对应的配置节点引用
                 * @note 如果当前节点不是 OBJECT 类型，会自动转换为 OBJECT
                 */
                ConfigNode &operator[](const std::string &key)
                {
                    if (type != ConfigNodeType::OBJECT)
                    {
                        type = ConfigNodeType::OBJECT;
                    }
                    return object_value[key];
                }

                /**
                 * @brief 通过键访问对象成员（const）
                 * @param key 键名
                 * @return 对应的配置节点引用（如果键不存在，返回空节点）
                 */
                const ConfigNode &operator[](const std::string &key) const
                {
                    static ConfigNode empty;
                    auto it = object_value.find(key);
                    return (it != object_value.end()) ? it->second : empty;
                }

                /**
                 * @brief 通过索引访问数组元素（非 const）
                 * @param index 索引
                 * @return 对应的配置节点引用（如果索引越界，返回空节点）
                 */
                ConfigNode &operator[](size_t index)
                {
                    if (type == ConfigNodeType::ARRAY && index < array_value.size())
                    {
                        return array_value[index];
                    }
                    static ConfigNode empty;
                    return empty;
                }

                /**
                 * @brief 通过索引访问数组元素（const）
                 * @param index 索引
                 * @return 对应的配置节点引用（如果索引越界，返回空节点）
                 */
                const ConfigNode &operator[](size_t index) const
                {
                    if (type == ConfigNodeType::ARRAY && index < array_value.size())
                    {
                        return array_value[index];
                    }
                    static ConfigNode empty;
                    return empty;
                }

                // ========== 辅助方法 ==========

                /**
                 * @brief 判断节点是否为空
                 * @return 如果是空字符串节点，返回 true
                 */
                bool IsEmpty() const
                {
                    return (type == ConfigNodeType::STRING && string_value.empty());
                }

                /**
                 * @brief 判断对象是否包含指定键
                 * @param key 键名
                 * @return 如果是 OBJECT 类型且包含该键，返回 true
                 */
                bool HasKey(const std::string &key) const
                {
                    return (type == ConfigNodeType::OBJECT) &&
                           (object_value.find(key) != object_value.end());
                }

                /**
                 * @brief 获取数组大小
                 * @return 如果是 ARRAY 类型，返回元素个数；否则返回 0
                 */
                size_t Size() const
                {
                    return (type == ConfigNodeType::ARRAY) ? array_value.size() : 0;
                }

                /**
                 * @brief 显式获取字符串值
                 * @return 字符串值
                 */
                std::string AsString() const
                {
                    return string_value;
                }

                // ========== 迭代器支持（用于数组遍历） ==========

                auto begin() { return array_value.begin(); }
                auto end() { return array_value.end(); }
                auto begin() const { return array_value.cbegin(); }
                auto end() const { return array_value.cend(); }
            };

        } // namespace common
    } // namespace framework
} // namespace humanoid_robot

#endif // HUMANOID_ROBOT_CONFIG_NODE_H
