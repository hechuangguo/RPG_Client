/**
 * @file    TextUtil.h
 * @brief   UTF-8 std::string 与 sf::String 转换
 *
 * 项目 UI/协议字符串为 UTF-8；SFML 2.x 的 sf::String(std::string) 按 ANSI 解析，须显式 fromUtf8。
 */

#pragma once

#include <SFML/System/String.hpp>

#include <string>

namespace TextUtil
{
inline sf::String utf8ToSfString(const std::string& utf8)
{
    if (utf8.empty())
    {
        return sf::String();
    }
    return sf::String::fromUtf8(utf8.begin(), utf8.end());
}
}  // namespace TextUtil
