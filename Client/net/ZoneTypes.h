/**
 * @file    ZoneTypes.h
 * @brief   游戏区信息（网络拉取或本地选中）
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief 单个游戏区条目
 */
struct GameZoneEntry
{
    uint32_t    zoneId   = 0;
    uint8_t     gameType = 0;
    std::string name;
    bool        enabled  = false;
};
