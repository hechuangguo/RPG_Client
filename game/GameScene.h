/**
 * @file    GameScene.h
 * @brief   游戏主场景编排器
 *
 * 职责：
 * - 协调 MapRenderer、WaterSystem、AmbientSystem、BuildingManager、EntityManager
 * - WASD 移动本地玩家，相机跟随，绘制 HUD
 * - 驱动 GameSession 移动同步
 *
 * 协作：GameApp、GameSession、ClientScriptHost、QuestModel。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientProtocol.h"
#include "game/AmbientSystem.h"
#include "game/BuildingManager.h"
#include "game/EntityManager.h"
#include "game/MapRenderer.h"
#include "game/WaterSystem.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <memory>
#include <string>

class GameSession;
class QuestModel;
class UiTheme;

/**
 * @brief 游戏世界场景
 */
class GameScene
{
public:
    GameScene();

    /**
     * @brief 进入游戏并加载地图
     * @param enter  进入游戏消息
     * @param theme  UI 主题（字体）
     * @param session 网络会话
     * @param quests  任务模型
     */
    void enter(const Msg_S2C_EnterGame& enter,
               UiTheme* theme,
               GameSession* session,
               QuestModel* quests);

    /** @brief 处理输入事件 */
    void handleEvent(const sf::Event& event);

    /**
     * @brief 逻辑更新
     * @param dt 帧间隔秒
     */
    void update(float dt);

    /**
     * @brief 渲染场景
     * @param target 渲染目标
     */
    void draw(sf::RenderTarget& target);

    /** @brief 设置窗口尺寸（相机/HUD） */
    void setViewSize(const sf::Vector2u& size);

    /** @brief 处理实体进入视野 */
    void onSpawn(const Msg_S2C_SpawnEntity& spawn);

    /** @brief 处理实体移动 */
    void onMove(const Msg_S2C_MoveNotify& move);

    /** @brief 处理实体离开视野 */
    void onDespawn(const Msg_S2C_DespawnEntity& despawn);

    /** @brief 离开游戏世界，清理场景状态 */
    void leave();

private:
    void updateCamera();
    void drawHud(sf::RenderTarget& target) const;

    MapRenderer     m_map;
    WaterSystem     m_water;
    AmbientSystem   m_ambient;
    BuildingManager m_buildings;
    EntityManager   m_entities;

    UiTheme*        m_theme;
    GameSession*    m_session;
    QuestModel*     m_quests;

    sf::View        m_worldView;
    sf::View        m_uiView;
    sf::Vector2u    m_viewSize;

    Msg_S2C_EnterGame m_playerInfo;
    bool            m_active;

    bool            m_moveUp;
    bool            m_moveDown;
    bool            m_moveLeft;
    bool            m_moveRight;
    float           m_moveSpeed;
};
