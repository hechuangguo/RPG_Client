/**
 * @file    ClientLocalError.h
 * @brief   客户端本地错误码（非 wire 协议）
 */

#pragma once

#include <cstdint>

/** @brief 客户端本地错误码（UI 提示与日志） */
enum class ClientLocalError : int32_t
{
    None = 0,
    ConnectFailed  = 1001, /**< TCP 连接失败 */
    ConnectTimeout = 1002, /**< 连接握手超时 */
    ResponseTimeout = 1003, /**< 等待服务端响应超时 */
    Disconnected   = 1004, /**< 连接意外断开 */
    ParseError     = 1005, /**< 消息解析失败 */
    GatewayError   = 1006, /**< 收到 S2C_ERROR */
    TlsHandshakeFailed = 1007, /**< TLS 握手失败 */
};

/**
 * @brief 登录/区列表超时上下文（用于细化 ResponseTimeout 文案）
 */
enum class LoginTimeoutContext : int32_t
{
    Generic = 0,
    Login,
    Register,
    UserList,
    EnterGame,
    Logout,
    CreateCharacter,
    ZoneList,
    GatewayConnect,
    LoginServerConnect,
};
