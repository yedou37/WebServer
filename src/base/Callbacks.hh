#pragma once
#include <functional>
#include <memory>

#include "base/Timestamp.hh"
class Buffer;
class TCPConnection;

using TCPConnectionPtr = std::shared_ptr<TCPConnection>;
using ConnectionCallback = std::function<void(const TCPConnectionPtr&)>;
using MessageCallback = std::function<void(const TCPConnectionPtr&, Buffer*, Timestamp)>;
using CloseCallback = std::function<void(const TCPConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TCPConnectionPtr&)>;