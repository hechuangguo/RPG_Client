# 实施范围（RPG_Client Unity 客户端）

## 本仓包含

- Unity C# 客户端：`assets/_Project/`、`ProjectSettings/`、`Packages/`
- 构建依赖：`3Party/`（protoc、Google.Protobuf）
- 协议生成物：根目录 `Protobuf/*.cs`（`scripts/sync_protobuf.ps1` 生成，**已 gitignore，不提交**）
- 运行时数据真源：`config/`、`script/`、`database/`、`basefile/`、`map/` → 经 `scripts/sync_streaming_assets.ps1` 生成 `assets/StreamingAssets/`（生成物不提交）
- 脚本：`sync_all.bat`、`build_unity_client.ps1` 等

## 本仓不包含

- **RPG_Server** 代码与部署
- **Common 内 `.proto` 修改**（Submodule 只读；服务端改 RPG_Common 后本仓 sync + bump 指针）

## Common 更新约定

1. 服务端在 [RPG_Common](https://github.com/hechuangguo/RPG_Common) 提交 `.proto`
2. 本仓：`.\sync_all.bat` 或 `.\scripts\sync_common.ps1`
3. 本仓：`.\scripts\sync_protobuf.ps1` → `Protobuf/*.cs`
4. Unity 侧联调 `assets/_Project/Scripts/Net/`（**不**改 Common 源文件）

## 引擎版本

团结引擎 **1.6.11** / Editor **2022.3.61t12**（见 `ProjectSettings/ProjectVersion.txt`）。
