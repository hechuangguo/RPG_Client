/**
 * @file    ScriptBindings.cpp
 * @brief   ScriptBindings 实现
 */

#include "lua/ScriptBindings.h"

#include "log/ClientLogger.h"
#include "net/GameSession.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <cstring>
#include <vector>

uint64_t ScriptBindings::s_userId = 0;
GameSession* ScriptBindings::s_session = nullptr;

bool ScriptBindings::registerAll(lua_State* L, GameSession* session)
{
    if (!L)
    {
        return false;
    }

    s_session = session;

    lua_register(L, "log_info", luaLogInfo);
    lua_register(L, "log_warn", luaLogWarn);
    lua_register(L, "log_err", luaLogErr);
    lua_register(L, "get_user_id", luaGetUserId);
    lua_register(L, "send_packet", luaSendPacket);
    return true;
}

void ScriptBindings::setUserId(uint64_t userId)
{
    s_userId = userId;
}

int ScriptBindings::luaLogInfo(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    ClientLogger::instance().info("[Lua] %s", msg);
    return 0;
}

int ScriptBindings::luaLogWarn(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    ClientLogger::instance().warn("[Lua] %s", msg);
    return 0;
}

int ScriptBindings::luaLogErr(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    ClientLogger::instance().err("[Lua] %s", msg);
    return 0;
}

int ScriptBindings::luaGetUserId(lua_State* L)
{
    lua_pushinteger(L, static_cast<lua_Integer>(s_userId));
    return 1;
}

int ScriptBindings::luaSendPacket(lua_State* L)
{
    const int module = static_cast<int>(luaL_checkinteger(L, 1));
    const int sub    = static_cast<int>(luaL_checkinteger(L, 2));
    size_t len       = 0;
    const char* bytes = luaL_checklstring(L, 3, &len);

    if (s_session && s_session->isConnected())
    {
        // Stub: real send would use TcpClient via GameSession friend API
        ClientLogger::instance().info("[Lua] send_packet module=%d sub=%d len=%zu",
                                      module, sub, len);
        (void)bytes;
    }
    else
    {
        ClientLogger::instance().warn("[Lua] send_packet stub (not connected)");
    }
    return 0;
}
