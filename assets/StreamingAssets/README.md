# StreamingAssets（生成物）

本目录下 `config/`、`script/`、`database/`、`basefile/`、`map/` 由同步脚本从仓库根目录复制生成，**请勿手改**。

## 生成

```powershell
.\scripts\sync_streaming_assets.ps1
```

## 真源目录

| 生成路径 | 真源 |
|----------|------|
| `config/` | [`config/`](../../config/) |
| `script/` | [`script/`](../../script/) |
| `database/` | [`database/`](../../database/) |
| `basefile/` | [`basefile/`](../../basefile/) |
| `map/` | [`map/`](../../map/) |

Play 模式与打包前须先执行同步。详见 [`docs/CONFIG.md`](../../docs/CONFIG.md)。
