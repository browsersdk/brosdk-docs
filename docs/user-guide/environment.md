# 环境管理

BroSDK 使用 `envId`（64位整数）管理浏览器环境。每个环境都有独立的 cookies、历史记录、本地存储等持久化数据。

## 创建环境

### 使用服务端 API

**端点**：`POST /api/v2/browser/create`

**认证**：需要（API Key）

```http
Authorization: Bearer YOUR_API_KEY
```

**请求参数**：

```json
{
  "envName": "测试环境",
  "remark": "用于测试",
  "serial": "001",
  "customerId": "user_12345",
  "region": "US",
  "proxy": "socks5://username:password@proxy:port",
  "bridgeProxy": "",
  "ipChannel": "ip2location",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148",
    "uaVersion": "148",
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
}
```

**响应**：

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
    "finger": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148",
      "uaVersion": "148",
      "ua": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.7741.0 Safari/537.36",
      "language": [],
      "zone": "",
      "geographic": {
        "enable": 1,
        "user": 1,
        "longitude": "",
        "latitude": "",
        "accuracy": ""
      },
      "dpi": "default",
      "font": {
        "enable": 1,
        "list": []
      },
      "fontFinger": 1,
      "clientRects": 1,
      "webRTC": 5,
      "webRTCIP": "192.168.88.12",
      "canvas": 4,
      "webGl": 1,
      "webGlInfo": 2,
      "webGlVendor": "Google Inc. (Intel)",
      "webGlRenderer": "ANGLE (Intel, Intel(R) HD Graphics 4600 Direct3D9Ex vs_3_0 ps_3_0, nvumdshimx.dll-10.18.13.5891)",
      "audioContext": 1,
      "speechVoices": 2,
      "mediaDevice": 2,
      "cpu": 4,
      "mem": 8,
      "deviceName": "DESKTOP-OiDAYHW",
      "mac": "08-9E-01-AA-D7-62",
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
  }
}
```

### 使用 SDK API

**端点**：`POST /sdk/v1/env/create`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
```

**请求参数**：与服务端 API 完全相同

```json
{
  "envName": "测试环境",
  "remark": "用于测试",
  "serial": "001",
  "customerId": "user_12345",
  "region": "US",
  "proxy": "socks5://username:password@proxy:port",
  "bridgeProxy": "",
  "ipChannel": "ip2location",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "148",
    "uaVersion": "148",
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
}
```

**响应**：与服务端 API 完全相同

```json
{
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2034183257439866880",
    "customerId": "user_12345",
    "envName": "测试环境",
    "serial": "",
    "proxy": "socks5://user:pass@proxy.com:1080",
    "finger": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148",
      "uaVersion": "148",
      "ua": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.7741.0 Safari/537.36",
      "language": [],
      "zone": "",
      "geographic": {
        "enable": 1,
        "user": 1,
        "longitude": "",
        "latitude": "",
        "accuracy": ""
      },
      "dpi": "default",
      "font": {
        "enable": 1,
        "list": []
      },
      "fontFinger": 1,
      "clientRects": 1,
      "webRTC": 5,
      "webRTCIP": "192.168.88.12",
      "canvas": 4,
      "webGl": 1,
      "webGlInfo": 2,
      "webGlVendor": "Google Inc. (Intel)",
      "webGlRenderer": "ANGLE (Intel, Intel(R) HD Graphics 4600 Direct3D9Ex vs_3_0 ps_3_0, nvumdshimx.dll-10.18.13.5891)",
      "audioContext": 1,
      "speechVoices": 2,
      "mediaDevice": 2,
      "cpu": 4,
      "mem": 8,
      "deviceName": "DESKTOP-OiDAYHW",
      "mac": "08-9E-01-AA-D7-62",
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
  }
}
```

---

## 更新环境

### 使用服务端 API

**端点**：`POST /api/v2/browser/update`

**认证**：需要（API Key）

```http
Authorization: Bearer YOUR_API_KEY
```

**请求参数**：

```json
{
  "envId": "env_id_to_update",
  "customerId": "",
  "envName": "",
  "remark": "",
  "serial": "",
  "region": "",
  "proxy": "",
  "bridgeProxy": "",
  "ipChannel": "",
  "finger": {}
}
```

**响应**：返回更新后的环境信息。

### 使用 SDK API

**端点**：`POST /sdk/v1/env/update`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
```

**请求参数**：与服务端 API 完全相同

```json
{
  "envId": "env_id_to_update",
  "customerId": "",
  "envName": "",
  "remark": "",
  "serial": "",
  "region": "",
  "proxy": "",
  "bridgeProxy": "",
  "ipChannel": "",
  "finger": {}
}
```

**响应**：与服务端 API 完全相同

```json
{
  "code": 0,
  "msg": "ok",
  "data": {
    "envId": "2034183257439866880",
    "customerId": "user_12345",
    "envName": "测试环境",
    "serial": "",
    "proxy": "socks5://user:pass@proxy.com:1080",
    "finger": {
      "system": "Windows 11",
      "kernel": "Chrome",
      "kernelVersion": "148",
      "uaVersion": "148",
      "ua": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.7741.0 Safari/537.36",
      "language": [],
      "zone": "",
      "geographic": {
        "enable": 1,
        "user": 1,
        "longitude": "",
        "latitude": "",
        "accuracy": ""
      },
      "dpi": "default",
      "font": {
        "enable": 1,
        "list": []
      },
      "fontFinger": 1,
      "clientRects": 1,
      "webRTC": 5,
      "webRTCIP": "192.168.88.12",
      "canvas": 4,
      "webGl": 1,
      "webGlInfo": 2,
      "webGlVendor": "Google Inc. (Intel)",
      "webGlRenderer": "ANGLE (Intel, Intel(R) HD Graphics 4600 Direct3D9Ex vs_3_0 ps_3_0, nvumdshimx.dll-10.18.13.5891)",
      "audioContext": 1,
      "speechVoices": 2,
      "mediaDevice": 2,
      "cpu": 4,
      "mem": 8,
      "deviceName": "DESKTOP-OiDAYHW",
      "mac": "08-9E-01-AA-D7-62",
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
  }
}
```

---

## 查询环境列表

### 使用服务端 API

**端点**：`POST /api/v2/browser/page`

**认证**：需要（API Key）

```http
Authorization: Bearer YOUR_API_KEY
```

**请求参数**：

```json
{
  "customerId": "",
  "envIds": [""],
  "pageIndex": 0,
  "pageSize": 10
}
```

**响应**：

```json
{
  "reqId": "629f445c-7373-4b3f-ad56-be74127a1889",
  "code": 200,
  "msg": "OK",
  "data": {
    "list": [],
    "total": 1,
    "pageSize": 10,
    "currentPage": 1
  }
}
```

### 使用 SDK API

**端点**：`POST /sdk/v1/env/page`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
```

**请求参数**：与服务端 API 完全相同

```json
{
  "customerId": "",
  "envIds": [""],
  "pageIndex": 0,
  "pageSize": 10
}
```

**响应**：与服务端 API 完全相同

```json
{
  "code": 0,
  "msg": "ok",
  "data": {
    "list": [
      {
        "envId": "2034183257439866880",
        "customerId": "user_12345",
        "envName": "测试环境",
        "serial": "",
        "proxy": "socks5://user:pass@proxy.com:1080",
        "finger": {
          "system": "Windows 11",
          "kernel": "Chrome",
          "kernelVersion": "148",
          "uaVersion": "148",
          "ua": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.7741.0 Safari/537.36",
          "language": [],
          "zone": "",
          "geographic": {
            "enable": 1,
            "user": 1,
            "longitude": "",
            "latitude": "",
            "accuracy": ""
          },
          "dpi": "default",
          "font": {
            "enable": 1,
            "list": []
          },
          "fontFinger": 1,
          "clientRects": 1,
          "webRTC": 5,
          "webRTCIP": "192.168.88.12",
          "canvas": 4,
          "webGl": 1,
          "webGlInfo": 2,
          "webGlVendor": "Google Inc. (Intel)",
          "webGlRenderer": "ANGLE (Intel, Intel(R) HD Graphics 4600 Direct3D9Ex vs_3_0 ps_3_0, nvumdshimx.dll-10.18.13.5891)",
          "audioContext": 1,
          "speechVoices": 2,
          "mediaDevice": 2,
          "cpu": 4,
          "mem": 8,
          "deviceName": "DESKTOP-OiDAYHW",
          "mac": "08-9E-01-AA-D7-62",
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
      }
    ],
    "total": 1,
    "pageSize": 10,
    "currentPage": 1
  }
}
```

---

## 销毁环境

### 使用服务端 API

**端点**：`POST /api/v2/browser/destroy`

**认证**：需要（API Key）

```http
Authorization: Bearer YOUR_API_KEY
```

**请求参数**：

```json
{
  "envId": "env_id_to_destroy"
}
```

**响应**：

```json
{
  "code": 200,
  "msg": "OK",
  "data": null
}
```

### 使用 SDK API

**端点**：`POST /sdk/v1/env/destroy`

**认证**：需要（User Sign）

```http
Authorization: Bearer YOUR_USER_SIGN
```

**请求参数**：与服务端 API 完全相同

```json
{
  "envId": "env_id_to_destroy"
}
```

**响应**：与服务端 API 完全相同

```json
{
  "code": 0,
  "msg": "ok",
  "data": null
}
```

---

## 环境生命周期

```plaintext
┌─────────────────────────────────────────────────────────────┐
│  1. 创建环境                                              │
│     POST /api/v2/browser/create                            │
│     Returns: envId                                        │
└────────────────────┬───────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  2. 使用环境                                              │
│     - SDK 初始化使用 envId                                 │
│     - 环境数据持久化（cookies, history, local storage）   │
└────────────────────┬───────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  3. 查询/更新环境                                         │
│     POST /api/v2/browser/page (查询列表)                  │
│     POST /api/v2/browser/update (更新配置)                 │
└────────────────────┬───────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│  4. 销毁环境                                              │
│     POST /api/v2/browser/destroy                          │
│     删除环境及所有持久化数据                                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 环境配置说明

### 代理配置

| 参数 | 说明 |
| --- | --- |
| proxy | 代理配置，格式：`socks5://user:pwd@ipaddr:6666` |
| bridgeProxy | 桥代理配置，格式：`socks5://user:pwd@ipaddr:6666` |
| ipChannel | IP监测渠道。海外代理：`ip2location`，国内代理：`ipdata` |
| region | 国家代号，当无法获取代理配置时，传此参数生成对应区域ip |

### 指纹配置

指纹配置项决定了浏览器环境的各种特征：

| 配置项 | 说明 |
| --- | --- |
| system | 操作系统 |
| kernel | 浏览器内核 |
| kernelVersion | 内核版本 |
| uaVersion | User-Agent 版本 |
| language | 语言列表 |
| geographic | 地理位置 |
| dpi | 屏幕分辨率 |
| font | 字体列表 |
| webRTC | WebRTC 配置 |
| canvas | Canvas 指纹 |
| webGl | WebGL 指纹 |
| audioContext | 音频上下文 |
| speechVoices | 语音合成 |
| mediaDevice | 媒体设备 |
| cpu | CPU 信息 |
| mem | 内存信息 |
| deviceName | 设备名称 |
| mac | MAC 地址 |
| bluetooth | 蓝牙信息 |
| doNotTrack | 不跟踪标志 |

---

## 最佳实践

1. **环境隔离**：每个用户或任务使用独立的环境
2. **代理使用**：为需要不同IP的场景配置不同代理
3. **指纹定制**：根据目标网站定制指纹配置
4. **定期清理**：定期销毁不需要的环境
5. **配额管理**：关注环境配额，避免超限

---

## 常见错误

| 错误码 | 说明 | 解决方法 |
| --- | --- | --- |
| 10700 | EnvNotFound | 环境不存在 |
| 10701 | EnvQuotaExceeded | 环境配额超限 |
| 10702 | EnvStatusAbnormal | 环境状态异常 |
| 10703 | EnvNameExist | 环境名称已存在 |
| 10704 | EnvCookieInvalid | 环境Cookie无效 |

---

## 下一步

- [服务端 API 参考](../api/server.md) - 查看完整的服务端 API 文档
- [SDK API 参考](../api/sdk.md) - 查看完整的 SDK API 文档
- [原生 C 集成指南](../integration/c-native.md) - 学习如何集成 SDK
