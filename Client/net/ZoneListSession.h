/**
 * @file    ZoneListSession.h
 * @brief   向 LoginServer 拉取游戏区列表的短连接会话
 */

#pragma once

#include "net/ZoneTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class ConfigLoader;
class TcpClient;

/**
 * @brief 区列表拉取会话（连接 → 请求 → 响应 → 断开）
 */
class ZoneListSession
{
public:
    using SuccessCallback = std::function<void(const std::vector<GameZoneEntry>&)>;
    using ErrorCallback   = std::function<void(const std::string&)>;

    ZoneListSession();
    ~ZoneListSession();

    void setConfig(const ConfigLoader* config);

    void setOnSuccess(SuccessCallback cb);
    void setOnError(ErrorCallback cb);

    /** @brief 发起拉取（异步，主线程 poll update） */
    void fetchZoneList(uint8_t gameType = 0);

    /** @brief 每帧 poll 网络 */
    void update();

    bool isBusy() const;
    void cancel();

private:
    enum class State
    {
        Idle,
        Connecting,
        WaitResponse,
    };

    void resetToIdle();
    void fail(const std::string& msg);
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpMessage(uint8_t module, uint8_t sub, const char* data, uint16_t len);
    void sendZoneListReq();
    std::string loginHost() const;
    uint16_t loginPort() const;

    const ConfigLoader*        m_config;
    std::unique_ptr<TcpClient> m_tcp;
    State                      m_state;
    uint8_t                    m_gameType;
    SuccessCallback            m_onSuccess;
    ErrorCallback              m_onError;
};
