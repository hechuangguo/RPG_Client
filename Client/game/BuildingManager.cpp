/**
 * @file    BuildingManager.cpp
 * @brief   BuildingManager 实现
 */

#include "game/BuildingManager.h"

#include "util/PathUtil.h"
#include "util/TextUtil.h"

#include <fstream>
#include <sstream>

BuildingManager::BuildingManager()
{
    loadDefaults();
}

bool BuildingManager::load(uint32_t mapId)
{
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
    while ((pos = content.find("\"label\"", pos)) != std::string::npos)
    {
        BuildingEntry b{};
        b.width  = 2.f;
        b.height = 2.f;
        b.color  = sf::Color(120, 90, 70);

        const size_t labelColon = content.find(':', pos);
        const size_t q1 = content.find('"', labelColon + 1);
        const size_t q2 = content.find('"', q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
        {
            b.label = content.substr(q1 + 1, q2 - q1 - 1);
        }
        else
        {
            pos += 7;
            continue;
        }

        const size_t xPos = content.rfind("\"x\"", pos);
        if (xPos != std::string::npos && xPos > pos - 80)
        {
            b.x = std::stof(content.substr(content.find(':', xPos) + 1));
        }
        const size_t zPos = content.rfind("\"z\"", pos);
        if (zPos != std::string::npos && zPos > pos - 80)
        {
            b.z = std::stof(content.substr(content.find(':', zPos) + 1));
        }

        m_buildings.push_back(b);
        pos = q2 + 1;
    }

    if (m_buildings.empty())
    {
        loadDefaults();
    }
    return true;
}

void BuildingManager::draw(sf::RenderTarget& target, const sf::Font& font, float tileSize) const
{
    for (const BuildingEntry& b : m_buildings)
    {
        sf::RectangleShape rect({b.width * tileSize, b.height * tileSize});
        rect.setPosition(b.x * tileSize, b.z * tileSize);
        rect.setFillColor(b.color);
        rect.setOutlineColor(sf::Color(180, 150, 90));
        rect.setOutlineThickness(2.f);
        target.draw(rect);

        sf::Text sign(TextUtil::utf8ToSfString(b.label), font, 14);
        sign.setFillColor(sf::Color(255, 230, 180));
        sign.setPosition(b.x * tileSize + 4.f, b.z * tileSize - 18.f);
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
