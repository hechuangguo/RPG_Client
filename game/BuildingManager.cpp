/**
 * @file    BuildingManager.cpp
 * @brief   BuildingManager 实现
 */

#include "game/BuildingManager.h"

#include "util/PathUtil.h"
#include "util/TextUtil.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
const std::unordered_map<std::string, std::string> kBuildingLabels = {
    {"shop", u8"杂货铺"},
    {"auction", u8"拍卖行"},
    {"pawn", u8"当铺"},
    {"martial", u8"武行"},
    {"inn", u8"醉仙楼"},
    {"plaza", u8"中央广场"},
};

sf::Color colorForId(const std::string& id)
{
    if (id == "shop")
    {
        return sf::Color(120, 85, 60);
    }
    if (id == "auction")
    {
        return sf::Color(100, 85, 70);
    }
    if (id == "pawn")
    {
        return sf::Color(95, 75, 55);
    }
    if (id == "martial")
    {
        return sf::Color(110, 75, 70);
    }
    if (id == "inn")
    {
        return sf::Color(125, 95, 80);
    }
    if (id == "plaza")
    {
        return sf::Color(140, 110, 85);
    }
    return sf::Color(120, 90, 70);
}
}  // namespace

BuildingManager::BuildingManager()
{
    loadDefaults();
}

bool BuildingManager::loadBuildingTexture(const std::string& id, const std::string& path)
{
    sf::Texture tex;
    if (!tex.loadFromFile(path))
    {
        return false;
    }
    tex.setSmooth(false);
    m_textures[id] = std::move(tex);
    return true;
}

bool BuildingManager::load(uint32_t mapId)
{
    m_mapId = mapId;
    m_textures.clear();

    const std::string bldRoot =
        PathUtil::joinPath(PathUtil::mapPath(mapId), "buildings");
    const std::string path = PathUtil::joinPath(PathUtil::mapPath(mapId), "buildings.json");
    std::ifstream file(path);
    if (!file.is_open())
    {
        loadDefaults();
        return true;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string content = ss.str();

    m_buildings.clear();
    size_t pos = 0;
    while ((pos = content.find("\"id\"", pos)) != std::string::npos)
    {
        BuildingEntry b{};
        b.width  = 2.f;
        b.height = 2.f;
        b.color  = sf::Color(120, 90, 70);

        const size_t idColon = content.find(':', pos);
        const size_t q1 = content.find('"', idColon + 1);
        const size_t q2 = content.find('"', q1 + 1);
        if (q1 == std::string::npos || q2 == std::string::npos)
        {
            pos += 4;
            continue;
        }
        b.id = content.substr(q1 + 1, q2 - q1 - 1);
        auto labelIt = kBuildingLabels.find(b.id);
        b.label = labelIt != kBuildingLabels.end() ? labelIt->second : b.id;
        b.color = colorForId(b.id);

        const size_t xPos = content.find("\"x\"", pos);
        if (xPos != std::string::npos && xPos < pos + 200)
        {
            b.x = std::stof(content.substr(content.find(':', xPos) + 1));
        }
        const size_t zPos = content.find("\"z\"", pos);
        if (zPos != std::string::npos && zPos < pos + 200)
        {
            b.z = std::stof(content.substr(content.find(':', zPos) + 1));
        }
        const size_t wPos = content.find("\"w\"", pos);
        if (wPos != std::string::npos && wPos < pos + 200)
        {
            b.width = std::stof(content.substr(content.find(':', wPos) + 1));
        }
        const size_t hPos = content.find("\"h\"", pos);
        if (hPos != std::string::npos && hPos < pos + 200)
        {
            b.height = std::stof(content.substr(content.find(':', hPos) + 1));
        }

        const std::string texPath = PathUtil::joinPath(bldRoot, b.id + ".png");
        loadBuildingTexture(b.id, texPath);

        m_buildings.push_back(b);
        pos = q2 + 1;
    }

    if (m_buildings.empty())
    {
        loadDefaults();
    }
    return true;
}

void BuildingManager::draw(sf::RenderTarget& target,
                           const sf::Font& font,
                           float tileW,
                           float tileH) const
{
    for (const BuildingEntry& b : m_buildings)
    {
        const float px = b.x * tileW;
        const float py = b.z * tileH;
        const float pw = b.width * tileW;
        const float ph = b.height * tileH;

        auto texIt = m_textures.find(b.id);
        if (texIt != m_textures.end())
        {
            sf::Sprite sprite(texIt->second);
            const sf::Vector2u ts = texIt->second.getSize();
            sprite.setScale(pw / static_cast<float>(ts.x), ph / static_cast<float>(ts.y));
            sprite.setPosition(px, py - ph * 0.15f);
            target.draw(sprite);
        }
        else
        {
            sf::RectangleShape rect({pw, ph});
            rect.setPosition(px, py);
            rect.setFillColor(b.color);
            rect.setOutlineColor(sf::Color(200, 160, 90));
            rect.setOutlineThickness(2.f);
            target.draw(rect);

            sf::RectangleShape roof({pw, tileH * 0.4f});
            roof.setPosition(px, py - tileH * 0.25f);
            roof.setFillColor(sf::Color(
                static_cast<uint8_t>(b.color.r + 30),
                static_cast<uint8_t>(b.color.g + 20),
                static_cast<uint8_t>(b.color.b + 15)));
            target.draw(roof);
        }

        sf::Text sign(TextUtil::utf8ToSfString(b.label), font, 14);
        sign.setFillColor(sf::Color(255, 230, 180));
        sign.setPosition(px + 4.f, py - 22.f);
        target.draw(sign);
    }
}

const std::vector<BuildingEntry>& BuildingManager::buildings() const
{
    return m_buildings;
}

void BuildingManager::loadDefaults()
{
    m_buildings = {
        {"shop", u8"杂货铺", 5.f, 6.f, 2.f, 2.f, sf::Color(110, 80, 60)},
        {"auction", u8"拍卖行", 28.f, 8.f, 2.5f, 2.f, sf::Color(100, 85, 70)},
        {"pawn", u8"当铺", 32.f, 14.f, 2.f, 2.f, sf::Color(95, 75, 55)},
        {"martial", u8"武行", 10.f, 22.f, 2.5f, 2.2f, sf::Color(105, 70, 65)},
        {"inn", u8"醉仙楼", 20.f, 18.f, 3.f, 2.f, sf::Color(115, 85, 75)},
        {"plaza", u8"中央广场", 16.f, 12.f, 3.f, 1.5f, sf::Color(130, 100, 80)},
    };
}
