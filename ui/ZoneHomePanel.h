/**
 * @file    ZoneHomePanel.h
 * @brief   选区首页：当前游戏区 + 选择服务器 + 进入游戏
 */

#pragma once

#include "ui/widgets/Button.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <functional>
#include <string>

class UiTheme;

/**
 * @brief 选区首页面板
 */
class ZoneHomePanel
{
public:
    ZoneHomePanel();

    void setup(UiTheme* theme, const sf::Vector2u& viewSize);

    void setCurrentZoneName(const std::string& name);
    void setHasValidZone(bool valid);

    void setOnSelectServer(std::function<void()> cb);
    void setOnEnterGame(std::function<void()> cb);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderTarget& target) const;

private:
    UiTheme*                m_theme;
    sf::Vector2u            m_viewSize;
    std::string             m_zoneName;
    bool                    m_hasValidZone;
    Button                  m_selectServerButton;
    Button                  m_enterGameButton;
    std::function<void()>   m_onSelectServer;
    std::function<void()>   m_onEnterGame;
};
