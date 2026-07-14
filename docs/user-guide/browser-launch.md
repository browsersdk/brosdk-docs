# 环境启动参数与 CDP 连接

本文介绍浏览器环境的启动参数配置（`args`、`cookies`、`extensions`、`forward`、`yunConfig` 等），以及如何通过 Chrome DevTools Protocol（CDP）远程连接并自动化操作已启动的浏览器。

---

## 请求结构总览

`sdk_browser_open` / `POST /sdk/v1/browser/open` 接口的 `envs` 数组中，每个环境对象支持以下字段：

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `envId` | string / integer | 是 | 环境 ID（64 位整数字符串形式） |
| `args` | array\<string\> | 否 | 追加的 Chromium 启动参数列表 |
| `urls` | array\<string\> | 否 | 启动后自动打开的 URL 列表 |
| `cookies` | array | 否 | 随启动注入的 Cookie 数组（WebExtension API 格式） |
| `extensions` | array | 否 | 需要加载的扩展列表（含透传数据） |
| `forward` | string | 否 | 本次启动使用的前置跳板，优先级高于环境绑定的 `bridgeProxy` |

```json
{
  "envs": [
    {
      "envId": "2037495132382564352",
      "args": ["--no-first-run", "--remote-debugging-port=9222"],
      "urls": ["https://example.com"],
      "cookies": [],
      "extensions": [],
      "forward": ""
    }
  ]
}
```

> **就绪信号**：`browser-open-success`（`eventId=20111`）表示浏览器已完全启动且 CDP 就绪。Cookie / Storage / 扩展注入应以此通知作为就绪信号。

---

## 网络与代理

### 代理字段口径

浏览器启动时的网络路径由以下三个字段共同决定：

| 字段 | 来源 | 说明 |
| --- | --- | --- |
| `proxy` | 环境创建 / 更新阶段绑定 | 最终上游代理；不作为 `browser/open` 参数传入 |
| `forward` | `browser/open` 本次启动参数 | 显式前置跳板，优先级高于 `bridgeProxy` |
| `bridgeProxy` | 环境创建 / 更新阶段绑定 | 备用前置跳板；不作为 `browser/open` 参数传入 |

> `proxy` 和 `bridgeProxy` 应在创建或更新环境时绑定，`forward` 用于本次启动临时覆盖跳板。

### 代理决策规则

SDK 在每次浏览器启动前按以下优先级决策网络路径：

| 优先级 | 条件 | 实际行为 |
| --- | --- | --- |
| 1 | 环境绑定的 `proxy` 为空 | 不走业务代理，回退至 Chromium 默认网络栈 |
| 2 | `proxy` 有值 + 宿主机已具备出海能力（`global=true`） | 直接使用 `proxy`，忽略 `forward` 和 `bridgeProxy` |
| 3 | `proxy` 有值 + `global=false` + `forward` 有值 | `本地 bridge -> forward -> proxy -> 目标网站` |
| 4 | `proxy` 有值 + `global=false` + `forward` 为空 + `bridgeProxy` 有值 | `本地 bridge -> bridgeProxy -> proxy -> 目标网站` |

> `global` 是 SDK 内部对宿主机出海能力的判断值，不是客户配置字段。

进入代理桥时，SDK 会为每个浏览器实例单独启动一个本地 loopback bridge，并向 Chromium 注入：

```text
--proxy-server=socks5://127.0.0.1:{port}
```

### forward 参数说明

`forward` 用于在 `browser/open` 时临时指定前置跳板：

```json
{
  "envs": [
    {
      "envId": "2037495132382564352",
      "forward": "socks5://jump-proxy:31034"
    }
  ]
}
```

- `forward` 与 `bridgeProxy` **互斥**，不支持双跳前置链
- 如需行为稳定可预期，建议在创建/更新环境时显式绑定完整 `proxy` URL，勿依赖客户机器隐式系统代理配置
- `netdiag`（网络诊断）只读取 `bridgeProxy` 作为诊断跳板；如需验证 `forward` 链路，把同一跳板值放入 `bridgeProxy` 中诊断

### 常用网络参数

| 参数 | 说明 |
| --- | --- |
| `--proxy-server=socks5://127.0.0.1:1080` | 指定代理服务器 |
| `--proxy-bypass-list=localhost,127.0.0.1` | 代理绕过列表 |
| `--no-proxy-server` | 禁用代理 |
| `--host-resolver-rules="MAP * ~NOTFOUND, EXCLUDE 127.0.0.1"` | 自定义 DNS 解析规则 |

---

## CDP 远程调试

### 启动参数

| 参数 | 说明 |
| --- | --- |
| `--remote-debugging-port=9222` | **开启 CDP 远程调试端口**（用于 Puppeteer / Playwright / MCP 连接） |
| `--remote-debugging-address=0.0.0.0` | 允许外部 IP 连接（默认仅 127.0.0.1），生产环境慎用 |
| `--remote-allow-origins=*` | 允许跨源 WebSocket 连接（CDP 连接必须配合使用） |

```json
{
  "envs": [
    {
      "envId": "2037495132382564352",
      "args": [
        "--no-first-run",
        "--remote-debugging-port=9222",
        "--remote-allow-origins=*"
      ]
    }
  ]
}
```

### 端口分配建议

批量启动多个环境时，**每个环境可以自动分配 CDP 端口**，避免端口冲突：

```json
{
  "envs": [
    { "envId": "1111111111111111111", "args": ["--remote-debugging-port=0"] },
    { "envId": "2222222222222222222", "args": ["--remote-debugging-port=0"] },
    { "envId": "3333333333333333333", "args": ["--remote-debugging-port=0"] }
  ]
}
```

### CDP 就绪判断

`browser-open-success` 事件中的 `data` 包含 CDP 就绪状态：

```json
{
  "type": "browser-open-success",
  "data": {
    "envId": "2041695386304778240",
    "remoteDebuggingPort": 65534,
    "cdpReady": true
  }
}
```

- `cdpReady: true` 表示 CDP 已可用于业务逻辑
- 建议在收到 `browser-open-success` 后再发起 CDP 连接

### CDP 连接方式

#### chrome-devtools-mcp（MCP Server）

适合 AI 辅助自动化场景，将 CDP 封装为 AI 可调用的工具。

如果你希望 AI Agent 不仅连接 CDP，还能直接调用 BroSDK 的环境管理、浏览器打开和录制回放能力，优先使用 [MCP 使用指南](../integration/mcp.md) 中的 `brosdk-mcp-go`。

**安装依赖**：确保 Node.js 18+

**MCP 配置**（`~/.workbuddy/mcp.json`）：

```json
{
  "mcpServers": {
    "chrome-devtools": {
      "command": "npx",
      "args": ["@browsertoolsai/chrome-devtools-mcp@latest", "--port", "9222"]
    }
  }
}
```

`--port` 对应浏览器启动时指定的 `--remote-debugging-port` 端口号。

#### Puppeteer

```javascript
import puppeteer from 'puppeteer-core';

const CDP_PORT = 9222;

// 连接已启动的浏览器
const browser = await puppeteer.connect({
  browserURL: `http://127.0.0.1:${CDP_PORT}`,
  defaultViewport: null,
});

const page = (await browser.pages())[0];
await page.goto('https://example.com');
console.log(await page.title());

// 断开连接（不关闭浏览器进程）
await browser.disconnect();
```

#### Playwright

```javascript
import { chromium } from 'playwright';

const CDP_PORT = 9222;

const browser = await chromium.connectOverCDP(`http://127.0.0.1:${CDP_PORT}`);
const context = browser.contexts()[0];
const page = context.pages()[0];

await page.goto('https://example.com');
console.log(await page.title());

await browser.close(); // 断开连接，不关闭浏览器进程
```

#### 完整 Puppeteer 操作示例

以下示例展示完整的"启动浏览器 -> CDP 操作 -> 关闭浏览器"流程：

```javascript
import puppeteer from 'puppeteer-core';
import fetch from 'node-fetch';

const SDK_BASE_URL = 'http://127.0.0.1:9527'; // SDK Web API 端口
const ENV_ID = '2037495132382564352';
const CDP_PORT = 9222;

async function main() {
  // 1. 通过 SDK 启动浏览器（启用 CDP）
  const openRes = await fetch(`${SDK_BASE_URL}/sdk/v1/browser/open`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      envs: [{
        envId: ENV_ID,
        args: [
          '--no-first-run',
          '--no-default-browser-check',
          `--remote-debugging-port=${CDP_PORT}`,
          '--remote-allow-origins=*',
        ],
        urls: ['https://example.com'],
      }],
    }),
  });
  console.log('open:', await openRes.json());

  // 2. 等待浏览器启动完成（实际项目中建议监听 WebSocket 事件）
  await new Promise(r => setTimeout(r, 3000));

  // 3. 通过 CDP 连接
  const browser = await puppeteer.connect({
    browserURL: `http://127.0.0.1:${CDP_PORT}`,
    defaultViewport: null,
  });

  const page = (await browser.pages())[0];

  // 4. 操作页面
  await page.goto('https://example.com');
  const title = await page.title();
  console.log('页面标题:', title);

  await page.screenshot({ path: 'screenshot.png' });
  console.log('截图已保存: screenshot.png');

  const h1 = await page.evaluate(() => document.querySelector('h1')?.innerText);
  console.log('H1 内容:', h1);

  // 5. 断开 CDP 连接（不关闭浏览器进程）
  await browser.disconnect();

  // 6. 通过 SDK 关闭浏览器（自动持久化数据）
  const closeRes = await fetch(`${SDK_BASE_URL}/sdk/v1/browser/close`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ envs: [ENV_ID] }),
  });
  console.log('close:', await closeRes.json());
}

main().catch(console.error);
```

安装依赖：`npm install puppeteer-core node-fetch`

> CDP 连接只是远程控制接口，不影响 SDK 的数据持久化机制。调用 `browser/close` 后，Cookie 和 Storage 数据仍会自动保存。

---

## 浏览器行为参数

### 窗口与界面

| 参数 | 说明 |
| --- | --- |
| `--start-maximized` | 启动时最大化窗口 |
| `--window-size=1280,800` | 指定窗口尺寸 |
| `--window-position=0,0` | 指定窗口位置 |
| `--lang=zh-CN` | 强制设置浏览器界面语言 |
| `--client-appicon=<path>` | 自定义浏览器窗口图标（SDK 扩展参数，需传本地图标绝对路径） |

### 自动化与安全

| 参数 | 说明 |
| --- | --- |
| `--no-first-run` | 禁用首次运行向导 |
| `--no-default-browser-check` | 禁用默认浏览器检测提示 |
| `--disable-web-security` | 禁用同源策略（**仅调试用**，生产环境勿用） |
| `--disable-blink-features=AutomationControlled` | 隐藏自动化特征（反检测） |
| `--user-agent=<ua>` | 覆盖 User-Agent |

### macOS 专属参数

!!! warning "macOS 必须传入 `--parent-bundle-identifier`"
    在 macOS 上启动环境时，**必须**在 `args` 中添加 `--parent-bundle-identifier=<你的 App Bundle ID>`，否则浏览器进程无法正常启动。Bundle Identifier 在 Xcode 项目的 `Info.plist` 中查看（字段 `CFBundleIdentifier`），例如 `com.example.myapp`。

```json
{
  "envs": [
    {
      "envId": "2037495132382564352",
      "args": [
        "--no-first-run",
        "--parent-bundle-identifier=com.example.myapp"
      ]
    }
  ]
}
```

---

## 扩展注入（extensions）

`extensions` 用于在启动环境时加载自定义 Chrome 扩展，并支持通过 `data` 字段向扩展透传初始化数据。

### 扩展文件放置

在 SDK 初始化目录（`workDir`）下有一个 `extensions/` 文件夹，将**解包后的扩展目录**（含 `manifest.json`）放入其中：

```
workDir/
└── extensions/
    ├── testExt1/          ← 解包扩展目录（manifest.json 在此层级）
    │   ├── manifest.json
    │   ├── background.js
    │   └── ...
    └── testExt2/
        ├── manifest.json
        └── ...
```

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `name` | string | 是 | 扩展目录名，对应 `${workDir}/extensions/<name>` 路径下的解包扩展文件夹 |
| `id` | string | 是 | 扩展的固定 Chrome Extension ID，由 `key` 字段（在 `manifest.json` 中）或开发者账号决定，**必须预先知晓** |
| `packType` | string | 是 | 扩展打包类型，目前固定为 `"unpack"`（解包模式），保持默认即可 |
| `component` | boolean | 是 | 是否为组件扩展，通常为 `false`，保持默认即可 |
| `data` | object | 否 | 向扩展透传的键值对数据。SDK 启动时会将这些数据写入扩展的 `chrome.storage.local`，扩展内可直接读取 |

### 扩展内读取透传数据

SDK 将 `data` 字段中的键值对写入 `chrome.storage.local`，扩展的 `background.js` 或 `content_script.js` 可通过标准 API 读取：

```javascript
// 读取单个 key
const result = await chrome.storage.local.get('key1');
console.log(result.key1); // "aGVsbG8="

// 批量读取
const data = await chrome.storage.local.get(['key1', 'key2', 'key3']);
console.log(data);
// { key1: "aGVsbG8=", key2: "5L2g5aW9", key3: "12345234634574568478asdfdgsdfg" }
```

> `data` 字段的值为字符串类型，建议对复杂数据使用 Base64 编码后传入，扩展内解码后使用。

### 使用示例

```json
{
  "envs": [
    {
      "envId": "2037495132382564352",
      "args": ["--no-first-run"],
      "extensions": [
        {
          "name": "myLoginHelper",
          "id": "ebglcogbaklfalmoeccdjbmgfcacengf",
          "packType": "unpack",
          "component": false,
          "data": {
            "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
            "userId": "dXNlcl8xMjM0NQ==",
            "config": "eyJlbnYiOiJwcm9kIn0="
          }
        }
      ]
    }
  ]
}
```

> 扩展 ID 可在 `chrome://extensions/` 开发者模式下查看，或在 `manifest.json` 的 `key` 字段中计算得出。

---

## Cookie 注入（cookies）

`cookies` 用于在启动浏览器时向指定环境注入 Cookie，格式与 WebExtension API（`chrome.cookies`）兼容，可直接传入从浏览器导出的 Cookie JSON 数据。

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `domain` | string | 是 | Cookie 所属域名。以 `.` 开头表示对所有子域名生效（如 `.example.com`）；不带点则仅限当前主机 |
| `name` | string | 是 | Cookie 名称 |
| `value` | string | 是 | Cookie 值 |
| `path` | string | 否 | Cookie 有效路径，通常为 `/`，表示全站有效 |
| `expirationDate` | number | 否 | 过期时间（Unix 时间戳，秒级，支持小数）。不传或 `session=true` 时为会话 Cookie |
| `hostOnly` | boolean | 否 | 是否仅限当前主机（不含子域名） |
| `httpOnly` | boolean | 否 | 是否为 HttpOnly Cookie（JS 无法通过 `document.cookie` 读取） |
| `secure` | boolean | 否 | 是否仅在 HTTPS 连接下传输 |
| `session` | boolean | 否 | 是否为会话 Cookie。`true` 时忽略 `expirationDate`，浏览器关闭即失效 |
| `sameSite` | string | 否 | 跨站请求策略：`"strict"` 完全禁止跨站、`"lax"` 允许导航时携带、`"no_restriction"` 不限制（需配合 `secure=true`） |
| `storeId` | string/null | 否 | Cookie 存储 ID，通常留空（`""` 或 `null`）即可，SDK 会使用当前环境的默认存储 |

### 使用示例

```json
{
  "envs": [
    {
      "envId": "2037495132382564352",
      "args": ["--no-first-run"],
      "cookies": [
        {
          "domain": ".baidu.com",
          "expirationDate": 1808188306.943,
          "hostOnly": false,
          "httpOnly": true,
          "name": "BDUSS",
          "path": "/",
          "sameSite": "no_restriction",
          "secure": true,
          "session": false,
          "storeId": null,
          "value": "YOUR_COOKIE_VALUE_HERE"
        }
      ]
    }
  ]
}
```

> **提示**：`cookies` 字段注入的 Cookie 会覆盖环境中已有的同名 Cookie。如需保留历史登录状态，建议只注入必要的鉴权 Cookie（如 `session_id`、`token` 等），避免全量覆盖。

### Cookie 持久化链路

浏览器关闭时，Cookie 的持久化链路如下：

1. 从浏览器快照提取 Cookie JSON 数组
2. 调用 `sdk_cookies_storage_cb_t` 回调（允许宿主查看或替换明文 JSON）
3. 将最终 JSON 加密后写入本地 SQLite
4. 全托管模式下异步上传到 OSS

> 客户如需接管 Cookie 明文，只能在 `sdk_cookies_storage_cb_t` 回调阶段处理；进入持久化阶段后，SDK 保存和上传的都是加密后的二进制。

---

## 定制配置（yunConfig）

`yunConfig` 用于向定制版浏览器透传配置内容，SDK 会将其平铺透传到浏览器定义层。

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `shop` | object | 否 | 定制版浏览器商家配置，具体字段请联系定制版浏览器厂商 |
| `whitelist` | array | 否 | URL 白名单，命中规则的请求不受 `blacklist` 限制 |
| `blacklist` | array | 否 | URL 黑名单，命中后请求被拦截 |

```json
{
  "envs": [
    {
      "envId": "2051156171976347648",
      "args": ["--no-first-run"],
      "yunConfig": {
        "shop": {
          "shopId": "cd9ff4d2e44746a5ab58b56c546dfcc6",
          "name": "141",
          "shortName": "27008",
          "platform": "",
          "serial": "27008"
        },
        "whitelist": ["www.baidu.com"],
        "blacklist": ["www.cn.bing.com"]
      }
    }
  ]
}
```

> `shop` 的具体字段和含义请联系定制版浏览器厂商确认。

---

## 完整启动示例

综合使用网络代理、CDP 调试、扩展注入和 Cookie 注入：

```json
{
  "envs": [
    {
      "envId": "2051156171976347648",
      "args": [
        "--no-first-run",
        "--no-default-browser-check",
        "--remote-debugging-port=9222",
        "--remote-allow-origins=*",
        "--window-size=1280,800"
      ],
      "urls": [
        "https://example.com"
      ],
      "forward": "socks5://jump-proxy:31034",
      "extensions": [
        {
          "name": "myLoginHelper",
          "id": "ebglcogbaklfalmoeccdjbmgfcacengf",
          "packType": "unpack",
          "component": false,
          "data": {
            "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
          }
        }
      ],
      "cookies": [
        {
          "domain": ".example.com",
          "name": "session_id",
          "path": "/",
          "secure": true,
          "value": "abc123"
        }
      ],
      "yunConfig": {
        "whitelist": ["www.example.com"]
      }
    }
  ]
}
```

---

## 注意事项

!!! warning "端口冲突"
    批量启动多个环境时，每个环境必须使用不同的 CDP 端口，否则后启动的环境将占用相同端口导致连接失败。

!!! warning "macOS 必须传入 --parent-bundle-identifier"
    在 macOS 上启动环境时，**必须**在 `args` 中添加 `--parent-bundle-identifier=<你的 App Bundle ID>`，否则浏览器进程无法正常启动。

!!! warning "安全提示"
    `--remote-debugging-port` 默认仅监听 `127.0.0.1`，如需对外暴露需评估安全风险。生产环境不建议使用 `--remote-debugging-address=0.0.0.0`。

!!! warning "forward 与 bridgeProxy 互斥"
    当前不支持双跳前置链，不存在 `forward -> bridgeProxy -> proxy` 这种路径。

!!! tip "连接时机"
    建议在收到 `browser-open-success`（`eventId=20111`）事件后再发起 CDP 连接，确保浏览器进程已完全就绪。

!!! info "数据持久化"
    CDP 连接只是远程控制接口，不影响 SDK 的数据持久化机制。调用 `browser/close` 后，Cookie 和 Storage 数据仍会自动保存。

!!! info "关闭完成信号"
    `browser-close-success` 只保证本地持久化完成，不保证 OSS 上传完成。如果全托管模式下 OSS 上传失败，本地 SQLite 会保留 `UploadFailed` 状态，下次浏览器关闭时会自动重试。

!!! info "敏感数据校验"
    `urls`、`args`、`whiteList`、`blackList`、`extensions`、`cookies` 这类数组字段建议由接入层做长度、条目格式和敏感值校验，并在日志中做脱敏处理。

---

## 相关文档

- [环境管理](environment.md) - 环境的创建、更新与销毁
- [SDK 参考](../sdk-reference.md) - sdk_browser_open 完整接口文档
- [TypeScript 集成](../integration/typescript.md) - TypeScript SDK 使用指南
