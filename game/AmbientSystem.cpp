/**
 * @file    AmbientSystem.cpp
 * @brief   AmbientSystem 实现
 */

#include "game/AmbientSystem.h"

#include "math/Random.h"
#include "util/PathUtil.h"
#include "util/TextUtil.h"

AmbientSystem::AmbientSystem()
{
    setupDefaults();
}

bool AmbientSystem::load(uint32_t mapId)
{
    const std::string path = PathUtil::joinPath(PathUtil::mapPath(mapId), "ambient.json");
    (void)path;
    setupDefaults();
    return true;
}

void AmbientSystem::update(float dt, const sf::Vector2u& viewSize)
{
    const float width = static_cast<float>(viewSize.x);

    for (CloudLayer& c : m_clouds)
    {
        c.x += c.speed * dt;
        if (c.x > width + 200.f)
        {
            c.x = -200.f;
        }
    }

    for (Bird& b : m_birds)
    {
        b.x += b.speed * dt;
        if (b.x > width + 50.f)
        {
            b.x = -50.f;
            b.y = static_cast<float>(Random::randInt(40, 120));
        }
    }

    for (AmbientNpc& n : m_npcs)
    {
        n.z += static_cast<float>(n.dir) * n.speed * dt;
        if (n.z < n.minZ)
        {
            n.dir = 1;
        }
        else if (n.z > n.maxZ)
        {
            n.dir = -1;
        }
    }
}

void AmbientSystem::drawSky(sf::RenderTarget& target, const sf::Vector2u& viewSize) const
{
    const float h = static_cast<float>(viewSize.y);

    sf::RectangleShape sky({static_cast<float>(viewSize.x), h * 0.45f});
    sky.setFillColor(sf::Color(135, 190, 220));
    target.draw(sky);

    sf::RectangleShape warm({static_cast<float>(viewSize.x), h * 0.25f});
    warm.setPosition(0.f, h * 0.2f);
    warm.setFillColor(sf::Color(255, 230, 180, 80));
    target.draw(warm);

    for (const CloudLayer& c : m_clouds)
    {
        sf::CircleShape cloud(40.f * c.scale);
        cloud.setFillColor(c.color);
        cloud.setPosition(c.x, c.y);
        target.draw(cloud);

        sf::CircleShape cloud2(30.f * c.scale);
        cloud2.setFillColor(c.color);
        cloud2.setPosition(c.x + 35.f, c.y + 8.f);
        target.draw(cloud2);
    }

    for (const Bird& b : m_birds)
    {
        sf::CircleShape bird(3.f);
        bird.setFillColor(sf::Color(40, 40, 40));
        bird.setPosition(b.x, b.y);
        target.draw(bird);
    }
}

void AmbientSystem::drawWorld(sf::RenderTarget& target, const sf::Font& font, float tileSize) const
{
    for (const AmbientNpc& n : m_npcs)
    {
        sf::RectangleShape body({tileSize * 0.5f, tileSize * 0.7f});
        body.setOrigin(body.getSize().x / 2.f, body.getSize().y);
        body.setPosition(n.x * tileSize, n.z * tileSize);
        body.setFillColor(sf::Color(160, 130, 100));
        target.draw(body);

        sf::Text label(TextUtil::utf8ToSfString(n.label), font, 11);
        label.setFillColor(sf::Color(230, 220, 200));
        label.setPosition(n.x * tileSize - 20.f, n.z * tileSize - tileSize);
        target.draw(label);
    }
}

void AmbientSystem::setupDefaults()
{
    m_clouds = {
        {0.f, 8.f, 40.f, 1.2f, sf::Color(255, 255, 255, 120)},
        {200.f, 15.f, 80.f, 0.9f, sf::Color(255, 255, 255, 90)},
        {400.f, 5.f, 30.f, 1.5f, sf::Color(255, 255, 255, 100)},
    };

    m_birds.clear();
    for (int i = 0; i < 3; ++i)
    {
        Bird b{};
        b.x     = static_cast<float>(Random::randInt(0, 800));
        b.y     = static_cast<float>(Random::randInt(40, 120));
        b.speed = Random::randFloat(40.f, 80.f);
        m_birds.push_back(b);
    }

    m_npcs = {
        {u8"符箓摊", 8.f, 10.f, 1.2f, 8.f, 14.f, 1},
        {u8"丹药铺", 25.f, 12.f, 0.8f, 10.f, 16.f, -1},
        {u8"路人", 15.f, 18.f, 1.f, 15.f, 22.f, 1},
        {u8"路人", 30.f, 20.f, 0.9f, 18.f, 24.f, -1},
    };
}
