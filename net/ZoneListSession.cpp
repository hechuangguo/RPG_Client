/**
 * @file    ZoneListSession.cpp
 * @brief   ZoneListSession 实现
 */

#include "net/ZoneListSession.h"

#include "log/ClientLogger.h"
#include "net/ClientErrorText.h"
#include "net/ClientMsgHandler.h"
#include "net/ClientTimingDefaults.h"
#include "net/TcpClient.h"
#include "util/ConfigLoader.h"
#include "time/TimeUtil.h"

namespace
{
constexpr uint8_t kLoginModule  = static_cast<uint8_t>(ClientModule::LOGIN);
constexpr uint8_t kSystemModule = static_cast<uint8_t>(ClientModule::SYSTEM);
}  // namespace

ZoneListSession::ZoneListSession()
    : m_config(nullptr)
    , m_tcp(std::make_unique<TcpClient>())
    , m_state(State::Idle)
    , m_gameType(0)
    , m_connectStartMs(0)
    , m_waitResponseStartMs(0)
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

void ZoneListSession::setOnStatus(std::function<void(const std::string&)> cb)
{
    m_onStatus = std::move(cb);
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

    m_gameType            = gameType;
    m_state               = State::Connecting;
    m_connectStartMs      = TimeUtil::nowMs();
    m_waitResponseStartMs = 0;

    const std::string host = loginHost();
    const uint16_t    port = loginPort();
    ClientLogger::instance().info("ZoneListSession：连接 %s:%u", host.c_str(), port);
    notifyStatus(u8"正在连接 LoginServer...");

    if (!m_tcp->connect(host, port))
    {
        fail(ClientLocalError::ConnectFailed, LoginTimeoutContext::LoginServerConnect);
        return;
    }
}

void ZoneListSession::update()
{
    if (m_state == State::Idle || !m_tcp)
    {
        return;
    }

    const int64_t now = TimeUtil::nowMs();
    if (m_state == State::Connecting &&
        TimeUtil::elapsed(m_connectStartMs, connectTimeoutMs(), now))
    {
        fail(ClientLocalError::ConnectTimeout, LoginTimeoutContext::LoginServerConnect);
        return;
    }
    if (m_state == State::WaitResponse && m_waitResponseStartMs > 0 &&
        TimeUtil::elapsed(m_waitResponseStartMs, zoneListResponseTimeoutMs(), now))
    {
        fail(ClientLocalError::ResponseTimeout, LoginTimeoutContext::ZoneList);
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
    resetToIdle();
    if (m_tcp)
    {
        m_tcp->disconnect();
    }
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

void ZoneListSession::fail(ClientLocalError err, LoginTimeoutContext ctx)
{
    fail(ClientErrorText::localErrorText(err, ctx));
}

int64_t ZoneListSession::connectTimeoutMs() const
{
    return m_config ? m_config->connectTimeoutMs() : ClientTiming::kConnectTimeoutMs;
}

int64_t ZoneListSession::zoneListResponseTimeoutMs() const
{
    return m_config ? m_config->zoneListResponseTimeoutMs()
                    : ClientTiming::kZoneListResponseTimeoutMs;
}

void ZoneListSession::onTcpConnected()
{
    if (m_state == State::Connecting)
    {
        m_waitResponseStartMs = TimeUtil::nowMs();
        notifyStatus(u8"正在获取区列表...");
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

    if (m_state == State::Connecting)
    {
        fail(ClientLocalError::Disconnected, LoginTimeoutContext::LoginServerConnect);
        return;
    }

    fail(ClientLocalError::Disconnected, LoginTimeoutContext::ZoneList);
}

void ZoneListSession::notifyStatus(const std::string& message)
{
    if (m_onStatus)
    {
        m_onStatus(message);
    }
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
    const uint16_t flatId = makeMsgId(module, sub);

    if (module == kSystemModule)
    {
        if (flatId == clientMsgFlatId<Msg_S2C_Error>())
        {
            Msg_S2C_Error err{};
            if (ClientMsgHandler::parseGatewayError(data, len, err))
            {
                fail(ClientErrorText::gatewayValidateText(err));
            }
            else
            {
                fail(u8"网关错误消息解析失败");
            }
            return;
        }
        if (sub == static_cast<uint8_t>(SystemMsgSub::S2C_KICK))
        {
            fail(u8"已被服务器踢下线");
        }
        return;
    }

    if (module != kLoginModule)
    {
        return;
    }

    if (flatId != clientMsgFlatId<Msg_S2C_ZoneListRspHeader>())
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
