/**
 * @file    GameSession.cpp
 * @brief   GameSession 实现
 */

#include "net/GameSession.h"

#include "EquipCommon.h"
#include "PropertyCommon.h"
#include "net/ClientErrorText.h"
#include "net/ClientLocalError.h"
#include "net/ClientMsgHandler.h"
#include "net/ClientTimingDefaults.h"
#include "lua/ClientScriptHost.h"
#include "log/ClientLogger.h"
#include "net/TcpClient.h"
#include "time/TimeUtil.h"
#include "util/ConfigLoader.h"

namespace
{
constexpr uint8_t kLoginModule  = static_cast<uint8_t>(ClientModule::LOGIN);
constexpr uint8_t kSystemModule = static_cast<uint8_t>(ClientModule::SYSTEM);
constexpr uint8_t kChatModule   = static_cast<uint8_t>(ClientModule::CHAT);

const char* logoutActionLabel(LogoutAction action)
{
    switch (action)
    {
    case LogoutAction::RETURN_CHAR_SELECT:
        return "返回选角";
    case LogoutAction::RETURN_LOGIN:
        return "返回登录";
    default:
        return "未知";
    }
}
}  // namespace

GameSession::GameSession()
    : m_config(nullptr)
    , m_tcp(nullptr)
    , m_scriptHost(nullptr)
    , m_localUserId(0)
    , m_lastHeartbeatMs(0)
    , m_lastMoveSendMs(0)
    , m_heartbeatSeq(0)
    , m_inWorld(false)
    , m_moveDirty(false)
    , m_pendingX(0)
    , m_pendingY(0)
    , m_pendingZ(0)
    , m_pendingDir(0)
    , m_pendingMoveType(0)
    , m_waitingLogoutRsp(false)
    , m_pendingLogoutAction(LogoutAction::RETURN_CHAR_SELECT)
    , m_logoutWaitStartMs(0)
    , m_serverTimeMs(0)
{
}

GameSession::~GameSession() = default;

void GameSession::setConfig(const ConfigLoader* config)
{
    m_config = config;
}

int64_t GameSession::heartbeatIntervalMs() const
{
    return m_config ? m_config->heartbeatIntervalMs() : ClientTiming::kHeartbeatIntervalMs;
}

int64_t GameSession::moveSendIntervalMs() const
{
    return m_config ? m_config->moveSendIntervalMs() : ClientTiming::kMoveSendIntervalMs;
}

int64_t GameSession::logoutTimeoutMs() const
{
    return m_config ? m_config->logoutTimeoutMs() : ClientTiming::kLogoutTimeoutMs;
}

void GameSession::setScriptHost(ClientScriptHost* host)
{
    m_scriptHost = host;
}

void GameSession::setOnSpawn(SpawnCallback cb)
{
    m_onSpawn = std::move(cb);
}

void GameSession::setOnMove(MoveCallback cb)
{
    m_onMove = std::move(cb);
}

void GameSession::setOnDespawn(DespawnCallback cb)
{
    m_onDespawn = std::move(cb);
}

void GameSession::setOnError(ErrorCallback cb)
{
    m_onError = std::move(cb);
}

void GameSession::setOnDisconnected(std::function<void()> cb)
{
    m_onDisconnected = std::move(cb);
}

void GameSession::bindTcpCallbacks()
{
    if (!m_tcp)
    {
        return;
    }

    m_tcp->setOnMessage([this](uint8_t module, uint8_t sub, const char* data, uint16_t len) {
        onTcpMessage(module, sub, data, len);
    });
    m_tcp->setOnDisconnected([this]() {
        ClientLogger::instance().warn("GameSession：连接已断开");
        clearLogoutWait();
        if (m_onDisconnected)
        {
            m_onDisconnected();
        }
    });
}

void GameSession::start(std::unique_ptr<TcpClient> tcp, const Msg_S2C_EnterGame& enter)
{
    disconnect();
    m_tcp             = std::move(tcp);
    m_enterInfo       = enter;
    m_localUserId     = enter.userID;
    m_lastHeartbeatMs = TimeUtil::nowMs();
    m_lastMoveSendMs  = m_lastHeartbeatMs;
    m_inWorld         = true;
    bindTcpCallbacks();

    ClientLogger::instance().info("GameSession：启动完成 角色=%llu 地图=%u",
                                  static_cast<unsigned long long>(m_localUserId), enter.mapID);
}

void GameSession::update(float /*dt*/)
{
    if (!m_tcp)
    {
        return;
    }

    m_tcp->poll();

    const int64_t now = TimeUtil::nowMs();
    if (m_waitingLogoutRsp && m_logoutWaitStartMs > 0 &&
        TimeUtil::elapsed(m_logoutWaitStartMs, logoutTimeoutMs(), now))
    {
        const LogoutAction action = m_pendingLogoutAction;
        ClientLogger::instance().warn("GameSession：离世界响应超时 意图=%s",
                                      logoutActionLabel(action));
        ErrorCallback errCb = std::move(m_onLogoutError);
        clearLogoutWait();
        if (errCb)
        {
            errCb(ClientErrorText::localErrorText(ClientLocalError::ResponseTimeout,
                                                  LoginTimeoutContext::Logout));
        }
        return;
    }

    if (!m_inWorld)
    {
        return;
    }

    if (TimeUtil::elapsed(m_lastHeartbeatMs, heartbeatIntervalMs(), now))
    {
        sendHeartbeat();
        m_lastHeartbeatMs = now;
    }

    flushMoveIfNeeded();
}

void GameSession::sendMove(uint64_t userID, float x, float y, float z, float dir, uint8_t moveType)
{
    if (!m_inWorld)
    {
        return;
    }

    m_moveDirty       = true;
    m_localUserId     = userID;
    m_pendingX        = x;
    m_pendingY        = y;
    m_pendingZ        = z;
    m_pendingDir      = dir;
    m_pendingMoveType = moveType;
}

void GameSession::requestLogout(LogoutAction action, LogoutCallback onSuccess, ErrorCallback onError)
{
    if (!m_tcp || !m_tcp->isConnected())
    {
        if (onError)
        {
            onError(u8"未连接游戏服务器");
        }
        return;
    }

    m_inWorld              = false;
    m_moveDirty            = false;
    m_waitingLogoutRsp     = true;
    m_pendingLogoutAction  = action;
    m_logoutWaitStartMs    = TimeUtil::nowMs();
    m_onLogoutSuccess      = std::move(onSuccess);
    m_onLogoutError        = std::move(onError);

    const auto packet = ClientMsgHandler::buildLogoutReq(action);
    m_tcp->sendRaw(packet);
    ClientLogger::instance().info("GameSession：请求离世界 意图=%s 角色=%llu",
                                  logoutActionLabel(action),
                                  static_cast<unsigned long long>(m_localUserId));
}

void GameSession::leaveWorld()
{
    m_inWorld   = false;
    m_moveDirty = false;
}

void GameSession::disconnect()
{
    const bool hadConnection = m_tcp != nullptr;
    clearLogoutWait();
    m_inWorld = false;
    if (m_tcp)
    {
        m_tcp->disconnect();
        m_tcp.reset();
    }
    m_moveDirty = false;
    if (hadConnection)
    {
        ClientLogger::instance().info("GameSession：断开连接");
    }
}

std::unique_ptr<TcpClient> GameSession::releaseTcpClient()
{
    clearLogoutWait();
    m_inWorld   = false;
    m_moveDirty = false;
    m_scriptHost = nullptr;

    if (m_tcp)
    {
        m_tcp->setOnMessage(nullptr);
        m_tcp->setOnDisconnected(nullptr);
    }
    ClientLogger::instance().info("GameSession：移交 Gateway 连接 角色=%llu",
                                  static_cast<unsigned long long>(m_localUserId));
    return std::move(m_tcp);
}

bool GameSession::isConnected() const
{
    return m_tcp && m_tcp->isConnected();
}

bool GameSession::isWaitingLogout() const
{
    return m_waitingLogoutRsp;
}

uint64_t GameSession::localUserId() const
{
    return m_localUserId;
}

const Msg_S2C_EnterGame& GameSession::enterGameInfo() const
{
    return m_enterInfo;
}

uint64_t GameSession::serverTimeMs() const
{
    return m_serverTimeMs;
}

void GameSession::sendRaw(const std::vector<char>& packet)
{
    if (m_tcp && m_tcp->isConnected())
    {
        m_tcp->sendRaw(packet);
    }
}

void GameSession::sendChat(uint8_t channel, const std::string& content)
{
    if (!m_inWorld || !m_tcp || !m_tcp->isConnected())
    {
        return;
    }
    m_tcp->sendRaw(ClientMsgHandler::buildChatReq(channel, content));
}

void GameSession::sendQuestAccept(uint32_t questId)
{
    if (!m_inWorld || !m_tcp || !m_tcp->isConnected())
    {
        return;
    }
    m_tcp->sendRaw(ClientMsgHandler::buildQuestAcceptReq(questId));
}

void GameSession::sendQuestSubmit(uint32_t questId)
{
    if (!m_inWorld || !m_tcp || !m_tcp->isConnected())
    {
        return;
    }
    m_tcp->sendRaw(ClientMsgHandler::buildQuestSubmitReq(questId));
}

void GameSession::requestBagInfo()
{
    if (!m_inWorld || !m_tcp || !m_tcp->isConnected())
    {
        return;
    }
    m_tcp->sendRaw(ClientMsgHandler::buildBagInfoReq(m_localUserId));
}

void GameSession::clearLogoutWait()
{
    m_waitingLogoutRsp  = false;
    m_logoutWaitStartMs = 0;
    m_onLogoutSuccess   = nullptr;
    m_onLogoutError     = nullptr;
}

void GameSession::sendHeartbeat()
{
    if (!m_tcp || !m_tcp->isConnected() || !m_inWorld)
    {
        return;
    }
    ++m_heartbeatSeq;
    const auto packet = ClientMsgHandler::buildHeartbeat(m_heartbeatSeq);
    m_tcp->sendRaw(packet);
}

void GameSession::flushMoveIfNeeded()
{
    if (!m_moveDirty || !m_tcp || !m_tcp->isConnected() || !m_inWorld)
    {
        return;
    }

    const int64_t now = TimeUtil::nowMs();
    if (!TimeUtil::elapsed(m_lastMoveSendMs, moveSendIntervalMs(), now))
    {
        return;
    }

    const auto packet = ClientMsgHandler::buildMoveReq(
        m_localUserId, m_pendingX, m_pendingY, m_pendingZ, m_pendingDir, m_pendingMoveType);
    m_tcp->sendRaw(packet);
    m_lastMoveSendMs = now;
    m_moveDirty      = false;
}

void GameSession::onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len)
{
    const uint16_t flatId = makeMsgId(module, sub);

    if (module == kSystemModule)
    {
        if (flatId == clientMsgFlatId<Msg_S2C_Error>())
        {
            Msg_S2C_Error err{};
            if (ClientMsgHandler::parseGatewayError(data, len, err) && m_onError)
            {
                m_onError(ClientErrorText::gatewayValidateText(err));
            }
            return;
        }
        if (sub == static_cast<uint8_t>(SystemMsgSub::S2C_KICK))
        {
            ClientLogger::instance().warn("GameSession：收到踢线通知");
            if (m_onError)
            {
                m_onError(u8"已被服务器踢下线");
            }
            disconnect();
            return;
        }
        if (flatId == clientMsgFlatId<Msg_S2C_Heartbeat>())
        {
            Msg_S2C_Heartbeat hb{};
            if (ClientMsgHandler::parseHeartbeat(data, len, hb))
            {
                m_serverTimeMs = hb.serverTime;
            }
            return;
        }
        if (flatId == clientMsgFlatId<Msg_S2C_Notice>())
        {
            Msg_S2C_Notice notice{};
            if (ClientMsgHandler::parseNotice(data, len, notice) && m_scriptHost)
            {
                m_scriptHost->onNotice(notice.content);
            }
            return;
        }
    }

    if (module == kLoginModule && flatId == clientMsgFlatId<Msg_S2C_LogoutRsp>())
    {
        Msg_S2C_LogoutRsp rsp{};
        if (!ClientMsgHandler::parseLogoutRsp(data, len, rsp))
        {
            ClientLogger::instance().warn("GameSession：离世界响应解析失败");
            if (m_onLogoutError)
            {
                m_onLogoutError(u8"离世界响应解析失败");
            }
            clearLogoutWait();
            return;
        }

        LogoutCallback successCb = std::move(m_onLogoutSuccess);
        ErrorCallback errCb      = std::move(m_onLogoutError);
        const LogoutAction action = m_pendingLogoutAction;
        clearLogoutWait();

        if (static_cast<LogoutResultCode>(rsp.code) != LogoutResultCode::OK)
        {
            ClientLogger::instance().warn("GameSession：离世界失败 意图=%s 错误码=%d",
                                          logoutActionLabel(action), rsp.code);
            if (errCb)
            {
                errCb(ClientErrorText::logoutResultText(static_cast<LogoutResultCode>(rsp.code),
                                                        rsp.msg));
            }
            return;
        }

        ClientLogger::instance().info("GameSession：离世界成功 意图=%s",
                                      logoutActionLabel(action));
        if (successCb)
        {
            successCb(action);
        }
        return;
    }

    if (!m_inWorld)
    {
        return;
    }

    if (flatId == clientMsgFlatId<Msg_S2C_SpawnEntity>())
    {
        Msg_S2C_SpawnEntity spawn{};
        if (ClientMsgHandler::parseSpawnEntity(data, len, spawn) && m_onSpawn)
        {
            m_onSpawn(spawn);
        }
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_MoveNotify>())
    {
        Msg_S2C_MoveNotify move{};
        if (ClientMsgHandler::parseMoveNotify(data, len, move) && m_onMove)
        {
            m_onMove(move);
        }
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_DespawnEntity>())
    {
        Msg_S2C_DespawnEntity despawn{};
        if (ClientMsgHandler::parseDespawnEntity(data, len, despawn) && m_onDespawn)
        {
            m_onDespawn(despawn);
        }
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_Heartbeat>())
    {
        Msg_S2C_Heartbeat hb{};
        if (ClientMsgHandler::parseHeartbeat(data, len, hb))
        {
            m_serverTimeMs = hb.serverTime;
        }
    }
    else if (module == kChatModule && flatId == clientMsgFlatId<Msg_S2C_Chat>())
    {
        Msg_S2C_Chat chat{};
        if (ClientMsgHandler::parseChatNotify(data, len, chat) && m_scriptHost)
        {
            m_scriptHost->onChat(chat);
        }
    }
    else if (module == kChatModule &&
             (sub == static_cast<uint8_t>(ChatMsgSub::C2S_WHISPER_REQ) ||
              sub == static_cast<uint8_t>(ChatMsgSub::S2C_WHISPER_NOTIFY)))
    {
        ClientLogger::instance().info("GameSession：私聊消息暂未实现 sub=%u", sub);
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_Error>())
    {
        Msg_S2C_Error err{};
        if (ClientMsgHandler::parseGatewayError(data, len, err) && m_onError)
        {
            m_onError(ClientErrorText::gatewayValidateText(err));
        }
    }
    else if (flatId == makeMsgId(static_cast<uint8_t>(ClientModule::QUEST),
                                 static_cast<uint8_t>(QuestMsgSub::S2C_QUEST_INFO)))
    {
        if (m_scriptHost)
        {
            m_scriptHost->onQuestInfo(data, len);
        }
    }
    else if (flatId == makeMsgId(static_cast<uint8_t>(ClientModule::BAG),
                                 static_cast<uint8_t>(EquipMsgSub::S2C_BAG_INFO_RSP)))
    {
        if (m_scriptHost)
        {
            m_scriptHost->onBagInfo(data, len);
        }
    }
}
