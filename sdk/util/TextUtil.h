/**
 * @file    TextUtil.h
 * @brief   UTF-8 std::string 与 sf::String 转换
 *
 * 项目 UI/协议字符串为 UTF-8；SFML 2.x 的 sf::String(std::string) 按 ANSI 解析，须显式 fromUtf8。
 */

#pragma once

#include <SFML/System/String.hpp>

#include <cstdint>
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

inline bool isPrintableCodepoint(char32_t cp)
{
    if (cp == 8 || cp == 127)
    {
        return false;
    }
    if (cp >= 0xD800 && cp <= 0xDFFF)
    {
        return false;
    }
    return cp >= 32;
}

inline std::string utf8Encode(char32_t cp)
{
    std::string out;
    if (cp <= 0x7F)
    {
        out.push_back(static_cast<char>(cp));
    }
    else if (cp <= 0x7FF)
    {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0xFFFF)
    {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp <= 0x10FFFF)
    {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

inline std::size_t utf8CodepointCount(const std::string& utf8)
{
    std::size_t count = 0;
    for (std::size_t i = 0; i < utf8.size();)
    {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        if ((c & 0xC0) == 0x80)
        {
            ++i;
            continue;
        }
        ++count;
        if (c < 0x80)
        {
            ++i;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            i += 4;
        }
        else
        {
            ++i;
        }
    }
    return count;
}

inline void utf8PopLastCodepoint(std::string& utf8)
{
    if (utf8.empty())
    {
        return;
    }

    std::size_t i = utf8.size() - 1;
    while (i > 0 && (static_cast<unsigned char>(utf8[i]) & 0xC0) == 0x80)
    {
        --i;
    }
    utf8.resize(i);
}
}  // namespace TextUtil
