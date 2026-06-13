/**
 * @file    Button.cpp
 * @brief   Button 实现
 */

#include "ui/widgets/Button.h"

#include "ui/UiTheme.h"

Button::Button()
    : m_theme(nullptr)
    , m_enabled(true)
    , m_hovered(false)
{
}

void Button::setup(const UiTheme* theme,
                   const std::string& label,
                   float x, float y,
                   float width, float height)
{
    m_theme = theme;
    m_label = label;
    m_bounds = sf::FloatRect(x, y, width, height);
}

void Button::setOnClick(std::function<void()> cb)
{
    m_onClick = std::move(cb);
}

void Button::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool Button::isEnabled() const
{
    return m_enabled;
}

bool Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_enabled)
    {
        m_hovered = false;
        return false;
    }

    if (event.type == sf::Event::MouseMoved)
    {
        const sf::Vector2i pixel(event.mouseMove.x, event.mouseMove.y);
        const sf::Vector2f pos = window.mapPixelToCoords(pixel);
        m_hovered = m_bounds.contains(pos);
    }
    else if (event.type == sf::Event::MouseButtonPressed &&
             event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f pos = window.mapPixelToCoords(pixel);
        if (m_bounds.contains(pos) && m_onClick)
        {
            m_onClick();
            return true;
        }
    }
    return false;
}

void Button::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    sf::RectangleShape rect({m_bounds.width, m_bounds.height});
    rect.setPosition(m_bounds.left, m_bounds.top);
    if (!m_enabled)
    {
        rect.setFillColor(sf::Color(50, 50, 50, 180));
        rect.setOutlineColor(sf::Color(100, 100, 100));
    }
    else
    {
        rect.setFillColor(m_hovered ? m_theme->buttonHover() : m_theme->buttonNormal());
        rect.setOutlineColor(m_theme->panelBorder());
    }
    rect.setOutlineThickness(1.5f);
    target.draw(rect);

    m_theme->drawTextCentered(
        target,
        m_label,
        m_bounds.left + m_bounds.width / 2.f,
        m_bounds.top + m_bounds.height / 2.f,
        18,
        m_enabled ? m_theme->textColor() : sf::Color(150, 150, 150),
        true);
}

void Button::setPosition(float x, float y)
{
    m_bounds.left = x;
    m_bounds.top  = y;
}
