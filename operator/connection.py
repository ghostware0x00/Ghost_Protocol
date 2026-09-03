import socket
import struct
import protocol
import console
import colors

command_dispather = ["help", "exit"]
TARGET_PORT = 9000
TARGET_IP = "127.0.0.1"
HOST = "0.0.0.0"


# [count]              4 bytes

# for every session:
#     [session_id]     4 bytes
#     [ip_length]       4 bytes
#     [ip_address]      variable
#     [port]            2 bytes

def receive_exact(operator_socket, length): # function to receive the data in chunks 
    data = b""
    while len(data) < length:
        chunk = operator_socket.recv(length - len(data))
        if not chunk:
            raise ConnectionError(f"{colors.Fore.RED} [!] c2 server closed connection")
        data += chunk
    return data



def receive_sessions(operator_socket): # deserialize sessionInfo bytes and display total sessions and session ids
    session_count_bytes = receive_exact(operator_socket, 4) # receive total number of sessions present
    sessionCount = struct.unpack("!I", session_count_bytes)[0] # !I is used to unpack network bytes (Big endian) data into Python integer
    # struct.unpack() returns a tuple, so [0] is used to get only one value
    session_info = [] # list of dictionaries
    for _ in range(sessionCount):
        # receive agent session_id
        agent_session_id_bytes = receive_exact(operator_socket, 4) 
        agent_session_id = struct.unpack("!I", agent_session_id_bytes)[0]

        # receive agent ip address length
        agent_ip_len_bytes = receive_exact(operator_socket, 4)
        agent_ip_len = struct.unpack("!I", agent_ip_len_bytes)[0] #!I or !H tells python how to interpret those bytes like !I = unsigned 4 byte integer or uint32_t

        # receive agent ip address
        agent_ip_bytes = receive_exact(operator_socket, agent_ip_len)
        agent_ip = agent_ip_bytes.decode() # since ip address is a strin object

        #receive agent port
        agent_port_bytes = receive_exact(operator_socket, 2)
        agent_port = struct.unpack("!H", agent_port_bytes)[0]# since reading 2 bytes so !H = unsigned short = 2 bytes

        session_info.append({
            "agent_sid": agent_session_id,
            "agent_ip": agent_ip,
            "agent_port": agent_port,
        })
    #session_info.sort() we cannot directly use .sort() because python doesn't know to sort dictionary values.
    # because sorting needs to happen based on a comparison so we need to use a different method
    session_info = sorted(session_info, key=lambda a : a["agent_sid"]) # sorting the session_info list containing dictionary values based on session_id
    return session_info, sessionCount



def connect(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as operator_socket:
        try:
            operator_socket.connect((TARGET_IP, TARGET_PORT))
            #print(f"[+] operator connected to server successfully")
            packet_bytes = protocol.command_packet_formation(command)
            operator_socket.sendall(packet_bytes)
            while True:
                if command == "sessions":
                    session_info, sessionCount = receive_sessions(operator_socket) # passing operator socket and 4 bytes cuz number of session ids are 4 bytes
                    console.display_sessionInfo(session_info, sessionCount)
                    break
        except OSError as e:
            print(f"{colors.Fore.RED}[!] couldn't connect to c2 server\n")
            operator_socket.close()