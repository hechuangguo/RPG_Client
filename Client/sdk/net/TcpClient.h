/**
 * @file    TcpClient.h
 * @brief   客户端非阻塞 TCP 连接
 *
 * 职责：
 * - Windows WinSock 非阻塞 connect/send/recv
 * - 主循环 poll() 驱动 IO 与按 MsgHeader 切帧
 * - 通过 std::function 回调上报连接与消息事件
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/**
 * @brief 客户端 TCP 连接器（单连接）
 */
class TcpClient
{
public:
    using MessageCallback = std::function<void(uint8_t module, uint8_t sub,
                                               const char* data, uint16_t len)>;
    using VoidCallback    = std::function<void()>;

    TcpClient();
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    /**
     * @brief 设置收到完整消息时的回调
     * @param cb 回调函数
     */
    void setOnMessage(MessageCallback cb);

    /** @brief 设置连接建立成功回调 */
    void setOnConnected(VoidCallback cb);

    /** @brief 设置连接断开回调 */
    void setOnDisconnected(VoidCallback cb);

    /**
     * @brief 发起非阻塞连接
     * @param host 目标 IP 或主机名
     * @param port 目标端口
     * @return 成功创建 socket 并发起 connect 返回 true
     */
    bool connect(const std::string& host, uint16_t port);

    /**
     * @brief 发送一条消息（自动添加 MsgHeader）
     * @param module 模块号
     * @param sub    子消息号
     * @param data   消息体
     * @param len    消息体长度
     * @return 已写入发送队列返回 true
     */
    bool send(uint8_t module, uint8_t sub, const char* data, uint16_t len);

    /**
     * @brief 发送已编码的完整帧
     * @param packet encode() 产出的字节流
     * @return 已写入发送队列返回 true
     */
    bool sendRaw(const std::vector<char>& packet);

    /**
     * @brief 单帧 IO 驱动（主循环每帧调用）
     *
     * 处理非阻塞 connect 完成、recv 收包、send 刷出及断线检测。
     */
    void poll();

    /** @brief 主动断开并释放 socket */
    void disconnect();

    /** @brief 是否处于已连接且未断开状态 */
    bool isConnected() const;

    /** @brief 是否正在连接中（connect 已发起但未 OnConnected） */
    bool isConnecting() const;

private:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
    };

    void closeSocket();
    void notifyDisconnected();
    bool flushSendBuffer();
    void handleReadable();
    void handleConnectComplete();
    bool initWinsock();

    MessageCallback m_onMessage;
    VoidCallback    m_onConnected;
    VoidCallback    m_onDisconnected;

    State           m_state;
    uintptr_t       m_socket;
    std::vector<char> m_recvBuffer;
    std::vector<char> m_sendBuffer;
    bool            m_connectNotified;
    bool            m_winsockReady;
};
