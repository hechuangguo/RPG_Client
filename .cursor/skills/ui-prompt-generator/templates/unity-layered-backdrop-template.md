# Unity 分层背景提示词片段（复制进 UI-Prompts.md）

用于登录流等全屏 UGUI 背景，一次生成**可拆分**的分层素材。

---

## 方案 A：完整底图（推荐当前项目）

**适用**：只需一张主背景 + 代码驱动雾/鸟动效。

```
Chinese xianxia MMORPG login screen background, 16:9, 1920x1080,
misty mountains at golden hour, jade-green lake, distant pagodas,
waterfall on cliff, pine trees foreground, cohesive single painting,
no UI elements, no text, cinematic atmospheric perspective,
teal and gold color palette, soft volumetric mist, game art style,
high detail, seamless composition, PNG
```

## 方案 B：真分层（base 仅远景）

**backdrop_base**（上半远景，底部留空或渐变到纯色）：

```
Xianxia landscape sky and far mountains only, upper 70% detailed,
lower 30% fades to transparent or soft dark teal gradient,
16:9 1920x1080, golden sunset, no water surface, no trees,
game background layer, PNG with alpha
```

**backdrop_water**（可横向平铺）：

```
Seamless tileable calm jade lake surface texture, top-down view,
subtle ripples, xianxia style, horizontal repeat, alpha channel,
no hard horizontal seam, 512x256 or 1024x512, PNG
```

**backdrop_mist**（可横向平铺）：

```
Seamless soft white mist cloud strip, ethereal, low contrast,
horizontal tile, alpha, 1024x256, PNG
```

**bird**：

```
Small silhouette bird flock, side view, 48x24 pixels style,
single dark shape on transparent, PNG
```

---

## 面板可读性（生成 UI mock 时附加）

```
Dark semi-transparent overlay panel rgba(20,26,36,0.5) centered form area,
Chinese labels, high contrast white text, minimal flat UGUI style,
1280x720 layout reference, login form mockup on top of xianxia background
```
