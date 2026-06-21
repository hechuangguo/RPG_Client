# UI Assets

Login and pre-game screens use an integrated animated background (full-scene sprite sheet).

| File | Description |
|------|-------------|
| `login_bg_sheet.png` | Horizontal sprite sheet: each column is one full 16:9 ink-wash landscape frame (birds, waterfall, boat motion baked in). Primary background. |
| `login_bg_anim.json` | Playback config: `sheet`, `frames`, `fps`. |
| `login_bg.png` | Optional static fallback if the sheet is missing. |

## `login_bg_anim.json`

```json
{
  "sheet": "login_bg_sheet.png",
  "frames": 8,
  "fps": 6
}
```

| Field | Description |
|-------|-------------|
| `sheet` | Filename in this folder |
| `frames` | Number of equal-width frames in the horizontal strip |
| `fps` | Playback speed |

Sheet width must be divisible by `frames`. Each frame should be a full-bleed landscape (aspect ratio roughly 1.4–2.1); grid thumbnails with black borders are rejected and the client falls back to `login_bg.png`.

If the sheet and JSON are missing, the client tries `login_bg.png`, then falls back to a gradient.

Assets live under `assets/ui/` in the Unity project; assign or copy into Boot Canvas as needed.
