import socket

command_dispather = ["help", "exit"]
TARGET_PORT = 9000
TARGET_IP = "127.0.0.1"
HOST = "0.0.0.0"




def connect(command):
    print(f"[+]starting gho$t protocol operator console")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as operator_console:
        try:
            operator_console.connect((TARGET_IP, TARGET_PORT))
            print(f"[+] operator connected to server successfully")
            operator_console.sendall(command.encode())
            while True:
                data = operator_console.recv(4096)
                if not data:
                    operator_console.close()
                    break
        except OSError as e:
            print(f"[-]couldn't connect to c2 server")
            print(f"{e}")
            operator_console.close()


# def receive():
#     with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as operator_console:
#         try:
#             operator_console.bind((HOST, TARGET_PORT))
#             operator_console.listen(1)
#             conn, addr = operator_console.accept()
#             if conn:
#                 while True:
#                     data = conn.recv(4096)
#                     if not data:
#                         conn.close()
#                         break
#                     return data
#         except OSError as e:
#             print(f"couldnt receive output : {e}")
#             operator_console.close()

