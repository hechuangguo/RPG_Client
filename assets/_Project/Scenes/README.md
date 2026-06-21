# World_1002 场景说明

在 Unity Editor 中：

1. **File → New Scene** → 保存为 `assets/_Project/Scenes/World_1002.unity`
2. 添加 **Terrain**（或 Plane 占位）表示 map 1002 地表
3. 放置 **MapLogicMarker**（SpawnPoint）标记出生点
4. 菜单 **RPG → Export Map Package** 导出 client 包

Boot 流程场景：`assets/_Project/Scenes/Boot.unity`（挂载 GameApp + GameUiController + Canvas）。

**首次生成完整 Canvas**：Unity 菜单 **RPG → Setup Boot Scene**（或 `.\scripts\setup_boot_scene.ps1`）。当前 `Boot.unity` 为占位场景，须执行 Setup 后才有 GameApp/Canvas 绑定。

Addressables 分组：`Map_1002`（见计划 §3.3）。
