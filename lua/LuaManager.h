/**
 * @file    LuaManager.h
 * @brief   客户端 Lua 虚拟机管理
 *
 * 职责：
 * - 创建/销毁 lua_State
 * - 配置 package.path（script/database/basefile）
 * - 加载 script/client/init.lua
 * - 提供 callGlobalVoid 等 C++ 调用 Lua 全局函数接口
 *
 * 协作：ScriptBindings、ClientScriptHost。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>

/**
 * @brief Lua 5.4 虚拟机封装
 */
class LuaManager
{
public:
    LuaManager();
    ~LuaManager();

    LuaManager(const LuaManager&) = delete;
    LuaManager& operator=(const LuaManager&) = delete;

    /**
     * @brief 初始化 Lua 并加载入口脚本
     * @param exeDir 可执行文件目录（用于 package.path）
     * @return 成功返回 true
     */
    bool init(const std::string& exeDir);

    /** @brief 关闭 Lua 状态 */
    void shutdown();

    /** @brief 是否已初始化 */
    bool isReady() const;

    /** @brief 原始 lua_State 指针 */
    lua_State* state();

    /**
     * @brief 调用 Lua 全局无返回值函数
     * @param funcName 全局函数名
     * @return 调用成功返回 true
     */
    bool callGlobalVoid(const char* funcName);

    /**
     * @brief 调用 Lua 全局函数（两个 number 参数）
     * @param funcName 全局函数名
     * @param a        参数 1
     * @param b        参数 2
     * @return 调用成功返回 true
     */
    bool callGlobalVoid2(const char* funcName, double a, double b);

    /**
     * @brief 调用 Lua 全局函数（一个 number 参数）
     * @param funcName 全局函数名
     * @param a        参数
     * @return 调用成功返回 true
     */
    bool callGlobalVoid1(const char* funcName, double a);

    /** @brief 最近一次错误信息 */
    const std::string& lastError() const;

private:
    bool setupPackagePath(const std::string& exeDir);
    bool loadInitScript();

    lua_State*  m_L;
    bool        m_ready;
    std::string m_lastError;
};
