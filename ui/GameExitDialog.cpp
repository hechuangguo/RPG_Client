/**
 * @file    GameExitDialog.cpp
 * @brief   GameExitDialog 实现
 */

#include "ui/GameExitDialog.h"

#include "ui/UiTheme.h"

namespace
{
constexpr float kPanelW    = 420.f;
constexpr float kPanelH    = 360.f;
constexpr float kBtnW      = 320.f;
constexpr float kBtnH      = 40.f;
constexpr float kBtnStartY = 150.f;
constexpr float kBtnGap    = 12.f;
}  // namespace

GameExitDialog::GameExitDialog()
    : m_theme(nullptr)
    , m_visible(false)
    , m_busy(false)
{
}

void GameExitDialog::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_viewSize = viewSize;
    layout(viewSize);
}

void GameExitDialog::layout(const sf::Vector2u& viewSize)
{
    if (!m_theme)
    {
        return;
    }

    m_viewSize = viewSize;
    const sf::FloatRect panel = panelRect();
    const float px          = panel.left;
    const float btnX        = px + (panel.width - kBtnW) / 2.f;

    m_returnCharSelectBtn.setup(m_theme, u8"返回选角", btnX, panel.top + kBtnStartY, kBtnW, kBtnH);
    m_returnLoginBtn.setup(
        m_theme, u8"返回登录", btnX, panel.top + kBtnStartY + kBtnH + kBtnGap, kBtnW, kBtnH);
    m_quitClientBtn.setup(m_theme,
                          u8"退出游戏",
                          btnX,
                          panel.top + kBtnStartY + (kBtnH + kBtnGap) * 2.f,
                          kBtnW,
                          kBtnH);
    m_cancelBtn.setup(m_theme,
                      u8"取消",
                      btnX,
                      panel.top + kBtnStartY + (kBtnH + kBtnGap) * 3.f + 8.f,
                      kBtnW,
                      36.f);

    m_returnCharSelectBtn.setOnClick([this]() {
        if (m_busy || !m_onReturnCharSelect)
        {
            return;
        }
        m_onReturnCharSelect();
    });
    m_returnLoginBtn.setOnClick([this]() {
        if (m_busy || !m_onReturnLogin)
        {
            return;
        }
        m_onReturnLogin();
    });
    m_quitClientBtn.setOnClick([this]() {
        if (m_busy || !m_onQuitClient)
        {
            return;
        }
        m_onQuitClient();
    });
    m_cancelBtn.setOnClick([this]() { hide(); });
}

void GameExitDialog::show()
{
    m_visible     = true;
    m_busy        = false;
    m_busyMessage.clear();
    const bool enabled = true;
    m_returnCharSelectBtn.setEnabled(enabled);
    m_returnLoginBtn.setEnabled(enabled);
    m_quitClientBtn.setEnabled(enabled);
    m_cancelBtn.setEnabled(enabled);
}

void GameExitDialog::hide()
{
    m_visible     = false;
    m_busy        = false;
    m_busyMessage.clear();
}

bool GameExitDialog::isVisible() const
{
    return m_visible;
}

void GameExitDialog::setBusy(bool busy, const std::string& message)
{
    m_busy        = busy;
    m_busyMessage = message;
    const bool enabled = !m_busy;
    m_returnCharSelectBtn.setEnabled(enabled);
    m_returnLoginBtn.setEnabled(enabled);
    m_quitClientBtn.setEnabled(enabled);
    m_cancelBtn.setEnabled(enabled);
}

void GameExitDialog::setOnReturnCharSelect(std::function<void()> cb)
{
    m_onReturnCharSelect = std::move(cb);
}

void GameExitDialog::setOnReturnLogin(std::function<void()> cb)
{
    m_onReturnLogin = std::move(cb);
}

void GameExitDialog::setOnQuitClient(std::function<void()> cb)
{
    m_onQuitClient = std::move(cb);
}

sf::FloatRect GameExitDialog::panelRect() const
{
    const float px = (static_cast<float>(m_viewSize.x) - kPanelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - kPanelH) / 2.f;
    return sf::FloatRect(px, py, kPanelW, kPanelH);
}

bool GameExitDialog::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_visible)
    {
        return false;
    }

    m_returnCharSelectBtn.handleEvent(event, window);
    m_returnLoginBtn.handleEvent(event, window);
    m_quitClientBtn.handleEvent(event, window);
    m_cancelBtn.handleEvent(event, window);
    return true;
}

void GameExitDialog::draw(sf::RenderTarget& target) const
{
    if (!m_visible || !m_theme)
    {
        return;
    }

    sf::RectangleShape overlay(
        sf::Vector2f(static_cast<float>(m_viewSize.x), static_cast<float>(m_viewSize.y)));
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    target.draw(overlay);

    const sf::FloatRect panel = panelRect();
    m_theme->drawPanel(target, panel);
    m_theme->drawTitle(target, u8"退出游戏", panel.left + panel.width / 2.f, panel.top + 16.f, 26);

    if (!m_busyMessage.empty())
    {
        m_theme->drawText(target,
                          m_busyMessage,
                          panel.left + 24.f,
                          panel.top + 56.f,
                          15,
                          m_theme->accentColor());
    }
    else
    {
        m_theme->drawText(target,
                          u8"请选择下一步操作",
                          panel.left + 24.f,
                          panel.top + 56.f,
                          15,
                          m_theme->textColor());
    }

    m_returnCharSelectBtn.draw(target);
    m_returnLoginBtn.draw(target);
    m_quitClientBtn.draw(target);
    m_cancelBtn.draw(target);
}
