import socket
import time

def test_fragmented_header():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 8080))

    # 1. 发送请求行
    s.send(b"GET /hello HTTP/1.1\r\n")
    time.sleep(0.1) 
    
    # 2. 发送 Host 头的一部分
    s.send(b"Host: local")
    time.sleep(0.1)
    
    # 3. 发送剩下的部分
    s.send(b"host\r\n\r\n")
    
    resp = s.recv(4096)
    s.close()
    
    # 服务器应该能正确拼凑并处理
    assert b"200 OK" in resp