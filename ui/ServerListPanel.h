/**
 * @file    ServerListPanel.h
 * @brief   网络拉取的游戏区列表选择面板
 */

#pragma once

#include "net/ZoneTypes.h"
#include "ui/widgets/Button.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class UiTheme;

/**
 * @brief 区列表选择面板
 */
class ServerListPanel
{
public:
    enum class Status
    {
        Loading,
        Ready,
        Error,
    };

    ServerListPanel();

    void setup(UiTheme* theme, const sf::Vector2u& viewSize);

    void setStatus(Status status, const std::string& message = "");
    void setZones(const std::vector<GameZoneEntry>& zones);

    void setOnConfirm(std::function<void(const GameZoneEntry&)> cb);
    void setOnCancel(std::function<void()> cb);
    void setOnRetry(std::function<void()> cb);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderTarget& target) const;

private:
    sf::FloatRect listAreaRect() const;
    int rowAtPosition(const sf::Vector2f& pos) const;
    void refreshConfirmButton();

    UiTheme*                    m_theme;
    sf::Vector2u                m_viewSize;
    Status                      m_status;
    std::string                 m_statusMessage;
    std::vector<GameZoneEntry>  m_zones;
    int                         m_selectedIndex;
    Button                      m_confirmButton;
    Button                      m_cancelButton;
    Button                      m_retryButton;
    std::function<void(const GameZoneEntry&)> m_onConfirm;
    std::function<void()>       m_onCancel;
    std::function<void()>       m_onRetry;
};
