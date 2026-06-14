/**
 * @file    LuaManager.cpp
 * @brief   LuaManager 实现
 */

#include "lua/LuaManager.h"

#include "log/ClientLogger.h"
#include "util/PathUtil.h"

LuaManager::LuaManager()
    : m_L(nullptr)
    , m_ready(false)
{
}

LuaManager::~LuaManager()
{
    shutdown();
}

bool LuaManager::init(const std::string& exeDir)
{
    shutdown();

    m_L = luaL_newstate();
    if (!m_L)
    {
        m_lastError = "luaL_newstate failed";
        return false;
    }

    luaL_openlibs(m_L);

    if (!setupPackagePath(exeDir))
    {
        shutdown();
        return false;
    }

    if (!loadInitScript())
    {
        shutdown();
        return false;
    }

    m_ready = true;
    ClientLogger::instance().info("LuaManager: initialized");
    return true;
}

void LuaManager::shutdown()
{
    if (m_L)
    {
        lua_close(m_L);
        m_L = nullptr;
    }
    m_ready = false;
}

bool LuaManager::isReady() const
{
    return m_ready;
}

lua_State* LuaManager::state()
{
    return m_L;
}

bool LuaManager::callGlobalVoid(const char* funcName)
{
    if (!m_ready || !funcName)
    {
        return false;
    }

    lua_getglobal(m_L, funcName);
    if (!lua_isfunction(m_L, -1))
    {
        lua_pop(m_L, 1);
        return false;
    }

    if (lua_pcall(m_L, 0, 0, 0) != LUA_OK)
    {
        m_lastError = lua_tostring(m_L, -1);
        ClientLogger::instance().err("LuaManager: %s failed: %s", funcName, m_lastError.c_str());
        lua_pop(m_L, 1);
        return false;
    }
    return true;
}

bool LuaManager::callGlobalVoid2(const char* funcName, double a, double b)
{
    if (!m_ready || !funcName)
    {
        return false;
    }

    lua_getglobal(m_L, funcName);
    if (!lua_isfunction(m_L, -1))
    {
        lua_pop(m_L, 1);
        return false;
    }

    lua_pushnumber(m_L, a);
    lua_pushnumber(m_L, b);

    if (lua_pcall(m_L, 2, 0, 0) != LUA_OK)
    {
        m_lastError = lua_tostring(m_L, -1);
        ClientLogger::instance().err("LuaManager: %s failed: %s", funcName, m_lastError.c_str());
        lua_pop(m_L, 1);
        return false;
    }
    return true;
}

bool LuaManager::callGlobalVoid1(const char* funcName, double a)
{
    if (!m_ready || !funcName)
    {
        return false;
    }

    lua_getglobal(m_L, funcName);
    if (!lua_isfunction(m_L, -1))
    {
        lua_pop(m_L, 1);
        return false;
    }

    lua_pushnumber(m_L, a);

    if (lua_pcall(m_L, 1, 0, 0) != LUA_OK)
    {
        m_lastError = lua_tostring(m_L, -1);
        ClientLogger::instance().err("LuaManager: %s failed: %s", funcName, m_lastError.c_str());
        lua_pop(m_L, 1);
        return false;
    }
    return true;
}

const std::string& LuaManager::lastError() const
{
    return m_lastError;
}

bool LuaManager::setupPackagePath(const std::string& exeDir)
{
    const std::string scriptPath  = PathUtil::joinPath(exeDir, "script/?.lua");
    const std::string clientPath  = PathUtil::joinPath(exeDir, "script/client/?.lua");
    const std::string dbPath      = PathUtil::joinPath(exeDir, "database/?.lua");
    const std::string basePath    = PathUtil::joinPath(exeDir, "basefile/?.lua");

    const std::string pathValue = scriptPath + ";" + clientPath + ";" + dbPath + ";" + basePath;

    const std::string luaCode =
        "package.path = \"" + pathValue + ";\" .. package.path";

    if (luaL_dostring(m_L, luaCode.c_str()) != LUA_OK)
    {
        m_lastError = lua_tostring(m_L, -1);
        ClientLogger::instance().err("LuaManager: package.path error: %s", m_lastError.c_str());
        lua_pop(m_L, 1);
        return false;
    }
    return true;
}

bool LuaManager::loadInitScript()
{
    const std::string initPath = PathUtil::joinPath(PathUtil::getExeDir(), "script/client/init.lua");
    if (luaL_dofile(m_L, initPath.c_str()) != LUA_OK)
    {
        m_lastError = lua_tostring(m_L, -1);
        ClientLogger::instance().warn("LuaManager: init.lua missing, using stub: %s", m_lastError.c_str());
        lua_pop(m_L, 1);

        const char* stub = R"(
function OnClientInit() end
function OnEnterGame(userId, mapId) end
function OnTick(nowMs) end
function OnQuestInfo(questId, name, progress, target, done) end
function OnBagInfo(slotCount) end
)";
        if (luaL_dostring(m_L, stub) != LUA_OK)
        {
            m_lastError = lua_tostring(m_L, -1);
            lua_pop(m_L, 1);
            return false;
        }
    }

    if (!callGlobalVoid("OnClientInit"))
    {
        ClientLogger::instance().warn("LuaManager: OnClientInit not defined");
    }
    return true;
}
