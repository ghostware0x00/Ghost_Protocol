## TODO

### PHASE 4 (Designing the Protocol)


#### WHAT I NEED TO DO

- create a packet structure and then use it to wrap the data sent from server to agent and vice versa.
- also find a way to encrypt the data sent from server to agent and vice versa.


#### WHAT TO FIX

- arrange the struct packet data into a sequence of bytes before sending to agent 

#### HOW TO FIX

- since sending struct packet address might cause padding, alignment, endianness issues we will serialize the struct packet data.
- form a sequence of bytes and then send it via socket.send().
- the compiler is free to layout the struct however it wants and also there is the network byte ordering issue so we need to form array of bytes so that there is no room for the compiler to include any padding making our data sequence secure.

#### DESIGN LOGIC AND QUESTIONS

Do the following for the phase 4
design the protocol
understand the strcuture
ask the communication rules whether the packet structure is following the stuff or not

- Packet structure — What fields does every message contain?
    - Command type
    - Command length
    - Session ID
    - Heartbeat

- Message types — What kinds of messages exist?
    - Register
    - Heartbeat
    - Execute command
    - Return output
    - Upload file
    - Download file
    - Disconnect

- Communication rules — What happens after each message?
    - Does the agent send an acknowledgment?
    - Does the server wait for a reply?
    - Can multiple requests be outstanding?
    - How are errors reported?
    - Encoding — How is data represented?
    - Plain text?
    - Binary?
    - JSON?
    - Custom serialized format?

    ## SERIALIZATION CODE

```c
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <sys/socket.h> // For send()
#include <arpa/inet.h>  // For htonl()

typedef struct packet {
    uint32_t command_length;
    uint32_t session_id;
    uint8_t heartbeat;
    std::string command;
} packet;

// --- Inside your sending function ---

packet p;
p.session_id = 105;
p.heartbeat = 1;
p.command = "WHOAMI";
p.command_length = p.command.size(); // Set the length of the string content

// 1. Calculate the exact total byte size needed
size_t total_size = sizeof(p.command_length) + 
                    sizeof(p.session_id) + 
                    sizeof(p.heartbeat) + 
                    p.command_length;

std::vector<uint8_t> byte_array(total_size);

// 2. Setup a pointer to track where we are writing in the buffer
uint8_t* writer = byte_array.data();

// 3. Convert multi-byte integers to Network Byte Order (Big Endian) for safety
uint32_t net_length = htonl(p.command_length);
uint32_t net_session = htonl(p.session_id);

// 4. Copy each field into the array step-by-step
std::memcpy(writer, &net_length, sizeof(net_length));
writer += sizeof(net_length);

std::memcpy(writer, &net_session, sizeof(net_session));
writer += sizeof(net_session);

std::memcpy(writer, &p.heartbeat, sizeof(p.heartbeat));
writer += sizeof(p.heartbeat);

// Copy the actual characters of the string, not the std::string object
std::memcpy(writer, p.command.data(), p.command_length);

// 5. Send the byte array over the socket
// Note: send() might not send all bytes at once in real-world scenarios. 
// You should ideally loop until total_size bytes are sent.
ssize_t bytes_sent = send(socket_fd, byte_array.data(), byte_array.size(), 0);
```