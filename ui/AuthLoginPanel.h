/**
 * @file    AuthLoginPanel.h
 * @brief   账号密码登录界面（选区完成后显示）
 */

#pragma once

#include "ui/widgets/Button.h"
#include "ui/widgets/Checkbox.h"
#include "ui/widgets/TextInput.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>
#include <functional>
#include <string>

class LocalSettings;
class UiTheme;

/**
 * @brief 账号登录面板
 */
class AuthLoginPanel
{
public:
    struct LoginRequest
    {
        std::string account;
        std::string password;
        bool        rememberAccount;
    };

    AuthLoginPanel();

    void setup(UiTheme* theme, LocalSettings* settings, const sf::Vector2u& viewSize);

    void setOnLogin(std::function<void(const LoginRequest&)> cb);
    void setOnRegisterClick(std::function<void()> cb);
    void setOnBackToZoneHome(std::function<void()> cb);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void draw(sf::RenderTarget& target) const;

    void setErrorMessage(const std::string& msg);
    void applyLocalSettings();

private:
    void refreshLoginButtonState();
    void focusInputByIndex(std::size_t idx);

    UiTheme*                m_theme;
    LocalSettings*          m_settings;
    TextInput               m_accountInput;
    TextInput               m_passwordInput;
    Checkbox                m_rememberBox;
    Checkbox                m_showPasswordBox;
    Button                  m_loginButton;
    Button                  m_registerButton;
    Button                  m_backToZoneButton;
    std::function<void(const LoginRequest&)> m_onLogin;
    std::function<void()>   m_onRegisterClick;
    std::function<void()>   m_onBackToZoneHome;
    std::string             m_errorMessage;
    sf::Vector2u            m_viewSize;
    std::size_t             m_focusedInputIndex;
};
