/**
 * @file    ClientErrorText.h
 * @brief   错误码到用户可见中文文案的统一映射
 */

#pragma once

#include "LoginCommon.h"
#include "net/ClientLocalError.h"

#include <string>

struct Msg_S2C_Error;

class ClientErrorText
{
public:
    static std::string loginResultText(LoginResultCode code, const char* serverMsg);
    static std::string registerResultText(RegisterResultCode code, const char* serverMsg);
    static std::string createCharacterText(CreateCharacterResultCode code, const char* serverMsg);
    static std::string gatewayInfoText(GatewayInfoResultCode code, const char* serverMsg);
    static std::string logoutResultText(LogoutResultCode code, const char* serverMsg);
    static std::string gatewayValidateText(GatewayValidateCode code, const char* serverMsg);
    static std::string gatewayValidateText(const Msg_S2C_Error& err);
    static std::string userListResultText(UserListResultCode code, const char* serverMsg);
    static std::string zoneListResultText(int32_t code, const char* serverMsg = nullptr);

    static std::string localErrorText(ClientLocalError err,
                                      LoginTimeoutContext ctx = LoginTimeoutContext::Generic);

private:
    static std::string preferServerMsg(const char* serverMsg, const std::string& fallback);
};
