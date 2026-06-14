/**
 * @file    AmbientSystem.h
 * @brief   环境氛围系统（天象、云、鸟、路人 NPC）
 *
 * 职责：
 * - 绘制阳光渐变天空与视差云朵
 * - 飞鸟掠过；ambient.json 驱动的氛围 NPC 巡逻
 *
 * 协作：GameScene、MapRenderer。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 氛围 NPC
 */
struct AmbientNpc
{
    std::string label;
    float       x, z;
    float       speed;
    float       minZ, maxZ;
    int         dir;
};

/**
 * @brief 云朵层
 */
struct CloudLayer
{
    float x;
    float speed;
    float y;
    float scale;
    sf::Color color;
};

/**
 * @brief 飞鸟
 */
struct Bird
{
    float x, y;
    float speed;
};

/**
 * @brief 环境氛围渲染与更新
 */
class AmbientSystem
{
public:
    AmbientSystem();

    /**
     * @brief 加载 ambient.json
     * @param mapId 地图 ID
     * @return 成功或 fallback 返回 true
     */
    bool load(uint32_t mapId);

    /**
     * @brief 更新云、鸟、路人
     * @param dt       帧间隔
     * @param viewSize 窗口尺寸
     */
    void update(float dt, const sf::Vector2u& viewSize);

    /**
     * @brief 绘制天空层（世界视图之前调用）
     * @param target   渲染目标
     * @param viewSize 窗口尺寸
     */
    void drawSky(sf::RenderTarget& target, const sf::Vector2u& viewSize) const;

    /**
     * @brief 绘制实体层氛围（地图之上）
     * @param target   渲染目标
     * @param font     字体
     * @param tileSize 瓦片尺寸
     */
    void drawWorld(sf::RenderTarget& target, const sf::Font& font, float tileSize) const;

private:
    void setupDefaults();

    std::vector<CloudLayer> m_clouds;
    std::vector<Bird>       m_birds;
    std::vector<AmbientNpc> m_npcs;
};
