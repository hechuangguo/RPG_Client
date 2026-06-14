# RPG_Client

本仓库为 MMORPG 2D 客户端（C++17 + SFML，Windows PC）。

客户端工程位于 [`Client/`](Client/)，编译、运行与目录说明见 [`Client/README.md`](Client/README.md)。

## 克隆（含共享协议）

`Client/Common/` 为 [RPG_Common](https://github.com/hechuangguo/RPG_Common) 的 **Git Submodule**，首次克隆须带 submodule：

```powershell
git clone --recurse-submodules https://github.com/hechuangguo/RPG_Client.git
```

已克隆但未拉取 Common 时：

```powershell
git submodule update --init --recursive
# 或
cd Client
.\scripts\init_common.ps1
```

## 构建

首次构建需下载第三方库（`Client/3Party/download_and_build.ps1`）与中文字体（`Client/assets/fonts/fetch_font.ps1`，或运行 `build_client.ps1` 自动执行）。

```powershell
cd Client
.\build_client.ps1
```

运行后为纯窗口客户端（无控制台），日志位于 `Client/build/bin/logs/`。

## 协议修改

在 `Client/Common/` 内改 `ClientMsg.h` 等 → 在 submodule 内 commit/push 到 **RPG_Common** → 回到本仓库 commit submodule 指针。详见 [`Client/README.md`](Client/README.md) 与 [RPG_Common README](https://github.com/hechuangguo/RPG_Common)。
