/**
 * @file    RegisterPanel.cpp
 * @brief   RegisterPanel 实现
 */

#include "ui/RegisterPanel.h"

#include "ui/UiTheme.h"

RegisterPanel::RegisterPanel()
    : m_theme(nullptr)
    , m_messageIsError(false)
    , m_zoneId(0)
    , m_gameType(0)
    , m_focusedInputIndex(0)
{
}

void RegisterPanel::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_viewSize = viewSize;

    const float panelW = 400.f;
    const float panelH = 430.f;
    const float px = (static_cast<float>(viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(viewSize.y) - panelH) / 2.f;

    m_accountInput.setup(theme, u8"新道号", px + 40.f, py + 80.f, panelW - 80.f, 36.f);
    m_passwordInput.setup(theme, u8"密钥", px + 40.f, py + 130.f, panelW - 80.f, 36.f, true);
    m_confirmInput.setup(theme, u8"确认密钥", px + 40.f, py + 180.f, panelW - 80.f, 36.f, true);
    m_showPasswordBox.setup(theme, u8"显示密钥", px + 40.f, py + 230.f);
    m_registerButton.setup(theme, u8"开宗立派", px + 40.f, py + 270.f, 150.f, 40.f);
    m_backButton.setup(theme, u8"返回", px + 210.f, py + 270.f, 150.f, 40.f);
    m_backToZoneButton.setup(theme, u8"返回选择服务器", px + 40.f, py + 320.f, panelW - 80.f, 36.f);
    m_showPasswordBox.setChecked(false);
    m_passwordInput.setPasswordMasked(true);
    m_confirmInput.setPasswordMasked(true);
    focusInputByIndex(0);

    m_registerButton.setOnClick([this]() {
        std::string err;
        if (!validate(err))
        {
            setMessage(err, true);
            return;
        }
        if (m_onRegister)
        {
            RegisterRequest req;
            req.account  = m_accountInput.text();
            req.password = m_passwordInput.text();
            req.confirmPassword = m_confirmInput.text();
            req.zoneId   = m_zoneId;
            req.gameType = m_gameType;
            m_onRegister(req);
        }
    });

    m_backButton.setOnClick([this]() {
        if (m_onBack)
        {
            m_onBack();
        }
    });

    m_backToZoneButton.setOnClick([this]() {
        if (m_onBackToZoneHome)
        {
            m_onBackToZoneHome();
        }
    });
}

void RegisterPanel::setZone(uint32_t zoneId, uint8_t gameType)
{
    m_zoneId   = zoneId;
    m_gameType = gameType;
}

void RegisterPanel::setOnRegister(std::function<void(const RegisterRequest&)> cb)
{
    m_onRegister = std::move(cb);
}

void RegisterPanel::setOnBack(std::function<void()> cb)
{
    m_onBack = std::move(cb);
}

void RegisterPanel::setOnBackToZoneHome(std::function<void()> cb)
{
    m_onBackToZoneHome = std::move(cb);
}

void RegisterPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Tab)
    {
        if (event.key.shift)
        {
            m_focusedInputIndex = (m_focusedInputIndex == 0) ? 2 : (m_focusedInputIndex - 1);
        }
        else
        {
            m_focusedInputIndex = (m_focusedInputIndex + 1) % 3;
        }
        focusInputByIndex(m_focusedInputIndex);
        return;
    }

    m_accountInput.handleEvent(event, window);
    m_passwordInput.handleEvent(event, window);
    m_confirmInput.handleEvent(event, window);
    m_showPasswordBox.handleEvent(event, window);
    m_registerButton.handleEvent(event, window);
    m_backButton.handleEvent(event, window);
    m_backToZoneButton.handleEvent(event, window);

    const bool masked = !m_showPasswordBox.isChecked();
    m_passwordInput.setPasswordMasked(masked);
    m_confirmInput.setPasswordMasked(masked);

    if (m_accountInput.isFocused())
    {
        m_focusedInputIndex = 0;
    }
    else if (m_passwordInput.isFocused())
    {
        m_focusedInputIndex = 1;
    }
    else if (m_confirmInput.isFocused())
    {
        m_focusedInputIndex = 2;
    }
}

void RegisterPanel::update(float dt)
{
    m_accountInput.update(dt);
    m_passwordInput.update(dt);
    m_confirmInput.update(dt);
}

void RegisterPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const float panelW = 400.f;
    const float panelH = 430.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;

    m_theme->drawPanel(target, sf::FloatRect(px, py, panelW, panelH));
    m_theme->drawTitle(target, u8"注册道号", px + panelW / 2.f, py + 16.f, 28);

    m_accountInput.draw(target);
    m_passwordInput.draw(target);
    m_confirmInput.draw(target);
    m_showPasswordBox.draw(target);
    m_registerButton.draw(target);
    m_backButton.draw(target);
    m_backToZoneButton.draw(target);

    if (!m_message.empty())
    {
        m_theme->drawText(
            target,
            m_message,
            px + 40.f,
            py + 370.f,
            14,
            m_messageIsError ? sf::Color(255, 120, 100) : m_theme->accentColor());
    }
}

void RegisterPanel::setMessage(const std::string& msg, bool isError)
{
    m_message         = msg;
    m_messageIsError  = isError;
}

bool RegisterPanel::validate(std::string& err) const
{
    const std::string& acc = m_accountInput.text();
    const std::string& pwd = m_passwordInput.text();
    const std::string& cfm = m_confirmInput.text();

    if (acc.empty() || pwd.empty())
    {
        err = u8"道号与密钥不可为空";
        return false;
    }
    if (acc.size() > 31 || pwd.size() > 31)
    {
        err = u8"长度不可超过 31 字符";
        return false;
    }
    if (pwd != cfm)
    {
        err = u8"两次密钥不一致";
        return false;
    }
    if (m_zoneId == 0)
    {
        err = u8"请先在登录页选择仙界";
        return false;
    }
    return true;
}

void RegisterPanel::focusInputByIndex(std::size_t idx)
{
    m_accountInput.setFocused(idx == 0);
    m_passwordInput.setFocused(idx == 1);
    m_confirmInput.setFocused(idx == 2);
}
