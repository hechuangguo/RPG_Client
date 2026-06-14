/**
 * @file    WaterSystem.h
 * @brief   河道与水生动物系统
 *
 * 职责：
 * - 绘制穿城河道流动水面（动画 UV/颜色变化）
 * - 生成鱼、虾等氛围实体并更新游动
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
 * @brief 水生动物占位实体
 */
struct WaterCreature
{
    float     x, z;     /**< 河道内坐标（格） */
    float     speed;    /**< 游动速度 */
    float     phase;    /**< 动画相位 */
    bool      isFish;   /**< true=鱼 false=虾 */
    sf::Color color;
};

/**
 * @brief 河道水面与鱼虾渲染
 */
class WaterSystem
{
public:
    WaterSystem();

    /**
     * @brief 加载河道配置
     * @param mapId 地图 ID
     * @return 成功或 fallback 返回 true
     */
    bool load(uint32_t mapId);

    /**
     * @brief 更新动画
     * @param dt 帧间隔秒
     */
    void update(float dt);

    /**
     * @brief 绘制水面与生物
     * @param target   渲染目标
     * @param tileSize 瓦片像素尺寸
     */
    void draw(sf::RenderTarget& target, float tileSize) const;

private:
    void spawnDefaultCreatures();

    float                   m_animTime;
    int                     m_riverStartCol;
    int                     m_riverEndCol;
    int                     m_mapHeight;
    std::vector<WaterCreature> m_creatures;
};
