# BroSDK v2.0.0.3 接入参考文档

> 当前版本：v2.0.0.3
> 最后更新：2026-07-09
> 适用对象：桌面客户端、自动化宿主、本机 Web 调试工具、浏览器扩展侧边栏、Agent/MCP 客户端。
> 说明：本文档只描述公开接入契约和稳定行为。动态库函数、结构与返回码以随包 `brosdk.h` 为最终依据。

## 1. 概述

BroSDK 是用于管理真实浏览器环境的本地运行时 SDK。它负责 SDK 初始化、环境管理、浏览器打开/关闭、代理诊断、Cookie/Storage 托管、日志回调、本地 Web API，以及 v2.0.0.3 新增的 MCP 浏览器自动化入口。

v2.0.0.3 的核心新增能力是 MCP：

| 入口 | 用途 |
| --- | --- |
| `/sdk/v1/mcp` | 全局环境管理 MCP，用于列出环境、打开/关闭环境、查询任务、获取单环境 MCP endpoint。 |
| `/sdk/v1/mcp/env/{envId}` | 单环境浏览器自动化 MCP，默认暴露 BrowserOS 对齐的 16 个公开浏览器工具。 |

MCP 是新增的 opt-in 入口，不替代 C ABI、C++ 接口、本地 Web API 或 WebSocket 事件。已有 v2.0.0.2 接入方通常只需替换 SDK 包并按需启用 MCP。

## 2. 接入方式

BroSDK 提供三种公开接入方式。三者共享同一个 SDK 单例、同一套浏览器生命周期和同一份环境数据。

| 接入方式 | 入口 | 适用场景 |
| --- | --- | --- |
| 动态库 C ABI / C++ `ISDK` | `brosdk.h` | 原生客户端、长期驻留宿主、需要回调和本地内存管理的集成。 |
| 本地 Web API + WebSocket | `http://127.0.0.1:{port}/sdk/v1/...` 与 `ws://127.0.0.1:{port}/` | 跨语言集成、调试工具、轻量控制台。 |
| MCP HTTP | `http://127.0.0.1:{port}/sdk/v1/mcp...` | Agent、浏览器扩展侧边栏、MCP Inspector、自然语言浏览器自动化。 |

推荐接入路线：

1. 原生宿主先完成动态库初始化。
2. 初始化时传入 `port` 启用本地 Web API。
3. 通过动态库或 Web API 打开环境。
4. 对 Agent/扩展/自动化客户端暴露单环境 MCP endpoint。
5. 所有异步浏览器生命周期仍以回调或 WebSocket 最终事件为准。

## 3. 版本兼容性

| 领域 | v2.0.0.3 说明 |
| --- | --- |
| C ABI | 保持现有公开函数兼容。新增或增强能力不要求迁移到 MCP。 |
| C++ `ISDK` | 与 C ABI 语义一致。 |
| 本地 Web API | 继续使用 `/sdk/v1/...` 路径。路径版本号不是 SDK 包版本号。 |
| WebSocket | 继续推送既有异步事件。MCP 不替代 WebSocket。 |
| Cookie/Storage 回调 | 保持原有回调签名和内存分配规则。 |
| MCP | 新增入口。客户端必须按 MCP JSON-RPC 流程 `initialize`、`tools/list`、`tools/call`。 |
| 大结果输出 | MCP 大结果通过 artifact 分片读取，不应放宽控制面请求或响应大小。 |

## 4. 平台与发布包

| 平台 | 架构 | 动态库 |
| --- | --- | --- |
| Windows | x64 | `brosdk.dll` |
| Linux | x64 | `brosdk.so` |
| macOS | arm64 / x64 | `brosdk.dylib` |

接入方需要随包使用：

| 文件 | 用途 |
| --- | --- |
| `brosdk.h` | C ABI 和 C++ `ISDK` 公开头文件。 |
| 动态库 | SDK 运行时。 |
| SDK 资源目录 | 浏览器核心、扩展、运行资源等由 SDK 包管理。 |

不要依赖 SDK 包内部目录结构、浏览器启动参数或调试实现细节。需要配置的能力以本文档和 `brosdk.h` 描述为准。

## 5. 通用约定

### 5.1 JSON 与编码

所有请求体、响应体和事件体均为 UTF-8 JSON。动态库接口中的 `len` 表示字节长度，不包含字符串末尾的 `\0`。

`envId` 必须按字符串处理。很多环境 ID 超过 JavaScript 安全整数范围，不要用 number 保存或比较。

### 5.2 内存管理

| 场景 | 规则 |
| --- | --- |
| SDK 通过 `char **out_data` 返回的缓冲区 | 使用后调用 `sdk_free()`。 |
| 回调中的 `data/len` | 只在当前回调有效；需要长期保存时立即复制。 |
| Cookie 替换回调返回 `new_data` | 必须用 `sdk_malloc()` 分配。 |
| 安全策略回调返回 `redirect` | 必须用 `sdk_malloc()` 分配。 |

### 5.3 返回码与异步受理

同步接口返回 `CL_OK` 表示调用完成。异步接口的返回值只表示请求是否被 SDK 受理，最终结果通过 `sdk_result_cb_t` 或 WebSocket 事件交付。

推荐判断方式：

```c
if (sdk_is_done(code) || sdk_is_reqid(code)) {
  /* request accepted */
}
```

常见分类：

| 分类 | 判断方式 | 含义 |
| --- | --- | --- |
| 成功 | `sdk_is_ok(code)` | 同步成功。 |
| 已受理 | `sdk_is_done(code)` 或 `sdk_is_reqid(code)` | 异步任务已进入 SDK。 |
| 警告 | `sdk_is_warn(code)` | 请求可能完成但存在降级或忙碌状态。 |
| 错误 | `sdk_is_error(code)` | 请求失败。 |

### 5.4 回调

| 回调 | 用途 | 建议 |
| --- | --- | --- |
| `sdk_result_cb_t` | 异步结果事件 | 在 `sdk_init` 前注册，回调内只做轻量处理或入队。 |
| `sdk_log_cb_t` | 本地/服务端视角日志 | 复制数据后异步处理，生产环境保持脱敏。 |
| `sdk_cookies_storage_cb_t` | Cookie 持久化拦截 | 需要替换 Cookie 时返回裸 Cookie JSON 数组。 |
| `sdk_security_decision_cb_t` | 安全策略拦截 | 可返回重定向 URL；不返回时使用 SDK 默认处理。 |

`sdk_result_cb_t` 的第一个 `code` 参数是粗粒度状态。事件路由应以 JSON body 的顶层 `type` 和 `reqId` 为准。

## 6. 快速接入：动态库

### 6.1 初始化顺序

1. 加载动态库。
2. 注册结果回调、日志回调和可选 Cookie/安全回调。
3. 准备初始化 JSON。
4. 调用 `sdk_init()`。
5. 保存返回的 SDK 句柄和本地 Web API 端口。
6. 打开环境并等待 `browser-open-success`。
7. 业务完成后关闭环境并等待 `browser-close-success`。
8. 调用 `sdk_shutdown()`。

### 6.2 初始化请求

最小请求：

```json
{
  "userSig": "YOUR_USER_SIG"
}
```

启用本地 Web API：

```json
{
  "userSig": "YOUR_USER_SIG",
  "workDir": "/absolute/path/to/workdir",
  "port": 0
}
```

常用字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | SDK 用户令牌。 |
| `workDir` | string | 否 | SDK 工作目录。生产环境建议显式传入。 |
| `extsDir` | string | 否 | 扩展资源目录。 |
| `port` | integer | 否 | 本地 Web API 端口。不传则不启用；`0` 表示自动分配。 |
| `sdkApiUrl` | string | 否 | SDK 后端地址覆盖，仅在交付方要求时配置。 |
| `debug` | bool | 否 | 开发调试开关。 |
| `verbose` | bool | 否 | 明文诊断开关。生产环境保持关闭。 |
| `autoUpdateKernel` | bool/int | 否 | 浏览器核心更新策略。 |

初始化成功后，响应中会包含 SDK 工作目录、应用信息、浏览器核心摘要，以及本地 Web API 端口等字段。具体字段以实际响应为准。

### 6.3 获取 `userSig`

如果接入方只持有 `apiKey`，可先调用 `sdk_get_user_sig()`：

```json
{
  "apiKey": "YOUR_API_KEY",
  "customerId": "optional-customer-id",
  "duration": 3600
}
```

该接口返回后端签发结果。请从响应中读取 `data.userSig`，再用于 `sdk_init()`。

### 6.4 打开环境

```json
{
  "envs": [
    {
      "envId": "2069040591861190656",
      "urls": ["https://example.com"]
    }
  ]
}
```

打开是异步操作。函数返回受理状态后，必须等待最终事件：

| 事件 | 含义 |
| --- | --- |
| `browser-open-success` | 环境已打开，浏览器自动化能力可用。 |
| `browser-open-failed` | 打开失败。 |
| `browser-open-timeout` | 打开超时。 |
| `browser-proxy-degraded` | 浏览器已尽力启动，但代理链路降级。 |

`browser-open-success` 是使用 MCP 单环境工具的前置条件。

### 6.5 关闭环境

```json
{
  "envs": ["2069040591861190656"]
}
```

关闭是异步操作。等待 `browser-close-success` 后再认为本地浏览器环境关闭完成。

### 6.6 停止 SDK

推荐顺序：

1. 对正在运行的环境调用 `sdk_browser_close()`。
2. 等待 `browser-close-success` 或明确失败事件。
3. 调用 `sdk_shutdown()`。

不要在回调线程中执行长耗时同步调用。

## 7. 本地 Web API

### 7.1 启用方式

初始化时传入 `port`：

```json
{
  "userSig": "YOUR_USER_SIG",
  "port": 0
}
```

| `port` | 行为 |
| --- | --- |
| 不传 | 不启用本地 Web API。 |
| `0` | SDK 自动选择本机可用端口。 |
| `>0` | SDK 尝试监听该端口。 |

本地服务只面向本机接入：

| 通道 | 地址 |
| --- | --- |
| HTTP | `http://127.0.0.1:{port}` |
| WebSocket | `ws://127.0.0.1:{port}/` |
| 全局 MCP | `http://127.0.0.1:{port}/sdk/v1/mcp` |
| 单环境 MCP | `http://127.0.0.1:{port}/sdk/v1/mcp/env/{envId}` |

### 7.2 Web API 路由

| 路由 | 模式 | 用途 |
| --- | --- | --- |
| `POST /sdk/v1/init` | 同步 | 初始化 SDK。 |
| `POST /sdk/v1/info` | 同步 | 查询 SDK 信息。 |
| `POST /sdk/v1/netdiag` | 同步 | 网络/代理诊断。 |
| `POST /sdk/v1/proxydiag` | 同步 | 系统代理诊断。 |
| `POST /sdk/v1/browser/info` | 同步 | 查询运行中浏览器环境。 |
| `POST /sdk/v1/browser/cleanup` | 同步 | 清理本地环境缓存或核心下载缓存。 |
| `POST /sdk/v1/browser/install` | 异步 | 安装或准备浏览器核心。 |
| `POST /sdk/v1/browser/open` | 异步 | 打开环境。 |
| `POST /sdk/v1/browser/close` | 异步 | 关闭环境。 |
| `POST /sdk/v1/token/update` | 异步 | 刷新 `userSig`。 |
| `POST /sdk/v1/env/create` | 同步 | 创建环境，后端 JSON 透传。 |
| `POST /sdk/v1/env/update` | 同步 | 更新环境，后端 JSON 透传。 |
| `POST /sdk/v1/env/page` | 同步 | 分页查询环境，后端 JSON 透传。 |
| `POST /sdk/v1/env/getinfo` | 同步 | 查询环境详情，后端 JSON 透传。 |
| `POST /sdk/v1/env/destroy` | 同步 | 销毁环境，后端 JSON 透传。 |

异步 Web API 的 HTTP 响应表示请求已受理，不表示浏览器最终完成。最终状态必须通过 WebSocket 事件确认。

### 7.3 WebSocket 事件

| 项目 | 说明 |
| --- | --- |
| URL | `ws://127.0.0.1:{port}/` |
| 数据格式 | UTF-8 JSON 文本。 |
| 用途 | 推送异步 SDK 结果，与 `sdk_result_cb_t` JSON body 对齐。 |
| 多客户端 | 已连接客户端都会收到广播。 |
| 断线重连 | SDK 不补发断线期间事件。 |

## 8. MCP 浏览器自动化

### 8.1 适用场景

MCP 适合以下接入方：

| 接入方 | 推荐方式 |
| --- | --- |
| Agent / LLM 自动化 | 连接单环境 MCP endpoint，使用 `tools/list` 发现工具。 |
| 浏览器扩展侧边栏 | 从本机页面或扩展访问 MCP endpoint，保存 `Mcp-Session-Id`。 |
| MCP Inspector / 调试工具 | 连接 `/sdk/v1/mcp` 或 `/sdk/v1/mcp/env/{envId}`。 |
| 桌面自动化控制台 | 用全局 MCP 管理环境，再进入单环境 MCP 操作页面。 |

MCP 客户端不需要也不应该直连浏览器调试端口。

### 8.2 入口

| 入口 | 状态 | 用途 |
| --- | --- | --- |
| `/sdk/v1/mcp` | 标准 | 全局管理 MCP。 |
| `/sdk/v1/mcp/health` | 标准 | 全局 MCP 健康检查。 |
| `/sdk/v1/mcp/env/{envId}` | 标准 | 单环境 MCP JSON-RPC 入口。 |
| `/sdk/v1/mcp/env/{envId}/health` | 标准 | 单环境 MCP 健康检查。 |
| `/sdk/v1/mcp/env/{envId}/artifacts/{artifactId}` | 标准 | artifact 分片读取。 |
| `/sdk/mcp` | 兼容 | 全局 MCP 兼容入口。 |
| `/sdk/mcp/env/{envId}` | 兼容 | 单环境 MCP 兼容入口。 |
| `/sdk/v1/mcp?envId={envId}` | 兼容 | 单环境 MCP query 形式。 |

新接入统一使用 `/sdk/v1/mcp` 和 `/sdk/v1/mcp/env/{envId}`。

### 8.3 五分钟接入流程

1. 初始化 BroSDK，并启用本地 Web API。
2. 打开一个环境，等待 `browser-open-success`。
3. 组合单环境 MCP endpoint：`http://127.0.0.1:{port}/sdk/v1/mcp/env/{envId}`。
4. 向该 endpoint 发送 JSON-RPC `initialize`。
5. 从响应头读取并保存 `Mcp-Session-Id`。
6. 后续 `tools/list` 和 `tools/call` 都发送到同一个 endpoint，并携带 `Mcp-Session-Id`。
7. 如果工具返回 artifact 元数据，使用 artifact 分片入口读取。
8. 工作结束后 DELETE 同一个 endpoint 关闭 MCP 会话。

`initialize`、`tools/list`、`tools/call` 是 JSON-RPC 方法，不是独立 HTTP 路径。不要请求 `/tools/list` 或 `/sdk/v1/mcp/env/{envId}/tools/list`。

### 8.4 `initialize`

请求：

```http
POST /sdk/v1/mcp/env/2069040591861190656 HTTP/1.1
Host: 127.0.0.1:35600
Content-Type: application/json
Accept: application/json

{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","clientInfo":{"name":"my-client","version":"1.0.0"}}}
```

成功响应会在响应头包含：

```http
Mcp-Session-Id: <session-id>
```

客户端必须保存该值。除 `initialize` 外，后续请求缺少 `Mcp-Session-Id` 会失败。

### 8.5 `tools/list`

```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/list",
  "params": {}
}
```

单环境 MCP 默认返回 16 个公开工具：

```text
tabs, tab_groups, navigate, snapshot, diff, act, download, upload, read, grep,
screenshot, pdf, wait, windows, evaluate, run
```

客户端应以 `tools/list` 返回结果为准，不要硬编码隐藏工具或内部工具。

### 8.6 `tools/call`

请求：

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "tools/call",
  "params": {
    "name": "navigate",
    "arguments": {
      "page": 1,
      "action": "url",
      "url": "https://example.com"
    }
  }
}
```

工具结果：

```json
{
  "content": [
    {
      "type": "text",
      "text": "{\"ok\":true}"
    }
  ],
  "structuredContent": {
    "ok": true
  },
  "isError": false
}
```

接入方应优先读取 `structuredContent`，并将 `content[].text` 作为面向模型或人的摘要。

### 8.7 全局 MCP 工具

`/sdk/v1/mcp` 默认暴露环境和生命周期管理工具：

```text
sdk.health, sdk.info,
env.list, env.resolve, env.get, env.create, env.update, env.destroy,
browser.open, browser.close, browser.cleanup, browser.status, browser.install,
task.list, task.get, mcp.endpoint
```

典型流程：

1. `env.list` 或 `env.resolve` 找到环境。
2. `browser.open` 打开环境。
3. `task.get` 或 WebSocket 等待打开完成。
4. `mcp.endpoint` 获取单环境 MCP endpoint。
5. 使用单环境 MCP 执行浏览器自动化。

`env.destroy` 需要显式确认参数。不要把销毁环境作为普通关闭浏览器动作。

### 8.8 单环境 MCP 工具

| 工具 | 用途 | 典型输入 |
| --- | --- | --- |
| `tabs` | 列出、激活、新建、关闭页面 | `action`, `page`, `url` |
| `tab_groups` | 标签组能力 | `action`, `pages`, `group` |
| `windows` | 窗口能力 | `action` |
| `navigate` | 导航、前进、后退、刷新 | `page`, `action`, `url` |
| `snapshot` | 获取可操作页面快照 | `page` |
| `diff` | 查看相对上次快照的变化 | `page` |
| `act` | 点击、填写、按键、滚动等动作 | `page`, `kind`, `ref`, `value` |
| `wait` | 等待文本、选择器或时间 | `page`, `for`, `value`, `timeout` |
| `read` | 提取页面文本、链接或 Markdown | `page`, `format` |
| `grep` | 在页面内容中搜索 | `page`, `pattern`, `limit` |
| `screenshot` | 截图 | `page`, `fullPage`, `format` |
| `pdf` | 输出 PDF artifact | `page` |
| `upload` | 给文件输入框设置本地文件 | `page`, `ref`, `file` 或 `files` |
| `download` | 触发并捕获浏览器下载 | `page`, `ref` |
| `evaluate` | 在页面内执行有界 JS | `page`, `code`, `timeout` |
| `run` | 执行受限页面自动化脚本 | `code`, `timeout` |

平台不支持某个能力时，工具返回结构化失败，例如 `UNSUPPORTED_CAPABILITY`，不会伪装成功。

### 8.9 快照 ref

`snapshot` 会返回页面文本和可操作 ref。`act`、`download`、`upload` 等工具可以使用这些 ref。

规则：

| 规则 | 说明 |
| --- | --- |
| ref 是不透明句柄 | 不要解析或拼接 ref。 |
| ref 属于会话和环境 | 不可跨 MCP 会话或跨环境复用。 |
| 导航可能让 ref 过期 | 导航、刷新或页面重绘后重新 `snapshot`。 |
| ref 找不到时 | 工具返回结构化失败，客户端应重新观察页面。 |

### 8.10 Artifact

大结果会以 artifact 元数据返回：

```json
{
  "artifact": {
    "artifactId": "artifact_...",
    "uri": "brosdk://env/{envId}/artifacts/{artifactId}",
    "mimeType": "text/html",
    "size": 10485760,
    "sha256": "...",
    "chunkSize": 1048576
  }
}
```

读取分片：

```http
GET /sdk/v1/mcp/env/{envId}/artifacts/{artifactId}?offset=0&limit=1048576 HTTP/1.1
Host: 127.0.0.1:{port}
Mcp-Session-Id: <session-id>
```

artifact 规则：

| 规则 | 说明 |
| --- | --- |
| 会话归属 | 只有创建它的同一 MCP 会话可读取。 |
| 环境归属 | 不能跨环境读取。 |
| 分片读取 | 客户端按 `offset`/`limit` 循环读取直到 EOF。 |
| 大结果 | 不要把页面 HTML、截图、PDF、大文件塞进 JSON-RPC 控制面。 |

### 8.11 会话关闭

```http
DELETE /sdk/v1/mcp/env/{envId} HTTP/1.1
Host: 127.0.0.1:{port}
Mcp-Session-Id: <session-id>
```

关闭会话会清理该会话下的页面 ref 和 artifact。关闭 MCP 会话不等于关闭浏览器环境；需要关闭环境时仍调用 `browser.close` 或 `sdk_browser_close()`。

### 8.12 本机访问与浏览器 CORS

v2.0.0.3 标准包面向本机自动化场景，允许本机 CLI、桌面 Agent、MCP Inspector、本机 Web 调试页面和浏览器扩展侧边栏访问 MCP HTTP 入口。

公开安全边界：

| 边界 | 规则 |
| --- | --- |
| 本机访问 | MCP 面向本机进程和本机浏览器。 |
| 远程访问 | 默认不作为公开能力。不要把端口暴露给局域网或公网。 |
| 会话头 | 除 `initialize` 外必须携带 `Mcp-Session-Id`。 |
| 环境隔离 | 一个 MCP 会话只属于一个环境。 |
| artifact/ref 隔离 | 不可跨会话或跨环境读取。 |
| 敏感数据 | 不要在提示词、日志或公开报告中输出 API key、Cookie、代理凭据、令牌。 |

如果你拿到的是企业定制或严格 Origin 白名单构建，请按交付方提供的部署配置填写允许来源。

### 8.13 扩展侧边栏接入

浏览器扩展侧边栏只需要配置单环境 MCP endpoint：

```text
http://127.0.0.1:{port}/sdk/v1/mcp/env/{envId}
```

扩展侧可以用自己的自然语言解析、命令规划或 UI 工作流调用 MCP。扩展内部如何组织消息、状态或桥接能力不属于 SDK 公开协议，接入方只需要遵循 MCP JSON-RPC 流程。

## 9. 环境管理

### 9.1 创建、更新、查询、销毁

环境接口为同步后端 JSON 透传：

| 动态库函数 | Web API | 用途 |
| --- | --- | --- |
| `sdk_env_create` | `/sdk/v1/env/create` | 创建环境。 |
| `sdk_env_update` | `/sdk/v1/env/update` | 更新环境。 |
| `sdk_env_page` | `/sdk/v1/env/page` | 分页查询环境。 |
| `sdk_env_getinfo` | `/sdk/v1/env/getinfo` | 查询环境详情。 |
| `sdk_env_destroy` | `/sdk/v1/env/destroy` | 销毁环境。 |

这些接口的请求和响应字段由后端环境 API 契约决定。SDK 不在公开文档中重新定义完整业务字段。

### 9.2 生命周期建议

| 操作 | 建议 |
| --- | --- |
| 打开环境 | 使用 `sdk_browser_open`、Web API `browser/open` 或全局 MCP `browser.open`。 |
| 页面自动化 | 环境打开成功后使用单环境 MCP。 |
| 关闭环境 | 使用 `sdk_browser_close`、Web API `browser/close` 或全局 MCP `browser.close`。 |
| 销毁环境 | 先关闭浏览器，再调用 `env.destroy`。 |
| 清理本地缓存 | 使用 `browser.cleanup`，不会销毁后端环境记录。 |

## 10. Cookie 与 Storage

### 10.1 托管模式

SDK 会按环境配置管理浏览器数据。初始化响应和 `sdk_info()` 可用于查看当前数据托管状态。

公开语义：

| 状态 | 说明 |
| --- | --- |
| 半托管 | 以本地浏览器数据为主。 |
| 全托管 | 本地缓存与远端数据协同。 |

### 10.2 Cookie 回调

`sdk_register_cookies_storage_cb()` 用于 Cookie 持久化拦截。

| 行为 | 规则 |
| --- | --- |
| 不修改 | 保持 `*new_data == NULL` 且 `*new_len == 0`。 |
| 替换 | 返回完整裸 Cookie JSON 数组。 |
| 分配 | 替换数据必须由 `sdk_malloc()` 分配。 |
| 处理 | 回调中避免长耗时操作。 |

Storage 不通过 Cookie 回调替换。需要管理 Storage 时，应使用环境数据托管能力。

### 10.3 关闭事件与数据同步

`browser-close-success` 表示本地关闭链路完成，不应被理解为所有远端数据上传都已经完成。需要追踪远端同步状态时，通过日志和交付方运维视图确认。

## 11. 代理、网络与安全策略

### 11.1 代理配置原则

稳定业务链路应显式配置环境代理，或在打开请求中传入单次代理配置。系统代理主要用于诊断和策略输入，不建议作为生产依赖。

常见字段：

| 字段 | 说明 |
| --- | --- |
| `proxy` | 环境默认代理。 |
| `forward` | 单次打开的上游代理。 |
| `bridge` / `bridgeProxy` | 可选前置代理或跳板代理。 |

### 11.2 网络诊断

| 函数/路由 | 用途 |
| --- | --- |
| `sdk_network_diagnostics` / `/sdk/v1/netdiag` | 诊断指定 URL 与代理链路。 |
| `sdk_system_proxy_diagnostics` / `/sdk/v1/proxydiag` | 查询系统代理摘要。 |

示例：

```json
{
  "url": "https://example.com",
  "proxy": "",
  "bridgeProxy": ""
}
```

### 11.3 安全策略回调

`sdk_register_security_decision_cb()` 可在 SDK 安全策略拦截请求时让宿主返回重定向 URL。

| 行为 | 规则 |
| --- | --- |
| 使用 SDK 默认拦截 | 返回 `NULL/0`。 |
| 重定向 | 用 `sdk_malloc()` 分配 URL 字符串并返回长度。 |
| MCP 工具 | 不绕过安全策略回调。 |

## 12. 日志、脱敏与排障

### 12.1 日志通道

| 通道 | 用途 |
| --- | --- |
| `sdk_log_cb_t` 类型 `SDK_LOG_TYPE_LOCAL` | 本地 SDK 日志。 |
| `sdk_log_cb_t` 类型 `SDK_LOG_TYPE_SERVER` | 服务端视角结构化日志。 |
| `sdk_result_cb_t` / WebSocket | 业务异步结果事件。 |
| MCP 诊断资源 | MCP 会话、工具调用、artifact 和页面观察状态摘要。 |

### 12.2 脱敏规则

生产环境保持默认脱敏。以下内容不得写入公开日志、提示词或报告：

| 敏感内容 |
| --- |
| API key |
| `userSig` / token |
| Cookie / Storage 明文 |
| 代理用户名、密码、完整代理 URL |
| 页面原始大体积转储 |
| artifact 原始字节 |

`verbose=true` 仅用于短时诊断，诊断结束后关闭。

### 12.3 常见排障路径

| 问题 | 检查 |
| --- | --- |
| 初始化失败 | `userSig` 是否有效，`workDir` 是否可写，SDK 后端地址是否正确。 |
| 本地 Web API 不可访问 | 初始化是否传入 `port`，端口是否被占用，调用方是否在本机。 |
| 打开环境失败 | 等待最终事件，查看代理诊断和环境配置。 |
| MCP 401 | 除 `initialize` 外缺少 `Mcp-Session-Id`。 |
| MCP 403 | 会话不属于该环境，或访问来源不被当前构建允许。 |
| MCP 503 | 工具调用过多，稍后重试或降低并发。 |
| ref 找不到 | 页面已导航或重绘，重新 `snapshot`。 |
| 大结果不完整 | 使用 artifact 分片读取并校验总字节数。 |

## 13. 动态库公开函数

### 13.1 回调注册

| 函数 | 说明 |
| --- | --- |
| `sdk_register_result_cb` | 注册异步结果回调。 |
| `sdk_register_log_cb` | 注册日志回调。 |
| `sdk_register_cookies_storage_cb` | 注册 Cookie 拦截回调。 |
| `sdk_register_security_decision_cb` | 注册安全策略回调。 |

### 13.2 SDK 生命周期与信息

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_init` | 同步 | 初始化 SDK。 |
| `sdk_init_async` | 异步 | 异步初始化 SDK。 |
| `sdk_init_cpp` | 同步 | 获取当前 SDK 单例句柄。 |
| `sdk_init_webapi` | 兼容 | 仅启用本地 Web API 端口；新接入优先在 `sdk_init` 中传 `port`。 |
| `sdk_info` | 同步 | 读取 SDK 信息。 |
| `sdk_get_user_sig` | 同步 | 使用 `apiKey` 换取 `userSig`。 |
| `sdk_token_update` | 异步 | 刷新 `userSig`。 |
| `sdk_shutdown` | 同步 | 停止 SDK 单例。 |

### 13.3 浏览器函数

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_browser_install` | 异步 | 安装或准备浏览器核心。 |
| `sdk_browser_info` | 同步 | 查询当前运行中浏览器环境。 |
| `sdk_browser_open` | 异步 | 打开一个或多个环境。 |
| `sdk_browser_close` | 异步 | 关闭一个或多个环境。 |
| `sdk_browser_cleanup` | 同步 | 清理本地缓存。 |
| `sdk_browser_env_check` | 同步 | 打开内置环境检查页。 |
| `sdk_browser_snapshot` | 同步 | 获取页面快照、HTML、截图等诊断数据。 |
| `sdk_browser_command` | 同步 | 高级诊断接口。常规 Agent 自动化优先使用 MCP 工具。 |

### 13.4 环境函数

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_env_create` | 同步 | 创建环境。 |
| `sdk_env_update` | 同步 | 更新环境。 |
| `sdk_env_page` | 同步 | 分页查询环境。 |
| `sdk_env_getinfo` | 同步 | 查询环境详情。 |
| `sdk_env_destroy` | 同步 | 销毁环境。 |

`sdk_env_get_cookies` 是历史遗留声明，不作为 v2.0.0.3 稳定接入入口。

### 13.5 辅助函数

| 函数 | 说明 |
| --- | --- |
| `sdk_malloc` | 分配 SDK 回调替换缓冲区。 |
| `sdk_free` | 释放 SDK 返回缓冲区。 |
| `sdk_error_name` / `sdk_error_string` | 错误名称和说明。 |
| `sdk_event_name` | 事件名称。 |
| `sdk_is_error` / `sdk_is_warn` / `sdk_is_reqid` / `sdk_is_ok` / `sdk_is_done` / `sdk_is_event` | 返回码分类。 |

### 13.6 C++ `ISDK`

C++ 接入方可通过 `sdk_init()` 返回的 `sdk_handle_t` 或 `sdk_init_cpp()` 获取 `ISDK*`。`ISDK` 方法与 C ABI 语义对齐；经 `char **out` 返回的缓冲区同样用 `sdk_free()` 释放。

## 14. 事件与错误

### 14.1 常见事件

| 事件 | 说明 |
| --- | --- |
| `sdk-init-success` / `sdk-init-failed` | SDK 初始化结果。 |
| `sdk-token-update-success` / `sdk-token-update-failed` | 令牌刷新结果。 |
| `sdk-token-expire-warning` / `sdk-token-expired` | 令牌有效期提醒或过期。 |
| `browser-install-success` / `browser-install-failed` | 浏览器核心准备结果。 |
| `browser-open-success` / `browser-open-failed` / `browser-open-timeout` | 环境打开结果。 |
| `browser-close-success` / `browser-close-failed` | 环境关闭结果。 |
| `browser-proxy-degraded` | 代理链路降级。 |
| `browser-cookie-update-cb` | Cookie 持久化回调事件。 |

### 14.2 常见 MCP 错误

| 错误 | 说明 | 建议 |
| --- | --- | --- |
| `INVALID_PARAMS` | 请求或工具参数非法 | 按 `tools/list` schema 修正参数。 |
| `INVALID_SESSION` | 会话缺失、过期或不匹配 | 重新 `initialize`。 |
| `BROWSER_NOT_READY` | 目标环境或页面未就绪 | 等待 `browser-open-success` 后重试。 |
| `MCP_QUEUE_FULL` | 工具队列已满 | 降低并发或稍后重试。 |
| `TIMEOUT` | 工具超时 | 缩小任务或增加业务层等待。 |
| `REF_NOT_FOUND` | ref 失效或不存在 | 重新 `snapshot`。 |
| `NO_PREVIOUS_SNAPSHOT` | 没有前序快照 | 先调用 `snapshot`。 |
| `UNSUPPORTED_CAPABILITY` | 当前平台不支持该工具能力 | 使用替代工具或在 UI 中提示不可用。 |
| `ARTIFACT_READ_FAILED` | artifact 不可读或不属于该会话 | 检查 `artifactId`、`envId` 和 `Mcp-Session-Id`。 |

完整错误文本请用 `sdk_error_name()`、`sdk_error_string()` 或 MCP 响应体中的结构化错误读取。

## 15. 开发者接入检查清单

### 15.1 动态库 / Web API

| 检查项 | 要求 |
| --- | --- |
| 头文件 | 使用随包 `brosdk.h`。 |
| 回调 | 异步调用前注册 `sdk_result_cb_t`。 |
| 初始化 | 提供有效 `userSig`，生产环境显式设置 `workDir`。 |
| 本地 Web API | 需要 HTTP/MCP 时在初始化 JSON 中传 `port`。 |
| 环境 ID | 全链路按字符串处理。 |
| 异步完成 | 等待最终事件，不把 ACK 当完成。 |
| 内存 | SDK 输出用 `sdk_free()` 释放。 |
| 关闭 | 先关闭环境，再 `sdk_shutdown()`。 |

### 15.2 MCP

| 检查项 | 要求 |
| --- | --- |
| 环境状态 | 单环境 MCP 调用前环境已打开。 |
| endpoint | 新接入使用 `/sdk/v1/mcp/env/{envId}`。 |
| 初始化 | 先调用 JSON-RPC `initialize`。 |
| 会话头 | 后续每个请求带 `Mcp-Session-Id`。 |
| 工具发现 | 使用 `tools/list`，不要猜测隐藏工具。 |
| 工具参数 | 按 schema 传参，`envId` 不要当 number。 |
| 大结果 | 使用 artifact 分片读取。 |
| 清理 | 工作流结束后 DELETE MCP 会话。 |
| 安全 | 端口只给本机可信客户端使用。 |

### 15.3 发布前自验

| 项目 | 通过标准 |
| --- | --- |
| SDK 初始化 | `sdk-init-success`。 |
| 环境打开 | `browser-open-success`，且 `envId` 与请求一致。 |
| WebSocket | 能收到打开和关闭最终事件。 |
| 全局 MCP | `initialize` 后 `tools/list` 包含管理工具。 |
| 单环境 MCP | `tools/list` 返回 16 个公开工具。 |
| 页面自动化 | `navigate`、`snapshot`、`act`、`read` 至少完成一条业务链。 |
| artifact | 大结果能分片读完，并校验总字节数。 |
| 日志 | 无 API key、token、Cookie、代理凭据明文。 |
| 清理 | MCP 会话关闭，浏览器环境关闭。 |

## 16. 发行验证摘要

v2.0.0.3 发布验证覆盖以下公开行为：

| 类别 | 验证内容 |
| --- | --- |
| 既有 SDK | 初始化、环境管理、浏览器打开/关闭、回调/WebSocket、日志脱敏。 |
| MCP 协议 | `initialize`、`tools/list`、`tools/call`、会话关闭、会话隔离。 |
| 单环境工具 | BrowserOS 对齐 16 工具的公开 schema 和基础工作流。 |
| 全局管理 | 环境查询、生命周期管理、任务查询、单环境 endpoint 返回。 |
| 大结果 | artifact 分片读写和哈希/字节校验。 |
| 并发与清理 | 工具调用完成后队列和执行中计数回落，无残留浏览器进程。 |
| 敏感数据 | 公开报告和服务端视角日志不包含明文凭据。 |

公开发布资料只应包含计数、状态、字节数和哈希等摘要证据，不应包含 API key、Cookie、代理凭据、令牌、页面原始转储或 artifact 原始字节。
