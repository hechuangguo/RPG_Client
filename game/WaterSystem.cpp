/**
 * @file    WaterSystem.cpp
 * @brief   WaterSystem 实现
 */

#include "game/WaterSystem.h"

#include "math/Random.h"
#include "util/PathUtil.h"

#include <cmath>
#include <fstream>
#include <sstream>

WaterSystem::WaterSystem()
    : m_animTime(0.f)
    , m_horizontal(true)
    , m_riverStart(5)
    , m_riverEnd(35)
    , m_riverCenter(15)
    , m_riverHalfWidth(2)
    , m_mapWidth(40)
    , m_mapHeight(30)
{
}

bool WaterSystem::parseRiverJson(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    const size_t riverPos = content.find("\"river\"");
    if (riverPos != std::string::npos)
    {
        const size_t brace = content.find('{', riverPos);
        const size_t end   = content.find('}', brace);
        if (brace != std::string::npos && end != std::string::npos && end > brace)
        {
            content = content.substr(brace, end - brace + 1);
        }
    }

    auto readInt = [&](const std::string& key, int& out) -> bool {
        const size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos)
        {
            return false;
        }
        const size_t colon = content.find(':', pos);
        if (colon == std::string::npos)
        {
            return false;
        }
        out = std::stoi(content.substr(colon + 1));
        return true;
    };

    int y = 15;
    int width = 4;
    int xStart = 5;
    int xEnd = 35;
    readInt("y", y);
    readInt("width", width);
    readInt("xStart", xStart);
    readInt("xEnd", xEnd);

    m_horizontal       = true;
    m_riverCenter      = y;
    m_riverHalfWidth   = std::max(1, width / 2);
    m_riverStart       = xStart;
    m_riverEnd         = xEnd;
    return true;
}

bool WaterSystem::load(uint32_t mapId)
{
    const std::string path = PathUtil::joinPath(PathUtil::mapPath(mapId), "river.json");
    if (!parseRiverJson(path))
    {
        m_horizontal     = true;
        m_riverCenter  = 15;
        m_riverHalfWidth = 2;
        m_riverStart   = 5;
        m_riverEnd     = 35;
    }
    spawnDefaultCreatures();
    return true;
}

void WaterSystem::update(float dt)
{
    m_animTime += dt;
    for (WaterCreature& c : m_creatures)
    {
        c.x += c.speed * dt;
        c.phase += dt * 2.f;
        if (c.x > static_cast<float>(m_riverEnd) + 1.f)
        {
            c.x = static_cast<float>(m_riverStart);
        }
    }
}

void WaterSystem::draw(sf::RenderTarget& target, float tileW, float tileH) const
{
    const float wave = std::sin(m_animTime * 2.f) * 0.5f + 0.5f;
    const sf::Color waterColor(40, 120, 140, static_cast<uint8_t>(160 + wave * 40));

    if (m_horizontal)
    {
        const float riverTop =
            static_cast<float>(m_riverCenter - m_riverHalfWidth) * tileH;
        const float riverH =
            static_cast<float>(m_riverHalfWidth * 2 + 1) * tileH;
        sf::RectangleShape strip(
            {static_cast<float>(m_riverEnd - m_riverStart + 1) * tileW, riverH});
        strip.setPosition(static_cast<float>(m_riverStart) * tileW, riverTop);
        strip.setFillColor(waterColor);
        target.draw(strip);
    }

    for (const WaterCreature& c : m_creatures)
    {
        sf::CircleShape body(c.isFish ? 6.f : 4.f);
        body.setFillColor(c.color);
        body.setOrigin(body.getRadius(), body.getRadius());
        body.setPosition(c.x * tileW, c.z * tileH + std::sin(c.phase) * 3.f);
        target.draw(body);
    }
}

void WaterSystem::spawnDefaultCreatures()
{
    m_creatures.clear();
    for (int i = 0; i < 8; ++i)
    {
        WaterCreature c{};
        c.x      = static_cast<float>(Random::randInt(m_riverStart, m_riverEnd));
        c.z      = static_cast<float>(m_riverCenter) + Random::randFloat(-0.8f, 0.8f);
        c.speed  = Random::randFloat(0.5f, 1.5f);
        c.phase  = Random::randFloat(0.f, 6.28f);
        c.isFish = i < 5;
        c.color  = c.isFish ? sf::Color(220, 120, 80) : sf::Color(200, 80, 100);
        m_creatures.push_back(c);
    }
}
