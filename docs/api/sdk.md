# SDK API 参考

BroSDK SDK API 完整参考文档。

## 概述

BroSDK 是一个 **C++ 编写的动态链接库**，用于调用浏览器内核。SDK API 提供浏览器环境管理和控制功能，用于客户端操作。

**架构说明**：
```
用户应用 ──▶ SDK (C++ DLL) ──▶ 浏览器内核 (Chromium)
              │
              └── 本地 HTTP 服务 (localhost:9527)
```

**基础 URL**：`http://localhost:9527` (默认端口，可在初始化时配置)

**Content-Type**：`application/json`

**API 认证**：所有 SDK API 都需要 `Authorization: Bearer YOUR_USER_SIGN` 请求头

**与服务端 API 的区别**：
- 请求/响应参数结构与服务端 API 完全一致
- 认证方式不同：使用 User Sign 而非 API Key
- SDK 内部会自动处理认证，无需每次手动设置请求头
- SDK API 操作本地浏览器实例，服务端 API 操作云端资源

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

> 两种方式最终都会汇聚到同一个核心分发器，**功能和行为完全相同**。C API 性能更好，HTTP API 更适合脚本语言集成。

## 响应格式

### C API 响应

```cpp
// 同步接口
int32_t rc = sdk_env_create(handle, req, req_len, &out, &out_len);
// rc = 0 表示成功，out 包含 JSON 响应

// 异步接口（回调方式）
void on_result(int32_t code, void *user_data, const char *data, size_t len) {
    if (sdk_is_ok(code)) {
        // 成功
    } else if (sdk_is_error(code)) {
        // 失败
    }
}
```

### HTTP API 响应

所有 HTTP API 返回统一的 JSON 格式：

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

---

## 初始化 SDK

在调用任何 SDK API 之前，必须先初始化 SDK。

### C API 初始化

```cpp
#include "brosdk.h"

sdk_handle_t handle = nullptr;
char *out = nullptr;
size_t out_len = 0;

// 初始化请求
const char *init_req = R"({
  "userSig": "eyJhbGciOiJSUzI1NiIs...",  // 从服务端获取
  "workDir": "C:/brosdk/data",             // 工作目录
  "port": 9527                              // HTTP 服务端口
})";

int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);

if (rc == 0) {
    printf("SDK 初始化成功\n");
    printf("环境 ID: %s\n", out);
    sdk_free(out);  // 记得释放
} else {
    printf("SDK 初始化失败：%s\n", out);
    sdk_free(out);
}
```

### 初始化参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | 从服务端获取的 JWT 令牌 |
| workDir | string | 是 | 工作目录，存储环境数据 |
| port | integer | 否 | HTTP 服务端口（默认 9527） |

### 获取 userSig

userSig 需要从 BroSDK 服务端获取：

```bash
POST https://api.brosdk.com/api/v2/browser/getUserSig
Authorization: Bearer YOUR_API_KEY
Content-Type: application/json

{
  "customerId": "user_12345",
  "duration": 86400
}
```

---

## 创建环境

创建新的浏览器环境。

### HTTP API

**端点**：`POST /sdk/v1/env/create`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### C API

```cpp
const char *create_req = R"({
  "envName": "我的浏览器",
  "customerId": "user_12345",
  "proxy": "socks5://user:pass@proxy:1080",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148"
  }
})";

char *out = nullptr;
size_t out_len = 0;

int32_t rc = sdk_env_create(handle, create_req, strlen(create_req), &out, &out_len);

if (rc == 0) {
    printf("环境创建成功：%s\n", out);  // out 包含 envId
} else {
    printf("环境创建失败：%s\n", out);
}

sdk_free(out);  // 释放内存
```

### HTTP 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envName | string | 否 | 环境名称 |
| customerId | string | 否 | 三方用户唯一 ID |
| proxy | string | 否 | 代理地址（格式：`socks5://user:pass@proxy:1080`） |
| finger | object | 否 | 浏览器指纹配置 |
| finger.system | string | 否 | 操作系统（如：Windows 11） |
| finger.kernel | string | 否 | 浏览器内核（如：Chrome） |
| finger.kernelVersion | string | 否 | 内核版本（如：148） |

### HTTP 请求示例

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

### HTTP 响应示例

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

### 响应字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| envId | string | 环境 ID |

---

## 更新环境

更新已存在的浏览器环境配置。

**端点**：`POST /sdk/v1/env/update`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |
| envName | string | 否 | 环境名称 |
| proxy | string | 否 | 代理地址 |
| finger | object | 否 | 浏览器指纹配置 |

### 请求示例

```json
{
  "envId": "2034183257439866880",
  "envName": "更新后的名称",
  "proxy": "socks5://newproxy:1080"
}
```

### 响应示例

```json
{
  "reqid": 1006901417,
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2034183257439866880"
  }
}
```

---

## 查询环境列表

分页查询浏览器环境列表。

**端点**：`POST /sdk/v1/env/page`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| customerId | string | 否 | 三方用户 ID |
| page | int | 否 | 页码（默认 1） |
| pageSize | int | 否 | 每页数量（默认 20） |

### 请求示例

```json
{
  "customerId": "user_12345",
  "page": 1,
  "pageSize": 20
}
```

### 响应示例

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

### 响应字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| total | int | 总数量 |
| page | int | 当前页码 |
| pageSize | int | 每页数量 |
| list | array | 环境列表 |
| list[].envId | string | 环境 ID |
| list[].envName | string | 环境名称 |
| list[].customerId | string | 三方用户 ID |
| list[].status | int | 状态（1=运行中，0=已停止） |
| list[].createTime | int64 | 创建时间戳 |

---

## 销毁环境

删除指定的浏览器环境。

**端点**：`POST /sdk/v1/env/destroy`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |

### 请求示例

```json
{
  "envId": "2034183257439866880"
}
```

### 响应示例

```json
{
  "reqid": 1006901419,
  "code": 0,
  "msg": "ok",
  "data": null
}
```

---

## 打开浏览器

打开指定环境的浏览器。

**端点**：`POST /sdk/v1/browser/open`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |
| url | string | 否 | 要打开的 URL |

### 请求示例

```json
{
  "envId": "2034183257439866880",
  "url": "https://www.example.com"
}
```

### 响应示例

```json
{
  "reqid": 1006901420,
  "code": 0,
  "msg": "ok",
  "data": {
    "browserId": "browser_123456"
  }
}
```

---

## 关闭浏览器

关闭指定环境的浏览器。

**端点**：`POST /sdk/v1/browser/close`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |

### 请求示例

```json
{
  "envId": "2034183257439866880"
}
```

### 响应示例

```json
{
  "reqid": 1006901421,
  "code": 0,
  "msg": "ok",
  "data": null
}
```

---

## 更新 User Sign

更新 SDK 使用的 User Sign 令牌。

**端点**：`POST /sdk/v1/token/update`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json
```

### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | 新的 User Sign |

### 请求示例

```json
{
  "userSig": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

### 响应示例

```json
{
  "reqid": 1006901422,
  "code": 0,
  "msg": "ok",
  "data": {
    "eventId": 10121
  }
}
```

---

## 错误码

### 通用错误

| 错误码 | 说明 |
| --- | --- |
| 0 | 成功 |
| -1 | 未知错误 |
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

## C API 调用示例

### 创建环境

```cpp
const char *create_env_req =
    "{"
    "  \"envName\": \"我的浏览器\","
    "  \"customerId\": \"user_12345\","
    "  \"proxy\": \"socks5://user:pass@proxy:1080\","
    "  \"finger\": {"
    "    \"system\": \"Windows 11\","
    "    \"kernel\": \"Chrome\","
    "    \"kernelVersion\": \"148\""
    "  }"
    "}";

char *out = nullptr;
size_t out_len = 0;

int32_t rc = sdk_env_create(handle, create_env_req, strlen(create_env_req),
                            &out, &out_len);

if (rc == 0) {
    printf("环境创建成功: %s\n", out);
} else {
    printf("环境创建失败: %s\n", out);
}
```

### 查询环境

```cpp
const char *query_req =
    "{"
    "  \"customerId\": \"user_12345\","
    "  \"page\": 1,"
    "  \"pageSize\": 20"
    "}";

char *out = nullptr;
size_t out_len = 0;

int32_t rc = sdk_env_page(query_req, strlen(query_req), &out, &out_len);

if (rc == 0) {
    printf("环境列表: %s\n", out);
}
```

### 销毁环境

```cpp
const char *destroy_req =
    "{"
    "  \"envId\": \"2034183257439866880\""
    "}";

char *out = nullptr;
size_t out_len = 0;

int32_t rc = sdk_env_destroy(destroy_req, strlen(destroy_req), &out, &out_len);

if (rc == 0) {
    printf("环境销毁成功\n");
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

## 相关文档

- [快速开始](../quick-start.md) - SDK 初始化和配置
- [服务端 API](server.md) - 服务端 API 参考
- [C 语言集成](../integration/c-native.md) - C SDK 集成指南
