/**
 * @file    GameScene.cpp
 * @brief   GameScene 实现
 */

#include "game/GameScene.h"

#include "game/CharacterSprite.h"
#include "game/QuestModel.h"
#include "net/GameSession.h"
#include "ui/UiTheme.h"
#include "util/TextUtil.h"

#include <algorithm>
#include <cmath>

GameScene::GameScene()
    : m_theme(nullptr)
    , m_session(nullptr)
    , m_quests(nullptr)
    , m_viewSize(1280, 720)
    , m_active(false)
    , m_moveUp(false)
    , m_moveDown(false)
    , m_moveLeft(false)
    , m_moveRight(false)
    , m_moveSpeed(4.f)
    , m_hasClickTarget(false)
    , m_targetX(0.f)
    , m_targetZ(0.f)
    , m_clickMarkerTime(0.f)
{
}

void GameScene::enter(const Msg_S2C_EnterGame& enter,
                      UiTheme* theme,
                      GameSession* session,
                      QuestModel* quests,
                      uint8_t localVocation,
                      uint8_t localSex)
{
    m_theme      = theme;
    m_session    = session;
    m_quests     = quests;
    m_playerInfo = enter;
    m_active     = true;

    CharacterSpriteLibrary::instance().preload();
    m_map.load(enter.mapID);
    m_water.load(enter.mapID);
    m_ambient.load(enter.mapID);
    m_buildings.load(enter.mapID);
    m_entities.setLocalPlayer(enter);
    m_entities.setLocalAppearance(localVocation, localSex);

    m_worldView.setSize(static_cast<float>(m_viewSize.x), static_cast<float>(m_viewSize.y));
    m_uiView.setSize(static_cast<float>(m_viewSize.x), static_cast<float>(m_viewSize.y));
    m_uiView.setCenter(static_cast<float>(m_viewSize.x) / 2.f,
                       static_cast<float>(m_viewSize.y) / 2.f);
    m_hasClickTarget  = false;
    m_clickMarkerTime = 0.f;
    updateCamera();
}

bool GameScene::trySetClickTarget(float tileX, float tileZ)
{
    int tx = static_cast<int>(std::floor(tileX));
    int tz = static_cast<int>(std::floor(tileZ));

    if (!m_map.isWalkableTile(tx, tz))
    {
        bool found = false;
        for (int radius = 1; radius <= 3 && !found; ++radius)
        {
            for (int ox = -radius; ox <= radius && !found; ++ox)
            {
                for (int oz = -radius; oz <= radius; ++oz)
                {
                    if (std::abs(ox) != radius && std::abs(oz) != radius)
                    {
                        continue;
                    }
                    if (m_map.isWalkableTile(tx + ox, tz + oz))
                    {
                        tx += ox;
                        tz += oz;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found)
        {
            return false;
        }
    }

    m_targetX         = static_cast<float>(tx) + 0.5f;
    m_targetZ         = static_cast<float>(tz) + 0.5f;
    m_hasClickTarget  = true;
    m_clickMarkerTime = 0.f;
    return true;
}

void GameScene::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_active)
    {
        return;
    }

    if (event.type == sf::Event::KeyPressed)
    {
        switch (event.key.code)
        {
        case sf::Keyboard::W:
        case sf::Keyboard::Up:
            m_moveUp = true;
            m_hasClickTarget = false;
            break;
        case sf::Keyboard::S:
        case sf::Keyboard::Down:
            m_moveDown = true;
            m_hasClickTarget = false;
            break;
        case sf::Keyboard::A:
        case sf::Keyboard::Left:
            m_moveLeft = true;
            m_hasClickTarget = false;
            break;
        case sf::Keyboard::D:
        case sf::Keyboard::Right:
            m_moveRight = true;
            m_hasClickTarget = false;
            break;
        default:
            break;
        }
    }
    else if (event.type == sf::Event::KeyReleased)
    {
        switch (event.key.code)
        {
        case sf::Keyboard::W:
        case sf::Keyboard::Up:
            m_moveUp = false;
            break;
        case sf::Keyboard::S:
        case sf::Keyboard::Down:
            m_moveDown = false;
            break;
        case sf::Keyboard::A:
        case sf::Keyboard::Left:
            m_moveLeft = false;
            break;
        case sf::Keyboard::D:
        case sf::Keyboard::Right:
            m_moveRight = false;
            break;
        default:
            break;
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed &&
             event.mouseButton.button == sf::Mouse::Right)
    {
        const sf::Vector2f world = window.mapPixelToCoords(
            sf::Vector2i(event.mouseButton.x, event.mouseButton.y),
            m_worldView);
        const float tileW = m_map.tileWidth();
        const float tileH = m_map.tileHeight();
        float tileX = world.x / tileW;
        float tileZ = world.y / tileH;
        tileX = std::max(0.f, std::min(tileX, static_cast<float>(m_map.mapWidthTiles() - 1)));
        tileZ = std::max(0.f, std::min(tileZ, static_cast<float>(m_map.mapHeightTiles() - 1)));
        trySetClickTarget(tileX, tileZ);
    }
}

void GameScene::update(float dt)
{
    if (!m_active)
    {
        return;
    }

    m_water.update(dt);
    m_ambient.update(dt, m_viewSize);

    const GameEntity* local = m_entities.localPlayer();
    if (!local)
    {
        return;
    }

    float nx = local->x;
    float nz = local->z;
    float dir = local->dir;
    bool moving = m_moveUp || m_moveDown || m_moveLeft || m_moveRight;

    if (moving)
    {
        m_hasClickTarget = false;
    }

    if (moving)
    {
        if (m_moveUp)
        {
            nz -= m_moveSpeed * dt;
            dir = -1.5707963f;
        }
        if (m_moveDown)
        {
            nz += m_moveSpeed * dt;
            dir = 1.5707963f;
        }
        if (m_moveLeft)
        {
            nx -= m_moveSpeed * dt;
            dir = 3.1415926f;
        }
        if (m_moveRight)
        {
            nx += m_moveSpeed * dt;
            dir = 0.f;
        }
    }
    else if (m_hasClickTarget)
    {
        const float dx = m_targetX - nx;
        const float dz = m_targetZ - nz;
        const float dist = std::sqrt(dx * dx + dz * dz);
        constexpr float kArriveDist = 0.15f;

        if (dist < kArriveDist)
        {
            nx               = m_targetX;
            nz               = m_targetZ;
            m_hasClickTarget = false;
        }
        else
        {
            const float step = m_moveSpeed * dt;
            const float move = std::min(step, dist);
            nx += dx / dist * move;
            nz += dz / dist * move;
            dir = std::atan2(dz, dx);
            moving = true;
        }
    }

    if (m_hasClickTarget)
    {
        m_clickMarkerTime += dt;
    }

    nx = std::max(0.f, std::min(nx, static_cast<float>(m_map.mapWidthTiles() - 1)));
    nz = std::max(0.f, std::min(nz, static_cast<float>(m_map.mapHeightTiles() - 1)));

    m_entities.setLocalPosition(nx, local->y, nz, dir);
    m_entities.setLocalMoving(moving);

    if (m_session && moving)
    {
        m_session->sendMove(m_entities.localUserId(), nx, local->y, nz, dir, 1);
    }

    m_entities.update(dt);
    updateCamera();
}

void GameScene::draw(sf::RenderTarget& target)
{
    if (!m_active || !m_theme)
    {
        return;
    }

    target.setView(m_uiView);
    m_ambient.drawSky(target, m_viewSize);

    target.setView(m_worldView);

    const sf::Vector2f viewCenter = m_worldView.getCenter();
    const sf::Vector2f viewSize   = m_worldView.getSize();
    const sf::FloatRect worldViewRect(viewCenter.x - viewSize.x / 2.f,
                                      viewCenter.y - viewSize.y / 2.f,
                                      viewSize.x,
                                      viewSize.y);

    m_map.drawBackdrop(target, worldViewRect);
    m_map.draw(target);
    m_water.draw(target, m_map.tileWidth(), m_map.tileHeight());
    m_buildings.draw(target, m_theme->font(), m_map.tileWidth(), m_map.tileHeight());
    m_entities.draw(target,
                  m_theme->font(),
                  m_map.tileWidth(),
                  m_map.tileHeight());
    drawClickMarker(target);
    m_ambient.drawWorld(target, m_theme->font(), m_map.tileWidth(), m_map.tileHeight());

    target.setView(m_uiView);
    drawHud(target);
}

void GameScene::setViewSize(const sf::Vector2u& size)
{
    m_viewSize = size;
    m_worldView.setSize(static_cast<float>(size.x), static_cast<float>(size.y));
    m_uiView.setSize(static_cast<float>(size.x), static_cast<float>(size.y));
    m_uiView.setCenter(static_cast<float>(size.x) / 2.f, static_cast<float>(size.y) / 2.f);
}

void GameScene::onSpawn(const Msg_S2C_SpawnEntity& spawn)
{
    m_entities.onSpawn(spawn);
}

void GameScene::onMove(const Msg_S2C_MoveNotify& move)
{
    m_entities.onMove(move);
}

void GameScene::onDespawn(const Msg_S2C_DespawnEntity& despawn)
{
    m_entities.onDespawn(despawn);
}

void GameScene::leave()
{
    m_active    = false;
    m_moveUp    = false;
    m_moveDown  = false;
    m_moveLeft  = false;
    m_moveRight = false;
    m_hasClickTarget  = false;
    m_clickMarkerTime = 0.f;
    m_session   = nullptr;
    m_quests    = nullptr;
    m_entities  = EntityManager{};
    m_playerInfo = Msg_S2C_EnterGame{};
}

void GameScene::updateCamera()
{
    const GameEntity* local = m_entities.localPlayer();
    if (!local)
    {
        return;
    }

    const float tileW = m_map.tileWidth();
    const float tileH = m_map.tileHeight();
    const float mapPxW = static_cast<float>(m_map.mapWidthTiles()) * tileW;
    const float mapPxH = static_cast<float>(m_map.mapHeightTiles()) * tileH;
    const float halfW  = static_cast<float>(m_viewSize.x) * 0.5f;
    const float halfH  = static_cast<float>(m_viewSize.y) * 0.5f;

    float cx = local->x * tileW + tileW * 0.5f;
    float cz = local->z * tileH + tileH * 0.5f;

    const float minCx = halfW;
    const float maxCx = std::max(halfW, mapPxW - halfW);
    const float minCz = halfH;
    const float maxCz = std::max(halfH, mapPxH - halfH);

    cx = std::max(minCx, std::min(cx, maxCx));
    cz = std::max(minCz, std::min(cz, maxCz));

    m_worldView.setCenter(cx, cz);
}

void GameScene::drawHud(sf::RenderTarget& target) const
{
    const sf::Font& font = m_theme->font();

    sf::RectangleShape bar({320.f, 72.f});
    bar.setPosition(12.f, 12.f);
    bar.setFillColor(sf::Color(0, 0, 0, 140));
    bar.setOutlineColor(m_theme->panelBorder());
    bar.setOutlineThickness(1.f);
    target.draw(bar);

    std::string nameLine =
        std::string(m_playerInfo.name) + "  Lv." + std::to_string(m_playerInfo.level);
    sf::Text nameText(TextUtil::utf8ToSfString(nameLine), font, 16);
    nameText.setFillColor(m_theme->titleColor());
    nameText.setPosition(24.f, 20.f);
    target.draw(nameText);

    std::string hpLine =
        "HP " + std::to_string(m_playerInfo.hp) + "/" + std::to_string(m_playerInfo.maxHP);
    sf::Text hpText(TextUtil::utf8ToSfString(hpLine), font, 14);
    hpText.setFillColor(m_theme->textColor());
    hpText.setPosition(24.f, 44.f);
    target.draw(hpText);

    if (m_quests)
    {
        sf::Text questText(TextUtil::utf8ToSfString(m_quests->primarySummary()), font, 14);
        questText.setFillColor(m_theme->accentColor());
        questText.setPosition(24.f, static_cast<float>(m_viewSize.y) - 36.f);
        target.draw(questText);
    }
}

void GameScene::drawClickMarker(sf::RenderTarget& target) const
{
    if (!m_hasClickTarget || m_clickMarkerTime > 0.5f)
    {
        return;
    }

    const float tileW = m_map.tileWidth();
    const float tileH = m_map.tileHeight();
    const float cx    = m_targetX * tileW + tileW * 0.5f;
    const float cy    = m_targetZ * tileH + tileH * 0.5f;
    const float alpha = 200.f * (1.f - m_clickMarkerTime / 0.5f);

    sf::CircleShape ring(tileW * 0.35f);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(cx, cy);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(255, 220, 100, static_cast<uint8_t>(alpha)));
    ring.setOutlineThickness(2.f);
    target.draw(ring);
}
