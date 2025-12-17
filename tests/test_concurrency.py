import socket
import threading
import time
import random
import sys

# 配置
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8000
CLIENT_COUNT = 100       # 模拟 100 个并发客户端
MESSAGES_PER_CLIENT = 50 # 每个客户端发送 50 条消息

# 统计
lock = threading.Lock()
success_count = 0
fail_count = 0

def client_task(client_id):
    global success_count, fail_count
    try:
        # 1. 建立连接
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_IP, SERVER_PORT))
        
        for i in range(MESSAGES_PER_CLIENT):
            # 2. 构造消息，带上 ID 防止数据混淆
            msg = f"Client-{client_id}-Msg-{i}".encode('utf-8')
            
            # 3. 发送数据
            sock.sendall(msg)
            
            # 4. 接收回显
            # 注意：TCP 是流协议，recv(1024) 不一定能一次性收完，
            # 但对于短消息 Echo 测试通常没问题。严格来说应该循环 recv。
            data = sock.recv(1024)
            
            # 5. 验证数据
            if data == msg:
                # 稍微随机 sleep 一下，模拟真实网络的不均匀请求
                time.sleep(random.uniform(0.001, 0.01))
            else:
                print(f"[Error] Client {client_id}: Expected {msg}, got {data}")
                with lock:
                    fail_count += 1
                sock.close()
                return

        # 6. 完成任务，关闭连接
        sock.close()
        with lock:
            success_count += 1
            if success_count % 10 == 0:
                print(f"Progress: {success_count}/{CLIENT_COUNT} clients finished.")

    except Exception as e:
        print(f"[Exception] Client {client_id}: {e}")
        with lock:
            fail_count += 1

def main():
    print(f"Starting {CLIENT_COUNT} clients, aiming at {SERVER_IP}:{SERVER_PORT}...")
    threads = []
    start_time = time.time()

    # 启动多线程
    for i in range(CLIENT_COUNT):
        t = threading.Thread(target=client_task, args=(i,))
        threads.append(t)
        t.start()

    # 等待所有线程结束
    for t in threads:
        t.join()

    end_time = time.time()
    duration = end_time - start_time
    total_reqs = CLIENT_COUNT * MESSAGES_PER_CLIENT

    print("-" * 40)
    print(f"Test Finished in {duration:.2f} seconds")
    print(f"Total Clients: {CLIENT_COUNT}")
    print(f"Total Requests: {total_reqs}")
    print(f"Successful Clients: {success_count}")
    print(f"Failed Clients: {fail_count}")
    print(f"QPS (Queries Per Second): {total_reqs / duration:.2f}")

if __name__ == "__main__":
    main()