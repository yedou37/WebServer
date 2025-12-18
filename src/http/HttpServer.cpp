#include "http/HttpServer.hh"

#include "http/HttpContext.hh"
#include "http/HttpRequest.hh"
#include "http/HttpResponse.hh"

void defaultHttpCallback(const HttpRequest& req, HttpResponse* resp) {
  resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
  resp->SetStatusMessage("Not Found");
  resp->SetCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
    : server_(loop, listenAddr, name),
      httpCallback_(defaultHttpCallback)  // 初始化为默认回调
{
  server_.setConnectionCallback([this](const TCPConnectionPtr& conn) { this->onConnection(conn); });

  server_.setMessageCallback(
      [this](const TCPConnectionPtr& conn, Buffer* buf, Timestamp time) { this->onMessage(conn, buf, time); });

  server_.setThreadNum(8);  // NOLINT
}

HttpServer::~HttpServer() = default;

void HttpServer::start() {
  server_.start();
}

void HttpServer::onConnection(const TCPConnectionPtr& conn) {  // NOLINT
  if (conn->connected()) {
    conn->setContext(HttpContext());
  }
}

void HttpServer::onMessage(const TCPConnectionPtr& conn, Buffer* buf, Timestamp receiveTime) {
  // 1. 取出该连接绑定的 Context
  auto* context = std::any_cast<HttpContext>(conn->getMutableContext());

  // 2. 解析请求
  if (!context->ParseRequest(buf, receiveTime)) {
    // 解析出错（比如请求格式错误），发送 400 响应并关闭连接
    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->Shutdown();
    return;
  }

  // 3. 如果已解析完一个完整的 HTTP 请求
  if (context->IsGotAll()) {
    // 3.1 调用 onRequest，它会填充一个 HttpResponse 对象
    onRequest(conn, context->request());

    // 3.2 重置 Context，为处理下一个请求做准备 (Keep-Alive)
    context->reset();
  }
}

void HttpServer::onRequest(const TCPConnectionPtr& conn, const HttpRequest& req) {
  // 1. 判断是长连接还是短连接
  const std::string& connection = req.GetHeader("Connection");
  bool close =
      (connection == "close") || (req.GetVersion() == HttpRequest::Version::HTTP_1_0 && connection != "Keep-Alive");

  // 2. 创建一个 HttpResponse 对象，并将连接状态（长/短）告知它
  HttpResponse response(close);

  // 3. 调用用户注册的业务逻辑回调，填充 Response
  httpCallback_(req, &response);

  // 4. 将 Response 对象序列化到缓冲区中
  Buffer response_buf;
  response.AppendToBuffer(&response_buf);

  // 5. 发送缓冲区中的数据
  conn->Send(&response_buf);
  if (response.CloseConnection()) {
    // 这里需要处理两种情况

    // 情况 A：数据一次性全部发送出去了
    // conn->Send() 发现 output buffer 为空，会尝试直接 ::send()。
    // 如果内核缓冲区足够大，所有数据都被拷贝进去了。
    // 此时，应用程序的任务已经完成。
    if (conn->getOutputBuffferReadableSize() == 0) {
      // 我们可以立即、安全地发起关闭流程。
      conn->Shutdown();
    }
    // 情况 B：数据没能一次性发完，被暂存到了 output buffer
    // 这意味着 TCPConnection 的 Channel 已经注册了 EPOLLOUT 事件，
    // 等待内核缓冲区变得可写。
    else {
      // 我们不能现在就关闭，因为数据还在应用程序的缓冲区里！
      // 正确的做法是：给连接设置一个“写完就关闭”的回调。
      // 这个回调会在 output buffer 被清空时（在 HandleWrite 中）被触发。
      std::weak_ptr<TCPConnection> weak_conn = conn;
      conn->setWriteCompleteCallback([weak_conn](const TCPConnectionPtr&) {
        if (auto conn_ptr = weak_conn.lock()) {
          // 在这里，我们确信所有数据都已交给内核，现在可以安全关闭了。
          conn_ptr->Shutdown();
        }
      });
    }
  }
}