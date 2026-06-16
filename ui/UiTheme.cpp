/**
 * @file    UiTheme.cpp
 * @brief   UiTheme 实现
 */

#include "ui/UiTheme.h"

#include "log/ClientLogger.h"
#include "util/TextUtil.h"

#include <exception>
#include <algorithm>
#include <new>
#include <string>

namespace
{
bool tryLoadFont(sf::Font& font, const std::string& path)
{
    try
    {
        return font.loadFromFile(path);
    }
    catch (const std::bad_alloc&)
    {
        ClientLogger::instance().warn("UiTheme：字体过大或内存不足：%s", path.c_str());
        return false;
    }
    catch (const std::exception& ex)
    {
        ClientLogger::instance().warn("UiTheme：字体加载失败 %s：%s", path.c_str(), ex.what());
        return false;
    }
}
}  // namespace

UiTheme::UiTheme()
    : m_fontLoaded(false)
    , m_loginBgLoaded(false)
{
}

bool UiTheme::loadFont(const std::string& primaryPath)
{
    m_fontLoaded = false;

    const char* candidates[] = {
        primaryPath.c_str(),
        "assets/fonts/NotoSansSC-Regular.otf",
        "C:/Windows/Fonts/arial.ttf",
    };

    for (const char* path : candidates)
    {
        if (path == nullptr || path[0] == '\0')
        {
            continue;
        }
        if (tryLoadFont(m_font, path))
        {
            m_fontLoaded = true;
            ClientLogger::instance().info("UiTheme：已加载字体 %s", path);
            return true;
        }
        if (path != candidates[2])
        {
            ClientLogger::instance().warn("UiTheme：未找到字体：%s", path);
        }
    }

    ClientLogger::instance().err("UiTheme：没有可用字体");
    return false;
}

bool UiTheme::loadLoginBackground(const std::string& fallbackStaticPath, const std::string& exeDir)
{
    m_loginBgLoaded = false;

    if (!exeDir.empty() && m_loginAnim.load(exeDir))
    {
        m_loginBgLoaded = true;
        ClientLogger::instance().info("UiTheme：使用登录动态背景图集");
        return true;
    }

    if (fallbackStaticPath.empty())
    {
        return false;
    }

    try
    {
        if (m_loginBg.loadFromFile(fallbackStaticPath))
        {
            m_loginBgLoaded = true;
            ClientLogger::instance().info("UiTheme：已加载静态登录背景 %s",
                                          fallbackStaticPath.c_str());
            return true;
        }
    }
    catch (const std::exception& ex)
    {
        ClientLogger::instance().warn("UiTheme：登录背景加载失败 %s：%s",
                                      fallbackStaticPath.c_str(), ex.what());
        return false;
    }

    ClientLogger::instance().warn("UiTheme：未找到登录背景：%s",
                                  fallbackStaticPath.c_str());
    return false;
}

bool UiTheme::hasLoginBackground() const
{
    return m_loginBgLoaded;
}

bool UiTheme::isFontLoaded() const
{
    return m_fontLoaded;
}

const sf::Font& UiTheme::font() const
{
    return m_font;
}

sf::Color UiTheme::panelFill() const
{
    if (m_loginBgLoaded)
    {
        return sf::Color(42, 52, 55, 55);
    }
    return sf::Color(20, 40, 45, 210);
}

sf::Color UiTheme::panelBorder() const
{
    if (m_loginBgLoaded)
    {
        return sf::Color(200, 210, 205, 35);
    }
    return sf::Color(212, 175, 55, 255);
}

sf::Color UiTheme::titleColor() const
{
    if (m_loginBgLoaded)
    {
        return sf::Color(230, 220, 190, 200);
    }
    return sf::Color(212, 175, 55, 255);
}

sf::Color UiTheme::textColor() const
{
    return sf::Color(220, 235, 230, 255);
}

sf::Color UiTheme::accentColor() const
{
    return sf::Color(64, 180, 170, 255);
}

sf::Color UiTheme::buttonNormal() const
{
    if (m_loginBgLoaded)
    {
        return sf::Color(50, 65, 68, 70);
    }
    return sf::Color(30, 70, 75, 230);
}

sf::Color UiTheme::buttonHover() const
{
    if (m_loginBgLoaded)
    {
        return sf::Color(65, 80, 82, 95);
    }
    return sf::Color(45, 100, 105, 240);
}

sf::Color UiTheme::inputFill() const
{
    if (m_loginBgLoaded)
    {
        return sf::Color(35, 48, 50, 60);
    }
    return sf::Color(10, 25, 28, 200);
}

void UiTheme::updateLoginBackground(float dt)
{
    if (m_loginBgLoaded && m_loginAnim.isLoaded())
    {
        m_loginAnim.update(dt);
    }
}

void UiTheme::drawPanel(sf::RenderTarget& target, const sf::FloatRect& rect) const
{
    if (m_loginBgLoaded)
    {
        constexpr float kOuterFeather = 20.f;
        constexpr float kInnerFeather = 10.f;

        sf::RectangleShape outer({rect.width + kOuterFeather * 2.f, rect.height + kOuterFeather * 2.f});
        outer.setPosition(rect.left - kOuterFeather, rect.top - kOuterFeather);
        outer.setFillColor(sf::Color(25, 35, 38, 18));
        target.draw(outer);

        sf::RectangleShape inner({rect.width + kInnerFeather * 2.f, rect.height + kInnerFeather * 2.f});
        inner.setPosition(rect.left - kInnerFeather, rect.top - kInnerFeather);
        inner.setFillColor(sf::Color(30, 40, 42, 28));
        target.draw(inner);

        sf::RectangleShape bg({rect.width, rect.height});
        bg.setPosition(rect.left, rect.top);
        bg.setFillColor(panelFill());
        bg.setOutlineThickness(0.f);
        target.draw(bg);

        sf::RectangleShape highlight({rect.width, 1.f});
        highlight.setPosition(rect.left, rect.top);
        highlight.setFillColor(sf::Color(255, 255, 255, 12));
        target.draw(highlight);
        return;
    }

    sf::RectangleShape bg({rect.width, rect.height});
    bg.setPosition(rect.left, rect.top);
    bg.setFillColor(panelFill());
    bg.setOutlineColor(panelBorder());
    bg.setOutlineThickness(2.f);
    target.draw(bg);
}

void UiTheme::drawTitle(sf::RenderTarget& target,
                        const std::string& text,
                        float centerX,
                        float y,
                        unsigned size) const
{
    drawTextCentered(target, text, centerX, y, size, titleColor(), false);
}

void UiTheme::drawText(sf::RenderTarget& target,
                       const std::string& text,
                       float x,
                       float y,
                       unsigned size,
                       sf::Color color) const
{
    if (!m_fontLoaded)
    {
        return;
    }

    try
    {
        const sf::String sfText = TextUtil::utf8ToSfString(text);
        if (m_loginBgLoaded)
        {
            sf::Text shadow(sfText, m_font, size);
            shadow.setFillColor(sf::Color(10, 15, 18, 80));
            shadow.setPosition(x + 1.f, y + 1.f);
            target.draw(shadow);
        }

        sf::Text label(sfText, m_font, size);
        label.setFillColor(color);
        label.setPosition(x, y);
        target.draw(label);
    }
    catch (const std::bad_alloc&)
    {
    }
}

void UiTheme::drawTextCentered(sf::RenderTarget& target,
                               const std::string& text,
                               float centerX,
                               float centerY,
                               unsigned size,
                               sf::Color color,
                               bool centerVertical) const
{
    if (!m_fontLoaded)
    {
        return;
    }

    try
    {
        const sf::String sfText = TextUtil::utf8ToSfString(text);
        sf::Text probe(sfText, m_font, size);
        const sf::FloatRect bounds = probe.getLocalBounds();

        float originX = bounds.width / 2.f;
        float originY = centerVertical ? bounds.height / 2.f : 0.f;

        if (m_loginBgLoaded)
        {
            sf::Text shadow(sfText, m_font, size);
            shadow.setFillColor(sf::Color(10, 15, 18, 80));
            shadow.setOrigin(originX, originY);
            shadow.setPosition(centerX + 1.f, centerY + 1.f);
            target.draw(shadow);
        }

        sf::Text label(sfText, m_font, size);
        label.setFillColor(color);
        label.setOrigin(originX, originY);
        label.setPosition(centerX, centerY);
        target.draw(label);
    }
    catch (const std::bad_alloc&)
    {
    }
}

float UiTheme::measureTextWidth(const std::string& utf8, unsigned size) const
{
    if (!m_fontLoaded || utf8.empty())
    {
        return 0.f;
    }

    try
    {
        sf::Text probe(TextUtil::utf8ToSfString(utf8), m_font, size);
        return probe.getLocalBounds().width;
    }
    catch (const std::bad_alloc&)
    {
        return 0.f;
    }
}

void UiTheme::drawGradientBackground(sf::RenderTarget& target, const sf::Vector2u& size) const
{
    sf::RectangleShape top({static_cast<float>(size.x), static_cast<float>(size.y) * 0.5f});
    top.setFillColor(sf::Color(15, 35, 40));
    top.setPosition(0.f, 0.f);

    sf::RectangleShape bottom({static_cast<float>(size.x), static_cast<float>(size.y) * 0.5f});
    bottom.setFillColor(sf::Color(25, 55, 50));
    bottom.setPosition(0.f, static_cast<float>(size.y) * 0.5f);

    target.draw(top);
    target.draw(bottom);
}

void UiTheme::drawBackground(sf::RenderTarget& target, const sf::Vector2u& size) const
{
    if (!m_loginBgLoaded)
    {
        drawGradientBackground(target, size);
        return;
    }

    const float windowW = static_cast<float>(size.x);
    const float windowH = static_cast<float>(size.y);

    if (m_loginAnim.isLoaded())
    {
        const float texW = static_cast<float>(m_loginAnim.frameWidth());
        const float texH = static_cast<float>(m_loginAnim.frameHeight());
        if (texW <= 0.f || texH <= 0.f)
        {
            drawGradientBackground(target, size);
            return;
        }

        const float scale   = std::max(windowW / texW, windowH / texH);
        const float offsetX = (windowW - texW * scale) / 2.f;
        const float offsetY = (windowH - texH * scale) / 2.f;
        m_loginAnim.draw(target, scale, offsetX, offsetY);
    }
    else
    {
        const sf::Vector2u texSize = m_loginBg.getSize();
        if (texSize.x == 0 || texSize.y == 0)
        {
            drawGradientBackground(target, size);
            return;
        }

        const float texW  = static_cast<float>(texSize.x);
        const float texH  = static_cast<float>(texSize.y);
        const float scale = std::max(windowW / texW, windowH / texH);

        sf::Sprite sprite(m_loginBg);
        sprite.setScale(scale, scale);
        sprite.setPosition((windowW - texW * scale) / 2.f, (windowH - texH * scale) / 2.f);
        target.draw(sprite);
    }

    sf::RectangleShape scrim({windowW, windowH});
    scrim.setFillColor(sf::Color(15, 25, 30, 35));
    target.draw(scrim);
}
