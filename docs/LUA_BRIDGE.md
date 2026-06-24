# Lua 脚本桥接评估（Phase 3）

## 现状

- **真源目录**：`script/client/`（编辑此目录后运行 `scripts/sync_streaming_assets.ps1` 同步到 `assets/StreamingAssets/script/`）
- 运行时脚本：任务、背包、地图环境等 Lua 模块为 **Phase 3 设计稿**，当前 **未接入 Lua VM**
- C# 客户端已实现 **`GameScriptHost`**（[`assets/_Project/Scripts/Script/GameScriptHost.cs`](../assets/_Project/Scripts/Script/GameScriptHost.cs)），接收 Chat/Quest/Bag/Notice 并更新 `QuestModel`、`ItemBagModel`；HUD 已订阅模型变更

## 方案对比

| 方案 | 优点 | 缺点 |
|------|------|------|
| **XLua** | 复用现有 `.lua`；热更新友好 | 额外 native 插件；Unity 2022 需维护 |
| **逐步 C# 化** | 无 native 依赖；与 Unity 生态一致 | 工作量大；需重写 Lua 逻辑 |

## 结论（当前阶段）

- **Phase 1–2**：登录/进世界/移动 **不依赖 Lua**；`StreamingAssets/script` 随包挂载但 **不执行**
- **当前**：`GameSession` → `GameScriptHost` → C# 模型 → `GameHudPanel`；Lua 回调与 `EventBus` 预留
- **Phase 3 推荐**：引入 **XLua**，在 `GameScriptHost` 内调用 `script/client/init.lua` 的 `OnEnterGame`/`OnTick` 等

## 对接点

见 [`assets/_Project/Scripts/Script/GameScriptHost.cs`](../assets/_Project/Scripts/Script/GameScriptHost.cs)：

- `OnEnterGame` / `OnTick`
- `OnChat` / `OnNotice` / `OnQuestInfo` / `OnBagInfo`
