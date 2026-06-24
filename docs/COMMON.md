# Common 子模块工作流（RPG_Common）

[`Common/`](../Common/) 是独立仓库 [RPG_Common](https://github.com/hechuangguo/RPG_Common) 的 Git Submodule。**客户端与服务端均可**在各自主仓的 `Common/` 目录内修改、提交并推送，再 bump 主仓的 submodule 指针。

## 目录职责

| 路径 | 内容 | 提交到 |
|------|------|--------|
| `Common/*.proto` | 客户端 wire 协议 | RPG_Common |
| `Common/map/{mapId}/` | 双端共用地图 JSON（meta、spawns、collision 等） | RPG_Common |
| `assets/_Project/` | Unity 场景、Prefab、C# | RPG_Client |
| `Protobuf/*.cs` | protoc 生成物 | **不提交**（本地生成） |

## 修改并提交（客户端）

### 方式 A：一键脚本（推荐）

```powershell
.\scripts\commit_push_all.ps1 -m "feat: 协议与客户端联调"
```

顺序：**先** commit/push `Common/`（RPG_Common），**再** commit/push 主仓（含 `Common` 指针与 C# 改动）。

若本次**不要**推 Common，加 `-SkipCommon`：

```powershell
.\scripts\commit_push_all.ps1 -m "feat: 仅客户端" -SkipCommon
```

### 方式 B：手动两步

```powershell
# 1. Common 子模块（须在 main 分支，勿 detached HEAD）
cd Common
git checkout main
git pull origin main
git add .
git commit -m "feat(login): 增加 model_id"
git push origin main
cd ..

# 2. 主仓：更新指针 + 客户端代码
git add Common assets/_Project/
git commit -m "feat: 角色 model_id 联调"
git push origin main
```

### 修改 proto 后的本地生成

```powershell
.\scripts\sync_protobuf.ps1
```

然后在 `assets/_Project/Scripts/Net/` 等处联调新生成的 `Protobuf/*.cs`。

## 拉取对方更新

```powershell
.\scripts\sync_common.ps1
# 或完整同步
.\sync_all.bat
```

若本地 `Common/` 有未提交修改，`sync_common` / `sync_all` 会**警告**可能冲突，请先 commit 或 stash，再拉 remote。

## detached HEAD 说明

若 `git -C Common branch` 显示 `detached at xxxxx`，须先：

```powershell
git -C Common checkout main
```

否则 commit 无法直接 push 到 `main`。

## 禁止事项

- **不要**在 RPG_Client 或 RPG_Server 主仓内复制一份 `.proto` 独立维护
- **不要**手改 `Protobuf/*.cs`（AUTO-GENERATED，由 `sync_protobuf.ps1` 生成）
- **不要**把 Unity 场景、bundle、Addressables 构建物放进 Common

## 相关脚本

| 脚本 | 说明 |
|------|------|
| [`scripts/commit_push_all.ps1`](../scripts/commit_push_all.ps1) | 默认先推 Common，再推主仓 |
| [`scripts/sync_common.ps1`](../scripts/sync_common.ps1) | 拉取 RPG_Common `main` |
| [`scripts/sync_protobuf.ps1`](../scripts/sync_protobuf.ps1) | `Common/*.proto` → `Protobuf/*.cs` |
| [`scripts/init_common.ps1`](../scripts/init_common.ps1) | 首次 `git submodule update --init` |

## 服务端

RPG_Server 同样挂载 `Common/` submodule，流程对称：在 `Common/` 内改 proto 或 `map/`，push RPG_Common，主仓 bump 指针。详见 RPG_Common 仓 [`README.md`](../Common/README.md)。
