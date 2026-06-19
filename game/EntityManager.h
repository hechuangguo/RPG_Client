/**
 * @file    EntityManager.h
 * @brief   游戏实体管理器
 */

#pragma once

#include "ClientProtocol.h"

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct GameEntity
{
    uint64_t    id;
    std::string name;
    uint32_t    level;
    float       x, y, z;
    float       dir;
    uint8_t     entityType;
    uint8_t     vocation;
    uint8_t     sex;
    bool        isLocal;
    bool        moving;
    float       animTime;
};

class EntityManager
{
public:
    EntityManager();

    void setLocalPlayer(const Msg_S2C_EnterGame& enter);
    void setLocalAppearance(uint8_t vocation, uint8_t sex);
    void setLocalMoving(bool moving);
    void onSpawn(const Msg_S2C_SpawnEntity& spawn);
    void onMove(const Msg_S2C_MoveNotify& move);
    void onDespawn(const Msg_S2C_DespawnEntity& despawn);
    void setLocalPosition(float x, float y, float z, float dir);
    void update(float dt);

    uint64_t localUserId() const;
    const GameEntity* localPlayer() const;
    const std::unordered_map<uint64_t, GameEntity>& entities() const;

    void draw(sf::RenderTarget& target,
              const sf::Font& font,
              float tileW,
              float tileH) const;

private:
    sf::Color colorForType(uint8_t entityType) const;
    void drawEntity(sf::RenderTarget& target,
                    const sf::Font& font,
                    const GameEntity& e,
                    float tileW,
                    float tileH) const;

    uint64_t m_localUserId;
    uint8_t  m_localVocation;
    uint8_t  m_localSex;
    std::unordered_map<uint64_t, GameEntity> m_entities;
};
