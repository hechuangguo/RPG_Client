# World_1002 场景说明

在 Unity Editor 中：

1. **File → New Scene** → 保存为 `assets/_Project/Scenes/World_1002.unity`
2. 添加 **Terrain**（或 Plane 占位）表示 map 1002 地表
3. 放置 **MapLogicMarker**（SpawnPoint）标记出生点
4. 菜单 **RPG → Export Map Package** 导出 client 包

Boot 流程场景：`assets/_Project/Scenes/Boot.unity`（挂载 GameApp + GameUiController + Canvas）。

**首次生成完整 Canvas**：Unity 菜单 **RPG → Setup Boot Scene**（或 `.\scripts\setup_boot_scene.ps1`）。当前 `Boot.unity` 为占位场景，须执行 Setup 后才有 GameApp/Canvas 绑定。

Setup 会生成：

- `ServerListPanel`：ScrollView 区列表 + 确认/取消
- `CharacterSelectPanel`：ScrollView 角色列表 + 职业/性别 + 创角/进入世界
- `assets/_Project/Prefabs/UI/ZoneListItem.prefab`：区服行模板
- `assets/_Project/Prefabs/UI/CharacterListItem.prefab`：角色行模板

## 区服选择

点击「选择区服」→ 列表展示 → 选中行 → 点「确认」返回区服首页。维护中区不可选。

## 选角与进世界

- 登录成功后进入 **CharacterSelectPanel**
- 点击角色行高亮选中；多角色可切换
- **进入世界**：仅当已选中有效角色（`userId != 0`）时可点
- **创建角色**：输入 2–12 字符名称，选择职业（战士/法师）与性别（男/女）后提交
- 创角或进世界请求进行中，相关按钮会禁用直至服务器响应

Addressables 分组：`Map_1002`（见 World 数据文档）。

**EntityManager**：Setup 后须在 Inspector 为 `EntityManager` 指定 `_playerPrefab`（可为 Capsule 预制体）；未配置时不会静默创建 Primitive。
