/// <summary>
/// 游戏内网络会话。职责：Gateway 心跳、移动同步、离世界、实体与脚本消息分发。
/// </summary>
using System;
using Rpg.Client.Config;
using Rpg.Client.Log;
using Rpg.Client.Scripting;
using Rpg.Client.Util;
using Rpg.Proto.Bag;
using Rpg.Proto.Chat;
using Rpg.Proto.Client;
using Rpg.Proto.Login;
using Rpg.Proto.MapData;
using Rpg.Proto.Quest;
using Rpg.Proto.System;

namespace Rpg.Client.Net
{
    public sealed class GameSession
    {
        private GameTcpClient _tcp;
        private ClientConfigLoader _config;
        private GameScriptHost _scriptHost;
        private S2CEnterGame _enterInfo;
        private ulong _localUserId;
        private long _lastHeartbeatMs;
        private long _lastMoveSendMs;
        private uint _heartbeatSeq;
        private ulong _serverTimeMs;
        private float _lastSentX;
        private float _lastSentY;
        private float _lastSentZ;
        private float _lastSentDir;
        private bool _hasLastSentPos;
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

        public void ClearHandlers()
        {
            OnSpawn = null;
            OnMove = null;
            OnDespawn = null;
            OnError = null;
            OnLogoutSuccess = null;
            OnDisconnected = null;
        }

        public void SetConfig(ClientConfigLoader config) => _config = config;

        public void SetScriptHost(GameScriptHost host) => _scriptHost = host;

        public void Start(GameTcpClient tcp, S2CEnterGame enter)
        {
            _tcp = tcp;
            _enterInfo = enter;
            _localUserId = enter.UserId;
            _inWorld = true;
            _waitingLogout = false;
            _hasLastSentPos = false;
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
            var heartbeatInterval = _config?.HeartbeatIntervalMs ?? ClientTimingDefaults.HeartbeatIntervalMs;
            if (TimeUtil.ElapsedMs(now, _lastHeartbeatMs) >= heartbeatInterval)
            {
                _lastHeartbeatMs = now;
                _tcp.SendRaw(ClientMsgHandler.BuildHeartbeat(_heartbeatSeq++));
            }

            if (_waitingLogout &&
                TimeUtil.ElapsedMs(now, _logoutWaitStartMs) > (_config?.LogoutTimeoutMs ?? ClientTimingDefaults.LogoutTimeoutMs))
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
            var interval = _config?.MoveSendIntervalMs ?? ClientTimingDefaults.MoveSendIntervalMs;
            if (TimeUtil.ElapsedMs(now, _lastMoveSendMs) < interval)
            {
                return;
            }

            var minDelta = _config?.MoveMinDelta ?? ClientTimingDefaults.MoveMinDelta;
            var minDeltaSq = minDelta * minDelta;
            if (_hasLastSentPos)
            {
                var dx = x - _lastSentX;
                var dy = y - _lastSentY;
                var dz = z - _lastSentZ;
                var distSq = dx * dx + dy * dy + dz * dz;
                var dirChanged = Math.Abs(dir - _lastSentDir) > 0.01f;
                if (distSq < minDeltaSq && !dirChanged)
                {
                    return;
                }
            }

            _lastMoveSendMs = now;
            _lastSentX = x;
            _lastSentY = y;
            _lastSentZ = z;
            _lastSentDir = dir;
            _hasLastSentPos = true;
            _tcp.SendRaw(ClientMsgHandler.BuildMoveReq(userId, x, y, z, dir, moveType));
        }

        public void SendChat(ChatChannel channel, string content)
        {
            if (!_inWorld || _tcp == null)
            {
                return;
            }

            _tcp.SendRaw(ClientMsgHandler.BuildChatReq(channel, content));
        }

        public void SendQuestAccept(uint questId)
        {
            if (!_inWorld || _tcp == null)
            {
                return;
            }

            _tcp.SendRaw(ClientMsgHandler.BuildQuestAcceptReq(questId));
        }

        public void SendQuestSubmit(uint questId)
        {
            if (!_inWorld || _tcp == null)
            {
                return;
            }

            _tcp.SendRaw(ClientMsgHandler.BuildQuestSubmitReq(questId));
        }

        public void RequestBagInfo()
        {
            if (!_inWorld || _tcp == null)
            {
                return;
            }

            _tcp.SendRaw(ClientMsgHandler.BuildBagInfoReq(_localUserId));
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
        public ulong ServerTimeMs => _serverTimeMs;
        public S2CEnterGame EnterGameInfo => _enterInfo;

        private void OnMessage(byte module, byte sub, byte[] body)
        {
            if (module == (byte)ClientModule.System)
            {
                HandleSystemMessage(sub, body);
                return;
            }

            if (module == (byte)ClientModule.Login && sub == (byte)LoginMsgSub.S2CLogoutRsp)
            {
                HandleLogoutRsp(body);
                return;
            }

            if (module == (byte)ClientModule.Scene)
            {
                HandleSceneMessage(sub, body);
                return;
            }

            if (module == (byte)ClientModule.Chat && sub == (byte)ChatMsgSub.S2CChatNotify)
            {
                if (ClientMsgHandler.TryParseChatNotify(body, out var chat))
                {
                    _scriptHost?.OnChat(chat);
                }

                return;
            }

            if (module == (byte)ClientModule.Quest && sub == (byte)QuestMsgSub.S2CQuestInfo)
            {
                if (ClientMsgHandler.TryParseQuestInfo(body, out var info))
                {
                    _scriptHost?.OnQuestInfo(info);
                }

                return;
            }

            if (module == (byte)ClientModule.Bag && sub == (byte)BagMsgSub.S2CBagInfoRsp)
            {
                if (ClientMsgHandler.TryParseBagInfoRsp(body, out var bag))
                {
                    _scriptHost?.OnBagInfo(bag);
                }
            }
        }

        private void HandleSystemMessage(byte sub, byte[] body)
        {
            switch ((SystemMsgSub)sub)
            {
                case SystemMsgSub.S2CError:
                    if (ClientMsgHandler.TryParseGatewayError(body, out var err))
                    {
                        OnError?.Invoke(ClientErrorText.GatewayErrorText(err));
                    }

                    break;
                case SystemMsgSub.S2CKick:
                    ClientLogger.Instance.Warn("GameSession：收到踢线通知");
                    if (ClientMsgHandler.TryParseKick(body, out var kick))
                    {
                        var reason = string.IsNullOrEmpty(kick.Reason) ? "已被服务器踢下线" : kick.Reason;
                        OnError?.Invoke(reason);
                    }
                    else
                    {
                        OnError?.Invoke("已被服务器踢下线");
                    }

                    Disconnect();
                    break;
                case SystemMsgSub.S2CHeartbeat:
                    if (ClientMsgHandler.TryParseHeartbeat(body, out var hb))
                    {
                        _serverTimeMs = hb.ServerTime;
                    }

                    break;
                case SystemMsgSub.S2CNotice:
                    if (ClientMsgHandler.TryParseNotice(body, out var notice))
                    {
                        _scriptHost?.OnNotice(notice.Content);
                    }

                    break;
            }
        }

        private void HandleLogoutRsp(byte[] body)
        {
            if (!ClientMsgHandler.TryParseLogoutRsp(body, out var rsp))
            {
                _waitingLogout = false;
                ClientLogger.Instance.Warn("GameSession：离世界响应解析失败");
                OnError?.Invoke("离世界失败");
                return;
            }

            if (rsp.Code == (int)LogoutResultCode.LogoutOk)
            {
                _waitingLogout = false;
                _inWorld = false;
                ClientLogger.Instance.Info("GameSession：离世界成功");
                OnLogoutSuccess?.Invoke(_pendingLogoutAction);
            }
            else
            {
                _waitingLogout = false;
                var msg = !string.IsNullOrEmpty(rsp.Msg) ? rsp.Msg : "离世界失败";
                ClientLogger.Instance.WarnFormat("GameSession：离世界失败 {0}", msg);
                OnError?.Invoke(msg);
            }
        }

        private void HandleSceneMessage(byte sub, byte[] body)
        {
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
