# Net（Unity C#）

对标原 `net/`、`sdk/net/`。引用 `Assets/_Project/Protobuf/`（联接至根 `Protobuf/`）。

| 文件 | 对标 C++ |
|------|----------|
| `MsgHeader.cs` | `Common/NetDefine.h` |
| `PacketCodec.cs` | `sdk/net/ProtocolCodec.h` |
| `ClientModule.cs` | `Common/ClientTypes.h` |

帧格式：`MsgHeader(4B)` + Protobuf body（body **不含** module/sub 前缀）。

更新协议后运行：`..\..\..\..\scripts\sync_protobuf.ps1`
