# Protobuf 生成物（C#）

本目录 **`*.cs` 均为自动生成**，禁止手改。

| 项 | 说明 |
|----|------|
| **来源** | [`Common/`](../Common/) 内 `*Common.proto`、`*Msg.proto` |
| **工具** | `3Party/protoc/bin/protoc.exe`（见 [`3Party/README.md`](../3Party/README.md)） |
| **生成** | `.\scripts\sync_protobuf.ps1` |
| **Unity** | `assets/_Project/Protobuf/` 目录联接 → 本目录 |

## 更新流程

```powershell
.\sync_all.bat
# 离线：.\sync_all.bat -Offline
```

## 命名空间

各 `.proto` 的 `option csharp_namespace`，例如 `Rpg.Proto.Login`、`Rpg.Proto.Zone`。

## 路由

6 字节 **`MsgHeader`**（见 [`assets/_Project/Scripts/Net/MsgHeader.cs`](../assets/_Project/Scripts/Net/MsgHeader.cs) 与 [`Common/WireCommon.proto`](../Common/WireCommon.proto)）+ Protobuf body。
