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
    lua_register(L, "send_chat", luaSendChat);
    lua_register(L, "send_quest_accept", luaSendQuestAccept);
    lua_register(L, "send_quest_submit", luaSendQuestSubmit);
    lua_register(L, "request_bag_info", luaRequestBagInfo);
    return true;
}

void ScriptBindings::setUserId(uint64_t userId)
{
    s_userId = userId;
}

int ScriptBindings::luaLogInfo(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    ClientLogger::instance().info("【Lua】%s", msg);
    return 0;
}

int ScriptBindings::luaLogWarn(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    ClientLogger::instance().warn("【Lua】%s", msg);
    return 0;
}

int ScriptBindings::luaLogErr(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    ClientLogger::instance().err("【Lua】%s", msg);
    return 0;
}

int ScriptBindings::luaGetUserId(lua_State* L)
{
    lua_pushinteger(L, static_cast<lua_Integer>(s_userId));
    return 1;
}

int ScriptBindings::luaSendPacket(lua_State* L)
{
    size_t len        = 0;
    const char* bytes = luaL_checklstring(L, 1, &len);

    if (s_session && s_session->isConnected())
    {
        s_session->sendRaw(std::vector<char>(bytes, bytes + len));
    }
    else
    {
        ClientLogger::instance().warn("【Lua】send_packet 失败（未连接）");
    }
    return 0;
}

int ScriptBindings::luaSendChat(lua_State* L)
{
    const int channel = static_cast<int>(luaL_checkinteger(L, 1));
    const char* text  = luaL_checkstring(L, 2);
    if (s_session && s_session->isConnected())
    {
        s_session->sendChat(static_cast<uint8_t>(channel), text);
    }
    return 0;
}

int ScriptBindings::luaSendQuestAccept(lua_State* L)
{
    const uint32_t questId = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    if (s_session && s_session->isConnected())
    {
        s_session->sendQuestAccept(questId);
    }
    return 0;
}

int ScriptBindings::luaSendQuestSubmit(lua_State* L)
{
    const uint32_t questId = static_cast<uint32_t>(luaL_checkinteger(L, 1));
    if (s_session && s_session->isConnected())
    {
        s_session->sendQuestSubmit(questId);
    }
    return 0;
}

int ScriptBindings::luaRequestBagInfo(lua_State* L)
{
    (void)L;
    if (s_session && s_session->isConnected())
    {
        s_session->requestBagInfo();
    }
    return 0;
}
