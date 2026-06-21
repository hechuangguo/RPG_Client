/// <summary>
/// 游戏主控制器（对标 app/GameApp + game/GameScene）。
/// 职责：AppState 状态机、Session 生命周期、UI 切换。
/// 协作：LoginSession、GameSession、ZoneListSession、GameUiController、WorldController。
/// </summary>
using System.Collections.Generic;
using Rpg.Client.Config;
using Rpg.Client.Log;
using Rpg.Client.Net;
using Rpg.Client.UI;
using Rpg.Client.World;
using Rpg.Proto.Login;
using UnityEngine;

namespace Rpg.Client.App
{
    public sealed class GameApp : MonoBehaviour
    {
        [SerializeField] private GameUiController _ui;
        [SerializeField] private WorldController _world;

        private readonly ClientConfigLoader _config = new ClientConfigLoader();
        private readonly LocalSettings _localSettings = new LocalSettings();
        private readonly ZoneListSession _zoneList = new ZoneListSession();
        private readonly LoginSession _login = new LoginSession();
        private readonly GameSession _game = new GameSession();

        private AppState _state = AppState.ZoneHome;
        private uint _selectedZoneId;
        private byte _selectedGameType;
        private string _selectedZoneName = string.Empty;
        private string _pendingAccount = string.Empty;
        private string _pendingPassword = string.Empty;
        private List<CharacterEntry> _characters = new List<CharacterEntry>();
        private bool _suppressDisconnectNav;
        private bool _exitDialogVisible;
        private LogoutAction? _pendingExitAction;

        private void Awake()
        {
            ClientLogger.Instance.Initialize();
            _config.Load();
            _localSettings.Load();
            RestoreZoneFromSettings();

            _zoneList.SetConfig(_config);
            _login.SetConfig(_config);
            _game.SetConfig(_config);

            WireCallbacks();
            SetState(AppState.ZoneHome);
        }

        private void Update()
        {
            _zoneList.Update();
            _login.Update();
            _game.Update();

            if (_state == AppState.Game && Input.GetKeyDown(KeyCode.Escape))
            {
                _exitDialogVisible = !_exitDialogVisible;
                _ui.ShowExitDialog(_exitDialogVisible);
            }
        }

        private void WireCallbacks()
        {
            _zoneList.OnSuccess = zones => _ui.ShowServerList(zones, _selectedZoneId);
            _zoneList.OnError = msg => _ui.ShowError(msg);

            _login.OnStatus = msg => _ui.SetStatus(msg);
            _login.OnError = msg =>
            {
                _ui.ShowError(msg);
                if (_state is AppState.CharacterSelect or AppState.Connecting)
                {
                    SetState(AppState.AuthLogin);
                }
            };
            _login.OnRegisterSuccess = () =>
            {
                _ui.ShowMessage("注册成功，请登录");
                SetState(AppState.AuthLogin);
            };
            _login.OnUserList = (chars, highlight) =>
            {
                _characters = chars;
                _ui.ShowCharacterSelect(chars, highlight);
                SetState(AppState.CharacterSelect);
            };
            _login.OnEnterGame = enter =>
            {
                var tcp = _login.TakeTcpClient();
                _game.Start(tcp, enter);
                _world.BindSession(_game);
                _world.LoadMap(enter);
                SetState(AppState.Game);
            };

            _game.OnError = msg =>
            {
                _ui.ShowError(msg);
                if (_pendingExitAction.HasValue)
                {
                    _suppressDisconnectNav = false;
                    _exitDialogVisible = false;
                    _pendingExitAction = null;
                    _ui.ShowExitDialog(false);
                }
            };
            _game.OnDisconnected = () =>
            {
                if (_suppressDisconnectNav)
                {
                    return;
                }

                _ui.ShowError("游戏连接已断开");
                _exitDialogVisible = false;
                SetState(AppState.ZoneHome);
            };
            _game.OnLogoutSuccess = action =>
            {
                ClientLogger.Instance.InfoFormat("GameApp：离世界完成 action={0}", action);
                _suppressDisconnectNav = false;
                _exitDialogVisible = false;
                _pendingExitAction = null;
                _ui.ShowExitDialog(false);
                var highlightUserId = _game.LocalUserId;
                var tcp = _game.ReleaseTcpClient();
                _world.Leave();
                if (action == LogoutAction.ReturnCharSelect)
                {
                    _login.ResumeGatewayForCharSelect(tcp, highlightUserId);
                    SetState(AppState.CharacterSelect);
                }
                else
                {
                    tcp?.Disconnect();
                    _login.Cancel();
                    SetState(AppState.AuthLogin);
                }
            };

            _ui.OnSelectServerClicked = () =>
            {
                _ui.SetStatus("正在连接 LoginServer...");
                _ui.ShowError(string.Empty);
                _zoneList.FetchZoneList();
                SetState(AppState.ServerList);
            };
            _ui.OnCancelServerList = () =>
            {
                _zoneList.Cancel();
                SetState(AppState.ZoneHome);
            };
            _ui.OnGoToRegister = () => SetState(AppState.Register);
            _ui.OnBackToLogin = () => SetState(AppState.AuthLogin);
            _ui.OnZoneConfirmed = (zoneId, gameType, name) =>
            {
                _selectedZoneId = zoneId;
                _selectedGameType = gameType;
                _selectedZoneName = name;
                _localSettings.SetLastZoneId(zoneId);
                _localSettings.SetLastGameType(gameType);
                _localSettings.SetLastZoneName(name);
                _localSettings.Save();
                SetState(AppState.ZoneHome);
            };
            _ui.OnEnterGameFromHome = () => SetState(AppState.AuthLogin);
            _ui.OnLoginClicked = (account, password, remember) =>
            {
                _pendingAccount = account;
                _pendingPassword = password;
                _localSettings.SetRememberAccount(remember);
                if (remember)
                {
                    _localSettings.SetLastAccount(account);
                }

                _localSettings.Save();
                _ui.SetStatus("正在登录...");
                _ui.ShowError(string.Empty);
                _login.StartLogin(account, password, _selectedZoneId, _selectedGameType);
                SetState(AppState.Connecting);
            };
            _ui.OnRegisterClicked = (account, password, confirm) =>
            {
                _login.StartRegister(account, password, confirm, _selectedZoneId, _selectedGameType);
            };
            _ui.OnSelectCharacter = userId => _login.SelectCharacter(userId);
            _ui.OnCreateCharacter = (name, voc, sex) => _login.CreateCharacter(name, voc, sex);
            _ui.OnExitGameAction = action =>
            {
                if (action == LogoutAction.Unspecified)
                {
                    Application.Quit();
                    return;
                }

                _suppressDisconnectNav = true;
                _pendingExitAction = action;
                _game.RequestLogout(action);
            };
        }

        private void RestoreZoneFromSettings()
        {
            _selectedZoneId = _localSettings.LastZoneId;
            _selectedGameType = _localSettings.LastGameType;
            _selectedZoneName = _localSettings.LastZoneName;
        }

        private void SetState(AppState state)
        {
            _state = state;
            _ui.SetAppState(state, _selectedZoneName, _localSettings.LastAccount, _localSettings.RememberAccount);
        }
    }
}
