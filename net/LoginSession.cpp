/**
 * @file    LoginSession.cpp
 * @brief   LoginSession 实现
 */

#include "net/LoginSession.h"

#include "net/ClientErrorText.h"
#include "net/ClientMsgHandler.h"
#include "net/ClientTimingDefaults.h"
#include "log/ClientLogger.h"
#include "net/TcpClient.h"
#include "time/TimeUtil.h"
#include "util/ConfigLoader.h"

namespace
{
constexpr uint8_t kLoginModule  = static_cast<uint8_t>(ClientModule::LOGIN);
constexpr uint8_t kSystemModule = static_cast<uint8_t>(ClientModule::SYSTEM);

bool hasLoginToken(const Msg_S2C_LoginRsp& rsp)
{
    return rsp.loginToken[0] != '\0';
}

bool isLoopbackGatewayHost(const std::string& host)
{
    return host == "127.0.0.1" || host == "localhost" || host == "::1";
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
    , m_highlightUserId(0)
    , m_nextLoginTxnId(1)
    , m_pendingCreateVocation(0)
    , m_pendingCreateSex(0)
    , m_connectStartMs(0)
    , m_waitResponseStartMs(0)
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

void LoginSession::setOnStatus(StatusCallback cb)
{
    m_onStatus = std::move(cb);
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
    m_connectStartMs     = TimeUtil::nowMs();
    m_waitResponseStartMs = 0;

    ClientLogger::instance().info("LoginSession：连接 LoginServer %s:%u，区服=%u",
                                  loginHost().c_str(), loginPort(), zoneId);
    notifyStatus(u8"正在连接登录服务器...");

    if (!m_tcp->connect(loginHost(), loginPort()))
    {
        fail(ClientLocalError::ConnectFailed, LoginTimeoutContext::LoginServerConnect);
    }
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
    m_connectStartMs  = TimeUtil::nowMs();
    m_waitResponseStartMs = 0;

    ClientLogger::instance().info("LoginSession：注册流程连接 %s:%u", loginHost().c_str(), loginPort());
    notifyStatus(u8"正在连接登录服务器...");

    if (!m_tcp->connect(loginHost(), loginPort()))
    {
        fail(ClientLocalError::ConnectFailed, LoginTimeoutContext::LoginServerConnect);
    }
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

    m_pendingCreateName     = name;
    m_pendingCreateVocation = vocation;
    m_pendingCreateSex      = sex;

    const auto packet = ClientMsgHandler::buildCreateUserReq(name, vocation, sex);
    m_tcp->sendRaw(packet);
    m_state               = State::WaitCreateUserRsp;
    m_waitResponseStartMs = TimeUtil::nowMs();
    ClientLogger::instance().info("LoginSession：创建角色 name=%s", name.c_str());
}

void LoginSession::update()
{
    if (m_state == State::Idle || !m_tcp)
    {
        return;
    }

    const int64_t now = TimeUtil::nowMs();
    if (isConnectingState() && TimeUtil::elapsed(m_connectStartMs, connectTimeoutMs(), now))
    {
        fail(ClientLocalError::ConnectTimeout, connectTimeoutContext());
        return;
    }
    if (isWaitingResponseState() && m_waitResponseStartMs > 0 &&
        TimeUtil::elapsed(m_waitResponseStartMs, responseTimeoutMs(), now))
    {
        fail(ClientLocalError::ResponseTimeout, timeoutContextForState());
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

void LoginSession::resumeGatewayForCharSelect(std::unique_ptr<TcpClient> tcp, uint64_t highlightUserId)
{
    if (!tcp || !tcp->isConnected())
    {
        fail(u8"无法接回游戏网关连接");
        return;
    }

    m_tcp = std::move(tcp);
    m_tcp->setOnMessage([this](uint8_t module, uint8_t sub, const char* data, uint16_t len) {
        onTcpMessage(module, sub, data, len);
    });
    m_tcp->setOnDisconnected([this]() { onTcpDisconnected(); });

    m_gatewayConnected    = true;
    m_gotLoginRsp         = true;
    m_gotGatewayInfo      = true;
    m_gotUserList         = false;
    m_highlightUserId     = highlightUserId;
    m_characters.clear();
    m_pendingSelectUserId = 0;
    m_state               = State::WaitUserList;
    m_waitResponseStartMs = TimeUtil::nowMs();

    ClientLogger::instance().info("LoginSession：接回 Gateway，等待角色列表");
    notifyStatus(u8"正在获取角色列表...");
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
    m_highlightUserId     = 0;
    m_characters.clear();
    m_pendingCreateName.clear();
    m_connectStartMs      = 0;
    m_waitResponseStartMs = 0;
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

void LoginSession::fail(ClientLocalError err, LoginTimeoutContext ctx)
{
    fail(ClientErrorText::localErrorText(err, ctx));
}

int64_t LoginSession::connectTimeoutMs() const
{
    return m_config ? m_config->connectTimeoutMs() : ClientTiming::kConnectTimeoutMs;
}

int64_t LoginSession::responseTimeoutMs() const
{
    return m_config ? m_config->responseTimeoutMs() : ClientTiming::kResponseTimeoutMs;
}

LoginTimeoutContext LoginSession::connectTimeoutContext() const
{
    if (m_state == State::ConnectGateway)
    {
        return LoginTimeoutContext::GatewayConnect;
    }
    return LoginTimeoutContext::LoginServerConnect;
}

LoginTimeoutContext LoginSession::timeoutContextForState() const
{
    switch (m_state)
    {
    case State::WaitLoginRsp:
        return LoginTimeoutContext::Login;
    case State::RegisterWaitRsp:
        return LoginTimeoutContext::Register;
    case State::WaitUserList:
        return LoginTimeoutContext::UserList;
    case State::WaitEnterGame:
        return LoginTimeoutContext::EnterGame;
    case State::WaitCreateUserRsp:
        return LoginTimeoutContext::CreateCharacter;
    default:
        return LoginTimeoutContext::Generic;
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
    if (!m_tcp || !m_gotLoginRsp || !m_gotGatewayInfo || m_gotUserList)
    {
        return;
    }
    if (m_gatewayConnected || m_state == State::SwitchingGateway || m_state == State::ConnectGateway ||
        m_state == State::WaitEnterGame)
    {
        return;
    }

    m_state = State::SwitchingGateway;
    ClientLogger::instance().info("LoginSession：连接 Gateway %s:%u",
                                  m_gatewayHost.c_str(), m_gatewayPort);
    notifyStatus(u8"正在连接游戏网关...");
    m_tcp->disconnect();
    m_state          = State::ConnectGateway;
    m_connectStartMs = TimeUtil::nowMs();
    if (!m_tcp->connect(m_gatewayHost, m_gatewayPort))
    {
        fail(ClientLocalError::ConnectFailed, LoginTimeoutContext::GatewayConnect);
    }
}

void LoginSession::onGatewayAuthSent()
{
    if (m_pendingSelectUserId != 0)
    {
        sendSelectUserReq(m_pendingSelectUserId);
    }
}

void LoginSession::onTcpConnected()
{
    if (m_state == State::ConnectLogin || m_state == State::RegisterConnect)
    {
        m_waitResponseStartMs = TimeUtil::nowMs();
        if (m_isRegisterFlow)
        {
            sendRegisterReq();
            m_state = State::RegisterWaitRsp;
        }
        else
        {
            sendLoginReq();
            m_state = State::WaitLoginRsp;
            notifyStatus(u8"正在验证账号...");
        }
    }
    else if (m_state == State::ConnectGateway)
    {
        m_gatewayConnected = true;
        sendGatewayAuthOrLogin();
        m_state               = State::WaitUserList;
        m_waitResponseStartMs = TimeUtil::nowMs();
        notifyStatus(u8"正在获取角色列表...");
        if (m_pendingSelectUserId != 0)
        {
            onGatewayAuthSent();
        }
    }
}

void LoginSession::onTcpDisconnected()
{
    if (m_state == State::Idle || m_state == State::SwitchingGateway)
    {
        return;
    }

    if (m_state == State::ConnectLogin || m_state == State::RegisterConnect)
    {
        fail(ClientLocalError::Disconnected, LoginTimeoutContext::LoginServerConnect);
        return;
    }
    if (m_state == State::ConnectGateway)
    {
        fail(ClientLocalError::Disconnected, LoginTimeoutContext::GatewayConnect);
        return;
    }

    fail(ClientLocalError::Disconnected, timeoutContextForState());
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

    if (!hasLoginToken(m_loginRsp))
    {
        fail(u8"登录票据缺失，请重新登录");
        return;
    }

    if (m_loginRsp.tokenExpireMs > 0 &&
        TimeUtil::nowMs() >= static_cast<int64_t>(m_loginRsp.tokenExpireMs))
    {
        fail(u8"登录票据已过期，请重新登录");
        return;
    }

    const auto packet = ClientMsgHandler::buildGatewayAuthReq(
        m_account, m_loginRsp.loginToken, m_zoneId, m_gameType);
    m_tcp->sendRaw(packet);
    ClientLogger::instance().info("LoginSession：发送 Gateway 票据鉴权");
}

void LoginSession::sendSelectUserReq(uint64_t userID)
{
    if (!m_tcp || userID == 0)
    {
        return;
    }

    const uint64_t txnId = m_nextLoginTxnId++;
    const auto packet = ClientMsgHandler::buildSelectUserReq(userID, txnId);
    m_tcp->sendRaw(packet);
    m_state               = State::WaitEnterGame;
    m_waitResponseStartMs = TimeUtil::nowMs();
    notifyStatus(u8"正在加载地图与角色资源...");
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

    if (static_cast<LoginResultCode>(rsp.code) != LoginResultCode::OK)
    {
        fail(ClientErrorText::loginResultText(static_cast<LoginResultCode>(rsp.code), rsp.msg));
        return;
    }

    if (!m_gotUserList)
    {
        m_state               = State::WaitUserList;
        m_waitResponseStartMs = TimeUtil::nowMs();
        notifyStatus(u8"正在获取角色列表...");
        tryConnectGateway();
    }
}

void LoginSession::handleUserList()
{
    m_gotUserList = true;
    const uint64_t highlight = m_highlightUserId != 0 ? m_highlightUserId : m_loginRsp.userID;
    m_highlightUserId        = 0;
    if (!m_gatewayConnected)
    {
        deliverUserList(highlight);
        return;
    }
    if (m_state == State::WaitUserList)
    {
        deliverUserList(highlight);
    }
}

void LoginSession::handleCreateUserRsp(const Msg_S2C_CreateUserRsp& rsp)
{
    if (static_cast<CreateCharacterResultCode>(rsp.code) != CreateCharacterResultCode::OK)
    {
        m_state = State::WaitUserAction;
        const std::string errMsg = ClientErrorText::createCharacterText(
            static_cast<CreateCharacterResultCode>(rsp.code), rsp.msg);
        ClientLogger::instance().err("LoginSession：%s", errMsg.c_str());
        if (m_onError)
        {
            m_onError(errMsg);
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

    if (static_cast<GatewayInfoResultCode>(info.code) != GatewayInfoResultCode::OK)
    {
        fail(ClientErrorText::gatewayInfoText(static_cast<GatewayInfoResultCode>(info.code),
                                            info.msg));
        return;
    }

    m_gatewayHost = info.gatewayIP;
    m_gatewayPort = info.gatewayPort;
    if (isLoopbackGatewayHost(m_gatewayHost))
    {
        m_gatewayHost = loginHost();
        ClientLogger::instance().info("LoginSession：网关地址为回环，已替换为 %s",
                                      m_gatewayHost.c_str());
    }

    if (m_gotLoginRsp && !m_gotUserList)
    {
        m_state               = State::WaitUserList;
        m_waitResponseStartMs = TimeUtil::nowMs();
        notifyStatus(u8"正在获取角色列表...");
        tryConnectGateway();
    }
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
    fail(ClientErrorText::gatewayValidateText(err));
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
        else if (sub == static_cast<uint8_t>(SystemMsgSub::S2C_KICK))
        {
            fail(u8"已被服务器踢下线");
        }
        return;
    }

    if (module != kLoginModule)
    {
        return;
    }

    if (flatId == clientMsgFlatId<Msg_S2C_LoginRsp>())
    {
        if (m_gatewayConnected)
        {
            ClientLogger::instance().warn("LoginSession：Gateway 连接收到登录响应，已忽略");
            return;
        }
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

        if (static_cast<RegisterResultCode>(rsp.code) != RegisterResultCode::OK)
        {
            fail(ClientErrorText::registerResultText(static_cast<RegisterResultCode>(rsp.code),
                                                     rsp.msg));
            return;
        }

        ClientLogger::instance().info("LoginSession：注册成功");
        if (m_onRegisterSuccess)
        {
            m_onRegisterSuccess();
        }
    }
}

void LoginSession::notifyStatus(const std::string& message)
{
    if (m_onStatus)
    {
        m_onStatus(message);
    }
}

bool LoginSession::isConnectingState() const
{
    return m_state == State::ConnectLogin || m_state == State::ConnectGateway ||
           m_state == State::RegisterConnect;
}

bool LoginSession::isWaitingResponseState() const
{
    return m_state == State::WaitLoginRsp || m_state == State::RegisterWaitRsp ||
           m_state == State::WaitUserList || m_state == State::WaitEnterGame ||
           m_state == State::WaitCreateUserRsp;
}

std::string LoginSession::responseTimeoutMessage() const
{
    return ClientErrorText::localErrorText(ClientLocalError::ResponseTimeout,
                                           timeoutContextForState());
}
