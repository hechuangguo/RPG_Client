/**
 * @file    MapRenderer.cpp
 * @brief   MapRenderer 实现
 */

#include "game/MapRenderer.h"

#include "log/ClientLogger.h"
#include "math/Random.h"
#include "util/PathUtil.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
constexpr int kMaxMapTiles = 256;

uint8_t tileHash(int x, int y)
{
    return static_cast<uint8_t>((x * 73 + y * 131) % 256);
}

bool fileExists(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

uint32_t resolveMapId(uint32_t mapId)
{
    const std::string path = PathUtil::joinPath(PathUtil::mapPath(mapId), "ground.json");
    if (fileExists(path))
    {
        return mapId;
    }
    if (mapId != 1002u)
    {
        const std::string fallback =
            PathUtil::joinPath(PathUtil::mapPath(1002u), "ground.json");
        if (fileExists(fallback))
        {
            ClientLogger::instance().warn(
                "MapRenderer：地图 %u 资源缺失，回退加载 1002", mapId);
            return 1002u;
        }
    }
    return mapId;
}
}  // namespace

MapRenderer::MapRenderer()
    : m_tileW(64.f)
    , m_tileH(32.f)
    , m_width(0)
    , m_height(0)
    , m_useTileset(false)
{
}

bool MapRenderer::load(uint32_t mapId)
{
    const uint32_t resolved = resolveMapId(mapId);
    const std::string path =
        PathUtil::joinPath(PathUtil::mapPath(resolved), "ground.json");
    if (parseGroundJson(path))
    {
        ClientLogger::instance().info("MapRenderer：已加载地图 %s", path.c_str());
    }
    else
    {
        ClientLogger::instance().warn("MapRenderer：地图 %u 解析失败，回退默认地图", resolved);
        m_tileW = 64.f;
        m_tileH = 32.f;
        generateDefaultMap();
    }

    m_useTileset = loadTileset(resolved);
    rebuildTiles();
    return true;
}

bool MapRenderer::loadTileset(uint32_t mapId)
{
    const std::string tilePath =
        PathUtil::joinPath(PathUtil::mapPath(mapId), "tileset.png");
    if (!m_tilesetTexture.loadFromFile(tilePath))
    {
        ClientLogger::instance().warn("MapRenderer：tileset 加载失败 %s", tilePath.c_str());
        return false;
    }
    m_tilesetTexture.setSmooth(false);
    ClientLogger::instance().info("MapRenderer：已加载 tileset %s", tilePath.c_str());
    return true;
}

int MapRenderer::tilesetIndexFor(const std::string& tileType) const
{
    if (tileType.find("flower") != std::string::npos)
    {
        return 3;
    }
    if (tileType.find("grass") != std::string::npos)
    {
        return 2;
    }
    if (tileType.find("crack") != std::string::npos)
    {
        return 1;
    }
    if (tileType.find("bluestone") != std::string::npos)
    {
        return 0;
    }
    return 0;
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

std::string MapRenderer::tileTypeAt(int x, int z) const
{
    if (x < 0 || z < 0 || x >= m_width || z >= m_height)
    {
        return {};
    }
    const size_t idx = static_cast<size_t>(z * m_width + x);
    if (idx >= m_tiles.size())
    {
        return "bluestone";
    }
    return m_tiles[idx];
}

bool MapRenderer::isWalkableTile(int x, int z) const
{
    const std::string type = tileTypeAt(x, z);
    return !type.empty() && type != "water";
}

void MapRenderer::drawBackdrop(sf::RenderTarget& target, const sf::FloatRect& viewBounds) const
{
    sf::RectangleShape backdrop(
        {viewBounds.width + m_tileW * 2.f, viewBounds.height + m_tileH * 2.f});
    backdrop.setPosition(viewBounds.left - m_tileW, viewBounds.top - m_tileH);
    backdrop.setFillColor(sf::Color(45, 85, 48));
    target.draw(backdrop);
}

void MapRenderer::draw(sf::RenderTarget& target) const
{
    if (m_useTileset)
    {
        for (const sf::Sprite& sprite : m_tileSprites)
        {
            target.draw(sprite);
        }
        return;
    }

    for (const TileVisual& tile : m_fallbackShapes)
    {
        target.draw(tile.base);
        for (const sf::RectangleShape& decor : tile.decor)
        {
            target.draw(decor);
        }
    }
}

sf::Color MapRenderer::colorForTile(const std::string& tileType) const
{
    if (tileType.find("grass") != std::string::npos)
    {
        return sf::Color(62, 118, 62);
    }
    if (tileType.find("bluestone") != std::string::npos)
    {
        return sf::Color(88, 108, 122);
    }
    return sf::Color(80, 90, 85);
}

void MapRenderer::generateCityMap(float grassRatio)
{
    m_tiles.clear();
    m_tiles.reserve(static_cast<size_t>(m_width * m_height));

    const int riverY    = 15;
    const int riverHalf = 2;
    const int plazaX0   = 16;
    const int plazaX1   = 22;
    const int plazaZ0   = 16;
    const int plazaZ1   = 22;

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            const bool inRiver =
                y >= riverY - riverHalf && y <= riverY + riverHalf && x >= 5 && x <= 35;
            const bool verticalRoad   = x == 19 || x == 20;
            const bool horizontalRoad = y == 14 || y == 15 || y == 16;
            const bool inPlaza =
                x >= plazaX0 && x <= plazaX1 && y >= plazaZ0 && y <= plazaZ1;
            const bool road = !inRiver && (verticalRoad || horizontalRoad || inPlaza);

            if (inRiver)
            {
                m_tiles.push_back("water");
            }
            else if (road)
            {
                m_tiles.push_back("bluestone");
            }
            else
            {
                const int roll = static_cast<int>(tileHash(x, y)) % 100;
                const int threshold = static_cast<int>(grassRatio * 100.f);
                const bool edgeGrass =
                    x < 3 || y < 3 || x >= m_width - 3 || y >= m_height - 3;
                const bool grass = edgeGrass || roll < threshold;
                m_tiles.push_back(grass && (roll % 5 == 0) ? "grass_flower" : "grass");
            }
        }
    }
}

void MapRenderer::generateDefaultMap()
{
    m_width  = 40;
    m_height = 30;
    generateCityMap(0.22f);
}

void MapRenderer::addTileDecor(TileVisual& visual,
                               const std::string& tileType,
                               int x,
                               int y) const
{
    const float px = static_cast<float>(x) * m_tileW;
    const float py = static_cast<float>(y) * m_tileH;
    visual.base.setFillColor(colorForTile(tileType));
    (void)px;
    (void)py;
    (void)tileType;
}

void MapRenderer::rebuildTiles()
{
    m_tileSprites.clear();
    m_fallbackShapes.clear();
    if (m_width <= 0 || m_height <= 0)
    {
        return;
    }

    const int srcTileW = 64;
    const int srcTileH = 32;

    for (int y = 0; y < m_height; ++y)
    {
        for (int x = 0; x < m_width; ++x)
        {
            const size_t idx = static_cast<size_t>(y * m_width + x);
            const std::string& tileType =
                (idx < m_tiles.size()) ? m_tiles[idx] : "bluestone";

            if (tileType == "water")
            {
                continue;
            }

            if (m_useTileset)
            {
                const int col = tilesetIndexFor(tileType);
                sf::Sprite sprite(m_tilesetTexture);
                sprite.setTextureRect(
                    sf::IntRect(col * srcTileW, 0, srcTileW, srcTileH));
                sprite.setPosition(static_cast<float>(x) * m_tileW,
                                   static_cast<float>(y) * m_tileH);
                const float scaleX = m_tileW / static_cast<float>(srcTileW);
                const float scaleY = m_tileH / static_cast<float>(srcTileH);
                sprite.setScale(scaleX, scaleY);
                m_tileSprites.push_back(sprite);
            }
            else
            {
                TileVisual visual(m_tileW, m_tileH);
                visual.base.setPosition(static_cast<float>(x) * m_tileW,
                                        static_cast<float>(y) * m_tileH);
                addTileDecor(visual, tileType, x, y);
                m_fallbackShapes.push_back(std::move(visual));
            }
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
        size_t end = colon + 1;
        while (end < content.size() &&
               (content[end] == ' ' || content[end] == '\t' || content[end] == '\r' ||
                content[end] == '\n'))
        {
            ++end;
        }
        size_t numEnd = end;
        while (numEnd < content.size())
        {
            const char ch = content.at(numEnd);
            if (!std::isdigit(static_cast<unsigned char>(ch)) && ch != '-' && ch != '+')
            {
                break;
            }
            ++numEnd;
        }
        if (numEnd == end)
        {
            return false;
        }
        out = std::stoi(content.substr(end, numEnd - end));
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
        try
        {
            out = std::stof(content.substr(colon + 1));
            return true;
        }
        catch (...)
        {
            return false;
        }
    };

    int tileW = 64;
    int tileH = 32;
    readNumber("tileWidth", tileW);
    readNumber("tileHeight", tileH);
    readNumber("width", m_width);
    readNumber("height", m_height);
    m_tileW = static_cast<float>(tileW);
    m_tileH = static_cast<float>(tileH);

    m_width  = std::max(1, std::min(m_width, kMaxMapTiles));
    m_height = std::max(1, std::min(m_height, kMaxMapTiles));

    m_tiles.clear();
    const size_t arrStart = content.find("\"tiles\"");
    if (arrStart == std::string::npos)
    {
        float grassRatio = 0.22f;
        readFloat("grassRatio", grassRatio);
        generateCityMap(grassRatio);
        return true;
    }

    const size_t arrOpen = content.find('[', arrStart);
    const size_t arrClose = content.find(']', arrStart);
    if (arrOpen == std::string::npos || arrClose == std::string::npos || arrClose <= arrOpen)
    {
        float grassRatio = 0.22f;
        readFloat("grassRatio", grassRatio);
        generateCityMap(grassRatio);
        return true;
    }

    size_t pos = arrOpen;
    const size_t maxTiles = static_cast<size_t>(m_width * m_height);
    while (pos < arrClose && m_tiles.size() < maxTiles)
    {
        const size_t q1 = content.find('"', pos + 1);
        if (q1 == std::string::npos || q1 >= arrClose)
        {
            break;
        }
        const size_t q2 = content.find('"', q1 + 1);
        if (q2 == std::string::npos || q2 >= arrClose)
        {
            break;
        }
        m_tiles.push_back(content.substr(q1 + 1, q2 - q1 - 1));
        pos = q2 + 1;
    }

    if (m_tiles.empty())
    {
        float grassRatio = 0.22f;
        readFloat("grassRatio", grassRatio);
        generateCityMap(grassRatio);
    }

    return m_width > 0 && m_height > 0 && !m_tiles.empty();
}
