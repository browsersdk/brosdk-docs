# BroSDK v2.0.0.1 接入参考文档

> **当前版本：v2.0.0.1　　最后更新：2026-06-27**   本文档是 BroSDK v2.0.0.1 的统一接入参考，覆盖动态库接口与本地 Web API。文档面向 SDK 接入方，只描述公开接口、行为语义、接入原则和排障口径，如字段或行为与头文件存在差异，请以随包发布的 `brosdk.h` 为最终依据。

## 1. 概述

BroSDK Browser Agent Runtime 是用于浏览器环境初始化、生命周期管理、代理诊断和数据托管的 SDK。

当前版本负责 SDK 初始化、环境管理、浏览器核心安装、浏览器打开/关闭、Cookie/Storage 托管、安全策略回调、代理诊断、运行日志和异步事件通知。

SDK 提供两种接入方式：

| 接入方式 | 说明 | 适用场景 |
| --- | --- | --- |
| 动态库接口 | 直接加载 `brosdk.dll` / `brosdk.so` / `brosdk.dylib`，调用 `brosdk.h` 中的 C/C++ 接口 | 桌面客户端、自动化宿主、原生集成 |
| 本地 Web API | 初始化时开启本地 HTTP/WebSocket 服务，通过 HTTP 调用接口，通过 WebSocket 接收异步事件 | 跨语言接入、调试工具、轻量客户端 |

两种方式共享同一套浏览器生命周期、代理策略、数据托管和日志能力。区别主要在异步结果的交付方式：

| 能力 | 动态库接口 | 本地 Web API |
| --- | --- | --- |
| 同步接口 | 函数返回码 + `out_data` | HTTP 响应 |
| 异步接口 | 函数返回受理状态，最终结果走 `sdk_result_cb_t` | HTTP 返回 ACK，最终结果走 WebSocket |
| 日志 | `sdk_log_cb_t` | 建议由宿主通过动态库日志回调或工具侧展示 |

## 2. 能力与接入口径速览

本节用于快速定位 BroSDK 公开能力边界。详细参数、请求示例和返回字段见后续章节。

| 能力 | 接入口径 | 接入说明 |
| --- | --- | --- |
| SDK 初始化 | `sdk_init` / `ISDK::Init` / `/sdk/v1/init` | 同一进程内建议串行初始化；初始化成功后再打开环境 |
| 浏览器打开 | `sdk_browser_open` / `/sdk/v1/browser/open` | 异步受理，最终可用状态以 `browser-open-success` 事件为准 |
| 浏览器关闭 | `sdk_browser_close` / `/sdk/v1/browser/close` | 异步受理，关闭完成以 `browser-close-success` 事件为准 |
| SDK 停止 | `sdk_shutdown()` / `ISDK::Shutdown()` | Web API shutdown 当前不作为稳定公开接入面 |
| 代理配置 | `proxy` / `forward` / `netdiag` / `proxydiag` | 稳定业务链路应显式传入代理；系统代理主要用于诊断和策略输入 |
| 安全策略回调 | `sdk_register_security_decision_cb` | 命中安全拦截时可返回 redirect URL；不返回时使用默认拦截行为 |
| Cookie/Storage 托管 | 环境数据配置 / `sdk_register_cookies_storage_cb` | Cookie 回调可在关闭快照持久化前检查或替换 Cookie JSON 数组 |
| 日志与排障 | `sdk_register_log_cb` / 服务端日志结构化视图 | 服务端日志默认脱敏；`verbose=true` 仅建议短期排障使用 |
| CDP 命令 | `sdk_browser_command` | 动态库同步接口，返回浏览器调试协议结果 JSON |
| 快照与环境检查 | `sdk_browser_snapshot`、`sdk_browser_env_check` | 动态库同步接口，用于环境检查和诊断快照 |
| 用户签名 | `sdk_get_user_sig` | 动态库接口；本地 HTTP 路由不作为稳定公开入口 |

## 3. 平台支持

| 平台 | 架构 | 动态库 | 状态 | 备注 |
| --- | --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | 正式支持 | 桌面客户端接入 |
| Linux | x64 | `brosdk.so` | 正式支持 | 支持主流发行版 |
| macOS | arm64 / x64 | `brosdk.dylib` | 正式支持 | 按目标架构选择动态库 |

macOS 最低目标：

| 架构 | 最低系统版本 |
| --- | --- |
| arm64 | macOS 11.0 |
| x64 | macOS 10.15 |

## 4. 通用约定

### 4.1 JSON 与编码

所有请求体、响应体和通知体均为 UTF-8 JSON。动态库接口中的 `len` 均表示字节长度，不包含字符串末尾的 `\0`。

### 4.2 内存管理

| 场景 | 规则 |
| --- | --- |
| SDK 返回的 `out_data` | 使用后必须调用 `sdk_free()` |
| 回调中的 `data` | 只在当前回调有效；如需长期保存请立即复制 |
| Cookie 替换回调 | 如需返回 `new_data`，必须用 `sdk_malloc()` 分配 |
| 安全策略回调 | 如需返回 `redirect`，必须用 `sdk_malloc()` 分配 |

### 4.3 返回码与异步受理

BroSDK 需要区分“同步返回码”和“异步请求受理 ID”。

| 数值范围 | 含义 |
| --- | --- |
| `0` | `CL_OK`，同步成功 |
| `1` | `CL_DONE`，异步任务已受理，但返回值本身不是实际 `reqId` |
| `> 100000` | 异步任务已受理，返回值是实际 `reqId` |
| `100 ~ 255` | Warning，例如 `CL_WBUSY`、`CL_WPROXYDEGRADED` |
| `< 0` | Error |

判断异步接口是否受理时，不要只判断返回值是否等于 `CL_DONE`。推荐逻辑是：

```c
if (sdk_is_done(code) || sdk_is_reqid(code)) {
  /* request accepted */
}

```

### 4.4 回调与事件路由

`sdk_result_cb_t` 是动态库接入时的统一异步结果通道。

| 字段 | 说明 |
| --- | --- |
| `code` 参数 | 本次通知的粗粒度状态 |
| `data/len` | UTF-8 JSON 事件体 |
| JSON `type` | 推荐作为事件路由主键 |
| JSON `reqId` | 推荐作为请求关联字段 |
| JSON `data.eventId` | 事件枚举补充字段 |

推荐处理顺序：

1.  解析 JSON body。

2.  优先按顶层 `type` 路由。

3.  如需关联请求，使用顶层 `reqId`。

4.  如需枚举判断，再读取 `data.eventId`。


线程注意事项：

| 回调 | 并发语义 | 建议 |
| --- | --- | --- |
| `sdk_result_cb_t` | 不保证串行 | 回调中只做入队或轻量处理 |
| `sdk_log_cb_t` | 不应阻塞 SDK | 复制数据后异步处理 |
| `sdk_cookies_storage_cb_t` | SDK 保证串行触发 | 可检查或替换 Cookie JSON |
| `sdk_security_decision_cb_t` | SDK 保证串行触发 | 可返回 redirect URL；回调中不要执行长耗时操作 |

任何回调中都不建议调用 SDK 阻塞/同步接口，避免引入死锁或长时间卡顿。

### 4.5 同步响应 Envelope

BroSDK 直接生成的同步响应通常使用以下结构：

```json
{
  "code": 0,
  "reqId": 1309318677,
  "type": "sdk-init-success",
  "msg": "ok",
  "data": {
    "eventId": 10111
  }
}

```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `code` | int | SDK 返回码 |
| `reqId` | int | SDK 请求 ID |
| `type` | string | 事件名称 |
| `msg` | string | 可读状态 |
| `data` | object | 接口专属数据 |

例外接口：

| 接口 | 响应结构 |
| --- | --- |
| `env/create`、`env/update`、`env/page`、`env/getinfo`、`env/destroy` | 后端原始 JSON 透传 |
| 动态库 `sdk_get_user_sig` | 后端原始 JSON 透传 |
| `netdiag` | 网络诊断原始 JSON，通常为 `code/msg/ok/data` |
| `proxydiag` | 标准 SDK envelope，诊断对象在 `data.systemProxy`；动态库 `sdk_system_proxy_diagnostics` 直接返回诊断对象 |
| `browser/cleanup` | 清理结果原始 JSON，通常为 `code/msg/data` |
| 动态库 `sdk_info`、`sdk_browser_info` | 直接返回原始 info / env 列表 JSON；Web API `/sdk/v1/info`、`/sdk/v1/browser/info` 仍返回 SDK envelope |
| 动态库 `sdk_browser_command`、`sdk_browser_env_check`、`sdk_browser_snapshot` | 同步 CDP 相关结果 JSON；当前没有对应的稳定本地 HTTP 路由 |

### 4.6 异步 Web API ACK

异步 Web API 的 HTTP 响应表示“请求已受理”，不是最终结果。

```json
{
  "code": 0,
  "reqId": 1594794915,
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

接入方必须通过 WebSocket 等待最终事件，例如 `browser-open-success`、`browser-open-failed`、`browser-close-success`。

## 5. 浏览器生命周期

### 5.1 状态通知结构

浏览器打开和关闭通知使用统一结构：

```json
{
  "code": 0,
  "reqId": 369488048,
  "type": "browser-open-success",
  "msg": "ok",
  "data": {
    "envId": "2041695386304778240",
    "status": 2,
    "statusName": "Started",
    "progress": 100,
    "remoteDebuggingPort": 65534
  },
  "envList": [
    {
      "envId": "2041695386304778240",
      "status": 2,
      "statusName": "Started",
      "progress": 100
    }
  ]
}

```

常见 `statusName`：

| 状态 | 说明 |
| --- | --- |
| `Idle` | 空闲 |
| `Downloading` | 正在准备资源或数据 |
| `Preparing` | 正在准备环境 |
| `Starting` | 正在启动浏览器 |
| `Started` | 已启动并可用 |
| `Stopping` | 正在关闭 |
| `Stopped` | 已关闭 |
| `Destroyed` | 已销毁 |
| `StartFailed` | 启动失败 |
| `StopFailed` | 关闭失败 |

### 5.2 打开成功的准确定义

`browser-open-success` 是浏览器可用的最终信号。收到该事件后，接入方可以认为浏览器进程已运行、自动化通道已就绪，后续可以执行 CDP 命令、打开页面或进行业务操作。

HTTP ACK 或动态库异步函数返回受理状态，只表示任务进入调度，不表示浏览器可用。

### 5.3 关闭成功的准确定义

`browser-close-success` 表示本地浏览器关闭流程已完成，SDK 已完成本地必要数据收口。若当前是全托管模式，远端同步可能仍由后台链路继续处理，接入方不应把 `browser-close-success` 理解为所有远端上传都已经完成。

如果浏览器由用户手动关闭或进程自行退出，SDK 仍可能发送 `browser-close-success`，并在 `data.closeOrigin`、`data.closeReason*` 中说明原因。

### 5.4 Shutdown 语义

`sdk_shutdown` 会停止 SDK、关闭本地 Web API、收口运行中的浏览器任务并销毁当前单例。

稳定停止入口是动态库 `sdk_shutdown()` / ISDK `Shutdown()`。`/sdk/v1/shutdown` 仅为保留路径常量；本地 Web API 当前不提供稳定 shutdown 处理，不应作为公开接入依赖。

v2.0.0.1 的重要语义：

| 场景 | 行为 |
| --- | --- |
| 启动中 shutdown | 已受理但未完成的打开任务应收到失败终态，常见 code 为 `CL_WBUSY` |
| 运行中 shutdown | SDK 会尝试收口运行中的浏览器，并完成本地必要数据处理 |
| shutdown 成功事件 | `sdk-shutdown-success` 表示 SDK 停止流程完成 |
| 队列状态 | 成功事件中的队列摘要应表示无遗留待派发任务 |

## 6. 代理、网络与安全策略

### 6.1 字段口径

| 字段 | 来源 | 说明 |
| --- | --- | --- |
| `proxy` | 环境创建 / 更新阶段绑定 | 环境默认最终上游代理 |
| `bridgeProxy` | 环境创建 / 更新阶段绑定 | 环境默认备用前置跳板 |
| `forward` | `browser/open` 本次启动参数 | 本次启动显式代理，优先级最高 |
| `bridge` | `browser/open` 本次启动参数 | 本次启动备用前置跳板，覆盖环境绑定的 `bridgeProxy`，但不写回环境 |
| `args` | `browser/open` 本次启动参数 | Chromium 兼容命令行参数；代理相关公开口径仅说明 `--proxy-bypass-list` |

推荐统一使用完整代理 URL：

```text
socks5://host:port
socks5://user:pass@host:port
socks5h://user:pass@host:port
http://user:pass@host:port

```

### 6.2 代理决策原则

v2.0.0.1 的代理收口以 `docs/bridgeProxy.mmd` 中的流程图为准。核心原则是：浏览器流量先进入 SDK 本地桥，由本地桥统一承载代理例外、代理出口选择和代理链路诊断。

| 顺序 | 条件 | 行为 |
| --- | --- | --- |
| 1 | 命中 SDK 解析的代理例外 | 本地桥直连目标，不使用代理出口 |
| 2 | `browser/open` 传入 `bridge` | `bridge` 覆盖环境 `bridgeProxy`，仅本次启动生效 |
| 3 | 传入 `forward`，且环境有 `proxy` | `forward` 作为前置代理，再连接环境 `proxy` |
| 4 | 传入 `forward`，环境无 `proxy` | 使用 `forward` 作为代理出口 |
| 5 | 未传 `forward`，环境有 `proxy`，且需要/可使用 `bridgeProxy` | 使用 `bridgeProxy` 作为前置代理，再连接环境 `proxy` |
| 6 | 未传 `forward`，环境有 `proxy`，可直接访问代理出口 | 直接使用环境 `proxy` |
| 7 | 无 SDK 代理配置，系统固定代理可用 | 默认使用系统代理出口，固定 HTTP / SOCKS 行为对齐 Chrome |
| 8 | 无可用代理来源 | 本地桥直连目标 |

接入建议：

| 目标 | 推荐做法 |
| --- | --- |
| 行为稳定可控 | 在环境中绑定 `proxy`，或在 `browser/open` 中显式传 `forward` |
| 仅本次启动换代理 | 使用 `forward` |
| 仅本次启动换前置跳板 | 使用 `bridge` |
| 配置不走代理的目标 | 使用 `--proxy-bypass-list` |
| 不希望依赖客户机器网络状态 | 不依赖系统隐式代理，显式传完整代理 URL |

### 6.3 启动时实际代理口径

浏览器启动时，SDK 会在最终事件和结构化日志中记录本次启动实际采用的代理摘要。该摘要用于回答以下问题：

| 问题 | 推荐查看字段 |
| --- | --- |
| 本次启动是否用了代理 | `diagnosis.proxy.finalRoute` |
| 代理来源是什么 | `diagnosis.proxy.source` |
| 是否实际采用系统代理 | `diagnosis.proxy.systemProxyUsed` |
| 启动时探测到什么系统代理 | `diagnosis.proxy.systemProxyProbe` |
| 最终上游代理是什么 | `diagnosis.proxy.targetMasked` |
| 是否存在前置跳板 | `diagnosis.proxy.jumps` |
| 代理入口是否成功启动 | `diagnosis.proxy.bridgeStarted` |
| 是否命中代理例外 | `diagnosis.proxy.proxyBypassCount` 或代理旁路相关摘要字段 |

常见 `finalRoute`：

| 值 | 说明 |
| --- | --- |
| `direct` | 本次未使用上游代理 |
| `proxy` | 本次使用单个上游代理 |
| `proxy-with-jumps` | 本次使用前置跳板加最终代理 |
| `unsupported` | 输入中存在当前不承诺完整支持的代理语义 |

常见 `source`：

| 值 | 说明 |
| --- | --- |
| `sdk.proxy` | 来源于环境绑定的 `proxy` |
| `sdk.forward` | 来源于本次 `browser/open` 的 `forward` |
| `sdk.bridge` | 来源于本次 `browser/open` 的 `bridge` 或环境 `bridgeProxy` |
| `system` | 来源于当前系统固定代理 |
| `direct` / `none` | 未使用上游代理 |

代理凭据默认会脱敏展示。脱敏规则保留头尾少量字符用于核对，中间替换为 `****`。例如：

```text
socks5://account:password@example.com:1080

```

在默认服务端日志中会展示为类似：

```text
socks5://acc****unt:pas****ord@example.com:1080

```

短字符串会整体隐藏。`verbose=true` 是明文诊断模式，可能输出完整代理凭据，只建议短期排障使用。

### 6.4 诊断快照与启动快照

网络诊断接口、系统代理诊断接口和浏览器启动日志是三类不同快照，不能简单混为同一事实。

| 快照来源 | 触发时机 | 用途 |
| --- | --- | --- |
| `sdk.network_diagnostics` | 调用 `netdiag` 时 | 验证请求中指定的代理链路是否可达 |
| `sdk.system_proxy_diagnostics` | 调用 `proxydiag` / `sdk_system_proxy_diagnostics` 时 | 查看当前系统代理是否能作为 SDK 代理输入 |
| `sdk.startup` | `browser/open` 启动决策时 | 记录本次浏览器启动实际使用的代理来源 |
| `sdk.network_watcher` | SDK 运行中观察到网络变化时 | 提醒系统网络环境已变化 |
| `external` | 宿主或第三方工具自行检测 | 仅代表外部工具自己的检测结果 |

如果第三方工具使用自己的网络检测逻辑，检测结果可能与 SDK 启动时的网络快照不一致。排障时应优先查看 SDK 启动终态日志中的 `diagnosis.proxy`，并对比诊断接口的调用时间。

### 6.5 代理旁路参数

`args` 中可以携带 Chromium 参数。代理相关公开口径仅说明 `--proxy-bypass-list`：

```text
--proxy-bypass-list

```

`--proxy-bypass-list` 会被 SDK 解析为代理例外。如果目标命中 `--proxy-bypass-list`，则本地桥直连目标，不使用 `forward`、`proxy`、`bridgeProxy` 或系统代理出口。

SDK 会统一管理浏览器代理入口。接入方不应把其他 Chromium 代理启动参数作为 v2.0.0.1 的公开接入口径。

### 6.6 系统代理

系统代理只作为策略输入之一。SDK 会识别当前系统固定 HTTP、HTTPS、SOCKS5 代理是否可作为本次浏览器代理来源。

注意事项：

| 场景 | 说明 |
| --- | --- |
| 系统代理变化 | 运行中的浏览器代理链路不会因此自动热切换 |
| PAC / AutoDetect | 可被识别和记录，但当前不作为完整可转换代理路线承诺 |
| SOCKS4 | 可被识别和记录，但不作为推荐代理格式 |
| 排障 | 使用 `proxydiag` 或 `sdk_system_proxy_diagnostics` 查看当前系统代理快照 |

### 6.7 网络诊断与系统代理诊断

| 能力 | Web API | 动态库接口 | 说明 |
| --- | --- | --- | --- |
| 网络 / 代理链路诊断 | `POST /sdk/v1/netdiag` | `sdk_network_diagnostics` | 按请求中的 `proxy`、`bridgeProxy` 和 `url` 做链路诊断 |
| 系统代理诊断 | `POST /sdk/v1/proxydiag` | `sdk_system_proxy_diagnostics` | 读取当前系统代理状态和 SDK 可用路线 |

`netdiag` 不执行完整 `browser/open` 策略。若要诊断启动时的 `forward` 或 `bridge`，需要按实际链路映射：

| 启动链路 | `netdiag.proxy` | `netdiag.bridgeProxy` |
| --- | --- | --- |
| `forward + proxy` | 环境 `proxy` | 本次 `forward` |
| 只有 `forward` | 本次 `forward` | 空 |
| `bridge + proxy` | 环境 `proxy` | 本次 `bridge` |

### 6.8 黑白名单与 `yunConfig`

`whitelist`、`blacklist` 是 `yunConfig` 中的兼容透传字段，用于交给定制浏览器环境按业务策略消费。SDK 文档只承诺字段接收与透传，不额外承诺黑白名单的匹配算法、拦截页、redirect 或回调触发语义。

`yunConfig{}` 常用字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `shop` | object | 否 | 定制浏览器业务对象，具体结构以业务约定为准 |
| `whitelist` | array | 否 | 白名单兼容透传字段 |
| `blacklist` | array | 否 | 黑名单兼容透传字段 |

接入建议：

| 目标 | 建议 |
| --- | --- |
| 兼容既有业务行为 | 继续通过 `browser/open` 的 `yunConfig.whitelist` / `yunConfig.blacklist` 传入 |
| 降低歧义 | 使用完整域名或明确的业务约定格式，不依赖 SDK 文档外的隐式匹配 |
| 排障 | 同时保留 `browser/open` 请求摘要、最终事件和浏览器侧表现，避免只从 SDK 代理日志推断黑白名单命中情况 |

## 7. 本地 Web API

### 7.1 启用方式

推荐在 `sdk_init` 的初始化 JSON 中携带 `port`：

```json
{
  "userSig": "your-user-sign",
  "port": 9527
}

```

端口语义：

| 值 | 行为 |
| --- | --- |
| 不传 | 不启用本地 Web API |
| `0` | SDK 自动分配本机空闲端口，实际端口在初始化响应 `data.port` 返回 |
| `> 0` | SDK 尝试监听 `127.0.0.1:{port}`，端口不可用时返回 `CL_EPORT_UNAVAILABLE` |

初始化完成后：

| 通道 | 地址 |
| --- | --- |
| HTTP | `http://127.0.0.1:{port}` |
| WebSocket | `ws://127.0.0.1:{port}/` |

兼容入口 `sdk_init_webapi(port)` 会提交一个仅包含端口的初始化请求，用于拉起本地 Web API 服务；返回值表示该请求的受理或启动尝试，不代表业务初始化已经完成。服务可用后仍需随后调用 `POST /sdk/v1/init` 并传入 `userSig`。

### 7.2 认证模型

本地 Web API 当前不依赖 `Authorization` Header。

| 接口 | 认证字段 |
| --- | --- |
| `/sdk/v1/init` | JSON body 中的 `userSig` |
| `/sdk/v1/token/update` | JSON body 中的新 `userSig` |
| 其他接口 | 复用当前已初始化的 SDK 实例 |

### 7.3 WebSocket

WebSocket 是 Web API 的异步事件通道。

| 项 | 说明 |
| --- | --- |
| 地址 | `ws://127.0.0.1:{port}/` |
| 帧类型 | UTF-8 JSON 文本帧 |
| 事件结构 | 与 `sdk_result_cb_t` 的 JSON body 一致 |
| 多客户端 | 支持并发连接，事件广播到所有已连接客户端 |
| 断线重连 | SDK 不缓存断线期间事件；重连后可能收到当前运行状态快照 |

推荐顺序：

1.  调用 `sdk_init` 并启用 `port`。

2.  建立 WebSocket 连接。

3.  调用 HTTP 异步接口，例如 `POST /sdk/v1/browser/open`。

4.  记录 HTTP ACK 中的 `reqId`，若为 `0` 则结合 `type`、`envId` 和业务上下文判断。

5.  等待 WebSocket 最终事件。


### 7.4 主动推送事件

| 事件 | 触发场景 |
| --- | --- |
| `sdk-token-expire-warning` | token 剩余有效期低于阈值 |
| `sdk-token-expired` | token 已过期 |
| `sdk-network-env-changed` | SDK 观察到网络环境变化 |
| `browser-close-success`，含 `closeOrigin` | 浏览器进程意外退出或用户手动关闭 |

## 8. Web API 接口参考

### 8.1 接口总览

| 接口 | 语义 | 是否需要 WebSocket |
| --- | --- | --- |
| `POST /sdk/v1/init` | 同步初始化 SDK | 否 |
| `POST /sdk/v1/info` | 同步获取 SDK 信息 | 否 |
| `POST /sdk/v1/netdiag` | 同步网络 / 代理诊断 | 否 |
| `POST /sdk/v1/proxydiag` | 同步系统代理诊断 | 否 |
| `POST /sdk/v1/browser/info` | 同步获取运行中浏览器 | 否 |
| `POST /sdk/v1/browser/cleanup` | 同步清理本地缓存 | 否 |
| `POST /sdk/v1/browser/install` | 异步安装浏览器核心 | 是 |
| `POST /sdk/v1/browser/open` | 异步打开浏览器 | 是 |
| `POST /sdk/v1/browser/close` | 异步关闭浏览器 | 是 |
| `POST /sdk/v1/token/update` | 异步刷新 `userSig` | 是 |
| `POST /sdk/v1/env/create` | 同步创建环境，后端原始 JSON 透传 | 否 |
| `POST /sdk/v1/env/update` | 同步更新环境，后端原始 JSON 透传 | 否 |
| `POST /sdk/v1/env/page` | 同步分页查询环境，后端原始 JSON 透传 | 否 |
| `POST /sdk/v1/env/getinfo` | 同步获取环境详情，后端原始 JSON 透传 | 否 |
| `POST /sdk/v1/env/destroy` | 同步销毁环境，后端原始 JSON 透传 | 否 |

> 本文件对应 SDK v2.0.0.1；本地 Web API 路径当前仍使用 `/sdk/v1/...`。不要把路径版本号理解为 SDK 版本号。`getUserSig` 当前作为动态库 `sdk_get_user_sig` 暴露，不作为稳定本地 HTTP 入口。

### 8.2 `POST /sdk/v1/init`

同步初始化 SDK。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 后端签发的用户令牌 |
| `workDir` | string | 否 | 工作目录根路径，实际运行目录会按应用上下文隔离 |
| `extsDir` | string | 否 | 本地扩展包根目录 |
| `port` | integer | 否 | 本地 Web API 端口 |
| `sdkApiUrl` | string | 否 | 覆盖 SDK 后端地址 |
| `autoUpdateKernel` | bool | 否 | 覆盖后端配置的浏览器核心自动更新策略 |
| `logoPath` | string | 否 | 本地图标资源绝对路径 |
| `debug` | bool | 否 | 开启开发者日志，默认 `false` |
| `verbose` | bool | 否 | 明文诊断模式，默认 `false`，生产环境建议关闭 |
| `browserRuntime` | string | 否 | 兼容旧调用方；当前不会改变实际运行内核 |

成功响应示例：

```json
{
  "code": 0,
  "reqId": 1309318677,
  "type": "sdk-init-success",
  "msg": "ok",
  "data": {
    "workDir": "C:/BroSDK/1234567890",
    "port": 9527,
    "eventId": 10111
  }
}

```

扩展目录规则：

| 规则 | 说明 |
| --- | --- |
| 扫描时机 | 仅初始化阶段扫描 |
| 默认目录 | 不传 `extsDir` 时使用实际运行目录下的 `extensions` |
| 支持结构 | `extsDir/{extension}/manifest.json` 或 `extsDir/{extension}/{version}/manifest.json` |
| Manifest | 当前只加载 Manifest V3 |
| 扩展 ID | 来自 `manifest.json` 中的 `key`，请保持稳定 |

### 8.3 `POST /sdk/v1/info`

同步获取 SDK 运行信息。

请求体推荐传 `{}`。

成功响应示例：

```json
{
  "code": 0,
  "reqId": 123456789,
  "type": "sdk-info-success",
  "msg": "ok",
  "data": {
    "info": {
      "deviceId": "device_xxx",
      "version": "2.0.0.1",
      "startupTime": 1782367200000,
      "workDir": "C:/BroSDK/1234567890",
      "tokenExpiresInS": 3600,
      "dataFullyManaged": true,
      "browserRuntime": {
        "requested": "uv",
        "selected": "uv",
        "effective": "uv",
        "source": "built-in",
        "available": ["uv"]
      }
    },
    "eventId": 10131
  }
}

```

常用字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `version` | string | SDK 版本号 |
| `workDir` | string | 实际运行目录 |
| `tokenExpiresInS` | integer | 当前 token 剩余有效秒数 |
| `dataFullyManaged` | bool | 是否为全托管数据模式 |
| `browserRuntime` | object | 浏览器运行时诊断字段 |
| `coresInfo` | object | 已加载浏览器核心信息 |
| `netInfo` | object | 当前网络环境快照 |

### 8.4 `POST /sdk/v1/netdiag`

同步执行网络 / 代理链路诊断。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `url` | string | 是 | 需要探测的目标 URL |
| `proxy` | string | 否 | 最终上游代理 |
| `bridgeProxy` | string | 否 | 诊断链路中的前置跳板 |

响应示例：

```json
{
  "code": 0,
  "msg": "ok",
  "ok": true,
  "data": {
    "request": {
      "url": "https://example.com",
      "proxy": "socks5://target-proxy:5206",
      "bridgeProxy": "socks5://jump-proxy:31034"
    },
    "chain": {
      "targetProxy": "socks5://target-proxy:5206",
      "jumpCount": 1
    },
    "started": true,
    "runningAfterStart": true,
    "listenPort": 62000,
    "error": "",
    "bridgeDiagnostics": {},
    "urlProbe": {},
    "events": []
  }
}

```

接入侧应优先判断顶层 `ok/code/msg`，诊断明细字段会随底层网络能力扩展。

返回字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `code` | int | SDK 返回码；`0` 表示诊断成功 |
| `msg` | string | 可读状态说明 |
| `ok` | bool | 本次诊断是否成功 |
| `data.request.url` | string | 本次诊断的目标 URL |
| `data.request.proxy` | string | 本次诊断使用的最终上游代理；为空表示直连诊断 |
| `data.request.bridgeProxy` | string | 本次诊断使用的前置跳板 |
| `data.chain.targetProxy` | string | 诊断链路中的最终代理 |
| `data.chain.jumpCount` | integer | 前置跳板数量 |
| `data.chain.jumps[]` | array | 前置跳板列表，包含 `role` 和 `url` |
| `data.started` | bool | 诊断链路是否成功启动 |
| `data.runningAfterStart` | bool | 启动后诊断链路是否仍在运行 |
| `data.listenPort` | integer | 本次诊断临时监听端口；仅用于诊断，不应作为业务配置保存 |
| `data.error` | string | 失败原因；成功时通常为空 |
| `data.bridgeDiagnostics` | object | 代理链路诊断明细 |
| `data.urlProbe` | object | 目标 URL 探测明细 |
| `data.events[]` | array | 诊断过程中产生的事件明细 |
| `data.events[].isError` | bool | 该事件是否为错误事件 |
| `data.events[].diagnostics` | object | 单条事件的诊断内容 |

`netdiag` 是“按请求参数诊断”，不会自动读取环境详情，也不会自动模拟完整 `browser/open` 决策。第三方或宿主如果在浏览器启动后再调用 `netdiag`，得到的是调用时的诊断结果，不一定等于浏览器启动时实际使用的代理。

### 8.5 `POST /sdk/v1/proxydiag`

同步读取当前系统代理状态，并返回 SDK 可使用的代理路线摘要。

请求体推荐传 `{}` 或空 body。

Web API 典型响应字段：

| 字段 | 说明 |
| --- | --- |
| `code` | SDK 返回码 |
| `reqId` | SDK 请求 ID |
| `type` | 事件名称 |
| `msg` | 可读状态 |
| `data.systemProxy` | 系统代理诊断对象 |

该接口只反映调用时的系统网络快照，不代表已经运行中的浏览器会自动切换代理。

Web API 响应示例：

```json
{
  "code": 0,
  "reqId": 123456789,
  "type": "sdk-netdiag-success",
  "msg": "ok",
  "data": {
    "systemProxy": {
      "status": "socks5",
      "route": "socks5://127.0.0.1:7890",
      "bridgeSupported": true,
      "connectionType": "WiFi",
      "systemProxy": {
        "status": "socks5",
        "type": "SOCKS5",
        "host": "127.0.0.1",
        "port": 7890,
        "pacUrl": "",
        "autoDetect": false,
        "bypassCount": 2,
        "bypassList": ["localhost", "127.0.0.1"]
      },
      "detail": {}
    },
    "eventId": 10151
  }
}

```

动态库 `sdk_system_proxy_diagnostics` 直接返回诊断对象，结构与 Web API 的 `data.systemProxy` 一致：

```json
{
  "status": "socks5",
  "route": "socks5://127.0.0.1:7890",
  "bridgeSupported": true,
  "connectionType": "WiFi",
  "systemProxy": {
    "status": "socks5",
    "type": "SOCKS5",
    "host": "127.0.0.1",
    "port": 7890,
    "pacUrl": "",
    "autoDetect": false,
    "bypassCount": 2,
    "bypassList": ["localhost", "127.0.0.1"]
  },
  "detail": {}
}

```

返回字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `data.systemProxy.status` / `status` | string | SDK 对当前系统代理的识别结果 |
| `data.systemProxy.route` / `route` | string | SDK 可用于浏览器代理链路的上游代理 URL；为空表示没有可用路线 |
| `data.systemProxy.bridgeSupported` / `bridgeSupported` | bool | 当前系统代理是否可作为 SDK 管理代理链路的输入 |
| `data.systemProxy.connectionType` / `connectionType` | string | 当前网络连接类型摘要，可能为空 |
| `data.systemProxy.systemProxy` / `systemProxy` | object | 系统代理原始摘要 |
| `data.systemProxy.systemProxy.status` / `systemProxy.status` | string | 系统代理识别状态 |
| `data.systemProxy.systemProxy.type` / `systemProxy.type` | string | 系统代理类型 |
| `data.systemProxy.systemProxy.host` / `systemProxy.host` | string | 系统代理主机 |
| `data.systemProxy.systemProxy.port` / `systemProxy.port` | integer | 系统代理端口 |
| `data.systemProxy.systemProxy.pacUrl` / `systemProxy.pacUrl` | string | 系统 PAC 地址，若无则为空 |
| `data.systemProxy.systemProxy.autoDetect` / `systemProxy.autoDetect` | bool | 系统是否启用自动发现 |
| `data.systemProxy.systemProxy.bypassCount` / `systemProxy.bypassCount` | integer | 系统旁路规则数量 |
| `data.systemProxy.systemProxy.bypassList` / `systemProxy.bypassList` | array | 系统旁路规则列表 |
| `data.systemProxy.detail` / `detail` | object | 与 `systemProxy` 等价或更完整的诊断明细，字段可能扩展 |

常见 `status`：

| 值 | 说明 |
| --- | --- |
| `none` | 未发现可用系统代理 |
| `http` | 发现固定 HTTP 代理 |
| `https_as_http` | 发现 HTTPS 代理端点，并按 HTTP CONNECT 代理路线使用 |
| `socks5` | 发现固定 SOCKS5 代理 |
| `invalid_http` / `invalid_https` / `invalid_socks5` | 代理配置存在但 host/port 无法组成有效路线 |
| `unsupported_pac` | 发现 PAC，但当前不承诺转换为完整代理路线 |
| `unsupported_auto_detect` | 发现自动代理发现，但当前不承诺转换为完整代理路线 |
| `unsupported_socks4` | 发现 SOCKS4，但当前不作为推荐可用路线 |

排障时建议把 `proxydiag` 的结果与浏览器启动日志一起看：`data.systemProxy.route` 说明“当前系统代理可用路线”，`diagnosis.proxy.systemProxyUsed` 才说明“某次启动是否真的用了系统代理”。

### 8.6 `POST /sdk/v1/browser/info`

同步获取当前运行中的浏览器列表。

请求体推荐传 `{}`。

响应示例：

```json
{
  "code": 0,
  "reqId": 1191362648,
  "type": "browser-info-success",
  "msg": "ok",
  "data": {
    "envs": [
      {
        "envId": "2039469749536034816",
        "remoteDebuggingPort": 65534
      }
    ],
    "eventId": 20116
  }
}

```

### 8.7 `POST /sdk/v1/browser/install`

异步安装浏览器核心资源。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cores` | array | 是 | 需要安装的浏览器核心列表 |
| `cores[].major` | integer / string | 是 | 浏览器核心主版本号，例如 `141` |

请求示例：

```json
{
  "cores": [
    { "major": 141 }
  ]
}

```

最终异步事件：

| 事件 | 说明 |
| --- | --- |
| `browser-install` | 安装任务开始或受理 |
| `browser-install-progress` | 安装进度 |
| `browser-install-success` | 安装成功 |
| `browser-install-failed` | 安装失败 |

### 8.8 `POST /sdk/v1/browser/open`

异步打开一个或多个浏览器环境。

顶层字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要打开的环境列表 |

`envs[]` 支持字符串、数字或对象。

对象字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |
| `forward` | string | 否 | 本次启动显式代理 |
| `bridge` | string | 否 | 本次启动备用前置跳板 |
| `args` | array | 否 | Chromium 兼容命令行参数 |
| `urls` | array | 否 | 启动后打开的 URL |
| `extensions` | array | 否 | 传给已加载扩展的本次启动数据 |
| `cookies` | array | 否 | 本次启动注入的 Cookie JSON 数组 |
| `yunConfig` | object | 否 | 定制浏览器透传配置 |

请求示例：

```json
{
  "envs": [
    {
      "envId": "2051156171976347648",
      "forward": "",
      "bridge": "socks5://jump-proxy.example.com:31034",
      "args": [
        "--no-first-run",
        "--no-default-browser-check"
      ],
      "urls": [
        "https://myip.ipipv.com"
      ],
      "extensions": [
        {
          "id": "jicbihcejeehghnlckloefbklclkkbei",
          "data": {
            "key1": "value1"
          }
        }
      ],
      "cookies": [
        {
          "domain": ".example.com",
          "name": "sid",
          "value": "<redacted-cookie-value>",
          "path": "/",
          "secure": true,
          "httpOnly": true
        }
      ],
      "yunConfig": {
        "whitelist": ["www.example.com"],
        "blacklist": ["blocked.example.com"]
      }
    }
  ]
}

```

关键语义：

| 项 | 说明 |
| --- | --- |
| `browser-open-success` | 浏览器真正可用信号 |
| `browser-open-failed` | 启动失败终态 |
| `browser-open-timeout` | 启动超时终态 |
| 代理降级 | 若浏览器启动成功但代理未完全按预期工作，事件仍可能是 `browser-open-success`，但 `code` 可为 `CL_WPROXYDEGRADED` |
| `extensions[]` | 只给初始化阶段已加载的扩展传数据，不安装新扩展 |
| `cookies[]` | 可注入本次启动 Cookie，接入侧应限制长度并避免日志明文泄露 |
| `yunConfig` | 透传给定制浏览器环境；`whitelist` / `blacklist` 为兼容透传字段 |

### 8.9 `POST /sdk/v1/browser/close`

异步关闭一个或多个浏览器环境。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要关闭的环境列表 |

`envs[]` 支持字符串、数字或仅包含 `envId` 的对象。

请求示例：

```json
{
  "envs": [
    "2041415694746128384"
  ]
}

```

最终关闭成功示例：

```json
{
  "code": 0,
  "reqId": 1107807335,
  "type": "browser-close-success",
  "msg": "ok",
  "data": {
    "envId": "2041415694746128384",
    "status": 4,
    "statusName": "Stopped",
    "progress": 100
  },
  "envList": []
}

```

若浏览器进程自行退出或用户手动关闭，`data` 可能包含：

| 字段 | 说明 |
| --- | --- |
| `closeOrigin` | 关闭来源，例如 `process-exited` |
| `closeReasonCode` | 关闭原因码 |
| `closeReasonName` | 关闭原因名称 |
| `closeReasonMsg` | 可读说明 |

### 8.10 `POST /sdk/v1/browser/cleanup`

同步清理本地浏览器缓存和浏览器核心下载缓存。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 否 | 要清理本地浏览器数据缓存的环境 ID，最多 128 个 |
| `cores` | array | 否 | 要清理的浏览器核心下载缓存 |
| `cores[].major` | integer / string | 条件必填 | 核心主版本号 |
| `cores[].type` | string | 否 | 内核类型过滤 |

`envs` 和 `cores` 至少提供一个有效目标。

示例：

```json
{
  "envs": ["2041415694746128384"],
  "cores": [
    { "major": 141 }
  ]
}

```

`cores` 语义：

| 写法 | 行为 |
| --- | --- |
| 缺省 | 不处理核心下载缓存 |
| `[]` | 清理全部下载缓存 |
| `[{"major":141}]` | 清理指定主版本下载缓存 |

运行中或正在打开/关闭的环境会返回 busy。`browser/cleanup` 不销毁后端环境记录，也不删除已安装的浏览器核心目录。

### 8.11 `POST /sdk/v1/token/update`

异步刷新 `userSig`。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 新的用户令牌 |

最终事件：

| 事件 | 说明 |
| --- | --- |
| `sdk-token-update-success` | 刷新成功 |
| `sdk-token-update-failed` | 刷新失败 |

### 8.12 `env/*` 接口

环境接口均为同步接口，并直接透传后端原始 JSON。请求参数和完整响应字段以后端环境 API 契约为准。

| 接口 | 说明 |
| --- | --- |
| `POST /sdk/v1/env/create` | 创建环境 |
| `POST /sdk/v1/env/update` | 更新环境 |
| `POST /sdk/v1/env/page` | 分页查询环境 |
| `POST /sdk/v1/env/getinfo` | 获取单个环境详情 |
| `POST /sdk/v1/env/destroy` | 销毁环境 |

`env/destroy` 不等于关闭浏览器。若该环境浏览器仍在运行，请先调用 `browser/close` 并等待 `browser-close-success`。

### 8.13 Shutdown 接入口径

当前稳定停止入口为动态库 `sdk_shutdown()` 或 ISDK `Shutdown()`，不建议接入方依赖 `POST /sdk/v1/shutdown`。

行为：

| 项 | 说明 |
| --- | --- |
| 停止 SDK | 停止当前 SDK 单例 |
| 关闭 Web API | 如已启用本地 Web API，会关闭 HTTP/WebSocket 服务 |
| 收口浏览器任务 | 尝试完成运行中或排队中的浏览器任务终态 |
| 销毁单例 | 后续如需继续使用，应重新初始化 |

## 9. 动态库接口参考

头文件：

```c
#include "brosdk.h"

```

### 9.1 回调类型

```c
typedef void *sdk_handle_t;

```

SDK 不透明句柄。

```c
typedef void(SDK_CALL *sdk_result_cb_t)(
    int32_t code,
    void *user_data,
    const char *data,
    size_t len);

```

异步结果回调。业务字段以 JSON body 为准。

```c
typedef enum {
  SDK_LOG_TYPE_UNKNOWN = 0,
  SDK_LOG_TYPE_LOCAL = 1,
  SDK_LOG_TYPE_SERVER = 2,
} sdk_log_type_t;

typedef void(SDK_CALL *sdk_log_cb_t)(
    sdk_log_type_t type,
    const char *data,
    size_t len);

```

日志回调。`SDK_LOG_TYPE_LOCAL` 表示本地日志；`SDK_LOG_TYPE_SERVER` 表示服务端上报视图。

```c
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(
    const char *data,
    size_t len,
    char **new_data,
    size_t *new_len,
    void *user_data);

```

Cookie 持久化前拦截回调。`data/len` 是 SDK 事件 JSON，Cookie 数组位于 `data.cookies`；如需替换，`*new_data` 返回裸 Cookie JSON 数组。

```c
typedef void(SDK_CALL *sdk_security_decision_cb_t)(
    const char *data,
    size_t len,
    char **redirect,
    size_t *redirect_len,
    void *user_data);

```

安全策略拦截回调。`data/len` 是 SDK 事件 JSON，被拦截请求的描述位于 `data.securityDecision`；如需跳转到自定义页面，返回裸 redirect URL 字符串，使用 `sdk_malloc()` 分配 `*redirect` 并设置 `*redirect_len`。保持 `NULL/0` 时，SDK 使用默认拦截行为。触发条件以当前安全策略结果为准，不应从 `yunConfig.whitelist` / `yunConfig.blacklist` 推导匹配细节。

输入示例：

```json
{
  "code": 103,
  "type": "browser-proxy-degraded",
  "msg": "proxy degraded",
  "data": {
    "envId": "2051156171976347648",
    "eventId": 20609,
    "securityDecision": {
      "url": "https://blocked.example.com",
      "action": "block"
    }
  }
}

```

### 9.2 生命周期与信息接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_register_result_cb` | 同步 | 注册全局异步结果回调 |
| `sdk_register_log_cb` | 同步 | 注册 SDK 日志回调 |
| `sdk_register_cookies_storage_cb` | 同步 | 注册 Cookie 拦截回调 |
| `sdk_register_security_decision_cb` | 同步 | 注册安全策略拦截回调 |
| `sdk_init_cpp` | 同步 | 获取当前 SDK 句柄，不执行初始化 |
| `sdk_init` | 同步 | 初始化 SDK，返回 JSON 响应 |
| `sdk_init_async` | 异步 | 异步初始化 SDK |
| `sdk_init_webapi` | 兼容辅助 | 仅提交启动本地 Web API 服务的端口请求；业务初始化仍需 `userSig` |
| `sdk_info` | 同步 | 返回 SDK info JSON |
| `sdk_get_user_sig` | 同步 | 根据请求获取 `userSig`，返回后端原始 JSON |
| `sdk_network_diagnostics` | 同步 | 返回网络 / 代理诊断 JSON |
| `sdk_system_proxy_diagnostics` | 同步 | 返回系统代理诊断 JSON |
| `sdk_token_update` | 异步 | 刷新用户令牌 |
| `sdk_shutdown` | 同步 | 停止 SDK |

`sdk_get_user_sig` 可在 SDK 业务初始化前调用。请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `apiKey` | string | 是 | 用于向后端换取 `userSig` 的 API Key |
| `customerId` | string | 否 | 客户 ID，未传时使用 SDK 默认值 |
| `duration` | integer | 否 | `userSig` 有效期秒数，未传时使用 SDK 默认值 |

响应为后端原始 JSON。接入方通常从 `data.userSig` 读取令牌，再调用 `sdk_init` 或 `POST /sdk/v1/init`。

### 9.3 浏览器接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_browser_install` | 异步 | 安装浏览器核心资源 |
| `sdk_browser_info` | 同步 | 返回当前运行中的浏览器列表 |
| `sdk_browser_open` | 异步 | 打开一个或多个浏览器环境 |
| `sdk_browser_close` | 异步 | 关闭一个或多个浏览器环境 |
| `sdk_browser_cleanup` | 同步 | 清理本地环境缓存和核心下载缓存 |
| `sdk_browser_command` | 同步 | 向运行中的浏览器发送 CDP 命令 |
| `sdk_browser_env_check` | 同步 | 打开内置环境检测页 |
| `sdk_browser_snapshot` | 同步 | 抓取运行中浏览器页面 manifest、HTML 和截图分块 |

`sdk_browser_command` 请求示例：

```json
{
  "envId": "2041415694746128384",
  "method": "Runtime.evaluate",
  "params": {
    "expression": "navigator.userAgent"
  },
  "sessionId": ""
}

```

字段说明：

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `envId` | 是 | 运行中的环境 ID |
| `method` | 是 | CDP 方法名 |
| `params` | 否 | CDP 参数，默认 `{}` |
| `sessionId` | 否 | CDP session ID |

`sdk_browser_env_check` 请求示例：

```json
{
  "envId": "2041415694746128384"
}

```

`sdk_browser_snapshot` 请求示例：

```json
{
  "envId": "2041415694746128384",
  "includeHtml": true,
  "includeScreenshot": true,
  "emitEvents": true,
  "maxPages": 64
}
```

响应为同步 JSON，包含 `type=browser.snapshot.result` 的快照结果、页面 manifest 和分块数据。`emitEvents` 默认为 `false`；传 `true` 时会额外通过 `sdk_result_cb_t` / WebSocket 发送新的 `browser.snapshot.begin`、`browser.snapshot.page`、`browser.snapshot.chunk`、`browser.snapshot.end` 事件。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string | 是 | 运行中的环境 ID |
| `includeHtml` | bool | 否 | 是否抓取 HTML，默认 `true` |
| `includeScreenshot` | bool | 否 | 是否抓取截图，默认 `true` |
| `emitEvents` | bool | 否 | 是否额外发送 snapshot 事件，默认 `false` |
| `maxPages` | integer | 否 | 最多抓取页面数，默认 `64`，上限 `256` |

同步响应典型字段：

| 字段 | 说明 |
| --- | --- |
| `type` | 当前为 `browser.snapshot.result` |
| `snapshotId` | 本次快照 ID |
| `envId` | 环境 ID |
| `pageCount` | 页面数量 |
| `pages[]` | 页面清单，含 `targetId`、`url`、`title`、`htmlRef`、`screenshotRef` |
| `chunks[]` | 数据分块，含 `ref`、`index`、`total`、`encoding`、`data` |

HTML chunk 的 `encoding` 为 `utf8`，截图 chunk 的 `encoding` 为 `base64`。该接口不会改变浏览器 open/close 的异步事件顺序；snapshot 失败不应被接入方理解为浏览器生命周期失败。snapshot 事件只由显式 snapshot 请求触发，不应作为 open/close 终态事件处理。

### 9.4 环境接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_env_create` | 同步 | 创建环境，后端 JSON 透传 |
| `sdk_env_update` | 同步 | 更新环境，后端 JSON 透传 |
| `sdk_env_page` | 同步 | 分页查询环境，后端 JSON 透传 |
| `sdk_env_getinfo` | 同步 | 获取环境详情，后端 JSON 透传 |
| `sdk_env_destroy` | 同步 | 销毁环境，后端 JSON 透传 |

`sdk_env_get_cookies` 当前仅为历史头文件声明，未形成稳定实现和接入口径，不作为 v2.0.0.1 对外公开接口。

### 9.5 辅助接口

```c
SDK_API void *SDK_CALL sdk_malloc(size_t size);
SDK_API void  SDK_CALL sdk_free(void *ptr);

```

所有 SDK 返回的动态内存都应通过 `sdk_free()` 释放。

```c
SDK_API const char *SDK_CALL sdk_error_name(int32_t code);
SDK_API const char *SDK_CALL sdk_error_string(int32_t code);
SDK_API const char *SDK_CALL sdk_event_name(int32_t evtid);

```

返回静态字符串，调用方不能释放。

```c
SDK_API bool SDK_CALL sdk_is_error(int32_t code);
SDK_API bool SDK_CALL sdk_is_warn(int32_t code);
SDK_API bool SDK_CALL sdk_is_reqid(int32_t code);
SDK_API bool SDK_CALL sdk_is_ok(int32_t code);
SDK_API bool SDK_CALL sdk_is_done(int32_t code);
SDK_API bool SDK_CALL sdk_is_event(int32_t code);

```

返回码分类辅助函数。

## 10. Cookie 与 Storage

### 10.1 托管模式

可通过 `sdk_info().dataFullyManaged` 判断当前数据托管模式。

| 值 | 模式 | 说明 |
| --- | --- | --- |
| `false` | 半托管 | 主要使用本地数据 |
| `true` | 全托管 | 本地缓存与远端同步协同工作 |

### 10.2 Cookie 回调

`sdk_register_cookies_storage_cb()` 用于 Cookie 持久化前的明文 JSON 拦截。回调输入是 SDK 事件 JSON，不是裸 Cookie 数组；Cookie 数组位于 `data.cookies`。

输入示例：

```json
{
  "code": 0,
  "type": "browser-cookie-update-cb",
  "msg": "ok",
  "data": {
    "envId": "2051156171976347648",
    "eventId": 20268,
    "cookies": []
  }
}

```

行为：

| 项 | 说明 |
| --- | --- |
| 输入 | SDK 事件 JSON；读取 `data.cookies` |
| 不替换 | 保持 `*new_data == NULL` 且 `*new_len == 0`，SDK 继续使用原始快照 |
| 替换 | 返回裸 Cookie JSON 数组；合法非空数组会参与后续持久化与同步 |
| 空快照 | `data.cookies` 会是 `[]` |
| 分配 | 替换数据必须用 `sdk_malloc()` 分配，SDK 会用 `sdk_free()` 释放 |
| 无效替换 | 一旦返回 replacement，空数组或非法 JSON 不会回退为原始快照；本次 Cookie replacement 会被视为无效 |

Storage 不走 Cookie 回调。接入方如需处理 Storage，应通过 SDK 提供的数据托管能力和后端环境配置完成。

第三方如果在回调中编辑 Cookie，应返回完整的 Cookie JSON 数组，而不是只返回差量。全托管模式下，该 replacement 会进入 SDK 的关闭持久化和远端同步链路；运维侧可结合第 11 章的数据同步日志字段确认是否已完成 OSS Cookie 上传和后端 Cookie 元数据更新。

### 10.3 本地与远端同步

关闭浏览器时，SDK 会进行本地数据快照，并按当前托管模式处理本地缓存和远端同步。`browser-close-success` 表示本地关闭与本地必要数据收口完成，不应被单独理解为所有远端上传都已经完成。

全托管模式下，如果环境刚启动后立即关闭，SDK 会避免用尚未形成有效浏览器快照的空数据覆盖已有本地或远端 Cookie/Storage。若远端同步临时失败，SDK 会保留本地可用数据，并在后续生命周期中继续尝试同步，避免因为短时网络问题导致数据直接丢失。

需要证明远端 Cookie 已完成同步时，不要只看 `browser-close-success`。建议同时查看服务端日志中的数据上传摘要，重点关注 `ossCookieUploaded`、`backendCookieMetadataUpdated`、`cookieAttempted`、`cookieOk` 和对应的 `env.id`。

## 11. 日志、脱敏与排障

### 11.1 日志类型

| 类型 | 来源 | 用途 |
| --- | --- | --- |
| `SDK_LOG_TYPE_LOCAL` | 本地 SDK 日志 | 本机研发和联调排障 |
| `SDK_LOG_TYPE_SERVER` | 服务端上报视图 | 运维、技术支持、结构化排障 |
| SDK 结果事件 | `sdk_result_cb` / WebSocket | 业务状态通知，不替代日志 |

### 11.2 脱敏模式

| 输出目标 | 默认模式 | `verbose=true` |
| --- | --- | --- |
| 本地日志 | redacted | plain |
| `SDK_LOG_TYPE_LOCAL` | redacted | plain |
| 服务端上报日志 | redacted | plain |
| `SDK_LOG_TYPE_SERVER` | 跟随服务端上报视图 | plain |

`verbose=true` 是明文诊断模式，可能把代理凭据、token、Cookie/Storage 摘要等敏感信息带入本地或上报链路。生产环境默认必须关闭，仅建议短期排障使用。

日志文件落点：

| 模式 | 默认落点 |
| --- | --- |
| `verbose=false` | SDK 系统日志目录 |
| `verbose=true` | `workDir/appId/logs` |

### 11.3 服务端日志结构

服务端日志回调可返回结构化视图。典型顶层字段：

| 字段 | 说明 |
| --- | --- |
| `schemaVersion` | 日志结构版本，例如 `sdk-server-log-v2` |
| `redactionMode` | `redacted` 或 `plain` |
| `verbose` | 是否为明文诊断模式 |
| `event` | 事件 ID 和名称 |
| `code` | 返回码、名称和说明 |
| `operation` | 本次操作摘要 |
| `env` | 环境瘦身摘要 |
| `diagnosis` | 排障摘要 |
| `lifecycle` | 生命周期阶段摘要 |
| `context` | 设备和 SDK 上下文 |
| `debug` | 失败、告警或 verbose 下的扩展诊断 |

兼容说明：当前服务端日志回调中的结构化视图用于本地展示和排障；若字段中出现 `serverUploadSchema=brolog.proto.v1`，表示实际上传协议仍处于兼容承载阶段。接入方应优先消费新的结构化视图，不要依赖旧的字符串嵌套字段。

### 11.4 代理排障字段

代理相关日志会尽量用统一字段表达：

| 字段 | 说明 |
| --- | --- |
| `source` | 本次代理来源，例如 SDK 配置、本次参数、系统代理或直连 |
| `finalRoute` | 最终路线摘要，例如 `direct`、`proxy`、`proxy-with-jumps`、`unsupported` |
| `systemProxyProbe` | 探测到的系统代理类型 |
| `systemProxyUsed` | 本次是否实际采用系统代理 |
| `bridgeStarted` | SDK 管理的本地代理入口是否已启动 |
| `proxyBypassCount` | 旁路规则数量 |

排障时建议同时查看最终业务事件和服务端日志：业务事件回答“成功还是失败”，日志回答“为什么、走了什么路线、哪里降级”。

启动成功或失败时，建议至少保留以下摘要给技术支持核对：

| 字段 | 说明 |
| --- | --- |
| `operation.id` / `operation.traceId` | 本次启动链路 ID |
| `env.id` | 环境 ID |
| `operation.result` | `success`、`failed`、`degraded`、`timeout` 等 |
| `diagnosis.proxy.source` | 代理来源 |
| `diagnosis.proxy.finalRoute` | 最终代理路线 |
| `diagnosis.proxy.targetMasked` | 脱敏后的最终上游代理 |
| `diagnosis.proxy.systemProxyProbe` | 启动时探测到的系统代理类型 |
| `diagnosis.proxy.systemProxyUsed` | 启动时是否实际使用系统代理 |
| `diagnosis.proxy.proxyBypassCount` | 代理例外规则数量 |

如果需要把代理信息上报给第三方或客服系统，请使用 `targetMasked`、`bridgeUpstreamMasked`、`systemProxyRouteMasked` 等脱敏字段，不要复制 verbose 明文日志。

### 11.5 Cookie / Storage 同步排障字段

数据同步相关日志用于回答“本地是否已收口、远端是否已上传、后端元数据是否已更新”。接入方不需要消费内部细节，但运维和技术支持建议保留以下摘要字段：

| 字段 | 说明 |
| --- | --- |
| `operation.type` | 数据上传类日志通常为 `browser.data.upload` |
| `operation.phase` | 数据上传阶段通常为 `oss_upload` |
| `operation.result` | `success`、`failed`、`degraded` 等结果 |
| `env.id` | 环境 ID |
| `cookiesSize` / `storageSize` | 本次快照或上传的数据大小摘要 |
| `cookieAttempted` / `storageAttempted` | 本次是否尝试上传 Cookie / Storage |
| `cookieOk` / `storageOk` | Cookie / Storage 上传链路是否成功 |
| `ossCookieUploaded` | Cookie 对象是否已上传到远端对象存储 |
| `backendCookieMetadataUpdated` | 后端 Cookie 元数据是否已更新 |
| `cookieCallbackReplaced` | 本次 Cookie 是否来自第三方回调替换结果 |

排障建议：

| 场景 | 优先查看 |
| --- | --- |
| 关闭后远端 Cookie 未更新 | `browser-close-success` 附近的数据上传日志、`ossCookieUploaded`、`backendCookieMetadataUpdated` |
| 第三方 Cookie 编辑未生效 | Cookie 回调调用次数、`cookieCallbackReplaced`、replacement payload 是否为合法非空数组 |
| 快速启动后立即关闭 | close lifecycle 的 snapshot/cache/upload 摘要，以及是否存在空快照保护相关提示 |
| 日志难以关联 | `operation.id` / `operation.traceId`、`env.id`、`generation` |

## 12. 兼容与使用边界

| 项 | v2.0.0.1 口径 |
| --- | --- |
| C ABI | 保持当前 `brosdk.h` 公开接口为准 |
| Web API 路径 | 仍为 `/sdk/v1/...`；`getUserSig` 当前通过动态库 `sdk_get_user_sig` 获取 |
| Web API shutdown | 当前不作为稳定公开接入面；停止 SDK 使用 `sdk_shutdown()` / ISDK `Shutdown()` |
| 异步接口 | ACK 不代表最终成功，必须等待 callback 或 WebSocket |
| 浏览器运行内核 | `uv` 为当前唯一有效运行时 |
| `browserRuntime` 初始化字段 | 兼容解析，但不改变实际运行内核 |
| 系统代理 | 可作为策略输入，但不建议作为稳定业务配置依赖 |
| 代理旁路参数 | `--proxy-bypass-list` 会作为代理例外参与 SDK 本地桥决策 |
| 黑白名单 | `yunConfig.whitelist` / `yunConfig.blacklist` 为兼容透传字段 |
| 日志 | 默认服务端日志脱敏；`verbose=true` 会进入明文诊断模式 |
| env 接口 | 参数和完整响应以后端环境 API 为准 |

## 13. 集成检查清单

正式接入前建议逐项确认：

| 检查项 | 建议 |
| --- | --- |
| 回调注册 | 动态库接入先注册 `sdk_result_cb_t`，再调用异步接口 |
| WebSocket | Web API 接入先建立 WebSocket，再调用异步接口 |
| 初始化 | 同一进程内把 `sdk_init` 当作全局串行入口 |
| 内存释放 | 所有 `out_data` 使用后调用 `sdk_free()` |
| 异步判断 | 使用 `sdk_is_done(code) || sdk_is_reqid(code)` 判断请求是否已受理 |
| 打开成功 | 只以 `browser-open-success` 作为可用信号 |
| 关闭成功 | 只以 `browser-close-success` 作为关闭完成信号 |
| 代理配置 | 稳定业务链路显式配置 `proxy` 或 `forward` |
| 系统代理 | 用 `proxydiag` 排查，不把运行时系统代理变化当作自动热更新 |
| 黑白名单 | 如需兼容既有业务行为，通过 `yunConfig.whitelist` / `yunConfig.blacklist` 显式传入并做业务侧验证 |
| 日志安全 | 生产环境关闭 `verbose` |
| 大字段 | `urls`、`args`、`extensions`、`cookies` 做长度和格式限制 |

## 14. 事件与错误码附录

### 14.1 常用事件

| 事件 ID | 名称 |
| --- | --- |
| `10110` | `sdk-init` |
| `10111` | `sdk-init-success` |
| `10112` | `sdk-init-failed` |
| `10120` | `sdk-token-update` |
| `10121` | `sdk-token-update-success` |
| `10122` | `sdk-token-update-failed` |
| `10123` | `sdk-token-expire-warning` |
| `10124` | `sdk-token-expired` |
| `10130` | `sdk-info` |
| `10131` | `sdk-info-success` |
| `10132` | `sdk-info-failed` |
| `10140` | `sdk-shutdown` |
| `10141` | `sdk-shutdown-success` |
| `10142` | `sdk-shutdown-failed` |
| `10150` | `sdk-netdiag` |
| `10151` | `sdk-netdiag-success` |
| `10152` | `sdk-netdiag-failed` |
| `20110` | `browser-open` |
| `20111` | `browser-open-success` |
| `20112` | `browser-open-failed` |
| `20113` | `browser-open-timeout` |
| `20115` | `browser-info` |
| `20116` | `browser-info-success` |
| `20117` | `browser-info-failed` |
| `20140` | `browser-close` |
| `20141` | `browser-close-success` |
| `20142` | `browser-close-failed` |
| `20143` | `browser-close-timeout` |
| `20150` | `browser-cleanup` |
| `20151` | `browser-cleanup-success` |
| `20152` | `browser-cleanup-failed` |
| `20210` | `browser-env-create` |
| `20211` | `browser-env-create-success` |
| `20212` | `browser-env-create-failed` |
| `20220` | `browser-env-update` |
| `20221` | `browser-env-update-success` |
| `20222` | `browser-env-update-failed` |
| `20230` | `browser-env-page` |
| `20231` | `browser-env-page-success` |
| `20232` | `browser-env-page-failed` |
| `20240` | `browser-env-destroy` |
| `20241` | `browser-env-destroy-success` |
| `20242` | `browser-env-destroy-failed` |
| `20250` | `browser-env-info` |
| `20251` | `browser-env-info-success` |
| `20252` | `browser-env-info-failed` |
| `20350` | `browser-install` |
| `20351` | `browser-install-progress` |
| `20352` | `browser-install-success` |
| `20353` | `browser-install-failed` |
| `20600` | `browser-proxy` |
| `20601` | `browser-proxy-success` |
| `20602` | `browser-proxy-failed` |
| `20609` | `browser-proxy-degraded` |

### 14.2 常用错误 / 警告码

| 代码 | 名称 | 说明 |
| --- | --- | --- |
| `0` | `CL_OK` | 成功 |
| `1` | `CL_DONE` | 异步任务已受理 |
| `101` | `CL_WDIRNOTEXIST` | 目录不存在 |
| `102` | `CL_WBRWPROCEXITED` | 浏览器进程自行退出 |
| `103` | `CL_WPROXYDEGRADED` | 浏览器已启动，但代理链路存在降级 |
| `104` | `CL_WBUSY` | 忙碌或关闭中受理失败终态 |
| `-3001` | `CL_EBUSY` | 资源忙 |
| `-3002` | `CL_ETIMEOUT` | 超时 |
| `-3003` | `CL_EINVALID` | 参数错误 |
| `-3005` | `CL_EALREADY` | 已存在或重复初始化 |
| `-3012` | `CL_ENOTINITIALIZED` | SDK 未初始化 |
| `-3019` | `CL_EPORT_UNAVAILABLE` | 端口非法或已占用 |
| `-3023` | `CL_EOSS_NOCLIENT` | 远端存储客户端未初始化 |
| `-3024` | `CL_EOSS_DOWNLOAD` | 远端下载失败 |
| `-3025` | `CL_EOSS_UPLOAD` | 远端上传失败 |
| `-3027` | `CL_EOSS_NOTFOUND` | 远端对象不存在 |
| `-3028` | `CL_ECOOKIE_RESTORE` | Cookie 恢复失败 |
| `-3029` | `CL_ESTORAGE_RESTORE` | Storage 恢复失败 |
| `-3030` | `CL_ENOCORERESOURCE` | 没有可用浏览器核心资源 |
| `-3502` | `CL_EHTTP_POST` | 后端 HTTP 失败 |
| `-3509` | `CL_ETOKEN_INVALID` | Token 无效 |
| `-3511` | `CL_EWORKDIR_INVALID` | 工作目录无效 |
| `-3512` | `CL_ENETWORK` | 网络或代理诊断失败 |
| `-3513` | `CL_EBROWSER` | 浏览器错误 |
| `-3514` | `CL_EBRWPROCEXITED` | 浏览器进程异常退出 |
| `-4000` 及以下 | `CL_ESDKAPI` 系列 | 后端 SDK API 错误 |

---

本文档刻意只描述公开行为和接入原则。涉及后端环境模型、浏览器核心细节、数据封装格式和日志生成细节的内容，以各自专门文档或随包头文件为准，不在对外接入文档中展开。
