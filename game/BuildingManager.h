/**
 * @file    BuildingManager.h
 * @brief   主城建筑管理器
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct BuildingEntry
{
    std::string id;
    std::string label;
    float       x, z;
    float       width, height;
    sf::Color   color;
};

class BuildingManager
{
public:
    BuildingManager();

    bool load(uint32_t mapId);
    void draw(sf::RenderTarget& target, const sf::Font& font, float tileW, float tileH) const;

    const std::vector<BuildingEntry>& buildings() const;

private:
    void loadDefaults();
    bool loadBuildingTexture(const std::string& id, const std::string& path);

    std::vector<BuildingEntry> m_buildings;
    std::unordered_map<std::string, sf::Texture> m_textures;
    uint32_t m_mapId = 0;
};
