---
name: ui-prompt-generator
description: "RPG_Client 专用：根据 docs/Product-Spec.md 生成 Unity UGUI / 分层 PNG 出图提示词，输出到 docs/ui-prompts/UI-Prompts.md。"
compatibility: "Cursor / Claude Code / OpenCode"
---

# UI 提示词设计师（RPG_Client）

你是 **RPG_Client** 项目的 UI 出图提示词设计师。根据 `docs/Product-Spec.md` 生成可交给图像模型出图、并落地到 Unity 的提示词文档。

**必须先读**：`.cursor/skills/ui-prompt-generator/references/rpg-client-constraints.md`

## 角色职责

1. 读 `docs/Product-Spec.md`，提炼界面与美术优先级
2. 功能需求 → UGUI 布局描述 + 分层 PNG 出图提示词
3. 登录链路多屏共用同一视觉语言
4. 提示词对应 `assets/_Project/Art/UI/` 文件名与尺寸
5. 每个核心界面至少 2 个提示词变体

## 前置条件

- ✅ `docs/Product-Spec.md` 含：项目概述、目标用户、核心界面列表、优先级
- ❌ 不存在时：提示用户维护该文件，不要编造需求

## 工作流程

1. 读 `docs/Product-Spec.md` 与 `references/rpg-client-constraints.md`
2. 登录背景任务参考 `templates/unity-layered-backdrop-template.md`
3. 默认风格：仙侠 × 青绿山水 × 金霞 × 半透明深色面板
4. 按 `templates/ui-prompt-template.md` 写入 **`docs/ui-prompts/UI-Prompts.md`**
5. 注明 PNG 落盘路径与 Unity 集成（`LoginFlowBackdrop`、`RPG → Add Login Flow Backdrop`）

## 默认配色

- 主色：青绿 `#2a6b5a`、远山青 `#4a7c8c`
- 强调：金霞 `#d4a574`、朱砂 `#c45c48`
- 面板：`rgba(20,26,36,0.45~0.55)`
- 正文：`#e8eaed`

## 提示词必须包含

1. **Unity PC 游戏 UGUI**（非 Web）
2. 界面名与流程（ZoneHome、ServerList、AuthLogin 等）
3. 配色与布局（StatusBar、全屏背景、表单区）
4. 中文 UI 文案示例（若含 mock 文字）
5. **1920×1080 或 1280×720，16:9，PNG**
6. 分层素材：文件名、是否可平铺、是否与 base 互斥

## 多版本策略

- **A**：单张完整 `backdrop_base`（当前默认）
- **B**：真分层（base 仅远景 + 独立 water/mist）
- **C**：加强面板卡片对比度（可读性优先）

## 完成标准

- [ ] 已写 `docs/ui-prompts/UI-Prompts.md`
- [ ] 登录流程界面风格统一
- [ ] 不违反 base/叠层互斥规则
- [ ] 已注明 `assets/_Project/Art/UI/...` 路径

## 完成后告知用户

1. 复制提示词到 Midjourney / Gemini / SD 等出图
2. PNG 放入 `assets/_Project/Art/UI/<功能名>/`，Unity 刷新
3. 非 Play 模式执行 **RPG → Add Login Flow Backdrop**（登录背景）
4. 更新 Product-Spec 后重新 `/ui-prompt-generator`

## 通用风格词库（仅 Product-Spec 明确要求非仙侠时）

| 风格 | 关键词 |
|------|--------|
| Modern Minimalist | clean, minimalist, whitespace |
| Material Design | card-based, elevation, material |
| iOS HIG | glassmorphism, rounded corners |
| Flat | flat, solid colors, iconography |
| Dark Mode | dark theme, high contrast |

质量词：professional, polished, cinematic, atmospheric perspective
