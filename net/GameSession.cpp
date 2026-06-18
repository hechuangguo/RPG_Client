/**
 * @file    GameSession.cpp
 * @brief   GameSession 实现
 */

#include "net/GameSession.h"

#include "EquipCommon.h"
#include "PropertyCommon.h"
#include "net/ClientMsgHandler.h"
#include "lua/ClientScriptHost.h"
#include "log/ClientLogger.h"
#include "net/TcpClient.h"
#include "time/TimeUtil.h"

namespace
{
constexpr int64_t kHeartbeatIntervalMs = 10000;
constexpr int64_t kMoveSendIntervalMs  = 100;
}  // namespace

GameSession::GameSession()
    : m_tcp(nullptr)
    , m_scriptHost(nullptr)
    , m_localUserId(0)
    , m_lastHeartbeatMs(0)
    , m_lastMoveSendMs(0)
    , m_heartbeatSeq(0)
    , m_moveDirty(false)
    , m_pendingX(0)
    , m_pendingY(0)
    , m_pendingZ(0)
    , m_pendingDir(0)
    , m_pendingMoveType(0)
{
}

GameSession::~GameSession() = default;

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

void GameSession::start(std::unique_ptr<TcpClient> tcp, const Msg_S2C_EnterGame& enter)
{
    disconnect();
    m_tcp = std::move(tcp);
    m_enterInfo    = enter;
    m_localUserId  = enter.userID;
    m_lastHeartbeatMs = TimeUtil::nowMs();
    m_lastMoveSendMs  = m_lastHeartbeatMs;

    m_tcp->setOnMessage([this](uint8_t module, uint8_t sub, const char* data, uint16_t len) {
        onTcpMessage(module, sub, data, len);
    });
    m_tcp->setOnDisconnected([this]() {
        ClientLogger::instance().warn("GameSession：连接已断开");
        if (m_onDisconnected)
        {
            m_onDisconnected();
        }
    });

    ClientLogger::instance().info("GameSession：启动完成 user=%llu map=%u",
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
    if (TimeUtil::elapsed(m_lastHeartbeatMs, kHeartbeatIntervalMs, now))
    {
        sendHeartbeat();
        m_lastHeartbeatMs = now;
    }

    flushMoveIfNeeded();
}

void GameSession::sendMove(uint64_t userID, float x, float y, float z, float dir, uint8_t moveType)
{
    m_moveDirty         = true;
    m_localUserId       = userID;
    m_pendingX          = x;
    m_pendingY          = y;
    m_pendingZ          = z;
    m_pendingDir        = dir;
    m_pendingMoveType   = moveType;
}

void GameSession::disconnect()
{
    if (m_tcp)
    {
        m_tcp->disconnect();
        m_tcp.reset();
    }
    m_moveDirty = false;
}

bool GameSession::isConnected() const
{
    return m_tcp && m_tcp->isConnected();
}

uint64_t GameSession::localUserId() const
{
    return m_localUserId;
}

const Msg_S2C_EnterGame& GameSession::enterGameInfo() const
{
    return m_enterInfo;
}

void GameSession::sendHeartbeat()
{
    if (!m_tcp || !m_tcp->isConnected())
    {
        return;
    }
    ++m_heartbeatSeq;
    const auto packet = ClientMsgHandler::buildHeartbeat(m_heartbeatSeq);
    m_tcp->sendRaw(packet);
}

void GameSession::flushMoveIfNeeded()
{
    if (!m_moveDirty || !m_tcp || !m_tcp->isConnected())
    {
        return;
    }

    const int64_t now = TimeUtil::nowMs();
    if (!TimeUtil::elapsed(m_lastMoveSendMs, kMoveSendIntervalMs, now))
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
        // echo ack, no action
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_Error>())
    {
        Msg_S2C_Error err{};
        if (ClientMsgHandler::parseGatewayError(data, len, err) && m_onError)
        {
            m_onError(ClientMsgHandler::gatewayErrorText(err));
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
