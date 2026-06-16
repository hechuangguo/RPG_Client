/**
 * @file    PathUtil.cpp
 * @brief   客户端路径解析工具实现
 */

#include "PathUtil.h"

#include <Windows.h>

#include <cstdio>
#include <filesystem>

namespace
{
std::string normalizePath(std::string path)
{
    for (char& ch : path)
    {
        if (ch == '\\')
        {
            ch = '/';
        }
    }
    while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
    {
        path.pop_back();
    }
    return path;
}
}  // namespace

std::string PathUtil::getExeDir()
{
    char buffer[MAX_PATH];
    const DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
        return ".";
    }

    std::filesystem::path exePath(buffer);
    return normalizePath(exePath.parent_path().string());
}

std::string PathUtil::joinPath(const std::string& left, const std::string& right)
{
    if (left.empty())
    {
        return normalizePath(right);
    }
    if (right.empty())
    {
        return normalizePath(left);
    }

    const bool leftSlash = (left.back() == '/' || left.back() == '\\');
    const bool rightSlash = (!right.empty() && (right.front() == '/' || right.front() == '\\'));

    if (leftSlash && rightSlash)
    {
        return normalizePath(left + right.substr(1));
    }
    if (!leftSlash && !rightSlash)
    {
        return normalizePath(left + "/" + right);
    }
    return normalizePath(left + right);
}

std::string PathUtil::getProjectRoot()
{
    std::filesystem::path dir(getExeDir());

    for (int depth = 0; depth < 8; ++depth)
    {
        const std::filesystem::path marker = dir / "main.cpp";
        std::error_code ec;
        if (std::filesystem::exists(marker, ec))
        {
            return normalizePath(dir.string());
        }

        const std::filesystem::path parent = dir.parent_path();
        if (parent == dir || parent.empty())
        {
            break;
        }
        dir = parent;
    }

    return getExeDir();
}

std::string PathUtil::logDir()
{
    return joinPath(getProjectRoot(), "logs");
}

std::string PathUtil::mapPath(uint32_t mapId)
{
    char idBuf[16];
    std::snprintf(idBuf, sizeof(idBuf), "%u", mapId);
    return joinPath(joinPath(getExeDir(), "map"), idBuf);
}

std::string PathUtil::configPath()
{
    return joinPath(getExeDir(), "config");
}

std::string PathUtil::databasePath()
{
    return joinPath(getExeDir(), "database");
}
