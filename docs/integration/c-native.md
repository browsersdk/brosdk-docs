# 原生 C 集成指南

BroSDK 是一个高性能的多环境浏览器管理 SDK，使用 C/C++ 实现。本指南将帮助你在 C/C++ 应用程序中集成 BroSDK。

## 系统要求

- **64 位操作系统** — Windows x64（已发布） | Linux x64 / macOS arm64（计划中）
- **C 标准库**：最低 C11（`<stdint.h>`、`<stdbool.h>`、`<stddef.h>`）

## 下载 SDK

### 官方仓库

- **SDK 下载**：https://github.com/browsersdk/brosdk-sdk
- **浏览器内核**：https://github.com/browsersdk/brosdk-core
- **SDK Demo**：https://github.com/browsersdk/browser-sdk-demo
- **Go 服务端 SDK**：https://github.com/browsersdk/brosdk-server-go
- **TypeScript SDK**：https://github.com/browsersdk/brosdk-sdk-typescript

## 集成方式

你可以选择两种等价的集成路径：

| 方式 | 适用场景 |
| --- | --- |
| **C API** | 直接加载动态库并调用导出的函数。适用于 C/C++、Python、Go、Node.js 等 |
| **Web API** | 通过 SDK 内置 HTTP 服务访问 REST 端点。适用于任何可以发起 HTTP 请求的语言 |

> 两种方式最终都会汇聚到同一个核心分发器，**功能和行为完全相同**。

## 组件

### 头文件

`brosdk.h` — SDK 唯一的头文件，依赖以下 C11 标准头文件：

```c
#include <stddef.h>   /* size_t */
#include <stdint.h>   /* int32_t, uint16_t, int64_t */
#include <stdbool.h>  /* bool */
#include <stdio.h>
#include <stdlib.h>
```

### 动态库

| 平台 | 架构 | 库文件 | 状态 |
| --- | --- | --- | --- |
| Windows | x64 | `brosdk.dll` | ✅ 已发布 |
| Linux | x64 | `brosdk.so` | 🚧 计划中 |
| macOS | arm64 / x64 | `brosdk.dylib` | 🚧 测试中 |

> 目前仅 Windows x64 已完全验证。其他平台需要联系维护团队获取支持。

## 语言集成

任何可以加载 C 标准动态库的语言都可以集成：

- **C / C++**：直接 `#include "brosdk.h"` 并链接动态库
- **Python**：`ctypes.CDLL("brosdk.dll")`
- **Go**：cgo + `#cgo LDFLAGS: -L. -lbrosdk`
- **Node.js**：`ffi-napi` 或原生插件

## 类型定义

### sdk_handle_t

不透明的 C++ 实例句柄 — 由 `sdk_init()` 填充，C++ 调用者可以转换为 `ISDK*`

```c
typedef void *sdk_handle_t;
```

### sdk_result_cb_t

异步结果回调函数类型。

```c
typedef void(SDK_CALL *sdk_result_cb_t)(
    int32_t     code,       /* 状态码或请求ID */
    void       *user_data,  /* 注册时传入的用户数据指针（按原样返回） */
    const char *data,       /* 通知数据（UTF-8 JSON） */
    size_t      len         /* 数据的字节长度 */
);
```

### sdk_cookies_storage_cb_t

Cookie / Storage 存储拦截回调。

```c
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(
    const char *data,       /* [IN] SDK 提取的 Cookie JSON 数据 */
    size_t      len,        /* [IN] 数据的字节长度 */
    char      **new_data,   /* [OUT] 替换数据指针；NULL 表示透传 */
    size_t     *new_len,    /* [OUT] 替换数据的字节长度 */
    void       *user_data   /* [IN] 注册时传入的用户数据指针 */
);
```

## 调用约定

| 平台 | `SDK_CALL` | `SDK_API` |
| --- | --- | --- |
| Windows | `__cdecl` | `__declspec(dllexport)` |
| Linux / macOS | (空) | `__attribute__((visibility("default")))` |

## 状态码语义

所有 C API 函数返回 `int32_t`，根据数值范围划分语义：

| 范围 | 含义 | 检查函数 |
| --- | --- | --- |
| `0` | 操作成功（OK） | `sdk_is_ok(code)` |
| `1` | 异步任务已接受（DONE）— 结果通过回调通知 | `sdk_is_done(code)` |
| `100` ~ `255` | 警告（非致命，操作继续） | `sdk_is_warn(code)` |
| `10000` ~ `100000` | 事件 ID（在回调中标识事件类型） | `sdk_is_event(code)` |
| `> 100000` | 请求 ID（在回调中关联异步请求） | `sdk_is_reqid(code)` |
| `< 0` | 错误（操作失败） | `sdk_is_error(code)` |

> **最佳实践**：在 `sdk_result_cb_t` 回调中，按顺序检查 `sdk_is_reqid()` → `sdk_is_event()` → `sdk_is_error()`。

## JSON 输入约定

C API 中所有 `const char *data, size_t len` 参数都是 **UTF-8 编码的 JSON 字符串**，其中 `len` 是字节数（不包括结尾的 `'\0'`）。

## 内存管理

| 场景 | 规则 |
| --- | --- |
| 同步接口 `out_data` / `out_len` 输出 | SDK 内部分配；**必须**在使用后调用 `sdk_free()` 释放 |
| 回调中接收的 `data` 指针 | 生命周期仅限于当前回调，**不能**持有；如需保存必须在回调中复制 |
| Cookie 拦截回调的替换数据 | **必须**通过 `sdk_malloc()` 分配；SDK 会调用 `sdk_free()` 释放 |

> **Windows 注意**：SDK 分配的内存**必须**使用 `sdk_free()` 释放，不要使用主机的 `free()`，以避免跨 CRT 堆问题。

## 完整示例

### C 语言示例

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "brosdk.h"

// 全局回调函数
static void SDK_CALL OnResult(int32_t code, void *user_data, 
                               const char *data, size_t len) {
    // 检查是否为事件
    if (sdk_is_event(code)) {
        printf("事件 [%d]: %s - %.*s\n", code, sdk_event_name(code), (int)len, data);
        
        // Token 即将过期（事件 ID: 10123）
        if (code == 10123) {
            printf("警告：Token 即将过期，需要刷新\n");
            // 调用服务端 API 获取新的 User Sign
        }
    }
    // 检查是否为请求 ID（异步操作结果）
    else if (sdk_is_reqid(code)) {
        printf("请求 %d 完成：%.*s\n", code, (int)len, data);
    }
    // 检查是否错误
    else if (sdk_is_error(code)) {
        fprintf(stderr, "错误 [%d]: %s - %s\n", code, 
                sdk_error_name(code), sdk_error_string(code));
    }
}

// Cookie 持久化回调
static void SDK_CALL OnCookiesStorage(const char *data, size_t len,
                                       char **new_data, size_t *new_len,
                                       void *user_data) {
    // 透传模式：不修改 Cookie 数据
    printf("收到 Cookie 数据：%.*s\n", (int)len, data);
    
    // 如需修改，分配新内存并写入
    // const char *modified = "{\"modified\": true}";
    // *new_data = (char *)sdk_malloc(strlen(modified));
    // memcpy(*new_data, modified, strlen(modified));
    // *new_len = strlen(modified);
}

int main(int argc, char *argv[]) {
    // 1. 注册回调
    sdk_register_result_cb(OnResult, NULL);
    sdk_register_cookies_storage_cb(OnCookiesStorage, NULL);
    
    // 2. 初始化 SDK
    sdk_handle_t handle = NULL;
    const char *init_req = 
        "{"
        "  \"userSig\": \"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...\","
        "  \"workDir\": \"C:/brosdk/\","
        "  \"port\": 9527"
        "}";
    
    char *out_data = NULL;
    size_t out_len = 0;
    
    int32_t rc = sdk_init(&handle, init_req, strlen(init_req), 
                          &out_data, &out_len);
    
    if (rc == 0) {
        printf("SDK 初始化成功：%.*s\n", (int)out_len, out_data);
        sdk_free(out_data);  // 必须释放
        out_data = NULL;
    } else {
        fprintf(stderr, "SDK 初始化失败 [%d]: %s\n", rc, sdk_error_string(rc));
        return 1;
    }
    
    // 3. 创建环境
    const char *create_req = 
        "{"
        "  \"envName\": \"测试环境\","
        "  \"customerId\": \"user_12345\","
        "  \"proxy\": \"socks5://user:pass@proxy:1080\","
        "  \"finger\": {"
        "    \"system\": \"Windows 11\","
        "    \"kernel\": \"Chrome\","
        "    \"kernelVersion\": \"148\""
        "  }"
        "}";
    
    rc = sdk_env_create(create_req, strlen(create_req), &out_data, &out_len);
    
    if (rc == 0) {
        printf("环境创建成功：%.*s\n", (int)out_len, out_data);
        
        // 解析响应获取 envId
        // 假设响应为：{"envId": 2034183257439866880}
        sdk_free(out_data);
        out_data = NULL;
        
        // 4. 打开浏览器
        const char *open_req = 
            "{"
            "  \"envs\": [{"
            "    \"envId\": 2034183257439866880,"
            "    \"urls\": [\"https://www.example.com\"]"
            "  }]"
            "}";
        
        rc = sdk_browser_open(open_req, strlen(open_req));
        
        if (rc == 0) {
            printf("浏览器打开成功\n");
        } else {
            fprintf(stderr, "浏览器打开失败 [%d]: %s\n", rc, sdk_error_string(rc));
        }
        
        // 等待一段时间...
        printf("按任意键关闭浏览器...\n");
        getchar();
        
        // 5. 关闭浏览器
        const char *close_req = 
            "{"
            "  \"envs\": [2034183257439866880]"
            "}";
        
        rc = sdk_browser_close(close_req, strlen(close_req));
        
        if (rc == 0) {
            printf("浏览器关闭成功\n");
        } else {
            fprintf(stderr, "浏览器关闭失败 [%d]: %s\n", rc, sdk_error_string(rc));
        }
    } else {
        fprintf(stderr, "环境创建失败 [%d]: %s\n", rc, sdk_error_string(rc));
    }
    
    // 6. 关闭 SDK，释放所有资源
    sdk_shutdown();
    
    return 0;
}
```

### 编译示例（Windows）

```bash
# 使用 MSVC 编译
cl /I. main.c brosdk.lib /Fe:demo.exe

# 使用 MinGW 编译
gcc -I. main.c -L. -lbrosdk -o demo.exe
```

### 关键点说明

1. **回调注册**：必须在调用任何 API 之前注册回调
2. **内存释放**：所有返回 `out_data` 的接口，使用后必须调用 `sdk_free()` 释放
3. **状态码检查**：使用 `sdk_is_ok()`、`sdk_is_error()` 等辅助函数判断结果
4. **事件处理**：在回调中检查 `sdk_is_event()` 处理 Token 过期等事件
5. **批量操作**：`sdk_browser_open` 和 `sdk_browser_close` 支持批量传入多个 envId

## 错误处理最佳实践

```c
// 在回调中统一处理错误
static void SDK_CALL OnResult(int32_t code, void *user_data, 
                               const char *data, size_t len) {
    if (sdk_is_ok(code)) {
        printf("操作成功：%.*s\n", (int)len, data);
    }
    else if (sdk_is_done(code)) {
        printf("异步任务已接受，等待回调通知\n");
    }
    else if (sdk_is_warn(code)) {
        printf("警告 [%d]: %s\n", code, sdk_error_string(code));
    }
    else if (sdk_is_event(code)) {
        const char *eventName = sdk_event_name(code);
        printf("事件 [%s]: %.*s\n", eventName, (int)len, data);
        
        // 常见事件处理
        switch (code) {
            case 10123:  // Token 即将过期
                // 刷新 Token
                break;
            case 10124:  // Token 已过期
                // 重新获取 User Sign
                break;
            case 20111:  // 浏览器打开成功
                // 记录环境状态
                break;
            case 20112:  // 浏览器关闭成功
                // 清理环境状态
                break;
        }
    }
    else if (sdk_is_reqid(code)) {
        printf("异步请求 %d 完成：%.*s\n", code, (int)len, data);
    }
    else if (sdk_is_error(code)) {
        fprintf(stderr, "错误 [%d] %s: %s\n", code, 
                sdk_error_name(code), sdk_error_string(code));
    }
}
```

> **Windows 注意**：SDK 分配的内存**必须**使用 `sdk_free()` 释放，不要使用主机的 `free()`，以避免跨 CRT 堆问题。

## 环境数据管理机制

SDK 以 `envId`（64 位整数）为单位管理浏览器环境数据。每个环境都有独立的 cookies、浏览历史、本地存储、IndexedDB 等持久化数据。

### 整体架构

```plaintext
┌──────────────────────────────────────────────────────┐
│  浏览器实例(env1) │ 浏览器实例(env2) │ ... (envN) │
└──────────┬─────────┴──────────┬─────────┴────────────┘
           │ 关闭/启动                     │
┌──────────▼───────────────────────────────▼───────────┐
│   内存缓存映射 (envId → CacheData)            │
│   env1 → { cookies.pb, storage.br, oss_paths }      │
│   env2 → { cookies.pb, storage.br, oss_paths }      │
└─────┬──────────────────────────────────────┬─────────┘
      │ 入队                               │ 查询
┌─────▼─────────┐                   ┌────────▼────────┐
│  上传队列  │                   │  下载队列  │
└─────┬─────────┘                   └────────┬────────┘
      │ 消费                             │ 消费
┌─────▼─────────┐                   ┌────────▼────────┐
│  上传线程  │───上传对象──→   │  下载线程│
└───────────────┘      云存储  ←──获取对象──┘
```

### 浏览器启动（数据恢复）

调用 `sdk_browser_open` 时，SDK 按以下方式为每个 `envId` 恢复数据：

1. 检查内存缓存是否包含 envId 且数据是最新的
   - 是 → 直接使用内存的 cookies.pb + storage.br
   - 否 → 检查下载队列是否有此 envId 的任务
     - 是 → 等待下载完成
     - 否 → 触发云下载，加入下载队列
2. 解压 storage.br → 恢复到 UserData/${Profile}/
3. 恢复 cookies.pb → 写入 Cookies 文件
4. 恢复浏览器数据文件（History、IndexedDB、LocalStorage 等）
5. 验证后启动浏览器进程

> 数据等待超时为 60 秒。如果超时后仍未获取环境数据，SDK 将尝试使用空环境启动浏览器。

### 浏览器关闭（数据持久化）

调用 `sdk_browser_close` 时，SDK 为每个 `envId` 执行：

1. 终止浏览器进程
2. 扫描 UserData/${Profile}/ 目录，收集需要持久化的文件
3. 压缩归档 → storage.br
4. 提取 Cookies → cookies.pb
5. 更新内存缓存映射
6. 如果下载队列有此 envId → 中断旧下载，替换为最新数据
7. 通知调用者浏览器关闭完成
8. 如果上传队列有此 envId → 中断旧上传，使用最新数据重新入队
9. 添加上传任务到上传队列

> 关闭通知在数据收集完成后立即发送，不阻塞等待云上传。

## Cookie 与存储存储原理

### 数据托管模式

SDK 通过 `sdk_info` 接口的 `dataFullyManaged` 字段区分两种模式：

| 模式 | `dataFullyManaged` | 存储位置 | 说明 |
| --- | --- | --- | --- |
| **完全托管** | `true` | 云对象存储（OSS） | 数据自动同步到云，支持跨设备共享 |
| **自托管** | `false` | 本地 SQLite | 存储在本地数据库，以 `envId` 为主键 |

### Cookie 数据格式

SDK 从 Chromium 的 SQLite Cookies 数据库提取数据，解密后以 WebExtension API 兼容的 JSON 格式输出：

```json
[
  {
    "domain": ".example.com",
    "expirationDate": 1700000000,
    "hostOnly": false,
    "httpOnly": true,
    "name": "session_id",
    "path": "/",
    "sameSite": "lax",
    "secure": true,
    "session": false,
    "storeId": "",
    "value": "abc123"
  }
]
```

> Cookie 解密由 SDK 自动处理，调用者无需担心加密细节。

### Cookie 拦截回调

通过 `sdk_register_cookies_storage_cb` 注册的拦截回调，你可以在持久化之前拦截并修改 Cookie 数据。这提供了以下能力：

- **透传模式**：不修改，SDK 使用原始数据进行持久化
- **替换模式**：调用者可以对 Cookie 数据进行二次处理（如加密、过滤、注入），并将处理后的数据返回给 SDK

### 存储归档策略

SDK 根据内置的存储策略（`StoragePolicy`）决定哪些浏览器数据文件参与归档/恢复：

| 文件/目录 | 归档（关闭时） | 恢复（打开时） |
| --- | --- | --- |
| `Bookmarks` | ✅ | ✅ |
| `History` | ✅ | ✅ |
| `History-journal` | ✅ | ✅ |
| `IndexedDB/` | ✅ | ✅ |
| `Local Storage/` | ✅ | ✅ |
| `Preferences` | ❌ | ❌ |
| `Secure Preferences` | ❌ | ❌ |
| `Network/` | ❌ | ❌ |
| `Local Extension Settings/` | ❌ | ❌ |
| `Sync Extension Settings/` | ❌ | ❌ |

> 以 `/` 结尾的路径作为目录递归处理。在策略中，`push=true` 表示归档，`pop=true` 表示恢复。

## 并发模型

SDK 内部运行 3 个常驻工作线程，使用生产者-消费者模式并行处理环境数据同步任务。

### 线程概览

| 线程 | 职责 | 调度方式 |
| --- | --- | --- |
| **主分发线程** | 浏览器进程生命周期管理（启动/关闭状态机） | 事件驱动 |
| **上传线程** | 消费上传队列，将 Cookie/Storage 持久化到存储后端 | 队列 + 条件变量 |
| **下载线程** | 消费下载队列，从存储后端拉取环境数据到内存缓存 | 队列 + 条件变量 |

### 批量操作与并行性

- `sdk_browser_open` 支持**批量**传入多个 `envId`，SDK 会并行处理每个环境的数据恢复
- `sdk_browser_close` 也支持批量关闭，数据收集并行执行
- 上传/下载队列按 `envId` 去重：同一环境的新任务将**中断**旧任务，确保始终同步最新数据

### 同环境竞态处理

当同一 `envId` 的上传/下载任务冲突时，SDK 遵循"最新数据优先"原则：

- **关闭触发上传，但 envId 正在下载**：中断下载，替换为本地最新数据
- **关闭触发上传，但 envId 正在上传**：中断旧上传，使用最新数据重新入队
- **打开触发下载，但 envId 已有本地最新数据**：直接使用内存缓存，跳过下载

## 下一步

- [SDK API 参考](../api/sdk.md) - 完整的 SDK API 文档
- [服务端 API 参考](../api/server.md) - 服务端 API 文档
- [SDK Demo](https://github.com/browsersdk/browser-sdk-demo) - 完整的示例项目
