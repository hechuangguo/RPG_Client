/**
 * @file    LocalSettings.cpp
 * @brief   LocalSettings 实现
 */

#include "util/LocalSettings.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <ShlObj.h>
#include <Windows.h>
#endif

namespace fs = std::filesystem;

LocalSettings::LocalSettings()
    : m_lastZoneId(0)
    , m_lastGameType(0)
    , m_rememberAccount(false)
{
}

std::string LocalSettings::settingsPath() const
{
#ifdef _WIN32
    char path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path)))
    {
        return (fs::path(path) / "RPGClient" / "settings.json").string();
    }
#endif
    return "settings.json";
}

bool LocalSettings::ensureDirectory() const
{
    const fs::path filePath(settingsPath());
    const fs::path dir = filePath.parent_path();
    if (dir.empty())
    {
        return true;
    }
    std::error_code ec;
    fs::create_directories(dir, ec);
    return !ec;
}

bool LocalSettings::load()
{
    m_lastAccount.clear();
    m_lastZoneId = 0;
    m_lastGameType = 0;
    m_lastZoneName.clear();
    m_rememberAccount = false;

    std::ifstream file(settingsPath());
    if (!file.is_open())
    {
        return true;
    }

    std::string line;
    while (std::getline(file, line))
    {
        const size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && (value.back() == ',' || value.back() == '\r'))
        {
            value.pop_back();
        }
        while (!value.empty() && value.front() == ' ')
        {
            value.erase(value.begin());
        }
        if (!value.empty() && value.front() == '"')
        {
            value = value.substr(1);
            if (!value.empty() && value.back() == '"')
            {
                value.pop_back();
            }
        }

        if (key.find("lastAccount") != std::string::npos)
        {
            m_lastAccount = value;
        }
        else if (key.find("lastZoneId") != std::string::npos)
        {
            m_lastZoneId = static_cast<uint32_t>(std::stoul(value));
        }
        else if (key.find("lastGameType") != std::string::npos)
        {
            m_lastGameType = static_cast<uint8_t>(std::stoul(value));
        }
        else if (key.find("lastZoneName") != std::string::npos)
        {
            m_lastZoneName = value;
        }
        else if (key.find("rememberAccount") != std::string::npos)
        {
            m_rememberAccount = (value == "true" || value == "1");
        }
    }
    return true;
}

bool LocalSettings::save() const
{
    if (!ensureDirectory())
    {
        return false;
    }

    std::ofstream file(settingsPath());
    if (!file.is_open())
    {
        return false;
    }

    file << "{\n";
    file << "  \"lastAccount\": \"" << (m_rememberAccount ? m_lastAccount : "") << "\",\n";
    file << "  \"lastZoneId\": " << m_lastZoneId << ",\n";
    file << "  \"lastGameType\": " << static_cast<unsigned>(m_lastGameType) << ",\n";
    file << "  \"lastZoneName\": \"" << m_lastZoneName << "\",\n";
    file << "  \"rememberAccount\": " << (m_rememberAccount ? "true" : "false") << "\n";
    file << "}\n";
    return true;
}

const std::string& LocalSettings::lastAccount() const
{
    return m_lastAccount;
}

void LocalSettings::setLastAccount(const std::string& account)
{
    m_lastAccount = account;
}

uint32_t LocalSettings::lastZoneId() const
{
    return m_lastZoneId;
}

void LocalSettings::setLastZoneId(uint32_t zoneId)
{
    m_lastZoneId = zoneId;
}

uint8_t LocalSettings::lastGameType() const
{
    return m_lastGameType;
}

void LocalSettings::setLastGameType(uint8_t gameType)
{
    m_lastGameType = gameType;
}

const std::string& LocalSettings::lastZoneName() const
{
    return m_lastZoneName;
}

void LocalSettings::setLastZoneName(const std::string& name)
{
    m_lastZoneName = name;
}

bool LocalSettings::rememberAccount() const
{
    return m_rememberAccount;
}

void LocalSettings::setRememberAccount(bool remember)
{
    m_rememberAccount = remember;
}
