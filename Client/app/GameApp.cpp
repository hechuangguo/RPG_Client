/**
 * @file    GameApp.cpp
 * @brief   GameApp 实现
 */

#include "app/GameApp.h"

#include "log/ClientLogger.h"
#include "net/TcpClient.h"
#include "time/TimeUtil.h"
#include "util/PathUtil.h"

GameApp::GameApp()
    : m_state(AppState::Login)
    , m_pendingZoneId(0)
    , m_pendingGameType(0)
    , m_lastLuaTickMs(0)
{
}

GameApp::~GameApp()
{
    shutdown();
}

int GameApp::run()
{
    if (!init())
    {
        return 1;
    }

    FrameTimer timer;
    while (m_window.isOpen())
    {
        processEvents();
        const float dt = timer.tick();
        update(dt);
        render();
    }

    shutdown();
    return 0;
}

bool GameApp::init()
{
    const std::string exeDir = PathUtil::getExeDir();
    const std::string configPath = PathUtil::joinPath(exeDir, "config/client_config.json");
    m_config.load(configPath);

    ClientLogger::instance().setLogToConsole(m_config.logToConsole());
    if (m_config.logLevel() == "warn")
    {
        ClientLogger::instance().setMinLevel(ClientLogLevel::Warn);
    }
    else if (m_config.logLevel() == "err")
    {
        ClientLogger::instance().setMinLevel(ClientLogLevel::Err);
    }

    const std::string serverListPath = PathUtil::joinPath(exeDir, "config/serverlist.xml");
    if (!m_serverList.load(serverListPath))
    {
        ClientLogger::instance().warn("GameApp: serverlist load failed: %s", m_serverList.lastError().c_str());
    }

    m_localSettings.load();

    m_window.create(sf::VideoMode(m_config.windowWidth(), m_config.windowHeight()),
                    u8"仙侠世界 - RPG Client",
                    sf::Style::Close);
    m_window.setFramerateLimit(60);

    m_theme.loadFont(PathUtil::joinPath(exeDir, "assets/fonts/simsun.ttf"));

    const sf::Vector2u viewSize = m_window.getSize();
    m_loginPanel.setup(&m_theme, &m_serverList, &m_localSettings, viewSize);
    m_registerPanel.setup(&m_theme, viewSize);
    m_loginPanel.applyLocalSettings();

    if (!m_luaManager.init(exeDir))
    {
        ClientLogger::instance().warn("GameApp: Lua init failed");
    }

    m_scriptHost.setup(&m_luaManager, &m_questModel, &m_itemBagModel, &m_gameSession);
    m_gameSession.setScriptHost(&m_scriptHost);
    m_loginSession.setServerList(&m_serverList);

    wireCallbacks();
    m_lastLuaTickMs = TimeUtil::nowMs();

    ClientLogger::instance().info("GameApp: initialized");
    return true;
}

void GameApp::shutdown()
{
    m_loginSession.cancel();
    m_gameSession.disconnect();
    m_luaManager.shutdown();
    if (m_window.isOpen())
    {
        m_window.close();
    }
}

void GameApp::wireCallbacks()
{
    m_loginPanel.setOnLogin([this](const LoginPanel::LoginRequest& req) {
        beginLogin(req);
    });

    m_loginPanel.setOnRegisterClick([this]() {
        m_registerPanel.setZone(m_loginPanel.selectedZoneId(), m_loginPanel.selectedGameType());
        switchState(AppState::Register);
    });

    m_registerPanel.setOnRegister([this](const RegisterPanel::RegisterRequest& req) {
        beginRegister(req);
    });

    m_registerPanel.setOnBack([this]() {
        switchState(AppState::Login);
    });

    m_loginSession.setOnEnterGame([this](const Msg_S2C_EnterGame& enter) {
        onEnterGame(enter);
    });

    m_loginSession.setOnError([this](const std::string& err) {
        m_statusMessage = err;
        m_loginPanel.setErrorMessage(err);
        m_registerPanel.setMessage(err, true);
        switchState(AppState::Login);
    });

    m_loginSession.setOnRegisterSuccess([this]() {
        m_registerPanel.setMessage(u8"注册成功，请返回登录", false);
        switchState(AppState::Login);
    });

    m_gameSession.setOnSpawn([this](const Msg_S2C_SpawnEntity& spawn) {
        m_gameScene.onSpawn(spawn);
    });

    m_gameSession.setOnMove([this](const Msg_S2C_MoveNotify& move) {
        m_gameScene.onMove(move);
    });

    m_gameSession.setOnDespawn([this](const Msg_S2C_DespawnEntity& despawn) {
        m_gameScene.onDespawn(despawn);
    });

    m_gameSession.setOnError([this](const std::string& err) {
        ClientLogger::instance().err("GameSession: %s", err.c_str());
    });

    m_gameSession.setOnDisconnected([this]() {
        switchState(AppState::Login);
        m_loginPanel.setErrorMessage(u8"与服务器连接已断开");
    });
}

void GameApp::processEvents()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            m_window.close();
            return;
        }

        if (m_state == AppState::Login)
        {
            m_loginPanel.handleEvent(event, m_window);
        }
        else if (m_state == AppState::Register)
        {
            m_registerPanel.handleEvent(event, m_window);
        }
        else if (m_state == AppState::Game)
        {
            m_gameScene.handleEvent(event);
        }
    }
}

void GameApp::update(float dt)
{
    if (m_state == AppState::Connecting)
    {
        m_loginSession.update();
        m_gameSession.update(dt);
    }
    else if (m_state == AppState::Game)
    {
        m_gameSession.update(dt);
        m_gameScene.update(dt);

        const int64_t now = TimeUtil::nowMs();
        if (TimeUtil::elapsed(m_lastLuaTickMs, 1000, now))
        {
            m_scriptHost.onTick(now);
            m_lastLuaTickMs = now;
        }
    }
}

void GameApp::render()
{
    m_window.clear();

    if (m_state == AppState::Login || m_state == AppState::Connecting)
    {
        m_theme.drawBackground(m_window, m_window.getSize());
        m_loginPanel.draw(m_window);

        if (m_state == AppState::Connecting && !m_statusMessage.empty())
        {
            sf::Text status(m_statusMessage, m_theme.font(), 16);
            status.setFillColor(m_theme.accentColor());
            status.setPosition(40.f, static_cast<float>(m_window.getSize().y) - 40.f);
            m_window.draw(status);
        }
    }
    else if (m_state == AppState::Register)
    {
        m_theme.drawBackground(m_window, m_window.getSize());
        m_registerPanel.draw(m_window);
    }
    else if (m_state == AppState::Game)
    {
        m_gameScene.draw(m_window);
    }

    m_window.display();
}

void GameApp::switchState(AppState state)
{
    m_state = state;
    if (state == AppState::Connecting)
    {
        m_statusMessage = u8"正在连接仙界...";
    }
}

void GameApp::beginLogin(const LoginPanel::LoginRequest& req)
{
    m_pendingZoneId   = req.zoneId;
    m_pendingGameType = req.gameType;

    m_localSettings.setLastZoneId(req.zoneId);
    m_localSettings.setRememberAccount(req.rememberAccount);
    if (req.rememberAccount)
    {
        m_localSettings.setLastAccount(req.account);
    }
    m_localSettings.save();

    m_loginPanel.setErrorMessage("");
    switchState(AppState::Connecting);
    m_loginSession.startLogin(req.account, req.password, req.zoneId, req.gameType);
}

void GameApp::beginRegister(const RegisterPanel::RegisterRequest& req)
{
    m_pendingZoneId   = req.zoneId;
    m_pendingGameType = req.gameType;

    switchState(AppState::Connecting);
    m_loginSession.startRegister(req.account, req.password, req.zoneId, req.gameType);
}

void GameApp::onEnterGame(const Msg_S2C_EnterGame& enter)
{
    auto tcp = m_loginSession.releaseTcpClient();
    m_gameSession.start(std::move(tcp), enter);

    m_scriptHost.setup(&m_luaManager, &m_questModel, &m_itemBagModel, &m_gameSession);
    m_scriptHost.onEnterGame(enter.userID, enter.mapID);

    m_gameScene.enter(enter, &m_theme, &m_gameSession, &m_questModel);
    m_gameScene.setViewSize(m_window.getSize());

    switchState(AppState::Game);
    ClientLogger::instance().info("GameApp: entered game map=%u", enter.mapID);
}
