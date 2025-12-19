#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Integration tests for non-blocking read operations.
"""

import socket
import time
import select
import pytest
import sys
import os

# Add parent directory to path to import network_utils
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import network_utils


class TestNonBlockingOperations:
    def setup_method(self):
        """Setup for each test method"""
        self.sock = None
    
    def teardown_method(self):
        """Cleanup after each test method"""
        if self.sock:
            self.sock.close()
    
    def test_non_blocking_connect_and_read(self):
        """Test non-blocking connect and read operations"""
        # Create non-blocking socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setblocking(False)
        
        # Attempt to connect (will raise BlockingIOError with non-blocking socket)
        try:
            self.sock.connect(('127.0.0.1', 8080))
        except BlockingIOError:
            # This is expected for non-blocking socket
            pass
        
        # Wait for socket to be ready for writing (connection established)
        ready = select.select([], [self.sock], [], 5)  # 5 second timeout
        assert ready[1], "Socket connection timed out"
        
        # Test sending data
        test_data = b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        bytes_sent = self.sock.send(test_data)
        assert bytes_sent == len(test_data)
        
        # Wait for data to be available for reading
        ready = select.select([self.sock], [], [], 2)  # 2 second timeout
        if ready[0]:
            # Data is available to read
            data = self.sock.recv(4096)
            assert isinstance(data, bytes)
        else:
            # No data available within timeout, which is acceptable
            pass
    
    def test_select_based_non_blocking_read(self):
        """Test using select for non-blocking reads"""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5)
        
        try:
            self.sock.connect(('127.0.0.1', 8080))
        except Exception:
            pytest.skip("Server not running, skipping test")
        
        # Send a request
        request = b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        self.sock.sendall(request)
        
        # Use select to check if data is available
        ready = select.select([self.sock], [], [], 3)  # 3 second timeout
        if ready[0]:
            data = self.sock.recv(4096)
            assert len(data) > 0
        # If not ready, that's fine - means no data was available promptly
    
    @pytest.mark.slow
    def test_timeout_handling(self):
        """Test proper timeout handling"""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(2)  # 2 second timeout
        
        try:
            self.sock.connect(('127.0.0.1', 8080))
        except Exception:
            pytest.skip("Server not running, skipping test")
        
        # Try to read from socket with nothing to read
        start_time = time.time()
        try:
            data = self.sock.recv(1024)
        except socket.timeout:
            # This is expected behavior
            pass
        
        elapsed = time.time() - start_time
        # Should have waited approximately 2 seconds
        assert 1.5 <= elapsed <= 2.5

    def test_utility_send_and_receive(self):
        """Test our utility function for non-blocking send/receive"""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5)
        
        try:
            self.sock.connect(('127.0.0.1', 8080))
        except Exception:
            pytest.skip("Server not running, skipping test")
            
        # Test our utility function
        request = b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        response = network_utils.send_and_receive_with_timeout(self.sock, request, timeout=3.0)
        # Just verify it returns bytes, content depends on server
        assert isinstance(response, bytes)


if __name__ == "__main__":
    test = TestNonBlockingOperations()
    test.setup_method()
    try:
        test.test_non_blocking_connect_and_read()
        print("test_non_blocking_connect_and_read passed")
    finally:
        test.teardown_method()