# 实施范围（RPG_Client Unity 客户端）

## 本仓包含

- Unity C# 客户端：`assets/_Project/`、`ProjectSettings/`、`Packages/`
- 构建依赖：`3Party/`（protoc、Google.Protobuf）
- 协议生成物：根目录 `Protobuf/*.cs`（`scripts/sync_protobuf.ps1` 生成，**已 gitignore，不提交**）
- 运行时数据真源：`config/`、`script/`、`database/`、`basefile/`、`map/` → 经 `scripts/sync_streaming_assets.ps1` 生成 `assets/StreamingAssets/`（生成物不提交）
- 脚本：`sync_all.bat`、`build_unity_client.ps1` 等

## 本仓不包含

- **RPG_Server** 代码与部署

## Common 子模块（RPG_Common）

客户端**可以**在 `Common/` 内修改 `.proto`、`map/` 等共享数据，提交并推送到 RPG_Common，再在主仓 bump 指针。详见 [`docs/COMMON.md`](COMMON.md)。

### 典型流程

1. 编辑 `Common/*.proto` 或 `Common/map/{mapId}/`
2. `.\scripts\sync_protobuf.ps1`（若改了 proto）
3. 联调 `assets/_Project/Scripts/Net/` 等
4. `.\scripts\commit_push_all.ps1 -m "..."`（先推 Common，再推主仓）

### 拉取对方更新

1. `.\sync_all.bat` 或 `.\scripts\sync_common.ps1`
2. `.\scripts\sync_protobuf.ps1` → `Protobuf/*.cs`

## 引擎版本

团结引擎 **1.6.11** / Editor **2022.3.61t12**（见 `ProjectSettings/ProjectVersion.txt`）。
