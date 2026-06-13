/**
 * @file    ZoneSelectPanel.cpp
 * @brief   ZoneSelectPanel 实现
 */

#include "ui/ZoneSelectPanel.h"

#include "ui/UiTheme.h"
#include "util/ServerListLoader.h"

ZoneSelectPanel::ZoneSelectPanel()
    : m_theme(nullptr)
    , m_loader(nullptr)
    , m_selectedIndex(-1)
    , m_rowHeight(32.f)
{
}

void ZoneSelectPanel::setup(const UiTheme* theme,
                            const ServerListLoader* loader,
                            float x, float y,
                            float width, float height)
{
    m_theme  = theme;
    m_loader = loader;
    m_bounds = sf::FloatRect(x, y, width, height);
}

void ZoneSelectPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_loader || event.type != sf::Event::MouseButtonPressed ||
        event.mouseButton.button != sf::Mouse::Left)
    {
        return;
    }

    const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
    const sf::Vector2f pos = window.mapPixelToCoords(pixel);
    if (!m_bounds.contains(pos))
    {
        return;
    }

    const float localY = pos.y - m_bounds.top;
    const int row = static_cast<int>(localY / m_rowHeight);
    const auto& zones = m_loader->zones();
    if (row >= 0 && row < static_cast<int>(zones.size()) && zones[static_cast<size_t>(row)].enabled)
    {
        m_selectedIndex = row;
    }
}

void ZoneSelectPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme || !m_loader)
    {
        return;
    }

    m_theme->drawPanel(target, m_bounds);

    sf::Text header(u8"选择仙界", m_theme->font(), 18);
    header.setFillColor(m_theme->accentColor());
    header.setPosition(m_bounds.left + 12.f, m_bounds.top + 8.f);
    target.draw(header);

    const auto& zones = m_loader->zones();
    float y = m_bounds.top + 36.f;
    for (size_t i = 0; i < zones.size(); ++i)
    {
        const GameZoneEntry& z = zones[i];
        sf::RectangleShape row({m_bounds.width - 24.f, m_rowHeight - 4.f});
        row.setPosition(m_bounds.left + 12.f, y);

        const bool selected = static_cast<int>(i) == m_selectedIndex;
        if (!z.enabled)
        {
            row.setFillColor(sf::Color(40, 40, 40, 180));
        }
        else if (selected)
        {
            row.setFillColor(sf::Color(64, 120, 110, 200));
        }
        else
        {
            row.setFillColor(sf::Color(25, 55, 58, 160));
        }
        target.draw(row);

        std::string label = z.name;
        if (!z.enabled)
        {
            label += u8"（维护中）";
        }
        sf::Text text(label, m_theme->font(), 16);
        text.setFillColor(z.enabled ? m_theme->textColor() : sf::Color(130, 130, 130));
        text.setPosition(m_bounds.left + 20.f, y + 6.f);
        target.draw(text);

        y += m_rowHeight;
    }
}

uint32_t ZoneSelectPanel::selectedZoneId() const
{
    if (!m_loader || m_selectedIndex < 0)
    {
        return 0;
    }
    const auto& zones = m_loader->zones();
    if (m_selectedIndex >= static_cast<int>(zones.size()))
    {
        return 0;
    }
    return zones[static_cast<size_t>(m_selectedIndex)].zoneId;
}

uint8_t ZoneSelectPanel::selectedGameType() const
{
    if (!m_loader || m_selectedIndex < 0)
    {
        return 0;
    }
    const auto& zones = m_loader->zones();
    if (m_selectedIndex >= static_cast<int>(zones.size()))
    {
        return 0;
    }
    return zones[static_cast<size_t>(m_selectedIndex)].gameType;
}

bool ZoneSelectPanel::hasValidSelection() const
{
    return selectedZoneId() != 0;
}

void ZoneSelectPanel::selectZoneId(uint32_t zoneId)
{
    if (!m_loader || zoneId == 0)
    {
        return;
    }
    const auto& zones = m_loader->zones();
    for (size_t i = 0; i < zones.size(); ++i)
    {
        if (zones[i].zoneId == zoneId && zones[i].enabled)
        {
            m_selectedIndex = static_cast<int>(i);
            return;
        }
    }
}
