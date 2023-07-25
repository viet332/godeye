import socket, sys, struct

HOST = "192.168.20.59"
PORT = 8008

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((HOST, PORT))
sock.listen(1)
conn, addr = sock.accept()

def receiveFile(sock, filePath):
    with open(filePath, "wb") as file:
        # Receive the file size from the client
        fileSizeData = conn.recv(4)
        size = struct.unpack('!I', fileSizeData)[0]
        print("Size file receive ", +size)

        # Receive the data file
        received = 0
        while received < size:
            data = conn.recv(min(1024, size - received))
            file.write(data)
            received += len(data)

        file.close()

    
while True:
    data = conn.recv(1024).decode('utf-8', errors='replace')
    print(data)

    cmd = input('> ').upper()
    conn.send(cmd.encode('utf-8', errors='replace'))

    if cmd == "DF":
        filePathDown = input("Enter file path you want to download from client: ")
        conn.send(filePathDown.encode('utf-8', errors='replace'))
        filePathSave = input("Enter the file path to save: ")
        conn.send(filePathSave.encode('utf-8', errors='replace'))
        receiveFile(conn, filePathSave)
        print("File received successfully.")
    if cmd == "EXIT":
        print("Connection closed!")
        break
        
