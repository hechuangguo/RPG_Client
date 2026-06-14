/**
 * @file    TextInput.cpp
 * @brief   TextInput 实现
 */

#include "ui/widgets/TextInput.h"

#include "ui/UiTheme.h"

namespace
{
constexpr float kCursorBlinkInterval = 0.5f;
constexpr float kTextPadding         = 8.f;
constexpr float kCursorWidth         = 2.f;
constexpr unsigned kFontSize         = 16;
}  // namespace

TextInput::TextInput()
    : m_theme(nullptr)
    , m_password(false)
    , m_focused(false)
    , m_cursorPos(0)
    , m_blinkElapsed(0.f)
    , m_cursorVisible(true)
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
        if (m_focused)
        {
            m_blinkElapsed  = 0.f;
            m_cursorVisible = true;
        }
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
                syncCursorPos();
            }
        }
        else if (ch >= 32 && ch < 127)
        {
            if (m_text.size() < 31)
            {
                m_text.push_back(static_cast<char>(ch));
                syncCursorPos();
            }
        }
    }
    else if (event.type == sf::Event::KeyPressed &&
             event.key.code == sf::Keyboard::BackSpace)
    {
        if (!m_text.empty())
        {
            m_text.pop_back();
            syncCursorPos();
        }
    }
}

void TextInput::update(float dt)
{
    if (!m_focused)
    {
        return;
    }

    m_blinkElapsed += dt;
    if (m_blinkElapsed >= kCursorBlinkInterval)
    {
        m_blinkElapsed = 0.f;
        m_cursorVisible = !m_cursorVisible;
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
    box.setFillColor(m_theme->inputFill());

    const bool glass = m_theme->hasLoginBackground();
    if (m_focused)
    {
        sf::Color focusOutline = m_theme->accentColor();
        if (glass)
        {
            focusOutline.a = 120;
        }
        box.setOutlineColor(focusOutline);
        box.setOutlineThickness(glass ? 1.f : 2.f);
    }
    else if (glass)
    {
        box.setOutlineThickness(0.f);
    }
    else
    {
        box.setOutlineColor(m_theme->panelBorder());
        box.setOutlineThickness(1.f);
    }
    target.draw(box);

    const std::string shown = displayText();
    const bool empty        = shown.empty();
    const std::string label = empty ? m_placeholder : shown;
    m_theme->drawText(
        target,
        label,
        m_bounds.left + kTextPadding,
        m_bounds.top + kTextPadding,
        kFontSize,
        empty ? sf::Color(120, 140, 135) : m_theme->textColor());

    if (m_focused && m_cursorVisible && m_theme->isFontLoaded())
    {
        const std::string beforeCursor = empty ? std::string() : shown.substr(0, m_cursorPos);
        const float cursorX = m_bounds.left + kTextPadding +
                              m_theme->measureTextWidth(beforeCursor, kFontSize);
        const float cursorY = m_bounds.top + kTextPadding;
        const float cursorH = static_cast<float>(kFontSize) + 2.f;

        sf::RectangleShape caret({kCursorWidth, cursorH});
        caret.setPosition(cursorX, cursorY);
        caret.setFillColor(m_theme->accentColor());
        target.draw(caret);
    }
}

const std::string& TextInput::text() const
{
    return m_text;
}

void TextInput::setText(const std::string& text)
{
    m_text = text;
    syncCursorPos();
}

void TextInput::clear()
{
    m_text.clear();
    syncCursorPos();
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

void TextInput::syncCursorPos()
{
    m_cursorPos = m_text.size();
}
