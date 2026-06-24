# UI 出图提示词（生成物目录）

本目录由 Cursor Skill **`/ui-prompt-generator`** 生成，不要手改生成结果（改需求请编辑 `docs/Product-Spec.md` 后重新生成）。

| 文件 | 说明 |
|------|------|
| `UI-Prompts.md` | 出图提示词正文（运行 skill 后产生） |

## 工作流

1. 编辑 `docs/Product-Spec.md`
2. Agent 对话输入 `/ui-prompt-generator`
3. 将 `UI-Prompts.md` 中的提示词用于图像工具出图
4. PNG 放入 `assets/_Project/Art/UI/<功能名>/`
5. Unity：**RPG → Add Login Flow Backdrop**（登录背景，非 Play 模式）

## Skill 位置

`.cursor/skills/ui-prompt-generator/`（已纳入 Git，clone 即用）
