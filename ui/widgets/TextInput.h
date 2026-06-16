/**
 * @file    TextInput.h
 * @brief   SFML 单行文本输入框
 *
 * 职责：
 * - 支持 TextEntered 输入、Backspace 删除
 * - 密码模式掩码显示
 * - 聚焦时闪烁光标
 *
 * 协作：LoginPanel、RegisterPanel。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <string>

class UiTheme;

/**
 * @brief 单行文本输入控件
 */
class TextInput
{
public:
    TextInput();

    /**
     * @brief 初始化输入框
     * @param theme       UI 主题
     * @param placeholder 占位提示
     * @param x           左上角 X
     * @param y           左上角 Y
     * @param width       宽度
     * @param height      高度
     * @param password    是否密码模式
     */
    void setup(const UiTheme* theme,
               const std::string& placeholder,
               float x, float y,
               float width, float height,
               bool password = false);

    /** @brief 处理 SFML 事件 */
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    /** @brief 更新光标闪烁 */
    void update(float dt);

    /** @brief 绘制输入框 */
    void draw(sf::RenderTarget& target) const;

    /** @brief 当前文本 */
    const std::string& text() const;

    /** @brief 设置文本 */
    void setText(const std::string& text);

    /** @brief 清空 */
    void clear();

    /** @brief 是否聚焦 */
    bool isFocused() const;

    /** @brief 设置聚焦状态（供父面板切换焦点） */
    void setFocused(bool focused);

    /** @brief 是否为密码输入框 */
    bool isPasswordField() const;

    /** @brief 设置是否掩码显示（true=掩码，false=明文） */
    void setPasswordMasked(bool masked);

    /** @brief 当前是否掩码显示 */
    bool isPasswordMasked() const;

private:
    std::string displayText() const;
    void syncCursorPos();

    const UiTheme* m_theme;
    sf::FloatRect  m_bounds;
    std::string    m_text;
    std::string    m_placeholder;
    bool           m_password;
    bool           m_focused;
    std::size_t    m_cursorPos;
    float          m_blinkElapsed;
    bool           m_cursorVisible;
};
