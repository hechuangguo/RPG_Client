/**
 * @file    Button.h
 * @brief   SFML 仙侠风按钮控件
 *
 * 职责：
 * - 矩形按钮，支持 hover/press 视觉反馈
 * - handleEvent 处理鼠标点击
 *
 * 协作：LoginPanel、RegisterPanel。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <functional>
#include <string>

class UiTheme;

/**
 * @brief 可点击按钮
 */
class Button
{
public:
    Button();

    /**
     * @brief 初始化按钮
     * @param theme  UI 主题
     * @param label  显示文字
     * @param x      左上角 X
     * @param y      左上角 Y
     * @param width  宽度
     * @param height 高度
     */
    void setup(const UiTheme* theme,
               const std::string& label,
               float x, float y,
               float width, float height);

    /** @brief 设置点击回调 */
    void setOnClick(std::function<void()> cb);

    /** @brief 设置是否可用 */
    void setEnabled(bool enabled);

    /** @brief 是否可用 */
    bool isEnabled() const;

    /**
     * @brief 处理 SFML 事件
     * @param event  窗口事件
     * @param window 用于坐标映射
     * @return 本帧被点击返回 true
     */
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    /**
     * @brief 绘制按钮
     * @param target 渲染目标
     */
    void draw(sf::RenderTarget& target) const;

    /** @brief 设置位置 */
    void setPosition(float x, float y);

private:
    const UiTheme*       m_theme;
    sf::FloatRect        m_bounds;
    std::string          m_label;
    std::function<void()> m_onClick;
    bool                 m_enabled;
    bool                 m_hovered;
};
