/**
 * @file    ServerListLoader.cpp
 * @brief   客户端 serverlist.xml 解析器实现
 */

#include "ServerListLoader.h"

#include <tinyxml2.h>

#include <cstdlib>
#include <cstring>

namespace
{
bool parseEnabledAttr(const char* value, bool fallback)
{
    if (!value || value[0] == '\0')
    {
        return fallback;
    }
    if (value[0] == '1' || value[0] == 't' || value[0] == 'T' || value[0] == 'y' || value[0] == 'Y')
    {
        return true;
    }
    if (value[0] == '0' || value[0] == 'f' || value[0] == 'F' || value[0] == 'n' || value[0] == 'N')
    {
        return false;
    }
    return fallback;
}

uint32_t readUIntAttr(const tinyxml2::XMLElement* elem, const char* name, uint32_t fallback)
{
    if (!elem)
    {
        return fallback;
    }
    const char* v = elem->Attribute(name);
    if (!v || v[0] == '\0')
    {
        return fallback;
    }
    return static_cast<uint32_t>(std::strtoul(v, nullptr, 10));
}

int readIntAttr(const tinyxml2::XMLElement* elem, const char* name, int fallback)
{
    if (!elem)
    {
        return fallback;
    }
    const char* v = elem->Attribute(name);
    if (!v || v[0] == '\0')
    {
        return fallback;
    }
    return std::atoi(v);
}

const char* readStrAttr(const tinyxml2::XMLElement* elem, const char* name, const char* fallback)
{
    if (!elem)
    {
        return fallback;
    }
    const char* v = elem->Attribute(name);
    return (v && v[0] != '\0') ? v : fallback;
}
}  // namespace

ServerListLoader::ServerListLoader()
    : m_loginPort(9010)
{
    clear();
}

void ServerListLoader::clear()
{
    m_lastError.clear();
    m_loginHost = "127.0.0.1";
    m_loginPort = 9010;
    m_zones.clear();
}

bool ServerListLoader::load(const std::string& path)
{
    clear();

    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLError err = doc.LoadFile(path.c_str());
    if (err != tinyxml2::XML_SUCCESS)
    {
        m_lastError = std::string("failed to load serverlist: ") + doc.ErrorStr();
        return false;
    }

    const tinyxml2::XMLElement* root = doc.RootElement();
    if (!root || std::strcmp(root->Name(), "ClientServerList") != 0)
    {
        m_lastError = "root element must be ClientServerList";
        return false;
    }

    if (const tinyxml2::XMLElement* login = root->FirstChildElement("LoginServer"))
    {
        m_loginHost = readStrAttr(login, "ip", m_loginHost.c_str());
        const int port = readIntAttr(login, "port", m_loginPort);
        if (port > 0 && port <= 65535)
        {
            m_loginPort = static_cast<uint16_t>(port);
        }
    }

    for (const tinyxml2::XMLElement* gameType = root->FirstChildElement("GameType");
         gameType != nullptr;
         gameType = gameType->NextSiblingElement("GameType"))
    {
        const uint32_t gameTypeId = readUIntAttr(gameType, "id", 0);

        for (const tinyxml2::XMLElement* zone = gameType->FirstChildElement("Zone");
             zone != nullptr;
             zone = zone->NextSiblingElement("Zone"))
        {
            GameZoneEntry entry{};
            entry.zoneId = readUIntAttr(zone, "id", 0);
            entry.gameType = static_cast<uint8_t>(
                readUIntAttr(zone, "gameType", gameTypeId));
            entry.name = readStrAttr(zone, "name", "");
            entry.ip = readStrAttr(zone, "ip", "127.0.0.1");
            const int superPort = readIntAttr(zone, "superPort", 0);
            entry.superPort = (superPort > 0 && superPort <= 65535)
                                  ? static_cast<uint16_t>(superPort)
                                  : 0;
            entry.enabled = parseEnabledAttr(zone->Attribute("enabled"), true);

            if (entry.zoneId > 0)
            {
                m_zones.push_back(entry);
            }
        }
    }

    if (m_zones.empty())
    {
        m_lastError = "no game zones found in serverlist";
        return false;
    }

    return true;
}

const std::string& ServerListLoader::lastError() const
{
    return m_lastError;
}

const std::string& ServerListLoader::loginHost() const
{
    return m_loginHost;
}

uint16_t ServerListLoader::loginPort() const
{
    return m_loginPort;
}

const std::vector<GameZoneEntry>& ServerListLoader::zones() const
{
    return m_zones;
}

const GameZoneEntry* ServerListLoader::findZone(uint32_t zoneId) const
{
    for (const GameZoneEntry& zone : m_zones)
    {
        if (zone.zoneId == zoneId)
        {
            return &zone;
        }
    }
    return nullptr;
}
