/**
 * @file    PasswordDigest.h
 * @brief   登录 wire 密码摘要（SHA-256 UTF-8）
 */

#pragma once

#include <cstdint>
#include <string>

namespace PasswordDigest
{
/** @brief SHA-256(UTF-8 密码) 写入 32 字节 digest；失败返回 false */
bool sha256Utf8Password(const std::string& password, uint8_t digest[32]);
}  // namespace PasswordDigest
