# Net（Unity C#）

对标原 `net/`、`sdk/net/`。引用 `Assets/_Project/Protobuf/`（联接至根 `Protobuf/`）。

| 文件 | 对标 C++ |
|------|----------|
| `MsgHeader.cs` | `Common/NetDefine.h` |
| `PacketCodec.cs` | `sdk/net/ProtocolCodec.h` |
| `ClientModule.cs` | `Common/ClientTypes.h` |
| `GameSession.cs` | `net/GameSession` |

帧格式：`MsgHeader(4B)` + Protobuf body（body **不含** module/sub 前缀）。

`PacketCodec.TryDecode` 使用 `byte[]` + 长度索引拆帧，避免 `List<byte>.ToArray()` 每帧分配；`GameTcpClient` 复用 8KB 读缓冲。

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
