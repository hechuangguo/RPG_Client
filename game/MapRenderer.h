/**
 * @file    MapRenderer.h
 * @brief   地图瓦片渲染器
 *
 * 职责：
 * - 加载 map/{mapId}/ground.json
 * - 以 bluestone/grass 等 tile 类型绘制彩色矩形占位
 *
 * 协作：GameScene、PathUtil。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 地图地面渲染
 */
class MapRenderer
{
public:
    MapRenderer();

    /**
     * @brief 加载地图地面
     * @param mapId 地图 ID（如 1002）
     * @return 成功返回 true；文件缺失时使用程序化默认地图
     */
    bool load(uint32_t mapId);

    /** @brief 瓦片宽度（像素） */
    float tileWidth() const;

    /** @brief 瓦片高度（像素） */
    float tileHeight() const;

    /** @brief 地图宽（格数） */
    int mapWidthTiles() const;

    /** @brief 地图高（格数） */
    int mapHeightTiles() const;

    /**
     * @brief 绘制地图层
     * @param target 渲染目标
     */
    void draw(sf::RenderTarget& target) const;

private:
    sf::Color colorForTile(const std::string& tileType) const;
    void generateDefaultMap();
    bool parseGroundJson(const std::string& path);
    void rebuildTileShapes();

    float                m_tileW;
    float                m_tileH;
    int                  m_width;
    int                  m_height;
    std::vector<std::string> m_tiles;
    std::vector<sf::RectangleShape> m_tileShapes;
};
