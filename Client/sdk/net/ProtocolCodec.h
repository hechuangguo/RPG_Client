/**
 * @file    ProtocolCodec.h
 * @brief   客户端协议封包与拆包
 *
 * 基于 Common/NetDefine.h 的 MsgHeader 格式：
 * [bodyLen:uint16][module:uint8][sub:uint8][body...]
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include "NetDefine.h"

#include <cstdint>
#include <vector>

/**
 * @brief 协议编解码器（静态方法）
 */
class ProtocolCodec
{
public:
    /**
     * @brief 编码一条完整消息帧
     * @param module 功能模块号
     * @param sub    子消息号
     * @param body   消息体指针（可为 nullptr）
     * @param len    消息体长度
     * @return 含 MsgHeader 的完整字节流
     */
    static std::vector<char> encode(uint8_t module, uint8_t sub,
                                    const char* body, uint16_t len);

    /**
     * @brief 从接收缓冲尝试拆出一帧
     * @param buffer   [in/out] 累积接收缓冲，成功时移除已消费字节
     * @param outModule [out] 模块号
     * @param outSub    [out] 子消息号
     * @param outBody   [out] 消息体副本
     * @return 缓冲中有一帧完整消息返回 true；数据不足或非法返回 false
     */
    static bool tryDecode(std::vector<char>& buffer,
                          uint8_t& outModule,
                          uint8_t& outSub,
                          std::vector<char>& outBody);
};
