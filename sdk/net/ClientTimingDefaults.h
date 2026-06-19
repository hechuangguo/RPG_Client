/**
 * @file    ClientTimingDefaults.h
 * @brief   客户端网络时序默认值（可被 client_config.xml 覆盖）
 */

#pragma once

#include <cstdint>

namespace ClientTiming
{
constexpr int64_t kConnectTimeoutMs           = 10000;
constexpr int64_t kResponseTimeoutMs          = 15000;
constexpr int64_t kZoneListResponseTimeoutMs  = 10000;
constexpr int64_t kHeartbeatIntervalMs        = 10000;
constexpr int64_t kMoveSendIntervalMs         = 100;
constexpr int64_t kLogoutTimeoutMs            = 15000;
}  // namespace ClientTiming
