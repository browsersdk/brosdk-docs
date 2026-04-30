# BroSDK Docs

English | [简体中文](README.md)

[![Deploy MkDocs to GitHub Pages](https://github.com/browsersdk/brosdk-docs/actions/workflows/deploy.yml/badge.svg)](https://github.com/browsersdk/brosdk-docs/actions/workflows/deploy.yml)

`brosdk-docs` is the official documentation site source for BroSDK. It is built with MkDocs Material and covers browser environment management, native SDK APIs, callback mechanisms, language integrations, server APIs, deployment, and troubleshooting.

The documentation is written for backend, desktop, automation testing, AI Agent, and platform product developers who need to integrate BroSDK.

## Online Documentation

```text
https://browsersdk.github.io/brosdk-docs/
```

## Documentation Coverage

| Module | Description |
|--------|-------------|
| Quick Start | The shortest path from SDK initialization to creating, launching, and closing a browser environment |
| User Guide | Environment model, launch parameters, CDP connection, and browser lifecycle |
| Integration Guide | Native C SDK, TypeScript, Rust, and callback integration |
| SDK Reference | Native APIs, parameter structures, return values, and error codes |
| Server API | BroSDK server APIs, request/response formats, and OpenAPI description |
| Deployment Guide | Local, desktop, server-side, and CI/CD deployment guidance |
| Troubleshooting | Initialization, authentication, core download, browser launch, and callback issue diagnosis |

## Relationship With BroSDK

BroSDK is organized across multiple repositories. This repository provides the documentation site and API references.

| Repository | Module | Description |
|------------|--------|-------------|
| [brosdk](https://github.com/browsersdk/brosdk) | Native SDK | C/C++ APIs, headers, and platform dynamic libraries |
| [brosdk-core](https://github.com/browsersdk/brosdk-core) | Browser Core | Browser cores, versions, and runtime dependencies |
| [brosdk-docs](https://github.com/browsersdk/brosdk-docs) | Documentation | Documentation site, API references, and integration guides |
| [brosdk-typescript](https://github.com/browsersdk/brosdk-typescript) | TypeScript SDK | Node.js / Electron integration |
| [brosdk-python](https://github.com/browsersdk/brosdk-python) | Python SDK | Python automation, testing, and AI workflow integration |
| [brosdk-rust](https://github.com/browsersdk/brosdk-rust) | Rust SDK | Rust services, CLIs, and Tauri desktop integration |
| [browser-demo](https://github.com/browsersdk/browser-demo) | Demo | Full server-side and desktop client example |

## Local Development

### Requirements

- Python 3.11+
- pip

### Install Dependencies

```bash
pip install -r requirements.txt
```

Manual installation:

```bash
pip install mkdocs mkdocs-material mkdocs-include-markdown-plugin pymdown-extensions mkdocs-mermaid2-plugin
```

### Start Local Preview

```bash
mkdocs serve
```

Open:

```text
http://127.0.0.1:8000
```

### Build Static Site

```bash
mkdocs build
```

The generated site is written to `site/`.

## Directory Layout

```text
brosdk-docs/
├── docs/
│   ├── index.md                 # Documentation home
│   ├── quick-start.md           # Quick start
│   ├── sdk-reference.md         # SDK reference
│   ├── user-guide/              # User guide
│   ├── integration/             # Integration guide
│   ├── api/                     # Server API
│   └── troubleshooting.md       # Troubleshooting
├── mkdocs.yml                   # MkDocs configuration
├── requirements.txt             # Documentation build dependencies
├── doc.json                     # Structured SDK description data
├── SDK.md                       # Raw SDK documentation
└── SDK-callback.md              # Raw callback documentation
```

## Writing Guidelines

- Explain capability boundaries for developers instead of only listing API names.
- API documentation should include purpose, parameters, return values, error handling, and minimal examples.
- For asynchronous behavior, clearly describe the relationship between synchronous return values and callback events.
- Do not include real credentials in examples involving API keys, tokens, cookies, or proxies.
- Update `mkdocs.yml` navigation when adding new pages.

## Deployment

The site is automatically deployed to GitHub Pages by GitHub Actions. Pushing to `main` or `master` triggers the build and deployment workflow.

Recommended local verification:

```bash
mkdocs build --strict
```

## Contribution

Issues and pull requests are welcome. Before submitting, make sure the documentation builds locally and provide runnable snippets or reproducible issue descriptions where possible.

## License

MIT
