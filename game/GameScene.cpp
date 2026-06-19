/**
 * @file    GameScene.cpp
 * @brief   GameScene 实现
 */

#include "game/GameScene.h"

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
{
}

void GameScene::enter(const Msg_S2C_EnterGame& enter,
                      UiTheme* theme,
                      GameSession* session,
                      QuestModel* quests)
{
    m_theme      = theme;
    m_session    = session;
    m_quests     = quests;
    m_playerInfo = enter;
    m_active     = true;

    m_map.load(enter.mapID);
    m_water.load(enter.mapID);
    m_ambient.load(enter.mapID);
    m_buildings.load(enter.mapID);
    m_entities.setLocalPlayer(enter);

    m_worldView.setSize(static_cast<float>(m_viewSize.x), static_cast<float>(m_viewSize.y));
    m_uiView = m_worldView;
    updateCamera();
}

void GameScene::handleEvent(const sf::Event& event)
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
            break;
        case sf::Keyboard::S:
        case sf::Keyboard::Down:
            m_moveDown = true;
            break;
        case sf::Keyboard::A:
        case sf::Keyboard::Left:
            m_moveLeft = true;
            break;
        case sf::Keyboard::D:
        case sf::Keyboard::Right:
            m_moveRight = true;
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

    nx = std::max(0.f, std::min(nx, static_cast<float>(m_map.mapWidthTiles() - 1)));
    nz = std::max(0.f, std::min(nz, static_cast<float>(m_map.mapHeightTiles() - 1)));

    m_entities.setLocalPosition(nx, local->y, nz, dir);

    if (m_session && (m_moveUp || m_moveDown || m_moveLeft || m_moveRight))
    {
        m_session->sendMove(m_entities.localUserId(), nx, local->y, nz, dir, 1);
    }

    updateCamera();
}

void GameScene::draw(sf::RenderTarget& target)
{
    if (!m_active || !m_theme)
    {
        return;
    }

    target.setView(m_worldView);

    m_ambient.drawSky(target, m_viewSize);
    m_map.draw(target);
    m_water.draw(target, m_map.tileWidth());
    m_buildings.draw(target, m_theme->font(), m_map.tileWidth());
    m_entities.draw(target, m_theme->font(), m_map.tileWidth());
    m_ambient.drawWorld(target, m_theme->font(), m_map.tileWidth());

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

    const float tile = m_map.tileWidth();
    m_worldView.setCenter(local->x * tile, local->z * tile);
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

    std::string nameLine = std::string(m_playerInfo.name) + "  Lv." + std::to_string(m_playerInfo.level);
    sf::Text nameText(TextUtil::utf8ToSfString(nameLine), font, 16);
    nameText.setFillColor(m_theme->titleColor());
    nameText.setPosition(24.f, 20.f);
    target.draw(nameText);

    std::string hpLine = "HP " + std::to_string(m_playerInfo.hp) + "/" + std::to_string(m_playerInfo.maxHP);
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
