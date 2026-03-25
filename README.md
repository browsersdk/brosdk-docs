# BroSDK 文档

[![Deploy MkDocs to GitHub Pages](https://github.com/browsersdk/brosdk-docs/actions/workflows/deploy.yml/badge.svg)](https://github.com/browsersdk/brosdk-docs/actions/workflows/deploy.yml)

BroSDK 用户手册与 API 文档，基于 MkDocs Material 主题构建。

## 在线文档

文档会自动部署到 GitHub Pages，访问地址：

```
https://browsersdk.github.io/brosdk-docs/
```

## 本地开发

### 环境要求

- Python 3.11+
- pip

### 安装依赖

```bash
pip install mkdocs
pip install mkdocs-material
pip install mkdocs-include-markdown-plugin
pip install pymdown-extensions
```

### 启动本地服务器

```bash
mkdocs serve
```

访问 http://127.0.0.1:8000 查看文档

### 构建文档

```bash
mkdocs build
```

构建后的文件在 `site/` 目录中。

## 部署

文档使用 GitHub Actions 自动部署到 GitHub Pages。当推送到 `main` 或 `master` 分支时，会自动触发构建和部署。

### 手动触发部署

1. 修改文档内容
2. 提交并推送到 main 分支
3. GitHub Actions 会自动构建并部署

## 主题配置

文档使用 [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) 主题，具有以下特性：

- 响应式设计
- 搜索功能（支持中文）
- 深色/浅色模式切换
- 代码高亮
- 表格支持
- 导航优化

## 贡献

欢迎提交 Issue 和 Pull Request 来改进文档。

## 许可证

详见项目仓库中的 LICENSE 文件。

## 联系方式

如有问题，请通过 GitHub Issues 联系。
