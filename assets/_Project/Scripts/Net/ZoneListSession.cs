/// <summary>
/// 区列表短会话（对标 net/ZoneListSession）。
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
            _state = State.Connecting;
            _waitStartMs = TimeUtil.NowMs();
            if (!_tcp.Connect(_config.LoginHost, _config.LoginPort, _config.Tls))
            {
                Fail(ClientLocalError.ConnectTimeout);
            }
        }

        public void Update()
        {
            _tcp.Poll();
            if (_state == State.WaitResponse &&
                TimeUtil.NowMs() - _waitStartMs > (_config?.ZoneListResponseTimeoutMs ?? ClientTimingDefaults.ZoneListResponseTimeoutMs))
            {
                Fail(ClientLocalError.ZoneListTimeout);
            }
        }

        public void Cancel()
        {
            _state = State.Idle;
            _tcp.Disconnect();
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
                return;
            }

            if (!ClientMsgHandler.TryParseZoneListRsp(body, out var rsp))
            {
                Fail(ClientLocalError.ParseError);
                return;
            }

            _state = State.Idle;
            _tcp.Disconnect();
            var entries = ClientMsgHandler.ToZoneEntries(rsp);
            ClientLogger.Instance.InfoFormat("ZoneListSession：收到区列表 {0} 条", entries.Count);
            OnSuccess?.Invoke(entries);
        }

        private void Fail(ClientLocalError err) => Fail(ClientErrorText.LocalErrorText(err, LoginTimeoutContext.ZoneList));

        private void Fail(string msg)
        {
            _state = State.Idle;
            _tcp.Disconnect();
            ClientLogger.Instance.WarnFormat("ZoneListSession：{0}", msg);
            OnError?.Invoke(msg);
        }
    }
}
