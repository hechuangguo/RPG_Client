/**
 * @file    EntityManager.h
 * @brief   游戏实体管理器
 *
 * 职责：
 * - 维护本地玩家与远端实体（玩家/NPC/怪物等）
 * - 处理 spawn/move/despawn 协议更新
 *
 * 协作：GameSession、GameScene。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientMsg.h"

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

/**
 * @brief 单个可见实体
 */
struct GameEntity
{
    uint64_t    id;         /**< 实体 ID */
    std::string name;       /**< 展示名 */
    uint32_t    level;      /**< 等级 */
    float       x, y, z;    /**< 世界坐标 */
    float       dir;        /**< 朝向弧度 */
    uint8_t     entityType; /**< 实体类型 */
    bool        isLocal;    /**< 是否本地玩家 */
};

/**
 * @brief 实体集合管理
 */
class EntityManager
{
public:
    EntityManager();

    /**
     * @brief 从进入游戏消息初始化本地玩家
     * @param enter 进入游戏数据
     */
    void setLocalPlayer(const Msg_S2C_EnterGame& enter);

    /** @brief 添加或更新实体（spawn） */
    void onSpawn(const Msg_S2C_SpawnEntity& spawn);

    /** @brief 更新实体位置（move notify） */
    void onMove(const Msg_S2C_MoveNotify& move);

    /** @brief 移除实体（despawn） */
    void onDespawn(const Msg_S2C_DespawnEntity& despawn);

    /**
     * @brief 更新本地玩家位置（WASD）
     * @param x X
     * @param y Y
     * @param z Z
     * @param dir 朝向
     */
    void setLocalPosition(float x, float y, float z, float dir);

    /** @brief 本地玩家 ID */
    uint64_t localUserId() const;

    /** @brief 本地玩家实体（可能不存在） */
    const GameEntity* localPlayer() const;

    /** @brief 全部实体 */
    const std::unordered_map<uint64_t, GameEntity>& entities() const;

    /**
     * @brief 绘制所有实体（占位矩形+名字）
     * @param target 渲染目标
     * @param font   字体
     * @param tileSize 地图格尺寸（用于坐标换算）
     */
    void draw(sf::RenderTarget& target, const sf::Font& font, float tileSize) const;

private:
    sf::Color colorForType(uint8_t entityType) const;

    uint64_t m_localUserId;
    std::unordered_map<uint64_t, GameEntity> m_entities;
};
