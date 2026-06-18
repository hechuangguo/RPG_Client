/**
 * @file    LoginSession.cpp
 * @brief   LoginSession 实现
 */

#include "net/LoginSession.h"

#include "net/ClientMsgHandler.h"
#include "log/ClientLogger.h"
#include "net/TcpClient.h"
#include "util/ConfigLoader.h"

namespace
{
constexpr uint8_t kLoginModule  = static_cast<uint8_t>(ClientModule::LOGIN);
constexpr uint8_t kSystemModule = static_cast<uint8_t>(ClientModule::SYSTEM);

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
    , m_gotUserList(false)
    , m_userCreated(false)
    , m_gatewayConnected(false)
    , m_gatewayPort(0)
    , m_pendingSelectUserId(0)
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
    m_isRegisterFlow     = false;
    m_account            = account;
    m_password           = password;
    m_zoneId             = zoneId;
    m_gameType           = gameType;
    m_gotLoginRsp        = false;
    m_gotGatewayInfo     = false;
    m_gotUserList        = false;
    m_userCreated        = false;
    m_gatewayConnected   = false;
    m_pendingSelectUserId = 0;
    m_characters.clear();
    m_state              = State::ConnectLogin;

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

    m_pendingSelectUserId = userID;
    if (!m_gatewayConnected)
    {
        tryConnectGateway();
        return;
    }

    sendSelectUserReq(userID);
}

void LoginSession::createCharacter(const std::string& name, uint8_t vocation, uint8_t sex)
{
    if (m_state != State::WaitUserAction || !m_tcp || name.empty())
    {
        return;
    }
    if (m_gatewayConnected)
    {
        return;
    }

    m_pendingCreateName     = name;
    m_pendingCreateVocation = vocation;
    m_pendingCreateSex      = sex;

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

bool LoginSession::gatewayConnected() const
{
    return m_gatewayConnected;
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
    m_state               = State::Idle;
    m_gotLoginRsp         = false;
    m_gotGatewayInfo      = false;
    m_gotUserList         = false;
    m_userCreated         = false;
    m_gatewayConnected    = false;
    m_pendingSelectUserId = 0;
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

void LoginSession::tryConnectGateway()
{
    if (!m_tcp || !m_gotLoginRsp || !m_gotGatewayInfo)
    {
        return;
    }
    if (m_gatewayConnected || m_state == State::SwitchingGateway || m_state == State::ConnectGateway ||
        m_state == State::WaitEnterGame)
    {
        return;
    }

    bool canSwitch = false;
    if (!m_gotUserList)
    {
        canSwitch = true;
    }
    else if (!m_characters.empty())
    {
        canSwitch = true;
    }
    else if (m_userCreated)
    {
        canSwitch = true;
    }

    if (!canSwitch)
    {
        return;
    }

    m_state = State::SwitchingGateway;
    ClientLogger::instance().info("LoginSession：连接 Gateway %s:%u",
                                  m_gatewayHost.c_str(), m_gatewayPort);
    m_tcp->disconnect();
    m_state = State::ConnectGateway;
    m_tcp->connect(m_gatewayHost, m_gatewayPort);
}

void LoginSession::onGatewayAuthSent()
{
    if (m_gotUserList && m_pendingSelectUserId != 0)
    {
        sendSelectUserReq(m_pendingSelectUserId);
        return;
    }
    if (m_gotUserList && !m_characters.empty())
    {
        deliverUserList(m_loginRsp.userID);
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
        m_gatewayConnected = true;
        sendGatewayAuthOrLogin();
        m_state = State::WaitUserList;
        onGatewayAuthSent();
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
        ClientLogger::instance().warn("LoginSession：loginToken 为空，回退为账号密码登录 Gateway");
        sendLoginReq();
    }
}

void LoginSession::sendSelectUserReq(uint64_t userID)
{
    if (!m_tcp || userID == 0)
    {
        return;
    }

    const auto packet = ClientMsgHandler::buildSelectUserReq(userID, 0);
    m_tcp->sendRaw(packet);
    m_state = State::WaitEnterGame;
    ClientLogger::instance().info("LoginSession：选择角色 user=%llu",
                                  static_cast<unsigned long long>(userID));
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

void LoginSession::handleLoginRsp(const Msg_S2C_LoginRsp& rsp)
{
    m_loginRsp    = rsp;
    m_gotLoginRsp = true;

    if (rsp.code != 0)
    {
        fail(std::string(u8"登录失败：") + rsp.msg);
        return;
    }

    if (m_state == State::WaitUserList && m_gatewayConnected)
    {
        m_state = State::WaitEnterGame;
        ClientLogger::instance().info("LoginSession：旧版 Gateway 登录成功，等待进入游戏");
    }

    tryConnectGateway();
}

void LoginSession::handleUserList()
{
    m_gotUserList = true;
    if (m_characters.empty())
    {
        deliverUserList(m_loginRsp.userID);
        return;
    }

    if (!m_gatewayConnected)
    {
        tryConnectGateway();
    }
    else if (m_state == State::WaitUserList)
    {
        deliverUserList(m_loginRsp.userID);
    }
}

void LoginSession::handleCreateUserRsp(const Msg_S2C_CreateUserRsp& rsp)
{
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
    m_userCreated = true;
    deliverUserList(rsp.userID);
}

void LoginSession::handleGatewayInfo(const Msg_S2C_GatewayInfo& info)
{
    m_gatewayInfo    = info;
    m_gotGatewayInfo = true;

    if (info.code != 0)
    {
        fail(std::string(u8"无可用网关：") + info.msg);
        return;
    }

    m_gatewayHost = info.gatewayIP;
    m_gatewayPort = info.gatewayPort;
    tryConnectGateway();
}

void LoginSession::handleEnterGame(const char* data, uint16_t len)
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

void LoginSession::handleSystemError(const char* data, uint16_t len)
{
    Msg_S2C_Error err{};
    if (!ClientMsgHandler::parseGatewayError(data, len, err))
    {
        fail(u8"网关错误消息解析失败");
        return;
    }
    fail(ClientMsgHandler::gatewayErrorText(err));
}

void LoginSession::onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len)
{
    const uint16_t flatId = makeMsgId(module, sub);

    if (module == kSystemModule)
    {
        if (flatId == clientMsgFlatId<Msg_S2C_Error>())
        {
            handleSystemError(data, len);
        }
        return;
    }

    if (module != kLoginModule)
    {
        return;
    }

    if (flatId == clientMsgFlatId<Msg_S2C_LoginRsp>())
    {
        Msg_S2C_LoginRsp rsp{};
        if (!ClientMsgHandler::parseLoginRsp(data, len, rsp))
        {
            fail(u8"登录响应解析失败");
            return;
        }
        handleLoginRsp(rsp);
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_GatewayInfo>())
    {
        Msg_S2C_GatewayInfo info{};
        if (!ClientMsgHandler::parseGatewayInfo(data, len, info))
        {
            fail(u8"网关信息解析失败");
            return;
        }
        handleGatewayInfo(info);
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_UserListHeader>())
    {
        std::string errMsg;
        if (!ClientMsgHandler::parseUserList(data, len, m_characters, errMsg))
        {
            fail(errMsg);
            return;
        }
        handleUserList();
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_CreateUserRsp>())
    {
        Msg_S2C_CreateUserRsp rsp{};
        if (!ClientMsgHandler::parseCreateUserRsp(data, len, rsp))
        {
            fail(u8"创建角色响应解析失败");
            return;
        }
        handleCreateUserRsp(rsp);
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_EnterGame>())
    {
        handleEnterGame(data, len);
    }
    else if (flatId == clientMsgFlatId<Msg_S2C_RegisterRsp>())
    {
        Msg_S2C_RegisterRsp rsp{};
        if (!ClientMsgHandler::parseRegisterRsp(data, len, rsp))
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
}
