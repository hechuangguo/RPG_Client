# RPGClient

Windows PC MMORPG 2D client (C++17 + SFML). Runs as a **GUI application** (no console window).

## 目录结构

```
Client/
  3Party/      # SFML、Lua、tinyxml2 等第三方库
  Common/      # 与 Server 对齐的 wire 协议头（ClientMsg.h 等）
  sdk/         # 底层封装（net/log/time/math/util）
  app/ game/ lua/ net/ ui/ util/   # 应用层
  config/ script/ database/ basefile/ map/ assets/
  assets/fonts/   # NotoSansSC-Regular.otf（fetch_font.ps1 下载）
  main.cpp
  CMakeLists.txt
  build_client.ps1
```

## Build

Prerequisites: Visual Studio 2022/2026 with **Desktop development with C++** workload, CMake, Ninja.

首次构建前先准备第三方库与中文字体：

```powershell
cd Client
.\3Party\download_and_build.ps1
.\assets\fonts\fetch_font.ps1
.\build_client.ps1
```

`build_client.ps1` 会自动调用 `fetch_font.ps1`。字体未下载时登录界面中文会显示为方框/乱码。

Or manually (Developer Command Prompt for VS):

```powershell
cd Client
.\assets\fonts\fetch_font.ps1
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output: `Client/build/bin/RPGClient.exe`

If `cl.exe` is missing, open Visual Studio Installer → Modify → check **Desktop development with C++**.

### Visual Studio (Debug / Release)

- **Recommended for daily work:** `.\build_client.ps1` (Ninja + Release).
- **Opening in VS:** Open the `Client/` folder as a CMake project. After changing SFML link/copy logic, delete `build/` (and VS `out/` if present) and re-configure before F5.
- **Debug vs Release:** CMake links `sfml-*-d` import libs and copies `sfml-*-d-2.dll` for Debug; Release uses `sfml-*` / `sfml-*-2.dll`. Do not mix Debug EXE with Release SFML DLLs.
- **Fonts:** Run `fetch_font.ps1` before F5 so `out/build/x64-Debug/bin/assets/fonts/` contains `NotoSansSC-Regular.otf`.
- **No console:** F5 starts the SFML window only; logs go to `./logs/client_YYYYMMDD.log`.

## Run

Start Server (external `RPG_Server`), then double-click `RPGClient.exe` from `build/bin/`.
Ensure `config/`, `script/`, `database/`, `basefile/`, `map/`, `assets/` are beside the exe (POST_BUILD copies them).

登录界面：区服为**下拉框**选择（点击展开列表）；输入框聚焦时显示**闪烁光标**。

## Config

- `config/serverlist.xml` — game zones (sync with LoginServer ZoneInfo)
- `config/client_config.json` — window/log settings

| Field | Default | Notes |
|-------|---------|-------|
| `logToConsole` | `false` | GUI 子系统无 stdout；调试请查看日志文件 |
| `logLevel` | `info` | `info` / `warn` / `err` |

Logs: `./logs/client_YYYYMMDD.log`

## 中文 UI 与 UTF-8

- 源码字符串为 UTF-8（`u8""` + MSVC `/utf-8`）；`config/serverlist.xml` 亦为 UTF-8。
- UI 须加载 `NotoSansSC-Regular.otf`（见 `assets/fonts/`）；日志中应出现 `UiTheme: loaded font ...NotoSansSC-Regular.otf`。
- SFML 2.x 不可直接把 UTF-8 `std::string` 传给 `sf::Text`；项目通过 [`sdk/util/TextUtil.h`](sdk/util/TextUtil.h) 的 `TextUtil::utf8ToSfString()`（内部 `sf::String::fromUtf8`）统一转换。

若仍为方框/乱码：先确认字体文件存在；若字体已加载仍乱码，检查是否遗漏 `fromUtf8` 路径。

## Server 联调

协议头位于 `Client/Common/`。变更 `ClientMsg.h` 等文件后，需手动同步到外部 `RPG_Server/common/`（Server 不在本仓库）。
