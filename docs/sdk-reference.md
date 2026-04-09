# BroSDK SDK 参考

> 本文档是当前 BroSDK 的统一参考文档。  
> 内容同时覆盖原生 C API 与内嵌本地 HTTP / WebSocket API，并已经按当前 `brosdk.h` 与 `projects/brosdk` 实现重新校准。

## 1. 概述

BroSDK 是一个使用 C/C++ 实现的浏览器环境管理 SDK，对外提供两种接入方式：

- 原生 C API：直接加载 `brosdk.dll` / `brosdk.so` / `brosdk.dylib`
- 内嵌 Web API：初始化时开启本地 HTTP / WebSocket 服务，通过本地接口调用

两种方式共享同一套任务系统与浏览器生命周期实现，但返回结果的方式并不完全相同：

- C API：函数返回值 + 全局异步回调
- HTTP API：同步响应或异步 ACK，最终异步结果通过 WebSocket 推送

## 2. 平台支持

| 平台 | 架构 | 动态库 | 状态 |
| --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | 主验证平台 |
| Linux | x64 | `brosdk.so` | 预发 / 集成验证中 |
| macOS | arm64 / x64 | `brosdk.dylib` | 预发 / 集成验证中 |

## 3. 通用约定

### 3.1 JSON 与编码

- 所有请求体均为 UTF-8 JSON
- 所有返回体与通知体均为 UTF-8 JSON
- C API 中的 `len` 均为字节长度，不包含末尾 `\0`

### 3.2 内存管理

- 所有同步 C API 返回的 `out_data`，使用后必须调用 `sdk_free()`
- 回调中的 `data` 指针只在当前回调有效，如需长期保存请立即复制
- 如果 `sdk_cookies_storage_cb_t` 要替换 Cookie 数据，`*new_data` 必须通过 `sdk_malloc()` 分配

### 3.3 返回码语义

| 数值范围 | 含义 |
| --- | --- |
| `0` | `CL_OK`，同步成功 |
| `1` | `CL_DONE`，异步任务已受理 |
| `100 ~ 255` | Warning，例如 `CL_WBUSY` |
| `< 0` | Error |

常用辅助函数：

- `sdk_is_ok(code)`
- `sdk_is_done(code)`
- `sdk_is_warn(code)`
- `sdk_is_error(code)`
- `sdk_error_name(code)`
- `sdk_error_string(code)`
- `sdk_event_name(evtid)`

### 3.4 异步回调语义

`sdk_result_cb_t` 是原生接入时统一的异步结果通道。

当前实现请这样理解：

- 回调第一个参数 `code` 是本次通知的粗粒度状态码
- `reqId`、`type` 以及大部分业务字段都在 JSON 体里
- 不要再把回调第一个参数当作稳定的 `reqId` 或 `eventId`

推荐的回调处理方式：

1. 解析 JSON body
2. 优先根据顶层 `type` 路由
3. 如需关联请求，使用顶层 `reqId`
4. `data.eventId` 视为可选字段，因为并不是所有异步通知都走标准 envelope

### 3.5 同步响应 Envelope

大多数由 BroSDK 自己生成的同步 HTTP 响应，结构如下：

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

顶层字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `code` | int | SDK 返回码 |
| `reqId` | int | SDK 请求 ID |
| `type` | string | 事件名称 |
| `msg` | string | 可读状态描述 |
| `data` | object | 接口专属数据 |

### 3.6 异步 HTTP ACK Envelope

异步 HTTP 接口返回的是“已受理 ACK”，不是最终结果：

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

注意事项：

- 当前即时 ACK 的 `reqId` 固定为 `0`
- 该 ACK 只表示请求已经进入 SDK 调度器
- 最终成功或失败，必须通过 WebSocket 获取

### 3.7 浏览器生命周期通知结构

浏览器打开 / 关闭相关通知使用专门的结构：

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

`data` 字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `envId` | string | 环境 ID |
| `status` | int | 当前浏览器生命周期状态 |
| `statusName` | string | 当前浏览器生命周期状态名称 |
| `progress` | int | 当前进度百分比 |
| `cdpReady` | bool | CDP 是否已可用于业务逻辑 |

`envList` 字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `envList` | array | 当前时刻所有活跃环境的快照 |
| `envList[].envId` | string | 环境 ID |
| `envList[].status` | int | 生命周期状态 |
| `envList[].statusName` | string | 生命周期状态名称 |
| `envList[].progress` | int | 进度 |
| `envList[].cdpReady` | bool | CDP 就绪状态 |

当前 `statusName` 可能出现的值：

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

### 3.8 同步 / 异步矩阵

| 能力 | C API | HTTP | 最终结果通道 |
| --- | --- | --- | --- |
| SDK 初始化 | 同步或异步 | 同步 | 当前调用 / 回调 / WS |
| SDK 信息 | 同步 | 同步 | 当前调用 |
| 浏览器信息 | 同步 | 同步 | 当前调用 |
| 浏览器打开 | 异步 | 异步 ACK | 回调 / WS |
| 浏览器关闭 | 异步 | 异步 ACK | 回调 / WS |
| Token 更新 | 异步 | 异步 ACK | 回调 / WS |
| 环境创建 | 同步后端透传 | 同步后端透传 | 当前调用 |
| 环境更新 | 同步后端透传 | 同步后端透传 | 当前调用 |
| 环境分页 | 同步后端透传 | 同步后端透传 | 当前调用 |
| 环境销毁 | 同步后端透传 | 同步后端透传 | 当前调用 |

## 4. 内嵌 Web API

### 4.1 启用方式

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

### 4.2 认证模型

内嵌本地 HTTP API 当前不依赖 `Authorization` Header。

当前模型如下：

- `/sdk/v1/init` 在 JSON body 中携带 `userSig`
- 后续本地 HTTP 请求使用当前已初始化的 SDK 实例
- `/sdk/v1/token/update` 在 JSON body 中刷新 `userSig`

### 4.3 WebSocket 使用说明

如果你使用的是 `browser/open`、`browser/close` 这类异步 HTTP 接口，必须同时建立 WebSocket 连接，才能拿到最终结果。

当前行为：

- 浏览器打开 / 关闭的最终通知会通过 WebSocket 广播
- 推送体与原生 C API 回调收到的 JSON 结构一致
- 如果只发 HTTP 请求、不连 WebSocket，那么你只能拿到即时 ACK

## 5. HTTP API 参考

所有公开接口均为 `POST`。

### 5.1 `POST /sdk/v1/init`

同步初始化 SDK。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 后端签发的用户令牌 |
| `workDir` | string | 否 | 工作目录根路径，实际运行目录会被解析为 `workDir/appId` |
| `port` | integer | 否 | 内嵌本地 HTTP / WS 端口 |
| `sdkApiUrl` | string | 否 | 覆盖 SDK 后端地址 |
| `debug` | bool | 否 | 开启开发者日志 |

请求示例：

```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIs...",
  "workDir": "C:/BroSDK",
  "port": 9527,
  "sdkApiUrl": "https://api.example.com",
  "debug": true
}
```

成功响应：

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

响应 `data` 字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `workDir` | string | 实际生效的运行目录 |
| `port` | integer | 启用了内嵌本地 HTTP / WS 时返回 |
| `eventId` | int | 成功为 `10111`，失败为 `10112` |

当前实现的重要限制：

- `sdk_init` 在同一进程内是全局串行的
- 如果另一个初始化已经在途，后续初始化会被直接拒绝为 `CL_WBUSY`
- 后端初始化成功后，SDK 还会执行同 `appId` 的机器级实例锁检查

### 5.2 `POST /sdk/v1/info`

同步获取 SDK 运行信息。

请求体：

- 推荐传空对象 `{}`
- 空 body 也可以

成功响应：

```json
{
  "code": 0,
  "reqId": 123456789,
  "type": "sdk-info-success",
  "msg": "ok",
  "data": {
    "info": {
      "deviceId": "device_xxx",
      "version": "1.0.0.5",
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

`data.info` 字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `deviceId` | string | 当前机器指纹 |
| `version` | string | SDK 版本号 |
| `startupTime` | integer | SDK 启动时间戳 |
| `coresInfo` | object | 当前已加载浏览器核心信息 |
| `netInfo` | object | 当前网络环境快照 |
| `workDir` | string | 实际运行目录 |
| `tokenExpiresInS` | integer | 当前 token 剩余有效秒数 |
| `dataFullyManaged` | bool | `true` 为全托管，`false` 为半托管 |

### 5.3 `POST /sdk/v1/browser/info`

同步获取当前运行中的浏览器列表。

请求体：

- 当前版本 SDK 层始终返回全部运行中环境
- SDK 层暂未实现请求体过滤
- 建议传 `{}` 或空 body

成功响应：

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

`data.envs[]` 字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `envId` | string | 环境 ID |
| `remoteDebuggingPort` | integer | 该实例的远程调试端口 |

### 5.4 `POST /sdk/v1/browser/open`

异步打开一个或多个浏览器环境。

顶层请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要打开的环境列表 |

`envs[]` 支持三种写法：

- 直接传字符串环境 ID，例如 `"2041695386304778240"`
- 直接传数字环境 ID
- 传对象，对象中包含 `envId` 和可选的启动覆盖参数

`envs[]` 对象字段说明：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |
| `urls` | array<string> | 否 | 浏览器启动后自动打开的 URL |
| `args` | array<string> | 否 | 额外追加的 Chromium 启动参数 |
| `cookies` | array<object> | 否 | 本次启动要注入的 Cookie JSON 数组 |
| `extensions` | array<object> | 否 | 本次启动要加载的扩展及其透传数据 |
| `proxy` | string | 否 | 上游代理 URL |
| `bridgeProxy` | string | 否 | 代理桥覆盖配置 |
| `envName` | string | 否 | 环境显示名覆盖 |
| `shortName` | string | 否 | 短名称覆盖 |
| `serial` | string | 否 | 业务序列号覆盖 |
| `searchEngine` | object | 否 | 搜索引擎覆盖配置 |
| `enableDevtools` | integer | 否 | DevTools 策略覆盖 |
| `enableStorage` | integer | 否 | Storage 策略覆盖 |
| `finger` | object | 否 | 浏览器指纹对象 |

`extensions[]` 字段说明：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `id` | string | 是 | 扩展 ID |
| `name` | string | 是 | 扩展目录名或扩展包名 |
| `packType` | string | 否 | `unpack`、`crx` 或 `zip` |
| `component` | bool | 否 | 是否作为 component extension 加载 |
| `data` | object<string,string> | 否 | 透传到扩展本地存储的字符串键值对 |

`searchEngine` 字段说明：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `site` | string | 否 | 搜索引擎站点编码 |
| `siteDesc` | string | 否 | 搜索引擎名称 |
| `url` | string | 否 | 搜索 URL 前缀 |

请求示例：

```json
{
  "envs": [
    {
      "envId": "2041695386304778240",
      "urls": ["https://example.com"],
      "args": ["--no-first-run", "--remote-allow-origins=*"],
      "extensions": [
        {
          "id": "ebglcogbaklfalmoeccdjbmgfcacengf",
          "name": "testExt1",
          "packType": "unpack",
          "component": false,
          "data": {
            "key1": "aGVsbG8=",
            "key2": "value2"
          }
        }
      ]
    }
  ]
}
```

即时 HTTP ACK：

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

最终异步事件通过 WebSocket 返回：

- `browser-open`
- `browser-open-success`
- `browser-open-failed`

`browser-open-success` 示例：

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

当前实现的关键语义：

> `browser-open-success` 等价于浏览器已经完全启动，并且 CDP 已经就绪。  
> 后续依赖 Cookie / Storage / 扩展 / 自动化逻辑的业务，应以这条通知作为真正的 ready 信号。

### 5.5 `POST /sdk/v1/browser/close`

异步关闭一个或多个浏览器环境。

顶层请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要关闭的环境列表 |

`envs[]` 支持：

- 字符串环境 ID
- 数字环境 ID
- 包含 `envId` 的对象

请求示例：

```json
{
  "envs": ["2041695386304778240"]
}
```

即时 HTTP ACK：

```json
{
  "code": 0,
  "reqId": 0,
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

最终异步事件通过 WebSocket 返回：

- `browser-close-success`
- `browser-close-failed`

`browser-close-success` 示例：

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

当前实现的关键语义：

> 即时 ACK 只表示关闭任务已被受理。  
> 只有收到 `browser-close-success`，才表示环境真的关闭完成。

### 5.6 `POST /sdk/v1/token/update`

异步刷新 `userSig`。

请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `userSig` | string | 是 | 新的用户令牌 |

请求示例：

```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIs..."
}
```

即时 HTTP ACK 示例：

```json
{
  "code": 0,
  "reqId": 0,
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

调用建议：

- 当收到 token 即将过期的 SDK 事件后，及时调用本接口
- 不要把 ACK 当成最终成功
- 最终结果以回调 / WebSocket 推送为准

### 5.7 `POST /sdk/v1/env/create`

同步创建环境。

当前 SDK 行为：

- 请求体直接转发到后端 `env/create`
- 响应体直接返回后端原始 JSON
- BroSDK 不会为该接口再套一层自己的 envelope

当前环境模型里常见的请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envName` | string | 否 | 环境名称 |
| `shortName` | string | 否 | 短名称 |
| `serial` | string | 否 | 业务序列号 |
| `proxy` | string | 否 | 上游代理 URL |
| `bridgeProxy` | string | 否 | 代理桥覆盖配置 |
| `finger` | object | 否 | 浏览器完整指纹对象 |
| `searchEngine` | object | 否 | 搜索引擎配置 |
| `enableDevtools` | integer | 否 | DevTools 策略 |
| `enableStorage` | integer | 否 | Storage 策略 |
| `extensions` | array | 否 | 扩展定义 |

返回参数：

- 完全以后端原始响应为准
- 不会追加 BroSDK 的 `reqId` / `type` / `eventId`

### 5.8 `POST /sdk/v1/env/update`

同步更新环境。

当前 SDK 行为：

- 请求体直接转发到后端 `env/update`
- 响应体直接返回后端原始 JSON

常见请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |
| `envName` | string | 否 | 环境名称 |
| `shortName` | string | 否 | 短名称 |
| `serial` | string | 否 | 业务序列号 |
| `proxy` | string | 否 | 上游代理 URL |
| `bridgeProxy` | string | 否 | 代理桥覆盖配置 |
| `finger` | object | 否 | 浏览器完整指纹对象 |
| `searchEngine` | object | 否 | 搜索引擎配置 |
| `enableDevtools` | integer | 否 | DevTools 策略 |
| `enableStorage` | integer | 否 | Storage 策略 |
| `extensions` | array | 否 | 扩展定义 |

返回参数：

- 完全以后端原始响应为准
- 不追加 BroSDK envelope

### 5.9 `POST /sdk/v1/env/page`

同步分页查询环境。

当前 SDK 行为：

- 请求体直接转发到后端 `env/page`
- 响应体直接返回后端原始 JSON

常见请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `customerId` | string | 否 | 三方业务用户 ID |
| `envIds` | array | 否 | 环境筛选列表 |
| `pageIndex` | integer | 否 | 页码 |
| `pageSize` | integer | 否 | 每页数量 |

返回参数：

- 完全以后端原始响应为准
- 分页字段由后端定义

### 5.10 `POST /sdk/v1/env/destroy`

同步销毁环境。

当前 SDK 行为：

- 请求体直接转发到后端 `env/destroy`
- 响应体直接返回后端原始 JSON

常见请求字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID |

返回参数：

- 完全以后端原始响应为准
- 不追加 BroSDK envelope

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
| `sdk_init_async` | 异步 | 受理后返回 `CL_DONE` |
| `sdk_init_webapi` | 同步辅助函数 | 兼容入口，新接入建议直接使用带 `"port"` 的 `sdk_init` |
| `sdk_info` | 同步 | 返回原始 info JSON |
| `sdk_browser_info` | 同步 | 返回原始浏览器列表 JSON |
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

### 6.5 内存与辅助接口

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

返回码分类辅助函数。

## 7. 数据托管语义

可以通过 `sdk_info().dataFullyManaged` 判断当前模式：

| 值 | 含义 |
| --- | --- |
| `false` | 半托管模式，本地 SQLite 为主 |
| `true` | 全托管模式，本地 SQLite 为一级缓存，OSS 在后台异步同步 |

当前实现：

- 浏览器关闭时，先把最新 Cookie / Storage 快照写入本地 SQLite
- 全托管模式下，再在后台异步上传到 OSS
- 浏览器打开时，会优先使用仍然有效的本地 SQLite 元数据
- `browser-close-success` 表示本地持久化已经完成，不代表 OSS 上传已经完成

## 8. 集成时必须注意的规则

- 先调用 `sdk_register_result_cb()`，再进入异步业务流程
- 把 `sdk_init` 当成进程内全局串行入口
- 把 `sdk_browser_open` 和 `sdk_browser_close` 当成异步任务受理接口
- 浏览器可用的真正信号是 `browser-open-success`
- 浏览器真正关闭完成的信号是 `browser-close-success`
- 对于异步 HTTP 路由，必须同时建立 WebSocket
- 不要假设 `env/*` 接口和 `init/info` 的回包包装方式一致，前者是后端原始 JSON 透传

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

常用错误 / 警告码：

| 代码 | 名称 | 说明 |
| --- | --- | --- |
| `0` | `CL_OK` | 成功 |
| `1` | `CL_DONE` | 异步任务已受理 |
| `104` | `CL_WBUSY` | 资源忙 |
| `-3002` | `CL_ETIMEOUT` | 超时 |
| `-3003` | `CL_EINVALID` | 参数错误 |
| `-3005` | `CL_EALREADY` | 已存在 |
| `-3012` | `CL_ENOTINITIALIZED` | SDK 未初始化 |
| `-3019` | `CL_EPORT_UNAVAILABLE` | 端口非法或已占用 |
| `-3502` | `CL_EHTTP_POST` | 后端 HTTP 失败 |
| `-3509` | `CL_ETOKEN_INVALID` | Token 无效 |
| `-3511` | `CL_EWORKDIR_INVALID` | 工作目录无效 |
| `-4000` 及以下 | `CL_ESDKAPI` 系列 | 后端 SDK API 错误 |

---

本文档刻意区分了“BroSDK 自己生成的返回结构”和“直接透传后端的 env CRUD 返回结构”。  
如果你的接入同时依赖完整的后端环境模型，请以后端 env API 契约作为完整字段集合的最终依据。
