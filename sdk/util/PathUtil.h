/**
 * @file    PathUtil.h
 * @brief   客户端路径解析工具
 *
 * 以 .exe 所在目录为基准解析 map/、config/ 等运行时资源；
 * 日志目录通过向上查找 main.cpp 定位项目根下的 logs/。
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief 路径工具类（静态方法）
 */
class PathUtil
{
public:
    /**
     * @brief 获取当前可执行文件所在目录（不含末尾分隔符）
     * @return 绝对路径；失败时返回 "."
     */
    static std::string getExeDir();

    /**
     * @brief 拼接两段路径（自动处理分隔符）
     * @param left  左段路径
     * @param right 右段路径
     * @return 拼接后的路径
     */
    static std::string joinPath(const std::string& left, const std::string& right);

    /**
     * @brief 获取项目根目录（含 main.cpp 的目录）
     * @return 从 exe 向上查找；未找到时回退 getExeDir()
     */
    static std::string getProjectRoot();

    /**
     * @brief 获取日志目录
     * @return {projectRoot}/logs
     */
    static std::string logDir();

    /**
     * @brief 获取指定地图资源目录
     * @param mapId 地图 ID（如 1002）
     * @return {exeDir}/map/{mapId}
     */
    static std::string mapPath(uint32_t mapId);

    /**
     * @brief 获取 config 目录路径
     * @return {exeDir}/config
     */
    static std::string configPath();

    /**
     * @brief 获取 database 目录路径
     * @return {exeDir}/database
     */
    static std::string databasePath();
};
