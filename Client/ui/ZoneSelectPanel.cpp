/**
 * @file    ZoneSelectPanel.cpp
 * @brief   ZoneSelectPanel 实现
 */

#include "ui/ZoneSelectPanel.h"

#include "ui/UiTheme.h"
#include "util/ServerListLoader.h"

namespace
{
constexpr float kTitleHeight = 28.f;
constexpr float kComboHeight = 36.f;
constexpr float kRowHeight   = 32.f;
constexpr float kPadding     = 12.f;
}  // namespace

ZoneSelectPanel::ZoneSelectPanel()
    : m_theme(nullptr)
    , m_loader(nullptr)
    , m_collapsedHeight(72.f)
    , m_selectedIndex(-1)
    , m_dropdownOpen(false)
    , m_titleHeight(kTitleHeight)
    , m_comboHeight(kComboHeight)
    , m_rowHeight(kRowHeight)
{
}

void ZoneSelectPanel::setup(const UiTheme* theme,
                            const ServerListLoader* loader,
                            float x, float y,
                            float width, float height)
{
    m_theme           = theme;
    m_loader          = loader;
    m_bounds          = sf::FloatRect(x, y, width, height);
    m_collapsedHeight = height;
    m_selectedIndex   = -1;
    m_dropdownOpen    = false;
    refreshCollapsedHeight();
    tryAutoSelectSingleZone();
}

void ZoneSelectPanel::setOnSelectionChanged(std::function<void()> cb)
{
    m_onSelectionChanged = std::move(cb);
}

void ZoneSelectPanel::refreshCollapsedHeight()
{
    m_bounds.height = m_collapsedHeight;
}

sf::FloatRect ZoneSelectPanel::comboBoxRect() const
{
    return sf::FloatRect(
        m_bounds.left + kPadding,
        m_bounds.top + m_titleHeight,
        m_bounds.width - kPadding * 2.f,
        m_comboHeight);
}

sf::FloatRect ZoneSelectPanel::listAreaRect() const
{
    if (!m_loader)
    {
        return sf::FloatRect();
    }

    const float listTop = m_bounds.top + m_titleHeight + m_comboHeight;
    const float listH   = static_cast<float>(m_loader->zones().size()) * m_rowHeight;
    return sf::FloatRect(m_bounds.left + kPadding, listTop,
                         m_bounds.width - kPadding * 2.f, listH);
}

sf::FloatRect ZoneSelectPanel::hitBounds() const
{
    if (!m_dropdownOpen || !m_loader)
    {
        return sf::FloatRect(m_bounds.left, m_bounds.top, m_bounds.width, m_collapsedHeight);
    }

    const sf::FloatRect list = listAreaRect();
    return sf::FloatRect(
        m_bounds.left,
        m_bounds.top,
        m_bounds.width,
        (list.top + list.height) - m_bounds.top);
}

std::string ZoneSelectPanel::selectedLabel() const
{
    if (!m_loader || m_selectedIndex < 0)
    {
        return u8"请选择仙界";
    }

    const auto& zones = m_loader->zones();
    if (m_selectedIndex >= static_cast<int>(zones.size()))
    {
        return u8"请选择仙界";
    }

    return zones[static_cast<size_t>(m_selectedIndex)].name;
}

int ZoneSelectPanel::rowAtPosition(const sf::Vector2f& pos) const
{
    const sf::FloatRect list = listAreaRect();
    if (!list.contains(pos))
    {
        return -1;
    }

    return static_cast<int>((pos.y - list.top) / m_rowHeight);
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
    const sf::FloatRect hit = hitBounds();

    if (!hit.contains(pos))
    {
        if (m_dropdownOpen)
        {
            m_dropdownOpen = false;
            refreshCollapsedHeight();
        }
        return;
    }

    const sf::FloatRect combo = comboBoxRect();
    if (combo.contains(pos))
    {
        m_dropdownOpen = !m_dropdownOpen;
        if (m_dropdownOpen && m_loader)
        {
            m_bounds.height = m_titleHeight + m_comboHeight +
                              static_cast<float>(m_loader->zones().size()) * m_rowHeight +
                              kPadding;
        }
        else
        {
            refreshCollapsedHeight();
        }
        return;
    }

    if (!m_dropdownOpen)
    {
        return;
    }

    const int row = rowAtPosition(pos);
    const auto& zones = m_loader->zones();
    if (row >= 0 && row < static_cast<int>(zones.size()) && zones[static_cast<size_t>(row)].enabled)
    {
        m_selectedIndex = row;
        m_dropdownOpen  = false;
        refreshCollapsedHeight();
        if (m_onSelectionChanged)
        {
            m_onSelectionChanged();
        }
    }
}

void ZoneSelectPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme || !m_loader || !m_theme->isFontLoaded())
    {
        return;
    }

    const sf::FloatRect panelRect(m_bounds.left, m_bounds.top, m_bounds.width, hitBounds().height);
    m_theme->drawPanel(target, panelRect);

    m_theme->drawText(
        target,
        u8"选择仙界",
        m_bounds.left + kPadding,
        m_bounds.top + 6.f,
        18,
        m_theme->accentColor());

    const sf::FloatRect combo = comboBoxRect();
    sf::RectangleShape comboBox({combo.width, combo.height});
    comboBox.setPosition(combo.left, combo.top);
    comboBox.setFillColor(sf::Color(10, 25, 28, 200));
    comboBox.setOutlineColor(m_theme->panelBorder());
    comboBox.setOutlineThickness(1.f);
    target.draw(comboBox);

    m_theme->drawText(
        target,
        selectedLabel(),
        combo.left + 8.f,
        combo.top + 10.f,
        16,
        m_selectedIndex >= 0 ? m_theme->textColor() : sf::Color(120, 140, 135));

    m_theme->drawText(
        target,
        m_dropdownOpen ? u8"▲" : u8"▼",
        combo.left + combo.width - 22.f,
        combo.top + 8.f,
        16,
        m_theme->accentColor());

    if (!m_dropdownOpen)
    {
        return;
    }

    const auto& zones = m_loader->zones();
    float y           = listAreaRect().top;
    for (size_t i = 0; i < zones.size(); ++i)
    {
        const GameZoneEntry& z = zones[i];
        sf::RectangleShape row({combo.width, m_rowHeight - 4.f});
        row.setPosition(combo.left, y);

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
        m_theme->drawText(
            target,
            label,
            combo.left + 8.f,
            y + 6.f,
            16,
            z.enabled ? m_theme->textColor() : sf::Color(130, 130, 135));

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

void ZoneSelectPanel::tryAutoSelectSingleZone()
{
    if (!m_loader || m_selectedIndex >= 0)
    {
        return;
    }

    const auto& zones = m_loader->zones();
    int enabledCount  = 0;
    int onlyIndex     = -1;
    for (size_t i = 0; i < zones.size(); ++i)
    {
        if (zones[i].enabled)
        {
            ++enabledCount;
            onlyIndex = static_cast<int>(i);
        }
    }

    if (enabledCount == 1 && onlyIndex >= 0)
    {
        m_selectedIndex = onlyIndex;
        if (m_onSelectionChanged)
        {
            m_onSelectionChanged();
        }
    }
}
