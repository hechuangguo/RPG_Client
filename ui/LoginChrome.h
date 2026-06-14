/**
 * @file    LoginChrome.h
 * @brief   登录前界面公共控件（退出游戏按钮）
 */

#pragma once

#include "ui/widgets/Button.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <functional>

class UiTheme;

/**
 * @brief 登录流程公共 UI 装饰（底部退出按钮）
 */
class LoginChrome
{
public:
    LoginChrome();

    void setup(UiTheme* theme, const sf::Vector2u& viewSize);
    void setOnExit(std::function<void()> cb);
    void onResize(const sf::Vector2u& viewSize);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderTarget& target) const;

private:
    UiTheme*             m_theme;
    Button               m_exitButton;
    std::function<void()> m_onExit;
};
