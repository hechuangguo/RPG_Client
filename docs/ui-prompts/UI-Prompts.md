# UI 原型图提示词 (UI Prompts)

> 根据 `docs/Product-Spec.md` 生成。登录流程共用仙侠背景（**近视图拆分底图 + 动效层**）。

---

## 设计风格与配色

**视觉风格**：中国仙侠 × 青绿山水 × 金霞远景 × **低机位近视图**

**构图原则**：
- 近景水面/树冠约占屏高 **40~42%**
- 远景山峦压缩在上部 **50~55%**
- 低机位略仰视，增强近景湖水与枝叶体量感

**配色方案**：
- **主色** `#2a6b5a` — 湖水、近景植被
- **远山** `#4a7c8c` — 雾气中山体
- **强调** `#d4a574` — 霞光、高光
- **面板** `rgba(20,26,36,0.5)` — 全屏半透明遮罩
- **正文** `#e8eaed` — 表单与标题

---

## 核心 UI 提示词（近视图拆分模式）

登录背景采用 **远景 base + 近景动效层 + 前景 FX**：

| 层 | 动效 |
|----|------|
| Base | 静态：天空 + 远山 |
| Waterfall | Shader UV 下落 |
| Water | Shader 流水波纹 |
| Trees | Shader 风吹摇摆 |
| BirdFx | 序列帧横穿 |
| FishingBoat | 漂移 + 起伏 |

### backdrop_base — 远景-only（天空 + 远山）

**功能描述**：低机位近视图构图；上部 50~55% 为天空与远山；**不含湖面与左侧近景树冠**；下部 40~45% 透明或青绿柔和渐变，供水面层衔接。

**提示词**：
```
Chinese xianxia game background, low near camera angle slight upward view,
upper 55% golden hour sky and layered misty far mountains, distant pagoda peaks,
right cliff with distant waterfall hint, lower 45% fades to transparent soft teal gradient,
NO lake surface NO foreground pine trees NO water ripples,
cinematic 16:9 1920x1080, PNG with alpha channel, game UI backdrop layer
```

**落盘路径**：`assets/_Project/Art/UI/LoginFlowBackdrop/backdrop_base.png`

---

### backdrop_water — 近景可平铺湖面

**提示词**：
```
Seamless tileable near-view jade lake surface, stronger top-down ripple texture,
xianxia ink wash style, horizontal repeat only, subtle green-blue tones,
no objects no shore, alpha channel, no hard horizontal seam, 1024x512 PNG
```

**落盘路径**：`assets/_Project/Art/UI/LoginFlowBackdrop/backdrop_water.png`

---

### backdrop_trees — 左侧近景树冠/枝叶

**提示词**：
```
Foreground pine tree canopy and branches left frame only, xianxia ink silhouette,
transparent background, tall branches reaching upper middle for wind sway,
NO sky NO water NO mountain, covers lower 58% height left 42% width,
806x418 proportional to 1920x1080 layout, PNG with alpha
```

**落盘路径**：`assets/_Project/Art/UI/LoginFlowBackdrop/backdrop_trees.png`

---

### backdrop_waterfall — 右侧远景瀑帘条带

**提示词**：
```
Vertical waterfall strip on cliff edge, xianxia style soft white water mist,
tileable vertically, transparent sides, right-side composition element,
256x512 PNG with alpha
```

**落盘路径**：`assets/_Project/Art/UI/LoginFlowBackdrop/backdrop_waterfall.png`

---

### fx_bird_sheet — 飞鸟序列帧（4 帧）

**提示词**：
```
Bird flight cycle sprite sheet, 4 frames horizontal strip, 96x48 per frame,
light pale bird silhouette on transparent, side view wing flap loop,
384x48 PNG, visible against golden sky, no background
```

**落盘路径**：`assets/_Project/Art/UI/LoginFlowBackdrop/fx_bird_sheet.png`

---

### fx_boat_fisherman — 渔船 + 渔夫

**提示词**：
```
Traditional Chinese wooden fishing boat with seated fisherman silhouette,
side view, fisherman holding fishing rod, xianxia ink style on transparent,
about 200x100 pixels, game UI prop, PNG with alpha
```

**落盘路径**：`assets/_Project/Art/UI/LoginFlowBackdrop/fx_boat_fisherman.png`

---

## 交互流程提示词

### 流程：区服首页 → 登录 → 选角

**流程描述**：同一近视图背景连续显示，仅前景面板切换；顶部 StatusBar 固定。

**关键界面**：ZoneHome、ServerList、AuthLogin、Register、CharacterSelect

**Mock 提示词（含 UI，仅作布局参考）**：
```
PC game login UI mockup 1280x720, Chinese xianxia near-view lake background,
large foreground water bottom, distant mountains top, dark semi-transparent center panel,
account password fields Chinese labels 账号 密码 登录, status bar top, PNG
```

---

## 设计建议

- 面板 alpha 保持 0.45~0.55；对比度不足时加居中卡片
- 替换分层图后 Unity 非 Play 模式执行 **RPG → Add Login Flow Backdrop** 重新绑定
- `scripts/generate_login_backdrop_placeholders.py` 仅生成 FX 占位；分层底图请用 AI 按上文提示词出图后覆盖
- **勿使用** `bird.png`（全景误用）作为飞鸟资源

---

**文档版本**：0.3  
**最后更新**：2026-06-24  
**对应的产品文档**：docs/Product-Spec.md
