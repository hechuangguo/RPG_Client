/**
 * @file    ZoneListSession.cpp
 * @brief   ZoneListSession 实现
 */

#include "net/ZoneListSession.h"

#include "log/ClientLogger.h"
#include "net/ClientMsgHandler.h"
#include "net/TcpClient.h"
#include "util/ConfigLoader.h"

namespace
{
constexpr uint8_t kLoginModule = static_cast<uint8_t>(ClientModule::LOGIN);
}  // namespace

ZoneListSession::ZoneListSession()
    : m_config(nullptr)
    , m_tcp(std::make_unique<TcpClient>())
    , m_state(State::Idle)
    , m_gameType(0)
{
    m_tcp->setOnMessage([this](uint8_t module, uint8_t sub, const char* data, uint16_t len) {
        onTcpMessage(module, sub, data, len);
    });
    m_tcp->setOnConnected([this]() { onTcpConnected(); });
    m_tcp->setOnDisconnected([this]() { onTcpDisconnected(); });
}

ZoneListSession::~ZoneListSession() = default;

void ZoneListSession::setConfig(const ConfigLoader* config)
{
    m_config = config;
}

void ZoneListSession::setOnSuccess(SuccessCallback cb)
{
    m_onSuccess = std::move(cb);
}

void ZoneListSession::setOnError(ErrorCallback cb)
{
    m_onError = std::move(cb);
}

void ZoneListSession::fetchZoneList(uint8_t gameType)
{
    cancel();
    if (!m_tcp)
    {
        m_tcp = std::make_unique<TcpClient>();
        m_tcp->setOnMessage([this](uint8_t module, uint8_t sub, const char* data, uint16_t len) {
            onTcpMessage(module, sub, data, len);
        });
        m_tcp->setOnConnected([this]() { onTcpConnected(); });
        m_tcp->setOnDisconnected([this]() { onTcpDisconnected(); });
    }

    m_gameType = gameType;
    m_state    = State::Connecting;

    ClientLogger::instance().info("ZoneListSession：连接 %s:%u",
                                  loginHost().c_str(), loginPort());
    m_tcp->connect(loginHost(), loginPort());
}

void ZoneListSession::update()
{
    if (m_state == State::Idle || !m_tcp)
    {
        return;
    }
    m_tcp->poll();
}

bool ZoneListSession::isBusy() const
{
    return m_state != State::Idle;
}

void ZoneListSession::cancel()
{
    if (m_tcp)
    {
        m_tcp->disconnect();
    }
    resetToIdle();
}

void ZoneListSession::resetToIdle()
{
    m_state = State::Idle;
}

void ZoneListSession::fail(const std::string& msg)
{
    ClientLogger::instance().err("ZoneListSession：%s", msg.c_str());
    if (m_tcp)
    {
        m_tcp->disconnect();
    }
    resetToIdle();
    if (m_onError)
    {
        m_onError(msg);
    }
}

void ZoneListSession::onTcpConnected()
{
    if (m_state == State::Connecting)
    {
        sendZoneListReq();
        m_state = State::WaitResponse;
    }
}

void ZoneListSession::onTcpDisconnected()
{
    if (m_state == State::Idle)
    {
        return;
    }
    fail(u8"连接已断开");
}

void ZoneListSession::sendZoneListReq()
{
    if (!m_tcp)
    {
        return;
    }
    const auto packet = ClientMsgHandler::buildZoneListReq(m_gameType);
    m_tcp->sendRaw(packet);
}

std::string ZoneListSession::loginHost() const
{
    if (m_config && !m_config->loginHost().empty())
    {
        return m_config->loginHost();
    }
    return "127.0.0.1";
}

uint16_t ZoneListSession::loginPort() const
{
    if (m_config && m_config->loginPort() != 0)
    {
        return m_config->loginPort();
    }
    return 9010;
}

void ZoneListSession::onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len)
{
    if (module != kLoginModule)
    {
        return;
    }

    const uint16_t flatId = makeMsgId(module, sub);
    if (flatId != static_cast<uint16_t>(ClientMsgID::S2C_ZONE_LIST_RSP))
    {
        return;
    }

    std::vector<GameZoneEntry> zones;
    std::string errMsg;
    if (!ClientMsgHandler::parseZoneListRsp(data, len, zones, errMsg))
    {
        fail(errMsg.empty() ? u8"区列表解析失败" : errMsg);
        return;
    }

    if (m_tcp)
    {
        m_tcp->disconnect();
    }
    resetToIdle();

    ClientLogger::instance().info("ZoneListSession：收到 %zu 个区服", zones.size());
    if (m_onSuccess)
    {
        m_onSuccess(zones);
    }
}
