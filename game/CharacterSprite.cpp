/**
 * @file    CharacterSprite.cpp
 * @brief   CharacterSprite 实现
 */

#include "game/CharacterSprite.h"

#include "log/ClientLogger.h"
#include "net/CharacterTypes.h"
#include "util/PathUtil.h"

#include <cmath>
#include <fstream>
#include <sstream>

namespace
{
constexpr int kProcFrameW     = 48;
constexpr int kProcFrameH     = 72;
constexpr int kDirections     = 4;
constexpr int kWalkFrames     = 4;
constexpr float kDefaultScale = 1.8f;

void setPixel(sf::Image& img, int x, int y, sf::Color c)
{
    if (x >= 0 && y >= 0 && x < static_cast<int>(img.getSize().x) &&
        y < static_cast<int>(img.getSize().y))
    {
        img.setPixel(static_cast<unsigned>(x), static_cast<unsigned>(y), c);
    }
}

void fillRect(sf::Image& img, int x, int y, int w, int h, sf::Color c)
{
    for (int py = y; py < y + h; ++py)
    {
        for (int px = x; px < x + w; ++px)
        {
            setPixel(img, px, py, c);
        }
    }
}

void drawSwordsmanFrame(sf::Image& img,
                        int frameX,
                        int frameY,
                        int frame,
                        uint8_t vocation,
                        uint8_t sex)
{
    const sf::Color robe = vocation == CharacterDef::kVocationMage
                               ? sf::Color(80, 110, 170)
                               : sf::Color(70, 130, 140);
    const sf::Color robeDark = vocation == CharacterDef::kVocationMage
                                   ? sf::Color(55, 80, 130)
                                   : sf::Color(50, 95, 105);
    const sf::Color skin =
        sex == CharacterDef::kSexFemale ? sf::Color(255, 220, 200) : sf::Color(245, 205, 175);
    const sf::Color hair =
        sex == CharacterDef::kSexFemale ? sf::Color(50, 30, 20) : sf::Color(30, 25, 20);

    const int ox = frameX + 10;
    const int oy = frameY + 6 + (frame % 2);
    const int legSwing = (frame % 2 == 0) ? 0 : 2;

    fillRect(img, ox + 14, oy, 20, 14, hair);
    fillRect(img, ox + 15, oy + 4, 18, 12, skin);
    fillRect(img, ox + 10, oy + 16, 28, 30, robe);
    fillRect(img, ox + 12, oy + 40, 24, 5, sf::Color(200, 170, 80));
    fillRect(img, ox + 12 + legSwing, oy + 46, 10, 20, robeDark);
    fillRect(img, ox + 26 - legSwing, oy + 46, 10, 20, robeDark);
    fillRect(img, ox + 34, oy + 20, 5, 32, sf::Color(210, 220, 235));
}
}  // namespace

CharacterSprite::CharacterSprite()
    : m_frameW(kProcFrameW)
    , m_frameH(kProcFrameH)
    , m_frameCount(kWalkFrames)
    , m_fps(8.f)
    , m_drawScale(kDefaultScale)
    , m_ready(false)
{
}

std::string CharacterSprite::pngPathFor(uint8_t vocation, uint8_t sex) const
{
    std::string name = "warrior_";
    if (vocation == CharacterDef::kVocationMage)
    {
        name = "mage_";
    }
    name += (sex == CharacterDef::kSexFemale) ? "female" : "male";
    return PathUtil::joinPath(
        PathUtil::joinPath(PathUtil::joinPath(PathUtil::getExeDir(), "assets"),
                           "characters"),
        PathUtil::joinPath("player", name + ".png"));
}

bool CharacterSprite::loadAnimJson(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string content = ss.str();

    auto readInt = [&](const std::string& key, int& out) -> bool {
        const size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos)
        {
            return false;
        }
        const size_t colon = content.find(':', pos);
        if (colon == std::string::npos)
        {
            return false;
        }
        out = std::stoi(content.substr(colon + 1));
        return true;
    };

    auto readFloat = [&](const std::string& key, float& out) -> bool {
        const size_t pos = content.find("\"" + key + "\"");
        if (pos == std::string::npos)
        {
            return false;
        }
        const size_t colon = content.find(':', pos);
        if (colon == std::string::npos)
        {
            return false;
        }
        out = std::stof(content.substr(colon + 1));
        return true;
    };

    int fw = m_frameW;
    int fh = m_frameH;
    int fc = m_frameCount;
    float fps = m_fps;
    readInt("frameWidth", fw);
    readInt("frameHeight", fh);
    readInt("framesPerDir", fc);
    readFloat("fps", fps);
    m_frameW     = fw;
    m_frameH     = fh;
    m_frameCount = std::max(1, fc);
    m_fps        = fps;
    return true;
}

bool CharacterSprite::loadFromPng(const std::string& path)
{
    if (!m_texture.loadFromFile(path))
    {
        return false;
    }
    m_texture.setSmooth(false);
    const std::string animPath = PathUtil::joinPath(
        PathUtil::joinPath(
            PathUtil::joinPath(PathUtil::getExeDir(), "assets"), "characters"),
        PathUtil::joinPath("player", "anim.json"));
    loadAnimJson(animPath);
    m_loadSource = path;
    return true;
}

bool CharacterSprite::load(uint8_t vocation, uint8_t sex)
{
    const std::string pngPath = pngPathFor(vocation, sex);
    if (loadFromPng(pngPath))
    {
        m_ready = true;
        return true;
    }

    generateProceduralSheet(vocation, sex);
    m_loadSource = "procedural";
    m_ready      = true;
    return true;
}

float CharacterSprite::drawHeight(float scale) const
{
    return static_cast<float>(m_frameH) * m_drawScale * scale;
}

void CharacterSprite::generateProceduralSheet(uint8_t vocation, uint8_t sex)
{
    m_frameW     = kProcFrameW;
    m_frameH     = kProcFrameH;
    m_frameCount = kWalkFrames;
    m_fps        = 8.f;

    const unsigned texW = static_cast<unsigned>(kProcFrameW * kWalkFrames);
    const unsigned texH = static_cast<unsigned>(kProcFrameH * kDirections);
    sf::Image image;
    image.create(texW, texH, sf::Color::Transparent);

    for (int dir = 0; dir < kDirections; ++dir)
    {
        for (int frame = 0; frame < kWalkFrames; ++frame)
        {
            drawSwordsmanFrame(image,
                               frame * kProcFrameW,
                               dir * kProcFrameH,
                               frame + dir,
                               vocation,
                               sex);
        }
    }

    m_texture.loadFromImage(image);
    m_texture.setSmooth(false);
}

int CharacterSprite::directionFromRad(float dirRad)
{
    float a = dirRad;
    while (a < 0.f)
    {
        a += 6.2831853f;
    }
    while (a >= 6.2831853f)
    {
        a -= 6.2831853f;
    }

    if (a < 0.7853981f || a >= 5.4977871f)
    {
        return 2;
    }
    if (a < 2.3561944f)
    {
        return 3;
    }
    if (a < 3.9269908f)
    {
        return 0;
    }
    return 1;
}

void CharacterSprite::draw(sf::RenderTarget& target,
                           float screenX,
                           float screenY,
                           float dirRad,
                           bool moving,
                           float animTime,
                           float scale) const
{
    if (!m_ready)
    {
        return;
    }

    const int direction = directionFromRad(dirRad);
    int frame           = 0;
    if (moving)
    {
        frame = static_cast<int>(animTime * m_fps) % m_frameCount;
    }
    drawFrame(target, screenX, screenY, direction, frame, scale);
}

void CharacterSprite::drawFrame(sf::RenderTarget& target,
                                float screenX,
                                float screenY,
                                int direction,
                                int frame,
                                float scale) const
{
    const float totalScale = m_drawScale * scale;
    sf::Sprite sprite(m_texture);
    sprite.setTextureRect(
        sf::IntRect(frame * m_frameW, direction * m_frameH, m_frameW, m_frameH));
    sprite.setOrigin(static_cast<float>(m_frameW) / 2.f,
                     static_cast<float>(m_frameH));
    sprite.setScale(totalScale, totalScale);
    sprite.setPosition(screenX, screenY);
    target.draw(sprite);
}

CharacterSpriteLibrary& CharacterSpriteLibrary::instance()
{
    static CharacterSpriteLibrary lib;
    return lib;
}

void CharacterSpriteLibrary::preload()
{
    ensureLoaded();
}

void CharacterSpriteLibrary::ensureLoaded()
{
    if (m_init)
    {
        return;
    }
    m_warriorMale.load(CharacterDef::kVocationWarrior, CharacterDef::kSexMale);
    m_warriorFemale.load(CharacterDef::kVocationWarrior, CharacterDef::kSexFemale);
    m_mageMale.load(CharacterDef::kVocationMage, CharacterDef::kSexMale);
    m_mageFemale.load(CharacterDef::kVocationMage, CharacterDef::kSexFemale);

    ClientLogger::instance().info(
        "CharacterSprite：已加载 warrior_male=%s",
        m_warriorMale.loadSource().c_str());
    ClientLogger::instance().info(
        "CharacterSprite：已加载 warrior_female=%s",
        m_warriorFemale.loadSource().c_str());

    m_init = true;
}

CharacterSprite& CharacterSpriteLibrary::get(uint8_t vocation, uint8_t sex)
{
    ensureLoaded();
    if (vocation == CharacterDef::kVocationMage)
    {
        return sex == CharacterDef::kSexFemale ? m_mageFemale : m_mageMale;
    }
    return sex == CharacterDef::kSexFemale ? m_warriorFemale : m_warriorMale;
}

const CharacterSprite& CharacterSpriteLibrary::get(uint8_t vocation, uint8_t sex) const
{
    return const_cast<CharacterSpriteLibrary*>(this)->get(vocation, sex);
}
