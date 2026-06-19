/**
 * @file    GameExitDialog.h
 * @brief   游戏中退出二级确认弹窗
 *
 * 触发：游戏内 ESC 或窗口关闭按钮。
 * 选项：返回选角、返回登录、退出游戏、取消。
 *
 * 协作：GameApp、GameSession（C2S_LOGOUT_REQ）。
 */

#pragma once

#include "ui/widgets/Button.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <functional>

class UiTheme;

/**
 * @brief 游戏中退出确认对话框
 */
class GameExitDialog
{
public:
    GameExitDialog();

    void setup(UiTheme* theme, const sf::Vector2u& viewSize);

    void show();
    void hide();
    bool isVisible() const;

    void setBusy(bool busy, const std::string& message = "");

    void setOnReturnCharSelect(std::function<void()> cb);
    void setOnReturnLogin(std::function<void()> cb);
    void setOnQuitClient(std::function<void()> cb);

    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderTarget& target) const;

private:
    void layout(const sf::Vector2u& viewSize);
    sf::FloatRect panelRect() const;

    UiTheme* m_theme;
    sf::Vector2u m_viewSize;
    bool         m_visible;
    bool         m_busy;
    std::string  m_busyMessage;

    Button m_returnCharSelectBtn;
    Button m_returnLoginBtn;
    Button m_quitClientBtn;
    Button m_cancelBtn;

    std::function<void()> m_onReturnCharSelect;
    std::function<void()> m_onReturnLogin;
    std::function<void()> m_onQuitClient;
};
