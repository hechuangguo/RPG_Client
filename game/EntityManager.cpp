/**
 * @file    EntityManager.cpp
 * @brief   EntityManager 实现
 */

#include "game/EntityManager.h"

#include "game/CharacterSprite.h"
#include "net/CharacterTypes.h"
#include "util/TextUtil.h"

#include <algorithm>
#include <cmath>
#include <vector>

EntityManager::EntityManager()
    : m_localUserId(0)
    , m_localVocation(CharacterDef::kVocationWarrior)
    , m_localSex(CharacterDef::kSexMale)
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
    e.vocation   = m_localVocation;
    e.sex        = m_localSex;
    e.isLocal    = true;
    e.moving     = false;
    e.animTime   = 0.f;
    m_entities[e.id] = e;
}

void EntityManager::setLocalAppearance(uint8_t vocation, uint8_t sex)
{
    m_localVocation = vocation;
    m_localSex      = sex;
    auto it         = m_entities.find(m_localUserId);
    if (it != m_entities.end())
    {
        it->second.vocation = vocation;
        it->second.sex      = sex;
    }
}

void EntityManager::setLocalMoving(bool moving)
{
    auto it = m_entities.find(m_localUserId);
    if (it != m_entities.end())
    {
        it->second.moving = moving;
    }
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
    e.vocation   = CharacterDef::kVocationWarrior;
    e.sex        = CharacterDef::kSexMale;
    e.isLocal    = false;
    e.moving     = false;
    e.animTime   = 0.f;
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

void EntityManager::update(float dt)
{
    auto it = m_entities.find(m_localUserId);
    if (it == m_entities.end())
    {
        return;
    }
    if (it->second.moving)
    {
        it->second.animTime += dt;
    }
    else
    {
        it->second.animTime = 0.f;
    }
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

void EntityManager::draw(sf::RenderTarget& target,
                         const sf::Font& font,
                         float tileW,
                         float tileH) const
{
    std::vector<const GameEntity*> sorted;
    sorted.reserve(m_entities.size());
    for (const auto& pair : m_entities)
    {
        sorted.push_back(&pair.second);
    }
    std::sort(sorted.begin(), sorted.end(), [](const GameEntity* a, const GameEntity* b) {
        if (a->z != b->z)
        {
            return a->z < b->z;
        }
        return a->y < b->y;
    });

    for (const GameEntity* entity : sorted)
    {
        drawEntity(target, font, *entity, tileW, tileH);
    }
}

void EntityManager::drawEntity(sf::RenderTarget& target,
                               const sf::Font& font,
                               const GameEntity& e,
                               float tileW,
                               float tileH) const
{
    const float screenX = e.x * tileW + tileW * 0.5f;
    const float screenY = e.z * tileH + tileH;

    if (e.entityType == 0)
    {
        if (e.isLocal)
        {
            sf::CircleShape shadow(10.f);
            shadow.setOrigin(10.f, 5.f);
            shadow.setScale(1.2f, 0.5f);
            shadow.setPosition(screenX, screenY - 2.f);
            shadow.setFillColor(sf::Color(0, 0, 0, 80));
            target.draw(shadow);
        }

        const CharacterSprite& sprite =
            CharacterSpriteLibrary::instance().get(e.vocation, e.sex);
        const float npcScale = e.isLocal ? 1.f : 1.f;
        sprite.draw(target, screenX, screenY, e.dir, e.moving, e.animTime, npcScale);

        if (e.isLocal)
        {
            const float spriteH = sprite.drawHeight(npcScale);
            sf::RectangleShape outline(
                {static_cast<float>(tileW) * 0.55f, spriteH * 0.55f});
            outline.setOrigin(outline.getSize().x / 2.f, outline.getSize().y);
            outline.setPosition(screenX, screenY);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color(255, 220, 100, 160));
            outline.setOutlineThickness(1.5f);
            target.draw(outline);
        }
    }
    else
    {
        sf::RectangleShape body({tileW * 0.7f, tileH * 0.9f});
        body.setOrigin(body.getSize().x / 2.f, body.getSize().y);
        body.setPosition(screenX, screenY);
        body.setFillColor(colorForType(e.entityType));
        target.draw(body);
    }

    sf::Text name(TextUtil::utf8ToSfString(e.name), font, 12);
    name.setFillColor(sf::Color::White);
    if (e.entityType == 0)
    {
        const CharacterSprite& sprite =
            CharacterSpriteLibrary::instance().get(e.vocation, e.sex);
        const float nameY = screenY - sprite.drawHeight(1.f) - 8.f;
        name.setPosition(screenX - 20.f, nameY);
    }
    else
    {
        name.setPosition(screenX - 20.f, screenY - tileH - 16.f);
    }
    target.draw(name);
}

sf::Color EntityManager::colorForType(uint8_t entityType) const
{
    switch (entityType)
    {
    case 0:
        return sf::Color(100, 160, 220);
    case 1:
        return sf::Color(180, 140, 80);
    case 2:
        return sf::Color(200, 60, 60);
    default:
        return sf::Color(150, 150, 150);
    }
}
