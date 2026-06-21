# 角色资源（Unity 3D）

本目录存放玩家角色 **Prefab / 模型** 占位资产。

- 在 Boot 场景中为 `EntityManager._playerPrefab` 指定 Capsule 或本目录下的 Prefab
- 未配置 Prefab 时 `EntityManager.CreateEntity` 会报错并返回 null，不会静默创建 Primitive

可选子目录 `player/`：正式美术模型导入后在此维护。
