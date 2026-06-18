/**
 * @file    ClientMsgHandler.h
 * @brief   客户端消息构造与解析辅助（wire v2）
 *
 * 对 Common/LoginMsg.h、ZoneMsg.h、MapDataMsg.h 等提供 build/parse 接口。
 * 组包须 initClientMsg；解包须 clientMsgBodyMatches。
 */

#pragma once

#include "ClientProtocol.h"
#include "net/CharacterTypes.h"

#include <cstdint>
#include <string>
#include <vector>

struct GameZoneEntry;

class ClientMsgHandler
{
public:
    static std::vector<char> buildLoginReq(const std::string& account,
                                           const std::string& password,
                                           uint32_t zoneId,
                                           uint8_t gameType);

    static std::vector<char> buildRegisterReq(const std::string& account,
                                              const std::string& password,
                                              const std::string& confirmPassword,
                                              uint32_t zoneId,
                                              uint8_t gameType);

    static std::vector<char> buildZoneListReq(uint8_t gameType = ZONE_LIST_ALL_GAME_TYPES);

    static std::vector<char> buildHeartbeat(uint32_t seq);

    static std::vector<char> buildMoveReq(uint64_t userID,
                                          float x, float y, float z,
                                          float dir,
                                          uint8_t moveType);

    static std::vector<char> buildGatewayAuthReq(const std::string& account,
                                                 const std::string& loginToken,
                                                 uint32_t zoneId,
                                                 uint8_t gameType);

    static std::vector<char> buildSelectUserReq(uint64_t userID, uint64_t loginTxnId = 0);

    static std::vector<char> buildCreateUserReq(const std::string& name,
                                                uint8_t vocation,
                                                uint8_t sex);

    static bool parseZoneListRsp(const char* data,
                                 uint16_t len,
                                 std::vector<GameZoneEntry>& out,
                                 std::string& errMsg);

    static bool parseLoginRsp(const char* data, uint16_t len, Msg_S2C_LoginRsp& out);

    static bool parseRegisterRsp(const char* data, uint16_t len, Msg_S2C_RegisterRsp& out);

    static bool parseGatewayInfo(const char* data, uint16_t len, Msg_S2C_GatewayInfo& out);

    static bool parseEnterGame(const char* data, uint16_t len, Msg_S2C_EnterGame& out);

    static bool parseUserList(const char* data,
                              uint16_t len,
                              std::vector<CharacterEntry>& out,
                              std::string& errMsg);

    static bool parseCreateUserRsp(const char* data, uint16_t len, Msg_S2C_CreateUserRsp& out);

    static bool parseSpawnEntity(const char* data, uint16_t len, Msg_S2C_SpawnEntity& out);

    static bool parseMoveNotify(const char* data, uint16_t len, Msg_S2C_MoveNotify& out);

    static bool parseDespawnEntity(const char* data, uint16_t len, Msg_S2C_DespawnEntity& out);

    static bool parseGatewayError(const char* data, uint16_t len, Msg_S2C_Error& out);

    static std::string gatewayErrorText(const Msg_S2C_Error& err);

private:
    static void copyFixedString(char* dest, size_t destSize, const std::string& src);
};
