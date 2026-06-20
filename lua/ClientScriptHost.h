/**
 * @file    ClientScriptHost.h
 * @brief   C++ 与 Lua 脚本桥接层
 */

#pragma once

#include "ClientProtocol.h"

#include <cstdint>

class GameSession;
class ItemBagModel;
class LuaManager;
class QuestModel;

/**
 * @brief 客户端脚本宿主
 */
class ClientScriptHost
{
public:
    ClientScriptHost();

    void setup(LuaManager* lua, QuestModel* quests, ItemBagModel* bag, GameSession* session);

    void onEnterGame(uint64_t userId, uint32_t mapId);
    void onTick(int64_t nowMs);
    void onQuestInfo(const char* data, uint16_t len);
    void onBagInfo(const char* data, uint16_t len);
    void onChat(const Msg_S2C_Chat& chat);
    void onNotice(const char* content);

private:
    LuaManager*    m_lua;
    QuestModel*    m_quests;
    ItemBagModel*  m_bag;
    GameSession*   m_session;
};
