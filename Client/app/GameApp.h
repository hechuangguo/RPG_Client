/**
 * @file    GameApp.h
 * @brief   客户端主应用与主循环
 *
 * 职责：
 * - 初始化 SFML 窗口、ClientLogger、ConfigLoader、ServerListLoader、LocalSettings
 * - 主循环：pollEvents → sessions → lua → render
 * - 管理 AppState 切换（Login/Register/Connecting/Game）
 *
 * 协作：LoginSession、GameSession、GameScene、UI 面板、LuaManager。
 *
 * 线程：仅主线程，非线程安全。
 */

#pragma once

#include "ClientMsg.h"
#include "app/AppState.h"
#include "game/GameScene.h"
#include "game/ItemBagModel.h"
#include "game/QuestModel.h"
#include "lua/ClientScriptHost.h"
#include "lua/LuaManager.h"
#include "net/GameSession.h"
#include "net/LoginSession.h"
#include "ui/LoginPanel.h"
#include "ui/RegisterPanel.h"
#include "ui/UiTheme.h"
#include "util/ConfigLoader.h"
#include "util/LocalSettings.h"
#include "util/ServerListLoader.h"

#include <SFML/Graphics.hpp>

#include <memory>
#include <string>

/**
 * @brief 客户端主应用
 */
class GameApp
{
public:
    GameApp();
    ~GameApp();

    /**
     * @brief 初始化并进入主循环（阻塞直到窗口关闭）
     * @return 正常退出返回 0，初始化失败返回非 0
     */
    int run();

private:
    bool init();
    void shutdown();
    void processEvents();
    void update(float dt);
    void render();

    void switchState(AppState state);
    void beginLogin(const LoginPanel::LoginRequest& req);
    void beginRegister(const RegisterPanel::RegisterRequest& req);
    void onEnterGame(const Msg_S2C_EnterGame& enter);
    void wireCallbacks();

    ConfigLoader       m_config;
    LocalSettings      m_localSettings;
    ServerListLoader   m_serverList;
    UiTheme            m_theme;
    LoginPanel         m_loginPanel;
    RegisterPanel      m_registerPanel;
    LoginSession       m_loginSession;
    GameSession        m_gameSession;
    GameScene          m_gameScene;
    QuestModel         m_questModel;
    ItemBagModel       m_itemBagModel;
    LuaManager         m_luaManager;
    ClientScriptHost   m_scriptHost;

    sf::RenderWindow   m_window;
    AppState           m_state;
    std::string        m_statusMessage;
    uint32_t           m_pendingZoneId;
    uint8_t            m_pendingGameType;
    int64_t            m_lastLuaTickMs;
};
