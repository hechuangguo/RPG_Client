# Client 第三方库 (3Party)

客户端依赖均放在本目录或通过 CMake FetchContent 构建到此目录。

| 库 | 用途 |
|----|------|
| SFML 2.6+ | 2D 渲染、窗口、输入 |
| Lua 5.4 | 脚本运行时 |
| tinyxml2 | 解析 serverlist.xml |

## 构建

```powershell
cd d:\Study\RPG_Client\Client
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

首次配置会自动下载/编译依赖到 `3Party/_build/`。
