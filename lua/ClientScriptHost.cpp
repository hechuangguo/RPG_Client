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
#include "net/ClientMsgHandler.h"
#include "net/GameSession.h"

#include <cstring>
#include <vector>

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
    std::vector<Msg_S2C_QuestEntryWire> entries;
    std::string errMsg;
    if (!ClientMsgHandler::parseQuestInfo(data, len, entries, errMsg))
    {
        ClientLogger::instance().warn("ClientScriptHost：%s", errMsg.c_str());
        return;
    }

    if (m_quests)
    {
        m_quests->clear();
        for (const Msg_S2C_QuestEntryWire& wire : entries)
        {
            QuestEntry q{};
            q.questId  = wire.questId;
            q.name     = wire.name;
            q.progress = wire.progress;
            q.target   = wire.target;
            q.done     = wire.done != 0;
            m_quests->upsert(q);
        }
    }

    if (m_lua && m_lua->isReady())
    {
        for (const Msg_S2C_QuestEntryWire& wire : entries)
        {
            lua_State* L = m_lua->state();
            lua_getglobal(L, "OnQuestInfo");
            if (lua_isfunction(L, -1))
            {
                lua_pushnumber(L, wire.questId);
                lua_pushstring(L, wire.name);
                lua_pushnumber(L, wire.progress);
                lua_pushnumber(L, wire.target);
                lua_pushboolean(L, wire.done != 0);
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
}

void ClientScriptHost::onBagInfo(const char* data, uint16_t len)
{
    std::vector<Msg_S2C_BagSlotWire> slots;
    std::string errMsg;
    if (!ClientMsgHandler::parseBagInfoRsp(data, len, slots, errMsg))
    {
        ClientLogger::instance().warn("ClientScriptHost：%s", errMsg.c_str());
        return;
    }

    std::vector<BagSlot> bagSlots;
    bagSlots.reserve(slots.size());
    for (const Msg_S2C_BagSlotWire& wire : slots)
    {
        if (wire.itemId == 0 || wire.count == 0)
        {
            continue;
        }
        bagSlots.push_back({wire.itemId, wire.count});
    }

    if (m_bag)
    {
        m_bag->setSlots(bagSlots);
    }

    if (m_lua && m_lua->isReady())
    {
        lua_State* L = m_lua->state();
        lua_getglobal(L, "OnBagInfo");
        if (lua_isfunction(L, -1))
        {
            lua_createtable(L, static_cast<int>(bagSlots.size()), 0);
            int idx = 1;
            for (const BagSlot& slot : bagSlots)
            {
                lua_pushnumber(L, idx++);
                lua_createtable(L, 0, 2);
                lua_pushstring(L, "itemId");
                lua_pushnumber(L, slot.itemId);
                lua_settable(L, -3);
                lua_pushstring(L, "count");
                lua_pushnumber(L, slot.count);
                lua_settable(L, -3);
                lua_settable(L, -3);
            }
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
            {
                ClientLogger::instance().warn("ClientScriptHost：调用 OnBagInfo 失败");
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
    }
}

void ClientScriptHost::onChat(const Msg_S2C_Chat& chat)
{
    if (m_lua && m_lua->isReady())
    {
        lua_State* L = m_lua->state();
        lua_getglobal(L, "OnChat");
        if (lua_isfunction(L, -1))
        {
            lua_pushnumber(L, static_cast<lua_Number>(chat.fromID));
            lua_pushstring(L, chat.fromName);
            lua_pushnumber(L, chat.channel);
            lua_pushstring(L, chat.content);
            if (lua_pcall(L, 4, 0, 0) != LUA_OK)
            {
                ClientLogger::instance().warn("ClientScriptHost：调用 OnChat 失败");
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
    }
}

void ClientScriptHost::onNotice(const char* content)
{
    if (!content || content[0] == '\0')
    {
        return;
    }

    ClientLogger::instance().info("ClientScriptHost：系统公告 %s", content);

    if (m_lua && m_lua->isReady())
    {
        lua_State* L = m_lua->state();
        lua_getglobal(L, "OnNotice");
        if (lua_isfunction(L, -1))
        {
            lua_pushstring(L, content);
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
            {
                ClientLogger::instance().warn("ClientScriptHost：调用 OnNotice 失败");
                lua_pop(L, 1);
            }
        }
        else
        {
            lua_pop(L, 1);
        }
    }
}
