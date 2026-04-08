# 服务端 API 参考

BroSDK 服务端 API 完整参考文档。

## 概述

服务端 API 提供浏览器环境、用户和应用管理的后端服务。

**基础 URL**：`https://api.brosdk.com`

**Content-Type**：`application/json`

**API 认证**：所有 API 都需要 `Authorization: Bearer YOUR_API_KEY` 请求头

## 响应格式

所有 API 返回统一的 JSON 格式：

```json
{
  "code": 200,
  "msg": "OK",
  "data": null
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| code | int | 状态码（200 = 成功） |
| msg | string | 响应消息 |
| data | object | 响应数据 |

---

## 认证相关

### 获取 User Sign

获取用户签名（JWT 令牌）用于 SDK 身份认证。

**端点**：`POST /api/v2/browser/getUserSig`

**认证**：需要（API Key）

```http
Authorization: Bearer YOUR_API_KEY
```

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| customerId | string | 是 | 三方用户唯一 ID |
| duration | integer | 否 | 有效期（秒），默认 86400（1 天），最大 2592000（30 天） |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/browser/getUserSig
Authorization: Bearer your_api_key_here
Content-Type: application/json

{
  "customerId": "user_12345",
  "duration": 86400
}
```

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "userSig": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...",
    "expireTime": 1769304899
  }
}
```

#### 响应字段

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| userSig | string | JWT 令牌，用于 SDK 初始化 |
| expireTime | integer | 过期时间戳（秒） |

---

## 环境管理

### 创建环境

创建新的浏览器环境（指纹）。

**端点**：`POST /api/v2/browser/create`

**认证**：需要（API Key）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envName | string | 否 | 环境名称 |
| remark | string | 否 | 备注/描述 |
| serial | string | 否 | 环境序号 |
| customerId | string | 是 | 三方用户 ID |
| region | string | 否 | 国家代号（如 US, CN） |
| proxy | string | 否 | 代理配置，格式：`socks5://user:pwd@ipaddr:port` |
| ipChannel | string | 否 | IP 监测渠道：`ip2location`（海外）或 `ipdata`（国内） |
| finger | object | 是 | 浏览器指纹配置（见下方） |

#### 指纹配置详解

```json
{
  "system": "Windows 11",
  "kernel": "Chrome",
  "kernelVersion": "148",
  "ua": "",
  "language": [],
  "zone": "",
  "geographic": {
    "enable": 1,
    "useip": 1,
    "longitude": "",
    "latitude": "",
    "accuracy": ""
  },
  "dpi": "default",
  "fontFinger": 1,
  "font": {
    "enable": 1,
    "list": []
  },
  "webRTC": 5,
  "webRTCIP": "",
  "canvas": 4,
  "webGl": 1,
  "webGlInfo": 2,
  "webGlVendor": "",
  "webGlRenderer": "",
  "audioContext": 1,
  "speechVoices": 2,
  "mediaDevice": 2,
  "cpu": 4,
  "mem": 8,
  "deviceName": "",
  "mac": "",
  "hardware": 1,
  "bluetooth": 2,
  "doNotTrack": 2,
  "enableScanPort": 1,
  "scanPort": "",
  "enableOpen": 1,
  "enableNotice": 1,
  "enablePic": 2,
  "picSize": "",
  "enableGc": 2,
  "gcTime": 1,
  "battery": 1,
  "enableSound": 2,
  "enableVideo": 2,
  "searchEngine": 1,
  "translateLang": 1,
  "enableStorage": 1,
  "enableDevtools": 2,
  "clearCookie": 2,
  "clearStorage": 2,
  "shortName": 2
}
```

#### 指纹配置说明

| 配置项 | 可选值 | 说明 |
| --- | --- | --- |
| system | Windows 10, Windows 11, macOS, Linux | 操作系统 |
| kernel | Chrome, Firefox, Edge | 浏览器内核 |
| kernelVersion | 120-148 | 内核版本 |
| webRTC | 1=真实，2=替换，5=禁用 | WebRTC 配置 |
| canvas | 1=真实，2=随机，4=一致性，3=关闭 | Canvas 指纹 |
| webGl | 1=隐身，2=真实 | WebGL 指纹 |
| audioContext | 1=隐身，2=真实 | 音频上下文 |
| fontFinger | 1=隐身，2=真实 | 字体指纹 |
| mediaDevice | 1=关闭，2=启用（噪声） | 媒体设备 |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/browser/create
Authorization: Bearer your_api_key_here
Content-Type: application/json

{
  "envName": "测试环境",
  "remark": "用于测试",
  "customerId": "user_12345",
  "region": "US",
  "proxy": "socks5://user:pass@proxy.com:1080",
  "ipChannel": "ip2location",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148",
    "webRTC": 5,
    "canvas": 4,
    "webGl": 1
  }
}
```

#### 响应示例

```json
{
  "reqId": "12de063e-a39d-4c70-953c-90c026229d0c",
  "code": 200,
  "msg": "OK",
  "data": {
    "envId": "2034183257439866880",
    "customerId": "user_12345",
    "envName": "测试环境",
    "serial": "",
    "proxy": "socks5://user:pass@proxy.com:1080",
    "ipChannel": "ip2location",
    "region": "US",
    "finger": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148",
      "ua": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.7741.0 Safari/537.36",
      "webRTC": 5,
      "webRTCIP": "192.168.88.12",
      "canvas": 4,
      "webGl": 1,
      "webGlVendor": "Google Inc. (Intel)",
      "webGlRenderer": "ANGLE (Intel, Intel(R) HD Graphics 4600)",
      "audioContext": 1,
      "cpu": 4,
      "mem": 8,
      "deviceName": "DESKTOP-OiDAYHW",
      "mac": "08-9E-01-AA-D7-62"
    }
  }
}
```

---

### 更新环境

更新现有浏览器环境配置。

**端点**：`POST /api/v2/browser/update`

**认证**：需要（API Key）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |
| envName | string | 否 | 环境名称 |
| remark | string | 否 | 备注 |
| proxy | string | 否 | 代理配置 |
| finger | object | 否 | 指纹配置 |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/browser/update
Authorization: Bearer your_api_key_here
Content-Type: application/json

{
  "envId": "2034183257439866880",
  "envName": "更新后的名称",
  "proxy": "socks5://user:pass@new-proxy.com:1080"
}
```

#### 响应

返回更新后的完整环境信息（格式与创建相同）。

---

### 查询环境列表

分页查询环境列表。

**端点**：`POST /api/v2/browser/page`

**认证**：需要（API Key）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| customerId | string | 否 | 客户 ID 过滤 |
| envIds | array | 否 | 环境 ID 列表过滤 |
| pageIndex | int | 否 | 页码索引（从 0 开始） |
| pageSize | int | 否 | 每页大小（默认 10，最大 100） |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/browser/page
Authorization: Bearer your_api_key_here
Content-Type: application/json

{
  "customerId": "user_12345",
  "pageIndex": 0,
  "pageSize": 10
}
```

#### 响应示例

```json
{
  "reqId": "629f445c-7373-4b3f-ad56-be74127a1889",
  "code": 200,
  "msg": "OK",
  "data": {
    "list": [
      {
        "envId": "2029117313256525824",
        "customerId": "user_12345",
        "envName": "测试环境",
        "serial": "001",
        "proxy": "socks5://user:pass@proxy.com:1080",
        "region": "US",
        "finger": {
          "system": "Windows 10",
          "kernel": "Chrome",
          "kernelVersion": "127"
        }
      }
    ],
    "total": 1,
    "pageSize": 10,
    "currentPage": 1
  }
}
```

---

### 销毁环境

删除指定的浏览器环境及关联的持久化数据。

**端点**：`POST /api/v2/browser/destroy`

**认证**：需要（API Key）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| envId | string | 是 | 环境 ID |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/browser/destroy
Authorization: Bearer your_api_key_here
Content-Type: application/json

{
  "envId": "2034183257439866880"
}
```

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": null
}
```

---

### 获取全局指纹配置

获取指纹全局配置项。

**端点**：`POST /api/v2/browser/getGlobalFinger`

**认证**：需要（API Key）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| appId | string | 是 | 应用 ID |

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "globalFingerConfig": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148",
      "webRTC": 5,
      "canvas": 4,
      "webGl": 1
    }
  }
}
```

---

### 设置全局指纹配置

设置全局指纹配置。

**端点**：`POST /api/v2/browser/setGlobalFinger`

**认证**：需要（API Key）

#### 请求参数

全局指纹配置对象（同创建环境的 finger 配置）。

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "globalFingerConfig": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148"
    }
  }
}
```

---

## 应用管理

### 创建应用

**端点**：`POST /api/v2/user/app/create`

**认证**：需要（User Sign）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| appName | string | 是 | 应用名称 |
| logo | string | 否 | 图标 URL |
| mode | integer | 是 | 1=自托管，2=全托管 |
| config | object | 否 | 应用配置 |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/user/app/create
Authorization: Bearer YOUR_USER_SIGN
Content-Type: application/json

{
  "appName": "我的应用",
  "mode": 2,
  "config": {
    "autoDownloadKernel": true,
    "autoUpdateKernel": true,
    "storage": true
  }
}
```

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "id": "app_123456",
    "appName": "我的应用",
    "appKey": "ak_xxxxxxxxxxxx",
    "appSecret": "as_xxxxxxxxxxxx",
    "status": 3,
    "mode": 2
  }
}
```

---

### 获取应用列表

**端点**：`POST /api/v2/user/app/page`

**认证**：需要（User Sign）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| page | int | 否 | 页码（默认 1） |
| pageSize | int | 否 | 每页大小（默认 10） |
| status | int | 否 | 状态过滤 |

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "list": [
      {
        "id": "app_123456",
        "appName": "我的应用",
        "appKey": "ak_xxxxxxxxxxxx",
        "status": 3,
        "mode": 2,
        "createdAt": "2026-03-25T10:00:00Z"
      }
    ],
    "total": 1,
    "page": 1,
    "pageSize": 10
  }
}
```

---

### 重置 API Key

**端点**：`POST /api/v2/user/app/apikey/reset`

**认证**：需要（User Sign）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| id | string | 是 | 应用 ID |

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "id": "app_123456",
    "appKey": "ak_newkeyxxxxxxxx",
    "appSecret": "as_newsecretxxxxxxxx"
  }
}
```

---

## 用户管理

### 用户登录

**端点**：`POST /api/v2/user/login`

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| loginType | integer | 是 | 0=用户名，1=手机号，2=邮箱，3=钉钉 |
| loginValue | string | 是 | 登录标识 |
| password | string | 否 | 密码（loginType=0 时必填） |
| verifyCode | string | 否 | 验证码（手机/邮箱登录时必填） |

#### 请求示例

```http
POST https://api.brosdk.com/api/v2/user/login
Content-Type: application/json

{
  "loginType": 0,
  "loginValue": "admin",
  "password": "password123"
}
```

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "token": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...",
    "user": {
      "id": "user_12345",
      "username": "admin",
      "email": "admin@example.com"
    }
  }
}
```

---

### 获取用户信息

**端点**：`POST /api/v2/user/info`

**认证**：需要（User Sign）

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "id": "user_12345",
    "username": "admin",
    "email": "admin@example.com",
    "mobile": "13800138000",
    "account": {
      "balance": "100.00",
      "creditLimit": "1000.00",
      "frozenBalance": "0.00"
    }
  }
}
```

---

## 订单与支付

### 获取充值套餐列表

**端点**：`POST /api/v2/user/recharge-packages/page`

**认证**：需要（User Sign）

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "list": [
      {
        "id": 1,
        "name": "基础套餐",
        "price": "99.00",
        "credit": "100.00",
        "description": "适合个人开发者"
      },
      {
        "id": 2,
        "name": "专业套餐",
        "price": "499.00",
        "credit": "550.00",
        "description": "适合小型团队"
      }
    ],
    "total": 2
  }
}
```

---

### 创建订单

**端点**：`POST /api/v2/user/order/create`

**认证**：需要（User Sign）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| packageId | integer | 是 | 充值套餐 ID |
| payType | string | 是 | 支付方式：wx（微信）, zfb（支付宝） |

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "orderNo": "ORD202603250001",
    "actAmount": "99.00",
    "planAmount": "100.00",
    "payType": "wx",
    "orderStatus": 1,
    "qrCode": "data:image/png;base64,..."
  }
}
```

---

### 查询订单

**端点**：`POST /api/v2/user/order/get`

**认证**：需要（User Sign）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| orderNo | string | 是 | 订单编号 |

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "orderNo": "ORD202603250001",
    "actAmount": "99.00",
    "planAmount": "100.00",
    "orderStatus": 2,
    "payTime": "2026-03-25T10:30:00Z"
  }
}
```

---

## 统计信息

### 获取用户统计

**端点**：`POST /api/v2/user/user-total-stats/get`

**认证**：需要（User Sign）

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "envCreateCount": 50,
    "envStartCount": 200,
    "trafficBytes": 1073741824,
    "appCreateCount": 3
  }
}
```

---

### 获取用户日统计

**端点**：`POST /api/v2/user/user-daily-stats/page`

**认证**：需要（User Sign）

#### 请求参数

| 参数 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| startTime | string | 否 | 开始时间（ISO 8601） |
| endTime | string | 否 | 结束时间（ISO 8601） |
| page | int | 否 | 页码 |
| pageSize | int | 否 | 每页大小 |

#### 响应示例

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "list": [
      {
        "statDate": "2026-03-25",
        "envCreateCount": 5,
        "envStartCount": 20,
        "trafficBytes": 107374182
      }
    ],
    "total": 1
  }
}
```

---

## 错误码

### 通用状态码

| 错误码 | 常量 | 说明 |
| --- | --- | --- |
| 200 | SUCCESS | 成功 |
| 500 | FAILURE | 失败 |
| 400 | BadRequest | 错误的请求 |
| 401 | InvalidToken | 未登录/无效令牌 |
| 403 | AuthorizationError | 无权限 |
| 404 | NotFound | 资源未找到 |
| 429 | RequestsFrequently | 请求过于频繁 |

### 用户相关错误

| 错误码 | 常量 | 说明 |
| --- | --- | --- |
| 1027 | UserNotExist | 账号不存在，请先注册 |
| 1028 | UserLock | 账号被冻结 |
| 1029 | PwdNotExist | 未设置密码，请使用验证码登录 |
| 10011 | ErrUserExist | 账号已注册 |
| 10012 | ErrPwd | 密码错误 |
| 10013 | ErrUsernameOrPwd | 用户名或密码错误 |
| 10014 | UserNotFound | 用户不存在 |
| 10015 | UserDisabled | 用户被禁用 |

### 环境相关错误

| 错误码 | 常量 | 说明 |
| --- | --- | --- |
| 10700 | EnvNotFound | 环境不存在 |
| 10701 | EnvQuotaExceeded | 环境配额超限 |
| 10702 | EnvStatusAbnormal | 环境状态异常 |
| 10703 | EnvNameExist | 环境名称已存在 |
| 10704 | EnvCookieInvalid | 环境 Cookie 无效 |

### 应用相关错误

| 错误码 | 常量 | 说明 |
| --- | --- | --- |
| 10300 | AppNotFound | 应用不存在 |
| 10301 | AppKeyInvalid | 应用 Key 无效 |
| 10302 | AppStatusAbnormal | 应用状态异常 |
| 10303 | AppQuotaExceeded | 应用配额已用完 |
| 10304 | AppNameExist | 应用名称已存在 |

### Token 相关错误

| 错误码 | 常量 | 说明 |
| --- | --- | --- |
| 10122 | TokenUpdateFailed | Token 更新失败 |
| 10123 | TokenExpireWarning | Token 即将过期（警告） |
| 10124 | TokenExpired | Token 已过期 |

---

## 对应的 SDK API

上述环境管理 API 都有对应的 SDK API：

| 服务端 API | SDK API | 说明 |
| --- | --- | --- |
| `/api/v2/browser/create` | `/sdk/v1/env/create` | 创建环境 |
| `/api/v2/browser/update` | `/sdk/v1/env/update` | 更新环境 |
| `/api/v2/browser/destroy` | `/sdk/v1/env/destroy` | 销毁环境 |
| `/api/v2/browser/page` | `/sdk/v1/env/page` | 环境列表 |
| `/api/v2/browser/getUserSig` | - | 仅服务端 API |

---

## 最佳实践

1. **API Key 安全**
   - 只在服务端使用 API Key
   - 使用环境变量存储
   - 定期轮换

2. **错误处理**
   - 检查所有 API 返回的 code 值
   - 实现重试机制处理临时性错误
   - 记录错误日志用于调试

3. **Token 管理**
   - 监听 Token 过期事件（10123）
   - 提前刷新 Token（建议在过期前 1 小时）
   - 缓存 Token 减少 API 调用

4. **配额管理**
   - 定期检查环境配额
   - 清理不用的环境
   - 监控使用量避免超限

---

## 相关文档

- [原生 C 集成指南](../integration/c-native.md) - 如何集成 SDK
- [快速开始](../quick-start.md) - 快速上手
- [环境管理](../user-guide/environment.md) - 环境管理指南
