/**
 * @file    CharacterSprite.h
 * @brief   2D 仙侠风角色精灵
 */

#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>

class CharacterSprite
{
public:
    CharacterSprite();

    bool load(uint8_t vocation, uint8_t sex);
    void draw(sf::RenderTarget& target,
              float screenX,
              float screenY,
              float dirRad,
              bool moving,
              float animTime,
              float scale = 1.f) const;

    float drawHeight(float scale = 1.f) const;
    const std::string& loadSource() const { return m_loadSource; }

private:
    static int directionFromRad(float dirRad);
    bool loadFromPng(const std::string& path);
    bool loadAnimJson(const std::string& path);
    void generateProceduralSheet(uint8_t vocation, uint8_t sex);
    std::string pngPathFor(uint8_t vocation, uint8_t sex) const;

    void drawFrame(sf::RenderTarget& target,
                   float screenX,
                   float screenY,
                   int direction,
                   int frame,
                   float scale) const;

    sf::Texture m_texture;
    int         m_frameW;
    int         m_frameH;
    int         m_frameCount;
    float       m_fps;
    float       m_drawScale;
    bool        m_ready;
    std::string m_loadSource;
};

class CharacterSpriteLibrary
{
public:
    static CharacterSpriteLibrary& instance();

    void preload();
    CharacterSprite& get(uint8_t vocation, uint8_t sex);
    const CharacterSprite& get(uint8_t vocation, uint8_t sex) const;

private:
    CharacterSpriteLibrary() = default;
    CharacterSprite m_warriorMale;
    CharacterSprite m_warriorFemale;
    CharacterSprite m_mageMale;
    CharacterSprite m_mageFemale;
    bool            m_init = false;

    void ensureLoaded();
};
