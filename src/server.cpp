#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include "server.hpp"
#include "common.hpp"
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>
void socket_check(int soc_fd){ // socket failure error handling
    if(soc_fd < 0){
        std::perror("[x]connection failed");
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }
}

void send_command(int soc_fd){ // send message to client
    char message[100] = "[+] connected to server";
    send(soc_fd, message, sizeof(message), 0);
}

void listener(){
    /*
    listener of tcp requires to perform the below functions
    1. socket creation
    2. struct sockaddr_in values configuration
    3. bind
    4. listen
    5. accept
    */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // creating socket
    socket_check(server_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = INADDR_ANY; //  accept conncetions from all network interfaces 0.0.0.0
    server_address.sin_port = htons(PORT); // listen to port 1234
    bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)); // binding
    listen(server_fd, 2); // listening (max connection in queue = 2)
    int client_fd = accept(server_fd, NULL, NULL); // accept connections from client
    send_command(client_fd); // send msg to client
    close(server_fd); // close server_socket created for listening
}
