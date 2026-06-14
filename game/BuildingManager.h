/**
 * @file    BuildingManager.h
 * @brief   主城建筑管理器
 *
 * 职责：
 * - 加载 map/{mapId}/buildings.json
 * - 以矩形占位绘制建筑，并用中文字体显示招牌
 *
 * 协作：GameScene、UiTheme/Font。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 单栋建筑
 */
struct BuildingEntry
{
    std::string id;
    std::string label;
    float       x, z;
    float       width, height;
    sf::Color   color;
};

/**
 * @brief 建筑加载与绘制
 */
class BuildingManager
{
public:
    BuildingManager();

    /**
     * @brief 加载 buildings.json
     * @param mapId 地图 ID
     * @return 成功或 fallback 返回 true
     */
    bool load(uint32_t mapId);

    /**
     * @brief 绘制建筑
     * @param target   渲染目标
     * @param font     中文字体
     * @param tileSize 瓦片尺寸
     */
    void draw(sf::RenderTarget& target, const sf::Font& font, float tileSize) const;

    /** @brief 全部建筑 */
    const std::vector<BuildingEntry>& buildings() const;

private:
    void loadDefaults();

    std::vector<BuildingEntry> m_buildings;
};
