# 安装 Mermaid 插件

MkDocs 默认不支持 Mermaid 图表，需要安装插件。

## 步骤

### 1. 安装插件

```bash
pip install mkdocs-mermaid2-plugin
```

### 2. 确认配置

`mkdocs.yml` 中已配置：

```yaml
plugins:
  - search
  - mermaid2
```

### 3. 重新构建文档

```bash
mkdocs build
# 或本地预览
mkdocs serve
```

## 使用示例

在 Markdown 中使用 Mermaid：

~~~markdown
```mermaid
flowchart LR
    A[开始] --> B[结束]
```
~~~

## 参考资料

- 插件官网：https://github.com/fralau/mkdocs-mermaid2-plugin
- Mermaid 文档：https://mermaid.js.org/
