# RPGClient

Windows PC MMORPG 2D client (C++17 + SFML).

## Build

Prerequisites: Visual Studio 2022/2026 with **Desktop development with C++** workload, CMake, Ninja.

```powershell
cd Client
.\build_client.ps1
```

Or manually (Developer Command Prompt for VS):

```powershell
cd Client
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output: `Client/build/bin/RPGClient.exe`

If `cl.exe` is missing, open Visual Studio Installer → Modify → check **Desktop development with C++**.

## Run

Start Server (external `RPG_Server`), then double-click `RPGClient.exe` from `build/bin/`.
Ensure `config/`, `script/`, `database/`, `basefile/`, `map/`, `assets/` are beside the exe (POST_BUILD copies them).

## Config

- `config/serverlist.xml` — game zones (sync with LoginServer ZoneInfo)
- `config/client_config.json` — window/log settings

Logs: `./logs/client_YYYYMMDD.log`
