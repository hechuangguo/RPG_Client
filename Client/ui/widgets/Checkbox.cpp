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

    sf::FloatRect hit(m_boxBounds.left, m_boxBounds.top, 200.f, m_boxBounds.height);
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
    box.setFillColor(sf::Color(10, 25, 28, 200));
    box.setOutlineColor(m_theme->panelBorder());
    box.setOutlineThickness(1.f);
    target.draw(box);

    if (m_checked)
    {
        sf::RectangleShape mark({10.f, 10.f});
        mark.setPosition(m_boxBounds.left + 5.f, m_boxBounds.top + 5.f);
        mark.setFillColor(m_theme->accentColor());
        target.draw(mark);
    }

    sf::Text text(m_label, m_theme->font(), 16);
    text.setFillColor(m_theme->textColor());
    text.setPosition(m_boxBounds.left + 28.f, m_boxBounds.top + 1.f);
    target.draw(text);
}

bool Checkbox::isChecked() const
{
    return m_checked;
}

void Checkbox::setChecked(bool checked)
{
    m_checked = checked;
}
