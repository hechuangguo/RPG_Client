/**
 * @file    LoginSession.h
 * @brief   登录/注册/选角网络会话状态机
 *
 * 职责：
 * - LoginServer 账号登录/注册 → Gateway 票据鉴权 → 角色列表 → 选角/创角 → S2C_ENTER_GAME
 * - 每帧 update() 驱动 TcpClient::poll() 并推进状态机
 *
 * 协作：GameApp、ClientMsgHandler、ConfigLoader、CharacterSelectPanel。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientMsg.h"
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

    LoginSession();
    ~LoginSession();

    void setConfig(const ConfigLoader* config);

    void setOnEnterGame(EnterGameCallback cb);
    void setOnError(ErrorCallback cb);
    void setOnRegisterSuccess(VoidCallback cb);
    void setOnUserList(UserListCallback cb);

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

    void cancel();

    std::unique_ptr<TcpClient> releaseTcpClient();

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
    void deliverUserList(uint64_t highlightUserId);
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
    Msg_S2C_LoginRsp        m_loginRsp;
    Msg_S2C_GatewayInfo     m_gatewayInfo;

    std::string             m_gatewayHost;
    uint16_t                m_gatewayPort;

    std::vector<CharacterEntry> m_characters;
    std::string             m_pendingCreateName;
    uint8_t                 m_pendingCreateVocation;
    uint8_t                 m_pendingCreateSex;

    EnterGameCallback       m_onEnterGame;
    ErrorCallback           m_onError;
    VoidCallback            m_onRegisterSuccess;
    UserListCallback        m_onUserList;
};
