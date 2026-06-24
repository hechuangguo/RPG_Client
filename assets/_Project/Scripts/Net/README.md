# Net（Unity C#）

引用 `Assets/_Project/Protobuf/`（联接至根 `Protobuf/`）。

## 协议约定

线上帧格式（LoginServer 与 Gateway **同一套**）：

```text
┌────────────────────────────────────────┐
│ MsgHeader (4B, little-endian packed)   │
│   bodyLen : uint16  — Protobuf 字节数   │
│   module  : uint8   — ClientModule      │
│   sub     : uint8   — 域内 *MsgSub      │
├────────────────────────────────────────┤
│ Body: protobuf serialized message      │
│   （不含 body 内 module/sub 前缀）       │
└────────────────────────────────────────┘
```

- 编解码入口：[`PacketCodec.cs`](PacketCodec.cs)（`Encode` / `TryDecode`）、[`ClientMsgHandler.cs`](ClientMsgHandler.cs)（Build / TryParse）
- 解析 helper：[`ProtoParse.cs`](ProtoParse.cs)（失败记 WARN，不抛异常）
- **禁止**手写定长 struct body 或旧 `LoginMsg.h` 风格字段布局

**ProtocolVersion**：以下 C→S 首包须携带 `rpg.client.ProtocolVersion`（当前 major=1 **minor=1**，见 [`WireConstants.cs`](WireConstants.cs)）：

| 消息 | proto |
|------|-------|
| `C2SLoginReq` | `LoginMsg.proto` |
| `C2SRegisterReq` | `LoginMsg.proto` |
| `C2SGatewayAuthReq` | `LoginMsg.proto` |

登录密码 nonce 流程见 [`docs/SECURITY.md`](../../../../docs/SECURITY.md)。

## GameTcpClient

- **连接**：阻塞 `TcpClient.Connect` / TLS 握手在后台线程 `GameTcpClient.Connect` 执行；状态变更仅经 `_mainThreadQueue` 回主线程（IL2CPP 友好）。
- **接收**：初始 8KB 缓冲，单连接最大 `65539` 字节；解码后空缓冲收缩回 8KB；非法 `bodyLen` 断连。
- **发送**：`SendRaw` 入队；`Poll` 每帧最多刷 `maxFlushBytesPerPoll` 字节（默认 16384），避免主线程长时间阻塞。
- **回调**：`Dispose` 时 `ClearCallbacks`；`Disconnect` 保留回调供会话复用同一客户端实例。

## 已实现消息清单

| 域 | Module | C→S | S→C | proto 文件 |
|----|--------|-----|-----|-----------|
| Login | 0x00 | `C2SLoginReq` | `S2CLoginRsp` | `LoginMsg.proto` |
| Login | 0x00 | — | `S2CLoginChallenge` | `LoginMsg.proto` |
| Login | 0x00 | `C2SRegisterReq` | `S2CRegisterRsp` | `LoginMsg.proto` |
| Login | 0x00 | `C2SGatewayAuthReq` | — | `LoginMsg.proto` |
| Login | 0x00 | `C2SSelectUserReq` | `S2CEnterGame` | `LoginMsg.proto` |
| Login | 0x00 | `C2SCreateUserReq` | `S2CCreateUserRsp` | `LoginMsg.proto` |
| Login | 0x00 | `C2SLogoutReq` | `S2CLogoutRsp` | `LoginMsg.proto` |
| Login | 0x00 | — | `S2CGatewayInfo` | `LoginMsg.proto` |
| Login | 0x00 | — | `S2CUserList` | `LoginMsg.proto` |
| Zone | 0x00 | `C2SZoneListReq` | `S2CZoneListRsp` | `ZoneMsg.proto` |
| Scene | 0x01 | `C2SMoveReq` | `S2CSpawnEntity` / `S2CMoveNotify` / `S2CDespawnEntity` | `MapDataMsg.proto` |
| Chat | 0x05 | `C2SChatReq` | `S2CChatNotify` | `ChatMsg.proto` |
| Quest | 0x07 | `C2SQuestAcceptReq` / `C2SQuestSubmitReq` | `S2CQuestInfo` | `QuestMsg.proto` |
| Bag | 0x03 | `C2SBagInfoReq` | `S2CBagInfoRsp` | `BagMsg.proto` |
| System | 0x0F | `C2SHeartbeat` | `S2CHeartbeat` / `S2CError` / `S2CKick` / `S2CNotice` | `SystemMsg.proto` |
| NPC | 0x08 | — | — | `NpcMsg.proto`（**未实现**） |

## 登录五步流程

| 步骤 | 会话 | 关键消息 | code 校验 |
|------|------|----------|-----------|
| 1 区列表 | `ZoneListSession` | `S2CLoginChallenge` → `C2SZoneListReq` → `S2CZoneListRsp` | `S2CZoneListRsp.code == 0` |
| 2 注册 | `LoginSession` | `S2CLoginChallenge` → `C2SRegisterReq` → `S2CRegisterRsp` | `RegisterResultCode` |
| 3 登录 | `LoginSession` | `S2CLoginChallenge` → `C2SLoginReq` → `S2CLoginRsp` + `S2CGatewayInfo` → `C2SGatewayAuthReq` | 登录与 Gateway 鉴权均解析 `S2CLoginRsp.code` |
| 4 创角 | `LoginSession` | `C2SCreateUserReq` → `S2CCreateUserRsp` | `CreateCharacterResultCode` |
| 5 进世界 | `LoginSession` → `GameSession` | `C2SSelectUserReq` → `S2CEnterGame`；TCP 移交后心跳/Scene | `userId/mapId` 非零；`S2CUserList.code` 在选角前校验 |

**LoginServer 挑战**：连接 LoginServer 后须先收 `S2CLoginChallenge` 并回显 `login_nonce`；`password_digest` 仍为 `SHA-256(UTF-8 密码)`（见 [`docs/SECURITY.md`](../../../../docs/SECURITY.md)）。

**Gateway 心跳**：`LoginSession` 在 Gateway 已连接阶段（选角/创角/进世界前）与 `GameSession` 一样周期性发送 `C2SHeartbeat`。

**Gateway 鉴权**：连接 Gateway 后服务端可能再次下发 `S2CLoginRsp`；`code != 0` 须立即 `Fail` 展示登录失败文案，不可静默等待角色列表超时。

**返回选角**：`LogoutAction.ReturnCharSelect` 后 Gateway 账号会话保持，**不**重发 `C2SGatewayAuthReq`；客户端先用缓存角色列表恢复选角 UI，并等待服务端推送 `S2CUserList` 刷新。仅首次连接 Gateway 或鉴权未完成时才发送 `C2SGatewayAuthReq`。

客户端输入校验见 `Util/ClientInputValidator`（区服必选、账号/密码、角色名 2–12 码点）。

## 连接生命周期

| 阶段 | TCP | 说明 |
|------|-----|------|
| 区列表 | LoginServer 短连接 | 拉完即断；**不复用** |
| 登录 | LoginServer → Gateway | 换 host 须断开重连；**设计使然** |
| 进世界 | Gateway 移交 | `LoginSession.TakeTcpClient` → `GameSession` |
| 返回选角 | Gateway 复用 | `ResumeGatewayForCharSelect` |
| 返回登录 | Gateway 断开 | `LogoutAction.ReturnLogin` |

## GameSession 入站分发

| Module | Sub | 消息 | 下游 |
|--------|-----|------|------|
| System | S2CError | 网关错误 | `OnError` |
| System | S2CKick | 踢线 | `OnError` + `Disconnect` |
| System | S2CHeartbeat | 心跳 | `_serverTimeMs` |
| System | S2CNotice | 公告 | `GameScriptHost.OnNotice` |
| Login | S2CLogoutRsp | 离世界 | `OnLogoutSuccess` |
| Scene | Spawn/Move/Despawn | 实体 | `WorldController` |
| Chat | S2CChatNotify | 聊天 | `GameScriptHost.OnChat` |
| Quest | S2CQuestInfo | 任务 | `GameScriptHost.OnQuestInfo` |
| Bag | S2CBagInfoRsp | 背包 | `GameScriptHost.OnBagInfo` |

**移动同步**：`SendMove` 受 `moveSendIntervalMs` 与 `moveMinDelta` 双重约束，减少无效位移包。

更新协议后运行：`..\..\..\..\scripts\sync_protobuf.ps1`
