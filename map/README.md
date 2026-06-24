# Legacy 地图 JSON（根目录 map/）

本目录为 **遗留镜像**，与 `Common/map/` 内容重复。新流程请使用：

- 编辑真源：`Common/map/{mapId}/`
- 3D 发布：MapExport → CDN（见 `docs/RPG_WorldData.md`）
- `ground.json`、`collision.json`（2D 网格）已废弃，新地图使用 NavMesh + `npc_spawns.json`

同步到 StreamingAssets：`.\scripts\sync_streaming_assets.ps1`（可选，CDN 不可用时回退）。
