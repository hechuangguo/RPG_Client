/**
 * @file    ConfigLoader.cpp
 * @brief   ConfigLoader 实现
 */

#include "util/ConfigLoader.h"

#include "net/ClientTimingDefaults.h"
#include "net/ClientTlsConfig.h"

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

bool isXmlDeclarationOnly(const std::string& content)
{
    static const std::regex declPattern(R"(<\?xml[\s\S]*?\?>)");
    std::string remainder = std::regex_replace(content, declPattern, "");
    return trim(remainder).empty();
}

std::string readXmlAttr(const std::string& attrs, const std::string& name)
{
    const std::string pattern = name + "=\"([^\"]*)\"";
    std::regex attrPattern(pattern);
    std::smatch match;
    if (std::regex_search(attrs, match, attrPattern) && match.size() > 1)
    {
        return match[1].str();
    }
    return {};
}

bool readXmlBoolAttr(const std::string& attrs, const std::string& name, bool defaultValue)
{
    const std::string value = readXmlAttr(attrs, name);
    if (value.empty())
    {
        return defaultValue;
    }
    return (value == "1" || value == "true");
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

    m_connectTimeoutMs          = ClientTiming::kConnectTimeoutMs;
    m_responseTimeoutMs         = ClientTiming::kResponseTimeoutMs;
    m_zoneListResponseTimeoutMs = ClientTiming::kZoneListResponseTimeoutMs;
    m_heartbeatIntervalMs       = ClientTiming::kHeartbeatIntervalMs;
    m_moveSendIntervalMs        = ClientTiming::kMoveSendIntervalMs;
    m_logoutTimeoutMs           = ClientTiming::kLogoutTimeoutMs;

    m_tls.enabled            = true;
    m_tls.caPath             = "config/tls/ca.crt";
    m_tls.insecureSkipVerify = true;
    m_tls.minVersion         = "1.2";
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

int64_t ConfigLoader::connectTimeoutMs() const
{
    return m_connectTimeoutMs;
}

int64_t ConfigLoader::responseTimeoutMs() const
{
    return m_responseTimeoutMs;
}

int64_t ConfigLoader::zoneListResponseTimeoutMs() const
{
    return m_zoneListResponseTimeoutMs;
}

int64_t ConfigLoader::heartbeatIntervalMs() const
{
    return m_heartbeatIntervalMs;
}

int64_t ConfigLoader::moveSendIntervalMs() const
{
    return m_moveSendIntervalMs;
}

int64_t ConfigLoader::logoutTimeoutMs() const
{
    return m_logoutTimeoutMs;
}

NetworkTiming ConfigLoader::networkTiming() const
{
    return NetworkTiming{m_connectTimeoutMs,
                         m_responseTimeoutMs,
                         m_zoneListResponseTimeoutMs,
                         m_heartbeatIntervalMs,
                         m_moveSendIntervalMs,
                         m_logoutTimeoutMs};
}

const ClientTlsConfig& ConfigLoader::tls() const
{
    return m_tls;
}

bool ConfigLoader::parseXmlContent(const std::string& content)
{
    const std::string stripped = stripXmlComments(content);
    const std::string trimmed = trim(stripped);

    static const std::regex tlsPattern(R"(<Tls\s+([^>]*?)(?:/>|>\s*</Tls>))");
    std::smatch tlsMatch;
    if (std::regex_search(trimmed, tlsMatch, tlsPattern) && tlsMatch.size() > 1)
    {
        const std::string attrs = tlsMatch[1].str();
        m_tls.enabled            = readXmlBoolAttr(attrs, "enabled", m_tls.enabled);
        m_tls.insecureSkipVerify = readXmlBoolAttr(attrs, "insecureSkipVerify",
                                                   m_tls.insecureSkipVerify);
        const std::string ca = readXmlAttr(attrs, "ca");
        if (!ca.empty())
        {
            m_tls.caPath = ca;
        }
        const std::string minVer = readXmlAttr(attrs, "minVersion");
        if (!minVer.empty())
        {
            m_tls.minVersion = minVer;
        }
    }

    if (trimmed.empty())
    {
        m_lastError = "配置文件为空";
        return false;
    }

    static const std::regex tagPattern(R"(<([A-Za-z_][\w]*)>([^<]*)</\1>)");

    auto begin = std::sregex_iterator(trimmed.begin(), trimmed.end(), tagPattern);
    const auto end = std::sregex_iterator();
    if (begin == end)
    {
        if (isXmlDeclarationOnly(trimmed))
        {
            return true;
        }
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
        else if (tag == "connectTimeoutMs")
        {
            m_connectTimeoutMs = std::stoll(value);
        }
        else if (tag == "responseTimeoutMs")
        {
            m_responseTimeoutMs = std::stoll(value);
        }
        else if (tag == "zoneListResponseTimeoutMs")
        {
            m_zoneListResponseTimeoutMs = std::stoll(value);
        }
        else if (tag == "heartbeatIntervalMs")
        {
            m_heartbeatIntervalMs = std::stoll(value);
        }
        else if (tag == "moveSendIntervalMs")
        {
            m_moveSendIntervalMs = std::stoll(value);
        }
        else if (tag == "logoutTimeoutMs")
        {
            m_logoutTimeoutMs = std::stoll(value);
        }
    }
    return true;
}
