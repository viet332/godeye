import socket, sys

HOST = "192.168.20.59"
PORT = 8008

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((HOST, PORT))
sock.listen(5)
conn, addr = sock.accept()

while 1:
    data = conn.recv(8192)
    print(data.decode('utf-8', errors='replace'))

    cmd = input('> ').upper()
    conn.send(cmd.encode('utf-8', errors='replace'))


