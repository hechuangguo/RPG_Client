/**
 * @file    LoginBackgroundAnim.cpp
 * @brief   LoginBackgroundAnim 实现
 */

#include "ui/LoginBackgroundAnim.h"

#include "log/ClientLogger.h"
#include "util/PathUtil.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
std::string readTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool findNumberAfterKey(const std::string& json, const std::string& key, double& out)
{
    const std::string needle = "\"" + key + "\"";
    const size_t pos         = json.find(needle);
    if (pos == std::string::npos)
    {
        return false;
    }

    size_t colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos)
    {
        return false;
    }

    size_t i = colon + 1;
    while (i < json.size() && std::isspace(static_cast<unsigned char>(json[i])))
    {
        ++i;
    }

    size_t end = i;
    while (end < json.size() &&
           (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '.' ||
            json[end] == '-' || json[end] == '+'))
    {
        ++end;
    }

    if (end == i)
    {
        return false;
    }

    try
    {
        out = std::stod(json.substr(i, end - i));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool findStringAfterKey(const std::string& json, const std::string& key, std::string& out)
{
    const std::string needle = "\"" + key + "\"";
    const size_t pos         = json.find(needle);
    if (pos == std::string::npos)
    {
        return false;
    }

    const size_t quoteStart = json.find('"', pos + needle.size());
    if (quoteStart == std::string::npos)
    {
        return false;
    }

    const size_t quoteEnd = json.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos)
    {
        return false;
    }

    out = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    return true;
}

bool tryLoadTexture(sf::Texture& texture, const std::string& path)
{
    try
    {
        return texture.loadFromFile(path);
    }
    catch (const std::exception& ex)
    {
        ClientLogger::instance().warn("LoginBackgroundAnim：纹理加载失败 %s：%s",
                                      path.c_str(), ex.what());
        return false;
    }
}
}  // namespace

LoginBackgroundAnim::LoginBackgroundAnim()
    : m_frames(10)
    , m_fps(8.f)
    , m_frameTimer(0.f)
    , m_frameIndex(0)
    , m_loaded(false)
{
}

bool LoginBackgroundAnim::load(const std::string& exeDir)
{
    m_loaded      = false;
    m_frameIndex  = 0;
    m_frameTimer  = 0.f;
    m_frames      = 10;
    m_fps         = 8.f;

    const std::string uiDir    = PathUtil::joinPath(exeDir, "assets/ui");
    const std::string jsonPath = PathUtil::joinPath(uiDir, "login_bg_anim.json");
    const std::string json     = readTextFile(jsonPath);

    std::string sheetName = "login_bg_sheet.png";
    if (!json.empty())
    {
        double value = 0.0;
        if (findNumberAfterKey(json, "frames", value) && value > 0.0)
        {
            m_frames = static_cast<int>(value);
        }
        if (findNumberAfterKey(json, "fps", value) && value > 0.0)
        {
            m_fps = static_cast<float>(value);
        }
        findStringAfterKey(json, "sheet", sheetName);
    }
    else
    {
        ClientLogger::instance().warn("LoginBackgroundAnim：未找到配置文件：%s", jsonPath.c_str());
    }

    const std::string sheetPath = PathUtil::joinPath(uiDir, sheetName);
    if (!tryLoadTexture(m_texture, sheetPath))
    {
        ClientLogger::instance().warn("LoginBackgroundAnim：未找到图集：%s", sheetPath.c_str());
        return false;
    }

    const sf::Vector2u texSize = m_texture.getSize();
    if (texSize.x == 0 || texSize.y == 0 || m_frames <= 0)
    {
        ClientLogger::instance().warn("LoginBackgroundAnim：图集尺寸或帧数无效");
        return false;
    }

    if (texSize.x / static_cast<unsigned>(m_frames) == 0)
    {
        ClientLogger::instance().warn("LoginBackgroundAnim：图集尺寸或帧数无效");
        return false;
    }

    const unsigned frameW = texSize.x / static_cast<unsigned>(m_frames);
    const float    aspect   = static_cast<float>(frameW) / static_cast<float>(texSize.y);
    if (aspect < 1.4f || aspect > 2.1f)
    {
        ClientLogger::instance().warn(
            "LoginBackgroundAnim：图集布局无效（帧宽高比 %.2f），回退静态背景",
            aspect);
        return false;
    }

    m_loaded = true;
    ClientLogger::instance().info("LoginBackgroundAnim：已加载 %s（%d 帧，%.1f fps）",
                                  sheetPath.c_str(),
                                  m_frames,
                                  m_fps);
    return true;
}

bool LoginBackgroundAnim::isLoaded() const
{
    return m_loaded;
}

unsigned LoginBackgroundAnim::frameWidth() const
{
    if (!m_loaded || m_frames <= 0)
    {
        return 0;
    }
    return m_texture.getSize().x / static_cast<unsigned>(m_frames);
}

unsigned LoginBackgroundAnim::frameHeight() const
{
    if (!m_loaded)
    {
        return 0;
    }
    return m_texture.getSize().y;
}

void LoginBackgroundAnim::update(float dt)
{
    if (!m_loaded || m_frames <= 0 || m_fps <= 0.f)
    {
        return;
    }

    m_frameTimer += dt;
    const float frameDuration = 1.f / m_fps;
    while (m_frameTimer >= frameDuration)
    {
        m_frameTimer -= frameDuration;
        m_frameIndex = (m_frameIndex + 1) % m_frames;
    }
}

void LoginBackgroundAnim::draw(sf::RenderTarget& target,
                               float scale,
                               float offsetX,
                               float offsetY) const
{
    if (!m_loaded || m_frames <= 0)
    {
        return;
    }

    const unsigned frameW = frameWidth();
    const unsigned frameH = frameHeight();
    if (frameW == 0 || frameH == 0)
    {
        return;
    }

    sf::Sprite sprite(m_texture);
    sprite.setTextureRect(sf::IntRect(static_cast<int>(m_frameIndex * frameW),
                                      0,
                                      static_cast<int>(frameW),
                                      static_cast<int>(frameH)));
    sprite.setScale(scale, scale);
    sprite.setPosition(offsetX, offsetY);
    target.draw(sprite);
}
