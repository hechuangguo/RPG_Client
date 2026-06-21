/// <summary>
/// 区列表短会话。
/// 职责：连接 LoginServer，发送 C2SZoneListReq，解析 S2CZoneListRsp 后断开。
/// </summary>
using System;
using System.Collections.Generic;
using Rpg.Client.Config;
using Rpg.Client.Log;
using Rpg.Client.Util;
using Rpg.Proto.Login;
using Rpg.Proto.System;
using Rpg.Proto.Zone;

namespace Rpg.Client.Net
{
    public sealed class ZoneListSession
    {
        private enum State { Idle, Connecting, WaitResponse }

        private readonly GameTcpClient _tcp = new GameTcpClient();
        private ClientConfigLoader _config;
        private State _state = State.Idle;
        private long _waitStartMs;
        private long _connectStartMs;
        private int _fetchToken;
        private int _activeFetchToken;

        public Action<List<GameZoneEntry>> OnSuccess;
        public Action<string> OnError;

        public ZoneListSession()
        {
            _tcp.SetOnMessage(OnMessage);
            _tcp.SetOnConnected(OnConnected);
            _tcp.SetOnDisconnected(OnDisconnected);
        }

        public void SetConfig(ClientConfigLoader config) => _config = config;

        public bool IsBusy => _state != State.Idle;

        public void FetchZoneList(byte gameType = WireConstants.ZoneListAllGameTypes)
        {
            Cancel();
            _activeFetchToken = ++_fetchToken;
            _state = State.Connecting;
            _connectStartMs = TimeUtil.NowMs();
            _waitStartMs = _connectStartMs;
            _tcp.Connect(_config.LoginHost, _config.LoginPort, _config.Tls);
        }

        public void Update()
        {
            _tcp.Poll();
            CheckTimeouts();
        }

        public void Cancel()
        {
            if (_activeFetchToken != 0)
            {
                _fetchToken++;
                _activeFetchToken = 0;
            }

            _state = State.Idle;
            _tcp.Disconnect();
        }

        private void CheckTimeouts()
        {
            var now = TimeUtil.NowMs();
            var connectTimeout = _config?.ConnectTimeoutMs ?? ClientTimingDefaults.ConnectTimeoutMs;

            if (_state == State.Connecting && now - _connectStartMs > connectTimeout)
            {
                Fail(ClientLocalError.ConnectTimeout);
                return;
            }

            if (_state == State.WaitResponse &&
                now - _waitStartMs > (_config?.ZoneListResponseTimeoutMs ?? ClientTimingDefaults.ZoneListResponseTimeoutMs))
            {
                Fail(ClientLocalError.ZoneListTimeout);
            }
        }

        private void OnConnected()
        {
            _tcp.SendRaw(ClientMsgHandler.BuildZoneListReq());
            _state = State.WaitResponse;
            _waitStartMs = TimeUtil.NowMs();
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
            if (_activeFetchToken == 0)
            {
                return;
            }

            if (module == (byte)ClientModule.System && sub == (byte)SystemMsgSub.S2CError)
            {
                if (ClientMsgHandler.TryParseGatewayError(body, out var err))
                {
                    Fail(ClientErrorText.GatewayErrorText(err));
                }

                return;
            }

            if (module != (byte)ClientModule.Login || sub != (byte)ZoneMsgSub.S2CZoneListRsp)
            {
                if (_state == State.WaitResponse)
                {
                    ClientLogger.Instance.WarnFormat(
                        "ZoneListSession：等待区列表时忽略未知消息 module=0x{0:X2} sub=0x{1:X2}",
                        module, sub);
                }

                return;
            }

            if (!ClientMsgHandler.TryParseZoneListRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            var entries = ClientMsgHandler.ToZoneEntries(rsp);
            var errText = ClientErrorText.ZoneListResultText(rsp.Code, string.Empty);
            if (!string.IsNullOrEmpty(errText))
            {
                ClientLogger.Instance.WarnFormat(
                    "ZoneListSession：区列表失败 code={0} 条目={1}",
                    rsp.Code, entries.Count);
                Fail(errText);
                return;
            }

            if (_activeFetchToken == 0 || _activeFetchToken != _fetchToken)
            {
                return;
            }

            _activeFetchToken = 0;
            _state = State.Idle;
            _tcp.Disconnect();
            ClientLogger.Instance.InfoFormat("ZoneListSession：收到区列表 {0} 条", entries.Count);
            OnSuccess?.Invoke(entries);
        }

        private void Fail(ClientLocalError err) => Fail(ClientErrorText.LocalErrorText(err, LoginTimeoutContext.ZoneList));

        private void Fail(string msg)
        {
            _activeFetchToken = 0;
            _state = State.Idle;
            _tcp.Disconnect();
            ClientLogger.Instance.WarnFormat("ZoneListSession：{0}", msg);
            OnError?.Invoke(msg);
        }
    }
}
