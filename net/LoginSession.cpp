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

bool hasLoginToken(const Msg_S2C_LoginRsp& rsp)
{
    return rsp.loginToken[0] != '\0';
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
    , m_pendingCreateVocation(0)
    , m_pendingCreateSex(0)
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

void LoginSession::setOnUserList(UserListCallback cb)
{
    m_onUserList = std::move(cb);
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
    m_characters.clear();
    m_state          = State::ConnectLogin;

    ClientLogger::instance().info("LoginSession：连接 LoginServer %s:%u，区服=%u",
                                  loginHost().c_str(), loginPort(), zoneId);
    m_tcp->connect(loginHost(), loginPort());
}

void LoginSession::startRegister(const std::string& account,
                                 const std::string& password,
                                 const std::string& confirmPassword,
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
    m_account         = account;
    m_password        = password;
    m_confirmPassword = confirmPassword;
    m_zoneId          = zoneId;
    m_gameType        = gameType;
    m_state           = State::RegisterConnect;

    ClientLogger::instance().info("LoginSession：注册流程连接 %s:%u", loginHost().c_str(), loginPort());
    m_tcp->connect(loginHost(), loginPort());
}

void LoginSession::selectCharacter(uint64_t userID)
{
    if (m_state != State::WaitUserAction || !m_tcp || userID == 0)
    {
        return;
    }

    const auto packet = ClientMsgHandler::buildSelectUserReq(userID, 0);
    m_tcp->sendRaw(packet);
    m_state = State::WaitEnterGame;
    ClientLogger::instance().info("LoginSession：选择角色 user=%llu",
                                  static_cast<unsigned long long>(userID));
}

void LoginSession::createCharacter(const std::string& name, uint8_t vocation, uint8_t sex)
{
    if (m_state != State::WaitUserAction || !m_tcp || name.empty())
    {
        return;
    }

    m_pendingCreateName      = name;
    m_pendingCreateVocation  = vocation;
    m_pendingCreateSex       = sex;

    const auto packet = ClientMsgHandler::buildCreateUserReq(name, vocation, sex);
    m_tcp->sendRaw(packet);
    m_state = State::WaitCreateUserRsp;
    ClientLogger::instance().info("LoginSession：创建角色 name=%s", name.c_str());
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
    m_state          = State::Idle;
    m_gotLoginRsp    = false;
    m_gotGatewayInfo = false;
    m_characters.clear();
    m_pendingCreateName.clear();
}

void LoginSession::fail(const std::string& msg)
{
    ClientLogger::instance().err("LoginSession：%s", msg.c_str());
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

void LoginSession::deliverUserList(uint64_t highlightUserId)
{
    m_state = State::WaitUserAction;
    ClientLogger::instance().info("LoginSession：收到 %zu 个角色", m_characters.size());
    if (m_onUserList)
    {
        m_onUserList(m_characters, highlightUserId);
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
        sendGatewayAuthOrLogin();
        m_state = State::WaitUserList;
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
    const auto packet = ClientMsgHandler::buildRegisterReq(
        m_account, m_password, m_confirmPassword, m_zoneId, m_gameType);
    m_tcp->sendRaw(packet);
}

void LoginSession::sendGatewayAuthOrLogin()
{
    if (!m_tcp)
    {
        return;
    }

    if (hasLoginToken(m_loginRsp))
    {
        const auto packet = ClientMsgHandler::buildGatewayAuthReq(
            m_account, m_loginRsp.loginToken, m_zoneId, m_gameType);
        m_tcp->sendRaw(packet);
        ClientLogger::instance().info("LoginSession：发送 Gateway 票据鉴权");
    }
    else
    {
        sendLoginReq();
        ClientLogger::instance().info("LoginSession：票据为空，回退账号密码登录 Gateway");
    }
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
            fail(u8"登录响应解析失败");
            return;
        }
        m_loginRsp    = rsp;
        m_gotLoginRsp = true;

        if (rsp.code != 0)
        {
            fail(std::string(u8"登录失败：") + rsp.msg);
            return;
        }

        if (m_state == State::WaitUserList)
        {
            m_state = State::WaitEnterGame;
            ClientLogger::instance().info("LoginSession：旧版 Gateway 登录成功，等待进入游戏");
        }
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_GATEWAY_INFO))
    {
        Msg_S2C_GatewayInfo info{};
        if (!ClientMsgHandler::parseGatewayInfo(data, len, info))
        {
            fail(u8"网关信息解析失败");
            return;
        }
        m_gatewayInfo    = info;
        m_gotGatewayInfo = true;

        if (info.code != 0)
        {
            fail(std::string(u8"无可用网关：") + info.msg);
            return;
        }

        m_gatewayHost = info.gatewayIP;
        m_gatewayPort = info.gatewayPort;

        if (m_state == State::WaitLoginRsp && m_gotLoginRsp)
        {
            m_state = State::SwitchingGateway;
            m_tcp->disconnect();
            m_state = State::ConnectGateway;
            ClientLogger::instance().info("LoginSession：连接 Gateway %s:%u",
                                          m_gatewayHost.c_str(), m_gatewayPort);
            m_tcp->connect(m_gatewayHost, m_gatewayPort);
        }
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_USER_LIST))
    {
        std::string errMsg;
        if (!ClientMsgHandler::parseUserList(data, len, m_characters, errMsg))
        {
            fail(errMsg);
            return;
        }
        deliverUserList(m_loginRsp.userID);
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_CREATE_USER_RSP))
    {
        Msg_S2C_CreateUserRsp rsp{};
        if (!ClientMsgHandler::parseCreateUserRsp(data, len, rsp))
        {
            fail(u8"创建角色响应解析失败");
            return;
        }

        if (rsp.code != 0)
        {
            m_state = State::WaitUserAction;
            ClientLogger::instance().err("LoginSession：创建角色失败：%s", rsp.msg);
            if (m_onError)
            {
                m_onError(std::string(u8"创建角色失败：") + rsp.msg);
            }
            return;
        }

        CharacterEntry created{};
        created.userID   = rsp.userID;
        created.name     = m_pendingCreateName;
        created.level    = 1;
        created.vocation = m_pendingCreateVocation;
        created.sex      = m_pendingCreateSex;
        m_characters.push_back(created);
        deliverUserList(rsp.userID);
    }
    else if (flatId == static_cast<uint16_t>(ClientMsgID::S2C_ENTER_GAME))
    {
        Msg_S2C_EnterGame enter{};
        if (!ClientMsgHandler::parseEnterGame(data, len, enter))
        {
            fail(u8"进入游戏消息解析失败");
            return;
        }

        ClientLogger::instance().info("LoginSession：进入游戏 user=%llu map=%u",
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
            fail(u8"注册响应解析失败");
            return;
        }

        m_tcp->disconnect();
        resetToIdle();

        if (rsp.code != 0)
        {
            fail(std::string(u8"注册失败：") + rsp.msg);
            return;
        }

        ClientLogger::instance().info("LoginSession：注册成功");
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
            fail(std::string(u8"服务器错误：") + err.msg);
        }
        else
        {
            fail(u8"服务器返回错误");
        }
    }
}
