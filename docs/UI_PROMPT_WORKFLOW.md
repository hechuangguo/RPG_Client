# UI 出图规范工作流

## 一句话

**Product-Spec 定需求 → `/ui-prompt-generator` 出提示词 → 图像工具出 PNG → 放进 Unity Art 目录。**

## 文件约定

| 路径 | 角色 |
|------|------|
| `docs/Product-Spec.md` | 人工维护的界面需求（Git 跟踪） |
| `docs/ui-prompts/UI-Prompts.md` | Skill 生成的出图提示词（Git 跟踪，可重复生成覆盖） |
| `.cursor/skills/ui-prompt-generator/` | Cursor Agent Skill（Git 跟踪） |
| `assets/_Project/Art/UI/` | 运行时美术资源 |

## 新同事

1. `git clone` 后 Skill 已在 `.cursor/skills/`，重启 Cursor 即可
2. 无需再执行 `npx skills add`（除非要从上游 `zinohome/cozyengine` 升级 skill）
3. 升级 skill 后请把变更同步回 `.cursor/skills/ui-prompt-generator/` 并提交

## 上游来源

- Skill：`zinohome/cozyengine@ui-prompt-generator`
- 本项目已定制：见 skill 内 `references/rpg-client-constraints.md`
