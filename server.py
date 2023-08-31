import socket, sys, struct, os, gc
import threading

HOST = "0.0.0.0"
PORT = 8008

connections = [2]
lock = threading.Lock()

def bannerPW():
    print ("""
        0       lock workstation
        1       lock off
        2       reboot
        3       shut down
        4       power off
    """)

#RECEIVE FILE FROM CLIENT 
def receiveFile(conn, filePath):
    with open(filePath, "wb+") as file:
        # Receive the file size from the client
        fileSizeData = conn.recv(4)
        size = struct.unpack('!I', fileSizeData)[0]
        print("Size file receive ", +size)

        # Receive the data file
        received = 0
        while received < size:
            data = conn.recv(8096)
            file.write(data)
            received += len(data)
        del data
        del fileSizeData
        received = 0
        gc.collect()
        file.close()
        print("File received successfully.")

#SEND FILE TO CLIENT
def sendFile(conn, filePath):
    with open(filePath, 'rb') as file:
        # send size file to client
        file_data = file.read()
        file_size = len(file_data)
        conn.sendall(file_size.to_bytes(4, byteorder='big'))
        
        # send data file to client
        chunk_size = 8096
        offset = 0
        while offset < file_size:
            chunk = file_data[offset:offset + chunk_size]
            conn.sendall(chunk)
            offset += chunk_size
        
        file.close()
        print("File uploaded successfully")

def handleClient(conn):
   with conn:
        print(f"New client connected: {conn.getpeername()}")

        while True:
            try:
                data = conn.recv(8096).decode('utf-8', errors='replace')
                print(data)

                cmd = input('> ').upper()
                conn.send(cmd.encode('utf-8', errors='replace'))

                if cmd == "KPC":
                    pid = input("Enter PID of process to kill: ")
                    conn.send(pid.encode('utf-8', errors='replace'))
                if cmd == "DF":
                    filePathDown = input("Enter file path you want to download from client: ")
                    conn.send(filePathDown.encode('utf-8', errors='replace'))
                    filePathSave = input("Enter the file path to save: ")
                    conn.send(filePathSave.encode('utf-8', errors='replace'))
                    receiveFile(conn, filePathSave)
                    del filePathDown
                    del filePathSave
                    gc.collect()
                if cmd == "UF":
                    filePathUp = input("Enter file path you want to upload to client: ")
                    filePathSave = input("Enter the file path to save on the client: ")
                    conn.send(filePathSave.encode('utf-8', errors='replace'))
                    sendFile(conn, filePathUp)
                    del filePathSave
                    del filePathUp
                    gc.collect()
                if cmd == "EXIT":
                    print("Connection closed!")
                    conn.close()
                    break
                if cmd == "PW":
                    bannerPW()
                    buff = input("Enter a number to do something: ")
                    conn.send(buff.encode('utf-8', errors='replace'))
            except (ConnectionResetError, ConnectionAbortedError):
                print(f"Client {conn.getpeername()} disconnected unexpectedly.")
                break

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((HOST, PORT))
        sock.listen(5)
        print(f"Server listening on {HOST}:{PORT}")

        while True:
            conn, addr = sock.accept()
            with lock:
                connections.append(conn)
            client_thread = threading.Thread(target=handleClient, args=(conn,))
            client_thread.start()

if __name__ == "__main__":
    main()