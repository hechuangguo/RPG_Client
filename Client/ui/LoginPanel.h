/**
 * @file    LoginPanel.h
 * @brief   登录界面面板
 *
 * 职责：
 * - 集成 ZoneSelectPanel、账号/密码输入、记住账号、登录/注册按钮
 * - 未选区时禁用登录
 *
 * 协作：GameApp、LoginSession、LocalSettings。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ui/ZoneSelectPanel.h"
#include "ui/widgets/Button.h"
#include "ui/widgets/Checkbox.h"
#include "ui/widgets/TextInput.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <functional>
#include <string>

class LocalSettings;
class ServerListLoader;
class UiTheme;

/**
 * @brief 登录面板
 */
class LoginPanel
{
public:
    struct LoginRequest
    {
        std::string account;
        std::string password;
        uint32_t    zoneId;
        uint8_t     gameType;
        bool        rememberAccount;
    };

    LoginPanel();

    /**
     * @brief 初始化布局
     * @param theme    UI 主题
     * @param loader   区服列表
     * @param settings 本地设置
     * @param viewSize 窗口逻辑尺寸
     */
    void setup(UiTheme* theme,
               const ServerListLoader* loader,
               LocalSettings* settings,
               const sf::Vector2u& viewSize);

    /** @brief 设置登录请求回调 */
    void setOnLogin(std::function<void(const LoginRequest&)> cb);

    /** @brief 设置跳转注册回调 */
    void setOnRegisterClick(std::function<void()> cb);

    /** @brief 处理 SFML 事件 */
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    /** @brief 绘制登录界面 */
    void draw(sf::RenderTarget& target) const;

    /** @brief 显示错误提示 */
    void setErrorMessage(const std::string& msg);

    /** @brief 从 LocalSettings 恢复输入 */
    void applyLocalSettings();

    /** @brief 当前选中 zoneId */
    uint32_t selectedZoneId() const;

    /** @brief 当前选中 gameType */
    uint8_t selectedGameType() const;

private:
    void refreshLoginButtonState();

    UiTheme*                m_theme;
    LocalSettings*          m_settings;
    ZoneSelectPanel         m_zonePanel;
    TextInput               m_accountInput;
    TextInput               m_passwordInput;
    Checkbox                m_rememberBox;
    Button                  m_loginButton;
    Button                  m_registerButton;
    std::function<void(const LoginRequest&)> m_onLogin;
    std::function<void()>   m_onRegisterClick;
    std::string             m_errorMessage;
    sf::Vector2u            m_viewSize;
};
