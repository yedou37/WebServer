#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Network utilities for non-blocking socket operations in tests.
"""

import socket
import select
import time


def send_and_receive_with_timeout(sock, request_data, timeout=5.0):
    """
    Send request and receive response with non-blocking I/O and timeout.
    
    Args:
        sock: Connected socket object
        request_data: Data to send
        timeout: Maximum time to wait for response
        
    Returns:
        bytes: Response data received
    """
    # Send the request
    sock.sendall(request_data)
    
    # Receive data with improved handling to prevent indefinite blocking
    response = b""
    start_time = time.time()
    
    while True:
        # Check if we've exceeded our timeout
        if time.time() - start_time > timeout:
            break
            
        # Use select to check if data is available to read
        ready = select.select([sock], [], [], 0.1)  # Short timeout of 0.1 seconds
        if ready[0]:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
                
                # Simple heuristic: if we received less than full buffer, 
                # we've likely got all the data
                if len(chunk) < 4096:
                    break
            except socket.timeout:
                break
            except BlockingIOError:
                # No data available right now, continue loop
                continue
        else:
            # No data ready, if we already have data, break
            if response:
                break
    
    return response


def create_nonblocking_socket():
    """
    Create and return a non-blocking socket.
    
    Returns:
        socket: Non-blocking socket object
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setblocking(False)
    return sock


def connect_with_timeout(sock, address, timeout=5.0):
    """
    Connect socket with timeout using non-blocking approach.
    
    Args:
        sock: Socket object
        address: Tuple of (host, port)
        timeout: Connection timeout in seconds
        
    Returns:
        bool: True if connected successfully, False otherwise
    """
    try:
        sock.connect(address)
    except BlockingIOError:
        # This is expected for non-blocking socket
        pass
    
    # Wait for socket to be ready for writing (connection established)
    ready = select.select([], [sock], [], timeout)
    return bool(ready[1])


def receive_available_data(sock, timeout=0.1):
    """
    Receive any available data without blocking indefinitely.
    
    Args:
        sock: Connected socket object
        timeout: Time to wait for data
        
    Returns:
        bytes: Available data or empty bytes if none available
    """
    ready = select.select([sock], [], [], timeout)
    if ready[0]:
        try:
            return sock.recv(4096)
        except:
            pass
    return b''