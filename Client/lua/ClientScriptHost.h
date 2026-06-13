/**
 * @file    ClientScriptHost.h
 * @brief   C++ 与 Lua 脚本桥接层
 *
 * 职责：
 * - onEnterGame / onTick / onQuestInfo / onBagInfo 转发到 Lua 全局函数
 * - 同步更新 QuestModel、ItemBagModel
 *
 * 协作：LuaManager、ScriptBindings、GameSession、GameApp。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

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

    /**
     * @brief 绑定 Lua 与数据模型
     * @param lua    Lua 管理器
     * @param quests 任务模型
     * @param bag    背包模型
     * @param session 游戏会话
     */
    void setup(LuaManager* lua, QuestModel* quests, ItemBagModel* bag, GameSession* session);

    /**
     * @brief 进入游戏时通知 Lua
     * @param userId 用户 ID
     * @param mapId  地图 ID
     */
    void onEnterGame(uint64_t userId, uint32_t mapId);

    /**
     * @brief 每帧/定时 Tick
     * @param nowMs 当前毫秒时间戳
     */
    void onTick(int64_t nowMs);

    /**
     * @brief 收到任务同步（简化解析或占位）
     * @param data 消息体
     * @param len  长度
     */
    void onQuestInfo(const char* data, uint16_t len);

    /**
     * @brief 收到背包同步（占位）
     * @param data 消息体
     * @param len  长度
     */
    void onBagInfo(const char* data, uint16_t len);

private:
    LuaManager*    m_lua;
    QuestModel*    m_quests;
    ItemBagModel*  m_bag;
    GameSession*   m_session;
};
