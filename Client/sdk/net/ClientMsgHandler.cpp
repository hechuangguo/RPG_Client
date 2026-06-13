/**
 * @file    ClientMsgHandler.cpp
 * @brief   客户端消息构造与解析辅助实现
 */

#include "ClientMsgHandler.h"

#include "ProtocolCodec.h"

#include <cstring>

namespace
{
constexpr uint8_t kLoginModule  = static_cast<uint8_t>(ClientModule::LOGIN);
constexpr uint8_t kSceneModule  = static_cast<uint8_t>(ClientModule::SCENE);
constexpr uint8_t kSystemModule = static_cast<uint8_t>(ClientModule::SYSTEM);

constexpr uint8_t kLoginReqSub     = msgSub(static_cast<uint16_t>(ClientMsgID::C2S_LOGIN_REQ));
constexpr uint8_t kRegisterReqSub  = msgSub(static_cast<uint16_t>(ClientMsgID::C2S_REGISTER_REQ));
constexpr uint8_t kMoveReqSub      = msgSub(static_cast<uint16_t>(ClientMsgID::C2S_MOVE_REQ));
constexpr uint8_t kHeartbeatSub    = msgSub(static_cast<uint16_t>(ClientMsgID::C2S_HEARTBEAT));
}  // namespace

void ClientMsgHandler::copyFixedString(char* dest, size_t destSize, const std::string& src)
{
    if (destSize == 0)
    {
        return;
    }
    std::memset(dest, 0, destSize);
    const size_t copyLen = (src.size() < destSize - 1) ? src.size() : (destSize - 1);
    if (copyLen > 0)
    {
        std::memcpy(dest, src.c_str(), copyLen);
    }
}

bool ClientMsgHandler::copyStruct(const char* data, uint16_t len, void* out, size_t structSize)
{
    if (!data || !out || len < structSize)
    {
        return false;
    }
    std::memcpy(out, data, structSize);
    return true;
}

std::vector<char> ClientMsgHandler::buildLoginReq(const std::string& account,
                                                  const std::string& password,
                                                  uint32_t zoneId,
                                                  uint8_t gameType)
{
    Msg_C2S_LoginReq body{};
    copyFixedString(body.account, sizeof(body.account), account);
    copyFixedString(body.password, sizeof(body.password), password);
    body.zoneId   = zoneId;
    body.gameType = gameType;
    std::memset(body.reserved, 0, sizeof(body.reserved));

    return ProtocolCodec::encode(kLoginModule, kLoginReqSub,
                                 reinterpret_cast<const char*>(&body),
                                 static_cast<uint16_t>(sizeof(body)));
}

std::vector<char> ClientMsgHandler::buildRegisterReq(const std::string& account,
                                                     const std::string& password,
                                                     uint32_t zoneId,
                                                     uint8_t gameType)
{
    Msg_C2S_RegisterReq body{};
    copyFixedString(body.account, sizeof(body.account), account);
    copyFixedString(body.password, sizeof(body.password), password);
    body.zoneId   = zoneId;
    body.gameType = gameType;
    std::memset(body.reserved, 0, sizeof(body.reserved));

    return ProtocolCodec::encode(kLoginModule, kRegisterReqSub,
                                 reinterpret_cast<const char*>(&body),
                                 static_cast<uint16_t>(sizeof(body)));
}

std::vector<char> ClientMsgHandler::buildHeartbeat(uint32_t seq)
{
    Msg_C2S_Heartbeat body{};
    body.seq = seq;

    return ProtocolCodec::encode(kSystemModule, kHeartbeatSub,
                                 reinterpret_cast<const char*>(&body),
                                 static_cast<uint16_t>(sizeof(body)));
}

std::vector<char> ClientMsgHandler::buildMoveReq(uint64_t userID,
                                                 float x, float y, float z,
                                                 float dir,
                                                 uint8_t moveType)
{
    Msg_C2S_MoveReq body{};
    body.userID   = userID;
    body.x        = x;
    body.y        = y;
    body.z        = z;
    body.dir      = dir;
    body.moveType = moveType;

    return ProtocolCodec::encode(kSceneModule, kMoveReqSub,
                                 reinterpret_cast<const char*>(&body),
                                 static_cast<uint16_t>(sizeof(body)));
}

bool ClientMsgHandler::parseLoginRsp(const char* data, uint16_t len, Msg_S2C_LoginRsp& out)
{
    return copyStruct(data, len, &out, sizeof(out));
}

bool ClientMsgHandler::parseGatewayInfo(const char* data, uint16_t len, Msg_S2C_GatewayInfo& out)
{
    return copyStruct(data, len, &out, sizeof(out));
}

bool ClientMsgHandler::parseEnterGame(const char* data, uint16_t len, Msg_S2C_EnterGame& out)
{
    return copyStruct(data, len, &out, sizeof(out));
}

bool ClientMsgHandler::parseSpawnEntity(const char* data, uint16_t len, Msg_S2C_SpawnEntity& out)
{
    return copyStruct(data, len, &out, sizeof(out));
}

bool ClientMsgHandler::parseMoveNotify(const char* data, uint16_t len, Msg_S2C_MoveNotify& out)
{
    return copyStruct(data, len, &out, sizeof(out));
}
