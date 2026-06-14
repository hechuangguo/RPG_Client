/**
 * @file    TimeUtil.cpp
 * @brief   客户端时间与帧间隔工具实现
 */

#include "TimeUtil.h"

#include <chrono>
#include <ctime>
#include <cstdio>

int64_t TimeUtil::nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

int64_t TimeUtil::wallNowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

float TimeUtil::deltaSeconds(int64_t prevMs, int64_t currMs)
{
    if (currMs <= prevMs)
    {
        return 0.0f;
    }
    return static_cast<float>(currMs - prevMs) / 1000.0f;
}

bool TimeUtil::elapsed(int64_t lastMs, int64_t intervalMs, int64_t now)
{
    return (now - lastMs) >= intervalMs;
}

std::string TimeUtil::todayDateString()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmLocal{};
    localtime_s(&tmLocal, &t);

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d",
                  tmLocal.tm_year + 1900, tmLocal.tm_mon + 1, tmLocal.tm_mday);
    return buf;
}

std::string TimeUtil::formatLocalMs(int64_t unixMs)
{
    const std::time_t sec = static_cast<std::time_t>(unixMs / 1000);
    const int ms = static_cast<int>(unixMs % 1000);
    std::tm tmLocal{};
    localtime_s(&tmLocal, &sec);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  tmLocal.tm_year + 1900, tmLocal.tm_mon + 1, tmLocal.tm_mday,
                  tmLocal.tm_hour, tmLocal.tm_min, tmLocal.tm_sec, ms);
    return buf;
}

FrameTimer::FrameTimer()
    : m_lastMs(TimeUtil::nowMs())
    , m_lastDelta(0.0f)
{
}

float FrameTimer::tick()
{
    const int64_t now = TimeUtil::nowMs();
    m_lastDelta = TimeUtil::deltaSeconds(m_lastMs, now);
    m_lastMs = now;
    return m_lastDelta;
}

float FrameTimer::lastDeltaSeconds() const
{
    return m_lastDelta;
}

int64_t FrameTimer::lastFrameMs() const
{
    return m_lastMs;
}
