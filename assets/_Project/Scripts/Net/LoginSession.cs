/// <summary>
/// 登录/注册/选角网络会话（对标 net/LoginSession）。
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
            Idle, ConnectLogin, WaitLoginRsp, SwitchingGateway, ConnectGateway,
            WaitUserList, WaitUserAction, WaitCreateUserRsp, WaitEnterGame,
            RegisterConnect, RegisterWaitRsp
        }

        private GameTcpClient _tcp;
        private ClientConfigLoader _config;
        private State _state = State.Idle;
        private bool _registerFlow;
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
        private bool _gotUserList;
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
            _registerFlow = false;
            _account = account;
            _password = password;
            _zoneId = zoneId;
            _gameType = gameType;
            _state = State.ConnectLogin;
            _connectStartMs = TimeUtil.NowMs();
            ClientLogger.Instance.Info("LoginSession：开始登录");
            _tcp.Connect(_config.LoginHost, _config.LoginPort, _config.Tls);
        }

        public void StartRegister(string account, string password, string confirm, uint zoneId, byte gameType)
        {
            Cancel();
            _registerFlow = true;
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
            _pendingSelectUserId = userId;
            _tcp.SendRaw(ClientMsgHandler.BuildSelectUserReq(userId, _nextLoginTxnId++));
            _state = State.WaitEnterGame;
            _waitStartMs = TimeUtil.NowMs();
            NotifyStatus("正在进入游戏...");
        }

        public void CreateCharacter(string name, byte vocation, byte sex)
        {
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
            CheckTimeouts();
        }

        public void Cancel()
        {
            _state = State.Idle;
            _gatewayConnected = false;
            _gotLoginRsp = _gotGatewayInfo = _gotUserList = false;
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
            NotifyStatus("正在刷新角色列表...");
        }

        private void CheckTimeouts()
        {
            var now = TimeUtil.NowMs();
            if (_state is State.ConnectLogin or State.RegisterConnect or State.ConnectGateway &&
                now - _connectStartMs > (_config?.ConnectTimeoutMs ?? ClientTimingDefaults.ConnectTimeoutMs))
            {
                Fail(ClientLocalError.ConnectTimeout);
                return;
            }

            if (IsWaitingResponse() &&
                now - _waitStartMs > (_config?.ResponseTimeoutMs ?? ClientTimingDefaults.ResponseTimeoutMs))
            {
                Fail(ClientLocalError.ResponseTimeout);
            }
        }

        private bool IsWaitingResponse() =>
            _state is State.WaitLoginRsp or State.RegisterWaitRsp or State.WaitUserList
                or State.WaitCreateUserRsp or State.WaitEnterGame;

        private void OnConnected()
        {
            switch (_state)
            {
                case State.ConnectLogin:
                    _tcp.SendRaw(ClientMsgHandler.BuildLoginReq(_account, _password, _zoneId, _gameType));
                    _state = State.WaitLoginRsp;
                    _waitStartMs = TimeUtil.NowMs();
                    break;
                case State.RegisterConnect:
                    _tcp.SendRaw(ClientMsgHandler.BuildRegisterReq(_account, _password, _confirmPassword, _zoneId, _gameType));
                    _state = State.RegisterWaitRsp;
                    _waitStartMs = TimeUtil.NowMs();
                    break;
                case State.ConnectGateway:
                    _tcp.SendRaw(ClientMsgHandler.BuildGatewayAuthReq(_account, _loginRsp.LoginToken, _zoneId, _gameType));
                    _state = State.WaitUserList;
                    _waitStartMs = TimeUtil.NowMs();
                    _gatewayConnected = true;
                    break;
            }
        }

        private void OnDisconnected()
        {
            if (_state != State.Idle)
            {
                Fail(ClientLocalError.Disconnected);
            }
        }

        private void OnMessage(byte module, byte sub, byte[] body)
        {
            if (module == (byte)ClientModule.System)
            {
                if (sub == (byte)SystemMsgSub.S2CError && ClientMsgHandler.TryParseGatewayError(body, out var err))
                {
                    Fail(ClientErrorText.GatewayErrorText(err));
                }

                return;
            }

            if (module != (byte)ClientModule.Login)
            {
                return;
            }

            switch ((LoginMsgSub)sub)
            {
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

        private void HandleLoginRsp(byte[] body)
        {
            if (!ClientMsgHandler.TryParseLoginRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            if (_gatewayConnected && _state == State.WaitUserList)
            {
                ClientLogger.Instance.Info("LoginSession：Gateway 鉴权成功");
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
                Fail("网关信息错误：" + info.Msg);
                return;
            }

            _gatewayInfo = info;
            _gotGatewayInfo = true;
            _gatewayHost = info.GatewayIp ?? string.Empty;
            _gatewayPort = (ushort)info.GatewayPort;
            if (IsLoopback(_gatewayHost))
            {
                _gatewayHost = _config.LoginHost;
            }

            TryConnectGateway();
        }

        private void HandleUserList(byte[] body)
        {
            if (!ClientMsgHandler.TryParseUserList(body, out var list))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            _gotUserList = true;
            _state = State.WaitUserAction;
            var chars = ClientMsgHandler.ToCharacterEntries(list);
            _cachedCharacters.Clear();
            _cachedCharacters.AddRange(chars);
            var highlight = _highlightUserId != 0 ? _highlightUserId : _loginRsp?.UserId ?? 0;
            _highlightUserId = 0;
            OnUserList?.Invoke(chars, highlight);
        }

        private void HandleCreateUserRsp(byte[] body)
        {
            if (!ClientMsgHandler.TryParseCreateUserRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            var err = ClientErrorText.CreateCharacterText(rsp.Code, rsp.Msg);
            if (!string.IsNullOrEmpty(err))
            {
                Fail(err);
                return;
            }

            _state = State.WaitUserAction;
            if (rsp.UserId != 0)
            {
                _cachedCharacters.Add(new CharacterEntry
                {
                    UserId = rsp.UserId,
                    Name = _pendingCreateName,
                    Level = 1,
                    Vocation = _pendingCreateVocation,
                    Sex = _pendingCreateSex
                });
                OnUserList?.Invoke(new List<CharacterEntry>(_cachedCharacters), rsp.UserId);
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

            _state = State.Idle;
            ClientLogger.Instance.InfoFormat("LoginSession：进入游戏 mapId={0}", enter.MapId);
            OnEnterGame?.Invoke(enter);
        }

        private static bool IsLoopback(string host) =>
            host == "127.0.0.1" || host == "localhost" || host == "::1";

        private void Fail(ClientLocalError err) => Fail(ClientErrorText.LocalErrorText(err));

        private void Fail(string msg)
        {
            _state = State.Idle;
            _gatewayConnected = false;
            _tcp.Disconnect();
            ClientLogger.Instance.WarnFormat("LoginSession：{0}", msg);
            OnError?.Invoke(msg);
        }

        private void NotifyStatus(string msg) => OnStatus?.Invoke(msg);
    }
}
