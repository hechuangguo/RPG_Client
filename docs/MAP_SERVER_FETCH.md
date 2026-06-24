# SceneServer 地图包拉取规范

SceneServer 与客户端从 **同一 CDN 根**（`mapCdnBaseUrl`）拉取地图资源，缓存到本地 `data/map/{mapId}/`。

## CDN 目录

见 [`RPG_WorldData.md`](RPG_WorldData.md)。根目录示例：`https://cdn.example.com/rpg/map/`。

| 路径 | 用途 |
|------|------|
| `index.json` | 全地图清单（mapId、packageVersion、contentHash） |
| `server/{mapId}/manifest.json` | 服务端包索引 |
| `server/{mapId}/collision/navmesh.bin` | 寻路/碰撞 |
| `server/{mapId}/logic/spawn_points.json` | 出生点（与客户端一致） |
| `server/{mapId}/logic/npc_spawns.json` | NPC 配置（与客户端一致） |
| `client/{mapId}/scene/buildings.json` | 建筑占位（校验用） |
| `addressables/Map_{mapId}/` | 可选：拉取 bundle 做高度/网格校验 |

## navmesh.bin 格式（v1）

| 偏移 | 类型 | 说明 |
|------|------|------|
| 0 | char[4] | 魔数 `RPGN` |
| 4 | uint32 LE | 格式版本（当前 1） |
| 8 | uint32 LE | 顶点数 N |
| 12 | uint32 LE | 索引数 M |
| 16 | float×3×N | 顶点 (x,y,z) |
| 16+12N | uint32×M | 三角形索引 |

由 Unity 菜单 **RPG → Export Map Package** 从已烘焙 NavMesh 导出。

## 拉取流程

1. `GET {baseUrl}/index.json` — 查找 mapId 的 `packageVersion` / `contentHash`
2. `GET {baseUrl}/server/{mapId}/manifest.json` — 校验与 index 一致
3. 下载 `logic/*`、`collision/navmesh.bin` 到 `data/map/{mapId}/`
4. （可选）下载 `client/{mapId}/scene/buildings.json` 与 Addressables bundle

本地集成测试：

```powershell
.\scripts\fetch_map_package.ps1 -MapId 1002 -CdnBaseUrl http://127.0.0.1:8765/rpg/map/
```

## 进图版本校验（建议）

- 客户端 manifest 的 `packageVersion` 应在进图时上报（协议扩展 `map_package_version` 字段，见 RPG_Common）
- SceneServer 缓存版本与上报不一致时返回错误，提示客户端更新资源

## 与 Common/map 的关系

- **开发期**：`Common/map/{mapId}/` 仍为 JSON 真源，可同步到 CDN 包中的 `logic/`
- **运行时**：SceneServer 以 CDN 缓存为准；Common 仅用于编辑与 CI 校验
