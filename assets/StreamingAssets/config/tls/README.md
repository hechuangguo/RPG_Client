# TLS 证书（客户端）

客户端通过 `config/client_config.xml` 的 `<Tls>` 段启用传输层加密，需信任服务端 CA。

## 获取 ca.crt

从 RPG_Server 仓库复制（须先执行服务端 `scripts/gen_tls_certs.sh`）：

```
RPG_Server/config/tls/ca.crt  →  RPG_Client/config/tls/ca.crt
```

构建时 `config/` 会同步到 `build/bin/config/`，运行时从 exe 同目录下的 `config/tls/ca.crt` 加载。

## 配置项

| 属性 | 说明 |
|------|------|
| `enabled` | `1` 启用 TLS（与服务端 `Tls enabled=1` 对齐） |
| `ca` | CA 证书路径（相对 exe 工作目录） |
| `insecureSkipVerify` | `1` 跳过证书校验（仅本地调试） |
| `minVersion` | 最低 TLS 版本：`1.2` 或 `1.3` |

## 联调自检

服务端：

```bash
openssl s_client -connect 127.0.0.1:9010 -CAfile config/tls/ca.crt -brief </dev/null
```

客户端须对 **9010（Login）** 与 **9005（Gateway）** 均走 TLS，应用层协议帧不变。
