---
name: chinese-log-standardization
overview: 将客户端全部固定英文日志文案替换为中文，并在 README 与 .cursor/rules 中固化“新增日志必须中文”的长期规范。
todos:
  - id: replace-all-clientlogger-english-text
    content: 替换全项目 ClientLogger 固定英文日志为中文并保持格式参数一致
    status: completed
  - id: update-logger-example-comment
    content: 更新 sdk/log/ClientLogger.h 示例注释为中文日志写法
    status: completed
  - id: document-log-language-rule
    content: 在 README.md 增加日志中文规范
    status: completed
  - id: add-cursor-global-rule
    content: 新增 .cursor/rules/log-language.mdc（alwaysApply=true）约束后续日志中文化
    status: completed
  - id: verify-search-and-build
    content: 全文检索残留英文日志并执行 Debug 构建验证
    status: completed
isProject: false
---

# 客户端日志中文化计划

## 目标
- 将项目内所有 `ClientLogger` 固定英文文案统一改为中文。
- 建立长期约束：后续新增日志文案默认使用中文。
- 将约束写入两处文档：根 `README.md` 与 `.cursor/rules` 全局规则。

## 实施范围
- 代码范围：全项目 `ClientLogger::instance().info/warn/err(...)` 的固定字符串文本。
- 不改内容：动态透传文本（例如 Lua 侧传入的业务字符串 `%s` 内容本身）。
- 文档范围：
  - `[d:\Study\RPG_Client\README.md](d:\Study\RPG_Client\README.md)`
  - 新增规则文件 `[d:\Study\RPG_Client\.cursor\rules\log-language.mdc](d:\Study\RPG_Client\.cursor\rules\log-language.mdc)`（`alwaysApply: true`）

## 执行步骤
1. 盘点并替换英文日志
- 逐文件替换 `ClientLogger` 固定文案为中文，保持占位符和参数顺序不变。
- 重点文件（已检索到高频位置）：
  - `[d:\Study\RPG_Client\main.cpp](d:\Study\RPG_Client\main.cpp)`
  - `[d:\Study\RPG_Client\app\GameApp.cpp](d:\Study\RPG_Client\app\GameApp.cpp)`
  - `[d:\Study\RPG_Client\net\LoginSession.cpp](d:\Study\RPG_Client\net\LoginSession.cpp)`
  - `[d:\Study\RPG_Client\net\ZoneListSession.cpp](d:\Study\RPG_Client\net\ZoneListSession.cpp)`
  - `[d:\Study\RPG_Client\net\GameSession.cpp](d:\Study\RPG_Client\net\GameSession.cpp)`
  - `[d:\Study\RPG_Client\ui\UiTheme.cpp](d:\Study\RPG_Client\ui\UiTheme.cpp)`
  - `[d:\Study\RPG_Client\ui\LoginBackgroundAnim.cpp](d:\Study\RPG_Client\ui\LoginBackgroundAnim.cpp)`
  - `[d:\Study\RPG_Client\game\MapRenderer.cpp](d:\Study\RPG_Client\game\MapRenderer.cpp)`
  - `[d:\Study\RPG_Client\lua\LuaManager.cpp](d:\Study\RPG_Client\lua\LuaManager.cpp)`
  - `[d:\Study\RPG_Client\lua\ClientScriptHost.cpp](d:\Study\RPG_Client\lua\ClientScriptHost.cpp)`
  - `[d:\Study\RPG_Client\lua\ScriptBindings.cpp](d:\Study\RPG_Client\lua\ScriptBindings.cpp)`（仅固定前缀/固定句式改中文）

2. 同步头文件示例注释
- 更新 `ClientLogger` 头文件中英文示例，避免新代码继续复制英文写法：
  - `[d:\Study\RPG_Client\sdk\log\ClientLogger.h](d:\Study\RPG_Client\sdk\log\ClientLogger.h)`

3. 写入项目文档规范
- 在 `README.md` 增加“日志文案规范”小节：
  - 固定文案必须中文；
  - 保留 `%s/%d/%u/...` 格式占位；
  - 错误日志要包含上下文关键词（模块+动作）。

4. 添加 Cursor 全局规则
- 新增 `.cursor/rules/log-language.mdc`：
  - `alwaysApply: true`
  - 明确“新增/修改日志时，固定文案必须中文，禁止新增英文固定文案”。
  - 给出 1~2 条正反例，降低执行歧义。

5. 验证
- 执行一次全文检索，确认无残留英文固定日志（以 `ClientLogger` 字面串为准）。
- 进行 Debug 构建，确保替换后无格式化参数错误。

## 交付结果
- 运行日志中的固定文本统一中文。
- README 与 Cursor 规则均写明“后续日志中文化”的长期约束。