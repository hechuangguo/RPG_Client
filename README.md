# RPG_Client

Windows PC MMORPG 2D client (C++17 + SFML). Runs as a **GUI application** (no console window).

## 目录结构

```
RPG_Client/
  Common/      # Git Submodule → RPG_Common（ClientMsg.h 等 wire 协议）
  3Party/      # SFML、Lua 等第三方库
  scripts/     # init_common.ps1、sync_common.ps1
  sdk/         # 底层封装（net/log/time/math/util）
  app/ game/ lua/ net/ ui/ util/   # 应用层
  config/ script/ database/ basefile/ map/ assets/
  assets/fonts/   # NotoSansSC-Regular.otf（fetch_font.ps1 下载）
  main.cpp
  CMakeLists.txt
  build_client.ps1
```

## 克隆（含共享协议）

`Common/` 为 [RPG_Common](https://github.com/hechuangguo/RPG_Common) 的 **Git Submodule**，首次克隆须带 submodule：

```powershell
git clone --recurse-submodules https://github.com/hechuangguo/RPG_Client.git
```

已克隆但未拉取 Common 时：

```powershell
git submodule update --init --recursive
# 或
.\scripts\init_common.ps1
```

## Build

Prerequisites: Visual Studio 2022/2026 with **Desktop development with C++** workload, CMake, Ninja.

首次构建前先克隆 submodule 并准备第三方库与中文字体：

```powershell
git submodule update --init --recursive
.\3Party\download_and_build.ps1
.\assets\fonts\fetch_font.ps1
.\build_client.ps1
```

`build_client.ps1` 会在 Common 缺失时尝试 `git submodule update --init Common`，并自动调用 `fetch_font.ps1`。

Or manually (Developer Command Prompt for VS):

```powershell
.\assets\fonts\fetch_font.ps1
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output: `build/bin/RPGClient.exe`

If `cl.exe` is missing, open Visual Studio Installer → Modify → check **Desktop development with C++**.

### Visual Studio (Debug / Release)

本机为 **Visual Studio 2026** 时，须使用 **Visual Studio 18 2026** 生成器（勿选 VS 2022 / v143，未安装会报 MSB8020）。

- **推荐日常编译：** `.\build_client.ps1`（Ninja + Release，不依赖 VS IDE）。
- **在 VS 中打开：** 打开仓库根目录 `RPG_Client`（不是已删除的 `Client\` 子目录）。
- **配置选择：** 工具栏选 **x64-Debug** 或 **x64-Release**（不要选 x86-Debug）。
- **首次或迁移后：** 菜单 **项目 → 删除缓存并重新配置**；或删除 `.vs` 与 `out` 后重开 VS。
- **CMake 配置：** 根目录 [`CMakeSettings.json`](CMakeSettings.json) 已指定 VS 2026 生成器。
- **Debug 输出：** `out/build/x64-Debug/bin/RPGClient.exe`（与 F5 调试路径一致）。
- **Release（脚本）：** `build/bin/RPGClient.exe`（Ninja 单配置）。
- **Debug vs Release：** Debug 链接 `sfml-*-d` 并复制 `sfml-*-d-2.dll`；勿混用 Release DLL。
- **Fonts：** F5 前运行 `.\assets\fonts\fetch_font.ps1`，确保 `out/build/x64-Debug/bin/assets/fonts/` 有字体。
- **No console:** F5 仅启动 SFML 窗口；日志写在仓库根目录 `logs/client_YYYYMMDD.log`（与 `main.cpp` 同级，非 exe 旁）。

## Run

Start Server (external `RPG_Server`), then double-click `RPGClient.exe` from `build/bin/`.
Ensure `config/`, `script/`, `database/`, `basefile/`, `map/`, `assets/` are beside the exe (POST_BUILD copies them).

## 登录流程

1. **选区首页**：显示「当前游戏区：xxx」（未选则为「未选择」），按钮「选择服务器」「进入游戏」（未选区时后者禁用）。
2. **区列表**：点击「选择服务器」后连接 LoginServer（`client_config.xml` 的 `loginHost`/`loginPort`），发送 `C2S_ZONE_LIST_REQ` 拉取区列表；**不读本地 serverlist.xml**（区列表由 LoginServer 从 `serverlist.xml` 加载后下发）。选中可用区后点「确定」返回首页。
3. **区服状态**：列表右侧显示负载状态（畅通 / 繁忙 / 爆满 / 维护中）及在线人数（服务端 v2 协议下发时）。旧版 LoginServer 仅下发 `enabled` 时，可用区显示「畅通」，维护区显示「维护中」。
4. **加载资源**：点击「进入游戏」后初始化 Lua 等，进入账号登录界面。
5. **账号登录/注册**：输入账号密码登录，或注册后返回登录页（账号密码自动填入）。
6. **Gateway 鉴权**：LoginServer 验证通过后下发 `S2C_GATEWAY_INFO` 与 `loginToken`；客户端连接 Gateway 并发送 `C2S_GATEWAY_AUTH_REQ`（无票据时回退 `C2S_LOGIN_REQ` 兼容旧 Gateway）。
7. **角色列表**：Gateway 返回 `S2C_USER_LIST`；客户端展示角色名、等级、职业、性别，并标注「上次登录」角色。
8. **选角/创角**：选择角色点「进入游戏」发送 `C2S_SELECT_USER_REQ`；无角色时可「创建角色」（`C2S_CREATE_USER_REQ`）。职业/性别 wire 值：`0=战士/男, 1=法师/女`（须与 Server 一致）。
9. **进入场景**：收到 `S2C_ENTER_GAME`（含 mapID 与出生坐标）后加载地图并进入游戏世界。

**旧 Gateway 兼容**：若 Gateway 未下发角色列表而直接返回 `S2C_LOGIN_RSP` + `S2C_ENTER_GAME`，客户端跳过选角界面直接进入场景。

输入框聚焦时显示**闪烁光标**。

## Config

客户端配置使用 XML，支持注释，便于多人各自维护本机 `loginHost` 等差异。

| 文件 | 版本库 | 说明 |
|------|--------|------|
| `config/client_config.xml.example` | 提交 | 配置模板 |
| `config/client_config.xml` | **不入库**（gitignore） | 每人本地复制并修改 |

**首次配置：**

```powershell
.\scripts\setup_config.ps1
# 或手动：Copy-Item config/client_config.xml.example config/client_config.xml
# 编辑 loginHost / loginPort 为本机可访问的 LoginServer
```

示例（`client_config.xml`）：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ClientConfig>
    <windowWidth>1280</windowWidth>
    <windowHeight>720</windowHeight>
    <logLevel>info</logLevel>
    <logToConsole>false</logToConsole>
    <loginHost>127.0.0.1</loginHost>
    <loginPort>9010</loginPort>
</ClientConfig>
```

| 字段 | 默认值 | 说明 |
|------|--------|------|
| `loginHost` | `127.0.0.1` | LoginServer IP |
| `loginPort` | `9010` | LoginServer ClientListen 端口 |
| `logToConsole` | `false` | GUI 子系统无 stdout；调试请查看日志文件 |
| `logLevel` | `info` | `info` / `warn` / `err` |
| `windowWidth` | `1280` | 窗口宽度（像素） |
| `windowHeight` | `720` | 窗口高度（像素） |

配置文件缺失或解析失败时，客户端使用上表默认值并在日志中输出中文警告。

Logs: 仓库根目录 `logs/client_YYYYMMDD.log`（从 exe 向上查找含 `main.cpp` 的目录；发布包无源码时回退 exe 旁 `logs/`）

### 日志文案规范

- 客户端日志固定文案统一使用中文（`ClientLogger::info/warn/err`）。
- 保留格式占位符与参数顺序（如 `%s`、`%d`、`%u`、`%zu` 等）不变。
- 错误日志建议包含“模块 + 动作”上下文，便于定位问题，例如：`LoginSession：连接失败：%s`。

## 中文 UI 与 UTF-8

- 源码字符串为 UTF-8（`u8""` + MSVC `/utf-8`）。
- UI 须加载 `NotoSansSC-Regular.otf`（见 `assets/fonts/`）；日志中应出现 `UiTheme: loaded font ...NotoSansSC-Regular.otf`。
- 登录流程背景：`assets/ui/login_bg_sheet.png`（全画面序列帧水墨山水，选区/登录/注册共用）；由 `login_bg_anim.json` 配置帧数与 fps。缺失时回退 `login_bg.png` 静态图。
- SFML 2.x 不可直接把 UTF-8 `std::string` 传给 `sf::Text`；项目通过 [`sdk/util/TextUtil.h`](sdk/util/TextUtil.h) 的 `TextUtil::utf8ToSfString()`（内部 `sf::String::fromUtf8`）统一转换。

若仍为方框/乱码：先确认字体文件存在；若字体已加载仍乱码，检查是否遗漏 `fromUtf8` 路径。

## 共享协议（RPG_Common Submodule）

[`Common/`](Common/) 指向 GitHub 仓库 [RPG_Common](https://github.com/hechuangguo/RPG_Common)，与 Server 侧 `Common/` 为**同一源码**。无需再手动 copy 到 RPG_Server。

| 脚本 | 说明 |
|------|------|
| [`scripts/init_common.ps1`](scripts/init_common.ps1) | `git submodule update --init --recursive` |
| [`scripts/sync_common.ps1`](scripts/sync_common.ps1) | 拉取 RPG_Common 最新 main 到 `Common/` |

**修改协议：**

1. 在 `Common/` 内编辑并提交、push 到 RPG_Common。
2. 回到 RPG_Client 根目录：`git add Common && git commit -m "chore: bump Common submodule"`。

**拉取 Server 方更新：**

```powershell
git pull
git submodule update --init --recursive
```

## Server 联调

协议头位于 `Common/`（submodule）。Server 使用相同 RPG_Common 仓库，挂载路径同为 `Common/`。

区列表协议（LoginServer ClientListen）：

- `C2S_ZONE_LIST_REQ` (0x000B)
- `S2C_ZONE_LIST_RSP` (0x000C) — 变长 body：`Msg_S2C_ZoneListRspHeader` + N×`Msg_S2C_ZoneEntryWire`

账号登录与选角协议（LoginServer / Gateway）：

| 消息 | ID | 说明 |
|------|-----|------|
| `C2S_LOGIN_REQ` | 0x0001 | 账号密码登录 |
| `S2C_LOGIN_RSP` | 0x0002 | 含 `userID`（上次角色）、`loginToken` |
| `S2C_GATEWAY_INFO` | 0x000A | Gateway 地址 |
| `C2S_GATEWAY_AUTH_REQ` | 0x000D | Gateway 票据鉴权 |
| `S2C_USER_LIST` | 0x0006 | 角色列表（变长） |
| `C2S_SELECT_USER_REQ` | 0x0005 | 选择角色进入游戏 |
| `C2S_CREATE_USER_REQ` | 0x0007 | 创建角色 |
| `S2C_CREATE_USER_RSP` | 0x0008 | 创角结果 |
| `S2C_ENTER_GAME` | 0x0009 | 进入场景（mapID、坐标、属性） |

`Msg_S2C_ZoneEntryWire` 扩展字段（v2，单条 112 字节；旧版 v1 为 104 字节，客户端自动兼容）：

| 字段 | 说明 |
|------|------|
| `loadLevel` | `ZoneLoadLevel`：0=畅通 1=繁忙 2=爆满 3=维护 |
| `onlineCount` | 当前在线人数（uint32） |
| `gatewayCount` | 可用网关数量 |

服务端 LoginServer 需升级 RPG_Common 并填充上述字段后，客户端才显示真实繁忙度；SuperServer 上报在线数据后由 LoginServer 合并下发（见 Server 侧实现）。
