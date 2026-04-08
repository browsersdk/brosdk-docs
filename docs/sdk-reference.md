# BroSDK SDK 参考

BroSDK 是一个 **C++ 编写的动态链接库**，用于调用浏览器内核。本文档提供 SDK 的完整参考信息。

---

## SDK 下载

### 官方仓库

| 组件 | 说明 | 下载地址 |
| --- | --- | --- |
| **SDK** | C++ 动态链接库，核心调用接口 | [brosdk-sdk](https://github.com/browsersdk/brosdk-sdk/releases) |
| **SDK Demo** | 完整的使用示例 | [browser-sdk-demo](https://github.com/browsersdk/browser-sdk-demo) |
| **Rust SDK & Demo** | Rust 语言封装 | [brosdk-sdk-rust](https://github.com/browsersdk/brosdk-sdk-rust) |
| **TypeScript SDK** | TypeScript/Node.js 封装 | [brosdk-sdk-typescript](https://github.com/browsersdk/brosdk-sdk-typescript) |
| **TypeScript Demo** | TypeScript/Node.js SDK 封装 | [brosdk-sdk-typescript-demo](https://github.com/browsersdk/brosdk-sdk-demo) |

### 平台支持

| 平台 | 架构 | 库文件 | 状态 |
| --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | ✅ 已发布 |
| Linux | x64 | `brosdk.so` | 🚧 计划中 |
| macOS | arm64/x64 | `brosdk.dylib` | 🚧 测试中 |

---

## 调用方式

SDK 提供两种等价的调用方式：

### 方式一：C API（推荐）

直接加载动态库并调用导出的 C 函数：

```cpp
#include "brosdk.h"

// 1. 初始化 SDK
sdk_handle_t handle;
const char *init_req = R"({
  "userSig": "eyJhbGciOiJSUzI1NiIs...",
  "workDir": "C:/brosdk/data",
  "port": 9527
})";

int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);

// 2. 创建环境
const char *create_req = R"({
  "envName": "myenv",
  "customerId": "user123",
  "finger": {"system": "Windows 11", "kernel": "Chrome"}
})";

rc = sdk_env_create(handle, create_req, strlen(create_req), &out, &out_len);

// 3. 使用完毕后释放内存
sdk_free(out);
```

### 方式二：HTTP API

通过 SDK 内置的 HTTP 服务访问 REST 端点：

```bash
curl -X POST http://localhost:9527/sdk/v1/env/create \
  -H "Authorization: Bearer YOUR_USER_SIGN" \
  -H "Content-Type: application/json" \
  -d '{"envName":"myenv","customerId":"user123"}'
```

**基础 URL**：`http://localhost:9527`（默认端口，可在初始化时配置）

**Content-Type**：`application/json`

> 两种方式最终都会汇聚到同一个核心分发器，**功能和行为完全相同**。C API 性能更好，HTTP API 更适合脚本语言集成。

---

## HTTP API 端点

### 响应格式

```json
{
  "reqid": 1006901416,
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2034183257439866880"
  }
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| reqid | int64 | 请求 ID（异步请求时） |
| code | int | 状态码（0 = 成功，负数 = 错误） |
| msg | string | 响应消息 |
| data | object | 响应数据 |

### 初始化 SDK

**端点**：`POST /sdk/v1/init`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | 从服务端获取的 JWT 令牌 |
| workDir | string | 是 | 工作目录，存储环境数据 |
| port | integer | 否 | HTTP 服务端口（默认 9527） |

**请求示例**：

```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIs...",
  "workDir": "C:/brosdk/data",
  "port": 9527
}
```

### 创建环境

**端点**：`POST /sdk/v1/env/create`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envName | string | 否 | 环境名称 |
| customerId | string | 否 | 三方用户唯一 ID |
| proxy | string | 否 | 代理地址（格式：`socks5://user:pass@proxy:1080`） |
| finger | object | 否 | 浏览器指纹配置 |
| finger.system | string | 否 | 操作系统（如：Windows 11） |
| finger.kernel | string | 否 | 浏览器内核（如：Chrome） |
| finger.kernelVersion | string | 否 | 内核版本（如：148） |

**请求示例**：

```bash
curl -X POST http://localhost:9527/sdk/v1/env/create \
  -H "Authorization: Bearer YOUR_USER_SIGN" \
  -H "Content-Type: application/json" \
  -d '{
    "envName": "我的浏览器",
    "customerId": "user_12345",
    "proxy": "socks5://user:pass@proxy:1080",
    "finger": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148"
    }
  }'
```

**响应示例**：

```json
{
  "reqid": 1006901416,
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2034183257439866880"
  }
}
```

### 更新环境

**端点**：`POST /sdk/v1/env/update`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |
| envName | string | 否 | 环境名称 |
| proxy | string | 否 | 代理地址 |
| finger | object | 否 | 浏览器指纹配置 |

### 查询环境列表

**端点**：`POST /sdk/v1/env/page`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| customerId | string | 否 | 三方用户 ID |
| page | int | 否 | 页码（默认 1） |
| pageSize | int | 否 | 每页数量（默认 20） |

**响应示例**：

```json
{
  "reqid": 1006901418,
  "code": 0,
  "msg": "ok",
  "data": {
    "total": 100,
    "page": 1,
    "pageSize": 20,
    "list": [
      {
        "envId": "2034183257439866880",
        "envName": "我的浏览器",
        "customerId": "user_12345",
        "status": 1,
        "createTime": 1710000000000
      }
    ]
  }
}
```

### 销毁环境

**端点**：`POST /sdk/v1/env/destroy`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |

### 打开浏览器

**端点**：`POST /sdk/v1/browser/open`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |
| url | string | 否 | 要打开的 URL |

### 关闭浏览器

**端点**：`POST /sdk/v1/browser/close`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |

### 获取浏览器运行状态

**端点**：`POST /sdk/v1/browser/info`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envIds | array | 否 | 环境 ID 列表，传入则只查询指定环境；不传则返回所有运行中的浏览器 |

**请求示例**：

```bash
# 查询所有运行中的浏览器
curl -X POST http://localhost:9527/sdk/v1/browser/info \
  -H "Authorization: Bearer YOUR_USER_SIGN"

# 查询指定环境
curl -X POST http://localhost:9527/sdk/v1/browser/info \
  -H "Authorization: Bearer YOUR_USER_SIGN" \
  -H "Content-Type: application/json" \
  -d '{"envIds": ["2039469749536034816"]}'
```

**响应示例**：

```json
{
  "reqid": 1191362648,
  "code": 0,
  "msg": "ok",
  "data": {
    "envs": [
      {
        "envId": "2039469749536034816",
        "remoteDebuggingPort": 65534
      }
    ]
  }
}
```

**响应字段说明**：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| envs | array | 运行中的浏览器实例列表 |
| envs[].envId | string | 环境 ID |
| envs[].remoteDebuggingPort | int | 该实例的 `--remote-debugging-port` 端口号；未启用调试端口时为 `0` |

### 更新 User Sign

**端点**：`POST /sdk/v1/token/update`

**认证**：需要（User Sign）

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | 新的 User Sign |

---

## C API 参考

### 头文件

```c
#include "brosdk.h"
```

依赖以下 C11 标准头文件：
```c
#include <stddef.h>   /* size_t */
#include <stdint.h>   /* int32_t, uint16_t, int64_t */
#include <stdbool.h>  /* bool */
#include <stdio.h>
#include <stdlib.h>
```

### 类型定义

#### sdk_handle_t

不透明的 SDK 实例句柄。

```c
typedef void *sdk_handle_t;
```

#### sdk_result_cb_t

异步结果回调函数类型。

```c
typedef void(SDK_CALL *sdk_result_cb_t)(
    int32_t     code,       /* 状态码或请求 ID */
    void       *user_data,  /* 用户数据指针 */
    const char *data,       /* 通知数据（UTF-8 JSON） */
    size_t      len         /* 数据字节长度 */
);
```

#### sdk_cookies_storage_cb_t

Cookie/Storage 存储拦截回调函数类型。

```c
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(
    const char *data,       /* [IN]  SDK 提取的 Cookie JSON 数据 */
    size_t      len,        /* [IN]  data 字节长度               */
    char      **new_data,   /* [OUT] 替换数据指针；NULL 表示透传 */
    size_t     *new_len,    /* [OUT] 替换数据字节长度            */
    void       *user_data   /* [IN]  注册时传入的用户指针        */
);
```

### 核心函数

#### sdk_register_result_cb - 注册异步结果回调

```c
SDK_API int32_t SDK_CALL sdk_register_result_cb(
    sdk_result_cb_t  cb,        /* [IN] 回调函数指针，不得为 NULL */
    void            *user_data  /* [IN] 用户数据指针（每次回调原样返回）*/
);
```

**说明**：注册全局异步通知回调。所有 SDK 异步事件（初始化完成、浏览器打开/关闭、令牌刷新、数据同步等）均通过此回调通知。**必须在调用任何其他 SDK 接口之前注册**。

**参数**：
- `cb`：回调函数指针，不得为 `NULL`
- `user_data`：用户数据指针，原样传回回调，可为 `NULL`

**返回值**：
- `0`：注册成功
- `< 0`：注册失败

**示例**：
```c
static void on_result(int32_t code, void *user_data,
                      const char *data, size_t len) {
    if (sdk_is_reqid(code)) {
        printf("[ReqID=%d] %.*s\n", code, (int)len, data);
    } else if (sdk_is_event(code)) {
        printf("[Event] %s: %.*s\n", sdk_event_name(code), (int)len, data);
    } else if (sdk_is_error(code)) {
        fprintf(stderr, "[Error] %s: %s\n",
                sdk_error_name(code), sdk_error_string(code));
    }
}

sdk_register_result_cb(on_result, NULL);
```

---

#### sdk_register_cookies_storage_cb - 注册 Cookie 存储拦截回调

```c
SDK_API int32_t SDK_CALL sdk_register_cookies_storage_cb(
    sdk_cookies_storage_cb_t  cb,        /* [IN] 拦截回调函数指针 */
    void                     *user_data  /* [IN] 用户数据指针 */
);
```

**说明**：注册 Cookie 存储拦截回调。当浏览器关闭后 SDK 完成 Cookie 提取时，会通过此回调将 Cookie JSON 数据传递给调用方。调用方可选择：
- **透传**：将 `*new_data` 置 `NULL`，SDK 使用原始数据持久化
- **替换**：通过 `sdk_malloc()` 分配替换数据写入 `*new_data` / `*new_len`，SDK 将使用替换后数据

**参数**：
- `cb`：拦截回调函数指针
- `user_data`：用户数据指针，可为 `NULL`

**返回值**：
- `0`：注册成功
- `< 0`：注册失败

**回调实现示例**：
```c
/* 示例一：透传（不修改） */
static void on_cookies_passthrough(const char *data, size_t len,
                                    char **new_data, size_t *new_len,
                                    void *user_data) {
    *new_data = NULL;
    *new_len  = 0;
}

/* 示例二：对 Cookie 数据进行二次加工 */
static void on_cookies_transform(const char *data, size_t len,
                                  char **new_data, size_t *new_len,
                                  void *user_data) {
    /* data 是 JSON 数组格式的 Cookie 数据 */
    size_t out_size = 0;
    char  *processed = my_process_cookies(data, len, &out_size);

    /* 替换数据必须使用 sdk_malloc 分配 */
    *new_data = (char *)sdk_malloc(out_size);
    memcpy(*new_data, processed, out_size);
    *new_len = out_size;
    free(processed);
}

sdk_register_cookies_storage_cb(on_cookies_transform, NULL);
```

> **内存规则**：替换数据 `*new_data` **必须**通过 `sdk_malloc()` 分配，SDK 将自动调用 `sdk_free()` 释放。

---

#### sdk_init - 初始化 SDK

```c
int32_t sdk_init(
    sdk_handle_t *handle,
    const char   *init_req,
    size_t        init_req_len,
    char        **out_data,
    size_t       *out_len
);
```

**参数**：
- `handle`：输出参数，SDK 实例句柄
- `init_req`：JSON 格式的初始化请求
- `init_req_len`：请求字节长度
- `out_data`：输出参数，响应数据（需调用 `sdk_free()` 释放）
- `out_len`：输出参数，响应数据长度

**初始化请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | JWT 用户令牌，用于后端鉴权 |
| workDir | string | 是 | SDK 工作目录（缓存、日志、内核、本地数据库存放位置） |
| port | int | 否 | 内嵌 Web API 监听端口；0 或省略则不启动 HTTP 服务 |
| sdkApiUrl | string | 否 | 开发者内部调试使用 |

**初始化请求示例**：
```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...",
  "workDir": "C:/brosdk/data",
  "port": 9527
}
```

**响应示例（成功）**：
```json
{
    "code": 0,
    "reqId": 1309318677,
    "type": "sdk-init-success",
    "msg": "ok",
    "data": {
        "workDir": "C:/Users/Administrator/AppData/Local/BroSDK2",
        "eventId": 10111
    }
}
```

**响应示例（失败）**：
```json
{
    "reqId": 856525336,
    "type": "sdk-init-failed",
    "data": {
        "code": -4000,
        "msg": "401:token has invalid claims: token is expired",
        "envId": "0",
        "eventId": 10112
    }
}
```

**返回值**：
- `0`：成功
- `< 0`：失败（调用 `sdk_error_string(code)` 获取详情）

---

#### sdk_info - 获取 SDK 运行时信息

```c
SDK_API int32_t SDK_CALL sdk_info(
    char  **out_data,  /* [OUT] 响应 JSON（需 sdk_free 释放）*/
    size_t *out_len    /* [OUT] 响应数据字节长度              */
);
```

**说明**：获取 SDK 运行时状态快照。

**响应 JSON 字段**：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| deviceId | string | 设备指纹 |
| version | string | SDK 版本号 |
| startupTime | int64 | 启动时间戳（Unix 秒） |
| coresInfo | object | 已加载内核信息 |
| netInfo | object | 本机网络信息 |
| workDir | string | 工作目录 |
| tokenExpiresInS | int64 | 令牌剩余有效期（秒）；0 表示未初始化 |
| dataFullyManaged | bool | 数据托管模式：true = 全托管（云端），false = 自托管（本地） |

---

#### sdk_browser_info - 获取浏览器运行时信息

```c
SDK_API int32_t SDK_CALL sdk_browser_info(
    char  **out_data,  /* [OUT] 响应 JSON（需 sdk_free 释放）*/
    size_t *out_len    /* [OUT] 响应数据字节长度              */
);
```

**说明**：同步获取当前运行中的浏览器实例状态，可查询所有或指定环境的浏览器进程信息，包括调试端口。

**对应 Web API**：`POST /sdk/v1/browser/info`

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envIds | array | 否 | 环境 ID 列表，传入则只查询指定环境；不传则返回所有运行中的浏览器 |

**请求示例**：
```json
{
  "envIds": ["2039469749536034816"]
}
```

**响应示例**：
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

**响应字段说明**：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| envs | array | 运行中的浏览器实例列表 |
| envs[].envId | string | 环境 ID |
| envs[].remoteDebuggingPort | int | 该实例的 `--remote-debugging-port` 端口号；未启用调试端口时为 `0` |

**返回值**：`0` 成功 / `< 0` 失败。使用完毕后调用 `sdk_free(*out_data)`。

---

#### sdk_env_create - 创建环境

```c
int32_t sdk_env_create(
    sdk_handle_t  handle,
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**请求示例**：
```json
{
  "envName": "myenv",
  "customerId": "user123",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148"
  }
}
```

---

#### sdk_env_destroy - 销毁环境

```c
int32_t sdk_env_destroy(
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**请求示例**：
```json
{
  "envId": "2034183257439866880"
}
```

---

#### sdk_env_page - 查询环境列表

```c
int32_t sdk_env_page(
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**请求示例**：
```json
{
  "customerId": "user123",
  "page": 1,
  "pageSize": 20
}
```

---

#### sdk_browser_open - 打开浏览器

```c
int32_t sdk_browser_open(
    sdk_handle_t  handle,
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**说明**：异步打开一个或多个浏览器实例。每个 `envId` 对应独立浏览器进程与独立用户数据目录。打开时 SDK 自动从持久层恢复该环境的 Cookie 与 Storage 数据。操作结果通过回调通知。

**对应 Web API**：`POST /sdk/v1/browser/open`

**请求 JSON 字段**：

| 字段 | 二级字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- | --- |
| `envs` | | array | 是 | 要打开的环境列表（支持批量） |
| | `envId` | string | 是 | 环境 ID |
| | `args` | array | 否 | 追加的 Chromium 启动参数 |
| | `urls` | array | 否 | 启动后自动打开的 URL |
| | `cookies` | array | 否 | 设置当前环境的 cookies |
| | `extensions` | array | 否 | 设置当前环境加载的扩展和透传数据 |

**扩展投放方式**：

在 workDir 目录，SDK 初始化一个 `extensions` 文件夹。用户将解包扩展加入该文件夹，在启动时指定。

```json
{
    "name": "testExt1",  // 对应 ${workDir}/extensions/testExt1 目录
    "id": "ebglcogbaklfalmoeccdjbmgfcacengf",  // 扩展 ID，用户必须预知
    "packType": "unpack",  // 保持默认
    "component": false,  // 保持默认
    "data": {
        "key1": "aGVsbG8=",  // 透传数据键值对
        "key3": "5L2g5aW9",  // 透传数据键值对
        "key2": "12345234634574568478asdfdgsdfg"  // 透传数据键值对
    }
}
```

扩展内读取透传数据：
```javascript
const data1 = await new Promise((resolve) =>
  chrome.storage.local.get("key1", (r) => resolve(r))
);
const data2 = await new Promise((resolve) =>
  chrome.storage.local.get("key2", (r) => resolve(r))
);
const data3 = await new Promise((resolve) =>
  chrome.storage.local.get("key3", (r) => resolve(r))
);
```

**请求示例**：
```json
{
    "envs": [
        {
            "envId": "2037495132382564352",
            "args": [
                "--no-first-run",
                "--no-default-browser-check",
                "--disable-web-security",
                "--remote-allow-origins=*"
            ],
            "urls": [
                "https://baidu.com",
                "https://bing.com"
            ],
            "extensions": [
                {
                    "name": "testExt1",
                    "id": "ebglcogbaklfalmoeccdjbmgfcacengf",
                    "packType": "unpack",
                    "component": false,
                    "data": {
                        "key1": "aGVsbG8="
                    }
                }
            ],
            "cookies": [
                {
                    "domain": ".baidu.com",
                    "expirationDate": 1808188306.943084,
                    "hostOnly": false,
                    "httpOnly": true,
                    "name": "BDUSS_BFESS",
                    "path": "/",
                    "sameSite": "no_restriction",
                    "secure": true,
                    "session": false,
                    "storeId": null,
                    "value": "Xk2TVJmbGZ1WU1xOU9yVjJpMW9LZX..."
                }
            ]
        }
    ]
}
```

**返回值**：
- `1`（DONE）：任务已受理
- `< 0`：参数错误或 SDK 未初始化

**回调通知体**（成功时 `eventId` = `20111`）：
```json
{
    "code": 0,
    "reqId": 1963399787,
    "type": "browser-open-success",
    "msg": "ok",
    "data": {
        "envId": "2034073783806988288",
        "percent": 100,
        "eventId": 20111
    }
}
```

> **精度提醒**：`envId` 为 64 位整数，在 JavaScript 等语言中建议以字符串传递以避免精度丢失。

---

#### sdk_browser_close - 关闭浏览器

```c
int32_t sdk_browser_close(
    sdk_handle_t  handle,
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**说明**：异步关闭指定环境的浏览器进程。关闭期间 SDK 自动执行 Cookie/Storage 数据的收集、压缩与持久化。操作结果通过回调通知。

**对应 Web API**：`POST /sdk/v1/browser/close`

**请求 JSON 字段**：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envs` | array | 是 | 要关闭的环境 ID 列表 |

**请求示例**：
```json
{
  "envs": ["2028432501503954944"]
}
```

**返回值**：`1`（DONE）任务已受理 / `< 0` 参数错误。

**回调通知体**（成功时 `eventId` = `20141`）：
```json
{
    "code": 0,
    "reqId": 1577975507,
    "type": "browser-close-success",
    "msg": "ok",
    "data": {
        "envId": "2034073783806988288",
        "eventId": 20141
    }
}
```

---

#### sdk_env_create - 创建环境

```c
int32_t sdk_env_create(
    sdk_handle_t  handle,
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**说明**：同步创建浏览器环境，请求透传至后端服务。成功后返回新分配的 `envId`。

**对应 Web API**：`POST /sdk/v1/env/create`

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envName | string | 否 | 环境名称 |
| customerId | string | 否 | 三方用户唯一 ID |
| proxy | string | 否 | 代理地址（格式：`socks5://user:pass@proxy:1080`） |
| finger | object | 否 | 浏览器指纹配置 |
| finger.system | string | 否 | 操作系统（如：Windows 11） |
| finger.kernel | string | 否 | 浏览器内核（如：Chrome） |
| finger.kernelVersion | string | 否 | 内核版本（如：148） |

**请求示例**：
```json
{
  "envName": "我的浏览器",
  "customerId": "user_12345",
  "proxy": "socks5://user:pass@proxy:1080",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148"
  }
}
```

**响应示例**：
```json
{
  "reqid": 1006901416,
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2034183257439866880"
  }
}
```

**返回值**：`0` 成功 / `< 0` 失败。使用完毕后调用 `sdk_free(*out_data)`。

---

#### sdk_env_update - 更新环境

```c
int32_t sdk_env_update(
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**说明**：同步更新现有浏览器环境配置，请求体透传至后端服务。

**对应 Web API**：`POST /sdk/v1/env/update`

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |
| envName | string | 否 | 环境名称 |
| proxy | string | 否 | 代理地址 |
| finger | object | 否 | 浏览器指纹配置 |

**请求示例**：
```json
{
  "envId": "2034183257439866880",
  "envName": "更新后的名称",
  "proxy": "socks5://newproxy:1080"
}
```

**返回值**：`0` 成功 / `< 0` 失败。使用完毕后调用 `sdk_free(*out_data)`。

---

#### sdk_env_page - 查询环境列表（分页）

```c
int32_t sdk_env_page(
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**说明**：同步查询环境列表，支持分页，请求体透传至后端服务。

**对应 Web API**：`POST /sdk/v1/env/page`

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| customerId | string | 否 | 三方用户 ID |
| pageIndex | int | 否 | 页码（默认 0） |
| pageSize | int | 否 | 每页数量（默认 20） |
| envIds | array | 否 | 环境 ID 列表筛选 |

**请求示例**：
```json
{
  "customerId": "",
  "envIds": [],
  "pageIndex": 0,
  "pageSize": 20
}
```

**返回值**：`0` 成功 / `< 0` 失败。使用完毕后调用 `sdk_free(*out_data)`。

---

#### sdk_env_destroy - 销毁环境

```c
int32_t sdk_env_destroy(
    const char   *req,
    size_t        req_len,
    char        **out_data,
    size_t       *out_len
);
```

**说明**：同步删除指定浏览器环境及其关联的持久化数据（Cookie / Storage）。销毁后该 `envId` 不再可用。

**对应 Web API**：`POST /sdk/v1/env/destroy`

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |

**请求示例**：
```json
{
  "envId": "2034183257439866880"
}
```

**返回值**：`0` 成功 / `< 0` 失败。使用完毕后调用 `sdk_free(*out_data)`。

---

#### sdk_token_update - 刷新用户令牌

```c
int32_t sdk_token_update(
    const char   *req,
    size_t        req_len
);
```

**说明**：在 JWT 令牌过期前异步刷新。SDK 会在令牌即将到期时通过事件 `sdk-token-expire-warning`（`10123`）提醒调用方，收到此事件后应立即调用本接口更新令牌。

**对应 Web API**：`POST /sdk/v1/token/update`

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | 新的 JWT 用户令牌 |

**请求示例**：
```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**返回值**：`1`（DONE）任务已受理 / `< 0` 参数错误。

---

#### sdk_shutdown - 停止 SDK

```c
int32_t sdk_shutdown(void);
```

**说明**：同步停止 SDK：等待所有工作线程退出、关闭内嵌 HTTP 服务、释放所有资源。调用后如需再次使用 SDK，需重新调用 `sdk_init`。

**对应 Web API**：`POST /sdk/v1/shutdown`

**返回值**：`0` 成功 / `< 0` 失败。

---

### 内存与辅助函数

#### sdk_malloc / sdk_free - 堆内存管理

```c
SDK_API void *SDK_CALL sdk_malloc(size_t size);
SDK_API void  SDK_CALL sdk_free(void *ptr);
```

与 SDK 共享同一 C 运行时堆。所有 SDK 分配的输出缓冲区（`out_data`），均**必须**使用 `sdk_free()` 释放，不得使用宿主程序的 `free()`。

---

#### sdk_error_name / sdk_error_string - 错误码描述

```c
SDK_API const char *SDK_CALL sdk_error_name(int32_t code);
SDK_API const char *SDK_CALL sdk_error_string(int32_t code);
```

| 函数 | 返回值 | 示例（code = -3003） |
| --- | --- | --- |
| `sdk_error_name` | 短名称 | `"EINVALID"` |
| `sdk_error_string` | 可读描述 | `"invalid argument"` |

> 返回值指向 SDK 内部静态字符串，**不得** `free()`。

---

#### sdk_event_name - 事件 ID 名称

```c
SDK_API const char *SDK_CALL sdk_event_name(int32_t evtid);
```

将事件 ID（如 `20111`）映射到可读名称（如 `"browser-open-success"`）。用于日志与事件路由。

---

### 状态判断函数

```c
SDK_API bool SDK_CALL sdk_is_ok(int32_t code);       /* code == 0                    */
SDK_API bool SDK_CALL sdk_is_done(int32_t code);     /* code == 1                    */
SDK_API bool SDK_CALL sdk_is_warn(int32_t code);      /* 100 <= code <= 255           */
SDK_API bool SDK_CALL sdk_is_event(int32_t code);    /* 10000 <= code <= 100000      */
SDK_API bool SDK_CALL sdk_is_reqid(int32_t code);     /* code > 100000                */
SDK_API bool SDK_CALL sdk_is_error(int32_t code);     /* code < 0                     */
```

**推荐在回调中按以下顺序判断**：

```c
if      (sdk_is_reqid(code))  { /* 异步请求响应 */ }
else if (sdk_is_event(code))  { /* 事件通知     */ }
else if (sdk_is_done(code))   { /* 任务已受理   */ }
else if (sdk_is_ok(code))     { /* 操作成功     */ }
else if (sdk_is_warn(code))   { /* 警告         */ }
else if (sdk_is_error(code))  { /* 错误         */ }
```

---

### C++ 接口 · ISDK

`brosdk.h` 在 `#ifdef __cplusplus` 保护下定义了纯虚接口类，适合 C++ 调用方：

```cpp
class ISDK {
public:
  virtual ~ISDK() = default;
  virtual int32_t Info(char **, size_t *) const = 0;
  virtual int32_t UpdateToken(const char *data, size_t len) const = 0;
  virtual int32_t CreateEnv(const char *data, size_t len) const = 0;
  virtual int32_t DestroyEnv(const char *data, size_t len) const = 0;
  virtual int32_t PageEnv(const char *data, size_t len) const = 0;
  virtual int32_t UpdateEnv(const char *data, size_t len) const = 0;
  virtual int32_t BrowserOpen(const char *data, size_t len) const = 0;
  virtual int32_t BrowserClose(const char *data, size_t len) const = 0;
  virtual int32_t BrowserInfo(char **, size_t *) const = 0;
  virtual int32_t RegisterResultCb(sdk_result_cb_t cb, void *user_data) = 0;
  virtual int32_t RegisterCookiesStorageCb(sdk_cookies_storage_cb_t cb,
                                           void *user_data) = 0;
  virtual int32_t Shutdown() const = 0;
};
```

**使用方式**：

```cpp
sdk_handle_t h = nullptr;
sdk_init(&h, req, req_len, &out, &out_len);
ISDK *isdk = reinterpret_cast<ISDK *>(h);
isdk->BrowserOpen(open_req, strlen(open_req));
```

---

## 错误码

### 检查宏

```c
sdk_is_ok(code)      // code == 0，操作成功
sdk_is_done(code)    // code == 1，异步任务已接受
sdk_is_warn(code)    // 100-255，警告
sdk_is_event(code)   // 10000-100000，事件 ID
sdk_is_reqid(code)   // >100000，请求 ID
sdk_is_error(code)   // <0，错误
```

### 通用错误

| 错误码 | 说明 |
| --- | --- |
| 0 | 成功 |
| -1 | 未知错误 |
| -2 | 无效参数 |
| -3 | 内存不足 |
| -4 | SDK 未初始化 |
| 401 | 未认证或 Token 无效 |

### Token 相关

| 错误码 | 说明 |
| --- | --- |
| 10122 | Token 更新失败 |
| 10123 | Token 即将过期（警告） |
| 10124 | Token 已过期 |

### 环境相关

| 错误码 | 说明 |
| --- | --- |
| 10001 | 环境不存在 |
| 10002 | 环境状态异常 |
| 10003 | 环境数量超限 |

---

## 内存管理

### 规则

| 场景 | 规则 |
| --- | --- |
| 同步接口 `out_data` | SDK 内部分配，**必须**调用 `sdk_free()` 释放 |
| 回调中的 `data` 指针 | 生命周期仅限于当前回调，如需保存必须复制 |
| Cookie 拦截回调的替换数据 | **必须**通过 `sdk_malloc()` 分配 |

### 示例

```cpp
// 正确用法
char *out = nullptr;
size_t out_len = 0;

int32_t rc = sdk_env_create(handle, req, req_len, &out, &out_len);

// 使用 out...
printf("Result: %s\n", out);

// 释放
sdk_free(out);

// 错误用法（不要这样做）
// free(out);  // 可能导致跨 CRT 堆问题
// delete[] out;  // 错误
```

### Windows 注意事项

Windows 平台上，SDK 分配的内存**必须**使用 `sdk_free()` 释放，不要使用系统的 `free()` 或 `delete`，以避免跨 CRT 堆问题。

---

## 调用示例

### 完整示例（C++）

```cpp
#include "brosdk.h"
#include <stdio.h>
#include <string.h>

// 回调函数
static void on_result(int32_t code, void *user_data,
                      const char *data, size_t len) {
    if (sdk_is_event(code)) {
        switch (code) {
            case 10123:
                printf("警告：Token 即将过期\n");
                break;
            case 10124:
                printf("错误：Token 已过期\n");
                break;
        }
    }
}

int main() {
    // 1. 注册回调
    sdk_register_result_cb(on_result, nullptr);
    
    // 2. 初始化 SDK
    sdk_handle_t handle = nullptr;
    char *out = nullptr;
    size_t out_len = 0;
    
    const char *init_req = R"({
        "userSig": "eyJhbGciOiJSUzI1NiIs...",
        "workDir": "C:/brosdk/data",
        "port": 9527
    })";
    
    int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);
    
    if (rc != 0) {
        printf("SDK 初始化失败：%s\n", out);
        sdk_free(out);
        return 1;
    }
    
    printf("SDK 初始化成功\n");
    sdk_free(out);
    
    // 3. 创建环境
    const char *create_req = R"({
        "envName": "myenv",
        "customerId": "user123",
        "finger": {
            "system": "Windows 11",
            "kernel": "Chrome",
            "kernelVersion": "148"
        }
    })";
    
    rc = sdk_env_create(handle, create_req, strlen(create_req), &out, &out_len);
    
    if (rc == 0) {
        printf("环境创建成功：%s\n", out);
    } else {
        printf("环境创建失败：%s\n", out);
    }
    
    sdk_free(out);
    
    // 4. 使用完毕后清理
    // sdk_cleanup(handle);  // 如有清理函数
    
    return 0;
}
```

### Node.js 示例（使用 ffi-napi）

```javascript
const ffi = require('ffi-napi');
const ref = require('ref-napi');

// 加载动态库
const brosdk = ffi.Library('brosdk', {
    'sdk_init': ['int32', [ref.refType('void'), 'string', 'uint32', ref.refType('string'), ref.refType('uint32')]],
    'sdk_env_create': ['int32', ['void', 'string', 'uint32', ref.refType('string'), ref.refType('uint32')]],
    'sdk_free': ['void', ['pointer']]
});

// 初始化
const handle = ref.alloc(ref.types.void);
const out = ref.alloc(ref.types.CString);
const outLen = ref.alloc(ref.types.uint32);

const initReq = JSON.stringify({
    userSig: 'eyJhbGciOiJSUzI1NiIs...',
    workDir: 'C:/brosdk/data',
    port: 9527
});

const rc = brosdk.sdk_init(handle, initReq, initReq.length, out, outLen);

if (rc === 0) {
    console.log('SDK 初始化成功');
} else {
    console.log('SDK 初始化失败:', out.readCString());
}

brosdk.sdk_free(out.readPointer());
```

### Go 示例（使用 cgo）

```go
/*
#cgo LDFLAGS: -L. -lbrosdk
#include "brosdk.h"
*/
import "C"
import (
    "fmt"
    "unsafe"
)

func main() {
    var handle C.sdk_handle_t
    var out *C.char
    var outLen C.size_t
    
    initReq := C.CString(`{"userSig":"eyJhbGciOiJSUzI1NiIs...","workDir":"C:/brosdk/data","port":9527}`)
    defer C.free(unsafe.Pointer(initReq))
    
    rc := C.sdk_init(&handle, initReq, C.size_t(len(`{"userSig":"..."}`)), &out, &outLen)
    
    if rc == 0 {
        fmt.Println("SDK 初始化成功")
    } else {
        fmt.Println("SDK 初始化失败:", C.GoString(out))
    }
    
    C.sdk_free(unsafe.Pointer(out))
}
```

---

## 最佳实践

1. **认证管理**
   - 监听 Token 过期事件（10123）并主动刷新
   - 不要在客户端代码中暴露 User Sign 获取逻辑

2. **错误处理**
   - 检查所有 API 返回的 code 值
   - 记录错误日志用于调试
   - 实现重试机制处理临时性错误

3. **环境管理**
   - 定期清理不再使用的环境
   - 使用有意义的 envName 方便管理
   - 为不同用户使用不同的 customerId

4. **性能优化**
   - 批量查询而非频繁单条查询
   - 合理设置分页大小
   - 缓存环境信息减少 API 调用

---

## 附录

### 附录 A：错误码全表

| 错误码 | 名称 | 说明 |
| --- | --- | --- |
| `0` | OK | 操作成功 |
| `1` | DONE | 异步任务已受理 |
| `101` | WDIRNOTEXIST | 工作目录不存在（警告） |
| `102` | WNOCORERESOURCE | 核心资源不可用（警告） |
| `103` | WBRWPROCEXITED | 浏览器进程意外退出（警告） |
| `-3000` | ENO | 内部系统错误 |
| `-3001` | EBUSY | 资源忙 / 正在使用 |
| `-3002` | ETIMEOUT | 操作超时 |
| `-3003` | EINVALID | 参数或输入无效 |
| `-3004` | ENOTFOUND | 目标资源未找到 |
| `-3005` | EALREADY | 资源已存在 |
| `-3006` | ENOTSUPPORTED | 操作不被支持 |
| `-3007` | EINTERNAL | 内部故障 |
| `-3008` | ENOSPACE | 存储 / 空间不足 |
| `-3009` | EACCESS | 权限或访问被拒绝 |
| `-3010` | ECONFLICT | 资源冲突 |
| `-3011` | ERESOURCE | 资源不足 |
| `-3012` | ENOTINITIALIZED | SDK / 组件未初始化 |
| `-3013` | EOVERFLOW | 值或计数溢出 |
| `-3014` | EFORMAT | 数据格式无效 |
| `-3015` | ECANCELED | 操作被取消 |
| `-3016` | ENOTIMPLEMENTED | 功能未实现 |
| `-3017` | EDEADLINEEXCEEDED | 超出截止时间 |
| `-3018` | EUNAUTHORIZED | 未授权（令牌无效 / 过期） |
| `-3019` | EPORT_UNAVAILABLE | 端口无效或被占用 |
| `-3020` | ENOTSTARTED | 系统未启动 |
| `-3021` | ESVCSTARTED | 服务启动失败 |
| `-3022` | EREQIDOVERFLOW | 请求 ID 耗尽 / 溢出 |
| `-3023` | EOSS_NOCLIENT | 云端存储客户端未初始化 |
| `-3024` | EOSS_DOWNLOAD | 云端下载失败 |
| `-3025` | EOSS_UPLOAD | 云端上传失败 |
| `-3026` | EOSS_AUTH | 云端鉴权失败 |
| `-3027` | EOSS_NOTFOUND | 云端对象未找到 |
| `-3028` | ECOOKIE_RESTORE | Cookie 恢复失败 |
| `-3029` | ESTORAGE_RESTORE | Storage 恢复失败 |
| `-3500` | EINTERNAL_ERROR | 系统严重内部错误 |
| `-3501` | EDECRYPT | 数据解密失败 |
| `-3502` | EHTTP_POST | HTTP POST 请求失败 |
| `-3503` | EBRW_INVALIDENVID | envId 无效 |
| `-3504` | EBRW_PROCKILL | 浏览器进程终止失败 |
| `-3505` | EBRW_PROCCRE | 浏览器进程创建失败 |
| `-3506` | EBRW_PROCEXITED | 浏览器进程意外退出 |
| `-3507` | EBRW_NOTFOUND | 浏览器核心文件未找到 |
| `-3508` | EINTERNAL_GENAPIREQ | 生成 API 请求失败 |
| `-3509` | ETOKEN_INVALID | 令牌无效 |
| `-3510` | EOSS | 云端存储操作错误 |
| `-3511` | EWORKDIR_INVALID | 工作目录无效 |
| `-4000` | ESDKAPI | SDK 后端 API 错误 |
| `-4094` | EUNKNOWN | 未知错误 |

### 附录 B：事件码全表

#### SDK 初始化事件（10110~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `10110` | sdk-init | SDK 初始化开始 |
| `10111` | sdk-init-success | SDK 初始化成功 |
| `10112` | sdk-init-failed | SDK 初始化失败 |

#### 令牌相关事件（10120~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `10120` | sdk-token-update | 正在更新令牌 |
| `10121` | sdk-token-update-success | 令牌更新成功 |
| `10122` | sdk-token-update-failed | 令牌更新失败 |
| `10123` | sdk-token-expire-warning | **令牌即将过期**（应立即调用 `sdk_token_update`） |
| `10124` | sdk-token-expired | 令牌已过期 |

#### 浏览器打开事件（20110~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `20110` | browser-open | 浏览器打开中 |
| `20111` | browser-open-success | 浏览器打开成功 |
| `20112` | browser-open-failed | 浏览器打开失败 |
| `20113` | browser-open-timeout | 浏览器打开超时 |

#### 浏览器关闭事件（20140~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `20140` | browser-close | 浏览器关闭中 |
| `20141` | browser-close-success | 浏览器关闭成功 |
| `20142` | browser-close-failed | 浏览器关闭失败 |
| `20143` | browser-close-timeout | 浏览器关闭超时 |

#### 环境管理事件（20210~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `20210` | browser-env-create | 创建环境中 |
| `20211` | browser-env-create-success | 创建环境成功 |
| `20212` | browser-env-create-failed | 创建环境失败 |
| `20220` | browser-env-update | 更新环境中 |
| `20221` | browser-env-update-success | 更新环境成功 |
| `20222` | browser-env-update-failed | 更新环境失败 |
| `20230` | browser-env-page | 查询环境列表中 |
| `20231` | browser-env-page-success | 查询环境列表成功 |
| `20232` | browser-env-page-failed | 查询环境列表失败 |
| `20240` | browser-env-destroy | 销毁环境中 |
| `20241` | browser-env-destroy-success | 销毁环境成功 |
| `20242` | browser-env-destroy-failed | 销毁环境失败 |

#### Cookie / Storage 数据同步事件（20260~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `20260` | browser-cookie-upload | Cookie 上传中 |
| `20261` | browser-cookie-upload-success | Cookie 上传成功 |
| `20262` | browser-cookie-upload-failed | Cookie 上传失败 |
| `20265` | browser-cookie-download | Cookie 下载中 |
| `20266` | browser-cookie-download-success | Cookie 下载成功 |
| `20267` | browser-cookie-download-failed | Cookie 下载失败 |
| `20270` | browser-storage-upload | Storage 上传中 |
| `20271` | browser-storage-upload-success | Storage 上传成功 |
| `20272` | browser-storage-upload-failed | Storage 上传失败 |
| `20275` | browser-storage-download | Storage 下载中 |
| `20276` | browser-storage-download-success | Storage 下载成功 |
| `20277` | browser-storage-download-failed | Storage 下载失败 |

#### 云端存储事件（20300~）

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| `20300` | browser-oss | 云端操作 |
| `20301` | browser-oss-init-success | 云端存储初始化成功 |
| `20302` | browser-oss-init-failed | 云端存储初始化失败 |
| `20303` | browser-oss-not-initialized | 云端存储未初始化 |
| `20304` | browser-oss-token-updated | 云端 Token 已刷新 |
| `20305` | browser-oss-notfound | 云端对象未找到（首次使用该环境为正常现象） |
| `20306` | browser-oss-error | 云端存储错误 |
| `20307` | browser-oss-cache-hit | 本地缓存命中（无需下载） |
| `20308` | browser-oss-download | 云端下载中 |
| `20309` | browser-oss-download-success | 云端下载成功 |
| `20310` | browser-oss-download-failed | 云端下载失败 |
| `20311` | browser-oss-upload | 云端上传中 |
| `20312` | browser-oss-upload-success | 云端上传成功 |
| `20313` | browser-oss-upload-failed | 云端上传失败 |

### 附录 C：典型接入流程

```mermaid
flowchart TD
    A[① sdk_register_result_cb<br/>注册异步回调] --> B[② sdk_register_cookies_storage_cb<br/>注册 Cookie 拦截（可选）]
    B --> C[③ sdk_init<br/>初始化 SDK]
    C --> D{sdk-init-success?}
    D -->|否| E[处理初始化失败]
    D -->|是| F[④ sdk_env_create<br/>按需创建环境]
    F --> G[⑤ sdk_browser_open<br/>按 envId 打开浏览器]
    G --> H{browser-open-success?}
    H -->|否| I[处理打开失败]
    H --> J[用户操作浏览器]
    J --> K[⑥ sdk_browser_close<br/>关闭浏览器]
    K --> L{browser-close-success?}
    L -->|否| M[处理关闭失败]
    L --> N[数据自动持久化]
    N --> O{收到 eventId=10123?}
    O -->|是| P[调用 sdk_token_update<br/>刷新令牌]
    P --> F
    O -->|否| Q{退出?}
    Q -->|是| R[⑦ sdk_shutdown<br/>停止 SDK]
    Q -->|否| J
    
    style A fill:#e3f2fd,stroke:#1976d2
    style C fill:#e8f5e9,stroke:#388e3c
    style G fill:#fff3e0,stroke:#f57c00
    style K fill:#fff3e0,stroke:#f57c00
    style R fill:#ffebee,stroke:#d32f2f
```

---

## 相关文档

- [快速开始](quick-start.md) - SDK 初始化和配置
- [服务端 API](api/server.md) - 服务端 API 参考
- [C 语言集成](integration/c-native.md) - C SDK 集成指南
- [故障排除](troubleshooting.md) - 常见问题
