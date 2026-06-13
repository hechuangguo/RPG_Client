/**
 * @file    LoginPanel.cpp
 * @brief   LoginPanel 实现
 */

#include "ui/LoginPanel.h"

#include "ui/UiTheme.h"
#include "util/LocalSettings.h"
#include "util/ServerListLoader.h"

LoginPanel::LoginPanel()
    : m_theme(nullptr)
    , m_settings(nullptr)
{
}

void LoginPanel::setup(UiTheme* theme,
                       const ServerListLoader* loader,
                       LocalSettings* settings,
                       const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_settings = settings;
    m_viewSize = viewSize;

    const float panelW = 420.f;
    const float panelH = 520.f;
    const float px = (static_cast<float>(viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(viewSize.y) - panelH) / 2.f;

    m_zonePanel.setup(theme, loader, px + 20.f, py + 60.f, panelW - 40.f, 140.f);
    m_accountInput.setup(theme, u8"请输入道号", px + 40.f, py + 220.f, panelW - 80.f, 36.f);
    m_passwordInput.setup(theme, u8"请输入密钥", px + 40.f, py + 270.f, panelW - 80.f, 36.f, true);
    m_rememberBox.setup(theme, u8"记住道号", px + 40.f, py + 320.f);
    m_loginButton.setup(theme, u8"踏入仙途", px + 40.f, py + 360.f, 160.f, 40.f);
    m_registerButton.setup(theme, u8"注册道号", px + 220.f, py + 360.f, 160.f, 40.f);

    m_loginButton.setOnClick([this]() {
        if (!m_onLogin || !m_zonePanel.hasValidSelection())
        {
            return;
        }
        LoginRequest req;
        req.account         = m_accountInput.text();
        req.password        = m_passwordInput.text();
        req.zoneId          = m_zonePanel.selectedZoneId();
        req.gameType        = m_zonePanel.selectedGameType();
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

void LoginPanel::setOnLogin(std::function<void(const LoginRequest&)> cb)
{
    m_onLogin = std::move(cb);
}

void LoginPanel::setOnRegisterClick(std::function<void()> cb)
{
    m_onRegisterClick = std::move(cb);
}

void LoginPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    m_zonePanel.handleEvent(event, window);
    m_accountInput.handleEvent(event, window);
    m_passwordInput.handleEvent(event, window);
    m_rememberBox.handleEvent(event, window);
    m_loginButton.handleEvent(event, window);
    m_registerButton.handleEvent(event, window);
    refreshLoginButtonState();
}

void LoginPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const float panelW = 420.f;
    const float panelH = 520.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;

    m_theme->drawPanel(target, sf::FloatRect(px, py, panelW, panelH));
    m_theme->drawTitle(target, u8"仙侠世界", px + panelW / 2.f, py + 16.f, 32);

    m_zonePanel.draw(target);
    m_accountInput.draw(target);
    m_passwordInput.draw(target);
    m_rememberBox.draw(target);
    m_loginButton.draw(target);
    m_registerButton.draw(target);

    if (!m_errorMessage.empty())
    {
        sf::Text err(m_errorMessage, m_theme->font(), 14);
        err.setFillColor(sf::Color(255, 120, 100));
        err.setPosition(px + 40.f, py + 420.f);
        target.draw(err);
    }
}

void LoginPanel::setErrorMessage(const std::string& msg)
{
    m_errorMessage = msg;
}

void LoginPanel::applyLocalSettings()
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
    m_zonePanel.selectZoneId(m_settings->lastZoneId());
    refreshLoginButtonState();
}

void LoginPanel::refreshLoginButtonState()
{
    m_loginButton.setEnabled(m_zonePanel.hasValidSelection() &&
                             !m_accountInput.text().empty());
}

uint32_t LoginPanel::selectedZoneId() const
{
    return m_zonePanel.selectedZoneId();
}

uint8_t LoginPanel::selectedGameType() const
{
    return m_zonePanel.selectedGameType();
}
