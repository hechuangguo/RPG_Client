# UI Fonts

RPG Client 登录与 HUD 文本使用 **Noto Sans SC Regular** 显示简体中文。

| File | Description |
|------|-------------|
| `NotoSansSC-Regular.otf` | Noto Sans SC Regular（由 `fetch_font.ps1` 下载，不入 Git） |

## License

[Noto Sans SC](https://fonts.google.com/noto/specimen/Noto+Sans+SC) — [SIL Open Font License 1.1](https://scripts.sil.org/OFL)

## Download

从仓库根目录运行：

```powershell
.\assets\fonts\fetch_font.ps1
```

## Troubleshooting

若登录界面出现方框或乱码：

1. 确认 `NotoSansSC-Regular.otf` 已存在于本目录
2. 在 Unity 中将字体导入为 Text 组件可用的 Font 资产（Boot 场景 Setup 使用内置 LegacyRuntime 字体作为 fallback）
3. 重新运行 `fetch_font.ps1` 后刷新 Unity 资产
