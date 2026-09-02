import struct

# we are sending data in bytes in network
# in this protocol.py we are arranging what bytes are in what position
# or in what order based on which we can read them in recv()
# and run the respective function or operation

def command_packet_formation(command): # forms command packet strcuture so that we can receive it in c2 server and understand what part of the command is what
#     +----------------------+----------------------+
#     | Command Length       | Command              |
#     | 4 bytes              | N bytes              |
#     +----------------------+----------------------+
#         uint32              variable
    command_bytes = command.encode()
    command_length = len(command_bytes)
    command_len_bytes = struct.pack(">I", command_length) # > = big endian. this is to ensure that data sent is in big endian format just like the sockets expect it so that there is uniformity in byte ordering during sending and receiving without any byte ordering issues in the server side
    packet_bytes = command_len_bytes + command_bytes
    return packet_bytes