/**
 * @file    UiTheme.h
 * @brief   仙侠风 UI 主题与通用绘制
 *
 * 职责：
 * - 定义金色/青色配色方案
 * - loadFont：优先 assets/fonts/NotoSansSC-Regular.otf，fallback 到 Arial
 * - drawPanel、drawTitle 等通用 UI 绘制辅助
 *
 * 协作：LoginPanel、RegisterPanel、GameScene HUD。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ui/LoginBackgroundAnim.h"

#include <SFML/Graphics.hpp>

#include <string>

/**
 * @brief 仙侠风 UI 主题（静态配色 + 实例字体）
 */
class UiTheme
{
public:
    UiTheme();

    /**
     * @brief 加载 UI 字体
     * @param primaryPath 首选字体路径（如 assets/fonts/NotoSansSC-Regular.otf）
     * @return 加载成功返回 true（含 fallback）
     */
    bool loadFont(const std::string& primaryPath);

    /**
     * @brief 加载登录流程背景（优先全画面序列帧，失败则静态图）
     * @param fallbackStaticPath 静态回退路径，如 assets/ui/login_bg.png
     * @param exeDir             可执行文件目录（用于加载 login_bg_anim.json）
     * @return 任一背景加载成功返回 true；失败时 drawBackground 回退渐变
     */
    bool loadLoginBackground(const std::string& fallbackStaticPath, const std::string& exeDir);

    /** @brief 登录背景图是否已加载 */
    bool hasLoginBackground() const;

    /** @brief 当前可用字体 */
    const sf::Font& font() const;

    /** @brief 字体是否已成功加载 */
    bool isFontLoaded() const;

    /** @brief 面板背景色 */
    sf::Color panelFill() const;

    /** @brief 面板描边色（金色） */
    sf::Color panelBorder() const;

    /** @brief 标题文字色 */
    sf::Color titleColor() const;

    /** @brief 正文文字色 */
    sf::Color textColor() const;

    /** @brief 强调色（青色） */
    sf::Color accentColor() const;

    /** @brief 按钮普通态 */
    sf::Color buttonNormal() const;

    /** @brief 按钮悬停态 */
    sf::Color buttonHover() const;

    /** @brief 输入框背景色 */
    sf::Color inputFill() const;

    /** @brief 更新登录背景序列帧动画 */
    void updateLoginBackground(float dt);

    /**
     * @brief 绘制带金色描边的半透明面板
     * @param target 渲染目标
     * @param rect   面板区域
     */
    void drawPanel(sf::RenderTarget& target, const sf::FloatRect& rect) const;

    /**
     * @brief 绘制居中标题
     * @param target 渲染目标
     * @param text   标题文字
     * @param centerX 中心 X
     * @param y      顶部 Y
     * @param size   字号
     */
    void drawTitle(sf::RenderTarget& target,
                   const std::string& text,
                   float centerX,
                   float y,
                   unsigned size) const;

    /**
     * @brief 安全绘制文字（捕获 std::bad_alloc，字体未加载时跳过）
     */
    void drawText(sf::RenderTarget& target,
                  const std::string& text,
                  float x,
                  float y,
                  unsigned size,
                  sf::Color color) const;

    /**
     * @brief 安全绘制居中文字
     */
    void drawTextCentered(sf::RenderTarget& target,
                          const std::string& text,
                          float centerX,
                          float centerY,
                          unsigned size,
                          sf::Color color,
                          bool centerVertical = false) const;

    /** @brief 测量 UTF-8 文本渲染宽度（像素）；字体未加载返回 0 */
    float measureTextWidth(const std::string& utf8, unsigned size) const;

    /**
     * @brief 绘制登录背景（图片 cover 铺满，失败则渐变）
     * @param target 渲染目标
     * @param size   窗口尺寸
     */
    void drawBackground(sf::RenderTarget& target, const sf::Vector2u& size) const;

private:
    void drawGradientBackground(sf::RenderTarget& target, const sf::Vector2u& size) const;

    sf::Font    m_font;
    bool        m_fontLoaded;
    sf::Texture           m_loginBg;
    bool                  m_loginBgLoaded;
    LoginBackgroundAnim   m_loginAnim;
};
