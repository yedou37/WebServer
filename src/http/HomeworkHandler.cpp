#include "http/HomeworkHandler.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
std::unordered_map<std::string, std::string> parseQueryString(const std::string& query);
void HomeworkHandler::handle(const HttpRequest& req, HttpResponse* resp) {
  std::cout << "Handling request: " << " " << req.path() << std::endl;

  switch (req.GetMethod()) {
    case HttpRequest::Method::POST:
      std::cout << "Processing POST request" << std::endl;
      handlePost(req, resp);
      break;
    case HttpRequest::Method::GET:
      std::cout << "Processing GET request" << std::endl;
      handleGet(req, resp);
      break;
    default:
      resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
      resp->SetStatusMessage("Not Found");
      resp->SetBody("<html><body>Unsupported method</body></html>");
      resp->SetContentType("text/html");
      break;
  }
}

void HomeworkHandler::handlePost(const HttpRequest& req, HttpResponse* resp) {
  std::cout << "Handle POST: path=" << req.path() << std::endl;

  if (req.path() == "/dopost") {
    // 从 body 中获取登录凭据
    std::string body = req.body();
    std::cout << "body: " << body << "\n";

    // 解析POST请求体中的表单数据
    std::unordered_map<std::string, std::string> params = parseQueryString(body);
    std::string login = params["login"];
    std::string pass = params["pass"];

    std::cout << "POST credentials - login: " << login << ", pass: " << pass << std::endl;

    // 验证登录凭据
    if (login == "20210001" && pass == "0001") {
      std::cout << "POST Login Success" << std::endl;
      resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
      resp->SetStatusMessage("OK");
      resp->SetBody("<html><body><h1>Login Success</h1><p>User: " + login + "</p><p>Pass: " + pass + "</p></body></html>");
    } else {
      std::cout << "POST Login Failed" << std::endl;
      resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
      resp->SetStatusMessage("OK");
      resp->SetBody("<html><body><h1>Login Failed</h1><p>User: " + login + "</p><p>Pass: " + pass + "</p></body></html>");
    }
    resp->SetContentType("text/html");
  } else {
    std::cout << "POST Page Not Found: " << req.path() << std::endl;
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetBody("<html><body>Page Not Found</body></html>");
    resp->SetContentType("text/html");
  }
}

// 解析查询参数的辅助函数
std::unordered_map<std::string, std::string> parseQueryString(const std::string& query) {
  std::cout << "Parsing query string: " << query << std::endl;
  std::unordered_map<std::string, std::string> params;
  size_t start = 0;

  while (start < query.length()) {
    size_t equalPos = query.find('=', start);
    if (equalPos == std::string::npos) break;

    size_t ampPos = query.find('&', equalPos);
    if (ampPos == std::string::npos) ampPos = query.length();

    std::string key = query.substr(start, equalPos - start);
    std::string value = query.substr(equalPos + 1, ampPos - equalPos - 1);

    std::cout << "Parsed param: " << key << "=" << value << std::endl;

    // 简单解码：将 %XX 转换为字符
    // 这里只做简单的 '+' 转换为空格的处理
    for (size_t i = 0; i < value.length(); ++i) {
      if (value[i] == '+') {
        value[i] = ' ';
      }
    }

    params[key] = value;
    start = ampPos + 1;
  }

  return params;
}

void HomeworkHandler::handleGet(const HttpRequest& req, HttpResponse* resp) {
  std::cout << "Handle GET: path=" << req.path() << ", query=" << req.query() << std::endl;

  // 检查是否是登录请求
  if (req.path() == "/login") {
    // 从查询参数中获取用户名和密码
    std::string queryString = req.query();
    auto params = parseQueryString(queryString);

    auto loginIt = params.find("login");
    auto passIt = params.find("pass");

    if (loginIt != params.end() && passIt != params.end()) {
      std::string login = loginIt->second;
      std::string pass = passIt->second;

      std::cout << "GET credentials - login: " << login << ", pass: " << pass << std::endl;

      // 验证登录凭据
      if (login == "20210001" && pass == "0001") {
        std::cout << "GET Login Success" << std::endl;
        resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
        resp->SetStatusMessage("OK");
        resp->SetBody("<html><body><h1>Login Success</h1><p>User: " + login + "</p><p>Pass: " + pass + "</p></body></html>");
      } else {
        std::cout << "GET Login Failed" << std::endl;
        resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
        resp->SetStatusMessage("OK");
        resp->SetBody("<html><body><h1>Login Failed</h1><p>User: " + login + "</p><p>Pass: " + pass + "</p></body></html>");
      }
    } else {
      std::cout << "Missing login parameters in GET request" << std::endl;
      resp->SetStatusCode(HttpResponse::HttpStatusCode::k400BadRequest);
      resp->SetStatusMessage("Bad Request");
      resp->SetBody("<html><body>Missing login parameters</body></html>");
    }
    resp->SetContentType("text/html");
    return;
  }

  std::string filePath = "." + req.path();
  std::cout << "Serving file: " << filePath << std::endl;

  // 检查文件是否存在
  if (!std::filesystem::exists(filePath)) {
    std::cout << "File not found: " << filePath << std::endl;
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetBody("<html><body>File Not Found</body></html>");
    resp->SetContentType("text/html");
    return;
  }

  // 读取文件内容
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    std::cout << "Cannot open file: " << filePath << std::endl;
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetBody("<html><body>Cannot Open File</body></html>");
    resp->SetContentType("text/html");
    return;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  std::cout << "File served successfully: " << filePath << ", size: " << content.length() << " bytes" << std::endl;

  resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
  resp->SetStatusMessage("OK");
  resp->SetBody(content);
  resp->SetContentType(getFileType(filePath));
}

std::string HomeworkHandler::getFileType(const std::string& filePath) {
  size_t dotPos = filePath.find_last_of('.');
  if (dotPos == std::string::npos) {
    return "text/plain";
  }

  std::string extension = filePath.substr(dotPos);
  if (extension == ".html" || extension == ".htm") {
    return "text/html";
  } else if (extension == ".jpg" || extension == ".jpeg") {
    return "image/jpeg";
  } else if (extension == ".png") {
    return "image/png";
  } else if (extension == ".gif") {
    return "image/gif";
  } else if (extension == ".css") {
    return "text/css";
  } else if (extension == ".js") {
    return "application/javascript";
  } else {
    return "text/plain";
  }
}