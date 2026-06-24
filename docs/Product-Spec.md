# RPG_Client 产品界面规格（UI）

> 本文件为 `ui-prompt-generator` 的需求输入。改界面需求时先改这里，再运行 `/ui-prompt-generator` 生成 `docs/ui-prompts/UI-Prompts.md`。

## 项目概述

Windows PC 仙侠 MMORPG 客户端（团结/Unity UGUI）。玩家经区服选择 → 登录/注册 → 选角/创角 → 进入 3D 世界。

## 目标用户

- 国内 PC 端 MMORPG 玩家，熟悉传奇/仙侠类登录与选角流程
- 期望：界面沉稳、有东方意境，表单清晰可读

## 参考分辨率

- 设计稿：**1280×720**（与 `CanvasScaler` 一致）
- 背景素材可导出 **1920×1080**，Unity 内 stretch 全屏

## 核心界面列表

| 优先级 | 界面 | AppState | 说明 |
|--------|------|----------|------|
| P0 | 登录流程共用背景 | ZoneHome ~ CharacterSelect | 共用静态仙侠底图 `backdrop_base`，进 Game 后隐藏 |
| P0 | 区服首页 | ZoneHome | 当前区、选区、进入游戏 |
| P0 | 区列表 | ServerList | 区名、负载、维护状态 |
| P0 | 登录 | AuthLogin | 账号、密码、记住密码 |
| P0 | 注册 | Register | 账号、密码、确认密码 |
| P0 | 选角/创角 | CharacterSelect | 角色列表 + 创角表单同屏 |
| P1 | 游戏 HUD | Game | 非本 skill 出图范围（3D 世界 + HUD） |

## 登录流程背景（美术）

**目录**：`assets/_Project/Art/UI/LoginFlowBackdrop/`

**规格**：登录流程仅使用静态底图 `backdrop_base.png`，无分层动效、无飞鸟/渔船等特效。

- 进 `AppState.Game` 后隐藏背景
- 替换底图后执行 **RPG → Add Login Flow Backdrop** 重新绑定

## 功能说明模板（新增界面时复制）

### [界面名称]

- **用户目标**：
- **主要控件**：
- **输入/输出**：
- **业务规则**：
- **与背景关系**：共用登录背景 / 独立背景 / 无背景

## 版本

- v0.1 — 初始：登录链路 + 共用仙侠背景
