/**
 * @file    GameApp.h
 * @brief   客户端主应用与主循环
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
#include "net/ZoneListSession.h"
#include "net/ZoneTypes.h"
#include "ui/AuthLoginPanel.h"
#include "ui/RegisterPanel.h"
#include "ui/ServerListPanel.h"
#include "ui/UiTheme.h"
#include "ui/ZoneHomePanel.h"
#include "util/ConfigLoader.h"
#include "util/LocalSettings.h"

#include <SFML/Graphics.hpp>

#include <cstdint>
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

    int run();

private:
    struct SelectedZone
    {
        uint32_t    zoneId   = 0;
        uint8_t     gameType = 0;
        std::string name;

        bool isValid() const { return zoneId != 0 && !name.empty(); }
    };

    bool init();
    void shutdown();
    void processEvents();
    void update(float dt);
    void render();

    void switchState(AppState state);
    void restoreSelectedZoneFromSettings();
    void refreshZoneHomeDisplay();
    void beginFetchZoneList();
    void applySelectedZone(const GameZoneEntry& zone);
    void beginEnterAuth();
    void finishLoadingAuth();
    void beginLogin(const AuthLoginPanel::LoginRequest& req);
    void beginRegister(const RegisterPanel::RegisterRequest& req);
    void onEnterGame(const Msg_S2C_EnterGame& enter);
    void wireCallbacks();
    void onResize(const sf::Vector2u& size);

    ConfigLoader       m_config;
    LocalSettings      m_localSettings;
    UiTheme            m_theme;
    ZoneHomePanel      m_zoneHomePanel;
    ServerListPanel    m_serverListPanel;
    AuthLoginPanel     m_authLoginPanel;
    RegisterPanel      m_registerPanel;
    ZoneListSession    m_zoneListSession;
    LoginSession       m_loginSession;
    GameSession        m_gameSession;
    GameScene          m_gameScene;
    QuestModel         m_questModel;
    ItemBagModel       m_itemBagModel;
    LuaManager         m_luaManager;
    ClientScriptHost   m_scriptHost;

    sf::RenderWindow   m_window;
    AppState           m_state;
    SelectedZone       m_selectedZone;
    std::string        m_statusMessage;
    std::string        m_loadingMessage;
    uint32_t           m_pendingZoneId;
    uint8_t            m_pendingGameType;
    bool               m_luaInitialized;
    bool               m_loadingAuthPending;
    int64_t            m_lastLuaTickMs;
};
