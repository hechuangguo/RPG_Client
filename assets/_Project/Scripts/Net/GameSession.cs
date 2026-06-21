/// <summary>
/// 游戏内网络会话（对标 net/GameSession）。
/// 职责：Gateway 心跳、移动同步、离世界、实体消息分发。
/// 协作：WorldController、LoginSession、GameTcpClient。
/// 线程：Unity 主线程。
/// </summary>
using System;
using Rpg.Client.Config;
using Rpg.Client.Log;
using Rpg.Client.Util;
using Rpg.Proto.Login;
using Rpg.Proto.MapData;
using Rpg.Proto.System;

namespace Rpg.Client.Net
{
    public sealed class GameSession
    {
        private GameTcpClient _tcp;
        private ClientConfigLoader _config;
        private S2CEnterGame _enterInfo;
        private ulong _localUserId;
        private long _lastHeartbeatMs;
        private long _lastMoveSendMs;
        private uint _heartbeatSeq;
        private bool _inWorld;
        private bool _waitingLogout;
        private LogoutAction _pendingLogoutAction;
        private long _logoutWaitStartMs;

        public Action<S2CSpawnEntity> OnSpawn;
        public Action<S2CMoveNotify> OnMove;
        public Action<S2CDespawnEntity> OnDespawn;
        public Action<string> OnError;
        public Action<LogoutAction> OnLogoutSuccess;
        public Action OnDisconnected;

        public void SetConfig(ClientConfigLoader config) => _config = config;

        public void Start(GameTcpClient tcp, S2CEnterGame enter)
        {
            _tcp = tcp;
            _enterInfo = enter;
            _localUserId = enter.UserId;
            _inWorld = true;
            _waitingLogout = false;
            _lastHeartbeatMs = TimeUtil.NowMs();
            _tcp.SetOnMessage(OnMessage);
            _tcp.SetOnDisconnected(() =>
            {
                ClientLogger.Instance.Warn("GameSession：Gateway 连接断开");
                _inWorld = false;
                OnDisconnected?.Invoke();
            });
            ClientLogger.Instance.InfoFormat("GameSession：进入世界 mapId={0} userId={1}", enter.MapId, enter.UserId);
        }

        public void Update()
        {
            _tcp?.Poll();
            if (!_inWorld || _tcp == null)
            {
                return;
            }

            var now = TimeUtil.NowMs();
            if (now - _lastHeartbeatMs >= (_config?.HeartbeatIntervalMs ?? ClientTimingDefaults.HeartbeatIntervalMs))
            {
                _lastHeartbeatMs = now;
                _tcp.SendRaw(ClientMsgHandler.BuildHeartbeat(_heartbeatSeq++));
            }

            if (_waitingLogout &&
                now - _logoutWaitStartMs > (_config?.LogoutTimeoutMs ?? ClientTimingDefaults.LogoutTimeoutMs))
            {
                ClientLogger.Instance.Warn("GameSession：离世界响应超时");
                OnError?.Invoke(ClientErrorText.LocalErrorText(ClientLocalError.LogoutTimeout));
                _waitingLogout = false;
            }
        }

        public void SendMove(ulong userId, float x, float y, float z, float dir, MoveType moveType)
        {
            if (!_inWorld || _tcp == null)
            {
                return;
            }

            var now = TimeUtil.NowMs();
            if (now - _lastMoveSendMs < (_config?.MoveSendIntervalMs ?? ClientTimingDefaults.MoveSendIntervalMs))
            {
                return;
            }

            _lastMoveSendMs = now;
            _tcp.SendRaw(ClientMsgHandler.BuildMoveReq(userId, x, y, z, dir, moveType));
        }

        public void RequestLogout(LogoutAction action)
        {
            if (_tcp == null)
            {
                return;
            }

            ClientLogger.Instance.InfoFormat("GameSession：请求离世界 action={0}", action);
            _pendingLogoutAction = action;
            _waitingLogout = true;
            _logoutWaitStartMs = TimeUtil.NowMs();
            _tcp.SendRaw(ClientMsgHandler.BuildLogoutReq(action));
        }

        public void LeaveWorld() => _inWorld = false;

        public void Disconnect()
        {
            _inWorld = false;
            _waitingLogout = false;
            _tcp?.Disconnect();
            _tcp = null;
        }

        public GameTcpClient ReleaseTcpClient()
        {
            var tcp = _tcp;
            _tcp = null;
            _inWorld = false;
            return tcp;
        }

        public bool IsConnected => _tcp != null && _tcp.IsConnected;
        public bool IsWaitingLogout => _waitingLogout;
        public ulong LocalUserId => _localUserId;
        public S2CEnterGame EnterGameInfo => _enterInfo;

        private void OnMessage(byte module, byte sub, byte[] body)
        {
            if (module == (byte)ClientModule.System)
            {
                if (sub == (byte)SystemMsgSub.S2CError && ClientMsgHandler.TryParseGatewayError(body, out var err))
                {
                    OnError?.Invoke(ClientErrorText.GatewayErrorText(err));
                }

                return;
            }

            if (module == (byte)ClientModule.Login && sub == (byte)LoginMsgSub.S2CLogoutRsp)
            {
                if (ClientMsgHandler.TryParseLogoutRsp(body, out var rsp) && rsp.Code == (int)LogoutResultCode.LogoutOk)
                {
                    _waitingLogout = false;
                    _inWorld = false;
                    ClientLogger.Instance.Info("GameSession：离世界成功");
                    OnLogoutSuccess?.Invoke(_pendingLogoutAction);
                }
                else
                {
                    _waitingLogout = false;
                    var msg = rsp != null && !string.IsNullOrEmpty(rsp.Msg) ? rsp.Msg : "离世界失败";
                    ClientLogger.Instance.WarnFormat("GameSession：离世界失败 {0}", msg);
                    OnError?.Invoke(msg);
                }

                return;
            }

            if (module != (byte)ClientModule.Scene)
            {
                return;
            }

            switch ((MapDataMsgSub)sub)
            {
                case MapDataMsgSub.S2CSpawnEntity:
                    if (ClientMsgHandler.TryParseSpawnEntity(body, out var spawn))
                    {
                        OnSpawn?.Invoke(spawn);
                    }

                    break;
                case MapDataMsgSub.S2CMoveNotify:
                    if (ClientMsgHandler.TryParseMoveNotify(body, out var move))
                    {
                        OnMove?.Invoke(move);
                    }

                    break;
                case MapDataMsgSub.S2CDespawnEntity:
                    if (ClientMsgHandler.TryParseDespawnEntity(body, out var despawn))
                    {
                        OnDespawn?.Invoke(despawn);
                    }

                    break;
            }
        }
    }
}
