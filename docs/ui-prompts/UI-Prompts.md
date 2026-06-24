# UI 原型图提示词 (UI Prompts)

> 根据 `docs/Product-Spec.md` 生成。登录流程共用静态仙侠背景 `backdrop_base.png`。

---

## 设计风格与配色

**视觉风格**：中国仙侠 × 青绿山水 × 金霞远景

**构图原则**：
- 单张全屏底图，远景山峦与霞光天空占主体
- 下部可柔和渐变至透明，便于与 UI 面板衔接
- 无分层动效、无飞鸟/渔船等叠加特效

**配色方案**：
- **主色** `#2a6b5a` — 湖水、植被基调
- **远山** `#4a7c8c` — 雾气中山体
- **强调** `#d4a574` — 霞光、高光
- **面板** `rgba(20,26,36,0.5)` — 全屏半透明遮罩
- **正文** `#e8eaed` — 表单与标题

---

## 登录背景：backdrop_base

**目录**：`assets/_Project/Art/UI/LoginFlowBackdrop/`

**功能描述**：登录流程（ZoneHome ~ CharacterSelect）共用一张静态 16:9 仙侠山水底图；进 Game 后由 `LoginFlowBackdrop` 隐藏。

**提示词**：
```
Chinese xianxia MMORPG login background, panoramic ink-wash landscape,
golden hour sky with layered misty mountains and distant pagoda peaks,
serene lake or valley in mid-ground, soft atmospheric perspective,
cinematic 16:9 1920x1080, full-bleed game UI backdrop,
elegant teal-green and warm gold palette, no text no UI chrome,
high detail painterly style suitable for PC game login screen
```

**导出要求**：
- 比例 **16:9**，建议 **1920×1080**
- PNG；若下部需与面板融合，可使用真实 alpha 渐变
- 替换后执行 Unity 菜单 **RPG → Add Login Flow Backdrop** 重新绑定

---

## 区服首页 (ZoneHome)

**功能描述**：显示当前区名、「选择区服」「进入游戏」；半透明遮罩叠在静态背景上。

**提示词**：
```
Chinese xianxia MMORPG server home screen mockup, 1280x720,
dark semi-transparent panel overlay on full-screen landscape background,
centered zone name text area, two primary buttons (select server, enter game),
Simplified Chinese labels, UGUI-style flat buttons with subtle gold border,
teal and warm gold color scheme, clean readable layout
```

---

## 区列表 (ServerList)

**功能描述**：可滚动区服列表，显示区名、负载、维护状态；确认/取消按钮。

**提示词**：
```
Chinese xianxia game server list panel, 1280x720,
scrollable server rows on semi-transparent dark overlay,
each row shows server name load indicator and status,
confirm and cancel buttons at bottom,
Simplified Chinese, UGUI list style, teal gold palette
```

---

## 登录 (AuthLogin)

**功能描述**：账号、密码输入框，记住密码，登录/注册切换。

**提示词**：
```
Chinese xianxia MMORPG login form, 1280x720,
account and password input fields on translucent dark panel,
remember password checkbox, login button, link to register,
Simplified Chinese labels, elegant minimal UGUI form,
background visible through panel, teal and gold accents
```

---

## 注册 (Register)

**功能描述**：账号、密码、确认密码；注册/返回登录。

**提示词**：
```
Chinese xianxia game registration form, 1280x720,
three input fields account password confirm password,
register and back to login buttons,
Simplified Chinese, same visual style as login screen,
semi-transparent panel over landscape backdrop
```

---

## 选角/创角 (CharacterSelect)

**功能描述**：角色列表与创角表单同屏；选角进入世界。

**提示词**：
```
Chinese xianxia character select screen, 1280x720,
character list on left or center, create character form on same screen,
name input vocation and gender selectors, enter world button,
Simplified Chinese, dark translucent panels, landscape background,
MMORPG login flow consistent art style
```
