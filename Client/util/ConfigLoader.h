/**
 * @file    ConfigLoader.h
 * @brief   客户端 JSON 配置文件加载器
 *
 * 职责：
 * - 从 config/client_config.json 读取窗口尺寸、日志级别等简单键值
 * - 提供 loginHost/loginPort（LoginServer 连接地址）
 *
 * 协作：GameApp 启动时 load()；ClientLogger 读取 logToConsole。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief client_config.json 解析器（轻量键值解析，无第三方 JSON 库）
 */
class ConfigLoader
{
public:
    ConfigLoader();

    /**
     * @brief 加载 JSON 配置文件
     * @param path 文件路径（通常为 ./config/client_config.json）
     * @return 文件存在且解析成功返回 true；失败时保留默认值
     */
    bool load(const std::string& path);

    /** @brief 最近一次 load 的错误描述（成功时为空） */
    const std::string& lastError() const;

    /** @brief 窗口宽度（像素） */
    unsigned windowWidth() const;

    /** @brief 窗口高度（像素） */
    unsigned windowHeight() const;

    /** @brief 日志级别字符串：info / warn / err */
    const std::string& logLevel() const;

    /** @brief 是否同时输出日志到控制台 */
    bool logToConsole() const;

    /** @brief LoginServer fallback IP */
    const std::string& loginHost() const;

    /** @brief LoginServer fallback 端口 */
    uint16_t loginPort() const;

private:
    void applyDefaults();
    bool parseJsonContent(const std::string& content);

    std::string m_lastError;
    unsigned    m_windowWidth;
    unsigned    m_windowHeight;
    std::string m_logLevel;
    bool        m_logToConsole;
    std::string m_loginHost;
    uint16_t    m_loginPort;
};
