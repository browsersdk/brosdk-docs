# BroSDK 接入参考

> **当前版本：v2.1.0.2 最后更新：2026-08-09** 本文档是 BroSDK 的统一接入参考文档。内容覆盖动态库接口与 Web API；行为以当前版本源码和 `brosdk.h` 公开接口为准。

## 1. 概述

BroSDK 是一个使用 C/C++ 实现的浏览器环境管理 SDK，对外提供两种接入方式：

- 动态库接口：直接加载 `brosdk.dll` / `brosdk.so` / `brosdk.dylib`，通过 `brosdk.h` 公开接口调用

- Web API：初始化时开启本地 HTTP/WebSocket 服务，HTTP 发起请求，WebSocket 接收异步事件

两种方式共享同一套浏览器生命周期、环境管理和数据持久化实现，但异步结果的交付方式不同：

- 动态库接口：函数返回值 + `sdk_result_cb_t` 回调

- Web API：同步接口直接返回；异步接口先返回 ACK，最终结果通过 WebSocket 推送

## 2. 平台支持

| 平台    | 架构        | 动态库         | 状态     | 备注                 |
| ------- | ----------- | -------------- | -------- | -------------------- |
| Windows | x64         | `brosdk.dll`   | 正式支持 | 主力开发与测试平台   |
| Linux   | x64         | `brosdk.so`    | 正式支持 | 支持主流发行版       |
| macOS   | arm64 / x64 | `brosdk.dylib` | 正式支持 | arm64 为主要测试架构 |

## 3. 通用约定

### 3.1 JSON 与编码

- 所有请求体均为 UTF-8 JSON

- 所有返回体与通知体均为 UTF-8 JSON

- 动态库接口中的 `len` 均为字节长度，不包含末尾 `\0`

### 3.2 内存管理

- 动态库接口返回的 `out_data`，使用后必须调用 `sdk_free()`

- 回调中的 `data` 指针只在当前回调有效，如需长期保存请立即复制

- `sdk_cookies_storage_cb_t` 若要替换 Cookie 数据，`*new_data` 必须通过 `sdk_malloc()` 分配

### 3.3 返回值、`reqId` 与辅助判断

需区分“同步返回码”与“异步请求受理 ID”两类返回值：

| 数值范围    | 含义                                              |
| ----------- | ------------------------------------------------- |
| `0`         | `CL_OK`，同步成功                                 |
| `1`         | `CL_DONE`，异步任务已受理，返回值不含实际 `reqId` |
| `> 100000`  | 异步请求已受理，并直接返回实际 `reqId`            |
| `100 ~ 255` | Warning，例如 `CL_WBUSY`                          |
| `< 0`       | Error                                             |

辅助函数：

- `sdk_is_ok(code)`

- `sdk_is_done(code)`

- `sdk_is_reqid(code)`

- `sdk_is_warn(code)`

- `sdk_is_error(code)`

- `sdk_error_name(code)`

- `sdk_error_string(code)`

- `sdk_event_name(evtid)`

重要说明：

- 当前异步动态库接口可能返回 `CL_DONE`，也可能直接返回 `reqId`

- 对于 Web API 异步 ACK，顶层 `reqId` 也可能是 `0`，也可能是实际受理到的请求 ID

- 业务接入时，应把“`sdk_is_done(code)` 或 `sdk_is_reqid(code)` 为真”都视为“异步任务已进入调度”

### 3.4 异步回调语义

`sdk_result_cb_t` 是动态库接入时统一的异步结果通道。

- 回调第一个参数 `code` 是本次通知的粗粒度状态

- `reqId`、`type`、`eventId` 及业务字段以 JSON body 为准

- 回调第一个参数不应用作稳定的 `reqId` 或 `eventId`

推荐做法：

1.  解析 JSON body

2.  优先根据顶层 `type` 路由

3.  如需关联请求，使用顶层 `reqId`

4.  `data.eventId` 作为事件枚举补充字段使用

**线程安全注意事项：**

- `sdk_cookies_storage_cb_t` 在 SDK 内部已串行化，不会并发调用

- `sdk_result_cb_t` 不保证串行，同一回调可能在不同事件驱动时并发调用

- 建议宿主侧在回调中加锁，或只做最小操作（如入队）后在业务线程消费

- 无论哪种回调，**禁止在回调中调用 SDK 阻塞/同步接口**（如 `sdk_init`、`sdk_shutdown`），否则可能导致死锁

### 3.5 同步响应 Envelope

BroSDK 直接生成的同步响应结构如下：

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

| 字段    | 类型   | 说明         |
| ------- | ------ | ------------ |
| `code`  | int    | SDK 返回码   |
| `reqId` | int    | SDK 请求 ID  |
| `type`  | string | 事件名称     |
| `msg`   | string | 可读状态描述 |
| `data`  | object | 接口专属数据 |

例外：

| 接口                                                                                         | 返回语义                                                                                                                           |
| -------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `env/create`、`env/update`、`env/page`、`env/getinfo`、`env/getcookiehistory`、`env/destroy` | 后端原始 JSON 透传；字段和后端错误码以服务端契约为准                                                                               |
| `netdiag`、`env/netdiag`                                                                     | BroSDK 自己生成的网络诊断原始 JSON，结构为 `code/msg/ok/data`，不额外包含 `reqId/type`                                             |
| `proxydiag`                                                                                  | 成功时为标准 BroSDK envelope，诊断对象位于 `data.systemProxy`；诊断调用自身失败时返回 `code/msg/ok=false/data.error` 原始错误 JSON |
| 动态库 `sdk_system_proxy_diagnostics`                                                        | 直接返回与 Web API `data.systemProxy` 相同的诊断对象，不包含 Web API envelope                                                      |
| `browser/cleanup`                                                                            | BroSDK 清理结果原始 JSON，结构为 `code/msg/data`，不额外包含 `reqId/type`                                                          |

- 表中标记为“原始 JSON”或“后端透传”的接口不会再套一层 BroSDK envelope；`proxydiag` 本身就是标准 envelope

### 3.6 异步 Web API ACK Envelope

异步 Web API 接口的 HTTP 响应返回的是“已受理 ACK”，不是最终结果：

```json
{
  "code": 0,
  "reqId": 1677842284,
  "type": "browser-close",
  "msg": "accepted",
  "data": {
    "eventId": 20140,
    "accepted": true,
    "async": true,
    "dispatchCode": 1,
    "dispatchMsg": "done"
  }
}
```

注意事项：

- `reqId` 可能为 `0`，也可能为实际异步请求 ID

- ACK 只表示请求已经进入 SDK 调度器

- 最终成功或失败，必须通过 WebSocket 获取

### 3.7 浏览器生命周期通知结构

浏览器打开 / 关闭相关通知使用统一结构：

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

`data` 常见字段：

| 字段                  | 类型   | 说明                                                    |
| --------------------- | ------ | ------------------------------------------------------- |
| `envId`               | string | 环境 ID                                                 |
| `status`              | int    | 浏览器生命周期状态                                      |
| `statusName`          | string | 生命周期状态名称                                        |
| `progress`            | int    | 当前进度百分比                                          |
| `remoteDebuggingPort` | int    | CDP 端口，若当前实例已开启                              |
| `alreadyRunning`      | bool   | 仅重复打开已运行环境时为 `true`；表示复用并激活现有实例 |

当前 `statusName` 可能出现：

- `Idle`

- `Downloading`

- `Preparing`

- `Starting`

- `Started`

- `Stopping`

- `Stopped`

- `Destroyed`

- `StartFailed`

- `StopFailed`

成功事件与日志告警的关系：

- `browser-open-success` 是业务成功事件，表示浏览器进程已运行；正常成功时 `code=CL_OK`

- 若目标环境已经处于 `Started` 且进程仍在运行，SDK 不会重复启动进程，而是激活现有窗口并返回 `browser-open-success`；此时 `code=CL_WBRWALREADYRUNNING (107)`、`data.alreadyRunning=true`

- 如果打开链路中代理桥启动失败或运行时代理探测失败，但浏览器最终仍然启动成功，SDK 仍发送 `browser-open-success`

- 上述“启动成功但未 100% 按预期启动”的情况，事件 `type` 仍是 `browser-open-success`，但 `code` 可以是 `CL_WPROXYDEGRADED`；后端上报日志也会以 `level=Warn` 以及 `extra.lifecycle.steps[ ]` 中的 `proxy` 步骤体现

## 4. 代理字段与当前策略

### 4.1 字段口径

代理桥决策会综合环境绑定代理与本次打开浏览器参数。`proxy` 与 `bridgeProxy` 可在创建或更新环境时绑定；`browser/open` 支持通过 `forward` 或 `bridge` 覆盖本次启动的代理或跳板配置。

| 字段          | 来源                        | 说明                                                                                           |
| ------------- | --------------------------- | ---------------------------------------------------------------------------------------------- |
| `proxy`       | 环境创建 / 更新阶段绑定     | 最终上游代理；不作为 `browser/open` 参数传入                                                   |
| `forward`     | `browser/open` 本次启动参数 | 本次启动显式代理，优先级最高；有环境 `proxy` 时作为前置跳板，无环境 `proxy` 时作为本次最终代理 |
| `bridge`      | `browser/open` 本次启动参数 | 本次启动使用的备用代理；传入后会覆盖环境绑定的 `bridgeProxy`                                   |
| `bridgeProxy` | 环境创建 / 更新阶段绑定     | 环境备用代理；默认模式下可在 `proxy` 端点经本地候选路径均不可达时成为最终代理                  |

推荐统一使用完整代理 URL：

- `socks5://host:port`

- `socks5://user:pass@host:port`

- `socks5h://user:pass@host:port`

- `http://user:pass@host:port`

### 4.2 当前默认策略的决策规则

SDK 全局配置 `network.proxy.bridgeProxyMode` 的发布默认值为
`fallback-as-proxy`。配置文件为 SDK 系统目录下的 `.brosdk.yaml`；SDK 版本变化时写入发布默认值，
同版本启动保留本地配置。

网络拓扑快照复用可通过同一配置文件控制。`egressProbe` 是兼容键名，当前不会访问公共网站：

```yaml
network:
  egressProbe:
    successCacheMs: 5000
    failureBackoffMs: 1000
```

配置在 SDK 下一次初始化时加载，不进行运行中热更新。

`successCacheMs` 的有效范围是 `0-10000` 毫秒。普通环境启动仅在 SDK 生命周期、网络拓扑指纹
一致且快照仍在有效期内时复用；每次启动仍会读取本机代理、VPN、网卡和默认路由状态。
显式网络刷新和拓扑变化始终绕过缓存。`failureBackoffMs` 的有效范围是 `0-5000` 毫秒，
仅用于在拓扑采集异常后等待并合并下一次重试，不会把失败结果当作有效网络决策。任一字段设为 `0`
即可关闭对应优化。旧 `.brosdk.yaml` 缺少这些可选字段时使用上述默认值；非法值只关闭对应优化，
不会改变代理决策模式。若整份 YAML 无法解析或 schema 不受支持，两项优化均关闭。

| 模式                | 行为                                                                                                       |
| ------------------- | ---------------------------------------------------------------------------------------------------------- |
| `fallback-as-proxy` | 默认。当 `proxy` 经直连和固定系统代理路径均不可达时，直接把 `bridgeProxy` 作为最终代理，不再连接原 `proxy` |
| `fallback`          | 兼容旧行为。`proxy` 端点不可达时使用 `bridgeProxy -> proxy` 前置跳板链路                                   |
| `always`            | 只要 `proxy + bridgeProxy` 同时存在，就使用 `bridgeProxy -> proxy` 前置跳板链路                            |
| `disabled`          | 忽略 `bridgeProxy`，保留 `proxy` / `forward` 的既有行为                                                    |

当前版本在每次浏览器启动前，会依次评估以下规则（按优先级从高到低）：

| 优先级 | 条件                                                            | 实际行为                                                                                                                                                |
| ------ | --------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1      | `browser/open` 传入 `forward`，且环境绑定了 `proxy`             | `forward` 优先级最高，跳过自动候选探测；链路为 `本地 bridge -> forward -> proxy -> 目标网站`                                                            |
| 2      | `browser/open` 传入 `forward`，且环境没有绑定 `proxy`           | `forward` 作为本次启动最终代理；链路为 `本地 bridge -> forward -> 目标网站`                                                                             |
| 3      | `forward` 为空，且 `browser/open` 传入 `bridge`                 | 本次启动先用 `bridge` 覆盖环境绑定的 `bridgeProxy`；不写回环境配置                                                                                      |
| 4      | `forward` 为空，且环境没有绑定 `proxy`                          | 不启动 SDK 管理的业务代理链；`bridge` / `bridgeProxy` 不会单独生效                                                                                      |
| 5      | `forward` 为空，环境绑定了 `proxy`，但没有可用 `bridgeProxy`    | `本地 bridge -> proxy -> 目标网站`                                                                                                                      |
| 6      | `forward` 为空，直连可到达环境 `proxy` 端点                     | 使用 A 路径：`本地 bridge -> proxy -> 目标网站`                                                                                                         |
| 7      | A 不可达，存在可复现的固定系统代理，且其可到达环境 `proxy` 端点 | 使用 B 路径：`本地 bridge -> systemProxy -> proxy -> 目标网站`                                                                                          |
| 8      | A/B 均不可达，且有 `bridgeProxy`                                | 使用 C 路径：默认 `fallback-as-proxy` 为 `本地 bridge -> bridgeProxy -> 目标网站`；兼容 `fallback` 为 `本地 bridge -> bridgeProxy -> proxy -> 目标网站` |

> `global`、`directGlobal`、`bridgeGlobal` 是为旧策略脚本保留的内部兼容字段。当前 C++
> 选择器用它们表达已选代理端点路径，不代表代理能访问互联网或某个业务网站，也不是客户配置字段。

进入代理桥时，SDK 会为每个浏览器实例单独启动一个本地 loopback bridge，并向 Chromium 注入：

```text
--proxy-server=socks5://127.0.0.1:{port}


```

关键约束：

- `forward`、`bridge` 和环境 `bridgeProxy` 不会同时叠加，**当前不支持双跳前置链**（即不存在 `forward -> bridgeProxy -> proxy` 这种路径）

- `browser/open` 支持传入 `forward` 和 `bridge`；环境固定上游代理仍请在创建或更新环境时绑定 `proxy`，只影响本次启动的显式代理可使用 `forward`

- `bridge` 和环境绑定的 `bridgeProxy` 只有在同时存在环境 `proxy` 时参与默认模式决策；如果没有 `proxy`，且本次启动也没有传入 `forward`，它们不会单独启动代理链

- 自动探测验证代理端点的 DNS/TCP 可达性、协议协商和配置鉴权，但不会通过最终代理请求公共网站或验证真实出口。TCP 可达但协议握手关闭、超时或鉴权失败会切换候选；代理握手成功后没有实际出口、拒绝具体目标或只能访问部分目标，不触发自动切换

- 较低优先级候选只在客户端收到 CONNECT 成功前使用；最终代理对业务目标返回 HTTP/SOCKS 错误后不会改走其他链路

### 4.3 当前默认策略的重要说明

当前实现的关键行为：

1.  `forward` 和 `bridge` 是 `browser/open` 可传字段，参与本次启动的代理桥决策

2.  `forward` 是本次启动的显式代理，优先级高于 `bridge` 和环境绑定的 `bridgeProxy`，并跳过自动候选探测；有环境 `proxy` 时作为前置跳板，无环境 `proxy` 时作为最终代理

3.  `bridge` 只影响本次启动；如果传入非空 `bridge`，SDK 会在本次启动中把它作为 `bridgeProxy` 使用

4.  如果没有 `proxy`，且本次启动没有传入 `forward`，SDK 不注入本地代理桥，浏览器回退至 Chromium 默认网络栈

5.  如果传入 `forward` 但没有绑定 `proxy`，链路为 `本地 bridge -> forward -> 目标网站`

注意事项：

- 没有代理桥目标时，浏览器回退至 Chromium 默认网络栈，是否走系统代理取决于 Chromium / 操作系统默认行为

- 如需行为稳定可预期，请显式绑定完整 `proxy` URL，或在 `browser/open` 中显式传入完整 `forward` URL；勿依赖客户机器隐式系统代理配置

- 非空但格式无法识别的代理 URL 不会让浏览器回退本地直连；SDK 会把它归一化为不可达代理 `socks5://127.0.0.1:9`，从而保持“必须走代理链”的约束

- `forward`、`bridge` 与 `bridgeProxy` 互斥，不支持双跳前置链

### 4.4 故障与回退

当前实现中，如果代理桥启动失败：

- SDK 会记录代理桥诊断日志，并在后端上报日志中把打开链路标记为 `Warn`

- 浏览器仍然会继续启动

- 但此时不会再使用 SDK 管理的本地代理桥

- 如果浏览器最终启动成功，对外最终事件仍是 `browser-open-success`；代理降级时事件 `code=CL_WPROXYDEGRADED`，代理问题通过日志 `extra.lifecycle.steps[ ]` 中的 `proxy` 步骤说明

## 5. Web API 参考

### 5.1 启用方式

常见做法是在动态库 `sdk_init` 的初始化 JSON 中携带 `port`，一次完成 SDK 初始化并启用内嵌 Web API：

```json
{
  "userSig": "your-user-sign",
  "port": 9527
}
```

兼容入口 `sdk_init_webapi(port)` 只用于先启动本地 Web API 服务；业务初始化仍需随后调用 `POST /sdk/v1/init` 并传入 `userSig`。

初始化完成后：

- Web API HTTP 地址：`http://127.0.0.1:{port}`

- WebSocket 地址：`ws://127.0.0.1:{port}/`

- HTTP 请求与 WebSocket 使用同一个本地 TCP 端口

`port` 的当前语义：

| 值    | 行为                                                                     |
| ----- | ------------------------------------------------------------------------ |
| 不传  | 不启用内嵌 Web API                                                       |
| `0`   | SDK 自动分配本机空闲端口，实际端口在初始化响应的 `data.port` 中返回      |
| `> 0` | SDK 尝试监听 `127.0.0.1:{port}`，端口不可用时返回 `CL_EPORT_UNAVAILABLE` |

### 5.2 认证模型

内嵌 Web API 当前不依赖 `Authorization` Header。

当前模型如下：

- `/sdk/v1/init` 在 JSON body 中携带 `userSig`

- 后续 Web API 请求复用当前已初始化的 SDK 实例

- `/sdk/v1/token/update` 在 JSON body 中刷新 `userSig`

### 5.3 WebSocket 使用说明

WebSocket 是 Web API 的异步事件通道。调用异步接口（`browser/install`、`browser/open`、`browser/close`、`token/update`）时，HTTP 响应只返回受理 ACK；最终进度与结果通过 WebSocket 推送。

#### 连接地址

```plaintext
ws://127.0.0.1:{port}/


```

当前实现不区分连接 path，`/` 与任意子路径均可接受。

#### 消息格式

- 帧类型：UTF-8 JSON 文本帧

- 结构与 `sdk_result_cb_t` 回调收到的 JSON 一致（参见 §3.5 / §3.7）

- 常规 SDK 事件通常包含 `type`（事件名称）、`code`（返回码）和 `reqId` 字段；异常响应可能只在 `data` 中携带错误信息

#### 请求与事件关联

HTTP ACK 中的 `reqId > 0` 时，可用于匹配后续 WebSocket 事件。若 `reqId` 为 `0`，不要依赖它做精确关联，应结合 `type`、`envId` 和业务上下文判断事件归属。

#### 连接生命周期

| 场景                     | 行为                                                   |
| ------------------------ | ------------------------------------------------------ |
| 未启用 Web API 前        | 本地服务未启动，TCP 连接被拒绝                         |
| 连接建立时已有浏览器运行 | SDK 立即推送一次运行状态快照，可用于恢复 UI 显示       |
| 多个客户端并发连接       | 支持；所有 WebSocket 事件广播到全部已连接客户端        |
| 客户端断开               | SDK 不缓冲断线期间的事件；重连后可再次收到运行状态快照 |
| SDK shutdown             | SDK 主动关闭所有 WebSocket 连接                        |

#### 推荐接入顺序

1.  `sdk_init` 返回成功

2.  建立 WebSocket 连接，准备接收事件（可能立即收到运行状态快照）

3.  发送 HTTP 请求，例如 `POST /sdk/v1/browser/open`

4.  HTTP 响应返回 ACK，记录 `reqId`（若不为 `0`）

5.  通过 WebSocket 接收最终事件，如 `browser-open-success` 或 `browser-open-failed`

#### 主动推送事件

以下事件由 SDK 主动推送，不需要客户端先发起请求：

| 事件                                        | 触发场景                           |
| ------------------------------------------- | ---------------------------------- |
| `sdk-token-expire-warning`                  | token 剩余有效期低于阈值           |
| `sdk-token-expired`                         | token 已过期                       |
| `browser-close-success`（含 `closeOrigin`） | 浏览器进程意外退出或被用户手动关闭 |

#### 通过 WebSocket 发送请求

WebSocket 帧中可包含带 `path` 字段的请求体，SDK 会按异步任务调度处理。当前实现所有 WebSocket 请求均视为异步，不区分同步接口语义。接入侧建议统一使用“HTTP 请求 + WebSocket 监听”的模式，语义更清晰。

### 5.4 Web API 同步与异步接口

| 接口                                                                                         | HTTP 响应语义                                                                                   | 是否需要 WebSocket     |
| -------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------- | ---------------------- |
| `init`、`info`、`browser/info`、`netdiag`、`proxydiag`、`env/netdiag`                        | 返回本次调用结果；`netdiag` 和 `env/netdiag` 是原始诊断 JSON，`proxydiag` 成功时是标准 envelope | 不需要                 |
| `browser/cleanup`                                                                            | 返回本次清理结果                                                                                | 不需要                 |
| `env/create`、`env/update`、`env/page`、`env/getinfo`、`env/getcookiehistory`、`env/destroy` | 返回后端原始结果                                                                                | 不需要                 |
| `browser/install`、`browser/open`、`browser/close`、`token/update`                           | 返回受理 ACK                                                                                    | 需要，用于接收最终结果 |

### 5.5 `POST /sdk/v1/init`

同步初始化 SDK。

请求字段：

| 字段               | 类型    | 必填 | 说明                                                                   |
| ------------------ | ------- | ---- | ---------------------------------------------------------------------- |
| `userSig`          | string  | 是   | 后端签发的用户令牌                                                     |
| `workDir`          | string  | 否   | 工作目录根路径，实际运行目录会被解析为 `workDir/appId`                 |
| `extsDir`          | string  | 否   | 本地扩展包根目录；不传时默认使用实际运行目录下的 `extensions` 目录     |
| `port`             | integer | 否   | 内嵌 Web API 端口；不传则不开启，传 `0` 自动分配空闲端口               |
| `sdkApiUrl`        | string  | 否   | 覆盖 SDK 后端地址；不传则使用 SDK 内置默认地址                         |
| `autoUpdateKernel` | bool    | 否   | 覆盖 SDK 后端配置的`autoUpdateKernel`参数                              |
| `logoPath`         | string  | 否   | 覆盖 SDK 后端配置用户图标资源，使用本地图标资源(绝对路径)              |
| `debug`            | bool    | 否   | 开启开发者日志，默认 `false`                                           |
| `verbose`          | bool    | 否   | 开启详细日志（输出更细粒度的内部事件），默认 `false`；生产环境建议关闭 |

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

扩展目录说明：

- `extsDir` 只在初始化阶段用于扫描本地扩展包，不是 `browser/open` 的动态安装入口

- `extsDir` 为空时，SDK 会使用实际运行目录下的 `extensions` 目录，例如 `workDir/appId/extensions`

- 目录结构支持 `extsDir/{extension}/manifest.json`，也支持 `extsDir/{extension}/{version}/manifest.json`

- 扩展 `manifest.json` 必须包含 `key`，SDK 会按 Chrome 扩展 ID 算法从该 `key` 生成扩展 ID

- 当前只加载 Manifest V3 扩展；`manifest_version <= 2` 的扩展会被忽略

当前实现的重要限制：

- `sdk_init` 在同一进程内是全局串行入口

- 若已有初始化操作进行中，后续调用直接返回 `CL_WBUSY`

### 5.6 `POST /sdk/v1/info`

同步获取 SDK 运行信息。

请求体：

- 推荐传空对象 `{}`

- 空 body 也可

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
      "version": "1.0.1.0",
      "startupTime": 1744123456789,
      "coresInfo": {},
      "netInfo": {},
      "gpuInfo": {
        "vendor": "nvidia",
        "devices": [
          {
            "vendor": "nvidia",
            "name": "NVIDIA GeForce RTX 4060",
            "vendorId": 4318,
            "deviceId": 10307,
            "primary": true
          }
        ]
      },
      "workDir": "C:/BroSDK/1234567890",
      "tokenExpiresInS": 3600,
      "dataFullyManaged": true
    },
    "eventId": 10131
  }
}
```

`data.info` 常用字段：

| 字段               | 类型    | 说明                                                                                                                                           |
| ------------------ | ------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| `deviceId`         | string  | 当前机器指纹                                                                                                                                   |
| `version`          | string  | SDK 版本号                                                                                                                                     |
| `startupTime`      | integer | SDK 启动时间戳                                                                                                                                 |
| `coresInfo`        | object  | 已加载浏览器核心信息                                                                                                                           |
| `netInfo`          | object  | 当前网络环境快照                                                                                                                               |
| `gpuInfo`          | object  | 当前系统 GPU 信息；`vendor` 为主 GPU 厂商，稳定值为 `amd`、`apple`、`intel`、`nvidia`、`qualcomm` 或 `unknown`；`devices` 包含全部探测到的 GPU |
| `workDir`          | string  | 实际运行目录                                                                                                                                   |
| `tokenExpiresInS`  | integer | 当前 token 剩余有效秒数                                                                                                                        |
| `dataFullyManaged` | bool    | `true` 为全托管，`false` 为半托管                                                                                                              |

### 5.7 `POST /sdk/v1/netdiag`

同步执行网络 / 代理诊断。适用于浏览器启动前验证 `proxy` 与跳板链路可达性；调用方直接获取诊断结果，无需等待 WebSocket 事件。

请求字段：

| 字段                   | 类型           | 必填         | 说明                                                                                                                     |
| ---------------------- | -------------- | ------------ | ------------------------------------------------------------------------------------------------------------------------ |
| `url`                  | string         | 是           | 需要探测的目标 URL                                                                                                       |
| `proxy`                | string         | 否           | 最终上游代理；为空时按直连诊断                                                                                           |
| `bridgeProxy`          | string         | 否           | 兼容模式下的前置跳板；带 `mode` 时也作为推荐候选输入                                                                     |
| `mode`                 | string         | 否           | 不传或 `legacy` 为旧单链模式；`recommend` 复用浏览器启动候选策略；`probe` 按 `routes` 逐条验证                           |
| `forward`              | string         | 否           | 仅 `recommend` 使用；有值时作为强制前置跳板，绕过系统网络选择                                                            |
| `useSystemProxy`       | bool           | 否           | `recommend` 是否加入系统代理前置候选，默认 `true`                                                                        |
| `bridgeProxyMode`      | string         | 否           | `recommend` 的 `fallback`、`always`、`disabled` 或 `fallback-as-proxy`                                                   |
| `probeAll`             | bool           | 否           | 是否继续验证低优先级候选；默认首条目标可达链成功后停止                                                                   |
| `timeoutMs`            | integer        | 否           | 每条代理端点握手的超时，范围 `500-10000`，默认 `2500`                                                                    |
| `targetTimeoutMs`      | integer        | 否           | 每条链路目标 URL 探测的超时，范围 `500-10000`，默认 `5000`                                                               |
| `expectedNetworkEpoch` | integer/string | 否           | 只接受指定网络快照 epoch；不一致返回 `CL_WBUSY`，避免依据过期链路做决定                                                  |
| `routes`               | array          | `probe` 必填 | 自定义链路列表，最多 4 条；每项为 `{id, role, proxy, preceding, precedingRoles}`，`preceding` 按客户端到最终代理顺序排列 |

`netdiag` 是独立诊断接口，不读取环境详情，也不会复用运行中浏览器的实际 bridge。以下字段映射只适用于省略 `mode` 或 `mode="legacy"` 的兼容模式：

- `forward + proxy`：把环境 `proxy` 放到 `proxy`，把本次 `forward` 放到 `bridgeProxy`

- 只有 `forward`、没有环境 `proxy`：把本次 `forward` 放到 `proxy`，`bridgeProxy` 留空

- `bridge + proxy`：把环境 `proxy` 放到 `proxy`，把本次 `bridge` 放到 `bridgeProxy`

兼容模式请求示例：

```json
{
  "url": "https://example.com",
  "proxy": "socks5://target-proxy:5206",
  "bridgeProxy": "socks5://jump-proxy:31034"
}
```

兼容模式成功响应示例：

```json
{
  "code": 0,
  "msg": "ok",
  "ok": true,
  "data": {
    "request": {
      "url": "https://example.com",
      "proxy": { "raw": "socks5://****@target-proxy:5206", "valid": true },
      "bridgeProxy": { "raw": "socks5://****@jump-proxy:31034", "valid": true }
    },
    "chain": {
      "targetProxy": {
        "raw": "socks5://****@target-proxy:5206",
        "valid": true
      },
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

`bridgeDiagnostics`、`urlProbe` 和 `events[ ].diagnostics` 是诊断明细，字段会随底层网络探测能力扩展；接入侧应优先判断顶层 `ok/code/msg`，再展示或记录明细。

与 `env/netdiag` 不同，`netdiag` 的顶层 `ok` 直接表示本次目标 URL 探测是否成功：代理 bridge 启动成功但 `urlProbe` 失败时，仍然是 `ok=false`。在兼容模式中应同时查看 `data.started`、`data.runningAfterStart`、`data.error` 和 `data.urlProbe`，不要只根据临时监听端口存在就判定链路可用。

#### 5.7.1 候选链路推荐与显式探测

`mode="recommend"` 复用浏览器启动时的代理候选选择器。没有 `forward` 时，典型候选为：

```text
proxy
systemProxy -> proxy       (when system proxy is composable)
bridgeProxy                 (fallback-as-proxy)
```

`forward` 是强制链路：同时有 `proxy` 时形成 `forward -> proxy`；没有 `proxy` 时，`forward` 自身作为最终代理。`proxy` 和 `forward` 都为空时，接口诊断直连，或在提供 `bridgeProxy` 时诊断该候选。返回的 `networkEpoch`、`routeSelection` 和 `routes[]` 描述候选、端点握手与目标 URL 探测结果。代理端点握手成功只说明该节点可连接，目标地址是否可用必须看对应 `urlProbe.ok`。

`mode="probe"` 允许调用方显式提交最多 4 条自定义链路进行比较，不修改浏览器配置：

```json
{
  "mode": "probe",
  "url": "https://example.com",
  "probeAll": true,
  "routes": [
    { "id": "direct-proxy", "role": "proxy", "proxy": "socks5://proxy-a:1080" },
    {
      "id": "forward-proxy",
      "role": "forward->proxy",
      "proxy": "socks5://proxy-a:1080",
      "preceding": ["socks5://forward-a:1080"],
      "precedingRoles": ["forward"]
    }
  ]
}
```

`recommend` 和 `probe` 使用 `brosdk-netdiag-v2` 响应：

| 字段                       | 说明                                                                                                                     |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| 顶层 `ok/code/msg`         | 与 `env/netdiag` 不同，这里直接表示是否至少有一条候选链成功到达目标 URL；全部失败时通常为 `ok=false`、`code=CL_ENETWORK` |
| `data.networkEpoch`        | 本次读取的本地网络快照版本                                                                                               |
| `data.networkEpochMatched` | 未传 `expectedNetworkEpoch` 或 epoch 一致时为 `true`；不一致会在探测前返回 `CL_WBUSY`                                    |
| `data.routeSelection`      | `recommend` 模式的候选选择器证据；`probe` 或简单直连场景可能不存在                                                       |
| `data.routes[]`            | 实际探测过的候选；包含 `id`、`role`、脱敏节点、`ok`、bridge 状态、`error` 和 `urlProbe`                                  |
| `data.selectedIndex`       | 第一条目标探测成功的候选下标；没有候选成功时为 `-1`                                                                      |
| `data.verdict`             | `target_reachable` 或 `no_route_reached_target`                                                                          |
| `data.error`               | 第一条已记录失败原因的摘要；详细证据仍应查看每条 route                                                                   |

`probeAll=false` 时，第一条成功候选之后的低优先级候选不会继续探测；缺失的候选不能解释为失败。`probeAll=true` 适合诊断界面对比链路，但会增加同步调用耗时。

本接口只报告和比较链路，不安装代理、不修改浏览器设置，也不会把“代理端点握手成功”解释为“任意目标都能访问”。调用方可先使用 `sdk_system_proxy_diagnostics` 或 `POST /sdk/v1/proxydiag` 获取当前系统代理跳和 `networkEpoch`，再把该 epoch 作为 `expectedNetworkEpoch` 提交，避免使用网络切换前的旧快照做选路决定。

兼容模式说明：省略 `mode` 或显式传 `legacy` 时，接口仍使用旧的单链请求形状。这时 `bridgeProxy` 表示前置跳板，`proxy` 表示最终代理。只有在兼容模式下，诊断启动参数里的 `forward`/`bridge` 才需要人工映射到这两个字段；新接入优先使用 `mode="recommend"` 和显式 `forward`。

#### 5.7.2 `POST /sdk/v1/proxydiag`

同步读取调用时的系统代理与本地网络摘要。请求体建议传 `{}` 或空 body。该接口不探测目标 URL、不选择浏览器链路，也不修改已经运行中的浏览器配置；它用于回答“当前系统代理能否作为 SDK 临时代理桥的一个节点”。

Web API 使用标准 BroSDK envelope，诊断对象位于 `data.systemProxy`：

```json
{
  "code": 0,
  "reqId": 123456789,
  "type": "sdk-netdiag-success",
  "msg": "ok",
  "data": {
    "systemProxy": {
      "schemaVersion": "brosdk-proxydiag-v2",
      "kind": "system_proxy_diagnostics",
      "status": "socks5",
      "route": "socks5://127.0.0.1:7890",
      "resolutionKnown": true,
      "nativeFallback": false,
      "bridgeSupported": true,
      "composable": true,
      "nativeOnly": false,
      "networkEpoch": 12,
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
      }
    },
    "eventId": 10151
  }
}
```

如果系统代理诊断调用自身失败，Web API 返回 `code/msg/ok=false/data.error` 原始错误 JSON，不会出现 `data.systemProxy`。因此客户端应先判断是否存在顶层 `ok=false`，再按成功 envelope 解析。

关键字段：

| 字段                                 | 说明                                                                                                                                                |
| ------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `status`                             | 系统代理识别结果，例如 `none`、`http`、`https_as_http`、`socks5`、`unsupported_pac`、`unsupported_auto_detect`、`unsupported_socks4` 或 `invalid_*` |
| `route`                              | 可交给本地代理桥组合的代理 URL；为空不一定表示本机无网络，也可能是 PAC/WPAD 或当前不支持解析的系统代理                                              |
| `resolutionKnown`                    | SDK 是否解析出确定的代理路由                                                                                                                        |
| `nativeFallback`                     | 是否应保留 Chromium/系统原生代理栈处理，而不是强行当作直连或桥接失败                                                                                |
| `bridgeSupported`                    | 当前系统代理类型是否受本地代理桥支持                                                                                                                |
| `composable`                         | `bridgeSupported && resolutionKnown`，表示可作为 `systemProxy -> proxy` 等显式链路节点                                                              |
| `nativeOnly`                         | 当前配置只能依赖系统原生代理处理，不能可靠转换为本地 bridge 节点                                                                                    |
| `networkEpoch`                       | 本地网络快照版本；网络切换后可能变化，可交给 `netdiag.expectedNetworkEpoch` 做一致性校验                                                            |
| `egressProbeOk` / `egressCapability` | 本地网络探测摘要，不代表任意目标 URL 可达                                                                                                           |
| `systemProxy`                        | 固定代理、PAC、自动发现和 bypass 规则等原始摘要                                                                                                     |

动态库 `sdk_system_proxy_diagnostics` 直接返回上述 `data.systemProxy` 对象，内存通过 `sdk_free()` 释放。排障时，`proxydiag` 说明“系统代理当前是什么、能否组合”；`netdiag` 说明“给定候选链路能否到达指定 URL”；`env/netdiag` 说明“某个环境服务端配置经过一次性临时 bridge 后是否稳定到达指定 URL”。三者不能互相替代。

### 5.8 `POST /sdk/v1/browser/info`

同步获取当前运行中的浏览器列表。

请求体：

- SDK 层返回全部运行中环境，不支持请求体过滤

- 建议传 `{}` 或空 body

成功响应示例：

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

### 5.9 `POST /sdk/v1/browser/install`

异步安装浏览器核心资源。请求会根据初始化凭证中的核心版本列表匹配可安装包，下载 / 安装完成后 SDK 会重新加载本地核心列表。

请求字段：

| 字段    | 类型  | 必填 | 说明                     |
| ------- | ----- | ---- | ------------------------ |
| `cores` | array | 是   | 需要安装的浏览器核心列表 |

| `cores[ ].major` | integer / string | 是 | 浏览器核心主版本号，例如 `134` |

请求示例：

```json
{
  "cores": [{ "major": 134 }]
}
```

即时 ACK 示例：

```json
{
  "code": 0,
  "reqId": 0,
  "type": "browser-install",
  "msg": "accepted",
  "data": {
    "eventId": 20350,
    "accepted": true,
    "async": true,
    "dispatchCode": 1,
    "dispatchMsg": "done"
  }
}
```

最终异步事件：

- `browser-install`

- `browser-install-progress`

- `browser-install-success`

- `browser-install-failed`

### 5.10 `POST /sdk/v1/browser/open`

异步打开一个或多个浏览器环境。

顶层请求字段：

| 字段   | 类型  | 必填 | 说明             |
| ------ | ----- | ---- | ---------------- |
| `envs` | array | 是   | 要打开的环境列表 |

`envs[ ]` 支持三种写法：

- 直接传字符串环境 ID

- 直接传数字环境 ID

- 传对象，对象中包含 `envId` 和可选覆盖参数

`envs[ ]` 对象字段：

| 字段         | 类型             | 必填 | 说明                                                                                                                                       |
| ------------ | ---------------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------ |
| `envId`      | string / integer | 是   | 环境 ID                                                                                                                                    |
| `forward`    | string           | 否   | 本次启动使用的显式代理；优先级最高，高于 `bridge` 和环境绑定的 `bridgeProxy`；有环境 `proxy` 时作为前置跳板，无环境 `proxy` 时作为最终代理 |
| `bridge`     | string           | 否   | 本次启动使用的备用前置跳板；非空时覆盖环境绑定的 `bridgeProxy`，优先级低于 `forward`                                                       |
| `args`       | array            | 否   | Chromium 兼容命令行参数，每项为完整 switch                                                                                                 |
| `urls`       | array            | 否   | 启动后自动打开的 URL                                                                                                                       |
| `extensions` | array            | 否   | 传给已加载扩展的本次启动数据配置                                                                                                           |
| `cookies`    | array            | 否   | 本次启动注入的 Cookie JSON 数组                                                                                                            |
| `yunConfig`  | object           | 否   | 定制浏览器透传内容                                                                                                                         |

`browser/open` 不支持通过请求体覆盖环境绑定的 `proxy`。`bridgeProxy` 建议在创建 / 更新环境时绑定；如果只想覆盖本次启动的备用跳板，请在 `browser/open` 中传入 `bridge`。如果本次启动必须强制使用某个代理，请传入 `forward`。

`args[ ]` 仅描述 Chromium 兼容命令行参数，每项应是完整字符串，例如：

- `--no-first-run`

- `--no-default-browser-check`

- `--disable-web-security`

- `--remote-allow-origins=*`

- `--remote-debugging-port=0`

`extensions[ ]` 对象字段：

| 字段   | 类型                  | 必填 | 说明                                                     |
| ------ | --------------------- | ---- | -------------------------------------------------------- |
| `id`   | string                | 是   | 已从 `extsDir` 加载的 Chrome 扩展 ID                     |
| `data` | object<string,string> | 否   | 传给扩展的键值数据；键和值均为字符串，具体编码由扩展约定 |

扩展加载与传参是两个阶段：

- `sdk_init` 阶段通过 `extsDir` 扫描并加载本地扩展包

- `browser/open` 阶段的 `extensions[ ]` 不会安装新扩展，只会按 `id` 匹配已加载扩展，并把 `data` 透传给该扩展

- 如果 `extensions[ ].id` 没有匹配到初始化阶段已加载的扩展，该项会被忽略

- 扩展 ID 来自扩展 `manifest.json` 中的 `key`；请保持同一扩展的 `key` 稳定，否则生成的 ID 会变化

`cookies[ ]` 对象字段兼容浏览器 Cookie JSON：

| 字段             | 类型          | 必填 | 说明                                             |
| ---------------- | ------------- | ---- | ------------------------------------------------ |
| `domain`         | string        | 是   | Cookie 域                                        |
| `name`           | string        | 是   | Cookie 名称                                      |
| `value`          | string        | 是   | Cookie 值                                        |
| `path`           | string        | 是   | Cookie 路径，通常为 `/`                          |
| `expirationDate` | number        | 否   | 过期时间，Unix 秒时间戳；会话 Cookie 可不传      |
| `hostOnly`       | bool          | 否   | 是否仅主机匹配                                   |
| `httpOnly`       | bool          | 否   | 是否 HttpOnly                                    |
| `sameSite`       | string / null | 否   | 可为 `lax`、`strict`、`no_restriction` 或 `null` |
| `secure`         | bool          | 否   | 是否 Secure                                      |
| `session`        | bool          | 否   | 是否会话 Cookie                                  |
| `storeId`        | string / null | 否   | Cookie store ID                                  |

`yunConfig{}` 对象字段：

在yunConfig JSON对象内容会平铺透传到定义版浏览器，例如 `shop{}`、`whitelist[ ]`、`blacklist[ ]` ...

| 字段        | 类型   | 必填 | 说明                 |
| ----------- | ------ | ---- | -------------------- |
| `shop`      | object | 否   | 咨询定制版浏览器厂商 |
| `whitelist` | array  | 否   |                      |
| `blacklist` | array  | 否   |                      |

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
        "--no-default-browser-check",
        "--disable-web-security",
        "--remote-allow-origins=*"
      ],
      "urls": ["https://myip.ipipv.com"],
      "yunConfig": {
        "shop": {
          "shopId": "cd9ff4d2e44746a5ab58b56c546dfcc6",
          "name": "141",
          "shortName": "27008",
          "platform": "",
          "serial": "27008"
        },
        "whitelist": ["www.baidu.com"],
        "blacklist": ["www.cn.bing.com"]
      },
      "extensions": [
        {
          "id": "jicbihcejeehghnlckloefbklclkkbei",
          "data": {
            "key1": "aGVsbG8=",
            "key3": "5L2g5aW9",
            "key2": "12345234634574568478asdfdgsdfg"
          }
        },
        {
          "id": "afgbmmdnakcefnkchckgelobigkbboci",
          "data": {
            "data1": "5Zyo5ZCX77yfCg==",
            "ddataTest22345": "d2VsY29tZQo=",
            "testData": "!@#%^@$#^#$%&#%^*$%^*$^&*%^&*"
          }
        }
      ],
      "cookies": [
        {
          "domain": "www.baidu.com",
          "expirationDate": 1776502336,
          "hostOnly": true,
          "httpOnly": false,
          "name": "BD_UPN",
          "path": "/",
          "sameSite": null,
          "secure": false,
          "session": false,
          "storeId": null,
          "value": "123134753"
        },
        {
          "domain": ".baidu.com",
          "expirationDate": 1805640689.802383,
          "hostOnly": false,
          "httpOnly": false,
          "name": "BAIDUID",
          "path": "/",
          "sameSite": "no_restriction",
          "secure": true,
          "session": false,
          "storeId": null,
          "value": "7CC918200AED607C30178FC6F821F9D48:FG=1"
        }
      ]
    }
  ]
}
```

即时 ACK 示例：

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

最终异步事件：

- `browser-open`

- `browser-open-success`

- `browser-open-failed`

- `browser-open-timeout`

关键语义：

> `browser-open-success` 表示浏览器已完全启动且就绪。 Cookie / Storage / 扩展 / 自动化逻辑应以此通知作为就绪信号。

如果代理桥启动失败或运行中代理探测失败，但浏览器最终仍然启动成功，最终事件仍是 `browser-open-success`，但 `code` 可以是 `CL_WPROXYDEGRADED`。这类“成功但有降级”的情况会在后端上报日志中以 `Warn` 级别和 `extra.lifecycle.steps[ ].name="proxy"` 的步骤明细体现。

`whiteList` / `blackList` 会随环境配置写入浏览器侧配置；具体命中策略由当前浏览器核心实现决定。`extensions[ ].data` 与 `cookies[ ]` 可能包含敏感数据，接入层应限制数组长度、条目长度和字段格式，并在日志中做脱敏处理。

### 5.11 `POST /sdk/v1/browser/close`

异步关闭一个或多个浏览器环境。

请求字段：

| 字段   | 类型  | 必填 | 说明             |
| ------ | ----- | ---- | ---------------- |
| `envs` | array | 是   | 要关闭的环境列表 |

`envs[ ]` 支持：

- 字符串环境 ID

- 数字环境 ID

- 仅包含 `envId` 的对象

即时 ACK 示例：

```json
{
  "code": 0,
  "reqId": 1677842284,
  "type": "browser-close",
  "msg": "accepted",
  "data": {
    "eventId": 20140,
    "accepted": true,
    "async": true,
    "dispatchCode": 1,
    "dispatchMsg": "done"
  }
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

如果浏览器是被用户手动关窗，或者进程在运行中自行退出，SDK 仍会发送 `browser-close-success`，但会在 `data` 中附带关闭原因：

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
    "progress": 100,
    "closeReasonCode": 103,
    "closeReasonName": "WBRWPROCEXITED",
    "closeReasonMsg": "browser process exited unexpectedly",
    "closeOrigin": "process-exited"
  },

  "envList": []
}
```

关键语义：

> 即时 ACK 只表示关闭任务已被受理。 只有收到 `browser-close-success`，才表示环境已关闭完成。

### 5.12 `POST /sdk/v1/browser/cleanup`

同步清理本地浏览器缓存。该接口同时支持清理指定环境的 `userdatadir` 缓存，以及浏览器内核下载缓存 `cores/.cache`。

请求字段：

| 字段             | 类型             | 必填     | 说明                                                                                   |
| ---------------- | ---------------- | -------- | -------------------------------------------------------------------------------------- |
| `envs`           | array            | 否       | 要清理 `userdatadir` 缓存的环境 ID 列表，最多 128 个                                   |
| `cores`          | array            | 否       | 要清理的内核下载缓存列表；字段缺省表示不清理内核缓存，空数组表示清理全部 `.cache` 文件 |
| `cores[ ].major` | integer / string | 条件必填 | 要清理的浏览器核心主版本号，例如 `141`                                                 |
| `cores[ ].type`  | string           | 否       | 内核类型过滤，例如 `chrome`；不传则仅按 `major` 匹配                                   |

`envs` 和 `cores` 至少需要提供一个有效清理目标；两者可以同时提供。

`envs[ ]` 支持：

- 字符串环境 ID

- 数字环境 ID

请求示例：

```json
{
  "envs": ["2041415694746128384", "2041415694746128385"]
}
```

清理指定内核版本缓存：

```json
{
  "cores": [
    {
      "major": 141
    }
  ]
}
```

清理全部内核下载缓存：

```json
{
  "cores": []
}
```

同时清理环境 `userdatadir` 和内核下载缓存：

```json
{
  "envs": ["2041415694746128384"],
  "cores": [
    {
      "major": 141
    }
  ]
}
```

成功响应示例：

```json
{
  "code": 0,
  "msg": "ok",
  "data": {
    "deleted": 1,
    "notFound": 1,
    "busy": 0,
    "invalid": 0,
    "failed": 0,
    "results": [
      {
        "envId": "2041415694746128384",
        "code": 0,
        "msg": "ok",
        "status": "deleted",
        "userDataDir": "C:/BroSDK/1234567890/userdata/2041415694746128384"
      },
      {
        "envId": "2041415694746128385",
        "code": 0,
        "msg": "ok",
        "status": "not_found",
        "userDataDir": "C:/BroSDK/1234567890/userdata/2041415694746128385"
      }
    ],
    "coreCache": {
      "deleted": 1,
      "notFound": 0,
      "invalid": 0,
      "failed": 0,
      "filesDeleted": 3,
      "filesFailed": 0,
      "results": [
        {
          "all": false,
          "major": 141,
          "code": 0,
          "msg": "ok",
          "status": "deleted",
          "filesDeleted": 3,
          "filesFailed": 0
        }
      ]
    }
  }
}
```

关键语义：

- `browser/cleanup` 删除指定环境的本地 `userdatadir` 缓存时，不销毁后端环境记录

- `cores` 只删除内核下载缓存 `cores/.cache` 中的 `.zip`、`.part`、`.merge` 文件，不删除已安装内核目录

- `cores` 字段缺省表示不处理内核缓存；`"cores": []` 表示清理全部 `.cache` 缓存；`"cores": [{"major": 141}]` 表示只清理指定主版本缓存

- 如果指定环境正在运行，或仍处于 `browser/open` / `browser/close` 队列中，该环境返回 `CL_EBUSY`，结果项状态为 `busy`

- 如果请求中任一环境忙碌，顶层 `code` 返回 `CL_EBUSY`；如果存在非法环境 ID 或非法 `cores` 项，顶层 `code` 返回 `CL_EINVALID`

- `not_found` 表示该环境本地缓存目录不存在，按清理完成处理

### 5.13 `POST /sdk/v1/token/update`

异步刷新 `userSig`。

请求字段：

| 字段      | 类型   | 必填 | 说明         |
| --------- | ------ | ---- | ------------ |
| `userSig` | string | 是   | 新的用户令牌 |

即时 ACK 示例：

```json
{
  "code": 0,
  "reqId": 1587620091,
  "type": "sdk-token-update",
  "msg": "accepted",
  "data": {
    "eventId": 10120,
    "accepted": true,
    "async": true,
    "dispatchCode": 1,
    "dispatchMsg": "done"
  }
}
```

### 5.14 `POST /sdk/v1/env/create`

同步创建环境。

当前行为：

- 请求体直接转发到后端 `env/create`

- 响应体直接返回后端原始 JSON

- 不追加 BroSDK 自己的 envelope

参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/create` 接口契约。

### 5.15 `POST /sdk/v1/env/update`

同步更新环境。

当前行为：

- 请求体直接转发到后端 `env/update`

- 响应体直接返回后端原始 JSON

参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/update` 接口契约。

### 5.16 `POST /sdk/v1/env/page`

同步分页查询环境。

当前行为：

- 请求体直接转发到后端 `env/page`

- 响应体直接返回后端原始 JSON

参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/page` 接口契约。

### 5.17 `POST /sdk/v1/env/getinfo`

同步获取单个环境的后端 `getEnvInfo` 结果。

当前行为：

- 请求体直接转发到后端 `getEnvInfo`

- 响应体直接返回后端原始 JSON

- SDK 打开浏览器时也会基于该结果构造环境配置，因此这里返回的字段通常比分页列表更完整

- `envId` 会实际请求服务端；即使调用方刚从本地环境列表或缓存得到该 ID，服务端仍可能返回环境不存在。应按后端返回码处理，不要把本地列表命中当成 `getinfo` 成功

请求字段：

| 字段    | 类型             | 必填 | 说明                                               |
| ------- | ---------------- | ---- | -------------------------------------------------- |
| `envId` | string / integer | 是   | 环境 ID                                            |
| `os`    | string           | 否   | 当前系统标识，建议传 `Windows`、`macOS` 或 `Linux` |

请求示例：

```json
{
  "envId": "2041695386304778240",
  "os": "Windows"
}
```

### 5.18 `POST /sdk/v1/env/getcookiehistory`

同步查询指定环境的后端 Cookie 历史记录。该接口查询的是服务端历史数据，不读取当前运行浏览器的实时 Cookie，也不执行浏览器或网络代理操作。

请求字段：

| 字段    | 类型             | 必填 | 说明    |
| ------- | ---------------- | ---- | ------- |
| `envId` | string / integer | 是   | 环境 ID |

请求示例：

```json
{
  "envId": "2063888366256001024"
}
```

当前行为：

- 请求由 SDK 规范化为后端 `getCookieHistory` 请求，并实际访问服务端
- 成功或后端业务失败时，响应体直接返回后端原始 JSON，不额外套 BroSDK envelope
- 环境不存在、SDK 未初始化、请求体无效或 HTTP 调用失败时，可能返回 SDK 自己的错误码；调用方必须同时解析返回码和 JSON body
- 返回内容可能包含敏感的 Cookie 元数据；除非业务确实需要，不要写入普通日志或长期缓存

### 5.19 `POST /sdk/v1/env/netdiag`

检测一个环境的配置代理链路。每次调用都会通过 `getEnvInfo` 获取服务端环境配置，再创建一次性的临时代理桥检测，检测完成后销毁；不会复用、重配或 attach 到浏览器当前运行中的 bridge。因此环境处于 `Starting`、`Preparing` 或 `Stopping` 时也不会与浏览器生命周期争用代理桥。

请求字段：

| 字段             | 类型             | 必填 | 默认值                        | 说明                                                                                                               |
| ---------------- | ---------------- | ---- | ----------------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `envId`          | string / integer | 是   | -                             | 环境 ID；环境可以已启动、已停止或尚未在本地打开                                                                    |
| `url`            | string           | 是   | -                             | 要检测的 HTTP 或 HTTPS URL，长度为 `1..4096` 字节；建议使用稳定、可重复请求且响应体较小的地址                      |
| `count`          | integer          | 否   | `3`                           | 顺序检测次数，范围 `1..20`                                                                                         |
| `timeoutMs`      | integer          | 否   | `5000`                        | 单次检测超时，范围 `100..10000` 毫秒                                                                               |
| `includeBody`    | boolean          | 否   | `false`                       | 是否在成功结果中返回响应正文                                                                                       |
| `verifyTls`      | boolean          | 否   | `true`                        | HTTPS 是否校验证书链和主机名；仅在排查受控测试证书时使用 `false`                                                     |
| `totalTimeoutMs` | integer          | 否   | `min(25000, count*timeoutMs)` | 总超时，显式传入时范围为 `1..25000` 毫秒；省略时按默认值计算                                                       |
| `forward`        | string           | 否   | 不覆盖                        | 本次诊断使用的显式跳板代理；支持 `http`、`socks5`、`socks5h`。传入后只覆盖临时 bridge，不修改环境配置              |
| `detailLevel`    | string           | 否   | `standard`                    | `summary` 省略逐次结果和候选节点，`standard` 返回结构化诊断，`debug` 额外返回原始 `transport` 与 bridge 运行时证据 |

`includeBody=true` 时，SDK 仍发送 GET，但只在请求正文明确开启时读取并返回正文；`data.lastResponse` 返回最近一次成功请求的状态码和正文，最多 64 KiB。`bodyTruncated=true` 表示达到正文上限，不能据此推断正文完整。正文只返回一次，不在每个 `attempt` 中重复，也不由 SDK 解释；例如出口 IP 检测 URL 返回 IP 文本时，由调用方自行解析。正文可能包含 Cookie、Token 或个人信息，默认不要开启，并避免把返回值写入普通日志。

返回的 `data` 按职责分组：`environment` 表示环境状态；`request` 回显实际参数；`result` 返回机器可判定的聚合结果；`diagnosis` 返回故障层、稳定错误码、置信度和建议是否重试；`network` 返回本地网络和临时路由证据；`attempts` 返回逐次结果。`environment.configFetched=true` 表示已经从服务端获取环境配置。

`result.targetReachable` 表示至少一次通过临时 bridge 成功连接目标主机端口；`httpResponded` 表示至少收到一个合法 HTTP 状态码；`anySucceeded` 表示至少一次收到 HTTP 2xx/3xx；`allSucceeded` 表示全部请求均收到 HTTP 2xx/3xx且没有总超时。HTTP 4xx/5xx 会得到 `targetReachable=true`、`httpResponded=true`、`anySucceeded=false` 和 `diagnosis.layer="http"`，因此不会被误判为代理网络不通。`successLatencyMs` 只统计成功请求，并明确返回样本数。

`network.local` 是调用时重新读取的本机网络摘要，其中 `systemProxy.enabled` 表示系统代理是否开启，`systemProxy.address` 返回固定代理或 PAC 地址；还包括出口网卡、默认网关和 TUN/VPN 状态。地址和认证信息均按日志安全规则脱敏。

`network.observation.startEpoch/endEpoch` 是诊断开始和结束时的网络状态版本；`changedDuringRun=true` 时，运行期间发生了网络/VPN/系统代理切换，`result.inconclusive=true` 且不应把本次结果作为稳定基线。

`network.route.selected.name` 是选中的逻辑链路，例如 `systemProxy->proxy`；`chain` 是适合展示的节点数组；`dynamic=true` 表示备用链路或 bypass 可能让单次连接使用不同路径，`reasonCode` 给出机器可读原因。`verification.status` 使用 `passed`、`failed`、`not_run`，不再使用含义不明确的 nullable boolean。`nodes` 和 `candidates` 保留节点与候选链路证据。

临时诊断路由的 `browserParity` 固定为 `configured_subset`，表示它复现环境配置和选路候选，但不执行 Chromium 启动参数、浏览器代理策略脚本或运行中 bridge 状态；这些边界列在 `parityLimitations` 中。

`diagnosis.layer` 可能为 `none`、`request`、`route`、`system_proxy`、`forward`、`proxy`、`bridge_proxy`、`target_connect`、`tls`、`http`、`timeout`、`multiple` 或 `unknown`。代理认证、目标不可达、TLS、HTTP 状态错误和间歇失败都使用稳定的 `diagnosis.code`。证书过期、未生效、主机名不匹配、证书不受信和 CA 库不可用分别使用 `TLS_CERTIFICATE_EXPIRED`、`TLS_CERTIFICATE_NOT_YET_VALID`、`TLS_HOSTNAME_MISMATCH`、`TLS_CERTIFICATE_UNTRUSTED`、`TLS_CA_STORE_UNAVAILABLE`。每个 `attempt.error` 使用相同的 `layer/code/message/retryable` 结构；只有 `detailLevel=debug` 才返回原始 `transport` 和 `bridgeRuntime`。

#### 5.19.1 先按层级判读，不要只看 `code`

`env/netdiag` 的顶层 `code=0` 只表示“诊断任务已经执行并返回结果”，不表示目标 URL 一定成功。推荐按下面顺序处理：

1. 先判断请求是否得到有效 JSON。SDK 未初始化、环境不存在、`getEnvInfo` 失败、参数无效、临时 bridge 准备失败等属于**执行失败**：响应 `ok=false`，通常没有 `data.result`。这时先修正 SDK、环境 ID 或代理配置，再重试。

2. 执行成功后先读取 `data.result`：

   | 条件                                            | 结论                                               | 下一步                                                                                         |
   | ----------------------------------------------- | -------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
   | `allSucceeded=true`                             | 所有计划请求均收到 2xx/3xx，当前目标和本次链路正常 | 记录延迟即可；不要因为 `route.verification=not_run` 就判为失败                                 |
   | `anySucceeded=true` 且 `allSucceeded=false`     | 间歇性失败，链路可用但不稳定                       | 查看失败的 `attempt`、延迟和 `network.route.dynamic`；建议重试或降低并发                       |
   | `httpResponded=true` 且 `anySucceeded=false`    | 已连接到目标并收到 HTTP 状态，但目标返回 4xx/5xx   | 这是目标服务、权限、URL 或请求条件问题，不要归类为代理不可达                                   |
   | `targetReachable=true` 且 `httpResponded=false` | 已完成目标 CONNECT，但没有收到合法 HTTP 状态       | 优先查看 `diagnosis.layer=tls/http`，定位 TLS 握手、响应超时或 HTTP 传输错误                   |
   | `targetReachable=false`                         | 没有一次确认目标 CONNECT 成功                      | 查看 `diagnosis`、`attempt.error` 和 `network.route.failure`，定位代理节点、路由或目标连接阶段 |
   | `timedOut=true` 或 `skipped>0`                  | 总期限耗尽，未完成计划次数                         | 将结果视为不完整；结合 `attempted`、`skipped` 和具体超时层判断是否重试                         |

3. 再读取 `data.diagnosis`。`layer` 表示故障层，`code` 用于程序分支，`message` 用于展示，`retryable` 决定是否适合自动重试。顶层诊断是聚合结论；要定位某一次失败，使用 `attemptIndex` 或对应的 `attempt.error`。

4. 最后使用 `data.network` 解释路由证据。`route.verification` 只验证代理端点握手，不能替代目标 URL 请求；真正的目标可达性要以 `result` 和 `attempts[].targetConnected/http` 为准。`selected.dynamic=true` 时，实际请求可能因为 fallback 或 bypass 走不同链路，不要把 `selected.name` 当作每一次请求的绝对事实。

可以直接采用下面的判定伪代码：

```text
if top-level ok is false:
    diagnose SDK / envId / getEnvInfo / temporary bridge preparation
else if result.allSucceeded:
    healthy
else if result.httpResponded and not result.anySucceeded:
    target HTTP error; inspect attempts[].http.status
else if result.anySucceeded:
    intermittent; inspect failed attempts and dynamic route
else:
    inspect diagnosis, route.failure, candidates[].failure, and attempts[].error
```

#### 5.19.2 按候选链路和节点定位问题

`network.route.selected.chain` 是展示用的链路顺序，例如 `["systemProxy", "proxy"]`；它表示请求从本机经过系统代理再到最终代理。`nodes` 和 `candidates` 提供证据，但不要把“配置存在”当成“节点可用”：

- `route.verification.status=passed`：代理端点握手成功；仍需检查目标 URL 的 `attempt`。
- `route.verification.status=failed`：路由端点探测失败；优先检查 `route.failure.stage`、`nodeRole`、`error` 和 `message`。
- `route.verification.status=not_run`：直连、未配置可验证代理，或当前策略没有进行端点探测，不代表目标已经成功。
- `route.failure.scope=route`：只能确认整条链路失败，不能可靠归责到某一个节点；`nodeIndex=-1` 是有意保留的“不确定”，`confidence=route` 表示只有链路级证据。
- `route.failure.scope=node`：已经有足够证据定位到节点；`nodeRole` 是 `systemProxy`、`forward`、`bridgeProxy` 或 `proxy`，`nodeIndex` 从 0 开始，`confidence=endpoint` 表示已有端点级证据。
- `candidates[].attempted=false` 或 `result=skipped`：该候选没有被验证，不应显示为“失败”；查看 `skipReason`。
- `selected.dynamic=true`：候选选择包含 fallback/bypass，展示时应写“当前选中/首选链路”，不要写成“所有请求固定经过”。

典型定位顺序：

```text
route.failure.stage=proxy_authentication -> 检查对应节点账号、密码和认证方式
route.failure.stage=proxy_handshake     -> 检查节点协议、host、port 和握手
attempt.error.layer=target_connect      -> 代理已接管，但目标主机/端口不可达
attempt.error.layer=tls                 -> 目标连接后 TLS 协商失败
attempt.error.layer=http                -> 目标已响应，按 HTTP status 排查
attempt.error.layer=route/timeout       -> 检查链路连接、超时和瞬态网络变化
```

`network.local.egressProbeOk`、`egressCapability` 和 `networkEpoch` 是本机网络上下文，不是目标 URL 的健康分数。`systemProxy.enabled=true` 只说明系统代理配置已发现；是否能被临时 bridge 组合，要同时看 `bridgeSupported`、`resolutionKnown` 和 `nativeFallback`。网络发生切换后 `networkEpoch` 可能递增，旧诊断结果不能继续代表新链路。

请求示例：

```json
{
  "envId": "2063888366256001024",
  "url": "https://myip.ipipv.com/",
  "count": 3,
  "timeoutMs": 5000,
  "includeBody": true,
  "verifyTls": true,
  "totalTimeoutMs": 15000,
  "detailLevel": "standard"
}
```

Web API 调用示例：

```powershell
$body = @{
  envId = "2063888366256001024"
  url = "https://myip.ipipv.com/"
  count = 3
  timeoutMs = 5000
  totalTimeoutMs = 15000
  includeBody = $true
  verifyTls = $true
  detailLevel = "standard"
} | ConvertTo-Json

Invoke-RestMethod `
  -Method Post `
  -Uri "http://127.0.0.1:9527/sdk/v1/env/netdiag" `
  -ContentType "application/json" `
  -Body $body
```

接入侧应保存原始 `data`，而不是只保存一行 `msg`：先用 `result` 给出健康/失败结论，再用 `diagnosis` 和 `network.route` 生成排障信息；需要展示出口内容时才读取 `lastResponse.body`。

#### 5.19.3 响应判定与诊断输出伪代码

下面的伪代码描述调用方拿到响应后如何判断和输出结果，不描述 SDK 内部如何创建 bridge。Web API 使用 HTTP 响应体作为输入；C API 使用 `sdk_env_netdiag` 的返回码和 `out_data` 作为输入，两者随后使用同一套判定逻辑。

```text
function diagnoseEnvironmentNetwork(apiResult):
    # apiResult 对 Web API：
    #   {transportOk, httpStatus, body}
    # apiResult 对 C API：
    #   {nativeReturnCode, body}

    # Web API 的 HTTP 状态、C API 的 nativeReturnCode 是调用层结果；
    # response.code/response.ok 是 SDK 业务层结果。不能只检查其中一个。
    if not apiResult.transportOk:
        return output(
            status="execution_error",
            severity="error",
            title="诊断接口调用失败",
            summary="SDK HTTP 或本地动态库调用没有完成",
            details={httpStatus: apiResult.httpStatus,
                     nativeReturnCode: apiResult.nativeReturnCode})

    response = parseJson(apiResult.body)
    if response is invalid:
        return output(
            status="invalid_response",
            severity="error",
            title="诊断接口未返回合法 JSON",
            summary="检查 SDK 返回体和版本是否匹配",
            details={httpStatus: apiResult.httpStatus,
                     nativeReturnCode: apiResult.nativeReturnCode})

    if response.ok != true or response.code != 0:
        data = response.data or {}
        return output(
            status="execution_error",
            severity="error",
            title="环境网络诊断未执行完成",
            summary=data.error or response.msg or "SDK 返回执行错误",
            details={code: response.code,
                     environment: data.environment,
                     diagnosis: data.diagnosis})

    data = response.data
    result = data.result
    if result is missing:
        return output(
            status="invalid_response",
            severity="error",
            title="诊断响应缺少 result",
            summary="SDK 返回结构不符合 env/netdiag 契约",
            details={data: data})

    # 以 result.outcome 作为主结论，以布尔字段做一致性检查和补充说明。
    outcome = result.outcome
    targetReachable = result.targetReachable == true
    httpResponded = result.httpResponded == true
    anySucceeded = result.anySucceeded == true
    allSucceeded = result.allSucceeded == true
    timedOut = result.timedOut == true or result.skipped > 0
    diagnosis = data.diagnosis or {}
    network = data.network or {}
    route = network.route or {}
    selected = route.selected or {}
    attempts = data.attempts or []

    # 1. 全部目标请求成功：代理端点和目标 URL 都完成了实际请求。
    if outcome == "healthy" and allSucceeded:
        status = "healthy"
        severity = "info"
        title = "环境网络正常"
        summary = format(
            "目标请求全部成功（{}/{}），HTTP {}，平均延迟 {} ms",
            result.succeeded, result.attempted + result.skipped,
            successfulHttpStatus(attempts),
            result.successLatencyMs.avg)

    # 2. 目标返回了 HTTP 状态，但没有 2xx/3xx：目标可达，不能归为代理失败。
    else if httpResponded and not anySucceeded:
        status = "target_http_error"
        severity = "warning"
        title = "目标可达，但 HTTP 返回错误"
        summary = format(
            "目标返回 HTTP {}；这通常是 URL、权限或目标服务问题，不是代理不可达",
            responseHttpStatuses(attempts))

    # 3. 有成功也有失败：链路可用，但稳定性不足。
    else if anySucceeded and not allSucceeded:
        status = "degraded"
        severity = "warning"
        title = "环境网络不稳定"
        summary = format(
            "成功 {}/{}，失败 {}，延迟 min/max/avg={} / {} / {} ms",
            result.succeeded, result.attempted + result.skipped,
            result.failed + result.skipped,
            result.successLatencyMs.min, result.successLatencyMs.max,
            result.successLatencyMs.avg)

    # 总期限耗尽表示本次结果不完整；即使没有成功，也不要直接显示成永久不可达。
    else if timedOut:
        status = "timeout"
        severity = "error"
        title = "环境网络检测超时"
        summary = diagnosis.message or format(
            "仅完成 {}/{} 次检测，跳过 {} 次",
            result.attempted, result.attempted + result.skipped, result.skipped)

    # 4. 目标 CONNECT 成功，但没有合法 HTTP 响应：优先看 TLS/HTTP/超时。
    else if targetReachable and not httpResponded:
        status = "target_transport_error"
        severity = "error"
        title = "已连接目标，但未收到 HTTP 响应"
        summary = diagnosis.message or "检查 TLS 握手、HTTP 响应和超时"

    # 5. 一次都没有确认目标 CONNECT 成功：根据 diagnosis 和 route 证据归因。
    else if not targetReachable:
        if diagnosis.layer in ["proxy", "bridge_proxy", "system_proxy", "forward"]:
            status = "route_error"
            title = "代理链路失败"
            summary = diagnosis.message or "代理端点未能建立可用链路"
        else if diagnosis.layer == "target_connect":
            status = "target_unreachable"
            title = "目标主机不可达"
            summary = diagnosis.message or "代理已处理请求，但目标 CONNECT 失败"
        else:
            status = "network_error"
            title = "环境网络不可达"
            summary = diagnosis.message or "没有一次检测确认目标 CONNECT 成功"
        severity = "error"

    else:
        status = mapOutcomeToDisplayStatus(outcome)
        severity = "error"
        title = mapOutcomeToTitle(outcome)
        summary = diagnosis.message or outcome

    proxyCheckStatus = route.verification.status == "passed" ? "pass" :
                       route.verification.status == "not_run" ? "not_run" :
                       "fail"
    checks = [
        check("SDK 诊断任务", response.code == 0, response.msg),
        check("环境配置", data.environment.configFetched == true,
              data.environment.id),
        check("代理端点握手", proxyCheckStatus,
              selected.chain or selected.name),
        check("目标连接", targetReachable, targetReachable ? "可达" : "失败"),
        check("HTTP 响应", httpResponded,
              responseHttpStatuses(attempts) or "无合法状态码"),
        check("请求稳定性", allSucceeded ? "pass" :
              anySucceeded ? "warn" : "fail",
              format("成功 {}/{}，跳过 {}", result.succeeded,
                     result.attempted, result.skipped))
    ]

    # route.verification 只说明代理端点握手；目标 URL 结论必须来自 result/attempts。
    details = {
        "status": status,
        "severity": severity,
        "title": title,
        "summary": summary,
        "environment": data.environment,
        "route": {
            "selected": selected.name,
            "chain": selected.chain,
            "verification": route.verification,
            "failure": route.failure,
            "systemProxyUsed": route.systemProxyUsed
        },
        "diagnosis": diagnosis,
        "result": result,
        "checks": checks,
        "attempts": attempts
    }

    if data.lastResponse exists:
        details.lastResponse = data.lastResponse

    return details
```

成功响应示例（`candidates`、`nodes`、`transport` 会按实际链路补充详细字段）：

```json
{
  "code": 0,
  "msg": "ok",
  "ok": true,
  "data": {
    "environment": {
      "id": "2063888366256001024",
      "status": "NotLoaded",
      "running": false,
      "configFetched": true
    },
    "request": {
      "url": "https://myip.ipipv.com/",
      "count": 3,
      "timeoutMs": 5000,
      "totalTimeoutMs": 15000,
      "includeBody": true,
      "verifyTls": true,
      "forwardOverride": false,
      "detailLevel": "standard"
    },
    "result": {
      "outcome": "healthy",
      "targetReachable": true,
      "httpResponded": true,
      "anySucceeded": true,
      "allSucceeded": true,
      "timedOut": false,
      "inconclusive": false,
      "cleanupTimedOut": false,
      "attempted": 3,
      "succeeded": 3,
      "failed": 0,
      "skipped": 0,
      "elapsedMs": 420,
      "successLatencyMs": { "samples": 3, "min": 110, "max": 160, "avg": 140.0 }
    },
    "diagnosis": {
      "layer": "none",
      "code": "OK",
      "confidence": "high",
      "message": "All target requests succeeded",
      "retryable": false
    },
    "network": {
      "schemaVersion": "brosdk-env-netdiag-network-v2",
      "observation": {
        "startEpoch": 42,
        "endEpoch": 42,
        "changedDuringRun": false,
        "basis": "start_snapshot"
      },
      "local": {
        "connectionType": "Direct",
        "systemProxy": { "enabled": false, "address": "" }
      },
      "route": {
        "source": "temporary_bridge",
        "browserParity": "configured_subset",
        "parityLimitations": [
          "chromium_proxy_args",
          "browser_proxy_policy_script",
          "running_bridge_runtime_state"
        ],
        "selected": {
          "name": "direct",
          "chain": ["direct"],
          "dynamic": false,
          "reasonCode": "fixed"
        },
        "selectionStatus": "configured",
        "verification": { "status": "not_run", "method": "none" },
        "failure": null,
        "candidates": [],
        "nodes": []
      }
    },
    "attempts": [
      {
        "index": 1,
        "outcome": "success",
        "durationMs": 110,
        "targetConnected": true,
        "http": { "status": 200, "successful": true, "bodyBytes": 33 }
      },
      {
        "index": 2,
        "outcome": "success",
        "durationMs": 150,
        "targetConnected": true,
        "http": { "status": 200, "successful": true, "bodyBytes": 33 }
      },
      {
        "index": 3,
        "outcome": "success",
        "durationMs": 160,
        "targetConnected": true,
        "http": { "status": 200, "successful": true, "bodyBytes": 33 }
      }
    ],
    "lastResponse": {
      "status": 200,
      "body": "response body returned by the URL",
      "bodyBytes": 33,
      "bodyTruncated": false
    }
  }
}
```

字段和值说明：

- 顶层 `code` 是 SDK 业务码，`0` 表示诊断任务成功执行；`msg` 是简短说明；`ok=true` 表示 SDK 已完成诊断。它们不代表目标网站一定返回 2xx/3xx，目标访问结论必须读取 `data.result`。
- `data.environment` 描述本地 SDK 对该环境的生命周期视图：`id` 是环境 ID；环境没有进入本地浏览器注册表时为 `NotLoaded`，否则 `status` 可能是 `Idle`、`Downloading`、`Preparing`、`Starting`、`Started`、`Stopping`、`Stopped`、`Destroyed`、`StartFailed`、`StopFailed` 或 `Unknown`；`running` 表示本地浏览器进程是否运行；`configFetched=true` 表示已成功从服务端取得环境配置。该状态不是服务端环境有效性的替代判断。
- `data.request` 是实际生效参数回显：`url` 为检测地址；`count` 为计划次数；`timeoutMs` 为单次超时；`totalTimeoutMs` 为总超时；`includeBody` 表示是否请求响应正文；`verifyTls` 表示是否校验证书链和主机名；`forwardOverride` 表示本次是否用请求中的 `forward` 覆盖环境配置；`detailLevel` 为 `summary`、`standard` 或 `debug`。代理认证信息不会通过回显字段返回。

`data.result` 是给程序判断的汇总结果：

兼容性说明：原有汇总字段没有删除或改名；当前版本新增 `inconclusive` 和 `cleanupTimedOut`，并可能在诊断期间网络切换时返回 `outcome="inconclusive"`。调用方应忽略不认识的新增字段，并为未知 `outcome` 保留兜底分支。

| 字段               | 含义                                                                                                                                                                                                                                                                                      |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `outcome`          | 总体结论。各取值的失败阶段和代理链路含义见下表；调用方应允许未来新增值。                                                                                                                                                                                                                    |
| `targetReachable`  | 至少一次已经通过临时 bridge 连接到目标主机端口。                                                                                                                                                                                                                                          |
| `httpResponded`    | 至少一次收到合法 HTTP 状态码 `100..599`。                                                                                                                                                                                                                                                 |
| `anySucceeded`     | 至少一次收到 HTTP 2xx/3xx。                                                                                                                                                                                                                                                               |
| `allSucceeded`     | 所有计划请求都收到 HTTP 2xx/3xx，并且没有总超时。                                                                                                                                                                                                                                         |
| `timedOut`         | 总诊断期限到期，导致部分计划请求没有执行。单次超时还需查看对应 `attempt.outcome`。                                                                                                                                                                                                        |
| `inconclusive`     | 运行期间网络 epoch 发生变化，当前结果只代表变化前后的混合观测。                                                                                                                                                                                                                           |
| `cleanupTimedOut`  | 临时 bridge 未能在总期限内完全停止；其所有权已进入受控回收队列，目标结论仍应结合 `inconclusive` 判断。                                                                                                                                                                                     |
| `attempted`        | 实际执行的请求次数。                                                                                                                                                                                                                                                                      |
| `succeeded`        | HTTP 2xx/3xx 的请求次数。                                                                                                                                                                                                                                                                 |
| `failed`           | 已执行但没有得到成功 HTTP 响应的次数，包括 HTTP 错误和传输错误。                                                                                                                                                                                                                          |
| `skipped`          | 因总超时或总期限耗尽而未执行的次数。                                                                                                                                                                                                                                                      |
| `elapsedMs`        | 整个诊断的实际耗时。                                                                                                                                                                                                                                                                      |
| `successLatencyMs` | 仅使用成功请求统计延迟；`samples` 是样本数，`min`、`max`、`avg` 分别是最小、最大、平均毫秒数。没有成功样本时这三个值为 `null`。                                                                                                                                                           |

`result.outcome` 的取值按检测推进到的阶段解释：

| `outcome`                | 停止或失败阶段            | 对代理链路和目标的含义                                                                                                      |
| ------------------------ | ------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| `healthy`                | 全流程完成                | 所有计划请求均得到 2xx/3xx；本次代理链路、目标连接和 HTTP 请求均正常。                                                      |
| `degraded`               | 多次请求结果不一致        | 至少一次完整成功，证明链路曾经正常，但存在间歇失败或超时；不能判为稳定正常。                                                |
| `http_error`             | 目标 HTTP 响应            | 代理链路和目标连接已完成，并收到合法 HTTP 状态；只是目标返回 4xx/5xx。应按状态码处理，不要报“代理不可达”。                  |
| `http_transport_failed`  | HTTP 请求或响应传输       | 已进入 HTTP 阶段但没有收到合法状态行。若 `targetReachable=true`，说明代理已连到目标端口，问题位于目标响应或后续传输。        |
| `target_unreachable`     | 代理到目标的 CONNECT      | 代理入口和协议链路已建立到可以返回 CONNECT 错误，但代理出口无法连接目标网络、主机或端口；不等同于代理认证或握手失败。       |
| `tls_failed`             | 目标 TLS 握手或证书校验   | 代理已连接目标端口，但 HTTPS 握手、证书链或主机名校验失败；代理链路通常不是首要故障点。                                     |
| `proxy_auth_failed`      | 某个代理节点认证          | 代理凭据或认证方式错误，尚未到达目标；不能认为代理链路正常。                                                                |
| `proxy_handshake_failed` | 某个代理节点协议握手      | 代理协议、地址、端口或服务不匹配，尚未到达目标；不能认为代理链路正常。                                                      |
| `route_failed`           | 临时 bridge 或代理路由    | 在目标 CONNECT 前链路失败；结合 `diagnosis.layer` 和 `network.route.failure` 定位系统代理、forward、proxy 或 bridgeProxy。 |
| `timeout`                | 路由、TLS、HTTP 或总期限  | 仅说明某阶段超时；读取 `diagnosis.layer/code` 判断是否已到达目标，不能单凭该值认定代理正常或异常。                           |
| `invalid_request`        | 请求参数校验              | URL 或参数无效，网络检测没有形成有效结论；修正参数后再调用。                                                                |
| `inconclusive`           | 诊断期间本机网络发生变化  | VPN、系统代理或出口路由在运行中切换，观测混合了不同网络状态；等待网络稳定后重试。                                           |
| `unknown`                | 证据不足                  | 没有足够证据归类；保留原始结果并结合 `diagnosis`、`attempts` 和日志排查。                                                   |

程序判断“代理链路正常但目标有问题”时，不应只看 `outcome`。`httpResponded=true` 是最强证据，表示链路已到达目标应用并收到 HTTP 响应；`targetReachable=true` 表示至少已连接目标端口。典型组合如下：

| 条件 | 结论 |
| ---- | ---- |
| `httpResponded=true` 且 `anySucceeded=false` | 代理链路已通，目标返回 4xx/5xx；读取 `attempts[].http.status`。 |
| `targetReachable=true` 且 `httpResponded=false` | 代理已连到目标端口，但 TLS、HTTP 响应或后续传输失败；读取 `diagnosis.layer`。 |
| `targetReachable=false` | 尚未证明目标端口可达；继续区分代理认证、代理握手、路由和目标 CONNECT 失败。 |

因此，HTTP 404 不是代理网络失败：通常会得到 `targetReachable=true`、`httpResponded=true`、`anySucceeded=false`，并且 `diagnosis.layer="http"`。只有目标连接本身失败，才应按目标不可达处理。

`data.diagnosis` 说明失败发生在哪一层：

| 字段           | 含义                                                                                                                                                                                                                                                                                                                   |
| -------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `layer`        | 故障层，可为 `none`、`request`、`route`、`system_proxy`、`forward`、`proxy`、`bridge_proxy`、`target_connect`、`tls`、`http`、`timeout`、`multiple` 或 `unknown`。                                                                                                                                                     |
| `code`         | 稳定的机器可读错误码，例如 `OK`、`INTERMITTENT_FAILURE`、`HTTP_STATUS_ERROR`、`PROXY_AUTHENTICATION_FAILED`、`PROXY_HANDSHAKE_FAILED`、`ROUTE_CONNECT_FAILED`、`TLS_HANDSHAKE_FAILED`、`HTTP_RESPONSE_TIMEOUT`、`INVALID_URL`。SOCKS5 目标连接错误还可能是 `TARGET_HOST_UNREACHABLE`、`TARGET_CONNECTION_REFUSED` 等。 |
| `confidence`   | 证据置信度或粒度。普通 attempt 分类通常为 `high` 或 `medium`；由 `route.failure` 细化时可能为 `endpoint` 或 `route`，分别表示端点级和整条链路级证据。调用方应允许未知扩展值，不要写死枚举。                                                                                                                            |
| `message`      | 面向排查的说明文本。                                                                                                                                                                                                                                                                                                   |
| `retryable`    | 是否建议重试；认证错误通常为 `false`，暂时性连接、TLS 或超时错误通常为 `true`。                                                                                                                                                                                                                                        |
| `attemptIndex` | 能定位到单次请求时的序号，从 1 开始；路由整体失败时可能不存在。                                                                                                                                                                                                                                                        |
| `raw`          | 原始错误文本，仅在需要保留底层信息时出现。                                                                                                                                                                                                                                                                             |

常见 `diagnosis.code` 的处理建议：

| code                                              | 含义                                          | 建议                                                                                        |
| ------------------------------------------------- | --------------------------------------------- | ------------------------------------------------------------------------------------------- |
| `OK`                                              | 全部请求成功                                  | 读取延迟和正文即可                                                                          |
| `INTERMITTENT_FAILURE`                            | 部分成功、部分失败                            | 展示成功率与失败 attempts，允许有限重试                                                     |
| `HTTP_STATUS_ERROR`                               | 目标返回 4xx/5xx                              | 按 `attempts[].http.status` 处理；408、425、429 和 5xx 通常可重试，其余通常先修正请求或权限 |
| `HTTP_RESPONSE_TIMEOUT` / `HTTP_TRANSPORT_FAILED` | 已进入 HTTP 阶段但未收到有效响应              | 检查目标响应时间、中间链路和超时设置                                                        |
| `TLS_HANDSHAKE_TIMEOUT` / `TLS_HANDSHAKE_FAILED`  | 目标 CONNECT 后 TLS 失败                      | 检查目标证书、SNI、时间、TLS 拦截和代理协议                                                 |
| `TLS_CERTIFICATE_EXPIRED` / `TLS_CERTIFICATE_NOT_YET_VALID` / `TLS_HOSTNAME_MISMATCH` | 证书有效期或主机名校验失败 | 修正目标证书、系统时间或访问主机名，不要关闭校验 |
| `TLS_CERTIFICATE_UNTRUSTED` / `TLS_CA_STORE_UNAVAILABLE` | 系统信任链不接受证书或 CA 库不可用 | 修复系统根证书或受控测试配置；生产调用保持 `verifyTls=true` |
| `TARGET_*`                                        | SOCKS5 已接管，但目标网络、主机或端口连接失败 | 检查目标 DNS/地址/端口和代理出口策略                                                        |
| `PROXY_AUTHENTICATION_FAILED`                     | 某代理节点认证失败                            | 不自动盲重试；修复凭据或认证方式                                                            |
| `PROXY_HANDSHAKE_FAILED`                          | 某代理节点协议握手失败                        | 检查 scheme、host、port 和节点协议                                                          |
| `ROUTE_CONNECT_FAILED` / `ROUTE_TIMEOUT`          | 只能确认链路级失败或超时                      | 结合 `route.failure`、候选和节点证据定位                                                    |
| `TOTAL_TIMEOUT`                                   | 总期限到期且没有可用结果                      | 增大合理预算、减少次数或检查前置网络阻塞                                                    |
| `INVALID_URL`                                     | 诊断 URL 无效                                 | 修正请求，不重试原参数                                                                      |

`data.network.local` 是调用时读取的本机网络摘要：

| 字段                                        | 含义                                                                                |
| ------------------------------------------- | ----------------------------------------------------------------------------------- |
| `schemaVersion`                             | 本地网络摘要版本，例如 `brosdk-local-network-v2`。                                  |
| `networkEpoch`                              | 本机网络状态版本；网络切换、网卡变化后可能递增，不是网络质量分数。                  |
| `egressProbeOk`                             | 本地基础网络探测结果，不能替代目标 URL 的真实检测。                                 |
| `egressCapability`                          | 本地网络能力摘要，例如 `TopologyOnly`。它描述本机网络探测能力，不等于目标站点可达。 |
| `egressError`                               | 本地网络探测错误；无错误时为空字符串。                                              |
| `connectionType`                            | 网络类型摘要，例如 `Direct`、`WiFi` 或 `Ethernet`。                                 |
| `egressInterface`                           | 当前出口网卡名称。                                                                  |
| `defaultGatewayIpv4` / `defaultGatewayIpv6` | 当前默认网关地址。                                                                  |
| `tunActive` / `vpnActive`                   | 是否检测到活动 TUN/VPN。                                                            |
| `tunIfaces` / `vpnIfaces`                   | 检测到的 TUN/VPN 接口列表。                                                         |

`data.network.local.systemProxy` 描述 Windows 系统代理：`status` 是识别结果，常见值为 `none`、`http`、`https_as_http`、`socks5`、`unsupported_pac`、`unsupported_auto_detect`、`unsupported_socks4` 或 `invalid_*`；`type` 是代理类型；`host`、`port` 是固定代理地址；`pacUrl` 是 PAC 地址；`autoDetect` 表示是否启用自动发现；`bypassCount` 和 `bypassList` 表示绕过列表；`enabled` 是 SDK 综合判断的是否启用；`address` 是用于展示或组装链路的地址；`resolutionKnown` 表示是否解析出确定路由；`nativeFallback` 表示是否只能依赖系统原生代理；`bridgeSupported` 表示该系统代理能否被本地 bridge 组合使用。

`data.network.route` 描述本次临时桥实际准备的链路：

| 字段                  | 含义和值                                                                                                                 |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `source`              | 路由来源；本接口通常为 `temporary_bridge`。                                                                              |
| `selected.name`       | 选中的逻辑链路，例如 `direct`、`proxy`、`forward`、`bridgeProxy`、`systemProxy->proxy`。                                 |
| `selected.chain`      | 将链路拆成节点数组，例如 `["systemProxy", "proxy"]`。                                                                    |
| `selected.dynamic`    | `true` 表示存在 fallback 或 bypass，实际连接可能动态改变；`false` 表示链路固定。                                         |
| `selected.reasonCode` | 选路原因，常见为 `fixed`、`fallback_available`、`proxy_bypass_configured` 或 `route_unavailable`。                       |
| `selectionStatus`     | 常见值为 `primary_selected`、`fallback_selected`、`configured_fallback_unverified`、`configured` 或 `not_probed`。       |
| `verification.status` | `passed` 表示代理端点握手成功，`failed` 表示握手失败，`not_run` 表示直连或没有可验证的代理节点。                         |
| `verification.method` | 当前通常为 `proxy_endpoint_handshake`；未探测时为 `none`。                                                               |
| `failure`             | 选中链路失败时的结构化错误；无错误时为 `null`。                                                                          |
| `candidates`          | 所有候选链路及其探测证据；`summary` 仅删除该字段和 `nodes`。                                                             |
| `nodes`               | 最终链路节点；`status` 常见为 `verified`、`failed`、`configured`、`unverified` 或 `included_not_individually_verified`。 |
| `bridge`              | 临时 bridge 的状态、错误、监听端口和参数注入状态。                                                                       |
| `policy`              | 临时诊断策略，例如 `temporary_route_prepared`。                                                                          |
| `systemProxyUsed`     | 本次实际链路是否包含系统代理。                                                                                           |

`route.verification` 只验证代理节点本身，不验证目标 URL。代理握手通过后，目标仍可能 DNS 失败、TLS 失败或返回 HTTP 错误，最终必须结合 `attempts` 和 `result` 判断。

`route.candidates[]` 的 `role` 是候选链路名；`result` 可为 `selected`、`reachable`、`failed`、`skipped` 或 `selected_unverified`；`attempted` 表示是否探测；`reachable` 表示端点握手是否成功；`tcpReachable` 表示 TCP 是否可达；`handshakeAttempted` 和 `authAttempted` 表示是否进入协议握手和认证；`protocol` 例如为 `socks5` 或 `http`；`error` 为底层错误；`skipReason` 为跳过原因；`socks5Reply=0` 表示 SOCKS5 CONNECT 成功；`socks5Method=0` 表示无认证、`2` 表示用户名密码认证；`authStatus=0` 表示认证成功。`endpoint`、`preceding` 和其中的 `raw` 地址会脱敏。

`data.attempts[]` 保存逐次检测：`index` 从 1 开始；`outcome` 可为 `success`、`http_error`、`target_unreachable`、`tls_failed`、`http_transport_failed`、`timeout`、`invalid_request` 或 `route_failed`；`durationMs` 是单次耗时；`targetConnected` 表示是否连上目标；`http.status` 是目标 HTTP 状态；`http.successful` 表示是否为 2xx/3xx；`headerBytes`、`bodyBytes` 是大小；`http.tlsCertificateVerified` 表示 HTTPS 证书是否完成校验；`http.bodyTruncated` 表示正文是否达到限制；失败时的 `error` 使用与顶层诊断相同的 `layer/code/message/retryable` 结构。`transport` 和 `bridgeRuntime` 只在 `detailLevel=debug` 返回。

`data.lastResponse` 只在 `includeBody=true` 且至少一次请求返回 2xx/3xx 时出现。`status` 是最近一次成功状态码；`body` 是最多 64 KiB 的正文；`bodyBytes` 是正文字节数；`bodyTruncated` 表示是否截断。body 不会在每个 attempt 中重复，也不会由 SDK 解析。

#### 5.19.4 常见结果示例

**目标已到达，但返回 HTTP 错误**：

```json
{
  "code": 0,
  "ok": true,
  "data": {
    "result": {
      "outcome": "http_error",
      "targetReachable": true,
      "httpResponded": true,
      "anySucceeded": false,
      "allSucceeded": false
    },
    "diagnosis": {
      "layer": "http",
      "code": "HTTP_STATUS_ERROR",
      "retryable": false,
      "message": "Target responded with HTTP 403"
    },
    "attempts": [
      {
        "index": 1,
        "outcome": "http_error",
        "targetConnected": true,
        "http": { "status": 403, "successful": false },
        "error": {
          "layer": "http",
          "code": "HTTP_STATUS_ERROR",
          "retryable": false
        }
      }
    ]
  }
}
```

这个结果说明临时 bridge、代理到目标的连接和 HTTP 响应都存在；应检查目标 URL、认证、访问权限或目标站点策略，而不是更换代理。

**代理认证或端点握手失败**：

```json
{
  "code": 0,
  "ok": true,
  "data": {
    "result": {
      "outcome": "proxy_auth_failed",
      "targetReachable": false,
      "httpResponded": false,
      "anySucceeded": false
    },
    "diagnosis": {
      "layer": "proxy",
      "code": "PROXY_AUTHENTICATION_FAILED",
      "confidence": "endpoint",
      "retryable": false
    },
    "network": {
      "route": {
        "failure": {
          "scope": "node",
          "stage": "proxy_authentication",
          "nodeRole": "proxy",
          "nodeIndex": 0
        }
      }
    }
  }
}
```

`targetReachable=false` 不能单独证明目标网站故障。这里应先修复代理账号、密码或认证类型；若 `stage=proxy_handshake`，优先检查节点协议、host、port 和前置链路。

**间歇性失败**：`result.anySucceeded=true` 且 `allSucceeded=false` 时，顶层 `diagnosis.code` 为 `INTERMITTENT_FAILURE`，不应只展示最后一次失败。UI 或 Agent 应同时展示成功次数、失败次数、`successLatencyMs` 和失败 attempt 的错误，并把 `retryable=true` 作为“可以重试”的建议，而不是“重试必然成功”的保证。

**执行失败**：例如环境不存在或 `getEnvInfo` 失败时，顶层 `ok=false`，`data.error` 给出 SDK 层原因；这与“目标返回 404/500”不同，通常不会有 `result`、`attempts` 或 `lastResponse`。调用方应把这类结果归入 SDK/环境配置错误。

#### 5.19.5 推荐调用配置

- 普通连通性：`count=1`、`includeBody=false`、`verifyTls=true`、`detailLevel=summary`，适合 UI 快速检查。
- 稳定性检查：`count=3..5`、`detailLevel=standard`。当 `count*timeoutMs <= 25000` 时，可让 `totalTimeoutMs` 覆盖全部单次超时；超过上限时要接受部分请求因总期限而 `skipped`。
- 需要排查某个节点：使用 `detailLevel=debug`，只在受控诊断界面展示，避免把原始传输错误写入业务日志。
- 需要确认出口内容：`includeBody=true`，调用方读取 `lastResponse.body` 并自行解析；SDK 不猜测出口 IP、JSON 或文本含义。
- 环境未启动时也可以调用。接口只读取服务端环境配置并建立一次性临时 bridge，不会启动浏览器、改变环境代理配置或占用运行中环境的 bridge。

仓库 E2E 脚本会在生产响应外再包装 `checks`、`httpStatus` 等测试字段；这些字段不属于 `/sdk/v1/env/netdiag` 的生产契约。目标网站状态只读取 `data.attempts[].http.status` 或 `data.lastResponse.status`。

环境不存在属于接口执行错误并返回后端对应错误码。已启动、已停止、尚未在本地打开以及生命周期过渡中的环境都使用临时 bridge 诊断；临时 bridge 启动失败时返回执行错误，并在错误数据中保留 `environment` 状态。

### 5.20 `POST /sdk/v1/env/destroy`

同步销毁环境。

当前行为：

- 请求体直接转发到后端 `env/destroy`

- 响应体直接返回后端原始 JSON

- **注意：销毁环境不等于关闭浏览器**。如果该环境的浏览器仍在运行，请先调用 `browser/close` 并等待 `browser-close-success`，再调用 `env/destroy`

参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/destroy` 接口契约。

### 5.21 `POST /sdk/v1/shutdown`

同步停止 SDK。

当前行为：

- 停止 SDK

- 关闭内嵌 Web API 服务

- 销毁当前单例

## 6. 动态库接口参考

头文件：

```c
#include "brosdk.h"


```

### 6.1 核心类型

```c
typedef void *sdk_handle_t;


```

不透明的 SDK 实例句柄。

```c
typedef void(SDK_CALL *sdk_result_cb_t)(
    int32_t code,
    void *user_data,
    const char *data,
    size_t len);


```

动态库接口的统一异步结果回调。业务字段请以 JSON body 为准。

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

SDK 日志回调。`SDK_LOG_TYPE_LOCAL` 表示本地 SDK 日志行；`SDK_LOG_TYPE_SERVER` 表示已进入 sdk-server 上传队列的结构化日志 JSON。回调中的 `data` 只在当前回调有效。

```c
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(
    const char *data,
    size_t len,
    char **new_data,
    size_t *new_len,
    void *user_data);


```

Cookie 持久化前的拦截回调。

### 6.2 生命周期与信息接口

| 函数                              | 模式         | 说明                                                  |
| --------------------------------- | ------------ | ----------------------------------------------------- |
| `sdk_register_result_cb`          | 同步         | 注册全局异步回调                                      |
| `sdk_register_log_cb`             | 同步         | 注册 SDK 日志回调，传 `NULL` 可关闭                   |
| `sdk_register_cookies_storage_cb` | 同步         | 注册 Cookie 拦截回调                                  |
| `sdk_init_cpp`                    | 同步         | 仅获取 SDK 句柄，不执行初始化                         |
| `sdk_init`                        | 同步         | 初始化 SDK，返回堆分配的 JSON 响应                    |
| `sdk_init_async`                  | 异步         | 受理后返回 `CL_DONE` 或实际 `reqId`                   |
| `sdk_init_webapi`                 | 同步辅助函数 | 兼容入口，新接入建议直接在 `sdk_init` 中携带 `"port"` |
| `sdk_info`                        | 同步         | 返回 SDK info JSON                                    |
| `sdk_network_diagnostics`         | 同步         | 返回网络 / 代理诊断 JSON                              |
| `sdk_system_proxy_diagnostics`    | 同步         | 返回系统代理跳、可组合性与网络 epoch                  |
| `sdk_browser_info`                | 同步         | 返回当前运行中的浏览器列表 JSON                       |
| `sdk_token_update`                | 异步         | 刷新用户令牌                                          |
| `sdk_shutdown`                    | 同步         | 停止 SDK 并销毁单例                                   |

`sdk_network_diagnostics` 的请求体与 `/sdk/v1/netdiag` 一致，返回的 `out_data` 必须通过 `sdk_free()` 释放。`sdk_system_proxy_diagnostics` 直接返回系统代理诊断对象，内容与 Web API `/sdk/v1/proxydiag` 的 `data.systemProxy` 一致，但不包含 Web API 的 `code/reqId/type/msg/data` envelope；返回内存也必须通过 `sdk_free()` 释放。该接口只发现可复用的系统代理跳，不会替用户选择或安装代理链。

### 6.3 浏览器接口

| 函数                  | 模式 | 说明                                                         |
| --------------------- | ---- | ------------------------------------------------------------ |
| `sdk_browser_install` | 异步 | 安装浏览器核心资源                                           |
| `sdk_browser_open`    | 异步 | 最终 ready 信号是 `browser-open-success`                     |
| `sdk_browser_close`   | 异步 | 最终关闭完成信号是 `browser-close-success`                   |
| `sdk_browser_cleanup` | 同步 | 清理环境 `userdatadir` 缓存和内核下载缓存，返回清理结果 JSON |

`sdk_browser_install` 的请求体与 `/sdk/v1/browser/install` 一致，必须包含 `cores[ ].major`。进度和最终结果通过 `sdk_result_cb_t` 回调返回。

`sdk_browser_open` 的请求体与 `/sdk/v1/browser/open` 一致。`envs[ ]` 对象可携带 `forward` 和 `bridge`；`forward` 是本次启动显式代理，`bridge` 只覆盖本次启动的备用跳板，不修改环境绑定的 `bridgeProxy`。

`sdk_browser_cleanup` 的请求体与 `/sdk/v1/browser/cleanup` 一致，返回的 `out_data` 必须通过 `sdk_free()` 释放。`envs` 用于清理环境 `userdatadir`，`cores` 用于清理内核下载缓存 `cores/.cache`。运行中或正在打开/关闭队列中的环境会返回 `CL_EBUSY`，结果项状态为 `busy`。

### 6.4 环境接口

| 函数                       | 模式 | 说明                                               |
| -------------------------- | ---- | -------------------------------------------------- |
| `sdk_env_create`           | 同步 | 返回后端原始 JSON                                  |
| `sdk_env_update`           | 同步 | 返回后端原始 JSON                                  |
| `sdk_env_page`             | 同步 | 返回后端原始 JSON                                  |
| `sdk_env_getinfo`          | 同步 | 返回后端 `getEnvInfo` 原始 JSON                    |
| `sdk_env_netdiag`          | 同步 | 通过临时环境代理桥检测网络稳定性和响应正文         |
| `sdk_env_destroy`          | 同步 | 返回后端原始 JSON                                  |
| `sdk_get_cookies_history`  | 同步 | 按 `envId` 返回后端 `getCookieHistory` 原始 JSON   |
| `sdk_cookies_health_check` | 同步 | 离线分析 Cookie 数组并返回按 domain 分组的健康报告 |

`sdk_env_getinfo` 的请求体与 `/sdk/v1/env/getinfo` 一致，返回的 `out_data` 必须通过 `sdk_free()` 释放。`sdk_env_netdiag` 的请求体、原始 `code/msg/ok/data` 响应和 `/sdk/v1/env/netdiag` 一致，返回内存也必须通过 `sdk_free()` 释放；请求可以携带 `forward`，只覆盖本次临时诊断链路。C++ `ISDK` 虚接口提供 `EnvNetDiag(...)`，语义与对应动态库接口一致。

`sdk_get_cookies_history` 请求体为 `{"envId":"..."}`，返回的 `out_data` 必须通过 `sdk_free()` 释放。C++ `ISDK::GetCookieHistory(...)` 语义与其一致。

`sdk_cookies_health_check` 请求体为 `{"cookies":[]}`。该接口不需要初始化 SDK，不进行网络或浏览器 I/O；结果包含总览、domain 健康项、逐 Cookie 有效期和认证 Token 的时间窗提示，但不会返回 Cookie value。`expiresSoonThresholdSeconds` 只是临期告警窗口；`domains[].cookies[].remainingSeconds` 才是 Cookie 自身距离过期的秒数，负数表示已经过期，session Cookie 或缺少有效期时为 `null`。domain 同时提供 earliest/latest/next expiration 及对应的剩余秒数。JWT 的 `time_valid` 仅表示 `exp/nbf` 时间窗允许使用，不代表签名有效或服务端登录仍然有效。返回的 `out_data` 必须通过 `sdk_free()` 释放，C++ 接口为 `ISDK::CheckCookiesHealth(...)`。

### 6.5 Cookie 回调的当前行为

`sdk_register_cookies_storage_cb()` 有三个接入细节需注意：

- 回调接收的是明文 Cookie JSON 数组，而非最终落盘 / 上传的加密二进制

- 若返回替换后的 JSON，SDK 将以该 JSON 继续执行后续加密与持久化

- Cookie 快照为空时，SDK 也会规范化为 `[ ]` 后触发回调，不静默跳过

### 6.6 内存与辅助接口

```c
SDK_API void *SDK_CALL sdk_malloc(size_t size);
SDK_API void  SDK_CALL sdk_free(void *ptr);


```

所有 SDK 返回的动态内存都应通过 `sdk_free()` 释放。

```c
SDK_API const char *SDK_CALL sdk_error_name(int32_t code);
SDK_API const char *SDK_CALL sdk_error_string(int32_t code);
SDK_API const char *SDK_CALL sdk_event_name(int32_t evtid);


```

静态字符串辅助函数，返回值不能释放。

```c
SDK_API bool SDK_CALL sdk_is_error(int32_t code);
SDK_API bool SDK_CALL sdk_is_warn(int32_t code);
SDK_API bool SDK_CALL sdk_is_reqid(int32_t code);
SDK_API bool SDK_CALL sdk_is_ok(int32_t code);
SDK_API bool SDK_CALL sdk_is_done(int32_t code);
SDK_API bool SDK_CALL sdk_is_event(int32_t code);


```

## 7. Cookie 与 Storage 持久化语义

### 7.1 托管模式

可通过 `sdk_info().dataFullyManaged` 判断当前模式：

| 值      | 模式   | 说明                                  |
| ------- | ------ | ------------------------------------- |
| `false` | 半托管 | 只落本地 SQLite，不做 OSS 同步        |
| `true`  | 全托管 | SQLite 作为本地缓存，同时后台同步 OSS |

### 7.2 浏览器关闭时的持久化链路

当前实现中，浏览器关闭后的 Cookie 持久化链路如下：

1.  从浏览器快照提取 Cookie JSON 数组

2.  调用 `sdk_cookies_storage_cb_t`，允许宿主查看或替换明文 JSON

3.  把最终 JSON 转成 Cookie protobuf

4.  使用 `BroEncryptCookiesWithDEK(appId, coKeyVer, coKey, dek)` 加密 Cookie

5.  将加密 envelope 再次打包为 protobuf，并做 `br` 压缩

6.  立即写入本地 SQLite

7.  若为全托管，再异步上传到 OSS

Storage 的链路不同：

- Storage 不走 Cookie 这套 DEK 加密

- 当前实现是按存储策略收集浏览器数据文件，打包后做 `br` 压缩

- 本地 SQLite 与 OSS 持有的是同一份压缩归档

### 7.3 Cookie 在本地与 OSS 中的实际形态

当前实现里，Cookie 在本地 SQLite 和 OSS 中都不是明文 JSON：

| 存储位置            | 实际内容                                                                  |
| ------------------- | ------------------------------------------------------------------------- |
| SQLite `cookies` 列 | `BroEncryptCookiesWithDEK` 生成的加密 Cookie 包，再经 `br` 压缩后的二进制 |
| OSS Cookie 对象     | 与本地 SQLite 相同的加密 Cookie 二进制                                    |

加密所依赖的关键材料来源：

| 字段                 | 来源                            |
| -------------------- | ------------------------------- |
| `appId`              | SDK 当前应用上下文              |
| `coKey` / `coKeyVer` | SDK 初始化后的凭证信息          |
| `dek`                | 当前环境 `envInfo` 中返回的 DEK |

因此，客户如需接管 Cookie 明文，只能在 `sdk_cookies_storage_cb_t` 回调阶段处理；一旦进入持久化阶段，SDK 保存和上传的都是加密后的二进制。

### 7.4 当前 OSS 对象路径口径

当前代码实现不会要求客户自己拼接 Cookie / Storage 对象名：

- 后端返回 `cookieUpPath` / `storageUpPath` 前缀

- SDK 会自动去掉前导 `/`

- Cookie 自动追加 `{envId}-v2.br`，使用当前 v2 AES-GCM JSON 信封
- Storage 继续自动追加 `{envId}-v1.br`

因此：

- 请勿在接入文档中硬编码旧版的 `cookies.pb` / `storage.zst` 路径模板

- 业务如需记录对象路径，请以后端返回的元数据与 SDK 实际回填结果为准

### 7.5 全托管模式下的本地缓存仲裁

现在本地 SQLite 还会维护以下元数据：

- `sync_state`

- `cookie_md5`

- `storage_md5`

- `cookie_file_url`

- `storage_file_url`

- `last_sync_ms`

- `updated_ms`

浏览器打开时，全托管模式下会按以下规则判断是否直接使用本地 SQLite：

- `sync_state = Dirty`：说明本地刚关闭过、OSS 可能还未同步，优先使用本地

- `sync_state = UploadFailed`：说明上次上传失败，优先使用本地

- 后端远端元数据缺失：优先使用本地

- 本地 `md5` / `fileUrl` 与远端一致：优先使用本地

- 其余情况：认为远端更新更可信，回退到 OSS 下载

> 全托管模式下，SDK 不盲目信任 SQLite 缓存，而是先做本地 / 远端元数据比对。

**OSS 上传失败重试机制：**

当全托管模式下 OSS 上传失败时，SDK 不会丢弃数据：

- 本地 SQLite 中会保留 `sync_state = UploadFailed` 状态

- 下次该环境的浏览器启动时，SDK 会检测到 `UploadFailed` 并在生命周期内自动重试上传

- 重试时机：下次浏览器关闭后的持久化流程中

- 因此，即使 OSS 暂时不可用，数据也不会丢失，仅会延迟同步

### 7.6 `browser-close-success` 与上传完成不是同一件事

当前实现中：

- `browser-close-success` 表示本地快照已经完成、SQLite 已可用于下次启动

- 它不表示 OSS 上传已经完成

- 如果全托管上传失败，本地 SQLite 会保留 `UploadFailed` 状态，后续生命周期仍可重试

## 8. 集成时必须注意的规则

- 先调用 `sdk_register_result_cb()`，再进入异步业务流程

- 把 `sdk_init` 当成进程内全局串行入口

- 对于异步动态库接口，返回 `CL_DONE` 或 `reqId` 都表示“请求已受理”

- Web API 中 `init`、`info`、`browser/info`、`netdiag`、`proxydiag`、`env/netdiag`、`browser/cleanup` 与环境 CRUD/查询接口为同步接口；`browser/install`、`browser/open`、`browser/close`、`token/update` 为异步 ACK

- Web API 的异步接口必须同时建立 WebSocket，用于接收最终结果

- 浏览器可用的真正信号是 `browser-open-success`

- `browser-open-success` 是浏览器可用信号；代理桥降级这类“成功但有告警”的情况，`code` 可以是 `CL_WPROXYDEGRADED`，请同时查看后端上报日志的 `level=Warn` 和 `extra.lifecycle`

- 浏览器真正关闭完成的信号是 `browser-close-success`

- `browser-close-success` 只保证本地持久化完成，不保证 OSS 上传完成

- 如果浏览器是用户手动关闭或进程自行退出，仍可能上报 `browser-close-success`，但 `data.closeReason*` 会说明真实原因

- 环境 CRUD、分页、getinfo 和 getcookiehistory 接口与 `init/info/browser/*` 的回包包装方式不同，前者是后端原始 JSON 透传；`netdiag`、`env/netdiag` 是 BroSDK 生成的同步原始诊断 JSON；`proxydiag` 成功时使用标准 envelope、诊断自身失败时返回原始错误 JSON；`browser/cleanup` 返回同步清理结果 JSON

- `env/create`、`env/update`、`env/page`、`env/destroy` 的请求参数与响应字段请以后端对接文档为准，SDK 文档不重复维护

- 如需代理行为稳定可预期，请在创建 / 更新环境时绑定 `proxy`，并在 `browser/open` 中按需传入 `forward` 或 `bridge`；`forward` 可作为本次启动显式代理，`bridgeProxy` 是绑定到环境上的备用前置跳板；勿依赖客户机器隐式系统代理环境

- `netdiag` 不改变浏览器设置。新接入优先使用 `mode="recommend"` 并传 `forward`；只有兼容模式才把启动链路手工映射到 `proxy`/`bridgeProxy`。`env/netdiag` 会读取指定环境的服务端配置，并用一次性临时 bridge 验证，不启动浏览器

- `proxydiag` 只读取本机系统代理快照，`bridgeSupported=true` 也不等于目标 URL 可达；需要目标级结论时继续调用 `netdiag` 或 `env/netdiag`

- `urls`、`args`、`whiteList`、`blackList`、`extensions`、`cookies` 这类数组字段建议由接入层做长度、条目格式和敏感值校验

## 9. 事件与错误码附录

常用事件码：

| 事件 ID | 名称                                    |
| ------- | --------------------------------------- |
| `10110` | `sdk-init`                              |
| `10111` | `sdk-init-success`                      |
| `10112` | `sdk-init-failed`                       |
| `10120` | `sdk-token-update`                      |
| `10121` | `sdk-token-update-success`              |
| `10122` | `sdk-token-update-failed`               |
| `10123` | `sdk-token-expire-warning`              |
| `10124` | `sdk-token-expired`                     |
| `10130` | `sdk-info`                              |
| `10131` | `sdk-info-success`                      |
| `10132` | `sdk-info-failed`                       |
| `10140` | `sdk-shutdown`                          |
| `10141` | `sdk-shutdown-success`                  |
| `10142` | `sdk-shutdown-failed`                   |
| `10150` | `sdk-netdiag`                           |
| `10151` | `sdk-netdiag-success`                   |
| `10152` | `sdk-netdiag-failed`                    |
| `20110` | `browser-open`                          |
| `20111` | `browser-open-success`                  |
| `20112` | `browser-open-failed`                   |
| `20113` | `browser-open-timeout`                  |
| `20115` | `browser-info`                          |
| `20116` | `browser-info-success`                  |
| `20117` | `browser-info-failed`                   |
| `20140` | `browser-close`                         |
| `20141` | `browser-close-success`                 |
| `20142` | `browser-close-failed`                  |
| `20143` | `browser-close-timeout`                 |
| `20150` | `browser-cleanup`                       |
| `20151` | `browser-cleanup-success`               |
| `20152` | `browser-cleanup-failed`                |
| `20210` | `browser-env-create`                    |
| `20211` | `browser-env-create-success`            |
| `20212` | `browser-env-create-failed`             |
| `20220` | `browser-env-update`                    |
| `20221` | `browser-env-update-success`            |
| `20222` | `browser-env-update-failed`             |
| `20230` | `browser-env-page`                      |
| `20231` | `browser-env-page-success`              |
| `20232` | `browser-env-page-failed`               |
| `20240` | `browser-env-destroy`                   |
| `20241` | `browser-env-destroy-success`           |
| `20242` | `browser-env-destroy-failed`            |
| `20250` | `browser-env-info`                      |
| `20251` | `browser-env-info-success`              |
| `20252` | `browser-env-info-failed`               |
| `20350` | `browser-install`                       |
| `20351` | `browser-install-progress`              |
| `20352` | `browser-install-success`               |
| `20353` | `browser-install-failed`                |
| `20600` | `browser-proxy`                         |
| `20601` | `browser-proxy-success`                 |
| `20602` | `browser-proxy-failed`                  |
| `20603` | `browser-proxy-dns-resolve-failed`      |
| `20604` | `browser-proxy-tcp-connect-failed`      |
| `20605` | `browser-proxy-http-connect-rejected`   |
| `20606` | `browser-proxy-socks5-auth-failed`      |
| `20607` | `browser-proxy-socks5-connect-rejected` |
| `20608` | `browser-proxy-write-failed`            |
| `20609` | `browser-proxy-degraded`                |

常用错误 / 警告码：

| 代码           | 名称                    | 说明                                                            |
| -------------- | ----------------------- | --------------------------------------------------------------- |
| `0`            | `CL_OK`                 | 成功                                                            |
| `1`            | `CL_DONE`               | 异步任务已受理                                                  |
| `101`          | `CL_WDIRNOTEXIST`       | 目录不存在；常见于清理本地缓存时目标目录已不存在                |
| `102`          | `CL_WBRWPROCEXITED`     | 浏览器进程自行退出；常见于手动关窗或运行中异常退出              |
| `103`          | `CL_WPROXYDEGRADED`     | 代理桥降级；浏览器已启动，但代理链路未完全按预期工作            |
| `104`          | `CL_WBUSY`              | 资源忙，另一个初始化操作正在进行                                |
| `107`          | `CL_WBRWALREADYRUNNING` | 环境已运行；SDK 已复用并尝试激活现有浏览器窗口                  |
| `-3001`        | `CL_EBUSY`              | 资源忙；例如清理环境缓存时目标环境仍在运行或仍在打开/关闭队列中 |
| `-3002`        | `CL_ETIMEOUT`           | 超时                                                            |
| `-3003`        | `CL_EINVALID`           | 参数错误                                                        |
| `-3005`        | `CL_EALREADY`           | 已存在或重复初始化；进程内 SDK 单例已运行，无需再次初始化       |
| `-3012`        | `CL_ENOTINITIALIZED`    | SDK 未初始化                                                    |
| `-3019`        | `CL_EPORT_UNAVAILABLE`  | 端口非法或已占用                                                |
| `-3023`        | `CL_EOSS_NOCLIENT`      | OSS 客户端未初始化                                              |
| `-3024`        | `CL_EOSS_DOWNLOAD`      | OSS 下载失败                                                    |
| `-3025`        | `CL_EOSS_UPLOAD`        | OSS 上传失败                                                    |
| `-3027`        | `CL_EOSS_NOTFOUND`      | OSS 对象不存在                                                  |
| `-3028`        | `CL_ECOOKIE_RESTORE`    | Cookie 恢复失败                                                 |
| `-3029`        | `CL_ESTORAGE_RESTORE`   | Storage 恢复失败                                                |
| `-3030`        | `CL_ENOCORERESOURCE`    | 没有可用浏览器核心资源                                          |
| `-3502`        | `CL_EHTTP_POST`         | 后端 HTTP 失败                                                  |
| `-3509`        | `CL_ETOKEN_INVALID`     | Token 无效                                                      |
| `-3511`        | `CL_EWORKDIR_INVALID`   | 工作目录无效                                                    |
| `-3512`        | `CL_ENETWORK`           | 网络或代理诊断失败                                              |
| `-3513`        | `CL_EBROWSER`           | 浏览器错误                                                      |
| `-3514`        | `CL_EBRWPROCEXITED`     | 浏览器进程异常退出                                              |
| `-4000` 及以下 | `CL_ESDKAPI` 系列       | 后端 SDK API 错误                                               |

---

本文明确区分了 BroSDK 直接生成的响应结构与直接透传后端的 env 接口响应结构。 如接入侧同时依赖完整的后端环境模型，请以后端 env API 契约作为完整字段集合的最终依据。
