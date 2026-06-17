/**
 * @file    ZoneHomePanel.cpp
 * @brief   ZoneHomePanel 实现
 */

#include "ui/ZoneHomePanel.h"

#include "ui/UiTheme.h"

ZoneHomePanel::ZoneHomePanel()
    : m_theme(nullptr)
    , m_hasValidZone(false)
{
}

void ZoneHomePanel::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_viewSize = viewSize;

    const float panelW = 420.f;
    const float panelH = 320.f;
    const float px = (static_cast<float>(viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(viewSize.y) - panelH) / 2.f;

    m_selectServerButton.setup(theme, u8"选择服务器", px + 40.f, py + 180.f, 340.f, 44.f);
    m_enterGameButton.setup(theme, u8"进入游戏", px + 40.f, py + 240.f, 340.f, 44.f);

    m_selectServerButton.setOnClick([this]() {
        if (m_onSelectServer)
        {
            m_onSelectServer();
        }
    });

    m_enterGameButton.setOnClick([this]() {
        if (m_onEnterGame && m_hasValidZone)
        {
            m_onEnterGame();
        }
    });

    m_enterGameButton.setEnabled(false);
}

void ZoneHomePanel::setCurrentZoneName(const std::string& name)
{
    m_zoneName = name;
}

void ZoneHomePanel::setHasValidZone(bool valid)
{
    m_hasValidZone = valid;
    m_enterGameButton.setEnabled(valid);
}

void ZoneHomePanel::setOnSelectServer(std::function<void()> cb)
{
    m_onSelectServer = std::move(cb);
}

void ZoneHomePanel::setOnEnterGame(std::function<void()> cb)
{
    m_onEnterGame = std::move(cb);
}

void ZoneHomePanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    m_selectServerButton.handleEvent(event, window);
    m_enterGameButton.handleEvent(event, window);
}

void ZoneHomePanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const float panelW = 420.f;
    const float panelH = 320.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;

    m_theme->drawPanel(target, sf::FloatRect(px, py, panelW, panelH));
    m_theme->drawTitle(target, u8"仙侠世界", px + panelW / 2.f, py + 16.f, 32);

    const std::string zoneLabel =
        m_zoneName.empty() ? u8"未选择" : m_zoneName;
    const std::string line = u8"当前游戏区：" + zoneLabel;
    m_theme->drawText(
        target,
        line,
        px + 40.f,
        py + 100.f,
        18,
        m_hasValidZone ? m_theme->textColor() : sf::Color(120, 140, 135));

    m_selectServerButton.draw(target);
    m_enterGameButton.draw(target);
}
