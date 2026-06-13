/**
 * @file    WaterSystem.cpp
 * @brief   WaterSystem 实现
 */

#include "game/WaterSystem.h"

#include "math/Random.h"
#include "util/PathUtil.h"

#include <cmath>

WaterSystem::WaterSystem()
    : m_animTime(0.f)
    , m_riverStartCol(18)
    , m_riverEndCol(22)
    , m_mapHeight(30)
{
}

bool WaterSystem::load(uint32_t mapId)
{
    (void)mapId;
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
        if (c.x > static_cast<float>(m_riverEndCol) + 1.f)
        {
            c.x = static_cast<float>(m_riverStartCol);
        }
    }
}

void WaterSystem::draw(sf::RenderTarget& target, float tileSize) const
{
    const float wave = std::sin(m_animTime * 2.f) * 0.5f + 0.5f;
    const sf::Color waterColor(40, 120, 140, static_cast<uint8_t>(160 + wave * 40));

    for (int row = 0; row < m_mapHeight; ++row)
    {
        sf::RectangleShape strip({static_cast<float>(m_riverEndCol - m_riverStartCol + 1) * tileSize,
                                  tileSize});
        strip.setPosition(static_cast<float>(m_riverStartCol) * tileSize,
                          static_cast<float>(row) * tileSize);
        strip.setFillColor(waterColor);
        target.draw(strip);
    }

    for (const WaterCreature& c : m_creatures)
    {
        sf::CircleShape body(c.isFish ? 6.f : 4.f);
        body.setFillColor(c.color);
        body.setOrigin(body.getRadius(), body.getRadius());
        body.setPosition(c.x * tileSize, c.z * tileSize + std::sin(c.phase) * 3.f);
        target.draw(body);
    }
}

void WaterSystem::spawnDefaultCreatures()
{
    m_creatures.clear();
    for (int i = 0; i < 8; ++i)
    {
        WaterCreature c{};
        c.x      = static_cast<float>(Random::randInt(m_riverStartCol, m_riverEndCol));
        c.z      = static_cast<float>(Random::randInt(2, m_mapHeight - 2));
        c.speed  = Random::randFloat(0.5f, 1.5f);
        c.phase  = Random::randFloat(0.f, 6.28f);
        c.isFish = i < 5;
        c.color  = c.isFish ? sf::Color(220, 120, 80) : sf::Color(200, 80, 100);
        m_creatures.push_back(c);
    }
}
