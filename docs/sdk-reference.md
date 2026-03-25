# BroSDK SDK 参考

BroSDK 是一个 **C++ 编写的动态链接库**，用于调用浏览器内核。本文档提供 SDK 的完整参考信息。

## 目录

- [架构概述](#架构概述)
- [SDK 下载](#sdk 下载)
- [C API 参考](#c-api 参考)
- [状态码](#状态码)
- [内存管理](#内存管理)
- [调用示例](#调用示例)

---

## 架构概述

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│   用户应用      │────▶│  BroSDK SDK     │────▶│  浏览器内核     │
│  (C++/Go/TS)   │     │  (C++ DLL/SO)   │     │  (Chromium)    │
└─────────────────┘     └──────────────────┘     └─────────────────┘
       ▲                         │
       │                         │
       │ 1. 获取 User Sign       │ 2. 本地 HTTP 服务
       └─────────────────────────┘
              BroSDK 服务器           (localhost:9527)
```

**核心流程**：
1. 用户使用 **API Key** 从 BroSDK 服务器换取 **User Sign**（JWT 令牌）
2. 使用 User Sign 初始化 SDK
3. 通过 C API 或 HTTP API 调用浏览器内核

---

## SDK 下载

### 官方仓库

| 组件 | 说明 | 下载地址 |
| --- | --- | --- |
| **SDK** | C++ 动态链接库，核心调用接口 | https://github.com/browsersdk/brosdk-sdk |
| **浏览器内核** | Chromium 定制内核 | https://github.com/browsersdk/brosdk-core |
| **SDK Demo** | 完整的使用示例 | https://github.com/browsersdk/browser-sdk-demo |
| **Go 服务端 SDK** | Go 语言封装 | https://github.com/browsersdk/brosdk-server-go |
| **TypeScript SDK** | TypeScript/Node.js 封装 | https://github.com/browsersdk/brosdk-sdk-typescript |

### 平台支持

| 平台 | 架构 | 库文件 | 状态 |
| --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | ✅ 已发布 |
| Linux | x64 | `brosdk.so` | 🚧 计划中 |
| macOS | arm64/x64 | `brosdk.dylib` | 🚧 测试中 |

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

### 核心函数

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

**初始化请求示例**：
```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIs...",
  "workDir": "C:/brosdk/data",
  "port": 9527
}
```

**返回值**：
- `0`：成功
- `< 0`：失败（错误码）

---

#### sdk_free - 释放内存

```c
void sdk_free(void *ptr);
```

**重要**：所有 `sdk_*` 函数输出的 `out_data` 必须用 `sdk_free()` 释放。

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

**请求示例**：
```json
{
  "envId": "2034183257439866880",
  "url": "https://www.example.com"
}
```

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

**请求示例**：
```json
{
  "envId": "2034183257439866880"
}
```

---

#### sdk_register_result_cb - 注册回调

```c
void sdk_register_result_cb(
    sdk_result_cb_t  cb,
    void            *user_data
);
```

注册异步结果回调函数。

---

### 状态码

#### 检查宏

```c
sdk_is_ok(code)      // code == 0，操作成功
sdk_is_done(code)    // code == 1，异步任务已接受
sdk_is_warn(code)    // 100-255，警告
sdk_is_event(code)   // 10000-100000，事件 ID
sdk_is_reqid(code)   // >100000，请求 ID
sdk_is_error(code)   // <0，错误
```

#### 常见错误码

| 错误码 | 说明 |
| --- | --- |
| 0 | 成功 |
| -1 | 未知错误 |
| -2 | 无效参数 |
| -3 | 内存不足 |
| -4 | SDK 未初始化 |
| 10001 | 环境不存在 |
| 10002 | 环境状态异常 |
| 10123 | Token 即将过期（事件） |
| 10124 | Token 已过期（事件） |

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

## 相关文档

- [快速开始](quick-start.md) - SDK 初始化和配置
- [服务端 API](api/server.md) - 服务端 API 参考
- [SDK API](api/sdk.md) - SDK HTTP API 参考
- [C 语言集成](integration/c-native.md) - C SDK 集成指南
- [故障排除](troubleshooting.md) - 常见问题
