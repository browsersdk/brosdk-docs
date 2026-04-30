# BroSDK Docs

[English](README_EN.md) | 简体中文

[![Deploy MkDocs to GitHub Pages](https://github.com/browsersdk/brosdk-docs/actions/workflows/deploy.yml/badge.svg)](https://github.com/browsersdk/brosdk-docs/actions/workflows/deploy.yml)

`brosdk-docs` 是 BroSDK 的官方文档站点源码，基于 MkDocs Material 构建，覆盖浏览器环境管理、原生 SDK API、回调机制、多语言集成、服务端 API、部署和故障排查。

文档面向需要接入 BroSDK 的后端、桌面端、自动化测试、AI Agent 和平台型产品开发者。

## 在线文档

```text
https://browsersdk.github.io/brosdk-docs/
```

## 文档内容

| 模块 | 说明 |
|------|------|
| 快速开始 | 从初始化 SDK 到创建、启动、关闭浏览器环境的最短路径 |
| 用户指南 | 环境模型、启动参数、CDP 连接和浏览器生命周期说明 |
| 集成指南 | C 原生 SDK、TypeScript、Rust 和回调机制接入方式 |
| SDK 参考 | 原生接口、参数结构、返回值和错误码说明 |
| 服务端 API | BroSDK 服务端接口、请求响应格式和 OpenAPI 描述 |
| 部署指南 | 本地、桌面端、服务端和 CI/CD 部署建议 |
| 故障排除 | 初始化、鉴权、内核下载、浏览器启动和回调问题排查 |

## 与 BroSDK 的关系

BroSDK 由多个仓库组成，本仓库负责文档站点和 API 说明：

| 仓库 | 模块 | 说明 |
|------|------|------|
| [brosdk](https://github.com/browsersdk/brosdk) | Native SDK | C/C++ API、头文件和平台动态库 |
| [brosdk-core](https://github.com/browsersdk/brosdk-core) | Browser Core | 浏览器内核、版本和运行依赖 |
| [brosdk-docs](https://github.com/browsersdk/brosdk-docs) | Documentation | 文档站点、API 参考和接入指南 |
| [brosdk-typescript](https://github.com/browsersdk/brosdk-typescript) | TypeScript SDK | Node.js / Electron 接入 |
| [brosdk-python](https://github.com/browsersdk/brosdk-python) | Python SDK | Python 自动化、测试和 AI 工作流接入 |
| [brosdk-rust](https://github.com/browsersdk/brosdk-rust) | Rust SDK | Rust 服务、CLI 和 Tauri 桌面应用接入 |
| [browser-demo](https://github.com/browsersdk/browser-demo) | Demo | 完整服务端和桌面客户端示例 |

## 本地开发

### 环境要求

- Python 3.11+
- pip

### 安装依赖

```bash
pip install -r requirements.txt
```

如果需要手动安装：

```bash
pip install mkdocs mkdocs-material mkdocs-include-markdown-plugin pymdown-extensions mkdocs-mermaid2-plugin
```

### 启动预览

```bash
mkdocs serve
```

浏览器访问：

```text
http://127.0.0.1:8000
```

### 构建静态站点

```bash
mkdocs build
```

构建产物位于 `site/` 目录。

## 目录结构

```text
brosdk-docs/
├── docs/
│   ├── index.md                 # 文档首页
│   ├── quick-start.md           # 快速开始
│   ├── sdk-reference.md         # SDK 参考
│   ├── user-guide/              # 用户指南
│   ├── integration/             # 集成指南
│   ├── api/                     # 服务端 API
│   └── troubleshooting.md       # 故障排除
├── mkdocs.yml                   # MkDocs 配置
├── requirements.txt             # 文档构建依赖
├── doc.json                     # SDK 结构化说明数据
├── SDK.md                       # 原始 SDK 文档
└── SDK-callback.md              # 原始回调文档
```

## 编写规范

- 面向开发者解释能力边界，避免只罗列接口名称。
- 接口文档必须包含用途、参数、返回值、错误处理和最小示例。
- 涉及异步行为时，明确说明同步返回值与回调事件的关系。
- 涉及凭据、API Key、Token、Cookie 时，避免在示例中出现真实密钥。
- 新增页面后同步更新 `mkdocs.yml` 的 `nav` 配置。

## 部署

文档通过 GitHub Actions 自动部署到 GitHub Pages。推送到 `main` 或 `master` 分支后会触发构建和发布流程。

手动验证建议：

```bash
mkdocs build --strict
```

## 贡献

欢迎通过 Issue 或 Pull Request 改进文档。提交前请确认本地构建通过，并尽量提供可运行的代码片段或可复现的问题描述。

## License

MIT
