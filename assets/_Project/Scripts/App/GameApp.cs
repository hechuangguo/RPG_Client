/// <summary>
/// 游戏主控制器。
/// 职责：AppState 状态机、Session 生命周期、UI 切换。
/// 协作：LoginSession、GameSession、ZoneListSession、GameUiController、WorldController。
/// </summary>
using System.Collections.Generic;
using Rpg.Client.Config;
using Rpg.Client.Log;
using Rpg.Client.Net;
using Rpg.Client.Scripting;
using Rpg.Client.UI;
using Rpg.Client.Util;
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
        private readonly GameScriptHost _scriptHost = new GameScriptHost();

        private AppState _state = AppState.ZoneHome;
        private uint _selectedZoneId;
        private byte _selectedGameType;
        private string _selectedZoneName = string.Empty;
        private string _pendingAccount = string.Empty;
        private string _pendingPassword = string.Empty;
        private List<CharacterEntry> _characters = new List<CharacterEntry>();
        private List<GameZoneEntry> _cachedZones = new List<GameZoneEntry>();
        private bool _suppressDisconnectNav;
        private bool _exitDialogVisible;
        private bool _registerInProgress;
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
            _game.SetScriptHost(_scriptHost);

            WireCallbacks();
            SetState(AppState.ZoneHome);
        }

        private void Update()
        {
            _zoneList.Update();
            _login.Update();
            _game.Update();

            if (_state == AppState.Game)
            {
                _scriptHost.OnTick(TimeUtil.NowMs());
            }

            if (_state == AppState.Game && Input.GetKeyDown(KeyCode.Escape))
            {
                _exitDialogVisible = !_exitDialogVisible;
                _ui.ShowExitDialog(_exitDialogVisible);
            }
        }

        private void WireCallbacks()
        {
            _zoneList.OnSuccess = zones =>
            {
                _cachedZones = zones ?? new List<GameZoneEntry>();
                SyncSelectedZoneFromList(_cachedZones);
                SetState(AppState.ServerList);
                _ui.ShowServerList(_cachedZones, _selectedZoneId);
            };
            _zoneList.OnError = msg =>
            {
                _ui.ShowError(msg);
                _ui.SetServerListHint(msg);
            };

            _login.OnStatus = msg => _ui.SetStatus(msg);
            _login.OnError = msg =>
            {
                _ui.ShowError(msg);
                _ui.SetStatus(msg);
                _ui.SetRegisterBusy(false);
                _ui.SetCharacterBusy(false);

                if (_registerInProgress)
                {
                    _registerInProgress = false;
                    SetState(AppState.Register);
                    return;
                }

                if (_state == AppState.Connecting)
                {
                    SetState(AppState.AuthLogin);
                }
                else if (_state == AppState.CharacterSelect)
                {
                    SetState(AppState.CharacterSelect);
                }
            };
            _login.OnRegisterSuccess = () =>
            {
                _registerInProgress = false;
                _ui.SetRegisterBusy(false);
                _ui.ShowMessage("注册成功，请登录");
                SetState(AppState.AuthLogin);
            };
            _login.OnUserList = (chars, highlight) =>
            {
                _characters = chars;
                _ui.SetCharacterBusy(false);
                _ui.ShowCharacterSelect(chars, highlight);
                SetState(AppState.CharacterSelect);
            };
            _login.OnEnterGame = enter =>
            {
                if (enter.UserId == 0 || enter.MapId == 0)
                {
                    ClientLogger.Instance.Err("GameApp：进世界数据无效，拒绝启动 GameSession");
                    _ui.ShowError("进世界失败：服务器返回数据无效");
                    SetState(AppState.AuthLogin);
                    return;
                }

                _ui.SetCharacterBusy(false);
                var tcp = _login.TakeTcpClient();
                _game.Start(tcp, enter);
                _scriptHost.OnEnterGame(enter.UserId, enter.MapId);
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
                _ui.SetServerListHint("正在拉取区列表…");
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
                SyncSelectedZoneFromList(_cachedZones);
                _localSettings.SetLastZoneId(_selectedZoneId);
                _localSettings.SetLastGameType(_selectedGameType);
                _localSettings.SetLastZoneName(_selectedZoneName);
                _localSettings.Save();
                SetState(AppState.ZoneHome);
            };
            _ui.OnEnterGameFromHome = () =>
            {
                if (!ClientInputValidator.TryValidateZoneSelected(_selectedZoneId, out var err))
                {
                    _ui.ShowError(err);
                    return;
                }

                SetState(AppState.AuthLogin);
            };
            _ui.OnLoginClicked = (account, password, remember) =>
            {
                account = account?.Trim() ?? string.Empty;
                password = password ?? string.Empty;
                SyncSelectedZoneFromList(_cachedZones);

                if (_cachedZones.Count > 0)
                {
                    if (!ClientInputValidator.TryValidateZoneForLogin(
                            _selectedZoneId, _selectedGameType, _cachedZones, out var zoneErr))
                    {
                        _ui.ShowError(zoneErr);
                        _ui.SetStatus(zoneErr);
                        return;
                    }
                }
                else if (!ClientInputValidator.TryValidateZoneSelected(_selectedZoneId, out var staleZoneErr))
                {
                    _ui.ShowError(staleZoneErr);
                    _ui.SetStatus(staleZoneErr);
                    return;
                }

                if (!ClientInputValidator.TryValidateAccount(account, out var accountErr))
                {
                    _ui.ShowError(accountErr);
                    return;
                }

                if (!ClientInputValidator.TryValidatePassword(password, out var passwordErr))
                {
                    _ui.ShowError(passwordErr);
                    return;
                }

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
                account = account?.Trim() ?? string.Empty;
                password = password ?? string.Empty;
                confirm = confirm ?? string.Empty;
                SyncSelectedZoneFromList(_cachedZones);

                if (_cachedZones.Count > 0)
                {
                    if (!ClientInputValidator.TryValidateZoneForLogin(
                            _selectedZoneId, _selectedGameType, _cachedZones, out var zoneErr))
                    {
                        _ui.ShowError(zoneErr);
                        _ui.SetStatus(zoneErr);
                        return;
                    }
                }
                else if (!ClientInputValidator.TryValidateZoneSelected(_selectedZoneId, out var staleZoneErr))
                {
                    _ui.ShowError(staleZoneErr);
                    _ui.SetStatus(staleZoneErr);
                    return;
                }

                if (!ClientInputValidator.TryValidateAccount(account, out var accountErr))
                {
                    _ui.ShowError(accountErr);
                    return;
                }

                if (!ClientInputValidator.TryValidatePasswordMatch(password, confirm, out var passwordErr))
                {
                    _ui.ShowError(passwordErr);
                    return;
                }

                _registerInProgress = true;
                _ui.SetRegisterBusy(true);
                _ui.ShowError(string.Empty);
                _ui.SetStatus("正在注册...");
                _login.StartRegister(account, password, confirm, _selectedZoneId, _selectedGameType);
                SetState(AppState.Connecting);
            };
            _ui.OnSelectCharacter = userId =>
            {
                _ui.SetCharacterBusy(true);
                _login.SelectCharacter(userId);
            };
            _ui.OnCreateCharacter = (name, voc, sex) =>
            {
                if (!ClientInputValidator.TryValidateCharacterName(name, out var err))
                {
                    _ui.ShowError(err);
                    return;
                }

                _ui.SetCharacterBusy(true);
                _ui.ShowError(string.Empty);
                _login.CreateCharacter(name.Trim(), voc, sex);
            };
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

        /// <summary>用最新区列表校正 zoneId/gameType，避免本地缓存与服务端不一致。</summary>
        private void SyncSelectedZoneFromList(IReadOnlyList<GameZoneEntry> zones)
        {
            if (_selectedZoneId == 0 || zones == null)
            {
                return;
            }

            foreach (var zone in zones)
            {
                if (zone.ZoneId != _selectedZoneId)
                {
                    continue;
                }

                _selectedGameType = zone.GameType;
                if (!string.IsNullOrEmpty(zone.Name))
                {
                    _selectedZoneName = zone.Name;
                }

                return;
            }
        }

        private void SetState(AppState state)
        {
            _state = state;
            _ui.SetAppState(state, _selectedZoneName, _localSettings.LastAccount, _localSettings.RememberAccount);
        }

        private void OnDestroy()
        {
            _zoneList.ClearHandlers();
            _login.ClearHandlers();
            _game.ClearHandlers();
            ClientLogger.Instance.Shutdown();
        }
    }
}
