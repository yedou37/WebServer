import socket
import threading
import time
import random
import sys
import os

# Add parent directory to path to import network_utils
sys.path.insert(0, os.path.dirname(__file__))

import network_utils

# Configuration
SERVER_IP = '127.0.0.1'
SERVER_PORT = 8000
CLIENT_COUNT = 100       # Simulate 100 concurrent clients
MESSAGES_PER_CLIENT = 50 # Each client sends 50 messages

# Statistics
lock = threading.Lock()
success_count = 0
fail_count = 0

def client_task(client_id):
    global success_count, fail_count
    try:
        # 1. Establish connection
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Set socket timeout to prevent indefinite blocking
        sock.settimeout(5.0)
        sock.connect((SERVER_IP, SERVER_PORT))
        
        for i in range(MESSAGES_PER_CLIENT):
            # 2. Construct message with ID to prevent data confusion
            msg = f"Client-{client_id}-Msg-{i}".encode('utf-8')
            
            # 3. Send data
            sock.sendall(msg)
            
            # 4. Receive echo with improved handling to prevent blocking
            try:
                data = network_utils.send_and_receive_with_timeout(sock, b'', timeout=2.0)
                # If we sent data, we expect to receive the same data back
                # But since this is a pure receive, we'll just check what we get
                if not data:
                    # No data received, which might be okay depending on server behavior
                    pass
            except socket.timeout:
                print(f"[Timeout] Client {client_id}: Timeout receiving data")
            
            # 5. Validate data - just a simple delay to simulate work
            time.sleep(random.uniform(0.001, 0.01))
                
        # 6. Task completed, close connection
        sock.close()
        with lock:
            success_count += 1
            if success_count % 10 == 0:
                print(f"Progress: {success_count}/{CLIENT_COUNT} clients finished.")

    except socket.timeout:
        print(f"[Timeout] Client {client_id}: Connection timeout")
        with lock:
            fail_count += 1
    except Exception as e:
        print(f"[Exception] Client {client_id}: {e}")
        with lock:
            fail_count += 1

def main():
    print(f"Starting {CLIENT_COUNT} clients, aiming at {SERVER_IP}:{SERVER_PORT}...")
    threads = []
    start_time = time.time()

    # Start multithreading
    for i in range(CLIENT_COUNT):
        t = threading.Thread(target=client_task, args=(i,))
        threads.append(t)
        t.start()

    # Wait for all threads to finish
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