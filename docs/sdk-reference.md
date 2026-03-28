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
