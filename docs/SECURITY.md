# 客户端安全说明

## 登录密码（ProtocolVersion minor=1）

### Wire 格式

1. 客户端连接 **LoginServer** 后，服务端首包下发 `S2CLoginChallenge`（`nonce`，建议 16 字节随机数）。
2. 客户端在 `C2SLoginReq` / `C2SRegisterReq` 中携带：
   - `login_nonce`：回显服务端 `nonce`（防重放，单次连接有效）
   - `password_digest`：`SHA-256(UTF-8(密码))`（32 字节，**不含** nonce）
   - `confirm_password_digest`（注册）：`SHA-256(UTF-8(确认密码))`
3. `ProtocolVersion`：`major=1`，`minor=1`（见 [`WireConstants.cs`](../assets/_Project/Scripts/Net/WireConstants.cs)）。

### 客户端实现

- 摘要：[`PasswordDigest.Sha256Utf8Password`](../assets/_Project/Scripts/Util/PasswordDigest.cs)
- 构造：[`ClientMsgHandler.BuildLoginReq` / `BuildRegisterReq`](../assets/_Project/Scripts/Net/ClientMsgHandler.cs)
- 流程：[`LoginSession`](../assets/_Project/Scripts/Net/LoginSession.cs)、[`ZoneListSession`](../assets/_Project/Scripts/Net/ZoneListSession.cs) 在收到 challenge 后再发业务请求。

### 服务端对接清单

- [ ] LoginServer 在 TCP 建立后、其他 Login 消息之前推送 `S2CLoginChallenge`。
- [ ] 校验 `login_nonce` 与本次连接下发的 `nonce` 一致（防重放）。
- [ ] 使用 `SHA-256(UTF-8(密码))` 与库中凭证比对（与 `password_digest` 字段一致）。
- [ ] 拒绝 `ProtocolVersion.minor < 1` 且未带 `login_nonce` 的旧版请求（防降级）。
- [ ] 每个 TCP 连接仅接受一次 `nonce`；重连须重新下发 challenge。

### 已废弃（minor=0）

- 连接后不发 challenge、不带 `login_nonce` 的登录/注册流程。

## 传输层

- 生产环境应启用 TLS（`client_config.xml` 中 `<Tls enabled="1">`），`insecureSkipVerify` 仅用于联调。
- Gateway 鉴权使用 `login_token`（见 `LoginMsg.proto`），不重复传密码摘要。

## 日志

- 禁止将密码、完整 `password_digest` 或 `login_token` 写入日志（遵守 [`log-language.mdc`](../.cursor/rules/log-language.mdc)）。
