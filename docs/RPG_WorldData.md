# RPG_WorldData 目录规范（v1）

客户端 MapExport 产出 **client 包**；**server 包** 交付 SceneServer 部署，不改 RPG_Server 仓库。

## client 包（Addressables / `map/{mapId}/`）

```text
client/{mapId}/
  manifest.json          # version、mapId、sceneName
  logic/
    spawn_points.json
    npc_spawns.json
    triggers.json
  scene/
    buildings.json       # Prefab 路径 + Transform
  environment/
    lighting.json
```

## server 包（SceneServer `data/map/{mapId}/`）

```text
server/{mapId}/
  manifest.json          # 与 client version 一致
  collision/
    navmesh.bin
  logic/
    spawn_points.json
    npc_spawns.json
```

## manifest.json v1 示例

见 [`manifest_v1.example.json`](manifest_v1.example.json)。

## 坐标系

Unity 左手系 Y-up；MapExport 导出时写入 `meta/coord_transform.json`（若服务端仍为右手系则在此转换）。
