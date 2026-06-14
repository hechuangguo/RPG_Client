/**
 * @file    UiTheme.cpp
 * @brief   UiTheme 实现
 */

#include "ui/UiTheme.h"

#include "log/ClientLogger.h"
#include "util/TextUtil.h"

#include <exception>
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
        ClientLogger::instance().warn("UiTheme: font too large or OOM: %s", path.c_str());
        return false;
    }
    catch (const std::exception& ex)
    {
        ClientLogger::instance().warn("UiTheme: font load failed %s: %s", path.c_str(), ex.what());
        return false;
    }
}
}  // namespace

UiTheme::UiTheme()
    : m_fontLoaded(false)
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
            ClientLogger::instance().info("UiTheme: loaded font %s", path);
            return true;
        }
        if (path != candidates[2])
        {
            ClientLogger::instance().warn("UiTheme: font not found: %s", path);
        }
    }

    ClientLogger::instance().err("UiTheme: no font available");
    return false;
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
    return sf::Color(20, 40, 45, 210);
}

sf::Color UiTheme::panelBorder() const
{
    return sf::Color(212, 175, 55, 255);
}

sf::Color UiTheme::titleColor() const
{
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
    return sf::Color(30, 70, 75, 230);
}

sf::Color UiTheme::buttonHover() const
{
    return sf::Color(45, 100, 105, 240);
}

void UiTheme::drawPanel(sf::RenderTarget& target, const sf::FloatRect& rect) const
{
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
        sf::Text label(TextUtil::utf8ToSfString(text), m_font, size);
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
        sf::Text label(TextUtil::utf8ToSfString(text), m_font, size);
        label.setFillColor(color);
        const sf::FloatRect bounds = label.getLocalBounds();
        if (centerVertical)
        {
            label.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        }
        else
        {
            label.setOrigin(bounds.width / 2.f, 0.f);
        }
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

void UiTheme::drawBackground(sf::RenderTarget& target, const sf::Vector2u& size) const
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
