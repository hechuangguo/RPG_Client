# Lua 脚本桥接评估（Phase 3）

## 现状

- 运行时脚本：`script/client/`（任务、背包、地图环境等）
- C# 客户端已实现 **`GameScriptHost`**（[`assets/_Project/Scripts/Script/GameScriptHost.cs`](../assets/_Project/Scripts/Script/GameScriptHost.cs)），接收 Chat/Quest/Bag/Notice 并更新 `QuestModel`、`ItemBagModel`

## 方案对比

| 方案 | 优点 | 缺点 |
|------|------|------|
| **XLua** | 复用现有 `.lua`；热更新友好 | 额外 native 插件；Unity 2022 需维护 |
| **逐步 C# 化** | 无 native 依赖；与 Unity 生态一致 | 工作量大；需重写 Lua 逻辑 |

## 结论（本计划）

- **Phase 1–2**：登录/进世界/移动 **不依赖 Lua**；`StreamingAssets/script` 仅只读挂载。
- **当前**：`GameSession` → `GameScriptHost` 已接 System/Chat/Quest/Bag；日志与 C# 模型已更新，Lua 回调预留。
- **Phase 3 推荐**：引入 **XLua** 或在 `GameScriptHost` 内调用 `script/client/init.lua` 中的 `OnEnterGame`/`OnChat` 等全局。

## 对接点

见 [`assets/_Project/Scripts/Script/GameScriptHost.cs`](../assets/_Project/Scripts/Script/GameScriptHost.cs)：

- `OnEnterGame` / `OnTick`
- `OnChat` / `OnNotice` / `OnQuestInfo` / `OnBagInfo`
