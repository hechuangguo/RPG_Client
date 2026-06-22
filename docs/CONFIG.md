# 配置格式说明

客户端按职责使用多种配置格式，**不强行统一为单一格式**。

## 一览

| 用途 | 格式 | 路径 | 加载器 |
|------|------|------|--------|
| 网络/TLS/时序 | XML | `assets/StreamingAssets/config/client_config.xml` | [`ClientConfigLoader.cs`](../assets/_Project/Scripts/Config/ClientConfigLoader.cs) |
| 用户偏好 | JSON | `%AppData%/RPGClient/settings.json` | [`LocalSettings.cs`](../assets/_Project/Scripts/Config/LocalSettings.cs) |
| 策划数据表 | Lua | `assets/StreamingAssets/database/*.lua` | `DataTable.load()`（[`basefile/data_table.lua`](../basefile/data_table.lua)） |
| 地图导出 | JSON | `map/` 下 manifest、spawn_points | Editor [`MapExportWindow`](../assets/_Project/Scripts/Editor/MapExportWindow.cs) |

模板与回退：

- 客户端网络配置模板：[`config/client_config.xml.example`](../config/client_config.xml.example)
- 加载顺序：StreamingAssets → 仓库 `config/client_config.xml` → `.example`

## client_config.xml 主要字段

| 字段 | 说明 | 默认 |
|------|------|------|
| `loginHost` / `loginPort` | LoginServer 地址 | `127.0.0.1:9010` |
| `connectTimeoutMs` | TCP/TLS 连接与登录挑战超时 | 10000 |
| `responseTimeoutMs` | 业务响应超时 | 15000 |
| `heartbeatIntervalMs` | Gateway 心跳间隔 | 10000 |
| `moveSendIntervalMs` | 移动包最小间隔 | 100 |
| `moveMinDelta` | 移动位移阈值（米） | 0.01 |
| `maxFlushBytesPerPoll` | 每帧主线程最大发送字节 | 16384 |
| `Tls` | TLS 开关、CA、跳过校验 | 见 example |

## 何时修改

- **联调换 IP/端口**：改 `client_config.xml` 的 `loginHost`/`loginPort`，无需重编译。
- **记住账号/区服**：由 `LocalSettings` 自动读写 JSON，勿手改 XML。
- **物品/任务等数值**：改 `database/*.lua` 后执行 `sync_streaming_assets.ps1`。
- **协议/消息**：见 [`Common/`](../Common/) submodule，非本文件范畴。

## 安全相关

登录密码 wire 格式见 [`SECURITY.md`](SECURITY.md)；与 `client_config.xml` 分离。
