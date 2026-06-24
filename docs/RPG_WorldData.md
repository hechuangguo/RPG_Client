# RPG_WorldData 目录规范（v1）

客户端 MapExport 产出 **client 包** 与 **server 包**；二者与 Addressables Remote 构建物一并发布到 CDN。SceneServer 从同一 CDN 拉取并缓存到 `data/map/{mapId}/`。

开发期 JSON 真源亦可放在 [`Common/map/`](../Common/map/)（双端共享逻辑）。

## CDN 根目录（`rpg/map/`）

```text
rpg/map/
  index.json
  client/{mapId}/
    manifest.json
    logic/spawn_points.json
    logic/npc_spawns.json
    logic/triggers.json
    scene/buildings.json
    environment/lighting.json
  server/{mapId}/
    manifest.json
    collision/navmesh.bin
    logic/spawn_points.json      # 与 client 同内容
    logic/npc_spawns.json
  addressables/Map_{mapId}/      # catalog + *.bundle
```

## manifest.json v1

见 [`manifest_v1.example.json`](manifest_v1.example.json)。关键字段：

| 字段 | 说明 |
|------|------|
| `packageVersion` | 包版本（如 `20250624.1`），双端须一致 |
| `contentHash` | 整包 sha256，用于校验 |
| `cdn` | CDN 相对路径索引 |
| `assets.scene` | Addressables 场景地址键 |
| `sharedLogic` | 逻辑 JSON 相对路径 |
| `serverCollision` | 服务端碰撞文件相对路径（相对 server 包根） |

全地图清单：[`map_cdn_index.example.json`](map_cdn_index.example.json)。

## client 包

```text
client/{mapId}/
  manifest.json
  logic/
  scene/
  environment/
```

## server 包（SceneServer `data/map/{mapId}/`）

```text
server/{mapId}/
  manifest.json
  collision/navmesh.bin
  logic/
```

拉取规范见 [`MAP_SERVER_FETCH.md`](MAP_SERVER_FETCH.md)。

## Unity 制作流程

1. `assets/_Project/Scenes/World_{mapId}.unity` — 地形、建筑 Prefab、`MapLogicMarker`
2. Addressables 分组 `Map_{mapId}`（Remote）
3. **RPG → Setup World Scene 1002**（首次）
4. **RPG → Setup Map Addressables**（首次）
5. **RPG → Export Map Package** — 导出 client+server 双端包
6. **RPG → Build Map Addressables** 或 `.\scripts\build_map_packages.ps1`

## 坐标系

Unity 左手系 Y-up；MapExport 可写入 `meta/coord_transform.json`（服务端右手系时转换）。

## Legacy（2D 网格 JSON）

`Common/map/1002/ground.json`、`collision.json`（网格）、根目录 `map/` 镜像为 **遗留**，新地图不再产出；请使用 NavMesh + `npc_spawns.json`。
