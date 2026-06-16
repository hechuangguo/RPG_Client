/**
 * @file    ClientScriptHost.cpp
 * @brief   ClientScriptHost 实现
 */

#include "lua/ClientScriptHost.h"

#include "game/ItemBagModel.h"
#include "game/QuestModel.h"
#include "lua/LuaManager.h"
#include "lua/ScriptBindings.h"
#include "log/ClientLogger.h"
#include "net/GameSession.h"

extern "C" {
#include <lua.h>
}

ClientScriptHost::ClientScriptHost()
    : m_lua(nullptr)
    , m_quests(nullptr)
    , m_bag(nullptr)
    , m_session(nullptr)
{
}

void ClientScriptHost::setup(LuaManager* lua, QuestModel* quests, ItemBagModel* bag, GameSession* session)
{
    m_lua     = lua;
    m_quests  = quests;
    m_bag     = bag;
    m_session = session;

    if (m_lua && m_lua->isReady())
    {
        ScriptBindings::registerAll(m_lua->state(), m_session);
    }
}

void ClientScriptHost::onEnterGame(uint64_t userId, uint32_t mapId)
{
    ScriptBindings::setUserId(userId);

    if (m_lua && m_lua->isReady())
    {
        m_lua->callGlobalVoid2("OnEnterGame",
                               static_cast<double>(userId),
                               static_cast<double>(mapId));
    }
}

void ClientScriptHost::onTick(int64_t nowMs)
{
    if (m_lua && m_lua->isReady())
    {
        m_lua->callGlobalVoid1("OnTick", static_cast<double>(nowMs));
    }
}

void ClientScriptHost::onQuestInfo(const char* data, uint16_t len)
{
    (void)data;
    (void)len;

    if (m_quests)
    {
        QuestEntry q{};
        q.questId  = 1001;
        q.name     = u8"初入仙门";
        q.progress = 0;
        q.target   = 1;
        q.done     = false;
        m_quests->upsert(q);
    }

    if (m_lua && m_lua->isReady())
    {
        lua_State* L = m_lua->state();
        lua_getglobal(L, "OnQuestInfo");
        if (lua_isfunction(L, -1))
        {
            lua_pushnumber(L, 1001);
            lua_pushstring(L, u8"初入仙门");
            lua_pushnumber(L, 0);
            lua_pushnumber(L, 1);
            lua_pushboolean(L, 0);
            if (lua_pcall(L, 5, 0, 0) != LUA_OK)
            {
                ClientLogger::instance().warn("ClientScriptHost：调用 OnQuestInfo 失败");
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
    }
}

void ClientScriptHost::onBagInfo(const char* data, uint16_t len)
{
    (void)data;
    (void)len;

    if (m_bag)
    {
        m_bag->setSlots({{1001, 5}, {1002, 1}});
    }

    if (m_lua && m_lua->isReady())
    {
        m_lua->callGlobalVoid1("OnBagInfo", 2.0);
    }
}
