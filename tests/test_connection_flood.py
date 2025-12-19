import socket
import time
import sys
import os

# Add parent directory to path to import network_utils
sys.path.insert(0, os.path.dirname(__file__))

import network_utils

SERVER_IP = '127.0.0.1'
SERVER_PORT = 8000
CONNECTION_COUNT = 1000 # Attempt to establish 1000 connections

sockets = []

def main():
    print(f"Attempting to establish {CONNECTION_COUNT} connections...")
    
    for i in range(CONNECTION_COUNT):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Set socket timeout to prevent indefinite blocking
            s.settimeout(3.0)
            s.connect((SERVER_IP, SERVER_PORT))
            sockets.append(s)
            
            if (i + 1) % 100 == 0:
                print(f"Connected: {i + 1}")
        except socket.timeout:
            print(f"Timeout at connection {i}")
            break
        except Exception as e:
            print(f"Failed at connection {i}: {e}")
            break
    
    # Test non-blocking reads on a sample of sockets
    print(f"Testing non-blocking reads on {min(10, len(sockets))} sockets...")
    for i, s in enumerate(sockets[:10]):
        try:
            # Send a test message
            test_msg = f"Test message {i}".encode('utf-8')
            s.sendall(test_msg)
            
            # Try to receive with timeout using our utility function
            data = network_utils.receive_available_data(s)
            if data:
                print(f"Socket {i}: Received {len(data)} bytes")
        except Exception as e:
            print(f"Socket {i}: Error during read test: {e}")
    
    print(f"Established {len(sockets)} connections.")
    print("Sleeping for 10 seconds to keep connections alive...")
    time.sleep(10)
    
    print("Closing connections...")
    for s in sockets:
        s.close()
    print("Done.")

if __name__ == "__main__":
    main()