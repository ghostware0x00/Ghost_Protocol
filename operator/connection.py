import socket
import cli

command_dispather = ["help", "exit"]
SERVER_PORT = 9000
SERVER_IP = "127.0.0.1"


def server_connect():
    print(f"[+]starting gho$t protocol operator console")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        try:
            server_socket.connect((SERVER_IP, SERVER_PORT))
            print(f"[+] operator connected to server successfully")
            server_socket.sendall(b"HELLO")
            print(f"[+] message sent to {SERVER_IP}:{SERVER_PORT}")
            
        except OSError as e:
            print(f"[-]couldn't connect to c2 server")
            print(f"{e}")
            server_socket.close()
