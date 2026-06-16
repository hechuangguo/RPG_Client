/**
 * @file    ZoneTypes.h
 * @brief   游戏区信息（网络拉取或本地选中）
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief 区服负载状态（与 Msg_S2C_ZoneEntryWire.loadStatus 对齐）
 */
enum class ZoneLoadStatus : uint8_t
{
    Smooth      = 0, /**< 畅通 */
    Busy        = 1, /**< 繁忙 */
    Full        = 2, /**< 爆满 */
    Maintenance = 3, /**< 维护 */
};

/**
 * @brief 单个游戏区条目
 */
struct GameZoneEntry
{
    uint32_t       zoneId       = 0;
    uint8_t        gameType     = 0;
    std::string    name;
    bool           enabled      = false;
    ZoneLoadStatus loadStatus   = ZoneLoadStatus::Smooth;
    uint16_t       onlineCount  = 0;
    uint8_t        gatewayCount = 0;
};
