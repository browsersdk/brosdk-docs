# SDK 回调机制

BroSDK 的所有异步操作（浏览器打开/关闭、令牌刷新、数据同步等）均通过**回调**驱动，而非阻塞等待。本文档详细介绍回调的类型、状态码语义、完整事件码与错误码。

> **前置要求**：必须先调用 `sdk_register_result_cb()` 注册全局异步结果回调，才能调用其他 SDK 接口。

---

## 回调类型

BroSDK 提供两种回调：

| 回调类型 | 注册函数 | 触发时机 | 用途 |
|---|---|---|---|
| **异步结果回调** | `sdk_register_result_cb()` | 所有异步操作的进度/结果通知 | 接收事件通知、请求响应、错误等 |
| **Cookie 存储拦截回调** | `sdk_register_cookies_storage_cb()` | 浏览器关闭后、Cookie 持久化前 | 对 Cookie 数据进行二次处理（加密/过滤/注入） |

---

## 异步结果回调

### 回调函数类型

```c
typedef void(SDK_CALL *sdk_result_cb_t)(
    int32_t     code,       /* 状态码或请求 ID */
    void       *user_data,  /* 注册时传入的用户指针（原样返回） */
    const char *data,       /* 通知数据（UTF-8 JSON） */
    size_t      len          /* data 的字节长度 */
);
```

**回调参数说明**：

| 参数 | 类型 | 说明 |
|---|---|---|
| `code` | `int32_t` | 状态码或请求 ID，用于判断通知类型（见下节"状态码语义"） |
| `user_data` | `void *` | 注册时传入的用户指针，原样返回 |
| `data` | `const char *` | JSON 通知体（UTF-8），**生命周期仅限当次回调**，不得持有 |
| `len` | `size_t` | `data` 字节长度 |

### 状态码语义

`code` 按数值范围划分语义：

| 范围 | 含义 | 判断函数 | 说明 |
|---|---|---|---|
| `0` | 操作成功（OK） | `sdk_is_ok(code)` | 同步接口返回 |
| `1` | 异步任务已受理（DONE） | `sdk_is_done(code)` | 结果通过回调通知 |
| `100` ~ `255` | 警告（非致命） | `sdk_is_warn(code)` | 操作仍继续 |
| `10000` ~ `100000` | 事件 ID | `sdk_is_event(code)` | 标识事件类型 |
| `> 100000` | 请求 ID | `sdk_is_reqid(code)` | 关联异步请求响应 |
| `< 0` | 错误 | `sdk_is_error(code)` | 操作失败 |

**推荐判断顺序**：

```c
if      (sdk_is_reqid(code))  { /* 异步请求响应 */ }
else if (sdk_is_event(code))  { /* 事件通知     */ }
else if (sdk_is_done(code))   { /* 任务已受理   */ }
else if (sdk_is_ok(code))     { /* 操作成功     */ }
else if (sdk_is_warn(code))   { /* 警告         */ }
else if (sdk_is_error(code))  { /* 错误         */ }
```

### 回调处理示例（C）

```c
static void on_sdk_result(int32_t code, void *user_data,
                          const char *data, size_t len) {
    if (sdk_is_reqid(code)) {
        /* 异步请求响应 */
        printf("[ReqID=%d] %.*s\n", code, (int)len, data);

    } else if (sdk_is_event(code)) {
        /* 事件通知 */
        printf("[Event=%s] %.*s\n", sdk_event_name(code), (int)len, data);

        /* 常见事件分支处理 */
        switch (code) {
            case 10111:  /* SDK 初始化成功 */
                break;
            case 10123:  /* Token 即将过期，应立即调用 sdk_token_update */
                break;
            case 10124:  /* Token 已过期，需重新获取 User Sign */
                break;
            case 20110:  /* 浏览器打开中 */
                break;
            case 20111:  /* 浏览器打开成功 */
                break;
            case 20112:  /* 浏览器打开失败 */
                break;
            case 20113:  /* 浏览器打开超时 */
                break;
            case 20140:  /* 浏览器关闭中 */
                break;
            case 20141:  /* 浏览器关闭成功 */
                break;
            case 20142:  /* 浏览器关闭失败 */
                break;
            case 20143:  /* 浏览器关闭超时 */
                break;
        }

    } else if (sdk_is_error(code)) {
        fprintf(stderr, "[Error] %s: %s\n",
                sdk_error_name(code), sdk_error_string(code));
    }
}

/* 注册（必须在 sdk_init 之前调用）*/
sdk_register_result_cb(on_sdk_result, NULL);
```

### 回调处理示例（TypeScript）

```typescript
sdk.registerResultCb((code: number, data: string) => {
  if (sdk.isOk(code)) {
    // 操作成功
  } else if (sdk.isDone(code)) {
    // 异步任务已受理
  } else if (sdk.isEvent(code)) {
    console.log('事件:', sdk.eventName(code), data)

    switch (code) {
      case 10111: break // SDK 初始化成功
      case 10123: break // Token 即将过期
      case 20111: break // 浏览器打开成功
      case 20141: break // 浏览器关闭成功
    }
  } else if (sdk.isError(code)) {
    console.error('错误:', sdk.errorString(code))
  }
})
```

---

## Cookie 存储拦截回调

### 回调函数类型

```c
typedef void(SDK_CALL *sdk_cookies_storage_cb_t)(
    const char *data,       /* [IN]  SDK 提取的 Cookie JSON 数据 */
    size_t      len,        /* [IN]  data 字节长度               */
    char      **new_data,   /* [OUT] 替换数据指针；NULL = 透传   */
    size_t     *new_len,    /* [OUT] 替换数据字节长度            */
    void       *user_data   /* [IN]  注册时传入的用户指针        */
);
```

### 两种工作模式

- **透传模式**：将 `*new_data` 置 `NULL`，SDK 使用原始数据持久化
- **替换模式**：通过 `sdk_malloc()` 分配替换数据写入 `*new_data` / `*new_len`

> **内存规则**：替换数据 `*new_data` **必须**通过 `sdk_malloc()` 分配，SDK 将自动调用 `sdk_free()` 释放。

### Cookie 数据格式

SDK 输出的 Cookie 格式为 WebExtension API 兼容的 JSON 数组：

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

### 拦截回调示例

```c
/* 透传（不修改） */
static void on_cookies_passthrough(const char *data, size_t len,
                                    char **new_data, size_t *new_len,
                                    void *user_data) {
    *new_data = NULL;
    *new_len  = 0;
}

/* 替换（对 Cookie 数据进行二次加工） */
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

---

## 内存管理

| 场景 | 规则 |
|---|---|
| 同步接口 `out_data` / `out_len` 输出 | SDK 内部分配；使用完毕后**必须**调用 `sdk_free()` 释放 |
| 回调中接收的 `data` 指针 | 生命周期仅限当次回调，**不得**持有；如需保存请在回调内拷贝 |
| Cookie 拦截回调的替换数据 | **必须**通过 `sdk_malloc()` 分配；SDK 内部将调用 `sdk_free()` 释放 |

> **Windows 注意**：SDK 分配的内存必须用 `sdk_free()` 释放，不得使用宿主的 `free()`，以避免跨 CRT 堆问题。

---

## 统一响应体格式

所有同步 C API 输出及 Web API 响应均遵循以下 JSON 信封格式：

```json
{
  "reqid": 1006901415,
  "code": 0,
  "msg": "ok",
  "data": {
    "eventId": 0
  }
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `reqid` | int | 请求关联 ID（用于追踪异步请求） |
| `code` | int | `0` = 成功，`< 0` = 错误 |
| `msg` | string | 可读状态描述 |
| `data` | object | 接口特定返回数据 |

---

## 完整事件码表

### SDK 初始化事件（10110~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
| `10110` | sdk-init | SDK 初始化开始 |
| `10111` | sdk-init-success | SDK 初始化成功 |
| `10112` | sdk-init-failed | SDK 初始化失败 |

### 令牌相关事件（10120~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
| `10120` | sdk-token-update | 正在更新令牌 |
| `10121` | sdk-token-update-success | 令牌更新成功 |
| `10122` | sdk-token-update-failed | 令牌更新失败 |
| `10123` | sdk-token-expire-warning | **令牌即将过期**（应立即调用 `sdk_token_update`） |
| `10124` | sdk-token-expired | 令牌已过期 |

### 浏览器打开事件（20110~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
| `20110` | browser-open | 浏览器打开中 |
| `20111` | browser-open-success | 浏览器打开成功 |
| `20112` | browser-open-failed | 浏览器打开失败 |
| `20113` | browser-open-timeout | 浏览器打开超时 |

### 浏览器关闭事件（20140~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
| `20140` | browser-close | 浏览器关闭中 |
| `20141` | browser-close-success | 浏览器关闭成功 |
| `20142` | browser-close-failed | 浏览器关闭失败 |
| `20143` | browser-close-timeout | 浏览器关闭超时 |

### 环境管理事件（20210~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
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
| `20250` | browser-env-info | 获取环境信息中 |
| `20251` | browser-env-info-success | 获取环境信息成功 |
| `20252` | browser-env-info-failed | 获取环境信息失败 |

### Cookie / Storage 数据同步事件（20260~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
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

### 云端存储事件（20300~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
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

### 浏览器核心事件（20400~）

| 事件 ID | 名称 | 说明 |
|---|---|---|
| `20400` | browser-core | 浏览器核心 |
| `20401` | browser-core-count | 浏览器核心数量信息 |
| `20402` | browser-core-notfound | 浏览器核心文件未找到 |

---

## 完整错误码表

### 通用错误（-3000 ~ -3029）

| code | 名称 | 描述 |
|---|---|---|
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

### 浏览器运行时错误（-3500 ~ -3511）

| code | 名称 | 描述 |
|---|---|---|
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

### SDK API 错误（-4000 ~ -4094）

| code | 名称 | 描述 |
|---|---|---|
| `-4000` | ESDKAPI | SDK 后端 API 错误 |
| `-4094` | EUNKNOWN | 未知错误 |

---

## 相关文档

- [C / C++ 集成指南](./c-native.md)
- [TypeScript 集成指南](./typescript.md)
- [Rust 集成指南](./rust.md)
- [SDK API 参考](../sdk-reference.md)
