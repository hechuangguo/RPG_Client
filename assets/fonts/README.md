# UI Fonts

RPGClient login and HUD text use **Noto Sans SC Regular** for Simplified Chinese.

| File | Description |
|------|-------------|
| `NotoSansSC-Regular.otf` | Noto Sans SC Regular (downloaded from `NotoSansCJKsc-Regular.otf`, not in Git) |

## License

[Noto Sans SC](https://fonts.google.com/noto/specimen/Noto+Sans+SC) — [SIL Open Font License 1.1](https://scripts.sil.org/OFL)

## Download

From `Client/`:

```powershell
.\assets\fonts\fetch_font.ps1
```

Or run `.\build_client.ps1` (calls fetch automatically).

## Troubleshooting

If the login screen shows boxes or garbled text:

1. Confirm `NotoSansSC-Regular.otf` exists here and in `build/bin/assets/fonts/` after build.
2. Check `logs/client_YYYYMMDD.log` for `UiTheme: loaded font ...NotoSansSC-Regular.otf`.
3. Re-run `fetch_font.ps1` and rebuild.
