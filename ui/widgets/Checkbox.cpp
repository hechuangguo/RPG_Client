/**
 * @file    Checkbox.cpp
 * @brief   Checkbox 实现
 */

#include "ui/widgets/Checkbox.h"

#include "ui/UiTheme.h"

Checkbox::Checkbox()
    : m_theme(nullptr)
    , m_checked(false)
{
}

void Checkbox::setup(const UiTheme* theme, const std::string& label, float x, float y)
{
    m_theme     = theme;
    m_label     = label;
    m_boxBounds = sf::FloatRect(x, y, 20.f, 20.f);
}

void Checkbox::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type != sf::Event::MouseButtonPressed ||
        event.mouseButton.button != sf::Mouse::Left)
    {
        return;
    }

    const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
    const sf::Vector2f pos = window.mapPixelToCoords(pixel);

    float labelW = 80.f;
    if (m_theme)
    {
        labelW = m_theme->measureTextWidth(m_label, 16);
    }
    const float hitW = m_boxBounds.width + 8.f + labelW;
    sf::FloatRect hit(m_boxBounds.left, m_boxBounds.top, hitW, m_boxBounds.height);
    if (hit.contains(pos))
    {
        m_checked = !m_checked;
    }
}

void Checkbox::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    sf::RectangleShape box({m_boxBounds.width, m_boxBounds.height});
    box.setPosition(m_boxBounds.left, m_boxBounds.top);
    box.setFillColor(m_theme->inputFill());

    const bool glass = m_theme->hasLoginBackground();
    if (glass)
    {
        box.setOutlineThickness(0.f);
    }
    else
    {
        box.setOutlineColor(m_theme->panelBorder());
        box.setOutlineThickness(1.f);
    }
    target.draw(box);

    if (m_checked)
    {
        sf::RectangleShape mark({10.f, 10.f});
        mark.setPosition(m_boxBounds.left + 5.f, m_boxBounds.top + 5.f);
        mark.setFillColor(m_theme->accentColor());
        target.draw(mark);
    }

    m_theme->drawText(
        target,
        m_label,
        m_boxBounds.left + 28.f,
        m_boxBounds.top + 1.f,
        16,
        m_theme->textColor());
}

bool Checkbox::isChecked() const
{
    return m_checked;
}

void Checkbox::setChecked(bool checked)
{
    m_checked = checked;
}
