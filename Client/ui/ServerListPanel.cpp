/**
 * @file    ServerListPanel.cpp
 * @brief   ServerListPanel 实现
 */

#include "ui/ServerListPanel.h"

#include "ui/UiTheme.h"

namespace
{
constexpr float kRowHeight = 36.f;
constexpr float kListTopOffset = 120.f;
}  // namespace

ServerListPanel::ServerListPanel()
    : m_theme(nullptr)
    , m_status(Status::Loading)
    , m_selectedIndex(-1)
{
}

void ServerListPanel::setup(UiTheme* theme, const sf::Vector2u& viewSize)
{
    m_theme    = theme;
    m_viewSize = viewSize;

    const float panelW = 480.f;
    const float panelH = 520.f;
    const float px = (static_cast<float>(viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(viewSize.y) - panelH) / 2.f;

    m_confirmButton.setup(theme, u8"确定", px + 40.f, py + 460.f, 120.f, 40.f);
    m_cancelButton.setup(theme, u8"取消", px + 180.f, py + 460.f, 120.f, 40.f);
    m_retryButton.setup(theme, u8"重试", px + 320.f, py + 460.f, 120.f, 40.f);

    m_confirmButton.setOnClick([this]() {
        if (!m_onConfirm || m_selectedIndex < 0 ||
            m_selectedIndex >= static_cast<int>(m_zones.size()))
        {
            return;
        }
        const GameZoneEntry& zone = m_zones[static_cast<size_t>(m_selectedIndex)];
        if (zone.enabled)
        {
            m_onConfirm(zone);
        }
    });

    m_cancelButton.setOnClick([this]() {
        if (m_onCancel)
        {
            m_onCancel();
        }
    });

    m_retryButton.setOnClick([this]() {
        if (m_onRetry)
        {
            m_onRetry();
        }
    });

    m_confirmButton.setEnabled(false);
    m_retryButton.setEnabled(false);
}

void ServerListPanel::setStatus(Status status, const std::string& message)
{
    m_status        = status;
    m_statusMessage = message;
    m_retryButton.setEnabled(status == Status::Error);
    if (status == Status::Loading)
    {
        m_zones.clear();
        m_selectedIndex = -1;
        m_confirmButton.setEnabled(false);
    }
}

void ServerListPanel::setZones(const std::vector<GameZoneEntry>& zones)
{
    m_zones           = zones;
    m_selectedIndex   = -1;
    m_status          = Status::Ready;
    m_statusMessage.clear();
    m_retryButton.setEnabled(false);
    refreshConfirmButton();
}

void ServerListPanel::setOnConfirm(std::function<void(const GameZoneEntry&)> cb)
{
    m_onConfirm = std::move(cb);
}

void ServerListPanel::setOnCancel(std::function<void()> cb)
{
    m_onCancel = std::move(cb);
}

void ServerListPanel::setOnRetry(std::function<void()> cb)
{
    m_onRetry = std::move(cb);
}

sf::FloatRect ServerListPanel::listAreaRect() const
{
    const float panelW = 480.f;
    const float panelH = 520.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;

    const float listTop = py + kListTopOffset;
    const float listH   = 320.f;
    return sf::FloatRect(px + 30.f, listTop, panelW - 60.f, listH);
}

int ServerListPanel::rowAtPosition(const sf::Vector2f& pos) const
{
    const sf::FloatRect list = listAreaRect();
    if (!list.contains(pos))
    {
        return -1;
    }
    return static_cast<int>((pos.y - list.top) / kRowHeight);
}

void ServerListPanel::refreshConfirmButton()
{
    const bool ok = m_status == Status::Ready && m_selectedIndex >= 0 &&
                    m_selectedIndex < static_cast<int>(m_zones.size()) &&
                    m_zones[static_cast<size_t>(m_selectedIndex)].enabled;
    m_confirmButton.setEnabled(ok);
}

void ServerListPanel::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    m_confirmButton.handleEvent(event, window);
    m_cancelButton.handleEvent(event, window);
    m_retryButton.handleEvent(event, window);

    if (m_status != Status::Ready ||
        event.type != sf::Event::MouseButtonPressed ||
        event.mouseButton.button != sf::Mouse::Left)
    {
        return;
    }

    const sf::Vector2i pixel(event.mouseButton.x, event.mouseButton.y);
    const sf::Vector2f pos = window.mapPixelToCoords(pixel);
    const int row          = rowAtPosition(pos);
    if (row >= 0 && row < static_cast<int>(m_zones.size()) &&
        m_zones[static_cast<size_t>(row)].enabled)
    {
        m_selectedIndex = row;
        refreshConfirmButton();
    }
}

void ServerListPanel::draw(sf::RenderTarget& target) const
{
    if (!m_theme)
    {
        return;
    }

    const float panelW = 480.f;
    const float panelH = 520.f;
    const float px = (static_cast<float>(m_viewSize.x) - panelW) / 2.f;
    const float py = (static_cast<float>(m_viewSize.y) - panelH) / 2.f;

    m_theme->drawPanel(target, sf::FloatRect(px, py, panelW, panelH));
    m_theme->drawTitle(target, u8"选择服务器", px + panelW / 2.f, py + 16.f, 28);

    if (m_status == Status::Loading)
    {
        m_theme->drawText(
            target,
            m_statusMessage.empty() ? u8"正在连接..." : m_statusMessage,
            px + 40.f,
            py + 200.f,
            18,
            m_theme->accentColor());
    }
    else if (m_status == Status::Error)
    {
        m_theme->drawText(
            target,
            m_statusMessage.empty() ? u8"获取区列表失败" : m_statusMessage,
            px + 40.f,
            py + 200.f,
            16,
            sf::Color(255, 120, 100));
    }
    else
    {
        const sf::FloatRect list = listAreaRect();
        sf::RectangleShape listBg({list.width, list.height});
        listBg.setPosition(list.left, list.top);
        listBg.setFillColor(sf::Color(10, 25, 28, 180));
        listBg.setOutlineColor(m_theme->panelBorder());
        listBg.setOutlineThickness(1.f);
        target.draw(listBg);

        float y = list.top + 4.f;
        for (size_t i = 0; i < m_zones.size(); ++i)
        {
            const GameZoneEntry& z = m_zones[i];
            sf::RectangleShape row({list.width - 8.f, kRowHeight - 4.f});
            row.setPosition(list.left + 4.f, y);

            const bool selected = static_cast<int>(i) == m_selectedIndex;
            if (!z.enabled)
            {
                row.setFillColor(sf::Color(40, 40, 40, 180));
            }
            else if (selected)
            {
                row.setFillColor(sf::Color(64, 120, 110, 200));
            }
            else
            {
                row.setFillColor(sf::Color(25, 55, 58, 160));
            }
            target.draw(row);

            std::string label = z.name;
            if (!z.enabled)
            {
                label += u8"（维护中）";
            }
            m_theme->drawText(
                target,
                label,
                list.left + 12.f,
                y + 8.f,
                16,
                z.enabled ? m_theme->textColor() : sf::Color(130, 130, 135));

            y += kRowHeight;
            if (y > list.top + list.height)
            {
                break;
            }
        }
    }

    m_confirmButton.draw(target);
    m_cancelButton.draw(target);
    m_retryButton.draw(target);
}
