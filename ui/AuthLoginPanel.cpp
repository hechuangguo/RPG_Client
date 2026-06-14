/**
 * @file    AuthLoginPanel.cpp
 * @brief   AuthLoginPanel 实现
 */

#include "ui/AuthLoginPanel.h"

#include "ui/UiTheme.h"
#include "util/LocalSettings.h"

AuthLoginPanel::AuthLoginPanel()
    : m_theme(nullptr)
    , m_settings(nullptr)
{
}

void AuthLoginPanel::setup(UiTheme* theme, LocalSettings* settings, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_settings = settings;
    m_viewSize = viewSize;

    const float panelW = 420.f;
    const float panelH = 420.f;
    const float px = (static_cast<float>(viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(viewSize.y) - panelH) / 2.f;

    m_accountInput.setup(theme, u8"请输入账号", px + 40.f, py + 100.f, panelW - 80.f, 36.f);
    m_passwordInput.setup(theme, u8"请输入密码", px + 40.f, py + 150.f, panelW - 80.f, 36.f, true);
    m_rememberBox.setup(theme, u8"记住账号", px + 40.f, py + 200.f);
    m_loginButton.setup(theme, u8"登录", px + 40.f, py + 250.f, 160.f, 40.f);
    m_registerButton.setup(theme, u8"注册账号", px + 220.f, py + 250.f, 160.f, 40.f);

    m_loginButton.setOnClick([this]() {
        if (!m_onLogin || m_accountInput.text().empty())
        {
            return;
        }
        LoginRequest req;
        req.account         = m_accountInput.text();
        req.password        = m_passwordInput.text();
        req.rememberAccount = m_rememberBox.isChecked();
        m_onLogin(req);
    });

    m_registerButton.setOnClick([this]() {
        if (m_onRegisterClick)
        {
            m_onRegisterClick();
        }
    });
}

void AuthLoginPanel::setOnLogin(std::function<void(const LoginRequest&)> cb)
{
    m_onLogin = std::move(cb);
}

void AuthLoginPanel::setOnRegisterClick(std::function<void()> cb)
{
    m_onRegisterClick = std::move(cb);
}

void AuthLoginPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    m_accountInput.handleEvent(event, window);
    m_passwordInput.handleEvent(event, window);
    m_rememberBox.handleEvent(event, window);
    m_loginButton.handleEvent(event, window);
    m_registerButton.handleEvent(event, window);
    refreshLoginButtonState();
}

void AuthLoginPanel::update(float dt)
{
    m_accountInput.update(dt);
    m_passwordInput.update(dt);
}

void AuthLoginPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const float panelW = 420.f;
    const float panelH = 420.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;

    m_theme->drawPanel(target, sf::FloatRect(px, py, panelW, panelH));
    m_theme->drawTitle(target, u8"登录游戏", px + panelW / 2.f, py + 16.f, 28);

    m_accountInput.draw(target);
    m_passwordInput.draw(target);
    m_rememberBox.draw(target);
    m_loginButton.draw(target);
    m_registerButton.draw(target);

    if (!m_errorMessage.empty())
    {
        m_theme->drawText(
            target,
            m_errorMessage,
            px + 40.f,
            py + 320.f,
            14,
            sf::Color(255, 120, 100));
    }
}

void AuthLoginPanel::setErrorMessage(const std::string& msg)
{
    m_errorMessage = msg;
}

void AuthLoginPanel::applyLocalSettings()
{
    if (!m_settings)
    {
        return;
    }
    m_rememberBox.setChecked(m_settings->rememberAccount());
    if (m_settings->rememberAccount())
    {
        m_accountInput.setText(m_settings->lastAccount());
    }
    refreshLoginButtonState();
}

void AuthLoginPanel::refreshLoginButtonState()
{
    m_loginButton.setEnabled(!m_accountInput.text().empty());
}
