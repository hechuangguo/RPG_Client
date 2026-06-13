/**
 * @file    LoginSession.h
 * @brief   登录/注册网络会话状态机
 *
 * 职责：
 * - ConnectLogin → login rsp + gateway → connect gateway → login + enter game
 * - 注册流程：连 LoginServer → C2S_REGISTER_REQ → S2C_REGISTER_RSP
 * - 每帧 update() 驱动 TcpClient::poll() 并推进状态机
 *
 * 协作：GameApp、ClientMsgHandler、ServerListLoader。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientMsg.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class ServerListLoader;
class TcpClient;

/**
 * @brief 登录/注册会话（单连接状态机）
 */
class LoginSession
{
public:
    using EnterGameCallback = std::function<void(const Msg_S2C_EnterGame&)>;
    using ErrorCallback     = std::function<void(const std::string&)>;
    using VoidCallback      = std::function<void()>;

    LoginSession();
    ~LoginSession();

    /**
     * @brief 设置区服列表数据源
     * @param loader 已 load 的 ServerListLoader 指针（生命周期由 GameApp 管理）
     */
    void setServerList(const ServerListLoader* loader);

    /** @brief 进入游戏成功回调 */
    void setOnEnterGame(EnterGameCallback cb);

    /** @brief 错误回调（登录/注册失败、网络断线等） */
    void setOnError(ErrorCallback cb);

    /** @brief 注册成功回调 */
    void setOnRegisterSuccess(VoidCallback cb);

    /**
     * @brief 发起登录流程
     * @param account  账号
     * @param password 密码
     * @param zoneId   所选游戏区号
     * @param gameType 游戏类型
     */
    void startLogin(const std::string& account,
                    const std::string& password,
                    uint32_t zoneId,
                    uint8_t gameType);

    /**
     * @brief 发起注册流程
     * @param account  账号
     * @param password 密码
     * @param zoneId   所选游戏区号
     * @param gameType 游戏类型
     */
    void startRegister(const std::string& account,
                       const std::string& password,
                       uint32_t zoneId,
                       uint8_t gameType);

    /**
     * @brief 每帧更新（poll 网络 + 状态机）
     * @note GameApp 主循环在 Connecting 状态时调用
     */
    void update();

    /** @brief 是否正在登录/注册流程中 */
    bool isBusy() const;

    /** @brief 取消当前流程并断开连接 */
    void cancel();

    /**
     * @brief 释放 TcpClient 所有权（进入游戏后交给 GameSession，不断开连接）
     * @return 唯一 TcpClient 指针；Idle 状态返回 nullptr
     */
    std::unique_ptr<TcpClient> releaseTcpClient();

    /** @brief 最近一次 Gateway 地址（供 GameSession 复用连接，若需要） */
    const std::string& gatewayHost() const;

    /** @brief Gateway 端口 */
    uint16_t gatewayPort() const;

private:
    enum class State
    {
        Idle,
        ConnectLogin,
        WaitLoginRsp,
        WaitGatewayInfo,
        ConnectGateway,
        WaitGatewayLoginRsp,
        WaitEnterGame,
        SwitchingGateway,
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
    std::string loginHost() const;
    uint16_t loginPort() const;

    const ServerListLoader*       m_serverList;
    std::unique_ptr<TcpClient>    m_tcp;

    State                   m_state;
    bool                    m_isRegisterFlow;

    std::string             m_account;
    std::string             m_password;
    uint32_t                m_zoneId;
    uint8_t                 m_gameType;

    bool                    m_gotLoginRsp;
    bool                    m_gotGatewayInfo;
    Msg_S2C_LoginRsp        m_loginRsp;
    Msg_S2C_GatewayInfo     m_gatewayInfo;

    std::string             m_gatewayHost;
    uint16_t                m_gatewayPort;

    EnterGameCallback       m_onEnterGame;
    ErrorCallback           m_onError;
    VoidCallback            m_onRegisterSuccess;
};
