/**
 * @file    MapRenderer.cpp
 * @brief   MapRenderer 实现
 */

#include "game/MapRenderer.h"

#include "log/ClientLogger.h"
#include "math/Random.h"
#include "util/PathUtil.h"

#include <fstream>
#include <sstream>

MapRenderer::MapRenderer()
    : m_tileW(32.f)
    , m_tileH(32.f)
    , m_width(0)
    , m_height(0)
{
}

bool MapRenderer::load(uint32_t mapId)
{
    const std::string path = PathUtil::joinPath(PathUtil::mapPath(mapId), "ground.json");
    if (parseGroundJson(path))
    {
        ClientLogger::instance().info("MapRenderer: loaded %s", path.c_str());
        return true;
    }

    ClientLogger::instance().warn("MapRenderer: fallback default map for %u", mapId);
    generateDefaultMap();
    return true;
}

float MapRenderer::tileWidth() const
{
    return m_tileW;
}

float MapRenderer::tileHeight() const
{
    return m_tileH;
}

int MapRenderer::mapWidthTiles() const
{
    return m_width;
}

int MapRenderer::mapHeightTiles() const
{
    return m_height;
}

void MapRenderer::draw(sf::RenderTarget& target) const
{
    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            const size_t idx = static_cast<size_t>(y * m_width + x);
            if (idx >= m_tiles.size())
            {
                continue;
            }
            sf::RectangleShape tile({m_tileW, m_tileH});
            tile.setPosition(static_cast<float>(x) * m_tileW,
                             static_cast<float>(y) * m_tileH);
            tile.setFillColor(colorForTile(m_tiles[idx]));
            target.draw(tile);
        }
    }
}

sf::Color MapRenderer::colorForTile(const std::string& tileType) const
{
    if (tileType.find("grass") != std::string::npos)
    {
        return sf::Color(70, 130, 70);
    }
    if (tileType.find("bluestone") != std::string::npos)
    {
        return sf::Color(90, 110, 125);
    }
    return sf::Color(80, 90, 85);
}

void MapRenderer::generateDefaultMap()
{
    m_width  = 40;
    m_height = 30;
    m_tiles.clear();
    m_tiles.reserve(static_cast<size_t>(m_width * m_height));

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            const bool riverBand = x >= 18 && x <= 22;
            const bool grassPatch = !riverBand && Random::randInt(0, 9) < 2;
            m_tiles.push_back(grassPatch ? "grass" : "bluestone");
        }
    }
}

bool MapRenderer::parseGroundJson(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string content = ss.str();

    auto readNumber = [&](const std::string& key, int& out) -> bool {
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

    int tileW = 32;
    int tileH = 32;
    readNumber("tileWidth", tileW);
    readNumber("tileHeight", tileH);
    readNumber("width", m_width);
    readNumber("height", m_height);
    m_tileW = static_cast<float>(tileW);
    m_tileH = static_cast<float>(tileH);

    m_tiles.clear();
    const size_t arrStart = content.find("\"tiles\"");
    if (arrStart == std::string::npos)
    {
        return false;
    }
    size_t pos = content.find('[', arrStart);
    while (pos != std::string::npos && pos < content.size())
    {
        const size_t q1 = content.find('"', pos);
        if (q1 == std::string::npos)
        {
            break;
        }
        const size_t q2 = content.find('"', q1 + 1);
        if (q2 == std::string::npos)
        {
            break;
        }
        m_tiles.push_back(content.substr(q1 + 1, q2 - q1 - 1));
        pos = content.find('"', q2 + 1);
        if (content.find(']', q2) < pos)
        {
            break;
        }
    }

    return m_width > 0 && m_height > 0 && !m_tiles.empty();
}
