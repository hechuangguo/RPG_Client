/**
 * @file    WaterSystem.h
 * @brief   河道与水生动物系统
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <vector>

struct WaterCreature
{
    float     x, z;
    float     speed;
    float     phase;
    bool      isFish;
    sf::Color color;
};

class WaterSystem
{
public:
    WaterSystem();

    bool load(uint32_t mapId);
    void update(float dt);
    void draw(sf::RenderTarget& target, float tileW, float tileH) const;

private:
    void spawnDefaultCreatures();
    bool parseRiverJson(const std::string& path);

    float                   m_animTime;
    bool                    m_horizontal;
    int                     m_riverStart;
    int                     m_riverEnd;
    int                     m_riverCenter;
    int                     m_riverHalfWidth;
    int                     m_mapWidth;
    int                     m_mapHeight;
    std::vector<WaterCreature> m_creatures;
};
