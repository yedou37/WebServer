# Web Server 作业说明

## 1. 编译方法

### 工具版本和依赖库

- C++标准: C++20
- 编译器: clang++ (需要支持 C++20)
- 构建系统: CMake 3.10 或更高版本
- 系统依赖: Linux 系统（支持 epoll），pthread 库
- 其他依赖: 无特殊第三方库依赖

### 编译步骤

```bash
# 进入项目根目录
cd WebServer

# 创建构建目录
mkdir build && cd build

# 配置构建系统
cmake ..

# 编译项目
make
```

编译完成后，会在`build/bin/`目录下生成可执行文件`HttpServerMain`。

## 2. 运行方法

### 启动服务器

```bash
# 在build目录下执行
./bin/HttpServerMain
```

服务器将在端口 31885 上监听 HTTP 请求。

### 访问 URL 测试功能

#### 静态文件服务

1. 访问 HTML 文件:

   - `http://localhost:31885/html/test.html` - 包含图片和表单的测试页面
   - `http://localhost:31885/html/noimg.html` - 不包含图片的测试页面

2. 访问图片文件:

   - `http://localhost:31885/img/logo.jpg` - 测试图片文件

3. 访问文本文件:
   - `http://localhost:31885/txt/test.txt` - 测试文本文件

#### 登录功能测试

在 HTML 表单中输入以下凭据进行测试：

- 用户名: 20210001
- 密码: 0001

提交表单后，将发送 POST 请求到`/dopost`路径，服务器会验证凭据并返回相应结果页面。

### 文件路径安排

静态文件需要按照以下结构放置，相对于可执行文件([./bin/HttpServerMain](file:///root/WebServer/build/bin/HttpServerMain))的位置：

```
WebServer/                  # 项目根目录
├── build/
│   ├── bin/
│   │   └── HttpServerMain  # 可执行文件
│   ├── html/               # HTML文件目录
│   │   ├── test.html
│   │   └── noimg.html
│   ├── img/                # 图片文件目录
│   │   └── logo.jpg
│   └── txt/                # 文本文件目录
│       └── test.txt
```

注意：静态资源文件夹（html、img、txt）需要放在 build 目录下，与可执行文件在同一层级，这样才能被服务器正确访问。

在执行`make`命令编译项目时，CMake 会自动将这些资源文件从项目根目录复制到 build 目录下的相应位置。

## 3. 功能说明

### HTTP GET 请求处理

- 支持静态文件服务，包括 HTML、图片和文本文件
- 正确设置 Content-Type 响应头以匹配文件类型
- 对于不存在的文件返回 404 错误页面

### HTTP POST 请求处理

- 处理登录表单提交到`/dopost`路径的请求
- 解析表单数据中的用户名(login)和密码(pass)
- 验证凭据（用户名: 20210001，密码: 0001）
- 返回登录成功或失败的 HTML 页面

### 并发处理

- 服务器基于事件驱动的 Reactor 模式，支持多客户端并发访问
- 使用 epoll 进行高效的 I/O 多路复用
- 采用 One Loop Per Thread 模型处理并发连接
