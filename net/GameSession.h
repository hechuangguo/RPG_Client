/**
 * @file    GameSession.h
 * @brief   游戏内网络会话
 *
 * 职责：
 * - 维持 Gateway 连接，10 秒心跳 C2S_HEARTBEAT
 * - 处理 S2C_SPAWN_ENTITY / S2C_MOVE_NOTIFY / S2C_DESPAWN_ENTITY
 * - 节流发送 C2S_MOVE_REQ（本地玩家移动）
 * - C2S_LOGOUT_REQ 离世界（返回选角/返回登录）
 * - 将任务/背包等消息转发给 ClientScriptHost
 *
 * 协作：GameScene、EntityManager、ClientScriptHost、TcpClient、LoginSession。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientProtocol.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class ClientScriptHost;
class ConfigLoader;
class TcpClient;

/**
 * @brief 游戏世界网络会话
 */
class GameSession
{
public:
    using SpawnCallback   = std::function<void(const Msg_S2C_SpawnEntity&)>;
    using MoveCallback    = std::function<void(const Msg_S2C_MoveNotify&)>;
    using DespawnCallback = std::function<void(const Msg_S2C_DespawnEntity&)>;
    using ErrorCallback   = std::function<void(const std::string&)>;
    using LogoutCallback  = std::function<void(LogoutAction action)>;

    GameSession();
    ~GameSession();

    void setConfig(const ConfigLoader* config);

    /** @brief 设置 Lua 脚本桥（可选） */
    void setScriptHost(ClientScriptHost* host);

    void setOnSpawn(SpawnCallback cb);
    void setOnMove(MoveCallback cb);
    void setOnDespawn(DespawnCallback cb);
    void setOnError(ErrorCallback cb);

    /** @brief 连接断开回调 */
    void setOnDisconnected(std::function<void()> cb);

    /**
     * @brief 接管 LoginSession 已建立的 Gateway 连接
     * @param tcp   LoginSession 释放的 TcpClient（仍保持连接）
     * @param enter 进入游戏数据
     */
    void start(std::unique_ptr<TcpClient> tcp, const Msg_S2C_EnterGame& enter);

    /**
     * @brief 每帧更新
     * @param dt 帧间隔秒数
     */
    void update(float dt);

    /**
     * @brief 发送玩家移动（内部节流）
     */
    void sendMove(uint64_t userID, float x, float y, float z, float dir, uint8_t moveType);

    /**
     * @brief 请求离世界
     * @param action    RETURN_CHAR_SELECT 或 RETURN_LOGIN
     * @param onSuccess 收到 S2C_LOGOUT_RSP 且 code=0
     * @param onError   超时或失败
     */
    void requestLogout(LogoutAction action, LogoutCallback onSuccess, ErrorCallback onError);

    /** @brief 停止世界逻辑（心跳/移动），不断开连接 */
    void leaveWorld();

    /** @brief 断开连接 */
    void disconnect();

    /** @brief 移交 TcpClient 给 LoginSession（不断开） */
    std::unique_ptr<TcpClient> releaseTcpClient();

    /** @brief 是否已连接 */
    bool isConnected() const;

    /** @brief 是否正在等待离世界响应 */
    bool isWaitingLogout() const;

    /** @brief 本地玩家 userID */
    uint64_t localUserId() const;

    /** @brief 进入游戏快照 */
    const Msg_S2C_EnterGame& enterGameInfo() const;

private:
    void bindTcpCallbacks();
    void onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len);
    void sendHeartbeat();
    void flushMoveIfNeeded();
    void clearLogoutWait();

    int64_t heartbeatIntervalMs() const;
    int64_t moveSendIntervalMs() const;
    int64_t logoutTimeoutMs() const;

    const ConfigLoader*        m_config;
    std::unique_ptr<TcpClient> m_tcp;
    ClientScriptHost*          m_scriptHost;

    Msg_S2C_EnterGame          m_enterInfo;
    uint64_t                   m_localUserId;

    int64_t                    m_lastHeartbeatMs;
    int64_t                    m_lastMoveSendMs;
    uint32_t                   m_heartbeatSeq;

    bool                       m_inWorld;
    bool                       m_moveDirty;
    float                      m_pendingX;
    float                      m_pendingY;
    float                      m_pendingZ;
    float                      m_pendingDir;
    uint8_t                    m_pendingMoveType;

    bool                       m_waitingLogoutRsp;
    LogoutAction               m_pendingLogoutAction;
    int64_t                    m_logoutWaitStartMs;
    LogoutCallback             m_onLogoutSuccess;
    ErrorCallback              m_onLogoutError;

    SpawnCallback              m_onSpawn;
    MoveCallback               m_onMove;
    DespawnCallback            m_onDespawn;
    ErrorCallback              m_onError;
    std::function<void()>      m_onDisconnected;
};
