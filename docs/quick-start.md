# 快速开始

欢迎使用 BroSDK！本指南将帮助你快速上手并开始使用浏览器环境管理服务。

## 架构说明

BroSDK SDK 是一个 **C++ 编写的动态链接库**，用于调用浏览器内核。用户需要使用 **API Key** 从 BroSDK 服务器换取 **User Sign**，然后才能调用 SDK API。

**核心组件**：
- **SDK**：C++ 动态库（`brosdk.dll` / `brosdk.so` / `brosdk.dylib`）
- **浏览器内核**：基于 Chromium 的定制浏览器内核
- **服务端**：提供 API Key 认证和 User Sign 颁发

## 快速流程概览

```mermaid
graph TD
    A[注册账号] --> B[创建应用]
    B --> C[获取 API Key]
    C --> D[服务端：用 API Key 换取 User Sign]
    D --> E[初始化 SDK]
    E --> F[调用 SDK API 操作浏览器]
```

## 第一步：注册账号

访问 [BroSDK 用户中心](https://www.brosdk.com) 完成用户注册。

### 注册流程

1. 访问官网用户中心
2. 填写注册信息（手机号或邮箱）
3. 完成验证
4. 设置密码（可选，支持验证码登录）

### 账号状态

- **正常**：可以使用所有功能
- **冻结**：账号无法使用，请联系客服

## 第二步：创建应用

注册完成后，创建一个应用（APP）来获取 API Key。

### 创建步骤

1. 登录用户中心
2. 进入"我的应用"
3. 点击"创建应用"
4. 填写应用信息：
   - 应用名称：给你的应用起个名字
   - 应用描述：简要描述应用用途
5. 点击"创建"

## 第三步：获取 API Key

API Key 用于服务端 API 的身份认证。

### 获取 API Key

1. 进入"我的应用"
2. 选择你创建的应用
3. 在应用详情中找到 **API Key**
4. 点击复制

### API Key 安全注意事项

⚠️ **重要**：

- API Key 仅用于服务端调用，**永远不要在客户端代码中暴露**
- 请妥善保管，不要泄露
- 如需重置，可在应用详情中操作
- 定期轮换 API Key 以提高安全性

## 第四步：获取 User Sign

User Sign（用户签名）是用于 SDK 初始化的 JWT 令牌，包含用户身份和权限信息。

### API 接口

**端点**：`POST /api/v2/browser/getUserSig`

**请求头**：

```http
Authorization: Bearer YOUR_API_KEY
Content-Type: application/json
```

**请求参数**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| customerId | string | 是 | 三方用户唯一ID |
| duration | integer | 否 | 有效期（秒），默认1天，最大30天 |

**请求示例**：

```http
POST https://api.brosdk.com/api/v2/browser/getUserSig
Authorization: Bearer your_api_key_here
Content-Type: application/json

{
  "customerId": "user_12345",
  "duration": 86400
}
```

**响应示例**：

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "userSig": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c"
  }
}
```

### User Sign 使用

获取 User Sign 后，传递给 SDK 进行初始化：

```cpp
const char *init_req =
    "{"
    "  \"userSig\": \"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...\","
    "  \"workDir\": \"C:/brosdk/data\","
    "  \"port\": 9527"
    "}";

char *out = nullptr;
size_t out_len = 0;
sdk_handle_t handle = nullptr;

int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);
```

## 第五步：下载 SDK 和内核

### 核心组件下载

| 组件 | 说明 | 下载地址 |
| --- | --- | --- |
| **SDK** | C++ 动态链接库，核心调用接口 | https://github.com/browsersdk/brosdk-sdk |
| **浏览器内核** | Chromium 定制内核，运行浏览器环境 | https://github.com/browsersdk/brosdk-core |
| **SDK Demo** | 完整的使用示例 | https://github.com/browsersdk/browser-sdk-demo |
| **Go 服务端 SDK** | Go 语言封装，方便服务端集成 | https://github.com/browsersdk/brosdk-server-go |
| **TypeScript SDK** | TypeScript/Node.js封装 | https://github.com/browsersdk/brosdk-sdk-typescript |

**目录结构**：

```plaintext
C:/brosdk/
├── core/                    # 浏览器内核目录
│   ├── brosdk_core.exe     # Windows 内核
│   ├── brosdk_core         # Linux 内核
│   └── ...
└── data/                   # 数据目录（在初始化时指定）
    ├── env1/              # 环境1数据
    ├── env2/              # 环境2数据
    └── ...
```

**安装步骤**：

1. 下载对应平台的内核文件
2. 解压到指定目录（例如：`C:/brosdk/core`）
3. 在 SDK 初始化时指定工作目录

### SDK 初始化示例

```cpp
#include "brosdk.h"

// 初始化 SDK
const char *init_req =
    "{"
    "  \"userSig\": \"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...\","  // 从服务端获取的 User Sign
    "  \"workDir\": \"C:/brosdk/data\","                             // 工作目录（数据存储）
    "  \"port\": 9527"                                               // API 服务端口
    "}";

char *out = nullptr;
size_t out_len = 0;
sdk_handle_t handle = nullptr;

int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);

if (rc == 0) {
    printf("SDK 初始化成功\n");
    printf("环境 ID: %s\n", out);
} else {
    printf("SDK 初始化失败: %s\n", out);
    // 处理错误
}
```

**初始化参数说明**：

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| userSig | string | 是 | 从服务端获取的 User Sign |
| workDir | string | 是 | 工作目录，用于存储环境数据 |
| port | integer | 是 | SDK API 服务的监听端口 |

## 第六步：使用 SDK API 启动环境

### 启动环境流程

```mermaid
graph LR
    A[获取 User Sign] --> B[SDK 初始化]
    B --> C[创建环境]
    C --> D[打开浏览器]
    D --> E[加载页面]
    E --> F[执行操作]
```

### 创建环境并打开浏览器

```cpp
// 创建环境请求
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

char *env_out = nullptr;
size_t env_out_len = 0;

int32_t rc = sdk_env_create(handle, create_env_req, strlen(create_env_req),
                            &env_out, &env_out_len);

if (rc == 0) {
    printf("环境创建成功\n");
    // 解析 env_out 获取 envId
    
    // 打开浏览器
    const char *open_req =
        "{"
        "  \"envId\": \"2034183257439866880\","
        "  \"url\": \"https://www.example.com\""
        "}";
    
    char *open_out = nullptr;
    size_t open_out_len = 0;
    
    rc = sdk_open(handle, open_req, strlen(open_req), &open_out, &open_out_len);
    
    if (rc == 0) {
        printf("浏览器打开成功\n");
    }
}
```

### SDK API 认证

所有 SDK API 都需要使用 User Sign 进行认证：

```http
Authorization: Bearer YOUR_USER_SIGN
```

SDK 内部会自动使用初始化时的 User Sign，无需每次请求都手动设置认证头。

### SDK API 端点

| 功能 | 端点 | 说明 |
| --- | --- | --- |
| 创建环境 | `POST /sdk/v1/env/create` | 创建新的浏览器环境 |
| 更新环境 | `POST /sdk/v1/env/update` | 更新环境配置 |
| 查询环境 | `POST /sdk/v1/env/page` | 查询环境列表 |
| 销毁环境 | `POST /sdk/v1/env/destroy` | 删除环境 |
| 打开浏览器 | `POST /sdk/v1/browser/open` | 打开指定环境 |
| 关闭浏览器 | `POST /sdk/v1/browser/close` | 关闭浏览器 |

## 第七步：管理 User Sign

### Token 过期处理

User Sign 会在指定时间后过期（默认1天）。SDK 会提前通知你 token 即将过期。

#### 过期警告事件

当 User Sign 即将过期时，SDK 会触发事件 `10123`（sdk-token-expire-warning）。你应该立即调用 token 更新 API：

```cpp
static void on_result(int32_t code, void *user_data,
                      const char *data, size_t len) {
    if (sdk_is_event(code) && code == 10123) {
        printf("User Sign 即将过期，正在刷新...\n");
        // 调用你的服务端 API 获取新的 User Sign
        refresh_user_sign();
    }
}
```

#### 更新 Token

**SDK API**：`POST /sdk/v1/token/update`

**请求**：

```json
{
  "userSig": "new_user_sign_here"
}
```

**响应**：

```json
{
  "reqid": 1006901416,
  "code": 0,
  "msg": "ok",
  "data": {
    "eventId": 10121
  }
}
```

**C API**：

```cpp
const char *update_req =
    "{"
    "  \"userSig\": \"new_user_sign_here\""
    "}";

sdk_token_update(update_req, strlen(update_req));
```

### Token 事件说明

| 事件 ID | 名称 | 说明 |
| --- | --- | --- |
| 10123 | sdk-token-expire-warning | Token 即将过期（需立即刷新） |
| 10124 | sdk-token-expired | Token 已过期 |
| 10121 | sdk-token-update-success | Token 更新成功 |
| 10122 | sdk-token-update-failed | Token 更新失败 |

## 完整示例

### 服务端代码示例（Node.js）

```javascript
const axios = require('axios');

// 获取 User Sign
async function getUserSig(customerId, duration = 86400) {
  const response = await axios.post('https://api.brosdk.com/api/v2/browser/getUserSig', {
    customerId,
    duration
  }, {
    headers: {
      'Authorization': `Bearer ${process.env.API_KEY}`,
      'Content-Type': 'application/json'
    }
  });
  
  return response.data.data.userSig;
}

// 使用
getUserSig('user_12345', 86400)
  .then(userSig => console.log('User Sign:', userSig))
  .catch(error => console.error('Error:', error.response.data));
```

### 客户端代码示例（C++）

```cpp
#include "brosdk.h"

// 注册回调
void on_result(int32_t code, void *user_data, const char *data, size_t len) {
    if (sdk_is_event(code)) {
        switch (code) {
            case 10123:
                printf("警告：User Sign 即将过期\n");
                // 通知应用层刷新 token
                break;
            case 10124:
                printf("错误：User Sign 已过期\n");
                break;
        }
    }
}

// 初始化 SDK
void init_brosdk() {
    // 注册回调
    sdk_register_result_cb(on_result, nullptr);
    
    // 从服务端获取 User Sign
    const char *user_sig = get_user_sig_from_server();
    
    // 初始化 SDK
    const char *init_req = 
        "{"
        "  \"userSig\": \"" + std::string(user_sig) + "\","
        "  \"workDir\": \"C:/brosdk/data\","
        "  \"port\": 9527"
        "}";
    
    char *out = nullptr;
    size_t out_len = 0;
    sdk_handle_t handle = nullptr;
    
    int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);
    
    if (rc == 0) {
        printf("SDK 初始化成功\n");
    } else {
        printf("SDK 初始化失败: %s\n", out);
    }
}
```

## 常见错误

### 认证相关错误

| 错误码 | 说明 | 解决方法 |
| --- | --- | --- |
| 401 | 未登录或 Token 无效/过期 | 获取新的 User Sign |
| 10011 | 账号已注册 | 直接登录 |
| 10012 | 密码错误 | 检查密码或使用验证码登录 |
| 10013 | 用户名或密码无效 | 检查登录信息 |
| 10014 | 用户不存在 | 先注册账号 |
| 10015 | 用户被禁用 | 联系客服 |
| 1027 | 账号不存在 | 请先注册 |
| 1028 | 账号被冻结 | 联系客服 |
| 1029 | 未设置密码 | 使用验证码登录 |

### 应用相关错误

| 错误码 | 说明 | 解决方法 |
| --- | --- | --- |
| 10300 | 应用不存在 | 检查应用 ID |
| 10301 | 应用 Key 无效 | 检查 API Key |
| 10302 | 应用状态异常 | 联系客服 |
| 10303 | 应用配额已用完 | 升级套餐或等待配额恢复 |

### Token 相关错误

| 错误码 | 说明 | 解决方法 |
| --- | --- | --- |
| 10122 | Token 更新失败 | 检查新的 User Sign 是否有效 |
| 10124 | Token 已过期 | 获取新的 User Sign |

## 最佳实践

1. **API Key 安全**
   - 只在服务端使用 API Key
   - 使用环境变量存储 API Key
   - 定期轮换 API Key

2. **User Sign 管理**
   - 监听 token 过期事件并主动刷新
   - 临时缓存 User Sign 以减少 API 调用
   - 在可能的范围内提前刷新 token

3. **错误处理**
   - 优雅处理所有 API 错误
   - 记录错误日志用于调试
   - 实现重试机制处理临时性错误

4. **性能优化**
   - 批量获取 User Sign 以减少请求次数
   - 使用合适的数据有效期（duration）
   - 避免频繁刷新 token

## 下一步

- [环境管理](user-guide/environment.md) - 学习如何创建和管理浏览器环境
- [服务端 API 参考](api/server.md) - 查看完整的服务端 API 文档
- [SDK API 参考](api/sdk.md) - 查看完整的 SDK API 文档
- [原生 C 集成指南](integration/c-native.md) - 学习如何集成 SDK

## 相关资源

- **官网**：https://www.brosdk.com
- **SDK 下载**：https://github.com/browsersdk/brosdk-sdk
- **浏览器内核**：https://github.com/browsersdk/brosdk-core
- **SDK Demo**：https://github.com/browsersdk/browser-sdk-demo
- **Go 服务端 SDK**：https://github.com/browsersdk/brosdk-server-go
- **TypeScript SDK**：https://github.com/browsersdk/brosdk-sdk-typescript
