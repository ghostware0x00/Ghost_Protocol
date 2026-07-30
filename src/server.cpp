#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include "server.hpp"
#include "packet.hpp"
#include "common.hpp"
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>



void common::socket_check(int soc_fd){ // socket failure error handling
    if(soc_fd < 0){
        std::perror("[x]connection failed");
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }
}

packet packet_wrapping(std::string command){
    packet p1;
    p1.session_id = 
    p1.command = command;
    p1.command_length = command.length();
    p1.heartbeat = 
}

void server::send_commands(int soc_fd){ // send message to client
    std::string command;
    packet p1;
    packet p1;
    std::cout << "Enter command : " << std::endl;
    std::getline(std::cin >> std::ws, command);
    p1 = packet_wrapping(command);
    send(soc_fd, &p1, sizeof(p1), 0);
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
    int client_fd = accept(server_fd, NULL, NULL); // accept connections from client
    send_commands(client_fd); // send msg to client
    close(server_fd); // close server_socket created for listening
}
