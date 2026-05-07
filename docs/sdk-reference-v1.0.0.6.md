# BroSDK SDK 参考

> **当前版本：v1.0.0.6　　最后更新：2026-04-24**  
> 本文档是当前 BroSDK 的统一客户参考文档。  
> 内容覆盖原生 C API 与内嵌本地 HTTP / WebSocket API，并按当前 `projects/brosdk/interface/brosdk.h` 与 `projects/brosdk` 实现重新校准。  
> 如有字段或行为的疑问，请以头文件 `brosdk.h` 为最终依据。

## 1. 概述

BroSDK 是一个使用 C/C++ 实现的浏览器环境管理 SDK，对外提供两种接入方式：

- 原生 C API：直接加载 `brosdk.dll` / `brosdk.so` / `brosdk.dylib`
- 内嵌 Web API：初始化时开启本地 HTTP / WebSocket 服务，通过本地接口调用

两种方式共享同一套浏览器生命周期、环境管理和数据持久化实现，但异步结果的交付方式不同：

- C API：函数返回值 + `sdk_result_cb_t` 回调
- HTTP API：同步响应或异步 ACK，最终结果通过 WebSocket 推送

## 2. 平台支持

| 平台 | 架构 | 动态库 | 状态 | 备注 |
| --- | --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | 主验证平台 | 主力开发与测试平台，优先保障 |
| Linux | x64 | `brosdk.so` | 集成验证中 | 支持主流发行版，实验性支持 |
| macOS | arm64 / x64 | `brosdk.dylib` | 集成验证中 | arm64 为主要测试架构，实验性支持 |

## 3. 通用约定

### 3.1 JSON 与编码

- 所有请求体均为 UTF-8 JSON
- 所有返回体与通知体均为 UTF-8 JSON
- C API 中的 `len` 均为字节长度，不包含末尾 `\0`

### 3.2 内存管理

- 所有同步 C API 返回的 `out_data`，使用后必须调用 `sdk_free()`
- 回调中的 `data` 指针只在当前回调有效，如需长期保存请立即复制
- `sdk_cookies_storage_cb_t` 若要替换 Cookie 数据，`*new_data` 必须通过 `sdk_malloc()` 分配

### 3.3 返回值、`reqId` 与辅助判断

当前实现里需要区分“同步返回码”和“异步请求受理 ID”：

| 数值范围 | 含义 |
| --- | --- |
| `0` | `CL_OK`，同步成功 |
| `1` | `CL_DONE`，异步任务已受理，但当前返回未携带实际 `reqId` |
| `> 100000` | 异步请求已受理，并直接返回实际 `reqId` |
| `100 ~ 255` | Warning，例如 `CL_WBUSY` |
| `< 0` | Error |

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

- 当前异步 C API 可能返回 `CL_DONE`，也可能直接返回 `reqId`
- 对于 HTTP 异步 ACK，顶层 `reqId` 也可能是 `0`，也可能是实际受理到的请求 ID
- 业务接入时，应把“`sdk_is_done(code)` 或 `sdk_is_reqid(code)` 为真”都视为“异步任务已进入调度”

### 3.4 异步回调语义

`sdk_result_cb_t` 是原生接入时统一的异步结果通道。

当前实现请这样理解：

- 回调第一个参数 `code` 是本次通知的粗粒度状态
- `reqId`、`type`、`eventId` 及业务字段以 JSON body 为准
- 不要再把回调第一个参数当成稳定的 `reqId` 或 `eventId`

推荐做法：

1. 解析 JSON body
2. 优先根据顶层 `type` 路由
3. 如需关联请求，使用顶层 `reqId`
4. `data.eventId` 作为事件枚举补充字段使用

**线程安全注意事项：**

- `sdk_cookies_storage_cb_t` 在 SDK 内部已串行化，不会并发调用
- `sdk_result_cb_t` 不保证串行，同一回调可能在不同事件驱动时并发调用
- 建议宿主侧在回调中加锁，或只做最小操作（如入队）后在业务线程消费
- 无论哪种回调，**不要在回调中直接调用 SDK 的阻塞/同步接口**（如 `sdk_init`、`sdk_shutdown`），否则可能导致死锁

### 3.5 同步响应 Envelope

大多数由 BroSDK 自己生成的同步响应，结构如下：

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
| `code` | int | SDK 返回码 |
| `reqId` | int | SDK 请求 ID |
| `type` | string | 事件名称 |
| `msg` | string | 可读状态描述 |
| `data` | object | 接口专属数据 |

例外：

- `env/create`、`env/update`、`env/page`、`env/destroy` 为后端原始 JSON 透传
- 这些接口不会额外套一层 BroSDK 自己的 envelope

### 3.6 异步 HTTP ACK Envelope

异步 HTTP 接口返回的是“已受理 ACK”，不是最终结果：

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
    "remoteDebuggingPort": 65534,
    "cdpReady": true
  },
  "envList": [
    {
      "envId": "2041695386304778240",
      "status": 2,
      "statusName": "Started",
      "progress": 100,
      "cdpReady": true
    }
  ]
}
```

`data` 常见字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `envId` | string | 环境 ID |
| `status` | int | 浏览器生命周期状态 |
| `statusName` | string | 生命周期状态名称 |
| `progress` | int | 当前进度百分比 |
| `remoteDebuggingPort` | int | CDP 端口，若当前实例已开启 |
| `cdpReady` | bool | CDP 是否已可用于业务逻辑 |

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

## 4. 代理字段与当前策略

### 4.1 字段口径

当前实现中，以下字段都会参与浏览器启动时的代理桥决策：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `proxy` | string | 最终上游代理 |
| `forward` | string | 显式前置跳板，优先级高于 `bridgeProxy` |
| `bridgeProxy` | string | 备用前置跳板 |

推荐统一使用完整代理 URL：

- `socks5://host:port`
- `socks5://user:pass@host:port`
- `socks5h://user:pass@host:port`
- `http://user:pass@host:port`

### 4.2 当前默认策略的四条决策规则

当前版本在每次浏览器启动前，会依次评估以下四条规则（按优先级从高到低）：

| 优先级 | 条件 | 实际行为 |
| --- | --- | --- |
| 1 | `proxy` 为空 | 不走业务代理，`bridgeProxy` 和 `forward` 同时清空 |
| 2 | `proxy` 有值 + 宿主机已具备出海能力（`global=true`） | 直接使用 `proxy`，忽略 `bridgeProxy` 和 `forward` |
| 3 | `proxy` 有值 + `global=false` + `forward` 有值 | `本地 bridge -> forward -> proxy -> 目标网站` |
| 4 | `proxy` 有值 + `global=false` + `forward` 为空 + `bridgeProxy` 有值 | `本地 bridge -> bridgeProxy -> proxy -> 目标网站` |

> `global` 是 SDK 内部对宿主机出海能力的判断值，不是客户配置字段。

进入代理桥时，SDK 会为每个浏览器实例单独启动一个本地 loopback bridge，并向 Chromium 注入：

```text
--proxy-server=socks5://127.0.0.1:{port}
```

关键约束：

- `forward` 和 `bridgeProxy` 不会同时生效，**当前不支持双跳前置链**（即不存在 `forward -> bridgeProxy -> proxy` 这种路径）
- 若希望行为稳定、可预期，请显式传入 `proxy`，并按需配置 `forward` 或 `bridgeProxy`

### 4.3 当前默认策略的重要说明

与旧版本文档相比，当前实现有三个必须明确的变化：

1. `forward` 是实际可用字段，不再只是内部概念
2. 当宿主机已具备出海能力（`global=true`）时，SDK **会**自动忽略 `forward` 和 `bridgeProxy`，仅使用 `proxy`
3. 当 `proxy` 为空时，SDK 不会注入本地代理桥，同时清空 `bridgeProxy` 和 `forward`

这意味着：

- `proxy` 为空时，浏览器将回到 Chromium 默认网络栈
- 此时是否使用系统代理，取决于 Chromium / 操作系统默认行为，而不是 BroSDK 自己的代理桥逻辑
- 如果你希望行为稳定、可预测，建议显式传入完整代理 URL，而不要依赖客户机器上的隐式系统配置
- `forward` 和 `bridgeProxy` 是互斥的前置跳板，不要期望同时配置二者来构建双跳前置链

### 4.4 故障与回退

当前实现中，如果代理桥启动失败：

- SDK 会记录 `browser-proxy-*` 相关诊断日志
- 浏览器仍然会继续启动
- 但此时不会再使用 SDK 管理的本地代理桥

## 5. Web API 参考

### 5.1 启用方式

在初始化请求中携带 `port` 即可启用本地服务：

```json
{
  "userSig": "your-user-sign",
  "port": 9527
}
```

初始化完成后：

- HTTP 地址：`http://127.0.0.1:{port}`
- WebSocket：同一服务端口，用于接收异步事件

### 5.2 认证模型

内嵌本地 HTTP API 当前不依赖 `Authorization` Header。

当前模型如下：

- `/sdk/v1/init` 在 JSON body 中携带 `userSig`
- 后续本地 HTTP 请求复用当前已初始化的 SDK 实例
- `/sdk/v1/token/update` 在 JSON body 中刷新 `userSig`

### 5.3 WebSocket 使用说明

如果你调用的是 `browser/open`、`browser/close`、`token/update` 这类异步 HTTP 接口，必须同时建立 WebSocket 连接，才能拿到最终结果。

当前行为：

- 浏览器打开 / 关闭的最终通知通过 WebSocket 广播
- 推送体与原生 C API 回调收到的 JSON 结构一致
- 如果只发 HTTP 请求、不连 WebSocket，那么你只能拿到即时 ACK

### 5.4 `POST /sdk/v1/init`

同步初始化 SDK。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 后端签发的用户令牌 |
| `workDir` | string | 否 | 工作目录根路径，实际运行目录会被解析为 `workDir/appId` |
| `port` | integer | 否 | 内嵌本地 HTTP / WS 端口，不传则不开启 Web API |
| `sdkApiUrl` | string | 否 | 覆盖 SDK 后端地址；不传则使用 SDK 内置默认地址 |
| `debug` | bool | 否 | 开启开发者日志，默认 `false` |
| `verbose` | bool | 否 | 开启详细日志（输出更细粒度的内部事件），默认 `false`；生产环境建议关闭 |

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

当前实现的重要限制：

- `sdk_init` 在同一进程内是全局串行入口
- 如果另一个初始化正在进行，后续初始化会被直接拒绝为 `CL_WBUSY`

### 5.5 `POST /sdk/v1/info`

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
      "version": "1.0.0.25",
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

`data.info` 常用字段：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `deviceId` | string | 当前机器指纹 |
| `version` | string | SDK 版本号 |
| `startupTime` | integer | SDK 启动时间戳 |
| `coresInfo` | object | 已加载浏览器核心信息 |
| `netInfo` | object | 当前网络环境快照 |
| `workDir` | string | 实际运行目录 |
| `tokenExpiresInS` | integer | 当前 token 剩余有效秒数 |
| `dataFullyManaged` | bool | `true` 为全托管，`false` 为半托管 |

### 5.6 `POST /sdk/v1/browser/info`

同步获取当前运行中的浏览器列表。

请求体：

- 当前 SDK 层始终返回全部运行中环境
- SDK 层暂未实现请求体过滤
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

### 5.7 `POST /sdk/v1/browser/open`

异步打开一个或多个浏览器环境。

顶层请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要打开的环境列表 |

`envs[]` 支持三种写法：

- 直接传字符串环境 ID
- 直接传数字环境 ID
- 传对象，对象中包含 `envId` 和可选覆盖参数

`envs[]` 对象字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |
| `urls` | array<string> | 否 | 启动后自动打开的 URL |
| `args` | array<string> | 否 | 额外 Chromium 启动参数 |
| `cookies` | array<object> | 否 | 本次启动注入的 Cookie JSON 数组 |
| `extensions` | array<object> | 否 | 本次启动加载的扩展 |
| `proxy` | string | 否 | 最终上游代理 |
| `forward` | string | 否 | 前置跳板，优先级高于 `bridgeProxy` |
| `bridgeProxy` | string | 否 | 备用前置跳板 |
| `envName` | string | 否 | 环境显示名覆盖 |
| `shortName` | string | 否 | 短名称覆盖 |
| `serial` | string | 否 | 业务序列号覆盖 |
| `searchEngine` | object | 否 | 搜索引擎覆盖配置 |
| `enableDevtools` | integer | 否 | DevTools 策略覆盖 |
| `enableStorage` | integer | 否 | Storage 策略覆盖 |
| `finger` | object | 否 | 浏览器指纹对象 |

请求示例：

```json
{
  "envs": [
    {
      "envId": "2041695386304778240",
      "urls": ["https://example.com"],
      "args": ["--no-first-run", "--remote-allow-origins=*"],
      "proxy": "socks5://target-proxy:5206",
      "forward": "socks5://jump-proxy:31034"
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

关键语义：

> `browser-open-success` 等价于浏览器已经完全启动，并且 CDP 已经就绪。  
> Cookie / Storage / 扩展 / 自动化逻辑应以这条通知作为真正的 ready 信号。

### 5.8 `POST /sdk/v1/browser/close`

异步关闭一个或多个浏览器环境。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要关闭的环境列表 |

`envs[]` 支持：

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
    "progress": 100,
    "cdpReady": false
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
    "cdpReady": false,
    "closeReasonCode": 103,
    "closeReasonName": "WBRWPROCEXITED",
    "closeReasonMsg": "browser process exited unexpectedly",
    "closeOrigin": "process-exited"
  },
  "envList": []
}
```

关键语义：

> 即时 ACK 只表示关闭任务已被受理。  
> 只有收到 `browser-close-success`，才表示环境真的关闭完成。

### 5.9 `POST /sdk/v1/token/update`

异步刷新 `userSig`。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 新的用户令牌 |

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

### 5.10 `POST /sdk/v1/env/create`

同步创建环境。

当前行为：

- 请求体直接转发到后端 `env/create`
- 响应体直接返回后端原始 JSON
- 不追加 BroSDK 自己的 envelope

常见请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envName` | string | 否 | 环境名称 |
| `shortName` | string | 否 | 短名称 |
| `serial` | string | 否 | 业务序列号 |
| `proxy` | string | 否 | 最终上游代理 |
| `forward` | string | 否 | 前置跳板 |
| `bridgeProxy` | string | 否 | 备用前置跳板 |
| `finger` | object | 否 | 浏览器完整指纹对象 |
| `searchEngine` | object | 否 | 搜索引擎配置 |
| `enableDevtools` | integer | 否 | DevTools 策略 |
| `enableStorage` | integer | 否 | Storage 策略 |
| `extensions` | array | 否 | 扩展定义，结构见下方示例 |

`extensions[]` 对象结构：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `path` | string | 扩展本地绝对路径（目录或 .crx 文件路径） |
| `id` | string | 扩展 ID（可选，主要用于标识/去重） |

示例：

```json
{
  "envName": "测试环境",
  "proxy": "socks5://127.0.0.1:7890",
  "extensions": [
    { "path": "/path/to/extension-dir", "id": "extension-unique-id" }
  ],
  "finger": { "os": "Windows", "platform": "Win32" }
}
```

后端原始响应示例：

```json
{
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2041695386304778240",
    "envName": "测试环境"
  }
}
```

### 5.11 `POST /sdk/v1/env/update`

同步更新环境。

当前行为：

- 请求体直接转发到后端 `env/update`
- 响应体直接返回后端原始 JSON

常见请求字段与 `env/create` 基本一致，额外必填：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |

### 5.12 `POST /sdk/v1/env/page`

同步分页查询环境。

当前行为：

- 请求体直接转发到后端 `env/page`
- 响应体直接返回后端原始 JSON

常见请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `customerId` | string | 否 | 三方业务用户 ID |
| `envIds` | array | 否 | 环境筛选列表 |
| `pageIndex` | integer | 否 | 页码，从 `1` 开始 |
| `pageSize` | integer | 否 | 每页数量 |

请求示例：

```json
{
  "pageIndex": 1,
  "pageSize": 20
}
```

### 5.13 `POST /sdk/v1/env/destroy`

同步销毁环境。

当前行为：

- 请求体直接转发到后端 `env/destroy`
- 响应体直接返回后端原始 JSON
- **注意：销毁环境不等于关闭浏览器**。如果该环境的浏览器仍在运行，请先调用 `browser/close` 并等待 `browser-close-success`，再调用 `env/destroy`

常见请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |

### 5.14 `POST /sdk/v1/shutdown`

同步停止 SDK。

当前行为：

- 停止 SDK
- 关闭本地 HTTP / WS 服务
- 销毁当前单例

## 6. 原生 C API 参考

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

统一异步结果回调。业务字段请以 JSON body 为准。

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

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_register_result_cb` | 同步 | 注册全局异步回调 |
| `sdk_register_cookies_storage_cb` | 同步 | 注册 Cookie 拦截回调 |
| `sdk_init_cpp` | 同步 | 只获取当前 SDK 句柄，不执行初始化 |
| `sdk_init` | 同步 | 初始化 SDK，并返回 malloc 出来的 JSON 响应 |
| `sdk_init_async` | 异步 | 受理后返回 `CL_DONE` 或实际 `reqId` |
| `sdk_init_webapi` | 同步辅助函数 | 兼容入口，新接入建议直接在 `sdk_init` 中携带 `"port"` |
| `sdk_info` | 同步 | 返回 SDK info JSON |
| `sdk_browser_info` | 同步 | 返回当前运行中的浏览器列表 JSON |
| `sdk_token_update` | 异步 | 刷新用户令牌 |
| `sdk_shutdown` | 同步 | 停止 SDK 并销毁单例 |

### 6.3 浏览器接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_browser_install` | 异步 | 安装浏览器核心资源 |
| `sdk_browser_open` | 异步 | 最终 ready 信号是 `browser-open-success` |
| `sdk_browser_close` | 异步 | 最终关闭完成信号是 `browser-close-success` |

### 6.4 环境接口

| 函数 | 模式 | 说明 |
| --- | --- | --- |
| `sdk_env_create` | 同步 | 返回后端原始 JSON |
| `sdk_env_update` | 同步 | 返回后端原始 JSON |
| `sdk_env_page` | 同步 | 返回后端原始 JSON |
| `sdk_env_destroy` | 同步 | 返回后端原始 JSON |

### 6.5 Cookie 回调的当前行为

`sdk_register_cookies_storage_cb()` 当前还有三个接入细节需要特别注意：

- 回调拿到的是明文 Cookie JSON 数组，而不是最终落盘 / 上传的加密二进制
- 如果你返回替换后的 JSON，SDK 会继续用这份新 JSON 走后续加密和持久化
- 即使某次关闭时 Cookie 快照为空，SDK 也会规范化为 `[]` 后回调一次，不再静默跳过

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

| 值 | 模式 | 说明 |
| --- | --- | --- |
| `false` | 半托管 | 只落本地 SQLite，不做 OSS 同步 |
| `true` | 全托管 | SQLite 作为本地缓存，同时后台同步 OSS |

### 7.2 浏览器关闭时的持久化链路

当前实现中，浏览器关闭后的 Cookie 持久化链路如下：

1. 从浏览器快照提取 Cookie JSON 数组
2. 调用 `sdk_cookies_storage_cb_t`，允许宿主查看或替换明文 JSON
3. 把最终 JSON 转成 Cookie protobuf
4. 使用 `BroEncryptCookiesWithDEK(appId, coKeyVer, coKey, dek)` 加密 Cookie
5. 将加密 envelope 再次打包为 protobuf，并做 `br` 压缩
6. 立即写入本地 SQLite
7. 若为全托管，再异步上传到 OSS

Storage 的链路不同：

- Storage 不走 Cookie 这套 DEK 加密
- 当前实现是按存储策略收集浏览器数据文件，打包后做 `br` 压缩
- 本地 SQLite 与 OSS 持有的是同一份压缩归档

### 7.3 Cookie 在本地与 OSS 中的实际形态

当前实现里，Cookie 在本地 SQLite 和 OSS 中都不是明文 JSON：

| 存储位置 | 实际内容 |
| --- | --- |
| SQLite `cookies` 列 | `BroEncryptCookiesWithDEK` 生成的加密 Cookie 包，再经 `br` 压缩后的二进制 |
| OSS Cookie 对象 | 与本地 SQLite 相同的加密 Cookie 二进制 |

加密所依赖的关键材料来源：

| 字段 | 来源 |
| --- | --- |
| `appId` | SDK 当前应用上下文 |
| `coKey` / `coKeyVer` | SDK 初始化后的凭证信息 |
| `dek` | 当前环境 `envInfo` 中返回的 DEK |

因此，客户如需接管 Cookie 明文，只能在 `sdk_cookies_storage_cb_t` 回调阶段处理；一旦进入持久化阶段，SDK 保存和上传的都是加密后的二进制。

### 7.4 当前 OSS 对象路径口径

当前代码实现不会要求客户自己拼接 Cookie / Storage 对象名：

- 后端返回 `cookieUpPath` / `storageUpPath` 前缀
- SDK 会自动去掉前导 `/`
- 然后自动追加 `{envId}-v1.br`

因此：

- 不要在客户文档中硬编码旧版的 `cookies.pb` / `storage.zst` 路径模板
- 业务如需记录对象路径，请以后端返回的元数据与 SDK 实际回填结果为准

### 7.5 全托管模式下的本地缓存仲裁

旧版本文档中常见的说法是“全托管模式下，只要本地 SQLite 有数据就优先使用本地”。当前实现已经不是这样。

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

这也是近期修复的重要行为变化之一：

> 全托管模式下，SDK 不再盲目信任 SQLite 缓存，而是会先做本地 / 远端元数据比对。

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
- 对于异步 C API，返回 `CL_DONE` 或 `reqId` 都表示“请求已受理”
- 对于异步 HTTP 路由，必须同时建立 WebSocket
- 浏览器可用的真正信号是 `browser-open-success`
- 浏览器真正关闭完成的信号是 `browser-close-success`
- `browser-close-success` 只保证本地持久化完成，不保证 OSS 上传完成
- 如果浏览器是用户手动关闭或进程自行退出，仍可能上报 `browser-close-success`，但 `data.closeReason*` 会说明真实原因
- `env/*` 接口和 `init/info/browser/*` 的回包包装方式不同，前者是后端原始 JSON 透传
- 如果你希望代理行为稳定、可预期，请显式传入 `proxy` / `forward` / `bridgeProxy`，不要依赖客户机器上的隐式系统代理环境

## 9. 事件与错误码附录

常用事件码：

| 事件 ID | 名称 |
| --- | --- |
| `10110` | `sdk-init` |
| `10111` | `sdk-init-success` |
| `10112` | `sdk-init-failed` |
| `10120` | `sdk-token-update` |
| `10123` | `sdk-token-expire-warning` |
| `10124` | `sdk-token-expired` |
| `20110` | `browser-open` |
| `20111` | `browser-open-success` |
| `20112` | `browser-open-failed` |
| `20140` | `browser-close` |
| `20141` | `browser-close-success` |
| `20142` | `browser-close-failed` |
| `20210` | `browser-env-create` |
| `20220` | `browser-env-update` |
| `20230` | `browser-env-page` |
| `20240` | `browser-env-destroy` |
| `20600` | `browser-proxy` |
| `20601` | `browser-proxy-success` |
| `20602` | `browser-proxy-failed` |

常用错误 / 警告码：

| 代码 | 名称 | 说明 |
| --- | --- | --- |
| `0` | `CL_OK` | 成功 |
| `1` | `CL_DONE` | 异步任务已受理 |
| `103` | `CL_EALREADY` | 已存在或重复初始化；进程内 SDK 单例已运行，无需再次初始化 |
| `104` | `CL_WBUSY` | 资源忙，另一个初始化操作正在进行 |
| `-3002` | `CL_ETIMEOUT` | 超时 |
| `-3003` | `CL_EINVALID` | 参数错误 |
| `-3012` | `CL_ENOTINITIALIZED` | SDK 未初始化 |
| `-3019` | `CL_EPORT_UNAVAILABLE` | 端口非法或已占用 |
| `-3023` | `CL_EOSS_NOCLIENT` | OSS 客户端未初始化 |
| `-3024` | `CL_EOSS_DOWNLOAD` | OSS 下载失败 |
| `-3025` | `CL_EOSS_UPLOAD` | OSS 上传失败 |
| `-3502` | `CL_EHTTP_POST` | 后端 HTTP 失败 |
| `-3509` | `CL_ETOKEN_INVALID` | Token 无效 |
| `-3511` | `CL_EWORKDIR_INVALID` | 工作目录无效 |
| `-4000` 及以下 | `CL_ESDKAPI` 系列 | 后端 SDK API 错误 |

---

本文刻意区分了“BroSDK 自己生成的返回结构”和“直接透传后端的 env CRUD 返回结构”。  
如果你的接入同时依赖完整的后端环境模型，请以后端 env API 契约作为完整字段集合的最终依据。
