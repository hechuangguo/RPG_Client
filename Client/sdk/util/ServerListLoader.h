/**
 * @file    ServerListLoader.h
 * @brief   客户端 serverlist.xml 解析器
 *
 * 解析 config/serverlist.xml，提供 LoginServer 地址与游戏区列表，
 * 字段对齐 LoginServer ZoneInfo 表。
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief 单条游戏区配置
 */
struct GameZoneEntry
{
    uint32_t    zoneId;    /**< 游戏区号 */
    uint8_t     gameType;  /**< 游戏类型，对应 GameType@id */
    std::string name;      /**< 区服显示名 */
    std::string ip;        /**< 入口 IP */
    uint16_t    superPort; /**< Super 端口 */
    bool        enabled;   /**< true=可选，false=维护 */
};

/**
 * @brief serverlist.xml 加载器
 */
class ServerListLoader
{
public:
    ServerListLoader();

    /**
     * @brief 从 XML 文件加载区服列表
     * @param path 文件路径（通常为 ./config/serverlist.xml）
     * @return 解析成功返回 true
     */
    bool load(const std::string& path);

    /** @brief 最近一次 load 的错误描述（成功时为空） */
    const std::string& lastError() const;

    /** @brief LoginServer IP 地址 */
    const std::string& loginHost() const;

    /** @brief LoginServer 端口 */
    uint16_t loginPort() const;

    /** @brief 全部游戏区条目（含维护区） */
    const std::vector<GameZoneEntry>& zones() const;

    /**
     * @brief 按 zoneId 查找游戏区
     * @param zoneId 区号
     * @return 找到返回指针，否则 nullptr
     */
    const GameZoneEntry* findZone(uint32_t zoneId) const;

private:
    void clear();

    std::string              m_lastError;
    std::string              m_loginHost;
    uint16_t                 m_loginPort;
    std::vector<GameZoneEntry> m_zones;
};
