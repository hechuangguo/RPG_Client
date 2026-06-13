# Client 第三方库 (3Party)

客户端依赖均放在本目录或通过 CMake FetchContent 构建到 `_build/`。

| 库 | 用途 |
|----|------|
| SFML 2.6+ | 2D 渲染、窗口、输入 |
| Lua 5.4 | 脚本运行时 |
| tinyxml2 | 解析 serverlist.xml |

## 构建

首次使用前先下载/准备依赖：

```powershell
cd Client\3Party
.\download_and_build.ps1
```

然后回到 `Client/` 编译客户端：

```powershell
cd ..
.\build_client.ps1
```

首次配置 CMake 时，若缺少 tinyxml2 源码，会自动 FetchContent 到 `3Party/_build/`。
