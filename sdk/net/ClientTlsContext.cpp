/**
 * @file    ClientTlsContext.cpp
 * @brief   ClientTlsContext 实现
 */

#include "net/ClientTlsContext.h"

#include "log/ClientLogger.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <cstdio>
#include <string>

namespace
{
bool fileExists(const std::string& path)
{
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
    {
        return false;
    }
    std::fclose(f);
    return true;
}

int tlsMinVersion(const std::string& ver)
{
    if (ver == "1.3")
    {
        return TLS1_3_VERSION;
    }
    return TLS1_2_VERSION;
}

void logOpenSslErrors(const char* prefix)
{
    unsigned long err = 0;
    while ((err = ERR_get_error()) != 0)
    {
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        ClientLogger::instance().warn("%s：%s", prefix, buf);
    }
}
}  // namespace

ClientTlsContext& ClientTlsContext::instance()
{
    static ClientTlsContext ctx;
    return ctx;
}

ClientTlsContext::ClientTlsContext()
    : m_enabled(false)
    , m_ctx(nullptr)
{
}

ClientTlsContext::~ClientTlsContext()
{
    if (m_ctx)
    {
        SSL_CTX_free(static_cast<SSL_CTX*>(m_ctx));
        m_ctx = nullptr;
    }
}

bool ClientTlsContext::init(const ClientTlsConfig& cfg, std::string* errOut)
{
    m_config  = cfg;
    m_enabled = cfg.enabled;

    if (m_ctx)
    {
        SSL_CTX_free(static_cast<SSL_CTX*>(m_ctx));
        m_ctx = nullptr;
    }

    if (!m_enabled)
    {
        ClientLogger::instance().info("ClientTlsContext：TLS 未启用");
        return true;
    }

    if (!cfg.insecureSkipVerify && !fileExists(cfg.caPath))
    {
        if (errOut)
        {
            *errOut = "CA 证书文件缺失：" + cfg.caPath
                      + "（请从服务端 config/tls 复制 ca.crt）";
        }
        return false;
    }

    if (!createCtx(errOut))
    {
        return false;
    }

    ClientLogger::instance().info("ClientTlsContext：TLS 已启用 ca=%s min=%s verify=%d",
                                  cfg.caPath.c_str(),
                                  cfg.minVersion.c_str(),
                                  cfg.insecureSkipVerify ? 0 : 1);
    return true;
}

bool ClientTlsContext::enabled() const
{
    return m_enabled;
}

bool ClientTlsContext::createCtx(std::string* errOut)
{
    OPENSSL_init_ssl(0, nullptr);

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx)
    {
        if (errOut)
        {
            *errOut = "SSL_CTX_new 失败";
        }
        logOpenSslErrors("ClientTlsContext");
        return false;
    }

    SSL_CTX_set_min_proto_version(ctx, tlsMinVersion(m_config.minVersion));

    if (m_config.insecureSkipVerify)
    {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    }
    else
    {
        if (SSL_CTX_load_verify_locations(ctx, m_config.caPath.c_str(), nullptr) <= 0)
        {
            if (errOut)
            {
                *errOut = "加载 CA 失败：" + m_config.caPath;
            }
            logOpenSslErrors("ClientTlsContext");
            SSL_CTX_free(ctx);
            return false;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    }

    m_ctx = ctx;
    return true;
}

ssl_st* ClientTlsContext::newSsl(int fd)
{
    if (!m_enabled || !m_ctx || fd < 0)
    {
        return nullptr;
    }

    SSL* ssl = SSL_new(static_cast<SSL_CTX*>(m_ctx));
    if (!ssl)
    {
        return nullptr;
    }

    SSL_set_fd(ssl, fd);
    SSL_set_connect_state(ssl);
    return ssl;
}

void ClientTlsContext::freeSsl(ssl_st* ssl)
{
    if (!ssl)
    {
        return;
    }

    SSL_shutdown(static_cast<SSL*>(ssl));
    SSL_free(static_cast<SSL*>(ssl));
}
