/**
 * @file    GameApp.cpp
 * @brief   GameApp 实现
 */

#include "app/GameApp.h"

#include "log/ClientLogger.h"
#include "net/CharacterTypes.h"
#include "net/TcpClient.h"
#include "time/TimeUtil.h"
#include "util/PathUtil.h"
#include "util/TextUtil.h"

namespace
{
bool isPreGameState(AppState state)
{
    switch (state)
    {
    case AppState::ZoneHome:
    case AppState::ServerList:
    case AppState::LoadingAuth:
    case AppState::AuthLogin:
    case AppState::Register:
    case AppState::Connecting:
    case AppState::CharacterSelect:
        return true;
    default:
        return false;
    }
}
}  // namespace

GameApp::GameApp()
    : m_state(AppState::ZoneHome)
    , m_pendingZoneId(0)
    , m_pendingGameType(0)
    , m_luaInitialized(false)
    , m_loadingAuthPending(false)
    , m_suppressGameDisconnectNav(false)
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
    const std::string configPath = PathUtil::joinPath(exeDir, "config/client_config.xml");
    if (!m_config.load(configPath))
    {
        ClientLogger::instance().warn("GameApp：%s，使用默认配置", m_config.lastError().c_str());
    }

    ClientLogger::instance().setLogToConsole(m_config.logToConsole());
    if (m_config.logLevel() == "warn")
    {
        ClientLogger::instance().setMinLevel(ClientLogLevel::Warn);
    }
    else if (m_config.logLevel() == "err")
    {
        ClientLogger::instance().setMinLevel(ClientLogLevel::Err);
    }

    m_localSettings.load();
    restoreSelectedZoneFromSettings();

    const std::string windowTitle = u8"仙侠世界 - RPG Client";
    m_window.create(sf::VideoMode(m_config.windowWidth(), m_config.windowHeight()),
                    TextUtil::utf8ToSfString(windowTitle),
                    sf::Style::Close);
    m_window.setFramerateLimit(60);

    m_theme.loadFont(PathUtil::joinPath(exeDir, "assets/fonts/NotoSansSC-Regular.otf"));
    m_theme.loadLoginBackground(PathUtil::joinPath(exeDir, "assets/ui/login_bg.png"), exeDir);

    const sf::Vector2u viewSize = m_window.getSize();
    m_loginChrome.setup(&m_theme, viewSize);
    m_loginChrome.setOnExit([this]() { m_window.close(); });
    m_zoneHomePanel.setup(&m_theme, viewSize);
    m_serverListPanel.setup(&m_theme, viewSize);
    m_authLoginPanel.setup(&m_theme, &m_localSettings, viewSize);
    m_registerPanel.setup(&m_theme, viewSize);
    m_characterSelectPanel.setup(&m_theme, viewSize);
    m_gameExitDialog.setup(&m_theme, viewSize);
    refreshZoneHomeDisplay();

    m_loginSession.setConfig(&m_config);
    m_zoneListSession.setConfig(&m_config);

    wireCallbacks();
    m_lastLuaTickMs = TimeUtil::nowMs();

    ClientLogger::instance().info("GameApp：初始化完成");
    return true;
}

void GameApp::shutdown()
{
    m_zoneListSession.cancel();
    m_loginSession.cancel();
    m_gameSession.disconnect();
    if (m_luaInitialized)
    {
        m_luaManager.shutdown();
        m_luaInitialized = false;
    }
    if (m_window.isOpen())
    {
        m_window.close();
    }
}

void GameApp::wireCallbacks()
{
    m_zoneHomePanel.setOnSelectServer([this]() { beginFetchZoneList(); });
    m_zoneHomePanel.setOnEnterGame([this]() { beginEnterAuth(); });

    m_serverListPanel.setOnConfirm([this](const GameZoneEntry& zone) {
        applySelectedZone(zone);
        switchState(AppState::ZoneHome);
    });

    m_serverListPanel.setOnCancel([this]() {
        m_zoneListSession.cancel();
        switchState(AppState::ZoneHome);
    });

    m_serverListPanel.setOnRetry([this]() { beginFetchZoneList(); });

    m_zoneListSession.setOnSuccess([this](const std::vector<GameZoneEntry>& zones) {
        if (zones.empty())
        {
            m_serverListPanel.setStatus(ServerListPanel::Status::Error, u8"暂无可用游戏区");
            return;
        }
        m_serverListPanel.setZones(zones);
    });

    m_zoneListSession.setOnError([this](const std::string& err) {
        m_serverListPanel.setStatus(ServerListPanel::Status::Error, err);
    });

    m_zoneListSession.setOnStatus([this](const std::string& msg) {
        if (m_state == AppState::ServerList)
        {
            m_serverListPanel.setStatus(ServerListPanel::Status::Loading, msg);
        }
    });

    m_authLoginPanel.setOnLogin([this](const AuthLoginPanel::LoginRequest& req) {
        beginLogin(req);
    });

    m_authLoginPanel.setOnRegisterClick([this]() {
        m_authLoginPanel.setSuccessMessage("");
        m_registerPanel.setZone(m_selectedZone.zoneId, m_selectedZone.gameType);
        switchState(AppState::Register);
    });

    m_authLoginPanel.setOnBackToZoneHome([this]() {
        switchState(AppState::ZoneHome);
    });

    m_registerPanel.setOnRegister([this](const RegisterPanel::RegisterRequest& req) {
        beginRegister(req);
    });

    m_registerPanel.setOnBack([this]() {
        switchState(AppState::AuthLogin);
    });

    m_registerPanel.setOnBackToZoneHome([this]() {
        switchState(AppState::ZoneHome);
    });

    m_loginSession.setOnEnterGame([this](const Msg_S2C_EnterGame& enter) {
        onEnterGame(enter);
    });

    m_loginSession.setOnStatus([this](const std::string& msg) {
        if (m_state == AppState::CharacterSelect)
        {
            m_characterSelectPanel.setStatus(CharacterSelectPanel::Status::Loading, msg);
        }
        else if (m_state == AppState::Register)
        {
            m_registerPanel.setMessage(msg, false);
        }
    });

    m_loginSession.setOnUserList([this](const std::vector<CharacterEntry>& chars, uint64_t lastUserId) {
        m_characterSelectPanel.setCharacters(chars, lastUserId);
    });

    m_characterSelectPanel.setOnEnterGame([this](uint64_t userId) {
        m_characterSelectPanel.setStatus(CharacterSelectPanel::Status::Loading,
                                         u8"正在加载地图与角色资源...");
        m_loginSession.selectCharacter(userId);
    });

    m_characterSelectPanel.setOnCreateCharacter([this](const std::string& name,
                                                         uint8_t vocation,
                                                         uint8_t sex) {
        m_characterSelectPanel.setStatus(CharacterSelectPanel::Status::Loading, u8"正在创建角色...");
        m_loginSession.createCharacter(name, vocation, sex);
    });

    m_characterSelectPanel.setOnBack([this]() {
        m_loginSession.cancel();
        m_characterSelectPanel.reset();
        switchState(AppState::AuthLogin);
    });

    m_loginSession.setOnError([this](const std::string& err) {
        if (m_state == AppState::CharacterSelect)
        {
            const auto panelStatus = m_loginSession.isBusy()
                                         ? CharacterSelectPanel::Status::Ready
                                         : CharacterSelectPanel::Status::Error;
            m_characterSelectPanel.setStatus(panelStatus, err);
        }
        else if (m_state == AppState::Register)
        {
            m_registerPanel.setMessage(err, true);
        }
        else
        {
            m_authLoginPanel.setErrorMessage(err);
            switchState(AppState::AuthLogin);
        }
    });

    m_loginSession.setOnRegisterSuccess([this]() {
        m_authLoginPanel.setCredentials(m_pendingRegisterAccount, m_pendingRegisterPassword);
        m_authLoginPanel.setSuccessMessage(u8"注册成功！账号已填入，可直接登录");
        m_registerPanel.setMessage("", false);
        m_statusMessage.clear();
        switchState(AppState::AuthLogin);
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
        ClientLogger::instance().err("GameSession：%s", err.c_str());
    });

    m_gameSession.setOnDisconnected([this]() {
        if (m_suppressGameDisconnectNav)
        {
            return;
        }
        switchState(AppState::ZoneHome);
        m_authLoginPanel.setErrorMessage(u8"与服务器连接已断开");
    });

    m_gameExitDialog.setOnReturnCharSelect([this]() { exitToCharacterSelect(); });
    m_gameExitDialog.setOnReturnLogin([this]() { exitToAuthLogin(); });
    m_gameExitDialog.setOnQuitClient([this]() { quitClient(); });
}

void GameApp::restoreSelectedZoneFromSettings()
{
    if (m_localSettings.lastZoneId() > 0 && !m_localSettings.lastZoneName().empty())
    {
        m_selectedZone.zoneId   = m_localSettings.lastZoneId();
        m_selectedZone.gameType = m_localSettings.lastGameType();
        m_selectedZone.name     = m_localSettings.lastZoneName();
    }
}

void GameApp::refreshZoneHomeDisplay()
{
    m_zoneHomePanel.setCurrentZoneName(m_selectedZone.name);
    m_zoneHomePanel.setHasValidZone(m_selectedZone.isValid());
}

void GameApp::beginFetchZoneList()
{
    switchState(AppState::ServerList);
    m_serverListPanel.setStatus(ServerListPanel::Status::Loading, u8"正在连接 LoginServer...");
    m_zoneListSession.fetchZoneList(ZONE_LIST_ALL_GAME_TYPES);
}

void GameApp::applySelectedZone(const GameZoneEntry& zone)
{
    m_selectedZone.zoneId   = zone.zoneId;
    m_selectedZone.gameType = zone.gameType;
    m_selectedZone.name     = zone.name;

    m_localSettings.setLastZoneId(zone.zoneId);
    m_localSettings.setLastGameType(zone.gameType);
    m_localSettings.setLastZoneName(zone.name);
    m_localSettings.save();

    refreshZoneHomeDisplay();
}

void GameApp::beginEnterAuth()
{
    if (!m_selectedZone.isValid())
    {
        return;
    }
    m_loadingMessage       = u8"正在加载资源...";
    m_loadingAuthPending   = true;
    switchState(AppState::LoadingAuth);
}

void GameApp::finishLoadingAuth()
{
    const std::string exeDir = PathUtil::getExeDir();
    if (!m_luaInitialized)
    {
        if (!m_luaManager.init(exeDir))
        {
            ClientLogger::instance().warn("GameApp：Lua 初始化失败");
        }
        m_luaInitialized = true;
    }

    m_scriptHost.setup(&m_luaManager, &m_questModel, &m_itemBagModel, &m_gameSession);
    m_gameSession.setScriptHost(&m_scriptHost);

    m_authLoginPanel.setErrorMessage("");
    m_authLoginPanel.applyLocalSettings();
    switchState(AppState::AuthLogin);
}

void GameApp::processEvents()
{
    sf::Event event;
    while (m_window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            if (m_state == AppState::Game && !m_gameExitDialog.isVisible())
            {
                showGameExitDialog();
                continue;
            }
            m_window.close();
            return;
        }

        if (event.type == sf::Event::Resized)
        {
            onResize(sf::Vector2u(event.size.width, event.size.height));
        }

        if (m_state == AppState::Game)
        {
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                if (m_gameExitDialog.isVisible())
                {
                    m_gameExitDialog.hide();
                }
                else
                {
                    showGameExitDialog();
                }
                continue;
            }

            if (m_gameExitDialog.isVisible())
            {
                m_gameExitDialog.handleEvent(event, m_window);
                continue;
            }

            m_gameScene.handleEvent(event);
            continue;
        }

        switch (m_state)
        {
        case AppState::ZoneHome:
            m_zoneHomePanel.handleEvent(event, m_window);
            break;
        case AppState::ServerList:
            m_serverListPanel.handleEvent(event, m_window);
            break;
        case AppState::AuthLogin:
            m_authLoginPanel.handleEvent(event, m_window);
            break;
        case AppState::Register:
            m_registerPanel.handleEvent(event, m_window);
            break;
        case AppState::CharacterSelect:
            m_characterSelectPanel.handleEvent(event, m_window);
            break;
        default:
            break;
        }

        if (isPreGameState(m_state))
        {
            m_loginChrome.handleEvent(event, m_window);
        }
    }
}

void GameApp::update(float dt)
{
    if (isPreGameState(m_state))
    {
        m_theme.updateLoginBackground(dt);
    }

    if (m_state == AppState::ServerList)
    {
        m_zoneListSession.update();
    }

    if (m_state == AppState::LoadingAuth && m_loadingAuthPending)
    {
        m_loadingAuthPending = false;
        finishLoadingAuth();
    }

    if (m_state == AppState::AuthLogin)
    {
        m_authLoginPanel.update(dt);
    }
    else if (m_state == AppState::Register)
    {
        m_registerPanel.update(dt);
    }
    else if (m_state == AppState::CharacterSelect)
    {
        m_characterSelectPanel.update(dt);
    }

    if (m_loginSession.isBusy())
    {
        m_loginSession.update();
    }

    if (m_state == AppState::Game)
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

    switch (m_state)
    {
    case AppState::ZoneHome:
        m_theme.drawBackground(m_window, m_window.getSize());
        m_zoneHomePanel.draw(m_window);
        break;

    case AppState::ServerList:
        m_theme.drawBackground(m_window, m_window.getSize());
        m_serverListPanel.draw(m_window);
        break;

    case AppState::LoadingAuth:
        m_theme.drawBackground(m_window, m_window.getSize());
        m_theme.drawText(
            m_window,
            m_loadingMessage,
            static_cast<float>(m_window.getSize().x) / 2.f - 80.f,
            static_cast<float>(m_window.getSize().y) / 2.f,
            20,
            m_theme.accentColor());
        break;

    case AppState::AuthLogin:
        m_theme.drawBackground(m_window, m_window.getSize());
        m_authLoginPanel.draw(m_window);
        break;

    case AppState::Register:
        m_theme.drawBackground(m_window, m_window.getSize());
        m_registerPanel.draw(m_window);
        break;

    case AppState::CharacterSelect:
        m_theme.drawBackground(m_window, m_window.getSize());
        m_characterSelectPanel.draw(m_window);
        break;

    case AppState::Game:
        m_gameScene.draw(m_window);
        if (m_gameExitDialog.isVisible())
        {
            m_gameExitDialog.draw(m_window);
        }
        break;
    }

    if (isPreGameState(m_state))
    {
        m_loginChrome.draw(m_window);
    }

    m_window.display();
}

void GameApp::switchState(AppState state)
{
    m_state = state;
}

void GameApp::beginLogin(const AuthLoginPanel::LoginRequest& req)
{
    if (!m_selectedZone.isValid())
    {
        m_authLoginPanel.setErrorMessage(u8"请先选择游戏区");
        return;
    }

    m_pendingZoneId   = m_selectedZone.zoneId;
    m_pendingGameType = m_selectedZone.gameType;

    m_localSettings.setRememberAccount(req.rememberAccount);
    if (req.rememberAccount)
    {
        m_localSettings.setLastAccount(req.account);
    }
    m_localSettings.save();

    m_authLoginPanel.setErrorMessage("");
    m_authLoginPanel.setSuccessMessage("");
    m_characterSelectPanel.reset();
    m_characterSelectPanel.setStatus(CharacterSelectPanel::Status::Loading, u8"正在验证账号...");
    switchState(AppState::CharacterSelect);
    m_loginSession.startLogin(req.account, req.password, m_pendingZoneId, m_pendingGameType);
}

void GameApp::beginRegister(const RegisterPanel::RegisterRequest& req)
{
    m_pendingZoneId              = req.zoneId;
    m_pendingGameType            = req.gameType;
    m_pendingRegisterAccount     = req.account;
    m_pendingRegisterPassword    = req.password;

    m_registerPanel.setMessage(u8"正在注册...", false);
    m_loginSession.startRegister(
        req.account, req.password, req.confirmPassword, req.zoneId, req.gameType);
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
    ClientLogger::instance().info("GameApp：进入游戏成功，地图=%u", enter.mapID);
}

void GameApp::onResize(const sf::Vector2u& size)
{
    m_loginChrome.onResize(size);
    m_zoneHomePanel.setup(&m_theme, size);
    m_serverListPanel.setup(&m_theme, size);
    m_authLoginPanel.setup(&m_theme, &m_localSettings, size);
    m_registerPanel.setup(&m_theme, size);
    m_characterSelectPanel.setup(&m_theme, size);
    m_gameExitDialog.setup(&m_theme, size);
    refreshZoneHomeDisplay();
    m_gameScene.setViewSize(size);
}

void GameApp::showGameExitDialog()
{
    if (m_state != AppState::Game || m_gameSession.isWaitingLogout())
    {
        return;
    }
    m_gameExitDialog.show();
}

void GameApp::exitToCharacterSelect()
{
    if (m_state != AppState::Game)
    {
        return;
    }

    m_suppressGameDisconnectNav = true;
    m_gameExitDialog.setBusy(true, u8"正在离开世界...");

    const uint64_t highlightUserId = m_gameSession.localUserId();
    m_gameSession.requestLogout(
        LogoutAction::RETURN_CHAR_SELECT,
        [this, highlightUserId](LogoutAction /*action*/) {
            m_gameScene.leave();
            auto tcp = m_gameSession.releaseTcpClient();
            m_suppressGameDisconnectNav = false;

            m_characterSelectPanel.reset();
            m_characterSelectPanel.setStatus(CharacterSelectPanel::Status::Loading,
                                             u8"正在获取角色列表...");
            switchState(AppState::CharacterSelect);
            m_gameExitDialog.hide();

            m_loginSession.resumeGatewayForCharSelect(std::move(tcp), highlightUserId);
        },
        [this](const std::string& err) {
            m_suppressGameDisconnectNav = false;
            m_gameExitDialog.setBusy(false);
            ClientLogger::instance().warn("GameApp：返回选角失败，降级返回登录：%s", err.c_str());
            finishExitToAuthLogin(err);
        });
}

void GameApp::exitToAuthLogin()
{
    if (m_state != AppState::Game)
    {
        return;
    }

    m_suppressGameDisconnectNav = true;
    m_gameExitDialog.setBusy(true, u8"正在离开世界...");

    m_gameSession.requestLogout(
        LogoutAction::RETURN_LOGIN,
        [this](LogoutAction /*action*/) { finishExitToAuthLogin(""); },
        [this](const std::string& err) {
            ClientLogger::instance().warn("GameApp：返回登录离世界失败，强制断开：%s", err.c_str());
            finishExitToAuthLogin(err);
        });
}

void GameApp::finishExitToAuthLogin(const std::string& errorMsg)
{
    m_suppressGameDisconnectNav = true;
    m_gameScene.leave();
    m_gameSession.disconnect();
    m_loginSession.cancel();
    m_suppressGameDisconnectNav = false;

    m_characterSelectPanel.reset();
    m_gameExitDialog.hide();
    m_authLoginPanel.setErrorMessage(errorMsg);
    m_authLoginPanel.applyLocalSettings();
    switchState(AppState::AuthLogin);
}

void GameApp::quitClient()
{
    m_gameExitDialog.hide();
    m_window.close();
}
