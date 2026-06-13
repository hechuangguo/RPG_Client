/**
 * @file    AppState.h
 * @brief   客户端顶层应用状态枚举
 *
 * 职责：
 * - 定义 GameApp 主循环中 UI/网络/场景切换的离散状态
 * - Login/Register 为认证 UI；Connecting 为登录会话进行中；Game 为游戏场景
 *
 * 协作：GameApp 根据 AppState 分发 update/render 到 LoginPanel、RegisterPanel 或 GameScene。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

/**
 * @brief 客户端应用状态
 */
enum class AppState
{
    Login,      /**< 登录界面（含区服选择） */
    Register,   /**< 注册界面 */
    Connecting, /**< 登录/注册网络流程进行中 */
    Game,       /**< 已进入游戏世界 */
};
