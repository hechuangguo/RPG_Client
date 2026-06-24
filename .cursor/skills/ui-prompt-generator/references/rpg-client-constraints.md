# RPG_Client UI 出图约束（Unity UGUI）

本文件为 `ui-prompt-generator` 在 **RPG_Client** 仓库内的项目扩展，生成提示词时必须遵守。

## 引擎与画布

- 引擎：团结 1.6.11 / Unity 2022.3 URP
- 参考分辨率：**1280×720**（`CanvasScaler` match 0.5）
- UI 技术：**UGUI**（`Image` / `Text` / `Button` / `InputField`），非 Web、非 Figma 落地代码

## 视觉基调（仙侠 MMORPG）

- 主调：青绿山水 + 金霞远景 + 薄雾层次
- 面板：深色半透明遮罩（约 `rgba(20,26,36,0.45~0.55)`），不挡全屏背景
- 文案：**简体中文**（按钮、标题、表单标签）
- 避免：强烈「AI 塑料感」、霓虹赛博、西式奇幻城堡

## 登录流程共用界面（须风格一致）

| 界面 | AppState | 说明 |
|------|----------|------|
| 区服首页 | ZoneHome | 区名、选区、进入游戏 |
| 区列表 | ServerList | 区服列表 |
| 登录 | AuthLogin / Connecting | 账号密码、记住密码 |
| 注册 | Register | 注册表单 |
| 选角/创角 | CharacterSelect | 角色列表 + 创角同屏 |

进游戏（`Game`）后仅 HUD，**不使用**登录背景。

## 分层背景资源规范（静态 PNG + 代码动效）

目录：`assets/_Project/Art/UI/<FeatureName>/`

| 文件 | 用途 | 注意 |
|------|------|------|
| `backdrop_base.png` | 天空 + 远山 + 主场景 | **要么完整场景，要么仅远景+透明底** |
| `backdrop_water.png` | 可平铺水面 | 仅当 base 不含水面时使用 |
| `backdrop_waterfall.png` | 可平铺瀑布条 | 仅当 base 不含瀑布时使用 |
| `backdrop_trees.png` | 近景树（下半透明） | 仅当 base 不含树木时使用 |
| `backdrop_mist.png` | 可平铺雾气 | 全屏薄雾叠层 |
| `bird.png` | 飞鸟剪影 | 小图，16~48px 级 |

**禁止**：`backdrop_base` 已是完整山水时，再叠 water/trees 全宽条带（会出现「上下两张图」接缝）。

导出要求：

- 比例 **16:9**，建议 **1920×1080** 或 **1280×720**
- PNG，带 alpha 的层需真实透明通道
- 命名与上表一致，便于 `BootSceneSetup` / `LoginFlowBackdrop` 自动绑定

## 提示词输出用途

生成的提示词用于：

1. **Midjourney / DALL·E / SD** 等生成概念稿或分层素材
2. 导出 PNG 后放入 `assets/_Project/Art/UI/...`
3. 在 Unity 中由 `LoginFlowBackdrop` 等组件驱动动效（UV 滚动、雾、鸟）

## 生成物落盘路径

- 提示词文档：`docs/ui-prompts/UI-Prompts.md`
- 需求来源：`docs/Product-Spec.md`
- 勿写入 `Library/`、`Temp/`、本机 `client_config.xml`
