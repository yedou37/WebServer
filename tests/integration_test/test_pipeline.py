import socket
import pytest

def test_pipeline_requests():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2.0) # 2秒超时足够了
    s.connect(("127.0.0.1", 8080))

    payload = (
        b"GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n"
        b"GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n"
        b"GET /hello HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
    )

    s.sendall(payload)

    response = b""
    while True:
        try:
            chunk = s.recv(4096)
            if not chunk: # 对端关闭连接 (FIN)
                break
            response += chunk
        except socket.timeout:
            # 超时了，说明数据可能发完了，或者服务器卡住了
            break
        except Exception as e:
            print(f"Socket error: {e}")
            break
            
    s.close()

    # 【调试信息】打印实际收到的响应长度和内容片段
    print(f"\n[DEBUG] Total Received: {len(response)} bytes")
    print(f"[DEBUG] Response Count: {response.count(b'200 OK')}")
    # 打印前 500 个字符看看内容
    print(f"[DEBUG] Dump:\n{response[:500].decode(errors='ignore')}\n")

    count = response.count(b"200 OK")
    assert count == 3