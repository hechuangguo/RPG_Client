/**
 * @file    ClientMsgHandler.cpp
 * @brief   客户端消息构造与解析辅助实现（wire v2）
 */

#include "ClientMsgHandler.h"

#include "ProtocolCodec.h"
#include "log/ClientLogger.h"
#include "net/ClientErrorText.h"
#include "net/ZoneTypes.h"
#include "util/PasswordDigest.h"

#include <cstring>

namespace
{
ZoneLoadStatus loadStatusFromWire(uint8_t enabled, uint8_t loadLevel)
{
    if (enabled == 0)
    {
        return ZoneLoadStatus::Maintenance;
    }
    if (loadLevel <= static_cast<uint8_t>(ZoneLoadLevel::MAINTENANCE))
    {
        return static_cast<ZoneLoadStatus>(loadLevel);
    }
    return ZoneLoadStatus::Smooth;
}

template<typename MsgT>
std::vector<char> encodeWire(MsgT& body)
{
    initClientMsg(body);
    return ProtocolCodec::encode(MsgT::kModule,
                                 MsgT::kSub,
                                 reinterpret_cast<const char*>(&body),
                                 static_cast<uint16_t>(sizeof(body)));
}

template<typename MsgT>
bool parseWire(const char* data, uint16_t len, MsgT& out)
{
    if (!data || len != sizeof(MsgT))
    {
        ClientLogger::instance().warn(
            "ClientMsgHandler：固定包长度不符 module=%u sub=%u len=%u expected=%zu",
            static_cast<unsigned>(MsgT::kModule),
            static_cast<unsigned>(MsgT::kSub),
            static_cast<unsigned>(len),
            sizeof(MsgT));
        return false;
    }
    if (!clientMsgBodyMatches(MsgT::kModule, MsgT::kSub, data, len))
    {
        return false;
    }
    std::memcpy(&out, data, sizeof(out));
    return true;
}

template<typename MsgT>
bool parseMsgHeader(const char* data, uint16_t len, MsgT& out)
{
    if (!data || len < sizeof(MsgT))
    {
        return false;
    }
    if (!clientMsgBodyMatches(MsgT::kModule, MsgT::kSub, data, len))
    {
        return false;
    }
    std::memcpy(&out, data, sizeof(MsgT));
    return true;
}
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

std::vector<char> ClientMsgHandler::buildLoginReq(const std::string& account,
                                                  const std::string& password,
                                                  uint32_t zoneId,
                                                  uint8_t gameType)
{
    Msg_C2S_LoginReq body{};
    copyFixedString(body.account, sizeof(body.account), account);
    std::memset(body.passwordDigest, 0, sizeof(body.passwordDigest));
    PasswordDigest::sha256Utf8Password(password, body.passwordDigest);
    body.zoneId   = zoneId;
    body.gameType = gameType;
    std::memset(body.reserved, 0, sizeof(body.reserved));
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildRegisterReq(const std::string& account,
                                                     const std::string& password,
                                                     const std::string& confirmPassword,
                                                     uint32_t zoneId,
                                                     uint8_t gameType)
{
    Msg_C2S_RegisterReq body{};
    copyFixedString(body.account, sizeof(body.account), account);
    std::memset(body.passwordDigest, 0, sizeof(body.passwordDigest));
    std::memset(body.confirmPasswordDigest, 0, sizeof(body.confirmPasswordDigest));
    PasswordDigest::sha256Utf8Password(password, body.passwordDigest);
    PasswordDigest::sha256Utf8Password(confirmPassword, body.confirmPasswordDigest);
    body.zoneId   = zoneId;
    body.gameType = gameType;
    std::memset(body.reserved, 0, sizeof(body.reserved));
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildZoneListReq(uint8_t gameType)
{
    Msg_C2S_ZoneListReq body{};
    body.gameType = gameType;
    return encodeWire(body);
}

bool ClientMsgHandler::parseZoneListRsp(const char* data,
                                        uint16_t len,
                                        std::vector<GameZoneEntry>& out,
                                        std::string& errMsg)
{
    out.clear();
    errMsg.clear();

    if (!data || len < sizeof(Msg_S2C_ZoneListRspHeader))
    {
        errMsg = u8"区列表响应过短";
        return false;
    }

    Msg_S2C_ZoneListRspHeader hdr{};
    if (!parseMsgHeader(data, len, hdr))
    {
        errMsg = u8"区列表响应格式错误";
        return false;
    }

    if (hdr.code != 0)
    {
        errMsg = ClientErrorText::zoneListResultText(hdr.code, nullptr);
        return false;
    }

    if (hdr.count == 0)
    {
        if (len != zoneListBodyLen(0))
        {
            errMsg = u8"区列表数据不完整";
            return false;
        }
        return true;
    }

    const bool wireV2 = len == zoneListBodyLen(hdr.count);
    const bool wireV1 = len == zoneListBodyLenV1(hdr.count);
    if (!wireV1 && !wireV2)
    {
        errMsg = u8"区列表数据不完整";
        return false;
    }

    const size_t entrySize =
        wireV2 ? sizeof(Msg_S2C_ZoneEntryWire) : sizeof(Msg_S2C_ZoneEntryWireV1);
    const char* p = data + sizeof(Msg_S2C_ZoneListRspHeader);
    for (uint16_t i = 0; i < hdr.count; ++i)
    {
        GameZoneEntry zone{};

        if (wireV2)
        {
            Msg_S2C_ZoneEntryWire entry{};
            std::memcpy(&entry, p, sizeof(entry));

            zone.zoneId       = entry.zoneId;
            zone.gameType     = entry.gameType;
            zone.enabled      = entry.enabled != 0;
            zone.name         = entry.name;
            zone.onlineCount  = entry.onlineCount;
            zone.gatewayCount = entry.gatewayCount;
            zone.loadStatus   = loadStatusFromWire(entry.enabled, entry.loadLevel);
        }
        else
        {
            Msg_S2C_ZoneEntryWireV1 entry{};
            std::memcpy(&entry, p, sizeof(entry));

            zone.zoneId   = entry.zoneId;
            zone.gameType = entry.gameType;
            zone.enabled  = entry.enabled != 0;
            zone.name     = entry.name;
            zone.loadStatus =
                zone.enabled ? ZoneLoadStatus::Smooth : ZoneLoadStatus::Maintenance;
        }

        out.push_back(zone);
        p += entrySize;
    }
    return true;
}

std::vector<char> ClientMsgHandler::buildHeartbeat(uint32_t seq)
{
    Msg_C2S_Heartbeat body{};
    body.seq = seq;
    return encodeWire(body);
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
    return encodeWire(body);
}

bool ClientMsgHandler::parseLoginRsp(const char* data, uint16_t len, Msg_S2C_LoginRsp& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseRegisterRsp(const char* data, uint16_t len, Msg_S2C_RegisterRsp& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseGatewayInfo(const char* data, uint16_t len, Msg_S2C_GatewayInfo& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseEnterGame(const char* data, uint16_t len, Msg_S2C_EnterGame& out)
{
    return parseWire(data, len, out);
}

std::vector<char> ClientMsgHandler::buildGatewayAuthReq(const std::string& account,
                                                        const std::string& loginToken,
                                                        uint32_t zoneId,
                                                        uint8_t gameType)
{
    Msg_C2S_GatewayAuthReq body{};
    copyFixedString(body.account, sizeof(body.account), account);
    copyFixedString(body.loginToken, sizeof(body.loginToken), loginToken);
    body.zoneId   = zoneId;
    body.gameType = gameType;
    std::memset(body.reserved, 0, sizeof(body.reserved));
    return encodeWire(body);
}

bool ClientMsgHandler::parseUserList(const char* data,
                                     uint16_t len,
                                     std::vector<CharacterEntry>& out,
                                     std::string& errMsg)
{
    out.clear();
    errMsg.clear();

    if (!data || len < sizeof(Msg_S2C_UserListHeader))
    {
        errMsg = u8"角色列表响应过短";
        return false;
    }

    Msg_S2C_UserListHeader hdr{};
    if (!parseMsgHeader(data, len, hdr))
    {
        errMsg = u8"角色列表响应格式错误";
        return false;
    }

    if (static_cast<UserListResultCode>(hdr.code) != UserListResultCode::OK)
    {
        errMsg = ClientErrorText::userListResultText(static_cast<UserListResultCode>(hdr.code),
                                                     nullptr);
        return false;
    }

    if (len != userListBodyLen(hdr.count))
    {
        errMsg = u8"角色列表数据不完整";
        return false;
    }

    const char* p = data + sizeof(Msg_S2C_UserListHeader);
    for (uint16_t i = 0; i < hdr.count; ++i)
    {
        Msg_S2C_UserListEntryWire wire{};
        std::memcpy(&wire, p, sizeof(wire));

        CharacterEntry entry{};
        entry.userID   = wire.userID;
        entry.name     = wire.name;
        entry.level    = wire.level;
        entry.vocation = wire.vocation;
        entry.sex      = wire.sex;
        out.push_back(entry);
        p += sizeof(wire);
    }
    return true;
}

std::vector<char> ClientMsgHandler::buildSelectUserReq(uint64_t userID, uint64_t loginTxnId)
{
    Msg_C2S_SelectUserReq body{};
    body.userID     = userID;
    body.loginTxnId = loginTxnId;
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildCreateUserReq(const std::string& name,
                                                       uint8_t vocation,
                                                       uint8_t sex)
{
    Msg_C2S_CreateUserReq body{};
    copyFixedString(body.name, sizeof(body.name), name);
    body.vocation = vocation;
    body.sex      = sex;
    std::memset(body.reserved, 0, sizeof(body.reserved));
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildLogoutReq(LogoutAction action)
{
    Msg_C2S_LogoutReq body{};
    body.action = static_cast<uint8_t>(action);
    std::memset(body.reserved, 0, sizeof(body.reserved));
    return encodeWire(body);
}

bool ClientMsgHandler::parseCreateUserRsp(const char* data, uint16_t len, Msg_S2C_CreateUserRsp& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseLogoutRsp(const char* data, uint16_t len, Msg_S2C_LogoutRsp& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseSpawnEntity(const char* data, uint16_t len, Msg_S2C_SpawnEntity& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseMoveNotify(const char* data, uint16_t len, Msg_S2C_MoveNotify& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseDespawnEntity(const char* data, uint16_t len, Msg_S2C_DespawnEntity& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseGatewayError(const char* data, uint16_t len, Msg_S2C_Error& out)
{
    return parseWire(data, len, out);
}

std::string ClientMsgHandler::gatewayErrorText(const Msg_S2C_Error& err)
{
    return ClientErrorText::gatewayValidateText(err);
}

std::vector<char> ClientMsgHandler::buildChatReq(uint8_t channel, const std::string& content)
{
    Msg_C2S_Chat body{};
    body.channel = channel;
    copyFixedString(body.content, sizeof(body.content), content);
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildQuestAcceptReq(uint32_t questId)
{
    Msg_C2S_QuestAcceptReq body{};
    body.questId = questId;
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildQuestSubmitReq(uint32_t questId)
{
    Msg_C2S_QuestSubmitReq body{};
    body.questId = questId;
    return encodeWire(body);
}

std::vector<char> ClientMsgHandler::buildBagInfoReq(uint64_t userID)
{
    Msg_C2S_BagInfoReq body{};
    body.userID = userID;
    return encodeWire(body);
}

bool ClientMsgHandler::parseHeartbeat(const char* data, uint16_t len, Msg_S2C_Heartbeat& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseNotice(const char* data, uint16_t len, Msg_S2C_Notice& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseChatNotify(const char* data, uint16_t len, Msg_S2C_Chat& out)
{
    return parseWire(data, len, out);
}

bool ClientMsgHandler::parseQuestInfo(const char* data,
                                      uint16_t len,
                                      std::vector<Msg_S2C_QuestEntryWire>& out,
                                      std::string& errMsg)
{
    out.clear();
    errMsg.clear();

    if (!data || len < sizeof(Msg_S2C_QuestInfoHeader))
    {
        errMsg = u8"任务同步响应过短";
        return false;
    }

    Msg_S2C_QuestInfoHeader hdr{};
    if (!parseMsgHeader(data, len, hdr))
    {
        errMsg = u8"任务同步响应格式错误";
        return false;
    }

    if (hdr.code != 0)
    {
        errMsg = u8"任务同步失败";
        return false;
    }

    if (len != questInfoBodyLen(hdr.count))
    {
        errMsg = u8"任务同步数据不完整";
        return false;
    }

    const char* p = data + sizeof(Msg_S2C_QuestInfoHeader);
    for (uint16_t i = 0; i < hdr.count; ++i)
    {
        Msg_S2C_QuestEntryWire wire{};
        std::memcpy(&wire, p, sizeof(wire));
        out.push_back(wire);
        p += sizeof(wire);
    }
    return true;
}

bool ClientMsgHandler::parseBagInfoRsp(const char* data,
                                       uint16_t len,
                                       std::vector<Msg_S2C_BagSlotWire>& out,
                                       std::string& errMsg)
{
    out.clear();
    errMsg.clear();

    if (!data || len < sizeof(Msg_S2C_BagInfoRspHeader))
    {
        errMsg = u8"背包同步响应过短";
        return false;
    }

    Msg_S2C_BagInfoRspHeader hdr{};
    if (!parseMsgHeader(data, len, hdr))
    {
        errMsg = u8"背包同步响应格式错误";
        return false;
    }

    if (hdr.code != 0)
    {
        errMsg = u8"背包同步失败";
        return false;
    }

    if (len != bagInfoBodyLen(hdr.slotCount))
    {
        errMsg = u8"背包同步数据不完整";
        return false;
    }

    const char* p = data + sizeof(Msg_S2C_BagInfoRspHeader);
    for (uint16_t i = 0; i < hdr.slotCount; ++i)
    {
        Msg_S2C_BagSlotWire wire{};
        std::memcpy(&wire, p, sizeof(wire));
        out.push_back(wire);
        p += sizeof(wire);
    }
    return true;
}
