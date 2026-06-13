/**
 * @file    EntityManager.cpp
 * @brief   EntityManager 实现
 */

#include "game/EntityManager.h"

#include <cmath>

EntityManager::EntityManager()
    : m_localUserId(0)
{
}

void EntityManager::setLocalPlayer(const Msg_S2C_EnterGame& enter)
{
    m_localUserId = enter.userID;
    GameEntity e{};
    e.id         = enter.userID;
    e.name       = enter.name;
    e.level      = enter.level;
    e.x          = enter.x;
    e.y          = enter.y;
    e.z          = enter.z;
    e.dir        = 0.f;
    e.entityType = 0;
    e.isLocal    = true;
    m_entities[e.id] = e;
}

void EntityManager::onSpawn(const Msg_S2C_SpawnEntity& spawn)
{
    if (spawn.entityID == m_localUserId)
    {
        return;
    }
    GameEntity e{};
    e.id         = spawn.entityID;
    e.name       = spawn.name;
    e.level      = spawn.level;
    e.x          = spawn.x;
    e.y          = spawn.y;
    e.z          = spawn.z;
    e.dir        = spawn.dir;
    e.entityType = spawn.entityType;
    e.isLocal    = false;
    m_entities[e.id] = e;
}

void EntityManager::onMove(const Msg_S2C_MoveNotify& move)
{
    auto it = m_entities.find(move.userID);
    if (it == m_entities.end())
    {
        return;
    }
    it->second.x   = move.x;
    it->second.y   = move.y;
    it->second.z   = move.z;
    it->second.dir = move.dir;
}

void EntityManager::onDespawn(const Msg_S2C_DespawnEntity& despawn)
{
    m_entities.erase(despawn.entityID);
}

void EntityManager::setLocalPosition(float x, float y, float z, float dir)
{
    auto it = m_entities.find(m_localUserId);
    if (it == m_entities.end())
    {
        return;
    }
    it->second.x   = x;
    it->second.y   = y;
    it->second.z   = z;
    it->second.dir = dir;
}

uint64_t EntityManager::localUserId() const
{
    return m_localUserId;
}

const GameEntity* EntityManager::localPlayer() const
{
    auto it = m_entities.find(m_localUserId);
    if (it == m_entities.end())
    {
        return nullptr;
    }
    return &it->second;
}

const std::unordered_map<uint64_t, GameEntity>& EntityManager::entities() const
{
    return m_entities;
}

void EntityManager::draw(sf::RenderTarget& target, const sf::Font& font, float tileSize) const
{
    for (const auto& pair : m_entities)
    {
        const GameEntity& e = pair.second;
        const float screenX = e.x * tileSize;
        const float screenY = e.z * tileSize;

        sf::RectangleShape body({tileSize * 0.7f, tileSize * 0.9f});
        body.setOrigin(body.getSize().x / 2.f, body.getSize().y);
        body.setPosition(screenX, screenY);
        body.setFillColor(colorForType(e.entityType));
        if (e.isLocal)
        {
            body.setOutlineColor(sf::Color(255, 220, 100));
            body.setOutlineThickness(2.f);
        }
        target.draw(body);

        sf::Text name(e.name, font, 12);
        name.setFillColor(sf::Color::White);
        name.setPosition(screenX - 20.f, screenY - tileSize - 16.f);
        target.draw(name);
    }
}

sf::Color EntityManager::colorForType(uint8_t entityType) const
{
    switch (entityType)
    {
    case 0: return sf::Color(100, 160, 220);
    case 1: return sf::Color(180, 140, 80);
    case 2: return sf::Color(200, 60, 60);
    default: return sf::Color(150, 150, 150);
    }
}
