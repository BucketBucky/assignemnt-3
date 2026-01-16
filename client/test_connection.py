import socket
import sys

try:
    print("Attempting to connect to 127.0.0.1:7777...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5) # 5 שניות טיימאאוט
    s.connect(('127.0.0.1', 7777))
    print("SUCCESS! Connected via Python.")
    s.close()
except Exception as e:
    print(f"FAILED to connect via Python: {e}")