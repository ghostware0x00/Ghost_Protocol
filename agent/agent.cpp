#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>
#define PORT 1234

void socket_check(int soc_fd){
    if(soc_fd < 0){
        std::perror("[x]connection failed");
        std::cout << std::endl;
    }
}

void connection(){
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    socket_check(client_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // localhost
    server_address.sin_port = htons(PORT); // port 1234 assigned
    int connection_fd = connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    socket_check(connection_fd);
    char server_response[100];
    int connection_status = recv(client_fd, (struct sockaddr*)&server_response, sizeof(server_response), 0);
    socket_check(connection_status);
    std::cout << server_response << std::endl;
    close(client_fd);
}

int main(){
    connection();
    return 0;
}