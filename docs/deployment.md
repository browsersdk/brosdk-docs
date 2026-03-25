# 部署指南

本文档说明如何将 BroSDK 文档部署到 GitHub Pages。

## 架构概述

```
GitHub Repository (brosdk-docs)
         │
         │ push to main/master
         ▼
GitHub Actions (deploy.yml)
         │
         │ 1. Checkout code
         │ 2. Setup Python
         │ 3. Install dependencies
         │ 4. Build MkDocs
         │ 5. Deploy to GitHub Pages
         ▼
GitHub Pages
         │
         ▼
https://browsersdk.github.io/brosdk-docs/
```

## 前提条件

1. **GitHub 仓库**：文档必须托管在 GitHub 仓库中
2. **GitHub Pages 启用**：在仓库设置中启用 GitHub Pages
3. **分支配置**：GitHub Pages 应配置为使用 `gh-pages` 分支

## 配置步骤

### 1. 启用 GitHub Pages

1. 进入仓库 **Settings** → **Pages**
2. 在 **Build and deployment** 部分：
   - **Source**: 选择 `GitHub Actions`
3. 保存配置

### 2. 验证 GitHub Actions 配置

GitHub Actions workflow 文件位于：`.github/workflows/deploy.yml`

**关键配置项**：

```yaml
on:
  push:
    branches:
      - main      # 触发部署的分支
      - master

permissions:
  contents: read
  pages: write    # 需要写入 Pages 的权限
  id-token: write

jobs:
  deploy:
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-latest
```

### 3. 安装依赖

项目依赖在 `requirements.txt` 中定义：

```bash
# 核心依赖
mkdocs>=1.6.0
mkdocs-material>=9.5.0
markdown>=3.7
pymdown-extensions>=10.0
```

### 4. 本地测试构建

在推送之前，建议在本地测试构建：

```bash
# 安装依赖
pip install -r requirements.txt

# 预览文档（实时刷新）
mkdocs serve

# 构建文档（生成静态文件）
mkdocs build

# 清理并重新构建
mkdocs build --clean
```

### 5. 推送并触发部署

```bash
# 提交更改
git add .
git commit -m "docs: 更新文档内容"

# 推送到 main 分支（触发自动部署）
git push origin main
```

### 6. 监控部署状态

1. 进入仓库 **Actions** 标签页
2. 查看最新的 workflow 运行状态
3. 点击运行详情查看构建日志
4. 部署成功后，访问生成的 URL

## 配置说明

### mkdocs.yml 关键配置

```yaml
# 站点信息
site_name: BroSDK 文档
site_url: https://browsersdk.github.io/brosdk-docs/

# 仓库信息（用于编辑链接）
repo_name: browsersdk/brosdk-docs
repo_url: https://github.com/browsersdk/brosdk-docs

# 主题配置
theme:
  name: material
  language: zh  # 中文
  palette:
    - media: "(prefers-color-scheme: light)"
      scheme: default
    - media: "(prefers-color-scheme: dark)"
      scheme: slate

# 导航结构
nav:
  - 首页：index.md
  - 快速开始：quick-start.md
  ...
```

### GitHub Actions 权限

Workflow 需要以下权限：

```yaml
permissions:
  contents: read      # 读取仓库内容
  pages: write        # 写入 GitHub Pages
  id-token: write     # 用于 Pages 部署的身份验证
```

## 常见问题

### 问题：部署失败，提示权限不足

**解决方案**：
1. 检查 workflow 文件中的 `permissions` 配置
2. 确保仓库设置中允许 GitHub Actions 部署到 Pages

### 问题：文档更新后 Pages 没有变化

**可能原因**：
1. 推送到错误的分支（应推送到 `main` 或 `master`）
2. GitHub Actions 运行失败
3. 浏览器缓存问题

**排查步骤**：
1. 检查 **Actions** 标签页，确认 workflow 是否成功运行
2. 查看构建日志，确认没有错误
3. 强制刷新浏览器（Ctrl+F5）

### 问题：本地构建成功，但 GitHub Actions 构建失败

**可能原因**：
1. 依赖版本不一致
2. 文件路径大小写敏感（Linux vs Windows）
3. 缺少必要的文件

**解决方案**：
1. 确保 `requirements.txt` 包含所有依赖
2. 使用相对路径，避免绝对路径
3. 在本地使用相同环境测试（Ubuntu）

## 手动触发部署

如需手动触发部署（不推送代码）：

1. 进入仓库 **Actions** 标签页
2. 选择 **Deploy MkDocs to GitHub Pages** workflow
3. 点击 **Run workflow**
4. 选择分支，点击 **Run workflow**

## 部署最佳实践

1. **预览后再部署**：使用 `mkdocs serve` 本地预览
2. **小步提交**：每次只修改少量内容，便于回滚
3. **检查链接**：确保所有内部链接正确
4. **测试构建**：推送前运行 `mkdocs build --clean`
5. **监控日志**：部署失败时查看 Actions 日志

## 相关资源

- **MkDocs 官方文档**：https://www.mkdocs.org/
- **Material for MkDocs**：https://squidfunk.github.io/mkdocs-material/
- **GitHub Pages 文档**：https://pages.github.com/
- **GitHub Actions 文档**：https://docs.github.com/en/actions
