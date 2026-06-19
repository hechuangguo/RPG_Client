/**
 * @file    ConfigLoader.h
 * @brief   客户端 XML 配置文件加载器
 *
 * 职责：
 * - 从 config/client_config.xml 读取窗口尺寸、日志级别、网络时序等
 * - 提供 loginHost/loginPort（LoginServer 连接地址）
 *
 * 协作：GameApp 启动时 load()；各 Session 通过 setConfig 读取时序。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>

#include "net/ClientTlsConfig.h"

/**
 * @brief 网络时序配置视图
 */
struct NetworkTiming
{
    int64_t connectTimeoutMs;
    int64_t responseTimeoutMs;
    int64_t zoneListResponseTimeoutMs;
    int64_t heartbeatIntervalMs;
    int64_t moveSendIntervalMs;
    int64_t logoutTimeoutMs;
};

/**
 * @brief client_config.xml 解析器（轻量键值解析，无第三方 XML 库）
 */
class ConfigLoader
{
public:
    ConfigLoader();

    /**
     * @brief 加载 XML 配置文件
     * @param path 文件路径（通常为 ./config/client_config.xml）
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

    int64_t connectTimeoutMs() const;
    int64_t responseTimeoutMs() const;
    int64_t zoneListResponseTimeoutMs() const;
    int64_t heartbeatIntervalMs() const;
    int64_t moveSendIntervalMs() const;
    int64_t logoutTimeoutMs() const;

    /** @brief 网络时序配置聚合视图 */
    NetworkTiming networkTiming() const;

    /** @brief TLS 传输层配置 */
    const ClientTlsConfig& tls() const;

private:
    void applyDefaults();
    bool parseXmlContent(const std::string& content);

    std::string m_lastError;
    unsigned    m_windowWidth;
    unsigned    m_windowHeight;
    std::string m_logLevel;
    bool        m_logToConsole;
    std::string m_loginHost;
    uint16_t    m_loginPort;

    int64_t m_connectTimeoutMs;
    int64_t m_responseTimeoutMs;
    int64_t m_zoneListResponseTimeoutMs;
    int64_t m_heartbeatIntervalMs;
    int64_t m_moveSendIntervalMs;
    int64_t m_logoutTimeoutMs;

    ClientTlsConfig m_tls;
};
