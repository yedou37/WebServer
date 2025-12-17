import socket
import time

SERVER_IP = '127.0.0.1'
SERVER_PORT = 8000
CONNECTION_COUNT = 1000 # 尝试建立 1000 个连接

sockets = []

def main():
    print(f"Attempting to establish {CONNECTION_COUNT} connections...")
    
    for i in range(CONNECTION_COUNT):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((SERVER_IP, SERVER_PORT))
            sockets.append(s)
            
            if (i + 1) % 100 == 0:
                print(f"Connected: {i + 1}")
        except Exception as e:
            print(f"Failed at connection {i}: {e}")
            break
            
    print(f"Established {len(sockets)} connections.")
    print("Sleeping for 10 seconds to keep connections alive...")
    time.sleep(10)
    
    print("Closing connections...")
    for s in sockets:
        s.close()
    print("Done.")

if __name__ == "__main__":
    main()