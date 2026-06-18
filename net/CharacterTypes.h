/**
 * @file    CharacterTypes.h
 * @brief   角色列表 UI 与网络层共用的角色条目
 *
 * 职业/性别 wire 值需与 Server RecordServer 一致。
 */

#pragma once

#include <cstdint>
#include <string>

/** @brief 角色列表单条（由 S2C_USER_LIST 解析） */
struct CharacterEntry
{
    uint64_t    userID   = 0;
    std::string name;
    uint32_t    level    = 0;
    uint8_t     vocation = 0;
    uint8_t     sex      = 0;
};

namespace CharacterDef
{
constexpr uint8_t kVocationWarrior = 0;
constexpr uint8_t kVocationMage    = 1;
constexpr uint8_t kSexMale         = 0;
constexpr uint8_t kSexFemale       = 1;
}  // namespace CharacterDef
