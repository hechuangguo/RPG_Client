/**
 * @file    LoginSession.h
 * @brief   登录/注册/选角网络会话状态机
 *
 * 职责：
 * - LoginServer：账号登录/注册、S2C_GATEWAY_INFO
 * - Gateway：票据鉴权、S2C_USER_LIST、创角、选角、S2C_ENTER_GAME、离世界后接回列表
 * - 每帧 update() 驱动 TcpClient::poll() 并推进状态机
 *
 * 协议见 Common/LoginMsg.h（wire v2 body 前缀 module/sub）。
 *
 * 协作：GameApp、ClientMsgHandler、ConfigLoader、CharacterSelectPanel。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientProtocol.h"
#include "net/CharacterTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class ConfigLoader;
class TcpClient;

/**
 * @brief 登录/注册/选角会话（单连接状态机）
 */
class LoginSession
{
public:
    using EnterGameCallback = std::function<void(const Msg_S2C_EnterGame&)>;
    using ErrorCallback     = std::function<void(const std::string&)>;
    using VoidCallback      = std::function<void()>;
    using UserListCallback  = std::function<void(const std::vector<CharacterEntry>& chars,
                                                 uint64_t lastUserId)>;
    using StatusCallback    = std::function<void(const std::string&)>;

    LoginSession();
    ~LoginSession();

    void setConfig(const ConfigLoader* config);

    void setOnEnterGame(EnterGameCallback cb);
    void setOnError(ErrorCallback cb);
    void setOnRegisterSuccess(VoidCallback cb);
    void setOnUserList(UserListCallback cb);
    void setOnStatus(StatusCallback cb);

    void startLogin(const std::string& account,
                    const std::string& password,
                    uint32_t zoneId,
                    uint8_t gameType);

    void startRegister(const std::string& account,
                       const std::string& password,
                       const std::string& confirmPassword,
                       uint32_t zoneId,
                       uint8_t gameType);

    void selectCharacter(uint64_t userID);
    void createCharacter(const std::string& name, uint8_t vocation, uint8_t sex);

    void update();

    bool isBusy() const;
    bool gatewayConnected() const;

    void cancel();

    std::unique_ptr<TcpClient> releaseTcpClient();

    /**
     * @brief 从 GameSession 接回 Gateway 连接并等待角色列表（返回选角）
     * @param tcp              未断开的 Gateway TcpClient
     * @param highlightUserId  高亮角色（通常为刚离世界的角色）
     */
    void resumeGatewayForCharSelect(std::unique_ptr<TcpClient> tcp, uint64_t highlightUserId);

    const std::string& gatewayHost() const;
    uint16_t gatewayPort() const;

private:
    enum class State
    {
        Idle,
        ConnectLogin,
        WaitLoginRsp,
        SwitchingGateway,
        ConnectGateway,
        WaitUserList,
        WaitUserAction,
        WaitCreateUserRsp,
        WaitEnterGame,
        RegisterConnect,
        RegisterWaitRsp,
    };

    void resetToIdle();
    void fail(const std::string& msg);
    void onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len);
    void onTcpConnected();
    void onTcpDisconnected();
    void sendLoginReq();
    void sendRegisterReq();
    void sendGatewayAuthOrLogin();
    void sendSelectUserReq(uint64_t userID);
    void deliverUserList(uint64_t highlightUserId);
    void tryConnectGateway();
    void onGatewayAuthSent();
    void handleLoginRsp(const Msg_S2C_LoginRsp& rsp);
    void handleUserList();
    void handleCreateUserRsp(const Msg_S2C_CreateUserRsp& rsp);
    void handleGatewayInfo(const Msg_S2C_GatewayInfo& info);
    void handleEnterGame(const char* data, uint16_t len);
    void handleSystemError(const char* data, uint16_t len);
    void notifyStatus(const std::string& message);
    bool isConnectingState() const;
    bool isWaitingResponseState() const;
    std::string responseTimeoutMessage() const;
    std::string loginHost() const;
    uint16_t loginPort() const;

    const ConfigLoader*        m_config;
    std::unique_ptr<TcpClient> m_tcp;

    State                   m_state;
    bool                    m_isRegisterFlow;

    std::string             m_account;
    std::string             m_password;
    std::string             m_confirmPassword;
    uint32_t                m_zoneId;
    uint8_t                 m_gameType;

    bool                    m_gotLoginRsp;
    bool                    m_gotGatewayInfo;
    bool                    m_gotUserList;
    bool                    m_userCreated;
    bool                    m_gatewayConnected;
    Msg_S2C_LoginRsp        m_loginRsp;
    Msg_S2C_GatewayInfo     m_gatewayInfo;

    std::string             m_gatewayHost;
    uint16_t                m_gatewayPort;

    std::vector<CharacterEntry> m_characters;
    uint64_t                m_pendingSelectUserId;
    uint64_t                m_highlightUserId;
    std::string             m_pendingCreateName;
    uint8_t                 m_pendingCreateVocation;
    uint8_t                 m_pendingCreateSex;
    int64_t                 m_connectStartMs;
    int64_t                 m_waitResponseStartMs;

    EnterGameCallback       m_onEnterGame;
    ErrorCallback           m_onError;
    VoidCallback            m_onRegisterSuccess;
    UserListCallback        m_onUserList;
    StatusCallback          m_onStatus;
};
