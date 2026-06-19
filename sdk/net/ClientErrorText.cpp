/**
 * @file    ClientErrorText.cpp
 * @brief   ClientErrorText 实现
 */

#include "net/ClientErrorText.h"

#include "LoginMsg.h"

#include <cstring>

std::string ClientErrorText::preferServerMsg(const char* serverMsg, const std::string& fallback)
{
    if (serverMsg && serverMsg[0] != '\0')
    {
        return serverMsg;
    }
    return fallback;
}

std::string ClientErrorText::loginResultText(LoginResultCode code, const char* serverMsg)
{
    if (code == LoginResultCode::OK)
    {
        return {};
    }
    const std::string detail = preferServerMsg(
        serverMsg,
        code == LoginResultCode::BadCredentials ? u8"账号或密码错误" : u8"服务器错误");
    return std::string(u8"登录失败：") + detail;
}

std::string ClientErrorText::registerResultText(RegisterResultCode code, const char* serverMsg)
{
    if (code == RegisterResultCode::OK)
    {
        return {};
    }
    const std::string detail = preferServerMsg(
        serverMsg,
        code == RegisterResultCode::AccountExists ? u8"账号已存在" : u8"服务器错误");
    return std::string(u8"注册失败：") + detail;
}

std::string ClientErrorText::createCharacterText(CreateCharacterResultCode code,
                                                 const char* serverMsg)
{
    if (code == CreateCharacterResultCode::OK)
    {
        return {};
    }
    const std::string detail = preferServerMsg(
        serverMsg,
        code == CreateCharacterResultCode::DuplicateName ? u8"角色名不可用" : u8"系统错误");
    return std::string(u8"创建角色失败：") + detail;
}

std::string ClientErrorText::gatewayInfoText(GatewayInfoResultCode code, const char* serverMsg)
{
    if (code == GatewayInfoResultCode::OK)
    {
        return {};
    }
    std::string err = u8"无可用网关";
    if (serverMsg && serverMsg[0] != '\0')
    {
        err += u8"：";
        err += serverMsg;
    }
    return err;
}

std::string ClientErrorText::logoutResultText(LogoutResultCode code, const char* serverMsg)
{
    if (code == LogoutResultCode::OK)
    {
        return {};
    }
    std::string err = u8"离开世界失败";
    if (serverMsg && serverMsg[0] != '\0')
    {
        err += u8"：";
        err += serverMsg;
    }
    return err;
}

std::string ClientErrorText::gatewayValidateText(GatewayValidateCode code, const char* serverMsg)
{
    if (serverMsg && serverMsg[0] != '\0')
    {
        return serverMsg;
    }

    switch (code)
    {
    case GatewayValidateCode::UNKNOWN_MSG:
        return u8"未知消息类型";
    case GatewayValidateCode::BAD_LENGTH:
        return u8"消息长度非法";
    case GatewayValidateCode::BAD_STATE:
        return u8"连接状态不允许该操作";
    case GatewayValidateCode::BAD_PAYLOAD:
        return u8"请求参数非法";
    case GatewayValidateCode::RATE_LIMITED:
        return u8"请求过于频繁";
    default:
        break;
    }
    return u8"网关校验失败";
}

std::string ClientErrorText::gatewayValidateText(const Msg_S2C_Error& err)
{
    return gatewayValidateText(static_cast<GatewayValidateCode>(err.code), err.msg);
}

std::string ClientErrorText::localErrorText(ClientLocalError err, LoginTimeoutContext ctx)
{
    switch (err)
    {
    case ClientLocalError::ConnectFailed:
        if (ctx == LoginTimeoutContext::ZoneList)
        {
            return u8"无法连接 LoginServer，请确认服务器已启动";
        }
        if (ctx == LoginTimeoutContext::LoginServerConnect)
        {
            return u8"无法连接 LoginServer，请确认服务器已启动并监听 9010 端口";
        }
        if (ctx == LoginTimeoutContext::GatewayConnect)
        {
            return u8"无法连接游戏网关，请确认 Gateway 已启动";
        }
        return u8"无法连接服务器，请检查 loginHost/loginPort";

    case ClientLocalError::ConnectTimeout:
        if (ctx == LoginTimeoutContext::GatewayConnect)
        {
            return u8"连接游戏网关超时，请检查网络与配置";
        }
        if (ctx == LoginTimeoutContext::LoginServerConnect)
        {
            return u8"连接 LoginServer 超时，请检查 loginHost/loginPort 配置";
        }
        return u8"连接超时，请检查网络与配置";

    case ClientLocalError::ResponseTimeout:
        switch (ctx)
        {
        case LoginTimeoutContext::Login:
            return u8"验证账号超时，服务器未响应";
        case LoginTimeoutContext::Register:
            return u8"注册超时，服务器未响应";
        case LoginTimeoutContext::UserList:
            return u8"获取角色列表超时，服务器未响应";
        case LoginTimeoutContext::EnterGame:
            return u8"进入游戏超时，服务器未响应";
        case LoginTimeoutContext::Logout:
            return u8"离开世界超时，服务器未响应";
        case LoginTimeoutContext::CreateCharacter:
            return u8"创建角色超时，服务器未响应";
        case LoginTimeoutContext::ZoneList:
            return u8"获取区列表超时，服务器未响应";
        default:
            return u8"服务器响应超时";
        }

    case ClientLocalError::Disconnected:
        switch (ctx)
        {
        case LoginTimeoutContext::Login:
            return u8"验证账号时连接已断开";
        case LoginTimeoutContext::Register:
            return u8"注册时连接已断开";
        case LoginTimeoutContext::UserList:
            return u8"获取角色列表时连接已断开";
        case LoginTimeoutContext::EnterGame:
            return u8"进入游戏时连接已断开";
        case LoginTimeoutContext::CreateCharacter:
            return u8"创建角色时连接已断开";
        case LoginTimeoutContext::ZoneList:
            return u8"获取区列表时连接已断开";
        case LoginTimeoutContext::GatewayConnect:
            return u8"无法连接游戏网关，请确认 Gateway 已启动";
        case LoginTimeoutContext::LoginServerConnect:
            return u8"无法连接 LoginServer，请确认服务器已启动并监听 9010 端口";
        default:
            return u8"连接已断开";
        }

    case ClientLocalError::ParseError:
        return u8"消息解析失败";

    case ClientLocalError::GatewayError:
        return u8"网关校验失败";

    case ClientLocalError::TlsHandshakeFailed:
        if (ctx == LoginTimeoutContext::GatewayConnect)
        {
            return u8"TLS 握手失败，请确认 Gateway 已启用 TLS";
        }
        if (ctx == LoginTimeoutContext::LoginServerConnect)
        {
            return u8"TLS 握手失败，请确认 LoginServer 已启用 TLS 且 ca.crt 正确";
        }
        return u8"TLS 握手失败";

    default:
        return u8"未知错误";
    }
}
