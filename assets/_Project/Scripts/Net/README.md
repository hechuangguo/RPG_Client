# Net（Unity C#）

引用 `Assets/_Project/Protobuf/`（联接至根 `Protobuf/`）。

帧格式：`MsgHeader(4B)` + Protobuf body（body **不含** module/sub 前缀）。

`PacketCodec.TryDecode` 使用 `byte[]` + 长度索引拆帧，避免每帧额外分配；`GameTcpClient` 复用 8KB 读缓冲。

## 登录五步流程

| 步骤 | 会话 | 关键消息 | code 校验 |
|------|------|----------|-----------|
| 1 区列表 | `ZoneListSession` | `C2SZoneListReq` → `S2CZoneListRsp` | `S2CZoneListRsp.code == 0` |
| 2 注册 | `LoginSession` | `C2SRegisterReq` → `S2CRegisterRsp` | `RegisterResultCode` |
| 3 登录 | `LoginSession` | `C2SLoginReq` → `S2CLoginRsp` + `S2CGatewayInfo` → `C2SGatewayAuthReq` | 登录与 Gateway 鉴权均解析 `S2CLoginRsp.code` |
| 4 创角 | `LoginSession` | `C2SCreateUserReq` → `S2CCreateUserRsp` | `CreateCharacterResultCode` |
| 5 进世界 | `LoginSession` → `GameSession` | `C2SSelectUserReq` → `S2CEnterGame`；TCP 移交后心跳/Scene | `userId/mapId` 非零；`S2CUserList.code` 在选角前校验 |

**Gateway 鉴权**：连接 Gateway 后服务端可能再次下发 `S2CLoginRsp`；`code != 0` 须立即 `Fail` 展示登录失败文案，不可静默等待角色列表超时。

**返回选角**：`ResumeGatewayForCharSelect` 等待 `S2CUserList`；超时且 TCP 仍连接时会重发一次 `C2SGatewayAuthReq`。

客户端输入校验见 `Util/ClientInputValidator`（区服必选、账号/密码、角色名 2–12 码点）。

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

更新协议后运行：`..\..\..\..\scripts\sync_protobuf.ps1`
