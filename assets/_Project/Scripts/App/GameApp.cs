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

        /// <summary>所有活跃会话的统一注册表。Update 循环自动遍历，新增 Session 类型只需添加到此列表。</summary>
        private readonly List<ISession> _sessions = new List<ISession>();

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
        private bool _mapLoading;
        private LogoutAction? _pendingExitAction;
        private long _lastHudStatusMs;
        private const long HudStatusIntervalMs = 250;

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

            _sessions.Add(_login);
            _sessions.Add(_game);

            WireCallbacks();
            _world.OnMapLoaded += OnMapSceneLoaded;
            SetState(AppState.ZoneHome);
        }

        private void Update()
        {
            _zoneList.Update();
            foreach (var session in _sessions)
            {
                session.Update();
            }

            if (_state == AppState.Game)
            {
                _scriptHost.OnTick(TimeUtil.NowMs());
                UpdateHudStatus();
            }

            if (_state == AppState.Game && Input.GetKeyDown(KeyCode.Escape))
            {
                _exitDialogVisible = !_exitDialogVisible;
                _ui.ShowExitDialog(_exitDialogVisible);
            }
        }

        private void WireCallbacks()
        {
            WireZoneCallbacks();
            WireAuthCallbacks();
            WireGameCallbacks();
            WireUICallbacks();
        }

        /// <summary>区服列表相关回调。</summary>
        private void WireZoneCallbacks()
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
        }

        /// <summary>登录/注册/角色选择相关回调。</summary>
        private void WireAuthCallbacks()
        {
            _login.OnStatus = msg => _ui.SetStatus(msg);
            _login.OnError = msg =>
            {
                _ui.ShowError(msg);
                _ui.SetStatus(msg);
                _ui.SetRegisterBusy(false);
                _ui.SetCharacterBusy(false);
                _pendingPassword = string.Empty; // 登录/注册失败后清零密码暂存

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
                    _ui.SetCharacterBusy(false);
                    if (_login.IsResumingCharSelect && _characters.Count > 0)
                    {
                        _ui.ShowCharacterSelect(_characters, 0);
                        return;
                    }

                    _login.Cancel();
                    SetState(AppState.AuthLogin);
                }
            };
            _login.OnRegisterSuccess = () =>
            {
                _registerInProgress = false;
                _ui.SetRegisterBusy(false);
                _ui.ShowMessage("注册成功，请登录");
                _pendingPassword = string.Empty; // 清零明文密码暂存
                SetState(AppState.AuthLogin);
            };
            _login.OnUserList = (chars, highlight) =>
            {
                _characters = chars;
                _pendingPassword = string.Empty; // 登录成功进入角色选择后清零
                _ui.SetCharacterBusy(false);
                _ui.ShowCharacterSelect(chars, highlight);
                SetState(AppState.CharacterSelect);
            };
            _login.OnCreateCharacterFailed = msg =>
            {
                _ui.SetCharacterBusy(false);
                _ui.ShowError(msg);
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
                _mapLoading = true;
                _ui.SetStatus("正在进入游戏...");
                var tcp = _login.TakeTcpClient();
                _game.Start(tcp, enter);
                _scriptHost.OnEnterGame(enter.UserId, enter.MapId);
                _world.BindSession(_game);
                _world.LoadMap(enter);
                _ui.GameHud?.BindModels(_scriptHost.Quests, _scriptHost.Bag);
                SetState(AppState.Game);
            };
        }

        /// <summary>游戏内会话相关回调。</summary>
        private void WireGameCallbacks()
        {
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
                    if (_state != AppState.CharacterSelect && _characters.Count > 0)
                    {
                        _ui.ShowCharacterSelect(_characters, highlightUserId);
                        SetState(AppState.CharacterSelect);
                    }
                    else if (_state != AppState.CharacterSelect)
                    {
                        _ui.SetCharacterBusy(true);
                        SetState(AppState.CharacterSelect);
                    }
                }
                else
                {
                    tcp?.Disconnect();
                    _login.Cancel();
                    SetState(AppState.AuthLogin);
                }
            };

            // HUD 聊天
            _scriptHost.OnChatMessage += (channel, sender, content) =>
            {
                _ui.GameHud?.AppendChat(channel, sender, content);
            };
            _scriptHost.OnNoticeMessage += text =>
            {
                _ui.GameHud?.AppendNotice(text);
            };
            var hud = _ui.GameHud;
            hud?.SetChatSendCallback(text =>
            {
                try
                {
                    _game.SendChat(Rpg.Proto.Chat.ChatChannel.World, text);
                    return true;
                }
                catch (System.Exception ex)
                {
                    ClientLogger.Instance.ErrFormat("GameApp：发送聊天失败 {0}", ex.Message);
                    return false;
                }
            });
        }

        /// <summary>UI 交互相关回调（区服/登录/注册/角色/退出面板）。</summary>
        private void WireUICallbacks()
        {
            _ui.OnSelectServerClicked += () =>
            {
                _ui.SetStatus("正在连接 LoginServer...");
                _ui.SetServerListHint("正在拉取区列表…");
                _ui.ShowError(string.Empty);
                _zoneList.FetchZoneList();
                SetState(AppState.ServerList);
            };
            _ui.OnCancelServerList += () =>
            {
                _zoneList.Cancel();
                SetState(AppState.ZoneHome);
            };
            _ui.OnGoToRegister += () => SetState(AppState.Register);
            _ui.OnBackToLogin += () => SetState(AppState.AuthLogin);
            _ui.OnZoneConfirmed += (zoneId, gameType, name) =>
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
            _ui.OnEnterGameFromHome += () =>
            {
                if (!ClientInputValidator.TryValidateZoneSelected(_selectedZoneId, out var err))
                {
                    _ui.ShowError(err);
                    return;
                }

                SetState(AppState.AuthLogin);
            };
            _ui.OnLoginClicked += (account, password, remember) =>
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
            _ui.OnRegisterClicked += (account, password, confirm) =>
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
            _ui.OnSelectCharacter += userId =>
            {
                if (!_login.IsCharSelectReady)
                {
                    _ui.SetCharacterBusy(false);
                    _ui.ShowError("连接已断开，请重新登录");
                    _login.Cancel();
                    SetState(AppState.AuthLogin);
                    return;
                }

                _ui.SetCharacterBusy(true);
                _login.SelectCharacter(userId);
            };
            _ui.OnCreateCharacter += (name, voc, sex) =>
            {
                if (!_login.IsCharSelectReady)
                {
                    _ui.SetCharacterBusy(false);
                    _ui.ShowError("连接已断开，请重新登录");
                    _login.Cancel();
                    SetState(AppState.AuthLogin);
                    return;
                }

                if (!ClientInputValidator.TryValidateCharacterName(name, out var err))
                {
                    _ui.ShowError(err);
                    return;
                }

                _ui.SetCharacterBusy(true);
                _ui.ShowError(string.Empty);
                _login.CreateCharacter(name.Trim(), voc, sex);
            };
            _ui.OnExitGameAction += action =>
            {
                if (action == LogoutAction.Unspecified)
                {
                    ClientLogger.Instance.Info("GameApp：退出客户端");
                    _exitDialogVisible = false;
                    _ui.ShowExitDialog(false);
                    _game.Disconnect();
                    _login.Cancel();
                    ClientLogger.Instance.Shutdown();
                    ClientApplication.RequestQuit();
                    return;
                }

                _suppressDisconnectNav = true;
                _pendingExitAction = action;
                _game.RequestLogout(action);
            };
        }

        private void UpdateHudStatus()
        {
            if (_mapLoading)
            {
                return;
            }

            var now = TimeUtil.NowMs();
            if (TimeUtil.ElapsedMs(now, _lastHudStatusMs) < HudStatusIntervalMs)
            {
                return;
            }

            _lastHudStatusMs = now;

            var hud = _ui.GameHud;
            if (hud == null)
            {
                return;
            }

            var pos = _world.EntityPosition;
            var rtt = _game.SmoothRttMs > 0 ? $" RTT:{_game.SmoothRttMs:F0}ms" : "";
            var fps = $" FPS:{Mathf.RoundToInt(1f / Mathf.Max(Time.unscaledDeltaTime, 0.0001f))}";
            hud.SetStatus($"坐标 ({pos.x:F1}, {pos.y:F1}, {pos.z:F1}){rtt}{fps}");
        }

        private void OnMapSceneLoaded()
        {
            _mapLoading = false;
            _ui.SetStatus(string.Empty);
            ClientLogger.Instance.Info("GameApp：地图场景加载完成");
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
            _world.OnMapLoaded -= OnMapSceneLoaded;
            _zoneList.ClearHandlers();
            foreach (var session in _sessions)
            {
                session.Cancel();
                session.ClearHandlers();
            }

            ClientLogger.Instance.Shutdown();
        }
    }
}
