import socket
import pytest
import sys
import os

# Add parent directory to path to import network_utils
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import network_utils

SERVER_IP = "127.0.0.1"
SERVER_PORT = 8080

def send_req(request_data, timeout=5):
    """
    Send request and receive response with non-blocking I/O and timeout
    """
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)  # Set socket timeout
    
    try:
        s.connect((SERVER_IP, SERVER_PORT))
        response = network_utils.send_and_receive_with_timeout(s, request_data, timeout)
        return response
    except Exception as e:
        # Return empty response in case of error
        return b""
    finally:
        s.close()

def test_200_ok():
    req = b"GET /hello HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
    resp = send_req(req)
    assert b"200 OK" in resp
    assert b"Content-Length" in resp

def test_404_not_found():
    req = b"GET /not_exist_page HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
    resp = send_req(req)
    assert b"404 Not Found" in resp