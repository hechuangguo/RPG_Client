/**
 * @file    TimeUtil.h
 * @brief   客户端时间与帧间隔工具
 *
 * 职责：
 * - 提供单调时钟毫秒时间戳（nowMs）
 * - 计算帧间 deltaSeconds
 * - 辅助心跳/超时判断
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief 客户端时间工具（静态方法 + 帧计时器）
 */
class TimeUtil
{
public:
    /**
     * @brief 获取单调时钟当前毫秒数（适合帧间隔与超时）
     * @return 自 steady_clock 纪元起的毫秒数
     */
    static int64_t nowMs();

    /**
     * @brief 获取墙钟 Unix 毫秒时间戳（适合日志时间戳）
     * @return 自 1970-01-01 起的毫秒数
     */
    static int64_t wallNowMs();

    /**
     * @brief 计算两帧之间的秒数
     * @param prevMs 上一帧 nowMs()
     * @param currMs 当前帧 nowMs()
     * @return (currMs - prevMs) / 1000.0f，若 currMs < prevMs 则返回 0
     */
    static float deltaSeconds(int64_t prevMs, int64_t currMs);

    /**
     * @brief 判断是否已超过指定毫秒间隔
     * @param lastMs   上次事件时间戳（nowMs）
     * @param intervalMs 间隔毫秒
     * @param now      当前时间戳，默认 nowMs()
     * @return 已超过间隔返回 true
     */
    static bool elapsed(int64_t lastMs, int64_t intervalMs, int64_t now = nowMs());

    /**
     * @brief 获取本地日期字符串 YYYYMMDD（用于日志文件名）
     * @return 形如 20260613 的字符串
     */
    static std::string todayDateString();

    /**
     * @brief 将毫秒时间戳格式化为本地时间字符串
     * @param unixMs 毫秒时间戳（system_clock）
     * @return 形如 YYYY-MM-DD HH:MM:SS.mmm
     */
    static std::string formatLocalMs(int64_t unixMs);
};

/**
 * @brief 帧计时器，用于主循环 deltaTime 计算
 */
class FrameTimer
{
public:
    /** @brief 构造并记录当前时间为上一帧时刻 */
    FrameTimer();

    /**
     * @brief 推进一帧，更新 delta 并返回秒数
     * @return 本帧 deltaSeconds
     */
    float tick();

    /** @brief 上一帧 delta 秒数 */
    float lastDeltaSeconds() const;

    /** @brief 上一帧 tick 时的 nowMs */
    int64_t lastFrameMs() const;

private:
    int64_t m_lastMs;
    float   m_lastDelta;
};
