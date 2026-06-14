/**
 * @file    ClientMsgHandler.h
 * @brief   客户端消息构造与解析辅助
 *
 * 对 Common/ClientMsg.h 中的常用 C2S/S2C 结构体提供类型安全的
 * 组包（build*）与解包（parse*）接口，避免业务层手动偏移。
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include "ClientMsg.h"
#include "MsgId.h"

#include <cstdint>
#include <string>
#include <vector>

struct GameZoneEntry;

/**
 * @brief 客户端消息辅助类（静态方法）
 */
class ClientMsgHandler
{
public:
    /**
     * @brief 构造 C2S_LOGIN_REQ 完整帧
     * @param account  账号
     * @param password 密码
     * @param zoneId   所选游戏区号
     * @param gameType 游戏类型
     * @return 含 MsgHeader 的字节流
     */
    static std::vector<char> buildLoginReq(const std::string& account,
                                           const std::string& password,
                                           uint32_t zoneId,
                                           uint8_t gameType);

    /**
     * @brief 构造 C2S_REGISTER_REQ 完整帧
     * @param account  账号
     * @param password 密码
     * @param zoneId   所选游戏区号
     * @param gameType 游戏类型
     * @return 含 MsgHeader 的字节流
     */
    static std::vector<char> buildRegisterReq(const std::string& account,
                                              const std::string& password,
                                              uint32_t zoneId,
                                              uint8_t gameType);

    /**
     * @brief 构造 C2S_ZONE_LIST_REQ 完整帧
     * @param gameType 0=全部类型
     */
    static std::vector<char> buildZoneListReq(uint8_t gameType = 0);

    /**
     * @brief 解析 S2C_ZONE_LIST_RSP 消息体（含变长 entries）
     */
    static bool parseZoneListRsp(const char* data,
                                 uint16_t len,
                                 std::vector<GameZoneEntry>& out,
                                 std::string& errMsg);

    /**
     * @brief 构造 C2S_HEARTBEAT 完整帧
     * @param seq 心跳序列号
     * @return 含 MsgHeader 的字节流
     */
    static std::vector<char> buildHeartbeat(uint32_t seq);

    /**
     * @brief 构造 C2S_MOVE_REQ 完整帧
     * @param userID   用户 ID
     * @param x        目标 X
     * @param y        目标 Y
     * @param z        目标 Z
     * @param dir      朝向（弧度）
     * @param moveType 移动类型：0=行走 1=跑步
     * @return 含 MsgHeader 的字节流
     */
    static std::vector<char> buildMoveReq(uint64_t userID,
                                          float x, float y, float z,
                                          float dir,
                                          uint8_t moveType);

    /**
     * @brief 解析 S2C_LOGIN_RSP 消息体
     * @param data 消息体指针
     * @param len  消息体长度
     * @param out  [out] 解析结果
     * @return 长度合法返回 true
     */
    static bool parseLoginRsp(const char* data, uint16_t len, Msg_S2C_LoginRsp& out);

    /**
     * @brief 解析 S2C_GATEWAY_INFO 消息体
     * @param data 消息体指针
     * @param len  消息体长度
     * @param out  [out] 解析结果
     * @return 长度合法返回 true
     */
    static bool parseGatewayInfo(const char* data, uint16_t len, Msg_S2C_GatewayInfo& out);

    /**
     * @brief 解析 S2C_ENTER_GAME 消息体
     * @param data 消息体指针
     * @param len  消息体长度
     * @param out  [out] 解析结果
     * @return 长度合法返回 true
     */
    static bool parseEnterGame(const char* data, uint16_t len, Msg_S2C_EnterGame& out);

    /**
     * @brief 解析 S2C_SPAWN_ENTITY 消息体
     * @param data 消息体指针
     * @param len  消息体长度
     * @param out  [out] 解析结果
     * @return 长度合法返回 true
     */
    static bool parseSpawnEntity(const char* data, uint16_t len, Msg_S2C_SpawnEntity& out);

    /**
     * @brief 解析 S2C_MOVE_NOTIFY 消息体
     * @param data 消息体指针
     * @param len  消息体长度
     * @param out  [out] 解析结果
     * @return 长度合法返回 true
     */
    static bool parseMoveNotify(const char* data, uint16_t len, Msg_S2C_MoveNotify& out);

private:
    static void copyFixedString(char* dest, size_t destSize, const std::string& src);
    static bool copyStruct(const char* data, uint16_t len, void* out, size_t structSize);
};
