/**
 * @file    MapRenderer.h
 * @brief   地图瓦片渲染器
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 地图地面渲染
 */
class MapRenderer
{
public:
    MapRenderer();

    bool load(uint32_t mapId);

    float tileWidth() const;
    float tileHeight() const;
    int mapWidthTiles() const;
    int mapHeightTiles() const;

    void draw(sf::RenderTarget& target) const;
    void drawBackdrop(sf::RenderTarget& target, const sf::FloatRect& viewBounds) const;

    /** @brief 指定格子的 tile 类型（越界返回空串） */
    std::string tileTypeAt(int x, int z) const;

    /** @brief 是否可走（非 water） */
    bool isWalkableTile(int x, int z) const;

private:
    sf::Color colorForTile(const std::string& tileType) const;
    void generateDefaultMap();
    void generateCityMap(float grassRatio);
    bool parseGroundJson(const std::string& path);
    bool loadTileset(uint32_t mapId);
    int tilesetIndexFor(const std::string& tileType) const;
    void rebuildTiles();

    struct TileVisual
    {
        sf::RectangleShape base;
        std::vector<sf::RectangleShape> decor;

        TileVisual(float w, float h) : base({w, h}) {}
    };

    void addTileDecor(TileVisual& visual, const std::string& tileType, int x, int y) const;

    float                m_tileW;
    float                m_tileH;
    int                  m_width;
    int                  m_height;
    std::vector<std::string> m_tiles;

    bool                 m_useTileset;
    sf::Texture          m_tilesetTexture;
    std::vector<sf::Sprite> m_tileSprites;
    std::vector<TileVisual> m_fallbackShapes;
};
