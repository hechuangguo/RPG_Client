# Lua 脚本桥接评估（Phase 3）

## 现状

- 运行时脚本：`script/client/`（任务、背包、地图环境等）
- 旧 C++ 客户端通过 **原生 Lua 5.4** + `ClientScriptHost` 绑定

## 方案对比

| 方案 | 优点 | 缺点 |
|------|------|------|
| **XLua** | 复用现有 `.lua`；热更新友好 | 额外 native 插件；Unity 2022 需维护 |
| **逐步 C# 化** | 无 native 依赖；与 Unity 生态一致 | 工作量大；需重写 Lua 逻辑 |

## 结论（本计划）

- **Phase 1–2**：登录/进世界/移动 **不依赖 Lua**；`StreamingAssets/script` 仅只读挂载。
- **Phase 3 推荐**：优先 **C# 移植** 任务/背包（`QuestModel`、`ItemBagModel`）；聊天可先 C#。
- **可选**：若必须保留 Lua，引入 **XLua**，在 `assets/_Project/Plugins/XLua/` 放置预编译库，并实现 `LuaScriptHost.cs` 对标 `lua/ClientScriptHost.cpp`。

## 占位接口

见 [`assets/_Project/Scripts/Script/LuaScriptHost.cs`](../assets/_Project/Scripts/Script/LuaScriptHost.cs)（当前为空壳，记录对接点）。
