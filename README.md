# RPG_Client

Windows PC MMORPG **Unity 3D 客户端（C#）**，协议为 **Protobuf + 4 字节 MsgHeader**。

**团结引擎 1.6.11**（Editor **2022.3.61t12**）。须在 Hub 安装该版本；打开前可运行 `.\scripts\align_tuanjie_editor.ps1` 校验。

Hub 打开本仓**根目录**（非子目录）。

## 目录结构

```
RPG_Client/
  Common/              # Submodule → RPG_Common（*.proto 只读真源）
  Protobuf/            # protoc 生成 *.cs
  3Party/              # protoc、Google.Protobuf（离线 bundle）
  assets/_Project/     # Unity 脚本、场景、Editor
  assets/StreamingAssets/  # config/script/database（sync_streaming_assets.ps1）
  ProjectSettings/ Packages/
  config/ script/ database/ map/
  scripts/             # sync_*.ps1、build_unity_client.ps1
  docs/
```

## 首次设置（在线）

```powershell
git clone --recurse-submodules https://github.com/hechuangguo/RPG_Client.git
cd RPG_Client
.\sync_all.bat
```

或分步：

```powershell
git submodule update --init Common
.\3Party\fetch_3party.ps1
.\scripts\sync_protobuf.ps1
.\scripts\sync_streaming_assets.ps1
```

Hub 打开工程 → 等待 Package Manager 解析 URP / Addressables / InputSystem（勿 Upgrade 无关包）。

菜单 **RPG → Setup Boot Scene**（或 `.\scripts\setup_boot_scene.ps1`）生成 Boot 场景（含区服列表 ScrollView UI）。

## 区服选择

点击「选择区服」→ 拉取区列表 → 在列表中选中区服并点「确认」。维护中区不可选；上次所选区会预高亮。

## 离线开发

1. 联网机器执行一次 `.\3Party\fetch_3party.ps1`，并提交 `3Party/protoc/protoc.zip`、`3Party/google.protobuf/*.nupkg`
2. 离线机器：

```powershell
.\sync_all.bat -Offline
```

3. Hub 打开工程（需已安装 1.6.11 且 `Library/` 曾成功解析过 UPM 包）

详见 [`3Party/README.md`](3Party/README.md)。

## Package Manager EBUSY

1. 错误框点 **退出**（勿点 **继续** / **重试**）
2. 关闭 Tuanjie Editor（无 `Unity.exe` / `Tuanjie.exe`）
3. `.\scripts\clean_unity_library.ps1 -Full`（必要时 `-ForceKill -KillHub`）
4. 用 **1.6.11** 单实例重开工程

## 构建（Windows）

```powershell
.\scripts\build_unity_client.ps1
# 输出：build/unity/bin/RPGClient.exe
```

## 共享协议（Common Submodule）

[`Common/`](Common/) 指向 [RPG_Common](https://github.com/hechuangguo/RPG_Common)。**客户端不修改** `Common/*.proto`；变更在服务端仓库评审后：

```powershell
.\sync_all.bat
# 或 -Offline 仅本地
git add Common Protobuf
git commit -m "chore: bump Common and sync Protobuf"
```

| 脚本 | 说明 |
|------|------|
| [`scripts/sync_all.ps1`](scripts/sync_all.ps1) | 完整同步（git + 3Party + Protobuf + StreamingAssets） |
| [`scripts/sync_protobuf.ps1`](scripts/sync_protobuf.ps1) | `Common/*.proto` → `Protobuf/*.cs` |
| [`scripts/commit_push_all.ps1`](scripts/commit_push_all.ps1) | 默认只提交主仓；Common 推送需 `-AllowCommonEdit` |

## 联调

- 配置：`assets/StreamingAssets/config/client_config.xml`（模板 `config/client_config.xml.example`）
- 日志：运行目录向上查找 `logs/client_YYYYMMDD.log`
- 注释规范：[`.cursor/rules/code-comments.mdc`](.cursor/rules/code-comments.mdc)；日志中文：[`.cursor/rules/log-language.mdc`](.cursor/rules/log-language.mdc)

## 文档

- [`docs/SCOPE.md`](docs/SCOPE.md) — 实施范围
- [`docs/LUA_BRIDGE.md`](docs/LUA_BRIDGE.md) — Lua 桥接（Phase 3）
- [`Protobuf/README.md`](Protobuf/README.md) — 生成物说明
