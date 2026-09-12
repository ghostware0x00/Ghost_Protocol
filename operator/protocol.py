import struct

# we are sending data in bytes in network
# in this protocol.py we are arranging what bytes are in what position
# or in what order based on which we can read them in recv()
# and run the respective function or operation


MESSAGE_COMMAND = 1
MESSAGE_SHELL_START = 2
MESSAGE_SHELL_DATA = 3
MESSAGE_SHELL_EXIT = 4
MESSAGE_OUTPUT = 5
MESSAGE_ERROR = 6


def packet_formation(message_type, session_id, payload): # forms command packet strcuture so that we can receive it in c2 server and understand what part of the command is what
#     +----------------------+----------------------+
#     | Command Length       | Command              |
#     | 4 bytes              | N bytes              |
#     +----------------------+----------------------+
#         uint32              variable
    payload_bytes = payload.encode()
    packet = (#!I = 4 byte unsigned integer in big endian byte order or network byte order
        struct.pack("!I", message_type)+
        struct.pack("!I", session_id)+
        struct.pack("!I", len(payload_bytes))
    )
    return packet


# the below structure should be the packet strcuture
# so we gotta change the above structure
# +-------------+-------------+-------------+----------------+
# | Message Type|  Session ID | Payload Len |    Payload     |
# |   4 bytes   |   4 bytes   |   4 bytes   | variable size  |
# +-------------+-------------+-------------+----------------+