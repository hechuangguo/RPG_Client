---
name: register-flow-client
overview: 完善客户端“开宗立派”注册链路，确保请求体完整、状态提示准确，并覆盖成功/失败分支验证。
todos:
  - id: update-register-packet-signature
    content: 修改 ClientMsgHandler 的 buildRegisterReq 签名并写入 confirmPassword
    status: completed
  - id: thread-confirm-password-through-session
    content: 在 GameApp -> LoginSession -> ClientMsgHandler 调用链传递 confirmPassword
    status: completed
  - id: verify-register-user-feedback
    content: 验证成功/失败/异常提示逻辑并进行 Debug 构建回归
    status: completed
isProject: false
---

# 客户端注册链路完善计划

## 目标
让“输入账号/密码/确认密码 → 发送注册请求 → 接收结果并提示”完整闭环，且与当前协议保持一致。

## 现状确认
- UI 已有注册表单校验与按钮触发：`[d:\Study\RPG_Client\ui\RegisterPanel.cpp](d:\Study\RPG_Client\ui\RegisterPanel.cpp)`。
- 登录会话已支持注册状态机与回包处理：`[d:\Study\RPG_Client\net\LoginSession.cpp](d:\Study\RPG_Client\net\LoginSession.cpp)`。
- 协议中 `Msg_C2S_RegisterReq` 明确包含 `confirmPassword`：`[d:\Study\RPG_Client\Common\ClientMsg.h](d:\Study\RPG_Client\Common\ClientMsg.h)`。
- 目前 `buildRegisterReq` 只填了 `account/password`，未填 `confirmPassword`：`[d:\Study\RPG_Client\sdk\net\ClientMsgHandler.cpp](d:\Study\RPG_Client\sdk\net\ClientMsgHandler.cpp)`。

## 实施步骤
1. 调整注册组包接口与实现
- 在 `ClientMsgHandler` 的 `buildRegisterReq` 增加 `confirmPassword` 参数。
- 组包时写入 `body.confirmPassword`，并保持长度截断/零填充策略与 `account/password` 一致。

2. 打通调用链
- 在 `LoginSession` 中保存并传递 `confirmPassword`（`startRegister`、成员变量、`sendRegisterReq`）。
- 在 `GameApp` 的注册入口将 UI 的确认密码传入 `LoginSession`。

3. 提示文案与状态一致性
- 保持成功路径：注册成功后回到登录页并显示“注册成功，请返回登录”。
- 保持失败路径：服务器返回 `S2C_REGISTER_RSP.code != 0` 时显示“注册失败: <msg>”。
- 确认注册中断线、解析失败等错误仍落到统一错误提示。

4. 验证与回归
- 本地编译 Debug，确保接口改动无编译错误。
- 手工验证 4 个核心场景：
  - 密码与确认密码不一致（前端拦截）
  - 正常注册成功
  - 账号重复/参数非法（服务端失败码）
  - 注册阶段断线/超时

## 交付结果
- 注册请求体字段与协议定义一致（包含 `confirmPassword`）。
- 用户看到的注册反馈与服务端返回一致，且异常路径可感知。