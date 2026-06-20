/**
 * @file    ScriptBindings.h
 * @brief   C++ 注册到 Lua 的全局函数绑定
 */

#pragma once

#include <cstdint>

struct lua_State;

class GameSession;

class ScriptBindings
{
public:
    static bool registerAll(lua_State* L, GameSession* session);
    static void setUserId(uint64_t userId);

private:
    static int luaLogInfo(lua_State* L);
    static int luaLogWarn(lua_State* L);
    static int luaLogErr(lua_State* L);
    static int luaGetUserId(lua_State* L);
    static int luaSendPacket(lua_State* L);
    static int luaSendChat(lua_State* L);
    static int luaSendQuestAccept(lua_State* L);
    static int luaSendQuestSubmit(lua_State* L);
    static int luaRequestBagInfo(lua_State* L);

    static uint64_t s_userId;
    static GameSession* s_session;
};
