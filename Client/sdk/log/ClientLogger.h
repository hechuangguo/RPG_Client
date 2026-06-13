/**
 * @file    ClientLogger.h
 * @brief   客户端统一日志单例
 *
 * 职责：
 * - 按日滚动写入 ./logs/client_YYYYMMDD.log
 * - 可选同步输出到 stdout（默认关闭；GUI 子系统下 stdout 不可用，请查看 logs/）
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include <cstdarg>
#include <string>

/**
 * @brief 客户端日志级别
 */
enum class ClientLogLevel : int
{
    Info = 0, /**< 常规运行信息 */
    Warn = 1, /**< 警告，需关注但不中断流程 */
    Err  = 2, /**< 错误，功能可能受损 */
};

/**
 * @brief 客户端日志管理器（懒汉单例）
 *
 * 使用示例：
 * @code
 *   ClientLogger::instance().setLogToConsole(true);
 *   ClientLogger::instance().info("Client started");
 * @endcode
 */
class ClientLogger
{
public:
    /** @brief 获取全局唯一实例 */
    static ClientLogger& instance();

    /**
     * @brief 初始化日志系统（创建 logs 目录、打开当日文件）
     * @param logToConsole 是否同时输出到 stdout
     * @return 初始化成功返回 true
     */
    bool init(bool logToConsole = true);

    /** @brief 设置是否输出到控制台 */
    void setLogToConsole(bool enabled);

    /** @brief 设置最低输出级别（低于此级别的日志将被忽略） */
    void setMinLevel(ClientLogLevel level);

    /**
     * @brief 写入 INFO 级别日志
     * @param fmt printf 格式字符串
     */
    void info(const char* fmt, ...);

    /**
     * @brief 写入 WARN 级别日志
     * @param fmt printf 格式字符串
     */
    void warn(const char* fmt, ...);

    /**
     * @brief 写入 ERR 级别日志
     * @param fmt printf 格式字符串
     */
    void err(const char* fmt, ...);

    /**
     * @brief 通用日志入口
     * @param level 日志级别
     * @param fmt   printf 格式字符串
     */
    void log(ClientLogLevel level, const char* fmt, ...);

    /** @brief 刷新文件缓冲 */
    void flush();

private:
    ClientLogger();
    ~ClientLogger();

    ClientLogger(const ClientLogger&) = delete;
    ClientLogger& operator=(const ClientLogger&) = delete;

    void logV(ClientLogLevel level, const char* fmt, va_list ap);
    bool ensureLogFile();
    std::string currentLogPath() const;

    bool            m_initialized;
    bool            m_logToConsole;
    ClientLogLevel  m_minLevel;
    std::string     m_currentDate;
    FILE*           m_file;
};
