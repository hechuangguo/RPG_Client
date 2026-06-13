/**
 * @file    ConfigLoader.cpp
 * @brief   ConfigLoader 实现
 */

#include "util/ConfigLoader.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
std::string trim(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
    {
        --end;
    }
    return s.substr(start, end - start);
}

std::string unquote(const std::string& s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
    {
        return s.substr(1, s.size() - 2);
    }
    return s;
}
}  // namespace

ConfigLoader::ConfigLoader()
{
    applyDefaults();
}

void ConfigLoader::applyDefaults()
{
    m_windowWidth   = 1280;
    m_windowHeight  = 720;
    m_logLevel      = "info";
    m_logToConsole  = false;
    m_loginHost     = "127.0.0.1";
    m_loginPort     = 9010;
}

bool ConfigLoader::load(const std::string& path)
{
    applyDefaults();
    m_lastError.clear();

    std::ifstream file(path);
    if (!file.is_open())
    {
        m_lastError = "Cannot open config: " + path;
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    if (!parseJsonContent(ss.str()))
    {
        return false;
    }
    return true;
}

const std::string& ConfigLoader::lastError() const
{
    return m_lastError;
}

unsigned ConfigLoader::windowWidth() const
{
    return m_windowWidth;
}

unsigned ConfigLoader::windowHeight() const
{
    return m_windowHeight;
}

const std::string& ConfigLoader::logLevel() const
{
    return m_logLevel;
}

bool ConfigLoader::logToConsole() const
{
    return m_logToConsole;
}

const std::string& ConfigLoader::loginHost() const
{
    return m_loginHost;
}

uint16_t ConfigLoader::loginPort() const
{
    return m_loginPort;
}

bool ConfigLoader::parseJsonContent(const std::string& content)
{
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line))
    {
        const size_t colon = line.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        if (!value.empty() && value.back() == ',')
        {
            value.pop_back();
            value = trim(value);
        }
        value = unquote(value);

        if (key == "\"windowWidth\"")
        {
            m_windowWidth = static_cast<unsigned>(std::stoul(value));
        }
        else if (key == "\"windowHeight\"")
        {
            m_windowHeight = static_cast<unsigned>(std::stoul(value));
        }
        else if (key == "\"logLevel\"")
        {
            m_logLevel = value;
        }
        else if (key == "\"logToConsole\"")
        {
            m_logToConsole = (value == "true" || value == "1");
        }
        else if (key == "\"loginHost\"")
        {
            m_loginHost = value;
        }
        else if (key == "\"loginPort\"")
        {
            m_loginPort = static_cast<uint16_t>(std::stoul(value));
        }
    }
    return true;
}
