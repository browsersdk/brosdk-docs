# brosdk接入文档

> **当前版本：v1.0.1.1　　最后更新：2026-06-12** 本文档是 BroSDK 的统一接入参考文档。 内容覆盖动态库接口与 Web API，以 `brosdk.h` 公开接口为准。 如有字段或行为的疑问，请以头文件 `brosdk.h` 为最终依据。

## 1. 概述

BroSDK 是一个使用 C/C++ 实现的浏览器环境管理 SDK，对外提供两种接入方式：

*   动态库接口：直接加载 `brosdk.dll` / `brosdk.so` / `brosdk.dylib`，通过 `brosdk.h` 公开接口调用

*   Web API：初始化时开启本地 HTTP/WebSocket 服务，HTTP 发起请求，WebSocket 接收异步事件


两种方式共享同一套浏览器生命周期、环境管理和数据持久化实现，但异步结果的交付方式不同：

*   动态库接口：函数返回值 + `sdk_result_cb_t` 回调

*   Web API：同步接口直接返回；异步接口先返回 ACK，最终结果通过 WebSocket 推送


## 2. 平台支持

| 平台 | 架构 | 动态库 | 状态 | 备注 |
| --- | --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | 正式支持 | 主力开发与测试平台 |
| Linux | x64 | `brosdk.so` | 正式支持 | 支持主流发行版 |
| macOS | arm64 / x64 | `brosdk.dylib` | 正式支持 | arm64 为主要测试架构 |

## 3. 通用约定

### 3.1 JSON 与编码

*   所有请求体均为 UTF-8 JSON

*   所有返回体与通知体均为 UTF-8 JSON

*   动态库接口中的 `len` 均为字节长度，不包含末尾 `\0`


### 3.2 内存管理

*   动态库接口返回的 `out_data`，使用后必须调用 `sdk_free()`

*   回调中的 `data` 指针只在当前回调有效，如需长期保存请立即复制

*   `sdk_cookies_storage_cb_t` 若要替换 Cookie 数据，`*new_data` 必须通过 `sdk_malloc()` 分配


### 3.3 返回值、`reqId` 与辅助判断

需区分“同步返回码”与“异步请求受理 ID”两类返回值：

| 数值范围 | 含义 |
| --- | --- |
| `0` | `CL_OK`，同步成功 |
| `1` | `CL_DONE`，异步任务已受理，返回值不含实际 `reqId` |
| `> 100000` | 异步请求已受理，并直接返回实际 `reqId` |
| `100 ~ 255` | Warning，例如 `CL_WBUSY` |
| `< 0` | Error |

辅助函数：

*   `sdk_is_ok(code)`

*   `sdk_is_done(code)`

*   `sdk_is_reqid(code)`

*   `sdk_is_warn(code)`

*   `sdk_is_error(code)`

*   `sdk_error_name(code)`

*   `sdk_error_string(code)`

*   `sdk_event_name(evtid)`


重要说明：

*   当前异步动态库接口可能返回 `CL_DONE`，也可能直接返回 `reqId`

*   对于 Web API 异步 ACK，顶层 `reqId` 也可能是 `0`，也可能是实际受理到的请求 ID

*   业务接入时，应把“`sdk_is_done(code)` 或 `sdk_is_reqid(code)` 为真”都视为“异步任务已进入调度”


### 3.4 异步回调语义

`sdk_result_cb_t` 是动态库接入时统一的异步结果通道。

*   回调第一个参数 `code` 是本次通知的粗粒度状态

*   `reqId`、`type`、`eventId` 及业务字段以 JSON body 为准

*   回调第一个参数不应用作稳定的 `reqId` 或 `eventId`


推荐做法：

1.  解析 JSON body

2.  优先根据顶层 `type` 路由

3.  如需关联请求，使用顶层 `reqId`

4.  `data.eventId` 作为事件枚举补充字段使用


**线程安全注意事项：**

*   `sdk_cookies_storage_cb_t` 在 SDK 内部已串行化，不会并发调用

*   `sdk_result_cb_t` 不保证串行，同一回调可能在不同事件驱动时并发调用

*   建议宿主侧在回调中加锁，或只做最小操作（如入队）后在业务线程消费

*   无论哪种回调，**禁止在回调中调用 SDK 阻塞/同步接口**（如 `sdk_init`、`sdk_shutdown`），否则可能导致死锁


### 3.5 同步响应 Envelope

BroSDK 直接生成的同步响应结构如下：

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
| `msg` | string | 可读状态描述 |
| `data` | object | 接口专属数据 |

例外：

*   `env/create`、`env/update`、`env/page`、`env/getinfo`、`env/destroy` 为后端原始 JSON 透传

*   `netdiag` 返回 BroSDK 网络诊断原始 JSON，结构为 `code/msg/ok/data`，不额外包含 `reqId/type`

*   `browser/cleanup` 返回 BroSDK 清理结果原始 JSON，结构为 `code/msg/data`，不额外包含 `reqId/type`

*   上述接口不会额外套一层 BroSDK 自己的 envelope


### 3.6 异步 Web API ACK Envelope

异步 Web API 接口的 HTTP 响应返回的是“已受理 ACK”，不是最终结果：

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

*   `reqId` 可能为 `0`，也可能为实际异步请求 ID

*   ACK 只表示请求已经进入 SDK 调度器

*   最终成功或失败，必须通过 WebSocket 获取


### 3.7 浏览器生命周期通知结构

浏览器打开 / 关闭相关通知使用统一结构：

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

`data` 常见字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `envId` | string | 环境 ID |
| `status` | int | 浏览器生命周期状态 |
| `statusName` | string | 生命周期状态名称 |
| `progress` | int | 当前进度百分比 |
| `remoteDebuggingPort` | int | CDP 端口，若当前实例已开启 |

当前 `statusName` 可能出现：

*   `Idle`

*   `Downloading`

*   `Preparing`

*   `Starting`

*   `Started`

*   `Stopping`

*   `Stopped`

*   `Destroyed`

*   `StartFailed`

*   `StopFailed`


成功事件与日志告警的关系：

*   `browser-open-success` 是业务成功事件，表示浏览器进程已运行；正常成功时 `code=CL_OK`

*   如果打开链路中代理桥启动失败或运行时代理探测失败，但浏览器最终仍然启动成功，SDK 仍发送 `browser-open-success`

*   上述“启动成功但未 100% 按预期启动”的情况，事件 `type` 仍是 `browser-open-success`，但 `code` 可以是 `CL_WPROXYDEGRADED`；后端上报日志也会以 `level=Warn` 以及 `extra.lifecycle.steps[ ]` 中的 `proxy` 步骤体现


## 4. 代理字段与当前策略

### 4.1 字段口径

代理桥决策会综合环境绑定代理与本次打开浏览器参数。`proxy` 与 `bridgeProxy` 可在创建或更新环境时绑定；`browser/open` 支持通过 `forward` 或 `bridge` 覆盖本次启动的代理或跳板配置。

| 字段 | 来源 | 说明 |
| --- | --- | --- |
| `proxy` | 环境创建 / 更新阶段绑定 | 最终上游代理；不作为 `browser/open` 参数传入 |
| `forward` | `browser/open` 本次启动参数 | 本次启动显式代理，优先级最高；有环境 `proxy` 时作为前置跳板，无环境 `proxy` 时作为本次最终代理 |
| `bridge` | `browser/open` 本次启动参数 | 本次启动使用的备用前置跳板；传入后会覆盖环境绑定的 `bridgeProxy` |
| `bridgeProxy` | 环境创建 / 更新阶段绑定 | 环境默认备用前置跳板 |

推荐统一使用完整代理 URL：

*   `socks5://host:port`

*   `socks5://user:pass@host:port`

*   `socks5h://user:pass@host:port`

*   `http://user:pass@host:port`


### 4.2 当前默认策略的决策规则

当前版本在每次浏览器启动前，会依次评估以下规则（按优先级从高到低）：

| 优先级 | 条件 | 实际行为 |
| --- | --- | --- |
| 1 | `browser/open` 传入 `forward`，且环境绑定了 `proxy` | `forward` 优先级最高，不受 `global` 影响；链路为 `本地 bridge -> forward -> proxy -> 目标网站` |
| 2 | `browser/open` 传入 `forward`，且环境没有绑定 `proxy` | `forward` 作为本次启动最终代理；链路为 `本地 bridge -> forward -> 目标网站` |
| 3 | `forward` 为空，且 `browser/open` 传入 `bridge` | 本次启动先用 `bridge` 覆盖环境绑定的 `bridgeProxy`；不写回环境配置 |
| 4 | `forward` 为空，且环境没有绑定 `proxy` | 不启动 SDK 管理的业务代理链；`bridge` / `bridgeProxy` 不会单独生效 |
| 5 | `forward` 为空，环境绑定了 `proxy`，但没有可用 `bridgeProxy` | `本地 bridge -> proxy -> 目标网站` |
| 6 | `forward` 为空，环境绑定了 `proxy`，且有可用 `bridgeProxy`，同时 `global=true` | 宿主机网络已具备出海能力，不使用前置跳板；链路为 `本地 bridge -> proxy -> 目标网站` |
| 7 | `forward` 为空，环境绑定了 `proxy`，且有可用 `bridgeProxy`，同时 `global=false` | `本地 bridge -> bridgeProxy -> proxy -> 目标网站` |

> `global` 是 SDK 内部对宿主机出海能力的判断值，不是客户配置字段。当前策略中，`global` 只影响 `bridgeProxy -> proxy` 这一路径；显式传入的 `forward` 优先级最高，不会因为 `global=true` 被清空。

进入代理桥时，SDK 会为每个浏览器实例单独启动一个本地 loopback bridge，并向 Chromium 注入：

```text
--proxy-server=socks5://127.0.0.1:{port}


```

关键约束：

*   `forward`、`bridge` 和 `bridgeProxy` 不会同时生效，**当前不支持双跳前置链**（即不存在 `forward -> bridgeProxy -> proxy` 这种路径）

*   `browser/open` 支持传入 `forward` 和 `bridge`；环境固定上游代理仍请在创建或更新环境时绑定 `proxy`，只影响本次启动的显式代理可使用 `forward`

*   `bridge` 和环境绑定的 `bridgeProxy` 只是备用前置跳板；如果没有 `proxy`，且本次启动也没有传入 `forward`，它们不会单独启动代理链


### 4.3 当前默认策略的重要说明

当前实现的关键行为：

1.  `forward` 和 `bridge` 是 `browser/open` 可传字段，参与本次启动的代理桥决策

2.  `forward` 是本次启动的显式代理，优先级高于 `bridge` 和环境绑定的 `bridgeProxy`，且不受 `global` 影响；有环境 `proxy` 时作为前置跳板，无环境 `proxy` 时作为最终代理

3.  `bridge` 只影响本次启动；如果传入非空 `bridge`，SDK 会在本次启动中把它作为 `bridgeProxy` 使用

4.  如果没有 `proxy`，且本次启动没有传入 `forward`，SDK 不注入本地代理桥，浏览器回退至 Chromium 默认网络栈

5.  如果传入 `forward` 但没有绑定 `proxy`，链路为 `本地 bridge -> forward -> 目标网站`


注意事项：

*   没有代理桥目标时，浏览器回退至 Chromium 默认网络栈，是否走系统代理取决于 Chromium / 操作系统默认行为

*   如需行为稳定可预期，请显式绑定完整 `proxy` URL，或在 `browser/open` 中显式传入完整 `forward` URL；勿依赖客户机器隐式系统代理配置

*   非空但格式无法识别的代理 URL 不会让浏览器回退本地直连；SDK 会把它归一化为不可达代理 `socks5://127.0.0.1:9`，从而保持“必须走代理链”的约束

*   `forward`、`bridge` 与 `bridgeProxy` 互斥，不支持双跳前置链


### 4.4 故障与回退

当前实现中，如果代理桥启动失败：

*   SDK 会记录代理桥诊断日志，并在后端上报日志中把打开链路标记为 `Warn`

*   浏览器仍然会继续启动

*   但此时不会再使用 SDK 管理的本地代理桥

*   如果浏览器最终启动成功，对外最终事件仍是 `browser-open-success`；代理降级时事件 `code=CL_WPROXYDEGRADED`，代理问题通过日志 `extra.lifecycle.steps[ ]` 中的 `proxy` 步骤说明


## 5. Web API 参考

### 5.1 启用方式

常见做法是在动态库 `sdk_init` 的初始化 JSON 中携带 `port`，一次完成 SDK 初始化并启用内嵌 Web API：

```json
{
  "userSig": "your-user-sign",
  "port": 9527
}


```

兼容入口 `sdk_init_webapi(port)` 只用于先启动本地 Web API 服务；业务初始化仍需随后调用 `POST /sdk/v1/init` 并传入 `userSig`。

初始化完成后：

*   Web API HTTP 地址：`http://127.0.0.1:{port}`

*   WebSocket 地址：`ws://127.0.0.1:{port}/`

*   HTTP 请求与 WebSocket 使用同一个本地 TCP 端口


`port` 的当前语义：

| 值 | 行为 |
| --- | --- |
| 不传 | 不启用内嵌 Web API |
| `0` | SDK 自动分配本机空闲端口，实际端口在初始化响应的 `data.port` 中返回 |
| `> 0` | SDK 尝试监听 `127.0.0.1:{port}`，端口不可用时返回 `CL_EPORT_UNAVAILABLE` |

### 5.2 认证模型

内嵌 Web API 当前不依赖 `Authorization` Header。

当前模型如下：

*   `/sdk/v1/init` 在 JSON body 中携带 `userSig`

*   后续 Web API 请求复用当前已初始化的 SDK 实例

*   `/sdk/v1/token/update` 在 JSON body 中刷新 `userSig`


### 5.3 WebSocket 使用说明

WebSocket 是 Web API 的异步事件通道。调用异步接口（`browser/install`、`browser/open`、`browser/close`、`token/update`）时，HTTP 响应只返回受理 ACK；最终进度与结果通过 WebSocket 推送。

#### 连接地址

```plaintext
ws://127.0.0.1:{port}/


```

当前实现不区分连接 path，`/` 与任意子路径均可接受。

#### 消息格式

*   帧类型：UTF-8 JSON 文本帧

*   结构与 `sdk_result_cb_t` 回调收到的 JSON 一致（参见 §3.5 / §3.7）

*   常规 SDK 事件通常包含 `type`（事件名称）、`code`（返回码）和 `reqId` 字段；异常响应可能只在 `data` 中携带错误信息


#### 请求与事件关联

HTTP ACK 中的 `reqId > 0` 时，可用于匹配后续 WebSocket 事件。若 `reqId` 为 `0`，不要依赖它做精确关联，应结合 `type`、`envId` 和业务上下文判断事件归属。

#### 连接生命周期

| 场景 | 行为 |
| --- | --- |
| 未启用 Web API 前 | 本地服务未启动，TCP 连接被拒绝 |
| 连接建立时已有浏览器运行 | SDK 立即推送一次运行状态快照，可用于恢复 UI 显示 |
| 多个客户端并发连接 | 支持；所有 WebSocket 事件广播到全部已连接客户端 |
| 客户端断开 | SDK 不缓冲断线期间的事件；重连后可再次收到运行状态快照 |
| SDK shutdown | SDK 主动关闭所有 WebSocket 连接 |

#### 推荐接入顺序

1.  `sdk_init` 返回成功

2.  建立 WebSocket 连接，准备接收事件（可能立即收到运行状态快照）

3.  发送 HTTP 请求，例如 `POST /sdk/v1/browser/open`

4.  HTTP 响应返回 ACK，记录 `reqId`（若不为 `0`）

5.  通过 WebSocket 接收最终事件，如 `browser-open-success` 或 `browser-open-failed`


#### 主动推送事件

以下事件由 SDK 主动推送，不需要客户端先发起请求：

| 事件 | 触发场景 |
| --- | --- |
| `sdk-token-expire-warning` | token 剩余有效期低于阈值 |
| `sdk-token-expired` | token 已过期 |
| `browser-close-success`（含 `closeOrigin`） | 浏览器进程意外退出或被用户手动关闭 |

#### 通过 WebSocket 发送请求

WebSocket 帧中可包含带 `path` 字段的请求体，SDK 会按异步任务调度处理。当前实现所有 WebSocket 请求均视为异步，不区分同步接口语义。接入侧建议统一使用“HTTP 请求 + WebSocket 监听”的模式，语义更清晰。

### 5.4 Web API 同步与异步接口

| 接口 | HTTP 响应语义 | 是否需要 WebSocket |
| --- | --- | --- |
| `init`、`info`、`browser/info`、`netdiag` | 返回本次调用结果 | 不需要 |
| `browser/cleanup` | 返回本次清理结果 | 不需要 |
| `env/create`、`env/update`、`env/page`、`env/getinfo`、`env/destroy` | 返回后端原始结果 | 不需要 |
| `browser/install`、`browser/open`、`browser/close`、`token/update` | 返回受理 ACK | 需要，用于接收最终结果 |

### 5.5 `POST /sdk/v1/init`

同步初始化 SDK。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 后端签发的用户令牌 |
| `workDir` | string | 否 | 工作目录根路径，实际运行目录会被解析为 `workDir/appId` |
| `extsDir` | string | 否 | 本地扩展包根目录；不传时默认使用实际运行目录下的 `extensions` 目录 |
| `port` | integer | 否 | 内嵌 Web API 端口；不传则不开启，传 `0` 自动分配空闲端口 |
| `sdkApiUrl` | string | 否 | 覆盖 SDK 后端地址；不传则使用 SDK 内置默认地址 |
| `autoUpdateKernel` | bool | 否 | 覆盖 SDK 后端配置的`autoUpdateKernel`参数 |
| `logoPath` | string | 否 | 覆盖 SDK 后端配置用户图标资源，使用本地图标资源(绝对路径) |
| `debug` | bool | 否 | 开启开发者日志，默认 `false` |
| `verbose` | bool | 否 | 开启详细日志（输出更细粒度的内部事件），默认 `false`；生产环境建议关闭 |

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

*   `extsDir` 只在初始化阶段用于扫描本地扩展包，不是 `browser/open` 的动态安装入口

*   `extsDir` 为空时，SDK 会使用实际运行目录下的 `extensions` 目录，例如 `workDir/appId/extensions`

*   目录结构支持 `extsDir/{extension}/manifest.json`，也支持 `extsDir/{extension}/{version}/manifest.json`

*   扩展 `manifest.json` 必须包含 `key`，SDK 会按 Chrome 扩展 ID 算法从该 `key` 生成扩展 ID

*   当前只加载 Manifest V3 扩展；`manifest_version <= 2` 的扩展会被忽略


当前实现的重要限制：

*   `sdk_init` 在同一进程内是全局串行入口

*   若已有初始化操作进行中，后续调用直接返回 `CL_WBUSY`


### 5.6 `POST /sdk/v1/info`

同步获取 SDK 运行信息。

请求体：

*   推荐传空对象 `{}`

*   空 body 也可


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
      "workDir": "C:/BroSDK/1234567890",
      "tokenExpiresInS": 3600,
      "dataFullyManaged": true
    },
    "eventId": 10131
  }
}


```

`data.info` 常用字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `deviceId` | string | 当前机器指纹 |
| `version` | string | SDK 版本号 |
| `startupTime` | integer | SDK 启动时间戳 |
| `coresInfo` | object | 已加载浏览器核心信息 |
| `netInfo` | object | 当前网络环境快照 |
| `workDir` | string | 实际运行目录 |
| `tokenExpiresInS` | integer | 当前 token 剩余有效秒数 |
| `dataFullyManaged` | bool | `true` 为全托管，`false` 为半托管 |

### 5.7 `POST /sdk/v1/netdiag`

同步执行网络 / 代理诊断。适用于浏览器启动前验证 `proxy` 与跳板链路可达性；调用方直接获取诊断结果，无需等待 WebSocket 事件。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `url` | string | 是 | 需要探测的目标 URL |
| `proxy` | string | 否 | 最终上游代理；为空时按直连诊断 |
| `bridgeProxy` | string | 否 | 诊断链路中的前置跳板；当前接口只读取该字段，不读取 `forward` 或 `bridge` |

`netdiag` 是独立诊断接口，不执行 `browser/open` 的完整策略脚本。若要诊断本次启动中的 `forward` 或 `bridge` 链路，请按实际启动链路映射字段：

*   `forward + proxy`：把环境 `proxy` 放到 `proxy`，把本次 `forward` 放到 `bridgeProxy`

*   只有 `forward`、没有环境 `proxy`：把本次 `forward` 放到 `proxy`，`bridgeProxy` 留空

*   `bridge + proxy`：把环境 `proxy` 放到 `proxy`，把本次 `bridge` 放到 `bridgeProxy`


请求示例：

```json
{
  "url": "https://example.com",
  "proxy": "socks5://target-proxy:5206",
  "bridgeProxy": "socks5://jump-proxy:31034"
}


```

成功响应示例：

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

    "events": [ ]

  }
}


```

`bridgeDiagnostics`、`urlProbe` 和 `events[ ].diagnostics` 是诊断明细，字段会随底层网络探测能力扩展；接入侧应优先判断顶层 `ok/code/msg`，再展示或记录明细。

### 5.8 `POST /sdk/v1/browser/info`

同步获取当前运行中的浏览器列表。

请求体：

*   SDK 层返回全部运行中环境，不支持请求体过滤

*   建议传 `{}` 或空 body


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

### 5.9 `POST /sdk/v1/browser/install`

异步安装浏览器核心资源。请求会根据初始化凭证中的核心版本列表匹配可安装包，下载 / 安装完成后 SDK 会重新加载本地核心列表。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `cores` | array | 是 | 需要安装的浏览器核心列表 |

| `cores[ ].major` | integer / string | 是 | 浏览器核心主版本号，例如 `134` |

请求示例：

```json
{
  "cores": [
    { "major": 134 }
  ]
}


```

即时 ACK 示例：

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

*   `browser-install`

*   `browser-install-progress`

*   `browser-install-success`

*   `browser-install-failed`


### 5.10 `POST /sdk/v1/browser/open`

异步打开一个或多个浏览器环境。

顶层请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要打开的环境列表 |

`envs[ ]` 支持三种写法：

*   直接传字符串环境 ID

*   直接传数字环境 ID

*   传对象，对象中包含 `envId` 和可选覆盖参数


`envs[ ]` 对象字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |
| `forward` | string | 否 | 本次启动使用的显式代理；优先级最高，高于 `bridge` 和环境绑定的 `bridgeProxy`；有环境 `proxy` 时作为前置跳板，无环境 `proxy` 时作为最终代理 |
| `bridge` | string | 否 | 本次启动使用的备用前置跳板；非空时覆盖环境绑定的 `bridgeProxy`，优先级低于 `forward` |
| `args` | array | 否 | Chromium 兼容命令行参数，每项为完整 switch |
| `urls` | array | 否 | 启动后自动打开的 URL |
| `extensions` | array | 否 | 传给已加载扩展的本次启动数据配置 |
| `cookies` | array | 否 | 本次启动注入的 Cookie JSON 数组 |
| `yunConfig` | object | 否 | 定制浏览器透传内容 |

`browser/open` 不支持通过请求体覆盖环境绑定的 `proxy`。`bridgeProxy` 建议在创建 / 更新环境时绑定；如果只想覆盖本次启动的备用跳板，请在 `browser/open` 中传入 `bridge`。如果本次启动必须强制使用某个代理，请传入 `forward`。

`args[ ]` 仅描述 Chromium 兼容命令行参数，每项应是完整字符串，例如：

*   `--no-first-run`

*   `--no-default-browser-check`

*   `--disable-web-security`

*   `--remote-allow-origins=*`

*   `--remote-debugging-port=0`


`extensions[ ]` 对象字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | string | 是 | 已从 `extsDir` 加载的 Chrome 扩展 ID |
| `data` | object<string,string> | 否 | 传给扩展的键值数据；键和值均为字符串，具体编码由扩展约定 |

扩展加载与传参是两个阶段：

*   `sdk_init` 阶段通过 `extsDir` 扫描并加载本地扩展包

*   `browser/open` 阶段的 `extensions[ ]` 不会安装新扩展，只会按 `id` 匹配已加载扩展，并把 `data` 透传给该扩展

*   如果 `extensions[ ].id` 没有匹配到初始化阶段已加载的扩展，该项会被忽略

*   扩展 ID 来自扩展 `manifest.json` 中的 `key`；请保持同一扩展的 `key` 稳定，否则生成的 ID 会变化


`cookies[ ]` 对象字段兼容浏览器 Cookie JSON：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `domain` | string | 是 | Cookie 域 |
| `name` | string | 是 | Cookie 名称 |
| `value` | string | 是 | Cookie 值 |
| `path` | string | 是 | Cookie 路径，通常为 `/` |
| `expirationDate` | number | 否 | 过期时间，Unix 秒时间戳；会话 Cookie 可不传 |
| `hostOnly` | bool | 否 | 是否仅主机匹配 |
| `httpOnly` | bool | 否 | 是否 HttpOnly |
| `sameSite` | string / null | 否 | 可为 `lax`、`strict`、`no_restriction` 或 `null` |
| `secure` | bool | 否 | 是否 Secure |
| `session` | bool | 否 | 是否会话 Cookie |
| `storeId` | string / null | 否 | Cookie store ID |

`yunConfig{}` 对象字段：

在yunConfig JSON对象内容会平铺透传到定义版浏览器，例如 `shop{}`、`whitelist[ ]`、`blacklist[ ]` ...

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `shop` | object | 否 | 咨询定制版浏览器厂商 |
| `whitelist` | array | 否 |  |
| `blacklist` | array | 否 |  |

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
                "--remote-allow-origins=*",
            ],
            "urls": [
                "https://myip.ipipv.com"
            ],
            "yunConfig": {
                "shop": {
                    "shopId": "cd9ff4d2e44746a5ab58b56c546dfcc6",
                    "name": "141",
                    "shortName": "27008",
                    "platform": "",
                    "serial": "27008"
                },
                "whitelist": [
                    "www.baidu.com"
                ],
                "blacklist": [
                    "www.cn.bing.com"
                ]
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

即时 ACK 示例：

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

*   `browser-open`

*   `browser-open-success`

*   `browser-open-failed`

*   `browser-open-timeout`


关键语义：

> `browser-open-success` 表示浏览器已完全启动且就绪。 Cookie / Storage / 扩展 / 自动化逻辑应以此通知作为就绪信号。

如果代理桥启动失败或运行中代理探测失败，但浏览器最终仍然启动成功，最终事件仍是 `browser-open-success`，但 `code` 可以是 `CL_WPROXYDEGRADED`。这类“成功但有降级”的情况会在后端上报日志中以 `Warn` 级别和 `extra.lifecycle.steps[ ].name="proxy"` 的步骤明细体现。

`whiteList` / `blackList` 会随环境配置写入浏览器侧配置；具体命中策略由当前浏览器核心实现决定。`extensions[ ].data` 与 `cookies[ ]` 可能包含敏感数据，接入层应限制数组长度、条目长度和字段格式，并在日志中做脱敏处理。

### 5.11 `POST /sdk/v1/browser/close`

异步关闭一个或多个浏览器环境。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要关闭的环境列表 |

`envs[ ]` 支持：

*   字符串环境 ID

*   数字环境 ID

*   仅包含 `envId` 的对象


即时 ACK 示例：

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

  "envList": [ ]

}


```

如果浏览器是被用户手动关窗，或者进程在运行中自行退出，SDK 仍会发送 `browser-close-success`，但会在 `data` 中附带关闭原因：

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
    "closeReasonCode": 103,
    "closeReasonName": "WBRWPROCEXITED",
    "closeReasonMsg": "browser process exited unexpectedly",
    "closeOrigin": "process-exited"
  },

  "envList": [ ]

}


```

关键语义：

> 即时 ACK 只表示关闭任务已被受理。   只有收到 `browser-close-success`，才表示环境已关闭完成。

### 5.12 `POST /sdk/v1/browser/cleanup`

同步清理本地浏览器缓存。该接口同时支持清理指定环境的 `userdatadir` 缓存，以及浏览器内核下载缓存 `cores/.cache`。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 否 | 要清理 `userdatadir` 缓存的环境 ID 列表，最多 128 个 |
| `cores` | array | 否 | 要清理的内核下载缓存列表；字段缺省表示不清理内核缓存，空数组表示清理全部 `.cache` 文件 |
| `cores[ ].major` | integer / string | 条件必填 | 要清理的浏览器核心主版本号，例如 `141` |
| `cores[ ].type` | string | 否 | 内核类型过滤，例如 `chrome`；不传则仅按 `major` 匹配 |

`envs` 和 `cores` 至少需要提供一个有效清理目标；两者可以同时提供。

`envs[ ]` 支持：

*   字符串环境 ID

*   数字环境 ID


请求示例：

```json
{
  "envs": [
    "2041415694746128384",
    "2041415694746128385"
  ]
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

同时清理环境 `userdatadir` 和内核下载缓存：

```json
{
  "envs": [
    "2041415694746128384"
  ],
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

*   `browser/cleanup` 删除指定环境的本地 `userdatadir` 缓存时，不销毁后端环境记录

*   `cores` 只删除内核下载缓存 `cores/.cache` 中的 `.zip`、`.part`、`.merge` 文件，不删除已安装内核目录

*   `cores` 字段缺省表示不处理内核缓存；`"cores": []` 表示清理全部 `.cache` 缓存；`"cores": [{"major": 141}]` 表示只清理指定主版本缓存

*   如果指定环境正在运行，或仍处于 `browser/open` / `browser/close` 队列中，该环境返回 `CL_EBUSY`，结果项状态为 `busy`

*   如果请求中任一环境忙碌，顶层 `code` 返回 `CL_EBUSY`；如果存在非法环境 ID 或非法 `cores` 项，顶层 `code` 返回 `CL_EINVALID`

*   `not_found` 表示该环境本地缓存目录不存在，按清理完成处理


### 5.13 `POST /sdk/v1/token/update`

异步刷新 `userSig`。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 新的用户令牌 |

即时 ACK 示例：

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

### 5.14 `POST /sdk/v1/env/create`

同步创建环境。

当前行为：

*   请求体直接转发到后端 `env/create`

*   响应体直接返回后端原始 JSON

*   不追加 BroSDK 自己的 envelope


参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/create` 接口契约。

### 5.15 `POST /sdk/v1/env/update`

同步更新环境。

当前行为：

*   请求体直接转发到后端 `env/update`

*   响应体直接返回后端原始 JSON


参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/update` 接口契约。

### 5.16 `POST /sdk/v1/env/page`

同步分页查询环境。

当前行为：

*   请求体直接转发到后端 `env/page`

*   响应体直接返回后端原始 JSON


参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/page` 接口契约。

### 5.17 `POST /sdk/v1/env/getinfo`

同步获取单个环境的后端 `getEnvInfo` 结果。

当前行为：

*   请求体直接转发到后端 `getEnvInfo`

*   响应体直接返回后端原始 JSON

*   SDK 打开浏览器时也会基于该结果构造环境配置，因此这里返回的字段通常比分页列表更完整


请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |
| `os` | string | 否 | 当前系统标识，建议传 `Windows`、`macOS` 或 `Linux` |

请求示例：

```json
{
  "envId": "2041695386304778240",
  "os": "Windows"
}


```

### 5.18 `POST /sdk/v1/env/destroy`

同步销毁环境。

当前行为：

*   请求体直接转发到后端 `env/destroy`

*   响应体直接返回后端原始 JSON

*   **注意：销毁环境不等于关闭浏览器**。如果该环境的浏览器仍在运行，请先调用 `browser/close` 并等待 `browser-close-success`，再调用 `env/destroy`


参数与响应结构不在本文维护，请查阅环境后端对接文档中的 `env/destroy` 接口契约。

### 5.19 `POST /sdk/v1/shutdown`

同步停止 SDK。

当前行为：

*   停止 SDK

*   关闭内嵌 Web API 服务

*   销毁当前单例


## 6. 动态库接口参考

头文件：

```c
#include "brosdk.h"


```

### 6.1 核心类型

```c
typedef void *sdk_handle_t;


```

不透明的 SDK 实例句柄。

```c
typedef void(SDK_CALL *sdk_result_cb_t)(
    int32_t code,
    void *user_data,
    const char *data,
    size_t len);


```

动态库接口的统一异步结果回调。业务字段请以 JSON body 为准。

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

SDK 日志回调。`SDK_LOG_TYPE_LOCAL` 表示本地 SDK 日志行；`SDK_LOG_TYPE_SERVER` 表示已进入 sdk-server 上传队列的结构化日志 JSON。回调中的 `data` 只在当前回调有效。

```c
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(
    const char *data,
    size_t len,
    char **new_data,
    size_t *new_len,
    void *user_data);


```

Cookie 持久化前的拦截回调。

### 6.2 生命周期与信息接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_register_result_cb` | 同步 | 注册全局异步回调 |
| `sdk_register_log_cb` | 同步 | 注册 SDK 日志回调，传 `NULL` 可关闭 |
| `sdk_register_cookies_storage_cb` | 同步 | 注册 Cookie 拦截回调 |
| `sdk_init_cpp` | 同步 | 仅获取 SDK 句柄，不执行初始化 |
| `sdk_init` | 同步 | 初始化 SDK，返回堆分配的 JSON 响应 |
| `sdk_init_async` | 异步 | 受理后返回 `CL_DONE` 或实际 `reqId` |
| `sdk_init_webapi` | 同步辅助函数 | 兼容入口，新接入建议直接在 `sdk_init` 中携带 `"port"` |
| `sdk_info` | 同步 | 返回 SDK info JSON |
| `sdk_network_diagnostics` | 同步 | 返回网络 / 代理诊断 JSON |
| `sdk_browser_info` | 同步 | 返回当前运行中的浏览器列表 JSON |
| `sdk_token_update` | 异步 | 刷新用户令牌 |
| `sdk_shutdown` | 同步 | 停止 SDK 并销毁单例 |

`sdk_network_diagnostics` 的请求体与 `/sdk/v1/netdiag` 一致，返回的 `out_data` 必须通过 `sdk_free()` 释放。

### 6.3 浏览器接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_browser_install` | 异步 | 安装浏览器核心资源 |
| `sdk_browser_open` | 异步 | 最终 ready 信号是 `browser-open-success` |
| `sdk_browser_close` | 异步 | 最终关闭完成信号是 `browser-close-success` |
| `sdk_browser_cleanup` | 同步 | 清理环境 `userdatadir` 缓存和内核下载缓存，返回清理结果 JSON |

`sdk_browser_install` 的请求体与 `/sdk/v1/browser/install` 一致，必须包含 `cores[ ].major`。进度和最终结果通过 `sdk_result_cb_t` 回调返回。

`sdk_browser_open` 的请求体与 `/sdk/v1/browser/open` 一致。`envs[ ]` 对象可携带 `forward` 和 `bridge`；`forward` 是本次启动显式代理，`bridge` 只覆盖本次启动的备用跳板，不修改环境绑定的 `bridgeProxy`。

`sdk_browser_cleanup` 的请求体与 `/sdk/v1/browser/cleanup` 一致，返回的 `out_data` 必须通过 `sdk_free()` 释放。`envs` 用于清理环境 `userdatadir`，`cores` 用于清理内核下载缓存 `cores/.cache`。运行中或正在打开/关闭队列中的环境会返回 `CL_EBUSY`，结果项状态为 `busy`。

### 6.4 环境接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_env_create` | 同步 | 返回后端原始 JSON |
| `sdk_env_update` | 同步 | 返回后端原始 JSON |
| `sdk_env_page` | 同步 | 返回后端原始 JSON |
| `sdk_env_getinfo` | 同步 | 返回后端 `getEnvInfo` 原始 JSON |
| `sdk_env_destroy` | 同步 | 返回后端原始 JSON |

`sdk_env_getinfo` 的请求体与 `/sdk/v1/env/getinfo` 一致，返回的 `out_data` 必须通过 `sdk_free()` 释放。C++ `ISDK` 虚接口提供 `NetworkDiagnostics(...)` 与 `GetEnvInfo(...)`，语义与对应动态库接口一致。

### 6.5 Cookie 回调的当前行为

`sdk_register_cookies_storage_cb()` 有三个接入细节需注意：

*   回调接收的是明文 Cookie JSON 数组，而非最终落盘 / 上传的加密二进制

*   若返回替换后的 JSON，SDK 将以该 JSON 继续执行后续加密与持久化

*   Cookie 快照为空时，SDK 也会规范化为 `[ ]` 后触发回调，不静默跳过


### 6.6 内存与辅助接口

```c
SDK_API void *SDK_CALL sdk_malloc(size_t size);
SDK_API void  SDK_CALL sdk_free(void *ptr);


```

所有 SDK 返回的动态内存都应通过 `sdk_free()` 释放。

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

## 7. Cookie 与 Storage 持久化语义

### 7.1 托管模式

可通过 `sdk_info().dataFullyManaged` 判断当前模式：

| 值 | 模式 | 说明 |
| --- | --- | --- |
| `false` | 半托管 | 只落本地 SQLite，不做 OSS 同步 |
| `true` | 全托管 | SQLite 作为本地缓存，同时后台同步 OSS |

### 7.2 浏览器关闭时的持久化链路

当前实现中，浏览器关闭后的 Cookie 持久化链路如下：

1.  从浏览器快照提取 Cookie JSON 数组

2.  调用 `sdk_cookies_storage_cb_t`，允许宿主查看或替换明文 JSON

3.  把最终 JSON 转成 Cookie protobuf

4.  使用 `BroEncryptCookiesWithDEK(appId, coKeyVer, coKey, dek)` 加密 Cookie

5.  将加密 envelope 再次打包为 protobuf，并做 `br` 压缩

6.  立即写入本地 SQLite

7.  若为全托管，再异步上传到 OSS


Storage 的链路不同：

*   Storage 不走 Cookie 这套 DEK 加密

*   当前实现是按存储策略收集浏览器数据文件，打包后做 `br` 压缩

*   本地 SQLite 与 OSS 持有的是同一份压缩归档


### 7.3 Cookie 在本地与 OSS 中的实际形态

当前实现里，Cookie 在本地 SQLite 和 OSS 中都不是明文 JSON：

| 存储位置 | 实际内容 |
| --- | --- |
| SQLite `cookies` 列 | `BroEncryptCookiesWithDEK` 生成的加密 Cookie 包，再经 `br` 压缩后的二进制 |
| OSS Cookie 对象 | 与本地 SQLite 相同的加密 Cookie 二进制 |

加密所依赖的关键材料来源：

| 字段 | 来源 |
| --- | --- |
| `appId` | SDK 当前应用上下文 |
| `coKey` / `coKeyVer` | SDK 初始化后的凭证信息 |
| `dek` | 当前环境 `envInfo` 中返回的 DEK |

因此，客户如需接管 Cookie 明文，只能在 `sdk_cookies_storage_cb_t` 回调阶段处理；一旦进入持久化阶段，SDK 保存和上传的都是加密后的二进制。

### 7.4 当前 OSS 对象路径口径

当前代码实现不会要求客户自己拼接 Cookie / Storage 对象名：

*   后端返回 `cookieUpPath` / `storageUpPath` 前缀

*   SDK 会自动去掉前导 `/`

*   然后自动追加 `{envId}-v1.br`


因此：

*   请勿在接入文档中硬编码旧版的 `cookies.pb` / `storage.zst` 路径模板

*   业务如需记录对象路径，请以后端返回的元数据与 SDK 实际回填结果为准


### 7.5 全托管模式下的本地缓存仲裁

现在本地 SQLite 还会维护以下元数据：

*   `sync_state`

*   `cookie_md5`

*   `storage_md5`

*   `cookie_file_url`

*   `storage_file_url`

*   `last_sync_ms`

*   `updated_ms`


浏览器打开时，全托管模式下会按以下规则判断是否直接使用本地 SQLite：

*   `sync_state = Dirty`：说明本地刚关闭过、OSS 可能还未同步，优先使用本地

*   `sync_state = UploadFailed`：说明上次上传失败，优先使用本地

*   后端远端元数据缺失：优先使用本地

*   本地 `md5` / `fileUrl` 与远端一致：优先使用本地

*   其余情况：认为远端更新更可信，回退到 OSS 下载


> 全托管模式下，SDK 不盲目信任 SQLite 缓存，而是先做本地 / 远端元数据比对。

**OSS 上传失败重试机制：**

当全托管模式下 OSS 上传失败时，SDK 不会丢弃数据：

*   本地 SQLite 中会保留 `sync_state = UploadFailed` 状态

*   下次该环境的浏览器启动时，SDK 会检测到 `UploadFailed` 并在生命周期内自动重试上传

*   重试时机：下次浏览器关闭后的持久化流程中

*   因此，即使 OSS 暂时不可用，数据也不会丢失，仅会延迟同步


### 7.6 `browser-close-success` 与上传完成不是同一件事

当前实现中：

*   `browser-close-success` 表示本地快照已经完成、SQLite 已可用于下次启动

*   它不表示 OSS 上传已经完成

*   如果全托管上传失败，本地 SQLite 会保留 `UploadFailed` 状态，后续生命周期仍可重试


## 8. 集成时必须注意的规则

*   先调用 `sdk_register_result_cb()`，再进入异步业务流程

*   把 `sdk_init` 当成进程内全局串行入口

*   对于异步动态库接口，返回 `CL_DONE` 或 `reqId` 都表示“请求已受理”

*   Web API 中 `init`、`info`、`browser/info`、`netdiag`、`browser/cleanup` 与 `env/*` 为同步接口；`browser/install`、`browser/open`、`browser/close`、`token/update` 为异步 ACK

*   Web API 的异步接口必须同时建立 WebSocket，用于接收最终结果

*   浏览器可用的真正信号是 `browser-open-success`

*   `browser-open-success` 是浏览器可用信号；代理桥降级这类“成功但有告警”的情况，`code` 可以是 `CL_WPROXYDEGRADED`，请同时查看后端上报日志的 `level=Warn` 和 `extra.lifecycle`

*   浏览器真正关闭完成的信号是 `browser-close-success`

*   `browser-close-success` 只保证本地持久化完成，不保证 OSS 上传完成

*   如果浏览器是用户手动关闭或进程自行退出，仍可能上报 `browser-close-success`，但 `data.closeReason*` 会说明真实原因

*   `env/*` 接口和 `init/info/browser/*` 的回包包装方式不同，前者是后端原始 JSON 透传；`netdiag` 也是同步原始诊断 JSON；`browser/cleanup` 返回同步清理结果 JSON

*   `env/create`、`env/update`、`env/page`、`env/destroy` 的请求参数与响应字段请以后端对接文档为准，SDK 文档不重复维护

*   如需代理行为稳定可预期，请在创建 / 更新环境时绑定 `proxy`，并在 `browser/open` 中按需传入 `forward` 或 `bridge`；`forward` 可作为本次启动显式代理，`bridgeProxy` 是绑定到环境上的备用前置跳板；勿依赖客户机器隐式系统代理环境

*   `netdiag` 不执行 `browser/open` 策略脚本；诊断 `forward` 或 `bridge` 时，请按启动链路把值映射到 `proxy` 或 `bridgeProxy`，不要假设它会自动读取 `forward` / `bridge`

*   `urls`、`args`、`whiteList`、`blackList`、`extensions`、`cookies` 这类数组字段建议由接入层做长度、条目格式和敏感值校验


## 9. 事件与错误码附录

常用事件码：

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
| `20603` | `browser-proxy-dns-resolve-failed` |
| `20604` | `browser-proxy-tcp-connect-failed` |
| `20605` | `browser-proxy-http-connect-rejected` |
| `20606` | `browser-proxy-socks5-auth-failed` |
| `20607` | `browser-proxy-socks5-connect-rejected` |
| `20608` | `browser-proxy-write-failed` |
| `20609` | `browser-proxy-degraded` |

常用错误 / 警告码：

| 代码 | 名称 | 说明 |
| --- | --- | --- |
| `0` | `CL_OK` | 成功 |
| `1` | `CL_DONE` | 异步任务已受理 |
| `101` | `CL_WDIRNOTEXIST` | 目录不存在；常见于清理本地缓存时目标目录已不存在 |
| `102` | `CL_WBRWPROCEXITED` | 浏览器进程自行退出；常见于手动关窗或运行中异常退出 |
| `103` | `CL_WPROXYDEGRADED` | 代理桥降级；浏览器已启动，但代理链路未完全按预期工作 |
| `104` | `CL_WBUSY` | 资源忙，另一个初始化操作正在进行 |
| `-3001` | `CL_EBUSY` | 资源忙；例如清理环境缓存时目标环境仍在运行或仍在打开/关闭队列中 |
| `-3002` | `CL_ETIMEOUT` | 超时 |
| `-3003` | `CL_EINVALID` | 参数错误 |
| `-3005` | `CL_EALREADY` | 已存在或重复初始化；进程内 SDK 单例已运行，无需再次初始化 |
| `-3012` | `CL_ENOTINITIALIZED` | SDK 未初始化 |
| `-3019` | `CL_EPORT_UNAVAILABLE` | 端口非法或已占用 |
| `-3023` | `CL_EOSS_NOCLIENT` | OSS 客户端未初始化 |
| `-3024` | `CL_EOSS_DOWNLOAD` | OSS 下载失败 |
| `-3025` | `CL_EOSS_UPLOAD` | OSS 上传失败 |
| `-3027` | `CL_EOSS_NOTFOUND` | OSS 对象不存在 |
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

本文明确区分了 BroSDK 直接生成的响应结构与直接透传后端的 env 接口响应结构。 如接入侧同时依赖完整的后端环境模型，请以后端 env API 契约作为完整字段集合的最终依据。
