/**
 * @file    ClientTlsContext.h
 * @brief   客户端 OpenSSL SSL_CTX 单例（外联 Login/Gateway，仅校验服务端证书）
 */

#pragma once

#include "net/ClientTlsConfig.h"

#include <string>

struct ssl_st;

/**
 * @brief 进程级 TLS 上下文
 */
class ClientTlsContext
{
public:
    static ClientTlsContext& instance();

    /**
     * @brief 根据配置初始化 SSL_CTX
     * @param cfg TLS 配置
     * @param errOut 失败时错误描述
     * @return enabled=0 或初始化成功返回 true
     */
    bool init(const ClientTlsConfig& cfg, std::string* errOut = nullptr);

    bool enabled() const;

    /**
     * @brief 为已连接 socket 创建客户端 SSL 对象（connect 状态）
     * @param fd WinSock SOCKET 句柄
     */
    ssl_st* newSsl(int fd);

    void freeSsl(ssl_st* ssl);

private:
    ClientTlsContext();
    ~ClientTlsContext();
    ClientTlsContext(const ClientTlsContext&) = delete;
    ClientTlsContext& operator=(const ClientTlsContext&) = delete;

    bool createCtx(std::string* errOut);

    ClientTlsConfig m_config;
    bool            m_enabled;
    void*           m_ctx;
};
