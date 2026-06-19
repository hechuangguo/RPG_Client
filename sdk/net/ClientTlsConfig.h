/**
 * @file    ClientTlsConfig.h
 * @brief   客户端 TLS 传输层配置
 */

#pragma once

#include <string>

/**
 * @brief 客户端 TLS 配置（client_config.xml 的 Tls 段）
 */
struct ClientTlsConfig
{
    bool        enabled            = true;
    std::string caPath             = "config/tls/ca.crt";
    bool        insecureSkipVerify = false;
    std::string minVersion         = "1.2";
};
