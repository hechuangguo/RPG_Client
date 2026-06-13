/**
 * @file    ZoneSelectPanel.h
 * @brief   游戏区服选择面板
 *
 * 职责：
 * - 从 ServerListLoader 列出可选游戏区
 * - 维护区灰显不可选；必须选中一区后登录才可用
 *
 * 协作：LoginPanel、ServerListLoader。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>

class ServerListLoader;
class UiTheme;

/**
 * @brief 区服列表面板
 */
class ZoneSelectPanel
{
public:
    ZoneSelectPanel();

    /**
     * @brief 初始化面板布局
     * @param theme  UI 主题
     * @param loader 区服列表
     * @param x      左上角 X
     * @param y      左上角 Y
     * @param width  宽度
     * @param height 高度
     */
    void setup(const UiTheme* theme,
               const ServerListLoader* loader,
               float x, float y,
               float width, float height);

    /** @brief 处理 SFML 事件 */
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    /** @brief 绘制区服列表 */
    void draw(sf::RenderTarget& target) const;

    /** @brief 当前选中 zoneId（0 表示未选） */
    uint32_t selectedZoneId() const;

    /** @brief 当前选中 gameType */
    uint8_t selectedGameType() const;

    /** @brief 是否已选中可用区服 */
    bool hasValidSelection() const;

    /**
     * @brief 恢复上次选中区服
     * @param zoneId 区号
     */
    void selectZoneId(uint32_t zoneId);

private:
    const UiTheme*          m_theme;
    const ServerListLoader* m_loader;
    sf::FloatRect           m_bounds;
    int                     m_selectedIndex;
    float                   m_rowHeight;
};
