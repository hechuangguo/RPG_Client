/**
 * @file    AuthLoginPanel.cpp
 * @brief   AuthLoginPanel 实现
 */

#include "ui/AuthLoginPanel.h"

#include "ui/UiTheme.h"
#include "util/LocalSettings.h"

#include <algorithm>

namespace
{
constexpr float kSuccessHoldSec = 3.f;
constexpr float kSuccessFadeSec = 1.f;
constexpr float kSuccessBannerX = 40.f;
constexpr float kSuccessBannerY = -44.f;
constexpr float kSuccessBannerH = 40.f;
constexpr float kSuccessTextY   = -34.f;
}  // namespace

AuthLoginPanel::AuthLoginPanel()
    : m_theme(nullptr)
    , m_settings(nullptr)
    , m_successElapsed(0.f)
    , m_focusedInputIndex(0)
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
    m_showPasswordBox.setup(theme, u8"显示密码", px + 180.f, py + 200.f);
    m_loginButton.setup(theme, u8"登录", px + 40.f, py + 250.f, 160.f, 40.f);
    m_registerButton.setup(theme, u8"注册账号", px + 220.f, py + 250.f, 160.f, 40.f);
    m_backToZoneButton.setup(theme, u8"返回选择服务器", px + 40.f, py + 300.f, panelW - 80.f, 36.f);
    m_showPasswordBox.setChecked(false);
    m_passwordInput.setPasswordMasked(true);
    focusInputByIndex(0);

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

    m_backToZoneButton.setOnClick([this]() {
        if (m_onBackToZoneHome)
        {
            m_onBackToZoneHome();
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

void AuthLoginPanel::setOnBackToZoneHome(std::function<void()> cb)
{
    m_onBackToZoneHome = std::move(cb);
}

void AuthLoginPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab)
    {
        if (event.key.shift)
        {
            m_focusedInputIndex = (m_focusedInputIndex == 0) ? 1 : (m_focusedInputIndex - 1);
        }
        else
        {
            m_focusedInputIndex = (m_focusedInputIndex + 1) % 2;
        }
        focusInputByIndex(m_focusedInputIndex);
        return;
    }

    m_accountInput.handleEvent(event, window);
    m_passwordInput.handleEvent(event, window);
    m_rememberBox.handleEvent(event, window);
    m_showPasswordBox.handleEvent(event, window);
    m_loginButton.handleEvent(event, window);
    m_registerButton.handleEvent(event, window);
    m_backToZoneButton.handleEvent(event, window);

    m_passwordInput.setPasswordMasked(!m_showPasswordBox.isChecked());

    if (m_accountInput.isFocused())
    {
        m_focusedInputIndex = 0;
    }
    else if (m_passwordInput.isFocused())
    {
        m_focusedInputIndex = 1;
    }

    refreshLoginButtonState();
}

void AuthLoginPanel::update(float dt)
{
    m_accountInput.update(dt);
    m_passwordInput.update(dt);

    if (m_successMessage.empty())
    {
        return;
    }

    m_successElapsed += dt;
    if (m_successElapsed >= kSuccessHoldSec + kSuccessFadeSec)
    {
        m_successMessage.clear();
        m_successElapsed = 0.f;
    }
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
    m_showPasswordBox.draw(target);
    m_loginButton.draw(target);
    m_registerButton.draw(target);
    m_backToZoneButton.draw(target);

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

    if (!m_successMessage.empty())
    {
        const float alpha = successToastAlpha();
        const auto  a     = static_cast<sf::Uint8>(alpha * 255.f);

        sf::RectangleShape banner({panelW - kSuccessBannerX * 2.f, kSuccessBannerH});
        banner.setPosition(px + kSuccessBannerX, py + kSuccessBannerY);
        banner.setFillColor(sf::Color(35, 110, 85, static_cast<sf::Uint8>(alpha * 160.f)));
        banner.setOutlineColor(sf::Color(130, 230, 170, a));
        banner.setOutlineThickness(1.f);
        target.draw(banner);

        m_theme->drawTextCentered(
            target,
            m_successMessage,
            px + panelW / 2.f,
            py + kSuccessTextY,
            18,
            sf::Color(200, 255, 220, a),
            false);
    }
}

void AuthLoginPanel::setErrorMessage(const std::string& msg)
{
    m_errorMessage = msg;
    if (!msg.empty())
    {
        m_successMessage.clear();
        m_successElapsed = 0.f;
    }
}

void AuthLoginPanel::setSuccessMessage(const std::string& msg)
{
    m_successMessage = msg;
    m_errorMessage.clear();
    m_successElapsed = 0.f;
}

float AuthLoginPanel::successToastAlpha() const
{
    if (m_successMessage.empty())
    {
        return 0.f;
    }
    if (m_successElapsed <= kSuccessHoldSec)
    {
        return 1.f;
    }
    const float fade = (m_successElapsed - kSuccessHoldSec) / kSuccessFadeSec;
    return std::max(0.f, 1.f - fade);
}

void AuthLoginPanel::setCredentials(const std::string& account, const std::string& password)
{
    m_accountInput.setText(account);
    m_passwordInput.setText(password);
    m_showPasswordBox.setChecked(false);
    m_passwordInput.setPasswordMasked(true);
    refreshLoginButtonState();
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

void AuthLoginPanel::focusInputByIndex(std::size_t idx)
{
    m_accountInput.setFocused(idx == 0);
    m_passwordInput.setFocused(idx == 1);
}
