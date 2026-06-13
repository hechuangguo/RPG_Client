/**
 * @file    UiTheme.cpp
 * @brief   UiTheme 实现
 */

#include "ui/UiTheme.h"

#include "log/ClientLogger.h"

UiTheme::UiTheme()
    : m_fontLoaded(false)
{
}

bool UiTheme::loadFont(const std::string& primaryPath)
{
    if (m_font.loadFromFile(primaryPath))
    {
        m_fontLoaded = true;
        ClientLogger::instance().info("UiTheme: loaded font %s", primaryPath.c_str());
        return true;
    }

    if (m_font.loadFromFile("C:/Windows/Fonts/msyh.ttc") ||
        m_font.loadFromFile("C:/Windows/Fonts/simhei.ttf") ||
        m_font.loadFromFile("C:/Windows/Fonts/arial.ttf"))
    {
        m_fontLoaded = true;
        ClientLogger::instance().warn("UiTheme: fallback system font");
        return true;
    }

    ClientLogger::instance().err("UiTheme: no font available");
    m_fontLoaded = false;
    return false;
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
    if (!m_fontLoaded)
    {
        return;
    }

    sf::Text title(text, m_font, size);
    title.setFillColor(titleColor());
    const sf::FloatRect bounds = title.getLocalBounds();
    title.setOrigin(bounds.width / 2.f, 0.f);
    title.setPosition(centerX, y);
    target.draw(title);
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
