/**
 * @file    ProtocolCodec.cpp
 * @brief   客户端协议封包与拆包实现
 */

#include "ProtocolCodec.h"

#include <cstring>

std::vector<char> ProtocolCodec::encode(uint8_t module, uint8_t sub,
                                        const char* body, uint16_t len)
{
    std::vector<char> packet;
    packet.resize(MSG_HEADER_SIZE + len);

    MsgHeader hdr{};
    hdr.bodyLen = len;
    hdr.module  = module;
    hdr.sub     = sub;

    std::memcpy(packet.data(), &hdr, MSG_HEADER_SIZE);
    if (len > 0 && body != nullptr)
    {
        std::memcpy(packet.data() + MSG_HEADER_SIZE, body, len);
    }
    return packet;
}

bool ProtocolCodec::tryDecode(std::vector<char>& buffer,
                              uint8_t& outModule,
                              uint8_t& outSub,
                              std::vector<char>& outBody)
{
    if (buffer.size() < MSG_HEADER_SIZE)
    {
        return false;
    }

    MsgHeader hdr{};
    std::memcpy(&hdr, buffer.data(), MSG_HEADER_SIZE);

    if (hdr.bodyLen > MAX_PACKET_SIZE)
    {
        buffer.clear();
        return false;
    }

    const size_t total = static_cast<size_t>(MSG_HEADER_SIZE) + hdr.bodyLen;
    if (buffer.size() < total)
    {
        return false;
    }

    outModule = hdr.module;
    outSub    = hdr.sub;
    outBody.assign(buffer.begin() + MSG_HEADER_SIZE, buffer.begin() + total);
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(total));
    return true;
}
