/**
 * @file    AppState.h
 * @brief   客户端顶层应用状态枚举
 */

#pragma once

/**
 * @brief 客户端应用状态
 */
enum class AppState
{
    ZoneHome,     /**< 选区首页 */
    ServerList,   /**< 区列表（含网络拉取） */
    LoadingAuth,  /**< 加载资源（Lua 等） */
    AuthLogin,    /**< 账号密码登录 */
    Register,     /**< 注册界面 */
    Connecting,   /**< 登录/注册网络流程进行中 */
    Game,         /**< 已进入游戏世界 */
};
