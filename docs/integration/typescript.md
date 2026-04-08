# TypeScript 集成指南

BroSDK 提供官方的 TypeScript/Node.js SDK，通过 [koffi](https://koffi.dev/) 调用原生动态库，专为 Electron 主进程设计。

**Demo 项目**：[github.com/browsersdk/browser-sdk-demo](https://github.com/browsersdk/browser-sdk-demo)

## 环境要求

| 项目 | 要求 |
|------|------|
| Node.js | ≥ 18.0.0 |
| Electron | ≥ 20.0.0 |
| 平台 | Windows x64/arm64，macOS x64/arm64 |

## 安装

```bash
npm install brosdk
```

`koffi` 会随包自动安装。`electron` 由宿主应用提供，无需重复安装。

## 动态库放置

SDK 会在以下路径自动寻找原生库，**请按平台将对应目录放入项目**：

```
项目根目录/
└── sdk/
    ├── windows-x64/
    │   └── brosdk.dll        # Windows x64
    ├── arm64-windows/
    │   └── brosdk.dll        # Windows arm64
    ├── x64-osx/
    │   └── brosdk.dylib      # macOS x64
    └── arm64-osx/
        └── brosdk.dylib      # macOS arm64
```

- **开发阶段**：路径基于 `app.getAppPath()/sdk/...`
- **打包后**：路径基于 `process.resourcesPath/sdk/...`

## 快速上手

### 基本使用

```typescript
import BroSDK from 'brosdk'

const sdk = new BroSDK()

// 注册结果回调（异步操作的结果都通过此回调返回）
sdk.registerResultCb((code, data) => {
  console.log('SDK 回调', code, data)
})

// 注册 Cookie 持久化回调（可选）
sdk.registerCookiesStorageCb((cookies) => {
  console.log('收到 cookies', cookies)
  return null  // 返回 null 表示不修改，直接透传
})

// 同步初始化
const result = sdk.init({ port: 65535, userSig: 'your-user-sig' })
if (result.code === 0) {
  console.log('初始化成功', result.response)
  sdk.freePointer(result.ptr)  // 使用完毕后必须释放
} else {
  sdk.printErrno('init', result.code)
}

// 使用完毕后释放资源
sdk.shutdown()
```

### 返回值类型

所有 SDK 方法返回统一的响应格式：

```typescript
interface SDKResponse {
  code: number      // 状态码
  ptr: unknown      // 原生指针（需要 freePointer 释放）
  len: number       // 数据长度
  response: string | null  // JSON 响应数据
}
```

**简单接口**（如 `browserOpen`、`browserClose`、`tokenUpdate`）直接返回 `number`（状态码）。

### 初始化参数

```typescript
const result = sdk.init({
  port: 65535,        // 必需：内嵌 HTTP 服务监听端口
  userSig: 'xxx',     // 必需：从服务端获取的 User Sign
  workDir: 'C:/brosdk/'  // 可选：工作目录，默认为应用目录
})
```

## 在 Electron IPC 中使用

将 `BroSDK` 封装为 IPC 服务类，在 Electron 主进程中响应渲染进程的调用：

### 类型定义

```typescript
// types/sdk.ts
export interface IResponse {
  code: number  // 状态码
  msg: string   // 错误或提示信息
}

export interface IOpenCookie {
  domain: string
  expirationDate: number
  hostOnly: boolean
  httpOnly: boolean
  name: string
  path: string
  sameSite: string
  secure: boolean
  session: boolean
  storeId?: string
  value: string
}

export interface IOpenEnv {
  envId: string
  args?: string[]
  urls?: string[]
  cookies?: IOpenCookie[]
}

export interface IAppBindParams {
  port: number
  usersin: string  // 注意：是 usersin 不是 userSig
}

export interface IOpenParams {
  envs: IOpenEnv[]
}

export interface ICloseParams {
  envs: string[]
}
```

### IPC 服务实现

```typescript
import { ipcMain } from 'electron'
import type { IResponse, IAppBindParams, IOpenParams, ICloseParams } from '@type/sdk'
import BroSDK from 'brosdk'

export default class SDK {
  private bindStatus = false
  private broSDK: BroSDK

  constructor() {
    this.broSDK = new BroSDK()

    ipcMain.handle('app-bind', this.init)
    ipcMain.handle('app-token-update', this.tokenUpdate)
    ipcMain.handle('app-browser-open', this.browserOpen)
    ipcMain.handle('app-browser-close', this.browserClose)
    ipcMain.handle('app-shutdown', this.shutdown)
  }

  /** 初始化 SDK，绑定账户 */
  init = async (_event, data: IAppBindParams): Promise<IResponse> => {
    const initParam = {
      port: 65535,
      userSig: data.usersin  // 注意：SDK init 用 userSig，IPC 接口用 usersin
    }

    // 注册 Cookie 持久化回调
    this.broSDK.registerCookiesStorageCb((cookies) => {
      console.log('cookies:', cookies)
      return null
    })

    const res = await this.broSDK.init(JSON.stringify(initParam))
    console.log('init result:', res)

    let msg = 'Initialization failed.'
    if (res.code === 0) {
      msg = 'Initialization successful.'
      this.bindStatus = true
      this.broSDK.freePointer(res.ptr)  // 释放 SDK 分配的输出缓冲区
    }

    return {
      code: res.code,
      msg
    }
  }

  /** 更新 Token */
  tokenUpdate = async (_event, data: object): Promise<IResponse> => {
    const res = await this.broSDK.tokenUpdate(JSON.stringify(data))
    return {
      code: res,
      msg: ''
    }
  }

  /** 打开浏览器环境 */
  browserOpen = async (_event, data: IOpenParams): Promise<IResponse> => {
    console.log('启动环境', data)
    const res = await this.broSDK.browserOpen(JSON.stringify(data))
    return {
      code: res,
      msg: ''
    }
  }

  /** 关闭浏览器环境 */
  browserClose = async (_event, data: ICloseParams): Promise<IResponse> => {
    console.log('关闭环境', data)
    const res = await this.broSDK.browserClose(JSON.stringify(data))
    return {
      code: res,
      msg: ''
    }
  }

  /** 关闭 SDK，释放所有资源 */
  shutdown = async (_event): Promise<IResponse | void> => {
    if (!this.bindStatus) return

    const code = await this.broSDK.shutdown()
    if (code === 0) {
      this.bindStatus = false
    }
    return {
      code,
      msg: ''
    }
  }
}
```

## API 参考

### 初始化

```typescript
// 同步初始化
const result = sdk.init({ 
  port: 65535, 
  userSig: 'your-user-sig',
  workDir: 'C:/brosdk/'  // 可选
})
// result: { code: number, ptr: unknown, len: number, response: string | null }
if (result.code === 0) {
  sdk.freePointer(result.ptr)  // 必须释放
}

// 异步初始化，结果通过 registerResultCb 回调返回
const reqId = sdk.initAsync({ port: 65535, userSig: 'xxx' })

// 启动 Web API 服务
sdk.initWebAPI(8080)
```

### 环境管理

所有环境接口返回 `{ code, ptr, len, response }`，调用后需 `freePointer(res.ptr)` 释放内存。

#### 创建环境

```typescript
const res = sdk.envCreate({ 
  envName: '测试环境',
  customerId: 'user_12345',
  proxy: 'socks5://user:pass@proxy:1080',
  finger: {
    system: 'Windows 11',
    kernel: 'Chrome',
    kernelVersion: '148'
  }
})
const env = JSON.parse(res.response ?? '{}')
sdk.freePointer(res.ptr)
```

#### 更新环境

```typescript
const res = sdk.envUpdate({ 
  envId: 'env-001', 
  envName: '新名称' 
})
sdk.freePointer(res.ptr)
```

#### 查询环境列表

```typescript
const res = sdk.envPage({ page: 1, pageSize: 20 })
const list = JSON.parse(res.response ?? '{}')
sdk.freePointer(res.ptr)
```

#### 删除环境

```typescript
const res = sdk.envDestroy({ envId: 'env-001' })
sdk.freePointer(res.ptr)
```

### 浏览器控制

**注意**：浏览器控制接口直接返回 `number`（状态码），无需释放内存。

#### 打开浏览器

```typescript
// 参数格式：{ envs: [{ envId, urls?, cookies? }] }
const code = sdk.browserOpen({ 
  envs: [
    { 
      envId: 'env-001',
      urls: ['https://www.example.com']
    }
  ]
})
```

#### 关闭浏览器

```typescript
// 参数格式：{ envs: string[] } 或 string[]
const code = sdk.browserClose({ 
  envs: ['env-001'] 
})
// 或直接传数组
const code2 = sdk.browserClose(['env-001'])
```

### Token 管理

#### 更新 Token

```typescript
const code = sdk.tokenUpdate({ userSig: 'new-user-sig' })
// 返回 number（状态码）
```

### 信息查询

#### 获取 SDK 版本信息

```typescript
const info = sdk.sdkInfo()
console.log(info.response)  // JSON 字符串
sdk.freePointer(info.ptr)
```

#### 获取浏览器信息

```typescript
const info = sdk.browserInfo()
console.log(info.response)
sdk.freePointer(info.ptr)
```

### 回调注册

#### 结果回调

```typescript
sdk.registerResultCb((code: number, data: string) => {
  if (sdk.isOk(code)) {
    const payload = JSON.parse(data)
    // 处理成功结果...
  } else if (sdk.isEvent(code)) {
    console.log('事件:', sdk.eventName(code), data)
  } else if (sdk.isError(code)) {
    console.error('错误:', sdk.errorString(code))
  }
})
```

#### Cookie 持久化回调

```typescript
sdk.registerCookiesStorageCb((cookies) => {
  // 可将 cookies 持久化到本地文件/数据库
  saveCookiesToDisk(cookies)
  return null  // 不修改内容，直接透传
})
```

### 辅助方法

#### 判断状态码类型

```typescript
sdk.isOk(code)      // 是否成功
sdk.isError(code)   // 是否错误
sdk.isWarn(code)    // 是否警告
sdk.isEvent(code)   // 是否事件
sdk.isDone(code)    // 是否完成
sdk.isReqid(code)   // 是否请求 ID

// 获取错误/事件名称
sdk.errorName(code)     // 错误名称
sdk.errorString(code)   // 错误描述
sdk.eventName(evtid)    // 事件名称
```

#### 打印错误信息

```typescript
sdk.printErrno('init', result.code)
// 输出：[init] OK  code=0  (OK): Success
```

### 生命周期

#### 释放资源

```typescript
// 释放 SDK 分配的内存（init、sdkInfo、envCreate 等有输出缓冲区的接口）
sdk.freePointer(res.ptr)

// 关闭 SDK，自动注销所有已注册回调，释放原生资源
const code = sdk.shutdown()
```

## Web API 接口（HTTP）

> **前提**：需先在 `sdk.init` 请求 JSON 中指定 `port` 字段启动内嵌 HTTP 服务。

**通用请求头**：`Content-Type: application/json`  
**Base URL**：`http://127.0.0.1:{port}`

| 方法 | 路径 | 说明 | 对应 API |
|------|------|------|----------|
| `POST` | `/sdk/v1/init` | 初始化 SDK | `sdk.init` |
| `POST` | `/sdk/v1/token/update` | 刷新令牌 | `sdk.tokenUpdate` |
| `POST` | `/sdk/v1/browser/open` | 打开浏览器 | `sdk.browserOpen` |
| `POST` | `/sdk/v1/browser/close` | 关闭浏览器 | `sdk.browserClose` |
| `POST` | `/sdk/v1/env/create` | 创建环境 | `sdk.envCreate` |
| `POST` | `/sdk/v1/env/update` | 更新环境 | `sdk.envUpdate` |
| `POST` | `/sdk/v1/env/page` | 环境列表（分页） | `sdk.envPage` |
| `POST` | `/sdk/v1/env/destroy` | 销毁环境 | `sdk.envDestroy` |
| `POST` | `/sdk/v1/shutdown` | 停止 SDK | `sdk.shutdown` |

**请求示例**：

```http
POST http://127.0.0.1:9527/sdk/v1/browser/open
Content-Type: application/json

{
  "envs": [
    {
      "envId": 2028432501503954944,
      "urls": ["https://www.example.com"]
    }
  ]
}
```

**响应示例**：

```json
{
  "reqid": 1006901417,
  "code": 0,
  "msg": "ok",
  "result": {
    "envId": 2028432501503954944,
    "eventId": 20111
  }
}
```

## 完整示例

### Electron 主进程入口

```typescript
// main.ts
import { app, BrowserWindow, ipcMain } from 'electron'
import path from 'path'
import BroSDK from 'brosdk'
import type { IResponse, IAppBindParams, IOpenParams, ICloseParams } from '@type/sdk'

let mainWindow: BrowserWindow | null = null
let broSDK: BroSDK | null = null

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true
    }
  })

  mainWindow.loadFile('index.html')
}

// SDK 服务类
class SDKService {
  private bindStatus = false

  constructor(private sdk: BroSDK) {
    ipcMain.handle('app-bind', this.init)
    ipcMain.handle('app-token-update', this.tokenUpdate)
    ipcMain.handle('app-browser-open', this.browserOpen)
    ipcMain.handle('app-browser-close', this.browserClose)
    ipcMain.handle('app-shutdown', this.shutdown)
  }

  init = async (_event, data: IAppBindParams): Promise<IResponse> => {
    const initParam = { port: 65535, userSig: data.usersin }
    
    this.sdk.registerCookiesStorageCb((cookies) => {
      console.log('cookies:', cookies)
      return null
    })

    const res = await this.sdk.init(JSON.stringify(initParam))
    
    let msg = 'Initialization failed.'
    if (res.code === 0) {
      msg = 'Initialization successful.'
      this.bindStatus = true
      this.sdk.freePointer(res.ptr)
    }

    return { code: res.code, msg }
  }

  tokenUpdate = async (_event, data: object): Promise<IResponse> => {
    const res = await this.sdk.tokenUpdate(JSON.stringify(data))
    return { code: res, msg: '' }
  }

  browserOpen = async (_event, data: IOpenParams): Promise<IResponse> => {
    const res = await this.sdk.browserOpen(JSON.stringify(data))
    return { code: res, msg: '' }
  }

  browserClose = async (_event, data: ICloseParams): Promise<IResponse> => {
    const res = await this.sdk.browserClose(JSON.stringify(data))
    return { code: res, msg: '' }
  }

  shutdown = async (): Promise<IResponse | void> => {
    if (!this.bindStatus) return
    const code = await this.sdk.shutdown()
    if (code === 0) this.bindStatus = false
    return { code, msg: '' }
  }
}

// 初始化 SDK
async function initSDK() {
  broSDK = new BroSDK()
  
  // 注册全局回调
  broSDK.registerResultCb((code, data) => {
    console.log('SDK 回调', code, data)
    
    if (broSDK?.isEvent(code)) {
      const eventName = broSDK.eventName(code)
      console.log('事件:', eventName, data)
      
      // Token 即将过期（事件 ID: 10123）
      if (code === 10123) {
        console.log('Token 即将过期，需要刷新')
      }
    }
  })

  // 从服务端获取 User Sign（示例）
  const userSig = await getUserSigFromServer()
  
  const result = broSDK.init({
    port: 65535,
    userSig: userSig,
    workDir: 'C:/brosdk/'
  })

  if (result.code === 0) {
    console.log('SDK 初始化成功')
    broSDK.freePointer(result.ptr)
    new SDKService(broSDK)  // 注册 IPC 处理器
  } else {
    console.error('SDK 初始化失败:', result.code)
    broSDK.printErrno('init', result.code)
  }
}

app.whenReady().then(() => {
  createWindow()
  initSDK()
})

app.on('will-quit', () => {
  if (broSDK) {
    broSDK.shutdown()
    broSDK = null
  }
})
```

### 渲染进程调用

```typescript
// renderer 进程中通过 IPC 调用
import { ipcRenderer } from 'electron'

// 打开浏览器环境
async function openBrowser(envId: string, urls: string[] = []) {
  const response = await ipcRenderer.invoke('app-browser-open', {
    envs: [{ envId, urls }]
  })
  
  if (response.code === 0) {
    console.log('浏览器打开成功')
  } else {
    console.error('打开失败:', response.msg)
  }
}

// 关闭浏览器环境
async function closeBrowser(envId: string) {
  const response = await ipcRenderer.invoke('app-browser-close', {
    envs: [envId]
  })
  
  return response
}

// 更新 Token
async function updateToken(newUserSig: string) {
  const response = await ipcRenderer.invoke('app-token-update', {
    userSig: newUserSig
  })
  
  return response
}
```

## 注意事项

1. **内存管理**：所有返回 `{ code, ptr, len, response }` 的接口，使用后必须调用 `freePointer(ptr)` 释放内存
2. **线程安全**：SDK 内部线程触发回调时，koffi 会自动调度回 Node.js 主线程
3. **端口占用**：初始化时指定的 `port` 不能被其他程序占用
4. **动态库路径**：确保动态库按正确目录结构放置，否则 SDK 无法加载
5. **Electron 打包**：打包后动态库路径基于 `process.resourcesPath`，需确保打包配置正确

## 相关资源

- **TypeScript SDK**：[github.com/browsersdk/brosdk-typescript](https://github.com/browsersdk/brosdk-typescript)
- **C++ SDK**：[github.com/browsersdk/brosdk](https://github.com/browsersdk/brosdk/releases)
- **koffi 文档**：[koffi.dev](https://koffi.dev/)
