/// <summary>
/// 登录/注册/选角网络会话。
/// 职责：LoginServer 登录 → Gateway 鉴权 → 角色列表 → 进世界。
/// 协作：GameApp、ClientMsgHandler、GameSession。
/// 线程：Unity 主线程 Update 驱动 poll。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.Config;
using Rpg.Client.Log;
using Rpg.Client.Util;
using Rpg.Proto.Login;
using Rpg.Proto.System;

namespace Rpg.Client.Net
{
    public sealed class LoginSession
    {
        private enum State
        {
            Idle, ConnectLogin, WaitLoginChallenge, WaitLoginRsp, SwitchingGateway, ConnectGateway,
            WaitUserList, WaitUserAction, WaitCreateUserRsp, WaitEnterGame,
            RegisterConnect, RegisterWaitRsp
        }

        private GameTcpClient _tcp;
        private ClientConfigLoader _config;
        private State _state = State.Idle;
        private string _account = string.Empty;
        private string _password = string.Empty;
        private string _confirmPassword = string.Empty;
        private uint _zoneId;
        private byte _gameType;
        private S2CLoginRsp _loginRsp;
        private S2CGatewayInfo _gatewayInfo;
        private string _gatewayHost = string.Empty;
        private ushort _gatewayPort;
        private ulong _pendingSelectUserId;
        private ulong _highlightUserId;
        private ulong _nextLoginTxnId = 1;
        private string _pendingCreateName = string.Empty;
        private byte _pendingCreateVocation;
        private byte _pendingCreateSex;
        private long _connectStartMs;
        private long _waitStartMs;
        private bool _gatewayConnected;
        private bool _gotLoginRsp;
        private bool _gotGatewayInfo;
        private bool _gotGatewayAuthRsp;
        private bool _gotUserList;
        private bool _intentionalDisconnect;
        private bool _isRegisterFlow;
        private byte[] _loginNonce;
        private long _lastHeartbeatMs;
        private uint _heartbeatSeq;
        private readonly List<CharacterEntry> _cachedCharacters = new List<CharacterEntry>();

        public Action<S2CEnterGame> OnEnterGame;
        public Action<string> OnError;
        public Action OnRegisterSuccess;
        public Action<List<CharacterEntry>, ulong> OnUserList;
        public Action<string> OnStatus;

        public LoginSession()
        {
            _tcp = CreateTcp();
        }

        public void SetConfig(ClientConfigLoader config) => _config = config;
        public bool GatewayConnected => _gatewayConnected;
        public string GatewayHost => _gatewayHost;
        public ushort GatewayPort => _gatewayPort;

        public void StartLogin(string account, string password, uint zoneId, byte gameType)
        {
            Cancel();
            _isRegisterFlow = false;
            _loginNonce = null;
            _account = account;
            _password = password;
            _zoneId = zoneId;
            _gameType = gameType;
            _state = State.ConnectLogin;
            _connectStartMs = TimeUtil.NowMs();
            ClientLogger.Instance.InfoFormat(
                "LoginSession：开始登录 zoneId={0} gameType={1}", zoneId, gameType);
            _tcp.Connect(_config.LoginHost, _config.LoginPort, _config.Tls);
        }

        public void StartRegister(string account, string password, string confirm, uint zoneId, byte gameType)
        {
            Cancel();
            _isRegisterFlow = true;
            _loginNonce = null;
            _account = account;
            _password = password;
            _confirmPassword = confirm;
            _zoneId = zoneId;
            _gameType = gameType;
            _state = State.RegisterConnect;
            _connectStartMs = TimeUtil.NowMs();
            _tcp.Connect(_config.LoginHost, _config.LoginPort, _config.Tls);
        }

        public void SelectCharacter(ulong userId)
        {
            if (userId == 0)
            {
                ClientLogger.Instance.Warn("LoginSession：无效角色 ID，拒绝进世界");
                OnError?.Invoke("请先选择或创建角色");
                return;
            }

            if (!CanSendUserAction())
            {
                ClientLogger.Instance.WarnFormat("LoginSession：当前状态 {0} 不允许选角进世界", _state);
                return;
            }

            _pendingSelectUserId = userId;
            _tcp.SendRaw(ClientMsgHandler.BuildSelectUserReq(userId, _nextLoginTxnId++));
            _state = State.WaitEnterGame;
            _waitStartMs = TimeUtil.NowMs();
            NotifyStatus("正在进入游戏...");
        }

        public void CreateCharacter(string name, byte vocation, byte sex)
        {
            if (!CanSendUserAction())
            {
                ClientLogger.Instance.WarnFormat("LoginSession：当前状态 {0} 不允许创角", _state);
                return;
            }

            _pendingCreateName = name;
            _pendingCreateVocation = vocation;
            _pendingCreateSex = sex;
            _tcp.SendRaw(ClientMsgHandler.BuildCreateUserReq(name, vocation, sex));
            _state = State.WaitCreateUserRsp;
            _waitStartMs = TimeUtil.NowMs();
        }

        public void Update()
        {
            _tcp.Poll();
            SendGatewayHeartbeatIfNeeded();
            CheckTimeouts();
        }

        public void ClearHandlers()
        {
            OnEnterGame = null;
            OnError = null;
            OnRegisterSuccess = null;
            OnUserList = null;
            OnStatus = null;
        }

        public void Cancel()
        {
            _state = State.Idle;
            _gatewayConnected = false;
            _gotLoginRsp = _gotGatewayInfo = _gotGatewayAuthRsp = _gotUserList = false;
            _intentionalDisconnect = false;
            _isRegisterFlow = false;
            _loginNonce = null;
            _cachedCharacters.Clear();
            _tcp.Disconnect();
        }

        public GameTcpClient TakeTcpClient()
        {
            var tcp = _tcp;
            _tcp = CreateTcp();
            _gatewayConnected = false;
            _state = State.Idle;
            return tcp;
        }

        private GameTcpClient CreateTcp()
        {
            var tcp = new GameTcpClient();
            tcp.SetOnMessage(OnMessage);
            tcp.SetOnConnected(OnConnected);
            tcp.SetOnDisconnected(OnDisconnected);
            return tcp;
        }

        public void ResumeGatewayForCharSelect(GameTcpClient tcp, ulong highlightUserId)
        {
            _tcp?.Disconnect();
            _tcp = tcp ?? CreateTcp();
            _tcp.SetOnMessage(OnMessage);
            _tcp.SetOnConnected(OnConnected);
            _tcp.SetOnDisconnected(OnDisconnected);
            _highlightUserId = highlightUserId;
            _gatewayConnected = true;
            _gotUserList = false;
            _state = State.WaitUserList;
            _waitStartMs = TimeUtil.NowMs();
            _lastHeartbeatMs = TimeUtil.NowMs();
            NotifyStatus("正在刷新角色列表...");
        }

        private void SendGatewayHeartbeatIfNeeded()
        {
            if (!_gatewayConnected || _state == State.Idle)
            {
                return;
            }

            var now = TimeUtil.NowMs();
            var interval = _config?.HeartbeatIntervalMs ?? ClientTimingDefaults.HeartbeatIntervalMs;
            if (TimeUtil.ElapsedMs(now, _lastHeartbeatMs) < interval)
            {
                return;
            }

            _lastHeartbeatMs = now;
            _tcp.SendRaw(ClientMsgHandler.BuildHeartbeat(_heartbeatSeq++));
        }

        private bool CanSendUserAction() =>
            _state == State.WaitUserAction && _gatewayConnected;

        private void CheckTimeouts()
        {
            var now = TimeUtil.NowMs();
            var connectTimeout = _config?.ConnectTimeoutMs ?? ClientTimingDefaults.ConnectTimeoutMs;
            if ((_state is State.ConnectLogin or State.RegisterConnect or State.ConnectGateway
                     or State.WaitLoginChallenge) &&
                TimeUtil.ElapsedMs(now, _connectStartMs) > connectTimeout)
            {
                if (_state == State.WaitLoginChallenge)
                {
                    Fail(ClientLocalError.ResponseTimeout);
                }
                else if (_state == State.ConnectGateway)
                {
                    Fail(ClientErrorText.GatewayConnectFailedText(_gatewayHost, _gatewayPort));
                }
                else
                {
                    Fail(ClientLocalError.ConnectTimeout);
                }
                return;
            }

            if (!IsWaitingResponse())
            {
                return;
            }

            if (_state == State.WaitLoginChallenge)
            {
                return;
            }

            if (TimeUtil.ElapsedMs(now, _waitStartMs) <= (_config?.ResponseTimeoutMs ?? ClientTimingDefaults.ResponseTimeoutMs))
            {
                return;
            }

            var ctx = _state switch
            {
                State.WaitUserList when !_gotGatewayAuthRsp => LoginTimeoutContext.Gateway,
                State.WaitUserList => LoginTimeoutContext.UserList,
                State.RegisterWaitRsp => LoginTimeoutContext.Register,
                _ => LoginTimeoutContext.Generic
            };
            Fail(ClientErrorText.LocalErrorText(ClientLocalError.ResponseTimeout, ctx));
        }

        private bool IsWaitingResponse() =>
            _state is State.WaitLoginRsp or State.RegisterWaitRsp or State.WaitUserList
                or State.WaitCreateUserRsp or State.WaitEnterGame or State.WaitLoginChallenge;

        private void OnConnected()
        {
            switch (_state)
            {
                case State.ConnectLogin:
                    _state = State.WaitLoginChallenge;
                    _connectStartMs = TimeUtil.NowMs();
                    NotifyStatus("等待登录挑战...");
                    break;
                case State.RegisterConnect:
                    _state = State.WaitLoginChallenge;
                    _connectStartMs = TimeUtil.NowMs();
                    NotifyStatus("等待注册挑战...");
                    break;
                case State.ConnectGateway:
                    var tokenLen = _loginRsp?.LoginToken?.Length ?? 0;
                    ClientLogger.Instance.InfoFormat(
                        "LoginSession：发送 Gateway 票据鉴权 账号={0} 区服={1} gameType={2} tokenLen={3}",
                        _account, _zoneId, _gameType, tokenLen);
                    _tcp.SendRaw(ClientMsgHandler.BuildGatewayAuthReq(
                        _account, _loginRsp.LoginToken, _zoneId, _gameType));
                    _state = State.WaitUserList;
                    _gotGatewayAuthRsp = false;
                    _waitStartMs = TimeUtil.NowMs();
                    _gatewayConnected = true;
                    _lastHeartbeatMs = TimeUtil.NowMs();
                    NotifyStatus("正在验证登录票据...");
                    break;
            }
        }

        private void OnDisconnected()
        {
            if (_intentionalDisconnect)
            {
                _intentionalDisconnect = false;
                return;
            }

            if (_state != State.Idle)
            {
                if (_state == State.ConnectGateway)
                {
                    Fail(ClientErrorText.GatewayConnectFailedText(_gatewayHost, _gatewayPort));
                    return;
                }

                Fail(ClientLocalError.Disconnected);
            }
        }

        private void OnMessage(byte module, byte sub, byte[] body)
        {
            if (module == (byte)ClientModule.System)
            {
                if (sub == (byte)SystemMsgSub.S2CError)
                {
                    if (ClientMsgHandler.TryParseGatewayError(body, out var err))
                    {
                        Fail(ClientErrorText.GatewayErrorText(err));
                    }
                }
                else if (sub == (byte)SystemMsgSub.S2CHeartbeat)
                {
                    ClientMsgHandler.TryParseHeartbeat(body, out _);
                }

                return;
            }

            if (module != (byte)ClientModule.Login)
            {
                return;
            }

            switch ((LoginMsgSub)sub)
            {
                case LoginMsgSub.S2CLoginChallenge:
                    HandleLoginChallenge(body);
                    break;
                case LoginMsgSub.S2CLoginRsp:
                    HandleLoginRsp(body);
                    break;
                case LoginMsgSub.S2CRegisterRsp:
                    HandleRegisterRsp(body);
                    break;
                case LoginMsgSub.S2CGatewayInfo:
                    HandleGatewayInfo(body);
                    break;
                case LoginMsgSub.S2CUserList:
                    HandleUserList(body);
                    break;
                case LoginMsgSub.S2CCreateUserRsp:
                    HandleCreateUserRsp(body);
                    break;
                case LoginMsgSub.S2CEnterGame:
                    HandleEnterGame(body);
                    break;
            }
        }

        private void HandleLoginChallenge(byte[] body)
        {
            if (_state != State.WaitLoginChallenge)
            {
                return;
            }

            if (!ClientMsgHandler.TryParseLoginChallenge(body, out var challenge) ||
                challenge.Nonce == null || challenge.Nonce.Length == 0)
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            _loginNonce = challenge.Nonce.ToByteArray();
            ClientLogger.Instance.InfoFormat(
                "LoginSession：收到登录挑战 nonceLen={0}", _loginNonce.Length);
            if (_isRegisterFlow)
            {
                _tcp.SendRaw(ClientMsgHandler.BuildRegisterReq(
                    _account, _password, _confirmPassword, _loginNonce, _zoneId, _gameType));
                _state = State.RegisterWaitRsp;
            }
            else
            {
                _tcp.SendRaw(ClientMsgHandler.BuildLoginReq(
                    _account, _password, _loginNonce, _zoneId, _gameType));
                _state = State.WaitLoginRsp;
            }

            _waitStartMs = TimeUtil.NowMs();
        }

        private void HandleLoginRsp(byte[] body)
        {
            if (_gatewayConnected && _state == State.WaitUserList)
            {
                if (!ClientMsgHandler.TryParseLoginRsp(body, out var gwRsp))
                {
                    Fail(ClientLocalError.ParseError);
                    return;
                }

                var gatewayErr = ClientErrorText.GatewayLoginResultText(gwRsp.Code, gwRsp.Msg);
                if (!string.IsNullOrEmpty(gatewayErr))
                {
                    Fail(gatewayErr);
                    return;
                }

                _gotGatewayAuthRsp = true;
                _waitStartMs = TimeUtil.NowMs();
                ClientLogger.Instance.Info("LoginSession：Gateway 鉴权成功");
                NotifyStatus("正在获取角色列表...");
                return;
            }

            if (!ClientMsgHandler.TryParseLoginRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            var err = ClientErrorText.LoginResultText(rsp.Code, rsp.Msg);
            if (!string.IsNullOrEmpty(err))
            {
                Fail(err);
                return;
            }

            _loginRsp = rsp;
            _gotLoginRsp = true;
            ClientLogger.Instance.InfoFormat("LoginSession：登录成功 accId={0}", rsp.Accid);
            if (!_gotUserList)
            {
                _state = State.WaitUserList;
                _waitStartMs = TimeUtil.NowMs();
                NotifyStatus("正在获取角色列表...");
                TryConnectGateway();
            }
        }

        private void TryConnectGateway()
        {
            if (!_gotLoginRsp || !_gotGatewayInfo || _gotUserList)
            {
                return;
            }

            if (_gatewayConnected || _state is State.SwitchingGateway or State.ConnectGateway or State.WaitEnterGame)
            {
                return;
            }

            ClientLogger.Instance.InfoFormat("LoginSession：连接 Gateway {0}:{1}", _gatewayHost, _gatewayPort);
            _state = State.SwitchingGateway;
            _intentionalDisconnect = true;
            _tcp.Disconnect();
            _state = State.ConnectGateway;
            _connectStartMs = TimeUtil.NowMs();
            _tcp.Connect(_gatewayHost, _gatewayPort, _config.Tls);
        }

        private void HandleRegisterRsp(byte[] body)
        {
            if (!ClientMsgHandler.TryParseRegisterRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            var err = ClientErrorText.RegisterResultText(rsp.Code, rsp.Msg);
            if (!string.IsNullOrEmpty(err))
            {
                Fail(err);
                return;
            }

            _state = State.Idle;
            _tcp.Disconnect();
            OnRegisterSuccess?.Invoke();
        }

        private void HandleGatewayInfo(byte[] body)
        {
            if (!ClientMsgHandler.TryParseGatewayInfo(body, out var info))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            if (info.Code != (int)GatewayInfoResultCode.GatewayInfoOk)
            {
                var err = ClientErrorText.GatewayInfoResultText(info.Code, info.Msg);
                ClientLogger.Instance.WarnFormat(
                    "LoginSession：网关信息失败 code={0} zoneId={1} gameType={2} msg={3}",
                    info.Code, _zoneId, _gameType, info.Msg);
                Fail(err);
                return;
            }

            _gatewayInfo = info;
            _gotGatewayInfo = true;
            _gatewayHost = info.GatewayIp ?? string.Empty;
            _gatewayPort = (ushort)info.GatewayPort;
            ApplyGatewayHostOverride();

            TryConnectGateway();
        }

        private void HandleUserList(byte[] body)
        {
            if (!ClientMsgHandler.TryParseUserList(body, out var list))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            var err = ClientErrorText.UserListResultText(list.Code, string.Empty);
            var chars = ClientMsgHandler.ToCharacterEntries(list);
            if (!string.IsNullOrEmpty(err))
            {
                Fail(err);
                return;
            }

            _gotUserList = true;
            _state = State.WaitUserAction;
            _cachedCharacters.Clear();
            _cachedCharacters.AddRange(chars);
            var highlight = _highlightUserId != 0 ? _highlightUserId : _loginRsp?.UserId ?? 0;
            _highlightUserId = 0;
            ClientLogger.Instance.InfoFormat("LoginSession：收到角色列表 数量={0}", chars.Count);
            OnUserList?.Invoke(new List<CharacterEntry>(_cachedCharacters), highlight);
        }

        private void HandleCreateUserRsp(byte[] body)
        {
            if (!ClientMsgHandler.TryParseCreateUserRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            var err = ClientErrorText.CreateCharacterText(rsp.Code, rsp.Msg);
            var userId = rsp.UserId;
            if (!string.IsNullOrEmpty(err))
            {
                Fail(err);
                return;
            }

            _state = State.WaitUserAction;
            if (userId != 0)
            {
                _cachedCharacters.Add(new CharacterEntry
                {
                    UserId = userId,
                    Name = _pendingCreateName,
                    Level = 1,
                    Vocation = _pendingCreateVocation,
                    Sex = _pendingCreateSex
                });
                OnUserList?.Invoke(new List<CharacterEntry>(_cachedCharacters), userId);
            }

            NotifyStatus("创角成功");
        }

        private void HandleEnterGame(byte[] body)
        {
            if (!ClientMsgHandler.TryParseEnterGame(body, out var enter))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            if (enter.UserId == 0 || enter.MapId == 0)
            {
                Fail("进世界数据无效");
                return;
            }

            _state = State.Idle;
            ClientLogger.Instance.InfoFormat("LoginSession：进入游戏 mapId={0}", enter.MapId);
            OnEnterGame?.Invoke(enter);
        }

        private void ApplyGatewayHostOverride()
        {
            var serverHost = _gatewayHost;
            if (!string.IsNullOrWhiteSpace(_config?.GatewayHost))
            {
                _gatewayHost = _config.GatewayHost.Trim();
                ClientLogger.Instance.InfoFormat(
                    "LoginSession：Gateway 地址已覆盖为配置 gatewayHost={0}（服务端下发 {1}）",
                    _gatewayHost, serverHost);
                return;
            }

            if (_config?.GatewayUseLoginHost == true)
            {
                _gatewayHost = _config.LoginHost;
                ClientLogger.Instance.InfoFormat(
                    "LoginSession：gatewayUseLoginHost，Gateway 地址改用 loginHost={0}（服务端下发 {1}）",
                    _gatewayHost, serverHost);
                return;
            }

            if (IsLoopback(_gatewayHost))
            {
                _gatewayHost = _config.LoginHost;
                ClientLogger.Instance.InfoFormat(
                    "LoginSession：Gateway 回环地址已替换为 loginHost={0}", _gatewayHost);
            }
        }

        private static bool IsLoopback(string host) =>
            host == "127.0.0.1" || host == "localhost" || host == "::1";

        private void Fail(ClientLocalError err) => Fail(ClientErrorText.LocalErrorText(err));

        private void Fail(string msg)
        {
            _state = State.Idle;
            _gatewayConnected = false;
            _intentionalDisconnect = false;
            _tcp.Disconnect();
            ClientLogger.Instance.WarnFormat("LoginSession：{0}", msg);
            OnError?.Invoke(msg);
        }

        private void NotifyStatus(string msg) => OnStatus?.Invoke(msg);
    }
}
