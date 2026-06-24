/// <summary>
/// 游戏内网络会话。职责：Gateway 心跳、移动同步、离世界、实体与脚本消息分发。
/// OnMessage 使用字典 O(1) 分发替代 if 链，支持运行时动态扩展。
/// </summary>
using System;
using System.Collections.Generic;
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
    public sealed class GameSession : ISession
    {
        private GameTcpClient _tcp;
        private ClientConfigLoader _config;
        private GameScriptHost _scriptHost;
        private S2CEnterGame _enterInfo;
        private ulong _localUserId;
        private long _lastHeartbeatMs;
        private long _lastMoveSendMs;
        private uint _heartbeatSeq = 1;
        private ulong _serverTimeMs;

        // RTT 追踪
        private readonly Dictionary<uint, long> _heartbeatSendTimes = new Dictionary<uint, long>();
        private double _smoothRttMs;
        private const double RttSmoothingAlpha = 0.125; // 指数移动平均平滑系数

        private float _lastSentX;
        private float _lastSentY;
        private float _lastSentZ;
        private float _lastSentDir;
        private bool _hasLastSentPos;
        private bool _inWorld;
        private bool _waitingLogout;
        private LogoutAction _pendingLogoutAction;
        private long _logoutWaitStartMs;

        /// <summary>模块+子命令 → 处理函数的映射表，O(1) 分发。</summary>
        private readonly Dictionary<(byte module, byte sub), Action<byte[]>> _msgHandlers
            = new Dictionary<(byte, byte), Action<byte[]>>();

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
            InitHandlers();
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
                var seq = _heartbeatSeq++;
                _heartbeatSendTimes[seq] = now;
                _tcp.SendRaw(ClientMsgHandler.BuildHeartbeat(seq));
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

        public void Cancel() => Disconnect();

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
        /// <summary>平滑 RTT（毫秒）。首次心跳后有效，未测量过返回 -1。</summary>
        public double SmoothRttMs => _smoothRttMs > 0 ? _smoothRttMs : -1;
        public S2CEnterGame EnterGameInfo => _enterInfo;

        /// <summary>注册消息分发表。调用一次后不可更改；新增模块应在此处注册。</summary>
        private void InitHandlers()
        {
            _msgHandlers.Clear();

            // System 模块 — 多子命令，单独处理
            RegisterHandler(ClientModule.System, (byte)SystemMsgSub.S2CError,
                body => { if (ClientMsgHandler.TryParseGatewayError(body, out var err)) OnError?.Invoke(ClientErrorText.GatewayErrorText(err)); });
            RegisterHandler(ClientModule.System, (byte)SystemMsgSub.S2CKick, body =>
            {
                ClientLogger.Instance.Warn("GameSession：收到踢线通知");
                string reason;
                if (ClientMsgHandler.TryParseKick(body, out var kick))
                    reason = string.IsNullOrEmpty(kick.Reason) ? "已被服务器踢下线" : kick.Reason;
                else
                    reason = "已被服务器踢下线";
                OnError?.Invoke(reason);
                Disconnect();
            });
            RegisterHandler(ClientModule.System, (byte)SystemMsgSub.S2CHeartbeat,
                body =>
                {
                    if (ClientMsgHandler.TryParseHeartbeat(body, out var hb))
                    {
                        _serverTimeMs = hb.ServerTime;
                        // 计算 RTT：echo seq 反查发送时间
                        if (_heartbeatSendTimes.TryGetValue(hb.Seq, out var sendMs))
                        {
                            var rtt = TimeUtil.NowMs() - sendMs;
                            if (_smoothRttMs <= 0)
                                _smoothRttMs = rtt;
                            else
                                _smoothRttMs = _smoothRttMs * (1.0 - RttSmoothingAlpha) + rtt * RttSmoothingAlpha;
                            _heartbeatSendTimes.Remove(hb.Seq);
                        }

                        // 清理过旧的条目（心跳超时未回复）
                        if (_heartbeatSendTimes.Count > 16)
                        {
                            var hbInterval = _config?.HeartbeatIntervalMs ?? ClientTimingDefaults.HeartbeatIntervalMs;
                            var cutoff = TimeUtil.NowMs() - hbInterval * 4;
                            var stale = new List<uint>();
                            foreach (var kv in _heartbeatSendTimes)
                                if (kv.Value < cutoff)
                                    stale.Add(kv.Key);
                            foreach (var key in stale)
                                _heartbeatSendTimes.Remove(key);
                        }
                    }
                });
            RegisterHandler(ClientModule.System, (byte)SystemMsgSub.S2CNotice,
                body => { if (ClientMsgHandler.TryParseNotice(body, out var notice)) _scriptHost?.OnNotice(notice.Content); });

            // Login 模块 — 仅 LogoutRsp
            RegisterHandler(ClientModule.Login, (byte)LoginMsgSub.S2CLogoutRsp,
                body => HandleLogoutRsp(body));

            // Scene 模块 — 多子命令
            RegisterHandler(ClientModule.Scene, (byte)MapDataMsgSub.S2CSpawnEntity,
                body => { if (ClientMsgHandler.TryParseSpawnEntity(body, out var s)) OnSpawn?.Invoke(s); });
            RegisterHandler(ClientModule.Scene, (byte)MapDataMsgSub.S2CMoveNotify,
                body => { if (ClientMsgHandler.TryParseMoveNotify(body, out var m)) OnMove?.Invoke(m); });
            RegisterHandler(ClientModule.Scene, (byte)MapDataMsgSub.S2CDespawnEntity,
                body => { if (ClientMsgHandler.TryParseDespawnEntity(body, out var d)) OnDespawn?.Invoke(d); });

            // Chat / Quest / Bag — 各一个子命令
            RegisterHandler(ClientModule.Chat, (byte)ChatMsgSub.S2CChatNotify,
                body => { if (ClientMsgHandler.TryParseChatNotify(body, out var c)) _scriptHost?.OnChat(c); });
            RegisterHandler(ClientModule.Quest, (byte)QuestMsgSub.S2CQuestInfo,
                body => { if (ClientMsgHandler.TryParseQuestInfo(body, out var q)) _scriptHost?.OnQuestInfo(q); });
            RegisterHandler(ClientModule.Bag, (byte)BagMsgSub.S2CBagInfoRsp,
                body => { if (ClientMsgHandler.TryParseBagInfoRsp(body, out var b)) _scriptHost?.OnBagInfo(b); });
        }

        private void RegisterHandler(ClientModule module, byte sub, Action<byte[]> handler)
        {
            _msgHandlers[((byte)module, sub)] = handler;
        }

        private void OnMessage(byte module, byte sub, byte[] body)
        {
            if (_msgHandlers.TryGetValue((module, sub), out var handler))
            {
                handler(body);
                return;
            }

            ClientLogger.Instance.WarnFormat(
                "GameSession：未处理消息 module=0x{0:X2} sub=0x{1:X2}", module, sub);
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
    }
}
