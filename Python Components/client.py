import socket

# 1. Create a socket object
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 2. Bind the socket to a specific IP and Port 
Host = '127.0.0.1'
Port = 8081