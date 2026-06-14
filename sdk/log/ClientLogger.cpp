/**
 * @file    ClientLogger.cpp
 * @brief   客户端统一日志单例实现
 */

#include "ClientLogger.h"

#include "../time/TimeUtil.h"
#include "../util/PathUtil.h"

#include <cstdio>
#include <cstring>
#include <filesystem>

namespace
{
const char* levelToString(ClientLogLevel level)
{
    switch (level)
    {
    case ClientLogLevel::Info: return "INFO";
    case ClientLogLevel::Warn: return "WARN";
    case ClientLogLevel::Err:  return "ERR";
    default:                   return "UNK";
    }
}
}  // namespace

ClientLogger& ClientLogger::instance()
{
    static ClientLogger s_instance;
    return s_instance;
}

ClientLogger::ClientLogger()
    : m_initialized(false)
    , m_logToConsole(true)
    , m_minLevel(ClientLogLevel::Info)
    , m_file(nullptr)
{
}

ClientLogger::~ClientLogger()
{
    if (m_file)
    {
        std::fflush(m_file);
        std::fclose(m_file);
        m_file = nullptr;
    }
}

bool ClientLogger::init(bool logToConsole)
{
    m_logToConsole = logToConsole;
    m_initialized = ensureLogFile();
    return m_initialized;
}

void ClientLogger::setLogToConsole(bool enabled)
{
    m_logToConsole = enabled;
}

void ClientLogger::setMinLevel(ClientLogLevel level)
{
    m_minLevel = level;
}

void ClientLogger::info(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logV(ClientLogLevel::Info, fmt, ap);
    va_end(ap);
}

void ClientLogger::warn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logV(ClientLogLevel::Warn, fmt, ap);
    va_end(ap);
}

void ClientLogger::err(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logV(ClientLogLevel::Err, fmt, ap);
    va_end(ap);
}

void ClientLogger::log(ClientLogLevel level, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logV(level, fmt, ap);
    va_end(ap);
}

void ClientLogger::flush()
{
    if (m_file)
    {
        std::fflush(m_file);
    }
}

std::string ClientLogger::currentLogPath() const
{
    const std::string date = TimeUtil::todayDateString();
    return PathUtil::joinPath(PathUtil::joinPath(PathUtil::getExeDir(), "logs"),
                              "client_" + date + ".log");
}

bool ClientLogger::ensureLogFile()
{
    const std::string date = TimeUtil::todayDateString();
    if (m_file && m_currentDate == date)
    {
        return true;
    }

    if (m_file)
    {
        std::fflush(m_file);
        std::fclose(m_file);
        m_file = nullptr;
    }

    const std::string logDir = PathUtil::joinPath(PathUtil::getExeDir(), "logs");
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);

    const std::string path = currentLogPath();
    m_file = std::fopen(path.c_str(), "a");
    if (!m_file)
    {
        return false;
    }

    m_currentDate = date;
    return true;
}

void ClientLogger::logV(ClientLogLevel level, const char* fmt, va_list ap)
{
    if (level < m_minLevel)
    {
        return;
    }

    if (!m_initialized && !init(m_logToConsole))
    {
        return;
    }

    ensureLogFile();

    char message[2048];
    vsnprintf(message, sizeof(message), fmt, ap);

    const std::string ts = TimeUtil::formatLocalMs(TimeUtil::wallNowMs());
    char line[2300];
    const int n = snprintf(line, sizeof(line), "[%s][%s] %s\n",
                           ts.c_str(), levelToString(level), message);
    if (n <= 0)
    {
        return;
    }

    if (m_logToConsole && level >= ClientLogLevel::Info)
    {
        std::fwrite(line, 1, static_cast<size_t>(n), stdout);
        std::fflush(stdout);
    }

    if (m_file)
    {
        std::fwrite(line, 1, static_cast<size_t>(n), m_file);
        std::fflush(m_file);
    }
}
