# 3Party — Unity 客户端构建依赖

所有**构建期**第三方库集中在此目录，支持离线同步与编译。

| 组件 | 版本 | 路径 | 说明 |
|------|------|------|------|
| protoc | 25.3 | `protoc/protoc.zip` → `protoc/bin/protoc.exe` | 从 Common 生成 `Protobuf/*.cs` |
| Google.Protobuf | 3.25.3 | `google.protobuf/*.nupkg` → `lib/.../Google.Protobuf.dll` | 复制到 `assets/_Project/Plugins/` |

Unity UPM 包（URP、Addressables 等）由 Tuanjie Hub 解析，不在此目录。

## 在线首次

```powershell
.\3Party\fetch_3party.ps1
# 或
.\sync_all.bat
```

会下载缺失的 `protoc.zip` / `Google.Protobuf.*.nupkg` 并解压。

## 离线

1. 在一台联网机器执行 `.\3Party\fetch_3party.ps1`
2. 确保仓库含 `3Party/protoc/protoc.zip` 与 `3Party/google.protobuf/*.nupkg`（推荐提交到 Git）
3. 离线机器：

```powershell
.\sync_all.bat -Offline
```

缺 bundle 时会报错，不会静默联网下载。

## 版本与脚本

- 统一入口：[`fetch_3party.ps1`](fetch_3party.ps1)
- 协议生成：[`../scripts/sync_protobuf.ps1`](../scripts/sync_protobuf.ps1)
- 插件复制：[`../scripts/fetch_google_protobuf.ps1`](../scripts/fetch_google_protobuf.ps1)（调用 fetch_3party）
