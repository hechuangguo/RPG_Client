/**
 * @file    LoginChrome.cpp
 * @brief   LoginChrome 实现
 */

#include "ui/LoginChrome.h"

#include "ui/UiTheme.h"

namespace
{
constexpr float kExitButtonWidth  = 160.f;
constexpr float kExitButtonHeight = 40.f;
constexpr float kExitButtonBottom = 56.f;
}  // namespace

LoginChrome::LoginChrome()
    : m_theme(nullptr)
{
}

void LoginChrome::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme = theme;
    onResize(viewSize);
}

void LoginChrome::setOnExit(std::function<void()> cb)
{
    m_onExit = std::move(cb);
    m_exitButton.setOnClick([this]() {
        if (m_onExit)
        {
            m_onExit();
        }
    });
}

void LoginChrome::onResize(const sf::Vector2u& viewSize)
{
    if (!m_theme)
    {
        return;
    }

    const float centerX = static_cast<float>(viewSize.x) / 2.f - kExitButtonWidth / 2.f;
    const float y       = static_cast<float>(viewSize.y) - kExitButtonBottom;
    m_exitButton.setup(m_theme, u8"退出游戏", centerX, y, kExitButtonWidth, kExitButtonHeight);
}

void LoginChrome::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    m_exitButton.handleEvent(event, window);
}

void LoginChrome::draw(sf::RenderTarget& target) const
{
    m_exitButton.draw(target);
}
