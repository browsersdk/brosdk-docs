# Rust 集成指南

BroSDK 提供官方的 Rust SDK，通过 `libloading` 动态加载原生动态库，提供安全的、符合 Rust 惯用法的 API，支持可选的 Tauri 集成。

**Demo 项目**：[github.com/browsersdk/brosdk-rust](https://github.com/browsersdk/brosdk-rust)

## 环境要求

| 项目 | 要求 |
|------|------|
| Rust | 2021 Edition |
| Tauri | v2（可选） |
| 平台 | Windows x64, macOS arm64/x64 |

## 安装

### 添加依赖

从 [crates.io](https://crates.io/crates/brosdk) 安装（推荐）：

```toml
[dependencies]
brosdk = "1.0.1"
```

或使用最新版本：

```toml
[dependencies]
brosdk = "1"
```

如需使用本地开发版本：

```toml
[dependencies]
brosdk = { path = "../brosdk-rust" }
```

如果不需要 Tauri 集成，可以禁用默认特性：

```toml
brosdk = { path = "../brosdk-rust", default-features = false }
```

## 动态库放置

从 [github.com/browsersdk/brosdk/releases](https://github.com/browsersdk/brosdk/releases) 下载原生库，并放置到项目指定目录：

```
项目根目录/
└── libs/
    ├── windows-x64/
    │   └── brosdk.dll        # Windows x64
    ├── windows-arm64/
    │   └── brosdk.dll        # Windows arm64
    ├── macos-arm64/
    │   └── brosdk.dylib      # macOS arm64
    └── macos-x64/
        └── brosdk.dylib      # macOS x64
```

## 快速上手

### 基本使用

```rust
use brosdk_sdk::{load, init, browser_open, browser_close, shutdown};

// 1. 加载原生库并注册回调
load("libs/windows-x64/brosdk.dll")?;

// 2. 使用 User Sign 初始化 SDK（User Sign 通过 REST API 换取）
init("your_user_sig", "/path/to/work_dir", 8080)?;

// 3. 打开浏览器环境 - 结果通过 "brosdk-event" 事件返回
browser_open("env-001")?;

// 4. 关闭浏览器环境
browser_close("env-001")?;

// 5. 关闭 SDK，释放资源
shutdown()?;
```

### 使用 Tauri 集成（推荐）

当启用 `tauri-app` 特性（默认开启）时，SDK 事件会通过 Tauri 事件系统分发：

```rust
use brosdk_sdk::{load, init, browser_open, browser_close, shutdown, SdkEvent};
use tauri::AppHandle;

// 1. 加载原生库并注册回调
load(app_handle, "libs/windows-x64/brosdk.dll")?;

// 2. 初始化 SDK
init("your_user_sig", "/path/to/work_dir", 8080)?;

// 3. 打开浏览器环境
browser_open("env-001")?;

// 监听 SDK 事件
app.listen("brosdk-event", |event| {
    let e: SdkEvent = serde_json::from_str(event.payload()).unwrap();
    println!("code={} data={}", e.code, e.data);
    
    match e.code {
        20111 => println!("浏览器打开成功"),
        20112 => println!("浏览器关闭成功"),
        10123 => println!("Token 即将过期"),
        _ => {}
    }
});

// 4. 关闭浏览器环境
browser_close("env-001")?;

// 5. 关闭 SDK
shutdown()?;
```

### 前端监听事件

在前端可以通过 Tauri 监听 SDK 事件：

```javascript
const { listen } = window.__TAURI__.event;

await listen("brosdk-event", ({ payload }) => {
    console.log("SDK 事件:", payload);
    // payload 格式: { code: number, data: JSON string }
});
```

## API 参考

### 核心函数

| 函数 | 说明 |
|------|------|
| `load(app, path)` | 加载原生库，注册 result + cookies-storage 回调 |
| `init(user_sig, work_dir, port)` | 使用凭证初始化 SDK，返回 JSON 结果字符串 |
| `browser_open(env_id)` | 启动浏览器环境（异步，结果通过 `brosdk-event` 返回） |
| `browser_close(env_id)` | 关闭浏览器环境 |
| `token_update(token_json)` | 刷新访问令牌 |
| `shutdown()` | 优雅关闭 |

### SdkEvent 结构体

```rust
pub struct SdkEvent {
    pub code: i32,    // SDK 状态码
    pub data: String, // 原生回调的 JSON 数据
}
```

### 常见事件码

| 事件码 | 说明 |
|--------|------|
| 20111 | 浏览器打开成功 |
| 20112 | 浏览器关闭成功 |
| 10123 | Token 即将过期 |
| 10124 | Token 已过期 |

## 完整示例

### Tauri 应用入口

```rust
// src-tauri/main.rs
use brosdk_sdk::{load, init, browser_open, browser_close, shutdown, SdkEvent};
use tauri::AppHandle;

fn main() {
    tauri::Builder::default()
        .setup(|app| {
            let app_handle = app.handle().clone();
            
            // 1. 加载 SDK
            let lib_path = match std::env::consts::OS {
                "windows" => "libs/windows-x64/brosdk.dll",
                "macos" => "libs/macos-arm64/brosdk.dylib",
                _ => panic!("Unsupported platform"),
            };
            
            if let Err(e) = load(app_handle.clone(), lib_path) {
                eprintln!("SDK 加载失败: {}", e);
                return Ok(());
            }
            
            // 2. 初始化 SDK（实际应用中 User Sign 应从服务器获取）
            let user_sig = get_user_sig_from_server()?;
            if let Err(e) = init(&user_sig, "./brosdk_data", 8080) {
                eprintln!("SDK 初始化失败: {}", e);
                return Ok(());
            }
            
            // 3. 监听 SDK 事件
            app_handle.listen("brosdk-event", |event| {
                if let Ok(e) = serde_json::from_str::<SdkEvent>(event.payload()) {
                    match e.code {
                        20111 => println!("浏览器打开成功: {}", e.data),
                        20112 => println!("浏览器关闭成功: {}", e.data),
                        10123 => {
                            println!("Token 即将过期，刷新中...");
                            // 调用 token_update 刷新
                        }
                        _ => println!("SDK 事件 [{}]: {}", e.code, e.data),
                    }
                }
            });
            
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            open_browser,
            close_browser,
        ])
        .run(tauri::generate_context!())
        .expect("启动 Tauri 应用失败");
}

// Tauri 命令：打开浏览器
#[tauri::command]
fn open_browser(env_id: String) -> Result<(), String> {
    browser_open(&env_id).map_err(|e| e.to_string())
}

// Tauri 命令：关闭浏览器
#[tauri::command]
fn close_browser(env_id: String) -> Result<(), String> {
    browser_close(&env_id).map_err(|e| e.to_string())
}

// 获取 User Sign（示例，实际应调用你的后端 API）
fn get_user_sig_from_server() -> Result<String, String> {
    // 这里应该调用你的后端 API 获取 User Sign
    // POST /api/v2/browser/getUserSig
    // Authorization: Bearer YOUR_API_KEY
    Ok("your_user_sig_here".to_string())
}
```

### 前端调用

```javascript
// 调用 Tauri 命令打开浏览器
import { invoke } from '@tauri-apps/api/core';

// 打开浏览器
async function openBrowser(envId) {
    try {
        await invoke('open_browser', { envId });
        console.log('浏览器打开请求已发送');
    } catch (e) {
        console.error('打开失败:', e);
    }
}

// 关闭浏览器
async function closeBrowser(envId) {
    try {
        await invoke('close_browser', { envId });
        console.log('浏览器关闭请求已发送');
    } catch (e) {
        console.error('关闭失败:', e);
    }
}
```

## 运行 Demo

```bash
cargo run --bin brosdk-demo
```

Demo 流程：
1. 输入 API Key → 点击 **SDK 初始化**：通过 REST API 换取 `userSig`，然后初始化原生 SDK
2. 点击 **创建环境**：调用 REST API 创建新的浏览器环境，并自动填充 env ID
3. 输入 env ID → 点击 **启动环境** / **关闭环境**

## 构建

```bash
# Debug 构建
cargo build

# Release 构建
cargo build --release
```

## 注意事项

1. **动态库路径**：确保动态库路径正确，否则 SDK 无法加载
2. **Tauri 事件**：默认启用 `tauri-app` 特性，SDK 事件会通过 Tauri 事件系统分发
3. **内存管理**：Rust SDK 自动处理内存管理，无需手动释放
4. **线程安全**：SDK 内部线程触发回调时，会自动调度回主线程

## 相关资源

- **Rust SDK**：[github.com/browsersdk/brosdk-rust](https://github.com/browsersdk/brosdk-rust)
- **C++ SDK**：[github.com/browsersdk/brosdk](https://github.com/browsersdk/brosdk)
- **TypeScript SDK**：[github.com/browsersdk/brosdk-typescript](https://github.com/browsersdk/brosdk-typescript)
- **Tauri 文档**：[tauri.app](https://v2.tauri.app/)
