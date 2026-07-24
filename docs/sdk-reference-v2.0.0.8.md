# BroSDK v2.0.0.8 接入参考文档

> 当前版本：v2.0.0.8  
> 最后更新：2026-07-24  
> 适用对象：桌面客户端、自动化宿主、本机 Web 工具、浏览器扩展和 Agent/MCP 客户端。  
> 文档边界：本文只描述公开接口、输入输出和稳定行为。动态库符号、C/C++ 声明及返回码以发布包内的 `brosdk.h` 为最终依据。

## 1. 概述

BroSDK 是用于管理浏览器环境的本地运行时 SDK。它提供 SDK 初始化、环境管理、浏览器生命周期、Cookie/Storage 管理、网络诊断、本地 Web API、WebSocket 事件和 MCP 浏览器自动化能力。

### 1.1 接入方式

| 接入方式 | 入口 | 适用场景 |
| --- | --- | --- |
| C ABI / C++ `ISDK` | `brosdk.h` | 原生客户端、长期驻留宿主、需要直接回调的集成。 |
| 本地 Web API + WebSocket | `http://127.0.0.1:{port}/sdk/v1/...`、`ws://127.0.0.1:{port}/` | 跨语言宿主、桌面工具和本机控制台。 |
| MCP Streamable HTTP | `http://127.0.0.1:{port}/sdk/v1/mcp...` | Agent、MCP 客户端和浏览器扩展。 |

相应入口启用并完成 SDK 初始化后，三种方式可以独立调用，也可以组合使用，并观察同一组环境状态。C/C++ 异步操作通过结果回调收敛，Web API 通过 WebSocket 收敛；全局 MCP 的 `browser.open`/`browser.close` 则通过 `task.get` 收敛，不要求 MCP 客户端接入 SDK 回调或 WebSocket。

### 1.2 v2.0.0.8 公开变更

| 变更 | 接入影响 |
| --- | --- |
| 新增 Cookie 历史、本地快照、远端快照和健康检查接口 | 新增六个同步 C ABI，并在 C++ `ISDK` 中提供对应方法。 |
| 重复打开已运行环境改为幂等成功 | SDK 不重复启动浏览器；尝试激活现有窗口，并返回 `browser-open-success` 与 `CL_WBRWALREADYRUNNING`。 |
| Web API 增加 Cookie 历史查询 | 新增 `POST /sdk/v1/env/getcookiehistory`。 |
| MCP 协议与公开工具面更新 | 初始化示例使用 `2025-11-25`；客户端必须通过 `tools/list` 发现当前工具。 |

已有接入方可以继续使用原有 C ABI、C++、Web API 和 WebSocket 流程。使用 v2.0.0.8 新接口时，应同时更新动态库和 `brosdk.h`，不要混用不同版本的头文件与运行库。

## 2. 平台与发布包

| 平台 | 架构 | 动态库 |
| --- | --- | --- |
| Windows | x64 | `brosdk.dll` |
| Linux | x64 | `brosdk.so` |
| macOS | arm64 / x64 | `brosdk.dylib` |

发布包中的公共接入文件：

| 文件 | 用途 |
| --- | --- |
| `brosdk.h` | C ABI、回调类型和 C++ `ISDK`。 |
| 平台动态库 | SDK 运行时。 |
| 随包资源 | 由 SDK 按发布包约定管理。 |

接入方不得依赖未公开的目录布局、进程参数、诊断开关或实现组件。需要配置的能力以本文档和随包头文件为准。

## 3. 通用契约

### 3.1 JSON、编码与标识符

- 请求体、响应体和事件体均为 UTF-8 JSON。
- C/C++ 接口的 `len` 是字节长度，不包含末尾 `\0`。
- `envId` 必须作为十进制字符串传入、存储、比较和转发。
- 不得将 `envId` 转为 JavaScript `number`；环境 ID 可能超过 `Number.MAX_SAFE_INTEGER`。

推荐写法：

```json
{
  "envId": "1234567890123456789"
}
```

SDK 生成的生命周期事件使用字符串 `envId`。如果某个业务响应仍包含数值形式的标识符，接入方应在接收边界立即规范化为十进制字符串，不得让该值进入浮点数运算。

### 3.2 内存所有权

| 场景 | 规则 |
| --- | --- |
| `char **out_data` / `char **out` | 每次调用前设为 `NULL`，同时将 `out_len` 设为 `0`；仅在调用成功后按 `out_len` 读取，不假设以 `\0` 结尾；调用后若指针非空，必须且只能用 `sdk_free()` 释放一次。 |
| `sdk_result_cb_t` / `sdk_log_cb_t` 的 `data` | 只在当前回调期间有效；需要保留时立即按 `len` 复制。 |
| Cookie 回调返回 `new_data` | 必须用 `sdk_malloc()` 分配；返回后所有权交给 SDK。 |
| 安全回调返回 `redirect` | 必须用 `sdk_malloc()` 分配；返回后所有权交给 SDK。 |
| `sdk_error_name()`、`sdk_error_string()`、`sdk_event_name()` | 返回借用的静态字符串，不得释放。 |
| `sdk_handle_t` | 不透明句柄，不得由接入方释放。 |

输出参数必须先初始化，避免在参数校验失败等早退路径中读取或释放未初始化地址。调用失败时不得读取输出内容；若 SDK 仍返回了非空指针，仍应调用 `sdk_free()`。

### 3.3 同步、异步与终态

同步接口在函数返回时完成，并可通过 `out_data/out_len` 返回结果。

异步接口的直接返回只表示是否受理，不表示业务完成。

C ABI 在成功受理时通常返回 `CL_DONE`；为兼容直接返回请求 ID 的调用路径，使用以下判断：

```c
if (sdk_is_done(code) || sdk_is_reqid(code)) {
  /* accepted; wait for the terminal event */
}
```

Web API 使用 HTTP ACK 表示受理，ACK 的顶层 `code` 与 C ABI 的直接返回码不是同一层语义，详见 7.3。不要把直接返回值或 HTTP ACK 当作最终结果，也不要把它直接当作业务 `reqId`。C/C++ 与 Web API 的最终结果分别以结果回调或 WebSocket 的 JSON body 为准。

生命周期事件字段节选：

```json
{
  "code": 0,
  "reqId": 100042,
  "type": "browser-open-success",
  "msg": "ok",
  "data": {
    "envId": "1234567890123456789",
    "eventId": 20111,
    "status": 2,
    "statusName": "Started",
    "progress": 100
  },
  "envList": [
    {
      "envId": "1234567890123456789",
      "status": 2,
      "statusName": "Started",
      "progress": 100
    }
  ]
}
```

| 字段 | 含义 |
| --- | --- |
| `code` | 本次事件的状态码。警告码不必然表示失败。 |
| `reqId` | 用于关联异步请求；以 JSON body 为准。 |
| `type` | 稳定的业务事件名，应作为首要路由字段。 |
| `msg` | 面向诊断的简短说明，不建议用于业务分支。 |
| `data` | 事件数据；浏览器批量操作还可能带有顶层 `envList`。 |

终态判断顺序：

1. 先按 `type` 判断成功、失败或超时。
2. 再读取 `code` 区分正常成功与带警告的成功。
3. 使用顶层 `reqId` 关联请求。
4. 将 `data.eventId` 作为事件枚举补充信息。

### 3.4 回调

| 回调 | 用途 |
| --- | --- |
| `sdk_result_cb_t` | SDK 初始化、安装、打开、关闭和令牌更新等异步结果。 |
| `sdk_log_cb_t` | 本地日志与服务端视角结构化日志。 |
| `sdk_cookies_storage_cb_t` | Cookie 持久化前的检查或替换。 |
| `sdk_security_decision_cb_t` | 安全策略命中后的可选重定向决策。 |

必须在发起相应异步操作前注册结果回调；需要接收初始化结果时，应在 `sdk_init()` 或 `sdk_init_async()` 前注册。

`sdk_log_cb_t` 的 `SDK_LOG_TYPE_LOCAL` 表示本地日志文本，`SDK_LOG_TYPE_SERVER` 表示结构化日志 JSON。两类回调数据都只在当前回调期间有效。

回调中只执行复制、轻量校验或投递到宿主执行器。不要阻塞，不要等待其他 SDK 事件，也不要在回调中重新进入同步或阻塞式 SDK 接口。

## 4. 动态库快速接入

### 4.1 推荐顺序

1. 加载平台动态库并使用同版本 `brosdk.h`。
2. 注册 `sdk_result_cb_t`，按需注册日志、Cookie 和安全回调。
3. 准备初始化 JSON，调用 `sdk_init()` 或 `sdk_init_async()`。
4. 初始化成功后调用环境管理或浏览器接口。
5. 打开环境后等待 `browser-open-success`。
6. 业务结束后关闭环境并等待每个环境的关闭终态。
7. 所有环境完成关闭后调用 `sdk_shutdown()`。

### 4.2 初始化

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

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | SDK 用户令牌。 |
| `workDir` | string | 否 | SDK 工作目录；生产环境建议显式传入可写的绝对路径。 |
| `port` | integer | 否 | 本地 Web API 端口；不传表示不启用，`0` 表示自动选择可用端口。 |
| `sdkApiUrl` | string | 否 | 服务地址覆盖；仅在交付方明确要求时设置。 |
| `extsDir` | string | 否 | 扩展资源目录；仅在交付约定要求宿主提供时设置。 |
| `autoUpdateKernel` | bool | 否 | 浏览器核心自动更新策略覆盖；未明确要求时不要设置。 |
| `logoPath` | string | 否 | 自定义图标文件的绝对路径。 |
| `debug` | bool | 否 | 调试模式；生产环境通常保持关闭。 |
| `verbose` | bool | 否 | 详细诊断模式；只在排障期间短时开启，生产环境保持关闭。 |

未在公开文档中定义的初始化字段，只应按交付配置使用。

`sdk_init()` 会同步等待初始化完成，并通过 `out_data/out_len` 返回结果；`sdk_init_async()` 只负责受理，终态通过 `sdk_result_cb_t` 返回。同步初始化的最小 C 示例：

```c
#include "brosdk.h"

sdk_handle_t sdk = NULL;
char *out = NULL;
size_t out_len = 0;
const char request[] = "{\"userSig\":\"YOUR_USER_SIG\"}";

int32_t code = sdk_init(&sdk, request, sizeof(request) - 1, &out, &out_len);
if (sdk_is_ok(code) && out != NULL) {
  /* Parse exactly out_len bytes as UTF-8 JSON. */
}
if (out != NULL) {
  sdk_free(out);
}
```

当 `port=0` 时，实际监听端口由 SDK 选择，并只在初始化成功结果的 `data.port` 返回；`sdk_info()` 不提供该端口。接入方必须保存此值，不能继续使用请求值 `0`。

初始化成功事件字段节选：

```json
{
  "type": "sdk-init-success",
  "data": {
    "port": 54321
  }
}
```

如果接入方只有 `apiKey`，可先同步调用 `sdk_get_user_sig()`：

```json
{
  "apiKey": "YOUR_API_KEY",
  "customerId": "optional-customer-id",
  "duration": 3600
}
```

成功后从响应的 `data.userSig` 读取令牌，再执行初始化。API key 和 `userSig` 均为敏感信息，不得写入普通日志或前端页面。

### 4.3 打开环境

`sdk_browser_open()` 是异步接口。推荐请求：

```json
{
  "envs": [
    {
      "envId": "1234567890123456789",
      "urls": ["https://example.com"]
    }
  ]
}
```

| 事件 | 终态 | 含义 |
| --- | --- | --- |
| `browser-open-success` | 是 | 环境已就绪，可进行后续自动化。 |
| `browser-open-failed` | 是 | 环境打开失败。 |
| `browser-open-timeout` | 是 | 环境未在规定时间内就绪。 |
| `browser-proxy-degraded` | 否 | 代理能力降级提示；继续等待打开终态。 |

单个环境对象常用字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `envId` | string | 必填。环境 ID，始终按十进制字符串处理。 |
| `urls` | string[] | 可选。环境就绪后打开的初始 URL。 |
| `args` | string[] | 可选。交付方允许的附加启动参数。 |
| `forward` | string | 可选。交付配置定义的转发链路。 |
| `bridge` | string | 可选。本次打开的桥接代理覆盖值；存在时优先于环境已有的 `bridgeProxy`。 |
| `extensions` | object[] | 可选。向初始化阶段已加载的扩展传递本次启动数据；元素形状为 `{"id":"...","data":{"key":"value"}}`。 |
| `cookies` | object[] | 可选。随本次打开提供的完整 Cookie 数组。 |
| `yunConfig` | object | 可选。仅按交付方提供的配置契约传入。 |

`proxy` 和 `bridgeProxy` 是环境配置，不是本次打开的覆盖字段；需要修改时，先通过环境创建或更新接口保存，再调用打开。不要同时用 `args` 中的代理开关和结构化代理字段表达相互冲突的配置。代理凭据属于敏感信息，不得写入普通日志。

`extensions` 不安装新扩展，只按 `id` 匹配初始化时从 `extsDir` 加载的扩展，并传递字符串键值形式的 `data`；未匹配的 `id` 会被忽略。

#### 重复打开已运行环境

同一环境已经处于运行状态时，再次调用打开接口是幂等成功：

- 不启动第二个浏览器实例。
- SDK 尝试激活并显示现有窗口。
- 最终事件仍是 `browser-open-success`，不是 `browser-open-failed`。
- `code` 为 `CL_WBRWALREADYRUNNING (107)`。
- `data.alreadyRunning` 为 `true`。

终态字段节选：

```json
{
  "code": 107,
  "reqId": 100043,
  "type": "browser-open-success",
  "msg": "browser is already running",
  "data": {
    "envId": "1234567890123456789",
    "status": 2,
    "statusName": "Started",
    "progress": 100,
    "alreadyRunning": true,
    "eventId": 20111
  }
}
```

接入方不得使用 `code != 0` 直接判定打开失败。`browser-open-success` 携带警告码时，仍属于成功终态。

### 4.4 关闭环境

`sdk_browser_close()` 是异步接口：

```json
{
  "envs": ["1234567890123456789"]
}
```

等待 `browser-close-success`、`browser-close-failed` 或 `browser-close-timeout`。批量关闭时，应按 `envId` 分别收敛每个环境，不能用第一条事件代表整批完成。

`browser-close-success` 表示浏览器已退出，并且 SDK 已完成当前关闭流程要求的本地数据持久化。启用远端数据托管时，后续远端同步可以继续进行，不是关闭成功事件的前置条件。

### 4.5 停止 SDK

`sdk_shutdown()` 是同步接口。推荐在以下条件同时满足后调用：

- 不再提交新任务。
- 所有已受理的打开、关闭和安装请求已收到终态。
- 所有需要关闭的环境已完成关闭。
- 宿主不再使用 SDK 返回的句柄或回调数据。

## 5. Cookie 与 Storage

Cookie 接口的输入输出边界使用普通 JSON。Cookie value 属于敏感数据；不要记录完整请求、响应或回调内容。

### 5.1 v2.0.0.8 Cookie 接口

| C ABI | C++ `ISDK` | 是否需要初始化 | 浏览器可停止 | 结果 |
| --- | --- | --- | --- | --- |
| `sdk_get_cookies_history` | `GetCookieHistory` | 是 | 是 | Cookie 历史响应。 |
| `sdk_get_cookies_local` | `GetCookiesLocal` | 是 | 是 | 最新本地 Cookie 数组。 |
| `sdk_set_cookies_local` | `SetCookiesLocal` | 是 | 是 | 本地写入摘要。 |
| `sdk_get_cookies_remote` | `GetCookiesRemote` | 是 | 是 | 指定历史节点的 Cookie 数组。 |
| `sdk_set_cookies_remote` | `SetCookiesRemote` | 是 | 是 | 远端更新结果。 |
| `sdk_cookies_health_check` | `CheckCookiesHealth` | 否 | 是 | 按 domain 汇总的健康报告。 |

六个接口均为同步接口。返回的非空 `out_data` 必须用 `sdk_free()` 释放。

### 5.2 查询 Cookie 历史

请求：

```json
{
  "envId": "1234567890123456789"
}
```

`sdk_get_cookies_history()` 返回 Cookie 历史列表 JSON。`data[]` 中常用字段如下：

| 字段 | 用途 |
| --- | --- |
| `fileUrl` | 历史节点的不透明引用，原样传给 `sdk_get_cookies_remote()`。 |
| `md5` | 可选完整性校验值；存在时可随远端读取请求传入。 |
| `updatedAt` | 历史节点更新时间；存在时可用于 UI 排序和展示。 |

`fileUrl` 是远端读取的必填输入，`md5` 是可选校验输入。不要解析、拼接或自行构造 `fileUrl`；除这两个输入外，不要让展示字段成为读取流程的强依赖。

本地 Web API 对应路由：

```http
POST /sdk/v1/env/getcookiehistory
Content-Type: application/json
```

### 5.3 读取和设置本地 Cookie

读取：

```json
{
  "envId": "1234567890123456789"
}
```

成功响应是 Cookie JSON 数组：

```json
[
  {
    "name": "sessionid",
    "value": "***",
    "domain": ".example.com",
    "path": "/"
  }
]
```

没有本地快照时返回 `CL_ENOTFOUND`。该接口读取最近一次已持久化快照，不读取运行中浏览器的即时状态。

设置：

```json
{
  "envId": "1234567890123456789",
  "cookies": [
    {
      "name": "sessionid",
      "value": "***",
      "domain": ".example.com",
      "path": "/"
    }
  ]
}
```

`sdk_set_cookies_local()` 只更新本地快照，不修改运行中浏览器，也不触发远端更新。若环境正在运行，浏览器关闭时产生的较新快照可能覆盖本次写入。

### 5.4 读取和设置远端 Cookie

从历史节点读取：

```json
{
  "envId": "1234567890123456789",
  "fileUrl": "<fileUrl returned by sdk_get_cookies_history>",
  "md5": "0123456789abcdef0123456789abcdef"
}
```

`envId` 和 `fileUrl` 必填；`md5` 可选。成功响应是解码后的 Cookie JSON 数组。读取操作不修改本地快照或运行中浏览器。

设置远端快照：

```json
{
  "envId": "1234567890123456789",
  "cookies": []
}
```

`sdk_set_cookies_remote()` 在远端写入流程完整成功后返回 `CL_OK`，随后尽力刷新本地快照；本地刷新失败不会回滚已经成功的远端更新。该接口不修改运行中浏览器；环境随后关闭时，浏览器的较新 Cookie 状态可能替换远端快照。

### 5.5 Cookie 健康检查

`sdk_cookies_health_check()` 是纯同步、离线分析接口，不要求 SDK 初始化，也不访问浏览器或网络。

请求：

```json
{
  "cookies": [
    {
      "name": "preference",
      "value": "<cookie-value>",
      "domain": ".example.com",
      "path": "/",
      "session": false,
      "expirationDate": 2000003600,
      "secure": true
    }
  ]
}
```

响应字段节选（以下时间值用于说明字段关系）：

```json
{
  "checkedAt": 2000000000,
  "expiresSoonThresholdSeconds": 86400,
  "summary": {
    "status": "warning",
    "cookieCount": 1,
    "domainCount": 1,
    "expiredCookieCount": 0,
    "expiringSoonCookieCount": 1
  },
  "domains": [
    {
      "domain": "example.com",
      "status": "warning",
      "authStatus": "not_detected",
      "nextExpirationRemainingSeconds": 3600,
      "cookies": [
        {
          "cookieName": "preference",
          "path": "/",
          "persistence": "persistent",
          "expiration": 2000003600,
          "remainingSeconds": 3600,
          "status": "expiring_soon",
          "authCandidate": false
        }
      ],
      "tokens": [],
      "issues": [
        {
          "code": "cookie_expiring_soon",
          "severity": "warning",
          "count": 1,
          "cookieName": "preference"
        }
      ]
    }
  ],
  "globalIssues": []
}
```

关键字段：

| 字段 | 语义 |
| --- | --- |
| `checkedAt` | 检查时刻，Unix 秒。 |
| `expiresSoonThresholdSeconds` | “即将过期”的告警窗口；不是 Cookie 自身有效期。 |
| `remainingSeconds` | Cookie 距离过期的秒数；负数表示已过期，session Cookie 或无有效期时为 `null`。 |
| `nextExpirationRemainingSeconds` | 该 domain 下一次到期距离检查时刻的秒数；没有未来的持久 Cookie 到期点时为 `null`。 |
| `status` | 汇总、domain 或单 Cookie 的健康状态。 |
| `authStatus` | 认证候选 Cookie 的时间窗判断。 |
| `issues` / `globalIssues` | domain 级和全局问题列表。 |

稳定枚举值：

| 字段 | 可能值 |
| --- | --- |
| 汇总/domain `status` | `healthy`、`unknown`、`warning`、`critical`。 |
| Cookie `persistence` | `session`、`persistent`。 |
| Cookie `status` | `session`、`missing_expiration`、`not_expired`、`expiring_soon`、`expired`。 |
| domain `authStatus` | `not_detected`、`time_valid`、`expired`、`not_yet_valid`、`unknown`、`mixed`。 |
| Token `status` | `time_valid`、`expired`、`not_yet_valid`、`unknown`。 |
| issue `severity` | `info`、`warning`、`error`。 |

`expiresSoonThresholdSeconds` 在 v2.0.0.8 中固定为 `86400` 秒，只是告警窗口。Token 项的 `exp`、`nbf`、`iat` 和 `remainingSeconds` 在无法确定时为 `null`。健康报告不会返回 Cookie value；JWT 分析不验证签名，`signatureVerified` 为 `false`。因此 `time_valid` 只表示客户端可见时间窗有效，不证明服务端登录仍然有效。

### 5.6 Cookie 使用规则

- 从 SDK getter 或 Cookie 回调取得的数组，可以在保留原字段的前提下传给健康检查或 setter。
- 修改数组时保留未知字段；只修改业务明确授权的 Cookie。
- 不要为了“延长登录”把 session Cookie 强行改为持久 Cookie，也不要把过期时间统一改为极大值。这不会延长服务端会话，反而可能破坏原始 Cookie 语义。
- Cookie 健康检查只能发现结构、有效期、重复项、安全属性和 Token 时间窗问题，不能替代真实站点请求。
- Cookie 接口不读取或修改 Storage。v2.0.0.8 没有与上述六个 Cookie 接口对应的公开 Storage 直接读写 C ABI；Storage 按环境配置和浏览器生命周期处理。

### 5.7 Cookie 持久化回调

`sdk_register_cookies_storage_cb()` 用于在 SDK 持久化 Cookie 前检查或替换完整数组。回调输入是标准事件对象，Cookie 数组位于 `data.cookies`：

```json
{
  "code": 0,
  "type": "browser-cookie-update-cb",
  "msg": "ok",
  "data": {
    "envId": "1234567890123456789",
    "eventId": 20268,
    "cookies": []
  }
}
```

回调输出规则：

| 行为 | 返回规则 |
| --- | --- |
| 不修改 | 保持 `*new_data == NULL` 且 `*new_len == 0`。 |
| 替换 | 返回至少包含一个可注入 Cookie 的完整 JSON 数组，不是事件对象或增量补丁。 |
| 分配 | 使用 `sdk_malloc()` 分配，返回后由 SDK 释放。 |

空数组、非法 JSON，或规范化后不含可注入 Cookie 的结果都会回退到原浏览器快照；该回调不能用于清空 Cookie。需要清空持久化快照时，应使用明确支持空数组的 Cookie setter，并结合 5.3/5.4 的覆盖规则评估运行中环境。

回调必须快速返回。需要耗时处理时，先复制数据，再交由宿主异步处理。

## 6. 环境、浏览器与网络接口

### 6.1 环境管理

环境管理接口为同步调用，请求和响应字段遵循环境管理业务契约：

| C ABI | Web API | 用途 |
| --- | --- | --- |
| `sdk_env_create` | `POST /sdk/v1/env/create` | 创建环境。 |
| `sdk_env_update` | `POST /sdk/v1/env/update` | 更新环境。 |
| `sdk_env_page` | `POST /sdk/v1/env/page` | 分页查询环境。 |
| `sdk_env_getinfo` | `POST /sdk/v1/env/getinfo` | 查询环境详情。 |
| `sdk_env_destroy` | `POST /sdk/v1/env/destroy` | 销毁环境。 |

销毁环境与关闭浏览器是不同操作。销毁前应先关闭对应浏览器环境。

### 6.2 浏览器辅助接口

| C ABI | 模式 | 用途 |
| --- | --- | --- |
| `sdk_browser_install` | 异步 | 安装或准备浏览器核心。 |
| `sdk_browser_info` | 同步 | 查询当前运行中的环境。 |
| `sdk_browser_cleanup` | 同步 | 清理未运行环境的本地缓存或 SDK 管理的浏览器核心缓存。 |
| `sdk_browser_command` | 同步 | 向运行中环境发送高级浏览器命令。常规 Agent 自动化优先使用 MCP。 |
| `sdk_browser_env_check` | 同步 | 在运行中环境打开环境检查页。 |
| `sdk_browser_snapshot` | 同步 | 获取页面快照或诊断输出；可选发送 `browser.snapshot.*` 事件。 |

常用请求形状：

| 接口 | 请求 |
| --- | --- |
| `sdk_browser_install` | `{"cores":[{"major":141}]}`。`major` 为需要准备的浏览器主版本。 |
| `sdk_browser_cleanup` | `{"envs":["1234567890123456789"]}`、`{"cores":[]}` 或 `{"cores":[{"major":141}]}`。 |
| `sdk_browser_command` | `{"envId":"1234567890123456789","method":"<method>","params":{}}`；`sessionId` 可选。 |
| `sdk_browser_env_check` | `{"envId":"1234567890123456789"}`。 |
| `sdk_browser_snapshot` | `{"envId":"1234567890123456789","includeHtml":true,"includeScreenshot":true,"emitEvents":false}`。 |

`sdk_browser_snapshot` 当前是 C ABI 接口，不属于公共 `ISDK` 虚接口。C++ 接入方如需此能力，应通过 C ABI 调用。

`sdk_browser_cleanup()` 会删除本地数据，调用前必须确认目标。省略 `cores` 表示不清理浏览器核心缓存；`"cores":[]` 表示清理全部 SDK 管理的浏览器核心缓存；指定 `major` 只清理对应主版本。处于启动、运行或关闭过程中的环境不会被清理，并以 `CL_EBUSY`/`busy` 返回。

### 6.3 网络诊断

指定目标和代理参数：

```json
{
  "url": "https://example.com",
  "proxy": "",
  "bridgeProxy": ""
}
```

| C ABI | Web API | 用途 |
| --- | --- | --- |
| `sdk_network_diagnostics` | `POST /sdk/v1/netdiag` | 诊断目标 URL 与指定代理配置。 |
| `sdk_system_proxy_diagnostics` | `POST /sdk/v1/proxydiag` | 查询当前系统代理摘要。 |

生产环境应使用明确的环境代理配置。诊断结果用于定位问题，不应作为长期业务配置。

## 7. 本地 Web API 与 WebSocket

### 7.1 启用与地址

在初始化 JSON 中传入 `port`：

| `port` | 行为 |
| --- | --- |
| 不传 | 不启用本地 Web API。 |
| `0` | 自动选择本机可用端口。 |
| 大于 `0` | 尝试监听指定端口。 |

支持两种启动顺序：

- 原生优先：注册结果回调后，调用 `sdk_init()` 或 `sdk_init_async()`，并在初始化 JSON 中传入 `port`。这是推荐方式。
- Web 优先：先为一个明确的非零端口调用 `sdk_init_webapi(port)` 以请求启动本地入口；确认端口可连接后，再向该端口 `POST /sdk/v1/init`，请求体至少包含 `userSig`。该方式用于无法直接提交完整原生初始化 JSON 的兼容接入。

`sdk_init_webapi()` 的直接返回只表示请求受理或启动尝试，不表示端口已经可用。宿主应采用有上限的短暂连接重试确认服务就绪。Web 优先模式仍必须完成 `/sdk/v1/init`；只调用 `sdk_init_webapi()` 不代表 SDK 业务初始化成功。

v2.0.0.8 的本地 Web API 不使用 `Authorization` 请求头；业务凭据放在 `/sdk/v1/init` 的 JSON body 中。不要把 `userSig` 放入 URL、普通日志或页面持久化存储。

| 通道 | 地址 |
| --- | --- |
| HTTP | `http://127.0.0.1:{port}` |
| WebSocket | `ws://127.0.0.1:{port}/` |
| 全局 MCP | `http://127.0.0.1:{port}/sdk/v1/mcp` |
| 单环境 MCP | `http://127.0.0.1:{port}/sdk/v1/mcp/env/{envId}` |

这些入口面向本机可信客户端。不要将端口直接暴露到局域网或公网。

标准发布包允许本机客户端访问这些入口；定制交付可能额外限制浏览器页面或扩展的 Origin。收到 HTTP `403` 时，应核对 endpoint、会话归属和交付方提供的允许来源，不要通过关闭浏览器安全策略规避。

### 7.2 Web API 路由

| 路由 | 模式 | 用途 |
| --- | --- | --- |
| `POST /sdk/v1/init` | 同步 | 初始化 SDK。 |
| `POST /sdk/v1/info` | 同步 | 查询 SDK 信息。 |
| `POST /sdk/v1/netdiag` | 同步 | 网络诊断。 |
| `POST /sdk/v1/proxydiag` | 同步 | 系统代理诊断。 |
| `POST /sdk/v1/browser/info` | 同步 | 查询运行中环境。 |
| `POST /sdk/v1/browser/cleanup` | 同步 | 清理本地缓存。 |
| `POST /sdk/v1/browser/install` | 异步 | 安装或准备浏览器核心。 |
| `POST /sdk/v1/browser/open` | 异步 | 打开环境。 |
| `POST /sdk/v1/browser/close` | 异步 | 关闭环境。 |
| `POST /sdk/v1/token/update` | 异步 | 更新 `userSig`。 |
| `POST /sdk/v1/env/create` | 同步 | 创建环境。 |
| `POST /sdk/v1/env/update` | 同步 | 更新环境。 |
| `POST /sdk/v1/env/page` | 同步 | 分页查询环境。 |
| `POST /sdk/v1/env/getinfo` | 同步 | 查询环境详情。 |
| `POST /sdk/v1/env/getcookiehistory` | 同步 | 查询 Cookie 历史。 |
| `POST /sdk/v1/env/destroy` | 同步 | 销毁环境。 |

`sdk_shutdown()` 是稳定的 C/C++ 停止入口；v2.0.0.8 不将 HTTP shutdown 路由列为公共接入契约。

### 7.3 HTTP ACK 与 WebSocket 终态

同步路由在 HTTP 返回时完成。SDK 控制类响应通常使用 `code`、`reqId`、`type`、`msg`、`data` 结构；环境管理和 Cookie 历史路由按各自业务响应结构返回。所有路由都应解析 JSON body，不应仅按 HTTP 状态码判断业务结果。

异步路由先返回 ACK：

```json
{
  "code": 0,
  "reqId": 0,
  "type": "browser-open",
  "msg": "accepted",
  "data": {
    "eventId": 20110,
    "accepted": true,
    "async": true,
    "dispatchCode": 1,
    "dispatchMsg": "done"
  }
}
```

顶层 `code=0` 表示 HTTP 请求已受理；`data.dispatchCode=1` 对应 C ABI 的 `CL_DONE`。`reqId=0` 的 ACK 仍可表示受理成功。最终结果通过 WebSocket 推送，并与 `sdk_result_cb_t` 的 JSON body 使用同一业务事件格式。

WebSocket 使用 UTF-8 JSON 文本帧。连接建立时若已有环境运行，SDK 可能立即推送当前运行状态；多个连接会分别收到广播。断线期间的事件不保证补发，令牌到期提醒、用户关闭窗口等主动事件也可能没有对应的 HTTP 请求，因此消费者应按 `type` 和 `envId` 幂等更新状态。

推荐流程：

1. 先建立 WebSocket 连接。
2. 再通过 HTTP 发起异步请求。
3. 按事件 `type` 和 `envId` 收敛终态。
4. WebSocket 断线后重新连接，并通过 `/sdk/v1/browser/info` 对账当前运行状态；不要依赖断线事件补发。

## 8. MCP 接入

### 8.1 入口

| 方法与入口 | 用途 |
| --- | --- |
| `POST/GET/DELETE /sdk/v1/mcp` | 全局环境与生命周期管理及会话。 |
| `GET /sdk/v1/mcp/health` | 全局 MCP 健康检查。 |
| `POST/GET/DELETE /sdk/v1/mcp/env/{envId}` | 单环境浏览器自动化及会话。 |
| `GET /sdk/v1/mcp/env/{envId}/health` | 单环境 MCP 健康检查。 |
| `GET /sdk/v1/mcp/env/{envId}/artifacts/{artifactId}` | artifact 分片读取。 |

新接入统一使用以上 `/sdk/v1/mcp` 路径。`initialize`、`notifications/initialized`、`tools/list` 和 `tools/call` 是发送到同一 endpoint 的 JSON-RPC 方法，不是独立 HTTP 路径。

### 8.2 会话握手

全局与单环境 endpoint 使用相同握手流程；会话只属于建立它的 endpoint，不能跨 endpoint 复用。

第一步，发送 `initialize`。首次请求不得携带旧的 `Mcp-Session-Id`：

```http
POST /sdk/v1/mcp/env/1234567890123456789 HTTP/1.1
Host: 127.0.0.1:{port}
Content-Type: application/json
Accept: application/json, text/event-stream

{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-11-25","capabilities":{},"clientInfo":{"name":"my-client","version":"1.0.0"}}}
```

成功响应头包含：

```http
Mcp-Session-Id: <session-id>
Mcp-Protocol-Version: <negotiated-version>
```

第二步，发送初始化完成通知。后续请求携带返回的会话 ID 和协商版本：

```json
{
  "jsonrpc": "2.0",
  "method": "notifications/initialized",
  "params": {}
}
```

该通知不带 `id`，成功时 HTTP 状态为 `202` 且没有 JSON-RPC 响应体。未完成该通知前调用 `tools/list` 或 `tools/call` 会失败。

第三步，发现工具：

```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "tools/list",
  "params": {}
}
```

每个后续 POST 都应包含：

```http
Content-Type: application/json
Accept: application/json, text/event-stream
Mcp-Session-Id: <session-id>
Mcp-Protocol-Version: <negotiated-version>
```

客户端应采用服务端返回的协商版本，不要假定请求版本一定被原样接受。

### 8.3 工具发现

全局 MCP 的标准管理工具：

```text
sdk.health, sdk.info,
env.list, env.resolve, env.get, env.create, env.update, env.destroy,
browser.open, browser.close, browser.cleanup, browser.status, browser.install,
task.list, task.get, mcp.endpoint
```

v2.0.0.8 标准包的单环境公开工具：

```text
browser_state, tabs, bookmarks, history, tab_groups, windows,
navigate, snapshot, diff, read, grep, wait,
act, download, upload, screenshot, pdf, evaluate
```

不同交付配置的工具面可能不同。客户端必须以每个会话的 `tools/list` 返回值和 `inputSchema` 为准，不要硬编码工具数量、隐藏工具或参数结构。

调用示例：

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

优先读取工具结果中的 `structuredContent`；`content[].text` 用于面向人或模型的摘要。`isError: true` 表示工具业务失败。

### 8.4 MCP 生命周期终态

全局 MCP 的 `browser.open` 和 `browser.close` 是异步工具。工具调用成功只表示请求已受理；结果中的 `taskId` 是 MCP 管理任务 ID，`sdkReqId` 是可选的 SDK 请求 ID，两者不可混用。

纯 MCP 客户端应保存 `taskId`，随后调用：

```json
{
  "jsonrpc": "2.0",
  "id": 4,
  "method": "tools/call",
  "params": {
    "name": "task.get",
    "arguments": {
      "taskId": "mcp-task-1"
    }
  }
}
```

使用有上限的退避轮询 `task.get`；任务 `status` 为 `succeeded` 或 `failed` 时终止。工具即时结果中的 `opening`/`closing`，以及任务状态 `accepted`/`running`，都不是终态。批量请求只有在所有目标环境成功时才是 `succeeded`，任一环境失败则为 `failed`。纯 MCP 接入不需要另行连接 SDK 回调或 WebSocket。

`browser.install` 在 v2.0.0.8 中只通过 MCP 返回受理结果；其 `taskId` 不提供安装终态。需要可靠判断安装完成时，应通过 C/C++ 结果回调或 Web API + WebSocket 调用并等待 `browser-install-success`/`browser-install-failed`，不能轮询该 MCP task 代替终态事件。

MCP task 只保存在当前 SDK 进程内，且只保留最近 512 条。应及时轮询并保存业务终态；达到客户端等待上限、收到 `TASK_NOT_FOUND` 或 SDK 重启后，将任务结果视为 `unknown`，按 `envId` 使用 `browser.status` 和业务状态对账。不要无限等待，也不要在未对账时直接重放有副作用的请求。

### 8.5 SSE 事件流

握手完成后，可对 endpoint 发起 `GET` 并携带会话头。全局 endpoint 的 GET 只返回 SSE heartbeat，用于会话连通性检查，不提供浏览器生命周期事件重放。单环境 endpoint 提供页面状态事件流：

```http
GET /sdk/v1/mcp/env/1234567890123456789 HTTP/1.1
Host: 127.0.0.1:{port}
Accept: text/event-stream
Mcp-Session-Id: <session-id>
Mcp-Protocol-Version: <negotiated-version>
```

标准 MCP 客户端通常会自动维护该连接。单环境服务端通过 SSE `id:` 字段发送游标；断线重连时，客户端可把最后收到的 `id:` 原样放入 `Last-Event-ID` 请求头。该机制只恢复最新的 tabs 状态版本，不保证逐条重放断线期间的所有事件；游标是不透明值，不要自行解析或递增。

### 8.6 页面 ref 与 artifact

`snapshot` 返回的 ref 是不透明、会话级、环境级句柄：

- 不要解析或拼接 ref。
- 不要跨会话或跨环境复用。
- 导航、刷新或页面显著变化后重新调用 `snapshot`。
- 收到 `REF_NOT_FOUND` 时重新观察页面，再执行动作。

截图、PDF 或其他大结果可能返回以下 artifact 元数据：`artifactId`、`uri`、`mimeType`、`size`、`sha256`、`chunkSize`、`expiresAtMs`。其中 `size` 和 `sha256` 描述完整 artifact。

按 `offset` 和 `limit` 分片读取：

```http
GET /sdk/v1/mcp/env/{envId}/artifacts/{artifactId}?offset=0&limit=1048576 HTTP/1.1
Host: 127.0.0.1:{port}
Mcp-Session-Id: <session-id>
Mcp-Protocol-Version: <negotiated-version>
```

artifact 只能由创建它的同一 MCP 会话、同一环境读取。HTTP `200` body 是 `mimeType` 对应的原始字节，不得先按文本解码再计算哈希。客户端应读取到 EOF，并按元数据中的完整大小和完整 SHA-256 校验结果。

每个成功分片响应包含：

| 响应头 | 含义 |
| --- | --- |
| `X-BroSDK-Artifact-Offset` | 当前分片的起始偏移。 |
| `X-BroSDK-Artifact-Size` | 当前分片的字节数，不是完整 artifact 大小。 |
| `X-BroSDK-Artifact-Eof` | `true` 表示当前分片后已无数据。 |
| `X-BroSDK-Artifact-Sha256` | 当前分片的 SHA-256，不是完整 artifact 哈希。 |

每次用“当前 offset + 当前分片大小”推进下一次请求，并校验分片哈希；读取至 EOF 后，再按元数据中的 `size` 和 `sha256` 校验完整结果。offset 超出完整大小时返回 HTTP `416`；artifact 过期或不存在时返回 `404`。

### 8.7 关闭会话

```http
DELETE /sdk/v1/mcp/env/1234567890123456789 HTTP/1.1
Host: 127.0.0.1:{port}
Mcp-Session-Id: <session-id>
Mcp-Protocol-Version: <negotiated-version>
```

向建立该会话时使用的同一个 endpoint 发送 `DELETE`；全局会话使用 `/sdk/v1/mcp`，单环境会话使用 `/sdk/v1/mcp/env/{envId}`。成功时返回 HTTP `204` 和空 body。关闭会话会释放其会话状态；单环境会话的 ref 和 artifact 随之失效，但浏览器环境不会关闭。需要关闭环境时，仍调用 `browser.close`、`sdk_browser_close()` 或 Web API `/sdk/v1/browser/close`。

### 8.8 常见 MCP 错误

| 错误或状态 | 含义 | 处理 |
| --- | --- | --- |
| HTTP `400` | 后续请求的 `Mcp-Protocol-Version` 缺失或不受支持，或请求格式非法。 | 使用初始化响应返回的协商版本并修正请求。 |
| HTTP `401` | 初始化后的请求缺少 `Mcp-Session-Id`。 | 重新执行完整握手并携带会话头。 |
| HTTP `403` | 会话与环境不匹配，或访问来源不允许。 | 核对 endpoint、会话和本机访问边界。 |
| HTTP `404` / `INVALID_SESSION` | 会话未知或已经过期。 | 重新执行完整握手。 |
| HTTP `409` | 尚未完成 `notifications/initialized`，或同一会话中已有相同 JSON-RPC `id` 的请求正在执行。 | 完成握手；并确保并发请求使用不同 `id`，待原请求完成后再重试。 |
| HTTP `406` | POST 的 `Accept` 未同时包含 JSON 与 event-stream，或 GET 未接受 event-stream。 | 使用本文对应方法给出的 `Accept`。 |
| HTTP `415` | `Content-Type` 不是 JSON。 | 使用 `application/json`。 |
| HTTP `503` / JSON-RPC `-32000` | 工具队列已满或服务暂不可用。工具结果中也可能给出 `MCP_QUEUE_FULL`。 | 降低并发并采用有上限的退避重试。 |
| HTTP `504` / `TIMEOUT` | 工具调用超时；服务端已请求协作取消，但正在执行的动作不保证已经停止。 | 若已返回 `taskId`，先查 `task.get`；否则用 `browser.status` 或页面状态对账。有副作用的工具不要直接盲目重试。 |
| `BROWSER_NOT_READY` | 环境尚未就绪。 | 等待 `browser-open-success`。 |
| `REF_NOT_FOUND` | 页面 ref 已失效。 | 重新 `snapshot`。 |
| `UNSUPPORTED_CAPABILITY` | 当前平台或交付配置不支持该能力。 | 使用 `tools/list` 中可用的替代能力。 |

## 9. 事件、返回码与排障

### 9.1 常见事件

| 事件 | 含义 |
| --- | --- |
| `sdk-init-success` / `sdk-init-failed` | SDK 初始化终态。 |
| `sdk-token-update-success` / `sdk-token-update-failed` | 令牌更新终态。 |
| `sdk-token-expire-warning` / `sdk-token-expired` | 令牌有效期提醒或过期。 |
| `browser-install-progress` / `browser-install-success` / `browser-install-failed` | 浏览器核心准备过程与终态。 |
| `browser-open-success` / `browser-open-failed` / `browser-open-timeout` | 环境打开终态。 |
| `browser-close-success` / `browser-close-failed` / `browser-close-timeout` | 环境关闭终态。 |
| `browser-proxy-degraded` | 代理能力降级提示。 |
| `browser-cookie-update-cb` | Cookie 持久化回调事件。 |
| `browser.snapshot.begin/page/chunk/end` | 仅在快照请求启用事件输出时发送。 |

所有事件始终按 JSON 顶层 `type` 路由。对枚举事件，`data.eventId` 可配合 `sdk_event_name()` 做一致性校验；`browser.snapshot.*` 等事件不保证携带 `eventId`，不能把它作为通用路由前提。

### 9.2 常见返回码

| 数值 | 名称 | 含义 |
| --- | --- | --- |
| `0` | `CL_OK` | 同步成功或无警告成功。 |
| `1` | `CL_DONE` | 异步请求已受理，尚未完成。 |
| `103` | `CL_WPROXYDEGRADED` | 浏览器可运行，但代理能力降级。 |
| `104` | `CL_WBUSY` | 当前操作暂时繁忙。 |
| `105` | `CL_WCOOKIE_RESTORE_DEGRADED` | Cookie 恢复或快照存在降级。 |
| `107` | `CL_WBRWALREADYRUNNING` | 环境已运行，已复用并尝试激活现有窗口。 |
| `-3001` | `CL_EBUSY` | 资源正在使用，当前操作不能执行。 |
| `-3002` | `CL_ETIMEOUT` | 操作超时。 |
| `-3003` | `CL_EINVALID` | 请求或参数无效。 |
| `-3004` | `CL_ENOTFOUND` | 请求的数据不存在。 |

不要只维护静态错误表。运行时可使用 `sdk_error_name()`、`sdk_error_string()`、`sdk_is_error()` 和 `sdk_is_warn()` 读取和分类返回码。

### 9.3 排障

| 问题 | 检查顺序 |
| --- | --- |
| 初始化失败 | 检查 `userSig`、`workDir` 可写性、头文件与动态库版本；读取 `sdk-init-failed`。 |
| 打开请求已 ACK 但未完成 | 确认已提前注册结果回调或建立 WebSocket；按 `envId` 等待打开终态。 |
| 重复打开显示为告警 | 若 `type=browser-open-success` 且 `code=107`，这是幂等成功，不是失败。 |
| 关闭后仍有远端同步日志 | `browser-close-success` 不要求远端同步先完成；以关闭终态判断浏览器生命周期。 |
| 本地 Cookie 不存在 | `sdk_get_cookies_local()` 返回 `CL_ENOTFOUND` 时，改查历史或远端节点。 |
| Cookie 看似有效但站点掉登录 | 查看健康报告的过期、domain、重复项和 Token 时间窗；最终仍以站点实际响应为准。 |
| Web API 无最终事件 | 先建立 WebSocket，再发异步 HTTP；断线后用 `browser/info` 对账。 |
| MCP 返回 406 | `Accept` 必须同时包含 `application/json` 和 `text/event-stream`。 |
| MCP 工具不存在 | 重新 `tools/list`，不要使用旧版本硬编码工具表。 |

## 10. 动态库公开接口索引

### 10.1 回调注册

| 函数 | 说明 |
| --- | --- |
| `sdk_register_result_cb` | 注册或取消异步结果回调。 |
| `sdk_register_log_cb` | 注册或取消日志回调。 |
| `sdk_register_cookies_storage_cb` | 注册或取消 Cookie 持久化回调。 |
| `sdk_register_security_decision_cb` | 注册或取消安全策略回调。 |

### 10.2 SDK 生命周期与信息

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_init` | 同步 | 初始化 SDK，通过输出缓冲区返回 JSON，并可返回 `sdk_handle_t`。 |
| `sdk_init_async` | 异步 | 异步初始化 SDK。 |
| `sdk_init_cpp` | 同步 | 获取 C++ `ISDK` 句柄，但不执行业务初始化；调用业务接口前必须确保某一种初始化入口已成功完成。 |
| `sdk_init_webapi` | 兼容 | 仅按端口启动本地 Web API；新接入优先在初始化 JSON 中传 `port`。 |
| `sdk_info` | 同步 | 查询 SDK 信息。 |
| `sdk_get_user_sig` | 同步 | 使用 API key 获取 `userSig`。 |
| `sdk_token_update` | 异步 | 更新 `userSig`。 |
| `sdk_shutdown` | 同步 | 停止并销毁 SDK 实例。 |

### 10.3 Cookie

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_get_cookies_history` | 同步 | 查询 Cookie 历史。 |
| `sdk_get_cookies_local` | 同步 | 读取最新本地 Cookie 快照。 |
| `sdk_set_cookies_local` | 同步 | 设置本地 Cookie 快照。 |
| `sdk_get_cookies_remote` | 同步 | 读取指定远端 Cookie 快照。 |
| `sdk_set_cookies_remote` | 同步 | 设置远端 Cookie 快照。 |
| `sdk_cookies_health_check` | 同步 | 离线分析 Cookie 健康状态。 |

### 10.4 浏览器与诊断

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_network_diagnostics` | 同步 | 网络诊断。 |
| `sdk_system_proxy_diagnostics` | 同步 | 系统代理诊断。 |
| `sdk_browser_install` | 异步 | 安装或准备浏览器核心。 |
| `sdk_browser_info` | 同步 | 查询运行中环境。 |
| `sdk_browser_open` | 异步 | 打开或复用环境。 |
| `sdk_browser_close` | 异步 | 关闭环境。 |
| `sdk_browser_cleanup` | 同步 | 清理本地缓存。 |
| `sdk_browser_command` | 同步 | 高级浏览器命令。 |
| `sdk_browser_env_check` | 同步 | 打开环境检查页。 |
| `sdk_browser_snapshot` | 同步 | 获取页面快照或诊断输出。 |

### 10.5 环境

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_env_create` | 同步 | 创建环境。 |
| `sdk_env_update` | 同步 | 更新环境。 |
| `sdk_env_page` | 同步 | 分页查询环境。 |
| `sdk_env_getinfo` | 同步 | 查询环境详情。 |
| `sdk_env_destroy` | 同步 | 销毁环境。 |

v2.0.0.8 不支持 `sdk_env_get_cookies`，新接入不得调用；请使用 5.1 中的 Cookie 接口。

### 10.6 内存与辅助函数

| 函数 | 说明 |
| --- | --- |
| `sdk_malloc` / `sdk_free` | SDK 边界内存分配与释放。 |
| `sdk_error_name` / `sdk_error_string` | 返回码名称与说明。 |
| `sdk_event_name` | 事件名称。 |
| `sdk_is_error` / `sdk_is_warn` / `sdk_is_reqid` / `sdk_is_ok` / `sdk_is_done` / `sdk_is_event` | 状态分类。 |

### 10.7 C++ `ISDK`

公共 `ISDK` 提供：

- 生命周期：`Init`（同步/异步重载）、`Shutdown`、`UpdateToken`。
- 信息与诊断：`GetUserSig`、`Info`、`NetworkDiagnostics`、`SystemProxyDiagnostics`、`BrowserInfo`。
- 浏览器：`BrowserInstall`、`BrowserOpen`、`BrowserClose`、`BrowserCleanup`、`BrowserCommand`、`BrowserEnvCheck`。
- 环境：`CreateEnv`、`UpdateEnv`、`PageEnv`、`GetEnvInfo`、`DestroyEnv`。
- Cookie：`GetCookieHistory`、`GetCookiesLocal`、`SetCookiesLocal`、`GetCookiesRemote`、`SetCookiesRemote`、`CheckCookiesHealth`。
- 回调：`RegisterResultCb`、`RegisterLogCb`、`RegisterCookiesStorageCb`、`RegisterSecurityDecisionCb`。

经 `char **out` 返回的缓冲区同样使用 `sdk_free()`。不要假定所有 C ABI 都存在同名 `ISDK` 方法；例如 `sdk_browser_snapshot` 当前仅通过 C ABI 公开。

## 11. 上线前检查清单

| 检查项 | 通过标准 |
| --- | --- |
| 版本 | `brosdk.h` 与动态库均为 v2.0.0.8。 |
| 回调 | 在异步操作前完成注册，回调只做轻量处理。 |
| 标识符 | 所有请求和业务状态中的 `envId` 均按字符串处理。 |
| 异步终态 | C/C++ 等待 callback，Web API 等待 WebSocket；全局 MCP 的 `browser.open`/`browser.close` 有上限轮询 `task.get`。 |
| 警告成功 | 能正确处理 `browser-open-success + code 107`。 |
| 内存 | 按 `out_len` 读取，SDK 输出统一用 `sdk_free()`。 |
| Cookie | 保留原始字段，不伪造 `session`/`expirationDate`；健康检查不当作真实登录验证。 |
| Web API | 先建立 WebSocket，再发异步 HTTP；断线后主动对账。 |
| MCP | 完成 `initialize -> notifications/initialized -> tools/list -> tools/call`；打开/关闭任务有上限轮询至终态，artifact 按分片和整体双重校验。 |
| 安全 | 本地端口不对外暴露，不记录 API key、`userSig`、Cookie、代理凭据或页面敏感内容。 |
| 退出 | 先收敛所有环境关闭终态，再调用 `sdk_shutdown()`。 |

发布或升级前，至少完成一次初始化、打开、重复打开、关闭、Cookie 本地读写、Cookie 历史读取、Cookie 健康检查、WebSocket 终态、MCP 工具调用以及 MCP 打开/关闭任务终态的端到端验证；浏览器核心安装应单独验证 callback/WebSocket 终态，会返回 artifact 的能力还应验证分片下载与完整性校验。
