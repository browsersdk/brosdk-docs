# 常见问题与故障排除

本文档汇总了 BroSDK 使用过程中常见的问题和解决方案。

## 目录

- [认证问题](#认证问题)
- [环境管理问题](#环境管理问题)
- [SDK 集成问题](#sdk 集成问题)
- [网络与代理问题](#网络与代理问题)
- [性能优化](#性能优化)

---

## 认证问题

### 问题：获取 User Sign 返回 401 错误

**错误信息**：
```json
{
  "code": 401,
  "msg": "InvalidToken"
}
```

**可能原因**：
1. API Key 已过期或被重置
2. API Key 格式不正确
3. 请求头中缺少 `Authorization` 字段

**解决方案**：
1. 登录用户中心，检查 API Key 状态
2. 确认请求头格式正确：
   ```http
   Authorization: Bearer your_api_key_here
   ```
3. 如 API Key 已泄露，立即重置并更新代码

---

### 问题：User Sign 频繁过期

**现象**：SDK 频繁触发 token 过期警告（事件 10123）

**解决方案**：
1. 增加 User Sign 的有效期（duration 参数）：
   ```json
   {
     "customerId": "user_12345",
     "duration": 604800  // 7 天
   }
   ```
2. 实现自动刷新机制，在收到 10123 事件时立即获取新 Token
3. 缓存 User Sign，避免重复请求

**示例代码**（Node.js）：
```javascript
let userSig = null;
let expireTime = 0;

async function getUserSigWithCache() {
  // 提前 1 小时刷新
  if (userSig && Date.now() < (expireTime - 3600000)) {
    return userSig;
  }
  
  const response = await axios.post('/api/v2/browser/getUserSig', {
    customerId: 'user_12345',
    duration: 604800
  }, {
    headers: { 'Authorization': `Bearer ${API_KEY}` }
  });
  
  userSig = response.data.data.userSig;
  expireTime = response.data.data.expireTime * 1000;
  return userSig;
}
```

---

## 环境管理问题

### 问题：创建环境返回 10701 错误

**错误信息**：
```json
{
  "code": 10701,
  "msg": "EnvQuotaExceeded"
}
```

**原因**：环境数量超过配额限制

**解决方案**：
1. 检查当前环境数量：
   ```bash
   POST /api/v2/browser/page
   ```
2. 清理不用的环境：
   ```bash
   POST /api/v2/browser/destroy
   {
     "envId": "要删除的环境 ID"
   }
   ```
3. 如需更多配额，联系技术支持或升级套餐

---

### 问题：环境创建后无法启动

**现象**：环境创建成功，但 SDK 启动时报错

**可能原因**：
1. 浏览器内核未正确下载
2. 工作目录权限问题
3. 端口被占用

**排查步骤**：
1. 检查内核文件是否存在于指定目录
2. 确认工作目录有读写权限
3. 尝试更换初始化端口：
   ```cpp
   const char *init_req = 
     "{"
     "  \"userSig\": \"...\","
     "  \"workDir\": \"C:/brosdk/data\","
     "  \"port\": 9528"  // 更换端口
     "}";
   ```
4. 查看 SDK 日志获取详细错误信息

---

### 问题：环境名称重复错误（10703）

**错误信息**：
```json
{
  "code": 10703,
  "msg": "EnvNameExist"
}
```

**解决方案**：
1. 使用唯一的环境名称
2. 在名称中添加时间戳或 UUID：
   ```javascript
   const envName = `myenv_${Date.now()}`;
   ```
3. 或者不指定 envName，让系统自动生成

---

## SDK 集成问题

### 问题：SDK 初始化失败

**错误码**：返回负数

**可能原因**：
1. User Sign 无效或过期
2. 工作目录路径不正确
3. 动态库加载失败

**排查步骤**：
1. 验证 User Sign 有效性：
   ```javascript
   // 先调用 getUserSig 获取新的 token
   ```
2. 检查工作目录路径：
   - Windows: 使用绝对路径，如 `C:/brosdk/data`
   - 确保目录存在且有写权限
3. 确认动态库文件存在：
   - Windows: `brosdk.dll`
   - Linux: `brosdk.so`
   - macOS: `brosdk.dylib`

**示例**（正确的工作目录配置）：
```cpp
// Windows - 使用正斜杠或双反斜杠
const char *workDir = "C:/brosdk/data";
// 或
const char *workDir = "C:\\brosdk\\data";

// Linux/macOS
const char *workDir = "/opt/brosdk/data";
```

---

### 问题：回调函数未被调用

**现象**：异步 API 调用后，回调函数未执行

**可能原因**：
1. 未正确注册回调
2. 主线程过早退出
3. 回调函数被垃圾回收（脚本语言）

**解决方案**：
1. 确保在调用异步 API 前注册回调：
   ```cpp
   sdk_register_result_cb(on_result, nullptr);
   ```
2. 保持主线程运行，等待异步操作完成
3. 在脚本语言中，保持回调函数引用：
   ```python
   # Python 示例
   def on_result(code, user_data, data, len):
       print(f"Result: {code}")
   
   # 保持引用
   callback_ref = sdk.register_result_cb(on_result, None)
   ```

---

### 问题：内存泄漏

**现象**：程序运行一段时间后内存持续增长

**原因**：未正确释放 SDK 分配的内存

**解决方案**：
1. 所有 `sdk_*` 函数输出的 `out_data` 必须用 `sdk_free()` 释放：
   ```cpp
   char *out = nullptr;
   size_t out_len = 0;
   
   int32_t rc = sdk_env_create(handle, req, req_len, &out, &out_len);
   
   // 使用 out...
   
   // 必须释放
   sdk_free(out);
   ```
2. 不要使用系统 `free()` 释放 SDK 内存（Windows 尤其重要）
3. 使用智能指针或 RAII 模式管理内存：
   ```cpp
   struct SdkDeleter {
       void operator()(char* p) const { sdk_free(p); }
   };
   
   std::unique_ptr<char, SdkDeleter> out(sdk_output);
   ```

---

## 网络与代理问题

### 问题：代理连接失败

**现象**：环境创建时代理配置无效

**排查步骤**：
1. 验证代理地址格式：
   ```
   正确：socks5://user:pass@192.168.1.100:1080
   错误：192.168.1.100:1080（缺少协议）
   错误：socks5://192.168.1.100（缺少端口）
   ```
2. 测试代理连通性：
   ```bash
   # 使用 curl 测试
   curl --socks5-hostname user:pass@192.168.1.100:1080 https://api.ip.sb/ip
   ```
3. 检查防火墙是否阻止代理端口
4. 尝试不使用代理，确认是代理配置问题

---

### 问题：IP 检测渠道选择

**问题**：不确定选择 `ip2location` 还是 `ipdata`

**建议**：
- **海外代理**：使用 `ip2location`
- **国内代理**：使用 `ipdata`
- **不确定**：先尝试 `ip2location`，如有问题切换

**示例**：
```json
{
  "proxy": "socks5://user:pass@proxy.com:1080",
  "ipChannel": "ip2location",  // 海外代理
  "region": "US"
}
```

---

## 性能优化

### 批量操作

**问题**：大量环境操作导致 API 调用频繁

**优化方案**：
1. 批量创建环境（如支持）
2. 批量查询而非单条查询
3. 使用合适的数据有效期减少 Token 刷新频率

**示例**（批量查询）：
```javascript
// 不推荐：逐条查询
for (const envId of envIds) {
  await axios.post('/api/v2/browser/page', { envIds: [envId] });
}

// 推荐：批量查询
await axios.post('/api/v2/browser/page', { 
  envIds: envIds,  // 一次查询所有
  pageSize: 100 
});
```

---

### 缓存策略

**推荐缓存的内容**：
1. User Sign（在有效期内）
2. 环境列表（定期刷新）
3. 全局指纹配置（很少变化）

**缓存示例**（Redis）：
```javascript
const redis = require('redis');
const client = redis.createClient();

async function getCachedUserSig(customerId) {
  const cached = await client.get(`usersig:${customerId}`);
  if (cached) {
    return JSON.parse(cached);
  }
  
  // 获取新的 User Sign
  const response = await getUserSig(customerId);
  
  // 缓存 6 天（比实际有效期少 1 天）
  await client.setex(
    `usersig:${customerId}`,
    518400,
    JSON.stringify(response)
  );
  
  return response;
}
```

---

### 连接池

**问题**：频繁创建销毁 SDK 实例导致性能下降

**解决方案**：使用连接池模式复用 SDK 实例

**示例**：
```javascript
class SdkPool {
  constructor(size = 5) {
    this.instances = [];
    this.available = [];
    
    for (let i = 0; i < size; i++) {
      const instance = createSdkInstance();
      this.instances.push(instance);
      this.available.push(instance);
    }
  }
  
  async acquire() {
    if (this.available.length === 0) {
      // 等待或创建新实例
      await this.waitForAvailable();
    }
    return this.available.pop();
  }
  
  release(instance) {
    this.available.push(instance);
  }
}
```

---

## 日志与调试

### 启用 SDK 日志

**C API**：
```cpp
// 设置日志级别（如支持）
sdk_set_log_level(1);  // 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR
```

### 查看服务端日志

1. 记录所有 API 请求的 `reqId`
2. 出现问题时提供 `reqId` 给技术支持
3. 使用日志聚合工具（如 ELK）分析

---

## 获取帮助

如果以上方案无法解决问题：

1. **查看完整文档**：https://browsersdk.github.io/brosdk-docs/
2. **提交 Issue**：https://github.com/browsersdk/brosdk-docs/issues
3. **联系技术支持**：support@brosdk.com

**提供以下信息以加快问题解决**：
- 错误码和完整错误信息
- API 请求的 `reqId`
- SDK 版本和操作系统
- 复现步骤
