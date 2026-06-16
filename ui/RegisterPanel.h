/**
 * @file    RegisterPanel.h
 * @brief   注册界面面板
 *
 * 职责：
 * - 账号、密码、确认密码输入
 * - 本地校验后触发注册；返回登录页
 *
 * 协作：GameApp、LoginSession。
 *
 * 线程：仅主线程，非线程安全。
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

class UiTheme;

/**
 * @brief 注册面板
 */
class RegisterPanel
{
public:
    struct RegisterRequest
    {
        std::string account;
        std::string password;
        std::string confirmPassword;
        uint32_t    zoneId;
        uint8_t     gameType;
    };

    RegisterPanel();

    /**
     * @brief 初始化布局
     * @param theme    UI 主题
     * @param viewSize 窗口逻辑尺寸
     */
    void setup(UiTheme* theme, const sf::Vector2u& viewSize);

    /**
     * @brief 设置当前区服（从登录页带入）
     * @param zoneId   区号
     * @param gameType 游戏类型
     */
    void setZone(uint32_t zoneId, uint8_t gameType);

    void setOnRegister(std::function<void(const RegisterRequest&)> cb);
    void setOnBack(std::function<void()> cb);
    void setOnBackToZoneHome(std::function<void()> cb);

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void draw(sf::RenderTarget& target) const;

    void setMessage(const std::string& msg, bool isError);

private:
    bool validate(std::string& err) const;
    void focusInputByIndex(std::size_t idx);

    UiTheme*     m_theme;
    TextInput    m_accountInput;
    TextInput    m_passwordInput;
    TextInput    m_confirmInput;
    Checkbox     m_showPasswordBox;
    Button       m_registerButton;
    Button       m_backButton;
    Button       m_backToZoneButton;
    std::function<void(const RegisterRequest&)> m_onRegister;
    std::function<void()> m_onBack;
    std::function<void()> m_onBackToZoneHome;
    std::string  m_message;
    bool         m_messageIsError;
    uint32_t     m_zoneId;
    uint8_t      m_gameType;
    sf::Vector2u m_viewSize;
    std::size_t  m_focusedInputIndex;
};
