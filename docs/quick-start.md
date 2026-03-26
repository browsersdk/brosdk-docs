# 快速开始

欢迎使用 BroSDK！本指南将帮助你快速上手并开始使用浏览器环境管理服务。

## 简介

BroSDK 是一个基于 C++ 高性能浏览器环境管理和自动化平台，提供浏览器指纹管理、代理调度、环境隔离等核心功能。

### 核心组件

| 组件 | 说明 |
|------|------|
| **SDK** | C++ 动态库（`brosdk.dll` / `brosdk.so` / `brosdk.dylib`） |
| **浏览器内核** | 基于 Chromium 的定制浏览器内核 |
| **服务端** | 提供 API Key 认证和 User Sign 颁发 |

### 快速流程概览

```mermaid
graph TD
    A[注册账号] --> B[创建应用]
    B --> C[获取 API Key]
    C --> D[服务端：用 API Key 换取 User Sign]
    D --> E[初始化 SDK]
    E --> F[调用 SDK API 操作浏览器]
```

---

## SDK 安装

### 1. 下载 SDK

从 GitHub 下载对应平台的 SDK：

| 平台 | 架构 | 状态 |
|------|------|------|
| Windows | x64 | ✅ 已发布 |
| Linux | x64 | ✅ 已发布 |
| macOS | x64/arm64 | 🚧 开发中 |

**下载地址**：[https://github.com/browsersdk/brosdk-sdk](https://github.com/browsersdk/brosdk-sdk)

### 2. 下载浏览器内核

浏览器内核需要单独下载并放置到指定目录：

**下载地址**：[https://github.com/browsersdk/brosdk-core](https://github.com/browsersdk/brosdk-core)

### 3. 目录结构

```plaintext
C:/brosdk/
├── cores/                   # 浏览器内核目录（必需）
│   ├── YunBrowser119-1.0.1.9
│   ├── YunBrowser140-1.0.0.1
│   └── ...
├── userdata/                # Chrome 用户数据目录（自动创建）
│   ├── env1/                # 环境 1 数据
│   ├── env2/                # 环境 2 数据
│   └── ...
└── data/                    # 导出数据目录（BroSDK 自动从 userdata 导出）
```

**重要**：
- 下载的浏览器内核需要放在 `workDir/cores/` 目录下
- `userdata/` 和 `data/` 目录会在首次运行时自动创建
- `userdata/` 存储 Chrome 原始用户数据
- `data/` 是 BroSDK 从 userdata 导出的数据

---

## 身份验证配置

### 第一步：注册账号

访问 [BroSDK 用户中心](https://www.brosdk.com) 完成用户注册。

**注册流程**：
1. 访问官网用户中心
2. 输入**手机号**或**邮箱**
3. 获取并填写验证码
4. 完成登录

> 💡 **无需预先注册，无需设置密码**  
> 如果是首次使用，系统会自动注册账号。后续登录只需输入手机号/邮箱 + 验证码即可。

### 第二步：创建应用

注册完成后，创建一个应用（APP）来获取 API Key：

1. 登录用户中心
2. 进入"我的应用"
3. 点击"创建应用"
4. 填写应用信息（名称、描述）
5. 点击"创建"

### 第三步：获取 API Key

API Key 用于服务端 API 的身份认证：

1. 进入"我的应用"
2. 选择你创建的应用
3. 在应用详情中找到 **API Key**
4. 点击复制

⚠️ **重要**：API Key 仅用于服务端调用，**永远不要在客户端代码中暴露**

### 第四步：获取 User Sign

User Sign 是用于 SDK 初始化的 JWT 令牌。

**API 接口**：`POST /api/v2/browser/getUserSig`

**请求示例**：
```http
POST https://api.brosdk.com/api/v2/browser/getUserSig
Authorization: Bearer YOUR_API_KEY
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
    "userSig": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9..."
  }
}
```

### 第五步：初始化 SDK

```cpp
#include "brosdk.h"

const char *init_req =
    "{"
    "  \"userSig\": \"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...\","
    "  \"workDir\": \"C:/brosdk/\","
    "  \"port\": 9527"
    "}";

char *out = nullptr;
size_t out_len = 0;
sdk_handle_t handle = nullptr;

int32_t rc = sdk_init(&handle, init_req, strlen(init_req), &out, &out_len);

if (rc == 0) {
    printf("SDK 初始化成功\n");
} else {
    printf("SDK 初始化失败：%s\n", out);
}
```

**初始化参数**：

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| userSig | string | 是 | 从服务端获取的 User Sign |
| workDir | string | 是 | 工作目录（内核和数据的根目录） |
| port | integer | 是 | SDK API 服务的监听端口 |

---

## 核心 API

### 浏览器实例 (Browser)

#### 创建环境

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

char *env_out = nullptr;
size_t env_out_len = 0;

int32_t rc = sdk_env_create(handle, create_env_req, strlen(create_env_req),
                            &env_out, &env_out_len);
```

#### 打开浏览器

```cpp
const char *open_req =
    "{"
    "  \"envId\": \"2034183257439866880\","
    "  \"url\": \"https://www.example.com\""
    "}";

char *open_out = nullptr;
size_t open_out_len = 0;

rc = sdk_open(handle, open_req, strlen(open_req), &open_out, &open_out_len);
```

#### 关闭浏览器

```cpp
const char *close_req =
    "{"
    "  \"envId\": \"2034183257439866880\""
    "}";

rc = sdk_close(handle, close_req, strlen(close_req), &open_out, &open_out_len);
```

### 指纹配置 (Profile)

在创建环境时配置浏览器指纹：

```json
{
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148",
    "platform": "Win32",
    "language": "zh-CN",
    "timezone": "Asia/Shanghai",
    "screen": {
      "width": 1920,
      "height": 1080
    }
  }
}
```

**支持的指纹参数**：

| 参数 | 说明 | 示例 |
|------|------|------|
| system | 操作系统 | Windows 11, macOS 14, Linux |
| kernel | 浏览器内核 | Chrome, Firefox, Edge |
| kernelVersion | 内核版本 | 148, 140, 119 |
| platform | 平台标识 | Win32, MacIntel, Linux x86_64 |
| language | 语言 | zh-CN, en-US, ja-JP |
| timezone | 时区 | Asia/Shanghai, America/New_York |
| screen | 屏幕分辨率 | {width: 1920, height: 1080} |

### 代理调度 (Proxy)

支持多种代理协议：

```json
{
  "proxy": "socks5://user:pass@proxy.example.com:1080"
}
```

**支持的代理协议**：
- `http://host:port`
- `https://host:port`
- `socks5://host:port`
- `socks5://user:pass@host:port`

---

## 进阶使用

### Token 管理

User Sign 会在指定时间后过期（默认 1 天）。SDK 会提前通知 token 即将过期。

#### 监听过期事件

```cpp
static void on_result(int32_t code, void *user_data,
                      const char *data, size_t len) {
    if (sdk_is_event(code) && code == 10123) {
        printf("User Sign 即将过期，正在刷新...\n");
        // 调用服务端 API 获取新的 User Sign
        refresh_user_sign();
    }
}

// 注册回调
sdk_register_result_cb(on_result, nullptr);
```

#### 更新 Token

```cpp
const char *update_req =
    "{"
    "  \"userSig\": \"new_user_sign_here\""
    "}";

sdk_token_update(update_req, strlen(update_req));
```

### 错误处理

**常见错误码**：

| 错误码 | 说明 | 解决方法 |
|--------|------|----------|
| 401 | Token 无效/过期 | 获取新的 User Sign |
| 10301 | API Key 无效 | 检查 API Key |
| 10303 | 应用配额已用完 | 升级套餐或等待配额恢复 |
| 10122 | Token 更新失败 | 检查新的 User Sign 是否有效 |

---

## 部署方案

### 本地开发

适用于个人开发和测试：

1. 下载 SDK 和内核到本地
2. 配置 `workDir` 为本地路径
3. 直接调用 SDK API

### 服务器部署

适用于生产环境：

1. 在服务器安装 SDK
2. 使用环境变量存储 API Key
3. 实现自动化的 Token 刷新机制
4. 配置日志和监控

### 容器化部署

使用 Docker 部署：

```dockerfile
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    libxcb1 libxkbcommon-x11-0 libxcb-icccm4 \
    libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
    libxcb-render-util0 libxcb-xinerama0 \
    libxcb-xfixes0 libx11-xcb1

# 复制 SDK 和内核
COPY brosdk-sdk /opt/brosdk/
COPY brosdk-core /opt/brosdk/cores/

# 设置工作目录
ENV BROSDK_WORKDIR=/opt/brosdk/

CMD ["/opt/brosdk/app"]
```

---

## 相关资源

| 资源 | 链接 | 说明 |
|------|------|------|
| 🌐 官网 | [brosdk.com](https://www.brosdk.com) | 官方网站 |
| 📦 C++ SDK | [github.com/browsersdk/brosdk-sdk](https://github.com/browsersdk/brosdk-sdk) | 核心动态库（必需） |
| 📘 TypeScript SDK | [github.com/browsersdk/brosdk-sdk-typescript](https://github.com/browsersdk/brosdk-sdk-typescript) | C++ SDK 的 TS 封装 |
| 🔧 浏览器内核 | [github.com/browsersdk/brosdk-core](https://github.com/browsersdk/brosdk-core) | Chromium 内核 |
| 📖 SDK Demo | [github.com/browsersdk/browser-sdk-demo](https://github.com/browsersdk/browser-sdk-demo) | 示例代码 |
| 🚀 Go 服务端 SDK | [github.com/browsersdk/brosdk-server-go](https://github.com/browsersdk/brosdk-server-go) | 服务端 API 封装 |

---

## 下一步

- [环境管理](user-guide/environment.md) - 学习如何创建和管理浏览器环境
- [服务端 API 参考](api/server.md) - 查看完整的服务端 API 文档
- [SDK API 参考](api/sdk.md) - 查看完整的 SDK API 文档
- [原生 C 集成指南](integration/c-native.md) - 学习如何集成 SDK
