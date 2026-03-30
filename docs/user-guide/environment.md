# 环境管理

BroSDK 使用 `envId`（64位整数）管理浏览器环境。每个环境都有独立的 cookies、历史记录、本地存储等持久化数据。

## 创建环境

!!! tip "最简创建：只需三个关键参数"
    创建环境时，**绝大多数参数都有合理的默认值，无需手动设置**。只需关注以下三个核心参数即可：

    | 参数 | 说明 |
    |---|---|
    | `proxy` | **代理地址**（选填）。浏览器环境的出口 IP 由此决定，直接影响语言、时区、地理位置等指纹的自动生成；不填则使用本机网络 |
    | `finger.kernel` | **浏览器内核类型**，目前支持 `Chrome` |
    | `finger.kernelVersion` | **内核版本号**（如 `134`、`131`）。版本越新兼容性越好，建议使用最新稳定版本 |

    其他指纹参数（语言、时区、UA、Canvas、WebGL 等）均会**根据代理 IP 自动生成**，无需手动指定。

### 最简示例

```json
{
  "customerId": "user_12345",
  "proxy": "socks5://username:password@proxy:port",
  "finger": {
    "kernel": "Chrome",
    "kernelVersion": "134"
  }
}
```

### 使用服务端 API

**端点**：`POST /api/v2/browser/create`

**认证**：需要（API Key）

```http
Authorization: Bearer YOUR_API_KEY
```

**完整请求参数**：

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
    "kernel": "Chrome",
    "kernelVersion": "134",
    "system": "Windows 11",
    "uaVersion": "134",
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

**请求参数**：与服务端 API 完全相同，最简只需传核心三个参数：

```json
{
  "customerId": "user_12345",
  "proxy": "socks5://username:password@proxy:port",
  "finger": {
    "kernel": "Chrome",
    "kernelVersion": "134"
  }
}
```

**完整请求参数示例**：

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
    "kernel": "Chrome",
    "kernelVersion": "134",
    "system": "Windows 11",
    "uaVersion": "134",
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

```mermaid
flowchart TD
    A[创建环境<br/>POST /api/v2/browser/create<br/>Returns: envId] --> B[使用环境<br/>SDK 初始化使用 envId<br/>环境数据持久化]
    B --> C[查询/更新环境<br/>POST /api/v2/browser/page<br/>POST /api/v2/browser/update]
    C --> D[销毁环境<br/>POST /api/v2/browser/destroy<br/>删除环境及所有持久化数据]
    
    style A fill:#e3f2fd,stroke:#1976d2
    style B fill:#e8f5e9,stroke:#388e3c
    style C fill:#fff3e0,stroke:#f57c00
    style D fill:#ffebee,stroke:#d32f2f
```

---

## 环境配置说明

### 获取指纹配置可选值

在创建或更新环境时，可以通过 `getUiFingerList` 接口获取当前可用的指纹配置选项，避免填写无效值导致请求失败。

**接口信息**：

| 项目 | 说明 |
|------|------|
| 端点 | `GET /api/v2/browser/getUiFingerList` |
| 认证 | 需要（API Key） |
| 基础 URL | `https://api.brosdk.com` |

**请求示例**：

```bash
curl -X GET "https://api.brosdk.com/api/v2/browser/getUiFingerList" \
  -H "Authorization: Bearer YOUR_API_KEY"
```

**返回参数**：

| 字段 | 类型 | 说明 |
|------|------|------|
| system | object | 操作系统及内核版本信息 |
| system.Windows | array | Windows 系统及对应内核版本 |
| system.MacOS | array | macOS 系统及对应内核版本 |
| system.Linux | array | Linux 系统及对应内核版本 |
| platformKernelversion | array | 平台可用的内核版本 |
| chromeUAversion | array | Chrome 浏览器 UA 版本 |
| firefoxUAversion | array | Firefox 浏览器 UA 版本 |
| cpu | array | CPU 核心数选项 |
| mem | array | 内存大小选项（GB） |
| dpi | array | 屏幕分辨率选项 |
| language | array | 浏览器语言选项 |
| zone | array | 时区选项 |
| region | array | 国家/地区代码选项 |
| webgl | object | WebGL 配置选项 |

**返回示例**：

```json
{
  "code": 200,
  "msg": "OK",
  "data": {
    "system": {
      "Windows": [
        {"name": "Windows", "system": "10", "browser": "107"},
        {"name": "Windows", "system": "10", "browser": "119"},
        {"name": "Windows", "system": "11", "browser": "127"}
      ],
      "MacOS": [...]
    },
    "platformKernelversion": ["Chrome", "Firefox"],
    "chromeUAversion": ["107", "119", "127", "131"],
    "cpu": [4, 8, 12, 16],
    "mem": [4, 8, 16],
    "dpi": ["1920x1080", "1366x768", "1440x900"],
    "language": ["zh-CN", "en-US", "ja-JP"],
    "zone": ["Asia/Shanghai", "America/New_York", "Europe/London"],
    "region": ["CN", "US", "JP", "KR"],
    "webgl": {...}
  }
}
```

> 💡 **提示**：建议在应用初始化时调用此接口获取可选值列表，并缓存起来供用户选择。这样可以确保用户选择的配置项是有效的。

---

### 基本参数

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| customerId | string | 是 | 客户自定义 ID，用于标识环境所属用户 | - |
| envName | string | 否 | 环境名称，便于识别和管理 | - |
| remark | string | 否 | 环境备注信息 | - |
| serial | string | 否 | 环境序号 | - |
| cookie | string | 否 | Cookie 数据，用于预置登录状态 | - |
| ipChannel | string | 否 | IP监测渠道。海外代理填写 `ip2location`，国内代理填写 `ipdata` | - |
| proxy | string | 否 | 代理配置，格式：`socks5://user:pwd@ip:port` 或 `http://ip:port` | - |
| bridgeProxy | string | 否 | 桥代理配置，格式同 proxy | - |
| region | string | 否 | 国家代号，当无法获取代理配置时，传此参数生成对应区域IP | - |

### 代理配置

**支持的代理协议**：

| 协议 | 格式示例 |
|------|----------|
| SOCKS5 | `socks5://user:password@192.168.1.1:1080` |
| HTTP | `http://192.168.1.1:8080` |
| HTTPS | `https://192.168.1.1:8443` |

**region 国家代码参考**：

| 大洲 | 国家代码示例 |
|------|-------------|
| 亚洲 | CN, TW, HK, MO, JP, KR, IN, SG |
| 欧洲 | GB, FR, DE, IT, ES, NL, BE, CH |
| 北美 | US, CA, MX |
| 大洋洲 | AU, NZ |

---

### 指纹配置 (finger)

指纹配置决定了浏览器环境的各种特征，用于模拟真实用户的浏览器环境。

#### 基础指纹

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| kernel | string | 是 | 浏览器内核类型，目前支持：`Chrome` | - |
| kernelVersion | string | 是 | 内核版本号。Chrome 支持：107, 119, 122, 127, 131, 134, 138 | - |
| system | string | 否 | 操作系统：`Windows 7/8/8.1/10/11`、`MacOS 10-15`、`Android 9-13`、`IOS 14-16`、`Linux` | - |
| ua | string | 否 | 自定义 User-Agent，为空时根据系统和浏览器版本自动生成 | 自动生成 |
| uaVersion | string | 否 | User-Agent 版本号 | 与 kernelVersion 一致 |
| language | string[] | 否 | 浏览器语言列表，如 `["zh-CN", "en-US"]`，不传会根据代理 IP 自动生成 | 自动生成 |
| zone | string | 否 | 时区，如 `Asia/Shanghai`、`America/New_York`，不传会根据代理 IP 自动生成 | 自动生成 |

#### 屏幕与显示

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| dpi | string | 否 | 屏幕分辨率，格式：`1920x1080`，`default`=快捷选择，`auto`=随机 | default |

**支持的 DPI 分辨率**：

| 平台 | 可选值 |
|------|--------|
| Windows | 800x600, 1024x768, 1280x720, 1366x768, 1440x900, 1920x1080, 1920x1200, 2560x1440 等 |
| MacOS | 2048x1152, 2304x1440, 2560x1440, 2560x1600, 2880x1800, 4096x2304 等 |
| Linux | 1024x768, 1280x720, 1366x768, 1920x1080, 2560x1440 等 |
| Android | 720x1280, 768x1280, 1024x600, 1280x800, 1920x1200 等 |
| iOS | 1334x750, 828x1792, 1170x2532, 1125x2436 等 |

#### 字体指纹

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| fontFinger | int | 否 | 字体指纹模式：`1`=隐身（随机），`2`=真实（使用系统字体） | 1 |
| font | object | 否 | 字体列表配置 | - |
| font.enable | int | 否 | 字体启用模式：`1`=隐身包含，`2`=使用用户设置 | 1 |
| font.list | string[] | 否 | 自定义字体列表，当 enable=2 时生效 | 自动生成 |

#### WebRTC 配置

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| webRTC | int | 否 | WebRTC 处理模式：`1`=真实（网站获取真实IP），`2`=替换（使用代理IP覆盖），`5`=禁用（网站无法获取IP） | 5 |
| webRTCIP | string | 否 | 自定义内网 IP，当 webRTC=2 时使用 | - |

#### Canvas 指纹

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| canvas | int | 否 | Canvas 指纹模式：`1`=关闭，`2`=倾向随机性，`4`=倾向一致性 | 4 |

#### WebGL 指纹

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| webGl | int | 否 | WebGL 元数据指纹：`1`=隐身，`2`=真实 | 1 |
| webGlInfo | int | 否 | WebGL 信息模式：`1`=真实，`2`=自定义 | 2 |
| webGlVendor | string | 否 | WebGL 厂商标识 | 自动生成 |
| webGlRenderer | string | 否 | WebGL 渲染器标识 | 自动生成 |

**webGlVendor 可选值**：

| 平台 | 可选值 |
|------|--------|
| Windows | `Google Inc. (NVIDIA)`, `Google Inc. (AMD)`, `Google Inc. (Intel)` |
| macOS | `Google Inc. (Apple)`, `Google Inc. (ATI Technologies Inc.)`, `Google Inc. (NVIDIA)` |
| Android | `Qualcomm` |
| iOS | `Apple Inc` |

#### 音频与媒体

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| audioContext | int | 否 | 音频流指纹：`1`=隐身，`2`=真实 | 1 |
| speechVoices | int | 否 | 语音合成指纹：`1`=使用电脑默认，`2`=添加噪音（每浏览器不同） | 2 |
| mediaDevice | int | 否 | 媒体设备指纹：`1`=关闭（使用默认），`2`=启用（添加噪声） | 2 |

#### 硬件信息

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| cpu | int | 否 | CPU 核心数：`4`, `8`, `12`, `16`, `20`, `24` | 自动生成 |
| mem | int | 否 | 内存大小（GB）：`4`, `8` | 自动生成 |
| deviceName | string | 否 | 计算机名称 | 自动生成 |
| mac | string | 否 | MAC 地址 | 自动生成 |
| hardware | int | 否 | 硬件加速：`1`=开启，`2`=关闭 | 1 |
| bluetooth | int | 否 | 蓝牙：`1`=开启，`2`=关闭 | 2 |

#### 地理位置

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| geographic | object | 否 | 地理位置配置 | - |
| geographic.enable | int | 否 | 地理位置设置：`1`=启用，`2`=询问，`3`=禁止 | 1 |
| geographic.user | int | 否 | 定位方式：`1`=使用IP定位，`2`=自定义 | 1 |
| geographic.longitude | string | 否 | 经度，范围 -180 到 180 | - |
| geographic.latitude | string | 否 | 纬度，范围 -90 到 90 | - |
| geographic.accuracy | string | 否 | 精度（米），范围 10 到 5000 | - |

#### 浏览器行为

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| doNotTrack | int | 否 | "请勿跟踪"设置：`1`=启用，`2`=默认，`3`=不启用 | 2 |
| enableNotice | int | 否 | 网页通知：`1`=开启，`2`=关闭 | 1 |
| enableSound | int | 否 | 禁止播放声音：`1`=开启，`2`=关闭 | 2 |
| enableVideo | int | 否 | 禁止加载视频：`1`=开启，`2`=关闭 | 2 |
| enablePic | int | 否 | 禁止加载图片：`1`=开启，`2`=关闭 | 2 |
| picSize | string | 否 | 图片大小限制（KB），当 enablePic=1 时必填 | - |
| enableGc | int | 否 | 垃圾回收：`1`=开启，`2`=关闭 | 2 |
| gcTime | int | 否 | 垃圾回收时间（分钟），当 enableGc=1 时必填，范围 1-5 | 1 |
| enableScanPort | int | 否 | 端口扫描防护：`1`=开启，`2`=关闭 | 1 |
| scanPort | string | 否 | 端口白名单，多个用逗号分隔，范围 0-65535 | 自动生成 |
| enableOpen | int | 否 | 启动时自动打开浏览器：`1`=开启，`2`=关闭 | 1 |

#### 存储与清理

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| enableStorage | int | 否 | 缓存：`1`=开启，`2`=关闭 | 1 |
| clearCookie | int | 否 | 关闭时清理 Cookie：`1`=清理，`2`=不清理 | 2 |
| clearStorage | int | 否 | 关闭时清理缓存：`1`=清理，`2`=不清理 | 2 |

#### 开发者与搜索

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| enableDevtools | int | 否 | 开发者工具：`1`=开启，`2`=关闭 | 2 |
| searchEngine | int | 否 | 默认搜索引擎：`1`=百度，`2`=谷歌，`3`=360 | 1 |
| translateLang | int | 否 | 翻译目标语言：`1`=跟随IP，`2`=中文，`3`=英文 | 1 |
| shortName | int | 否 | 任务栏显示：`1`=不显示，`2`=显示序号，`3`=显示名称前5位，`4`=显示名称后5位 | 2 |

#### 电池状态（仅支持部分浏览器）

| 参数 | 类型 | 必填 | 说明 | 默认值 |
|------|------|------|------|--------|
| battery | int | 否 | 电池状态：`1`=真实，`2`=添加噪声 | 1 |

---

### 完整配置示例

```json
{
  "envName": "美国浏览器环境",
  "customerId": "user_12345",
  "proxy": "socks5://user:password@192.168.1.1:1080",
  "ipChannel": "ip2location",
  "finger": {
    "system": "Windows 11",
    "kernel": "Chrome",
    "kernelVersion": "134",
    "language": ["en-US"],
    "zone": "America/New_York",
    "dpi": "1920x1080",
    "webRTC": 2,
    "webRTCIP": "192.168.88.100",
    "canvas": 4,
    "webGl": 1,
    "cpu": 8,
    "mem": 8
  }
}
```

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
- [SDK 参考](../sdk-reference.md) - 查看完整的 SDK API 文档
- [原生 C 集成指南](../integration/c-native.md) - 学习如何集成 SDK
