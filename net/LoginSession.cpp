/**
 * @file    LoginSession.cpp
 * @brief   LoginSession 实现
 */

#include "net/LoginSession.h"

#include "net/ClientMsgHandler.h"
#include "log/ClientLogger.h"
#include "net/TcpClient.h"
#include "util/ConfigLoader.h"

#include <cstring>

namespace
{
constexpr uint8_t kLoginModule = static_cast<uint8_t>(ClientModule::LOGIN);

bool parseRegisterRsp(const char* data, uint16_t len, Msg_S2C_RegisterRsp& out)
{
    if (!data || len < sizeof(out))
    {
        return false;
    }
    std::memcpy(&out, data, sizeof(out));
    return true;
}
}  // namespace

LoginSession::LoginSession()
    : m_config(nullptr)
    , m_tcp(std::make_unique<TcpClient>())
    , m_state(State::Idle)
    , m_isRegisterFlow(false)
    , m_zoneId(0)
    , m_gameType(0)
    , m_gotLoginRsp(false)
    , m_gotGatewayInfo(false)
    , m_gatewayPort(0)
{
    m_tcp->setOnMessage([this](uint8_t module, uint8_t sub, const char* data, uint16_t len) {
        onTcpMessage(module, sub, data, len);
    });
    m_tcp->setOnConnected([this]() { onTcpConnected(); });
    m_tcp->setOnDisconnected([this]() { onTcpDisconnected(); });
}

LoginSession::~LoginSession() = default;

void LoginSession::setConfig(const ConfigLoader* config)
{
    m_config = config;
}

void LoginSession::setOnEnterGame(EnterGameCallback cb)
{
    m_onEnterGame = std::move(cb);
}

void LoginSession::setOnError(ErrorCallback cb)
{
    m_onError = std::move(cb);
}

void LoginSession::setOnRegisterSuccess(VoidCallback cb)
{
    m_onRegisterSuccess = std::move(cb);
}

void LoginSession::startLogin(const std::string& account,
                              const std::string& password,
                              uint32_t zoneId,
                              uint8_t gameType)
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
    m_isRegisterFlow = false;
    m_account        = account;
    m_password       = password;
    m_zoneId         = zoneId;
    m_gameType       = gameType;
    m_gotLoginRsp    = false;
    m_gotGatewayInfo = false;
    m_state          = State::ConnectLogin;

    ClientLogger::instance().info("LoginSession: connect LoginServer %s:%u zone=%u",
                                  loginHost().c_str(), loginPort(), zoneId);
    m_tcp->connect(loginHost(), loginPort());
}

void LoginSession::startRegister(const std::string& account,
                                 const std::string& password,
                                 uint32_t zoneId,
                                 uint8_t gameType)
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
    m_isRegisterFlow = true;
    m_account        = account;
    m_password       = password;
    m_zoneId         = zoneId;
    m_gameType       = gameType;
    m_state          = State::RegisterConnect;

    ClientLogger::instance().info("LoginSession: register connect %s:%u", loginHost().c_str(), loginPort());
    m_tcp->connect(loginHost(), loginPort());
}

void LoginSession::update()
{
    if (m_state == State::Idle || !m_tcp)
    {
        return;
    }
    m_tcp->poll();
}

bool LoginSession::isBusy() const
{
    return m_state != State::Idle;
}

void LoginSession::cancel()
{
    if (m_tcp)
    {
        m_tcp->disconnect();
    }
    resetToIdle();
}

std::unique_ptr<TcpClient> LoginSession::releaseTcpClient()
{
    return std::move(m_tcp);
}

const std::string& LoginSession::gatewayHost() const
{
    return m_gatewayHost;
}

uint16_t LoginSession::gatewayPort() const
{
    return m_gatewayPort;
}

void LoginSession::resetToIdle()
{
    m_state = State::Idle;
    m_gotLoginRsp = false;
    m_gotGatewayInfo = false;
}

void LoginSession::fail(const std::string& msg)
{
    ClientLogger::instance().err("LoginSession: %s", msg.c_str());
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

void LoginSession::onTcpConnected()
{
    if (m_state == State::ConnectLogin || m_state == State::RegisterConnect)
    {
        if (m_isRegisterFlow)
        {
            sendRegisterReq();
            m_state = State::RegisterWaitRsp;
        }
        else
        {
            sendLoginReq();
            m_state = State::WaitLoginRsp;
        }
    }
    else if (m_state == State::ConnectGateway)
    {
        sendLoginReq();
        m_state = State::WaitGatewayLoginRsp;
    }
}

void LoginSession::onTcpDisconnected()
{
    if (m_state == State::Idle || m_state == State::SwitchingGateway)
    {
        return;
    }
    fail(u8"连接已断开");
}

void LoginSession::sendLoginReq()
{
    if (!m_tcp)
    {
        return;
    }
    const auto packet = ClientMsgHandler::buildLoginReq(m_account, m_password, m_zoneId, m_gameType);
    m_tcp->sendRaw(packet);
}

void LoginSession::sendRegisterReq()
{
    if (!m_tcp)
    {
        return;
    }
    const auto packet = ClientMsgHandler::buildRegisterReq(m_account, m_password, m_zoneId, m_gameType);
    m_tcp->sendRaw(packet);
}

std::string LoginSession::loginHost() const
{
    if (m_config && !m_config->loginHost().empty())
    {
        return m_config->loginHost();
    }
    return "127.0.0.1";
}

uint16_t LoginSession::loginPort() const
{
    if (m_config && m_config->loginPort() != 0)
    {
        return m_config->loginPort();
    }
    return 9010;
}

void LoginSession::onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len)
{
    if (module != kLoginModule)
    {
        return;
    }

    const uint16_t flatId = makeMsgId(module, sub);

    if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_LOGIN_RSP))
    {
        Msg_S2C_LoginRsp rsp{};
        if (!ClientMsgHandler::parseLoginRsp(data, len, rsp))
        {
            fail("登录响应解析失败");
            return;
        }
        m_loginRsp    = rsp;
        m_gotLoginRsp = true;

        if (rsp.code != 0)
        {
            fail(std::string("登录失败: ") + rsp.msg);
            return;
        }

        if (m_state == State::WaitGatewayLoginRsp)
        {
            m_state = State::WaitEnterGame;
        }
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_GATEWAY_INFO))
    {
        Msg_S2C_GatewayInfo info{};
        if (!ClientMsgHandler::parseGatewayInfo(data, len, info))
        {
            fail("网关信息解析失败");
            return;
        }
        m_gatewayInfo    = info;
        m_gotGatewayInfo = true;

        if (info.code != 0)
        {
            fail(std::string("无可用网关: ") + info.msg);
            return;
        }

        m_gatewayHost = info.gatewayIP;
        m_gatewayPort = info.gatewayPort;

        if (m_state == State::WaitLoginRsp && m_gotLoginRsp)
        {
            m_state = State::SwitchingGateway;
            m_tcp->disconnect();
            m_state = State::ConnectGateway;
            ClientLogger::instance().info("LoginSession: connect Gateway %s:%u",
                                          m_gatewayHost.c_str(), m_gatewayPort);
            m_tcp->connect(m_gatewayHost, m_gatewayPort);
        }
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_ENTER_GAME))
    {
        Msg_S2C_EnterGame enter{};
        if (!ClientMsgHandler::parseEnterGame(data, len, enter))
        {
            fail("进入游戏消息解析失败");
            return;
        }

        ClientLogger::instance().info("LoginSession: enter game user=%llu map=%u",
                                      static_cast<unsigned long long>(enter.userID), enter.mapID);
        resetToIdle();
        if (m_onEnterGame)
        {
            m_onEnterGame(enter);
        }
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_REGISTER_RSP))
    {
        Msg_S2C_RegisterRsp rsp{};
        if (!parseRegisterRsp(data, len, rsp))
        {
            fail("注册响应解析失败");
            return;
        }

        m_tcp->disconnect();
        resetToIdle();

        if (rsp.code != 0)
        {
            fail(std::string("注册失败: ") + rsp.msg);
            return;
        }

        ClientLogger::instance().info("LoginSession: register success");
        if (m_onRegisterSuccess)
        {
            m_onRegisterSuccess();
        }
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_ERROR))
    {
        Msg_S2C_Error err{};
        if (data && len >= sizeof(err))
        {
            std::memcpy(&err, data, sizeof(err));
            fail(std::string("服务器错误: ") + err.msg);
        }
        else
        {
            fail("服务器返回错误");
        }
    }

    if (m_state == State::WaitLoginRsp && m_gotLoginRsp && m_gotGatewayInfo)
    {
        // Gateway connect triggered above when gateway info arrives
    }
    else if (m_state == State::WaitGatewayLoginRsp && m_gotLoginRsp)
    {
        m_state = State::WaitEnterGame;
    }
}
