/**
 * @file    TcpClient.cpp
 * @brief   客户端非阻塞 TCP 连接实现
 */

#include "TcpClient.h"

#include "ProtocolCodec.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <algorithm>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

namespace
{
constexpr uintptr_t kInvalidSocket = static_cast<uintptr_t>(INVALID_SOCKET);

bool setNonBlocking(SOCKET sock, bool nonBlocking)
{
    u_long mode = nonBlocking ? 1UL : 0UL;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
}

bool waitSocketWritable(SOCKET sock, int timeoutMs)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    timeval tv{};
    tv.tv_sec  = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    const int ret = select(0, nullptr, &writeSet, nullptr, &tv);
    return ret > 0 && FD_ISSET(sock, &writeSet);
}

bool checkConnectError(SOCKET sock)
{
    int err = 0;
    int len = sizeof(err);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len) != 0)
    {
        return false;
    }
    return err == 0;
}
}  // namespace

TcpClient::TcpClient()
    : m_state(State::Disconnected)
    , m_socket(kInvalidSocket)
    , m_connectNotified(false)
    , m_winsockReady(false)
{
}

TcpClient::~TcpClient()
{
    disconnect();
}

void TcpClient::setOnMessage(MessageCallback cb)
{
    m_onMessage = std::move(cb);
}

void TcpClient::setOnConnected(VoidCallback cb)
{
    m_onConnected = std::move(cb);
}

void TcpClient::setOnDisconnected(VoidCallback cb)
{
    m_onDisconnected = std::move(cb);
}

bool TcpClient::initWinsock()
{
    if (m_winsockReady)
    {
        return true;
    }

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        return false;
    }

    m_winsockReady = true;
    return true;
}

bool TcpClient::connect(const std::string& host, uint16_t port)
{
    disconnect();

    if (!initWinsock())
    {
        return false;
    }

    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    const std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0)
    {
        return false;
    }

    SOCKET sock = INVALID_SOCKET;
    bool connectPending = false;

    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock == INVALID_SOCKET)
        {
            continue;
        }

        if (!setNonBlocking(sock, true))
        {
            closesocket(sock);
            sock = INVALID_SOCKET;
            continue;
        }

        const int ret = ::connect(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));
        if (ret == 0)
        {
            connectPending = false;
            break;
        }

        const int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            connectPending = true;
            break;
        }

        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (sock == INVALID_SOCKET)
    {
        return false;
    }

    int opt = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    m_socket          = static_cast<uintptr_t>(sock);
    m_connectNotified = false;
    m_recvBuffer.clear();
    m_sendBuffer.clear();

    if (connectPending)
    {
        m_state = State::Connecting;
    }
    else
    {
        m_state = State::Connected;
        m_connectNotified = true;
        if (m_onConnected)
        {
            m_onConnected();
        }
    }

    return true;
}

bool TcpClient::send(uint8_t module, uint8_t sub, const char* data, uint16_t len)
{
    return sendRaw(ProtocolCodec::encode(module, sub, data, len));
}

bool TcpClient::sendRaw(const std::vector<char>& packet)
{
    if (m_state != State::Connected || packet.empty())
    {
        return false;
    }

    m_sendBuffer.insert(m_sendBuffer.end(), packet.begin(), packet.end());
    flushSendBuffer();
    return true;
}

void TcpClient::poll()
{
    if (m_socket == kInvalidSocket)
    {
        return;
    }

    SOCKET sock = static_cast<SOCKET>(m_socket);

    if (m_state == State::Connecting)
    {
        if (waitSocketWritable(sock, 0))
        {
            if (checkConnectError(sock))
            {
                handleConnectComplete();
            }
            else
            {
                notifyDisconnected();
                return;
            }
        }
    }

    if (m_state == State::Connected)
    {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        const int ret = select(0, &readSet, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(sock, &readSet))
        {
            handleReadable();
        }

        flushSendBuffer();
    }
}

void TcpClient::disconnect()
{
    if (m_state == State::Connected || m_state == State::Connecting)
    {
        notifyDisconnected();
    }
    else
    {
        closeSocket();
        m_state = State::Disconnected;
    }
}

bool TcpClient::isConnected() const
{
    return m_state == State::Connected;
}

bool TcpClient::isConnecting() const
{
    return m_state == State::Connecting;
}

void TcpClient::closeSocket()
{
    if (m_socket != kInvalidSocket)
    {
        closesocket(static_cast<SOCKET>(m_socket));
        m_socket = kInvalidSocket;
    }
}

void TcpClient::notifyDisconnected()
{
    const bool wasActive = (m_state != State::Disconnected);
    closeSocket();
    m_state           = State::Disconnected;
    m_connectNotified = false;
    m_recvBuffer.clear();
    m_sendBuffer.clear();

    if (wasActive && m_onDisconnected)
    {
        m_onDisconnected();
    }
}

bool TcpClient::flushSendBuffer()
{
    if (m_state != State::Connected || m_sendBuffer.empty())
    {
        return true;
    }

    SOCKET sock = static_cast<SOCKET>(m_socket);
    while (!m_sendBuffer.empty())
    {
        const int sent = ::send(sock, m_sendBuffer.data(),
                                static_cast<int>(m_sendBuffer.size()), 0);
        if (sent > 0)
        {
            m_sendBuffer.erase(m_sendBuffer.begin(),
                               m_sendBuffer.begin() + sent);
            continue;
        }

        const int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            return true;
        }

        notifyDisconnected();
        return false;
    }
    return true;
}

void TcpClient::handleReadable()
{
    SOCKET sock = static_cast<SOCKET>(m_socket);
    char chunk[4096];

    for (;;)
    {
        const int received = recv(sock, chunk, sizeof(chunk), 0);
        if (received > 0)
        {
            m_recvBuffer.insert(m_recvBuffer.end(), chunk, chunk + received);
            continue;
        }

        if (received == 0)
        {
            notifyDisconnected();
            return;
        }

        const int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            break;
        }

        notifyDisconnected();
        return;
    }

    uint8_t module = 0;
    uint8_t sub    = 0;
    std::vector<char> body;
    while (ProtocolCodec::tryDecode(m_recvBuffer, module, sub, body))
    {
        if (m_onMessage)
        {
            m_onMessage(module, sub,
                        body.empty() ? nullptr : body.data(),
                        static_cast<uint16_t>(body.size()));
        }
    }
}

void TcpClient::handleConnectComplete()
{
    if (m_state != State::Connecting)
    {
        return;
    }

    m_state = State::Connected;
    if (!m_connectNotified)
    {
        m_connectNotified = true;
        if (m_onConnected)
        {
            m_onConnected();
        }
    }
}
