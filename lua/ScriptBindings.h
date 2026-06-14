/**
 * @file    ScriptBindings.h
 * @brief   C++ 注册到 Lua 的全局函数绑定
 *
 * 职责：
 * - 注册 log_info / log_warn / log_err → ClientLogger
 * - get_user_id、send_packet 等 stub 供脚本调用
 *
 * 协作：LuaManager、GameSession、ClientScriptHost。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <cstdint>

struct lua_State;

class GameSession;

/**
 * @brief Lua C 函数绑定注册器
 */
class ScriptBindings
{
public:
    /**
     * @brief 注册全部全局函数到 lua_State
     * @param L       Lua 状态
     * @param session 游戏会话（可为 nullptr，send_packet 为 stub）
     * @return 成功返回 true
     */
    static bool registerAll(lua_State* L, GameSession* session);

    /** @brief 设置当前用户 ID（get_user_id 用） */
    static void setUserId(uint64_t userId);

private:
    static int luaLogInfo(lua_State* L);
    static int luaLogWarn(lua_State* L);
    static int luaLogErr(lua_State* L);
    static int luaGetUserId(lua_State* L);
    static int luaSendPacket(lua_State* L);

    static uint64_t s_userId;
    static GameSession* s_session;
};
