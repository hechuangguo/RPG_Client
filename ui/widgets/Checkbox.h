/**
 * @file    Checkbox.h
 * @brief   SFML 复选框控件
 *
 * 职责：
 * - 勾选/取消勾选，带标签文字
 *
 * 协作：LoginPanel（记住账号）。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <string>

class UiTheme;

/**
 * @brief 复选框
 */
class Checkbox
{
public:
    Checkbox();

    /**
     * @brief 初始化复选框
     * @param theme UI 主题
     * @param label 标签文字
     * @param x     左上角 X
     * @param y     左上角 Y
     */
    void setup(const UiTheme* theme, const std::string& label, float x, float y);

    /** @brief 处理 SFML 事件 */
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    /** @brief 绘制 */
    void draw(sf::RenderTarget& target) const;

    /** @brief 是否勾选 */
    bool isChecked() const;

    /** @brief 设置勾选状态 */
    void setChecked(bool checked);

private:
    const UiTheme* m_theme;
    sf::FloatRect  m_boxBounds;
    std::string    m_label;
    bool           m_checked;
};
