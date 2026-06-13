/**
 * @file    TextInput.cpp
 * @brief   TextInput 实现
 */

#include "ui/widgets/TextInput.h"

#include "ui/UiTheme.h"

TextInput::TextInput()
    : m_theme(nullptr)
    , m_password(false)
    , m_focused(false)
{
}

void TextInput::setup(const UiTheme* theme,
                      const std::string& placeholder,
                      float x, float y,
                      float width, float height,
                      bool password)
{
    m_theme       = theme;
    m_placeholder = placeholder;
    m_bounds      = sf::FloatRect(x, y, width, height);
    m_password    = password;
}

void TextInput::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f pos = window.mapPixelToCoords(pixel);
        m_focused = m_bounds.contains(pos);
    }

    if (!m_focused)
    {
        return;
    }

    if (event.type == sf::Event::TextEntered)
    {
        const char32_t ch = event.text.unicode;
        if (ch == 8)
        {
            if (!m_text.empty())
            {
                m_text.pop_back();
            }
        }
        else if (ch >= 32 && ch < 127)
        {
            if (m_text.size() < 31)
            {
                m_text.push_back(static_cast<char>(ch));
            }
        }
    }
    else if (event.type == sf::Event::KeyPressed &&
             event.key.code == sf::Keyboard::BackSpace)
    {
        if (!m_text.empty())
        {
            m_text.pop_back();
        }
    }
}

void TextInput::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    sf::RectangleShape box({m_bounds.width, m_bounds.height});
    box.setPosition(m_bounds.left, m_bounds.top);
    box.setFillColor(sf::Color(10, 25, 28, 200));
    box.setOutlineColor(m_focused ? m_theme->accentColor() : m_theme->panelBorder());
    box.setOutlineThickness(m_focused ? 2.f : 1.f);
    target.draw(box);

    const std::string shown = displayText();
    const std::string label = shown.empty() ? m_placeholder : shown;
    sf::Text text(label, m_theme->font(), 16);
    text.setFillColor(shown.empty() ? sf::Color(120, 140, 135) : m_theme->textColor());
    text.setPosition(m_bounds.left + 8.f, m_bounds.top + 8.f);
    target.draw(text);
}

const std::string& TextInput::text() const
{
    return m_text;
}

void TextInput::setText(const std::string& text)
{
    m_text = text;
}

void TextInput::clear()
{
    m_text.clear();
}

bool TextInput::isFocused() const
{
    return m_focused;
}

std::string TextInput::displayText() const
{
    if (m_password && !m_text.empty())
    {
        return std::string(m_text.size(), '*');
    }
    return m_text;
}
