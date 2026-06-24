# RPG_Client

Windows PC MMORPG **Unity 3D 客户端（C#）**，协议为 **Protobuf + 4 字节 MsgHeader**。

**团结引擎 1.6.11**（Editor **2022.3.61t12**）。须在 Hub 安装该版本；打开前可运行 `.\scripts\align_tuanjie_editor.ps1` 校验。

Hub 打开本仓**根目录**（非子目录）。

## 目录结构

```
RPG_Client/
  Common/              # Submodule → RPG_Common（*.proto 只读真源）
  Protobuf/            # protoc 生成 *.cs（gitignore，clone 后 sync_protobuf.ps1）
  3Party/              # protoc、Google.Protobuf（离线 bundle）
  assets/_Project/     # Unity 脚本、场景、Editor
  assets/StreamingAssets/  # config/script/database（sync_streaming_assets.ps1）
  ProjectSettings/ Packages/
  config/ script/ database/ map/   # script/ 为 Lua 真源；StreamingAssets 由 sync 脚本生成
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

**首次构建检查清单**：`Common` submodule 已初始化 → `sync_protobuf.ps1` → `sync_streaming_assets.ps1` → 复制 `config/client_config.xml.example` 为 `client_config.xml` 并按本机修改。

菜单 **RPG → Setup Boot Scene**（或 `.\scripts\setup_boot_scene.ps1`）生成 Boot 场景（含区服列表 ScrollView UI）。

## 区服选择与登录流程

1. **区服首页** →「选择区服」→ `ZoneListSession` 拉取列表 → 选中并「确认」（须 `enabled` 且非维护）
2. **进入游戏** → 登录/注册（须已选区；账号 4–32 字符、密码 6–32 字符）
3. **LoginServer** 登录成功 → 连接 **Gateway** 鉴权 → 角色列表
4. **选角** → 点击角色行 →「进入世界」；无角色时先创角（名称 2–12 字符，可选职业/性别）
5. **进世界** → `GameSession` 接管 TCP，WASD 移动；ESC 可返回选角/登录

客户端会在发送前校验区服与表单；Gateway 鉴权失败会立即显示登录错误，而非等待角色列表超时。详见 [`assets/_Project/Scripts/Net/README.md`](assets/_Project/Scripts/Net/README.md)。

## 区服选择（简要）

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
git add Common
git commit -m "chore: bump Common submodule"
```

**客户端仅使用 Protobuf wire**：线上帧为 4 字节 `MsgHeader` + Protobuf body；**不再使用**旧定长 struct（如已归档的 `LoginMsg.h`）。更新协议：`git submodule update` → `.\scripts\sync_protobuf.ps1` → 在 `ClientMsgHandler` 增加 Build/TryParse（见 [`Protobuf/README.md`](Protobuf/README.md) checklist）。

`Protobuf/` 为本地生成目录，**不提交**；clone 后须运行 `.\scripts\sync_protobuf.ps1`（或 `sync_all.bat`）。

| 脚本 | 说明 |
|------|------|
| [`scripts/sync_all.ps1`](scripts/sync_all.ps1) | 完整同步（git + 3Party + Protobuf + StreamingAssets） |
| [`scripts/sync_protobuf.ps1`](scripts/sync_protobuf.ps1) | `Common/*.proto` → `Protobuf/*.cs` |
| [`scripts/commit_push_all.ps1`](scripts/commit_push_all.ps1) | 默认只提交主仓；Common 推送需 `-AllowCommonEdit` |

## 联调

- 配置：[`docs/CONFIG.md`](docs/CONFIG.md)（`client_config.xml` 模板 `config/client_config.xml.example`）
- 安全/登录 nonce：[`docs/SECURITY.md`](docs/SECURITY.md)
- 日志：运行目录向上查找 `logs/client_YYYYMMDD.log`（缓冲写入，Err 立即刷盘）
- 注释规范：[`.cursor/rules/code-comments.mdc`](.cursor/rules/code-comments.mdc)；日志中文：[`.cursor/rules/log-language.mdc`](.cursor/rules/log-language.mdc)

## 文档

- [`docs/SCOPE.md`](docs/SCOPE.md) — 实施范围
- [`docs/CONFIG.md`](docs/CONFIG.md) — 配置格式（XML/JSON/Lua）
- [`docs/SECURITY.md`](docs/SECURITY.md) — 登录 nonce 与服务端对接
- [`docs/LUA_BRIDGE.md`](docs/LUA_BRIDGE.md) — Lua 桥接（Phase 3）
- [`docs/UI_PROMPT_WORKFLOW.md`](docs/UI_PROMPT_WORKFLOW.md) — UI 出图提示词工作流（`/ui-prompt-generator`）
- `scripts/sync_protobuf.ps1` — 从 Common 生成 `Protobuf/*.cs`（本地，不提交）
