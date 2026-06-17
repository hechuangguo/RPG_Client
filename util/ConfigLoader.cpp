/**
 * @file    ConfigLoader.cpp
 * @brief   ConfigLoader 实现
 */

#include "util/ConfigLoader.h"

#include <cctype>
#include <fstream>
#include <regex>
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

std::string stripXmlComments(const std::string& content)
{
    static const std::regex commentPattern(R"(<!--[\s\S]*?-->)");
    return std::regex_replace(content, commentPattern, "");
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
        m_lastError = "无法打开配置文件：" + path;
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    if (!parseXmlContent(ss.str()))
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

bool ConfigLoader::parseXmlContent(const std::string& content)
{
    const std::string stripped = stripXmlComments(content);
    static const std::regex tagPattern(R"(<([A-Za-z_][\w]*)>([^<]*)</\1>)");

    auto begin = std::sregex_iterator(stripped.begin(), stripped.end(), tagPattern);
    const auto end = std::sregex_iterator();
    if (begin == end)
    {
        m_lastError = "XML 解析失败：未找到有效配置项";
        return false;
    }

    for (auto it = begin; it != end; ++it)
    {
        const std::string tag = (*it)[1].str();
        const std::string value = trim((*it)[2].str());

        if (tag == "windowWidth")
        {
            m_windowWidth = static_cast<unsigned>(std::stoul(value));
        }
        else if (tag == "windowHeight")
        {
            m_windowHeight = static_cast<unsigned>(std::stoul(value));
        }
        else if (tag == "logLevel")
        {
            m_logLevel = value;
        }
        else if (tag == "logToConsole")
        {
            m_logToConsole = (value == "true" || value == "1");
        }
        else if (tag == "loginHost")
        {
            m_loginHost = value;
        }
        else if (tag == "loginPort")
        {
            m_loginPort = static_cast<uint16_t>(std::stoul(value));
        }
    }
    return true;
}
