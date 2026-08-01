#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <print>
#include "packet.hpp"
#include "agent.hpp"
#include "common.hpp"
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>

void common::code_exit(){
    std::cout << "[*]Corrupted data exiting program...." << std::endl;
    exit(EXIT_FAILURE);
}


void common::receive_failed(int connection_status){
    if(connection_status < 0){
        std::perror( "[*]connection failed to establish between c2 server and agent");
        std::cout << std::endl;
        return;
    }
}


void common::socket_check(int soc_fd){
    if(soc_fd < 0){
        std::perror("[*]connection failed");
        std::cout << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


void display_command_output(uint32_t command_length, uint32_t session_id, uint8_t heartbeat, std::vector<uint8_t> command){
    packet p1;
    p1.command_length = command_length;
    p1.session_id = session_id;
    p1.heartbeat = heartbeat + 1; //indicating agent is online
    p1.command.assign(command.begin(), command.end()); // convert uint8_t data to std::string type the .assign() is used because we already delcared packet p1 so we need to use .assign() to directly load the data otherwise we would have had to do this during packet p1 initialisation
    std::cout << "$$$$$$$ Payload Details $$$$$$$" << std::endl;
    std::println("{:<10}{:<10}{:<20}", "Session_ID", "Hearbeat", "Command_Length", "Command");
    std::println("{:<10}{:<10}{:<20}", p1.session_id, p1.heartbeat, p1.command_length, p1.command);
}


// packet deserialzation(uint8_t byte_array_ptr){
// /*
// +--------------+----------------+----------------+---------------+
// | Command Length | Session ID     | Heartbeat | Payload          |
// | 4 bytes        | 4 bytes        | 1 byte    | Variable bytes   |
// +----------------+----------------+-----------+------------------+
// */
//     if(byte_array_ptr == NULL){
//         common::code_exit();
//    }
//    packet p1;
//    std::memcpy(&p1.command_length, &byte_array_ptr+0, 4);
//    std::memcpy(&p1.session_id, &byte_array_ptr+4, 4);
//    std::memcpy(&p1.heartbeat, &byte_array_ptr+8, 1);
//    return p1;
// }


void agent::receive_commands(){
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    common::socket_check(client_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // localhost
    server_address.sin_port = htons(PORT); // port 1234 assigned
    int connection_fd = connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    common::socket_check(connection_fd);
    //char server_response[100];
    uint8_t payload_header[9];// payload header storage
    // we using &byte_array 
    // although might seem byte_array would work cuz array is a pointer. but byte_array is vector object first so we need to point to that vector object's address
    int payload_header_length = 9; // size of struct packet (command length, session_id, heartbeat)
    int connection_status = 0, recv_counter = 0;
    //int phi = 0;
    // receiving the payload header
    do{
        connection_status = recv(client_fd, payload_header+recv_counter, payload_header_length-recv_counter, 0);  // read bytes in each recv and then subtract when those bytes are read 
        if(connection_status > 0){ // > 0 means received something else nothing was received
            recv_counter += connection_status;
        }
        else
            common::receive_failed(connection_status);
    }while(recv_counter != payload_header_length);
    // now that payload header is stored
    // i need to need packet p1.command_length to understand how many bytes i need to read of command or payload
    uint32_t command_length, session_id;
    uint8_t heartbeat;
    std::memcpy(&command_length, payload_header+0, 4); // copying payload length
    command_length = ntohl(command_length); // data sent from server to client(big endian) but memcpy copies data (little endian) so received data is arranged in machine's default endianness which is mostly little endian (ntohl arranges byte order to machine's default byte ordering)
    std::memcpy(&session_id, payload_header+4, 4);
    session_id = ntohl(session_id);
    std::memcpy(&heartbeat, payload_header+8, 1);
    uint32_t payload_counter = 0;
    // now reading payload from the tcp buffer
    std::vector<uint8_t> payload(command_length);
    do{
        connection_status = recv(client_fd, payload.data()+payload_counter, command_length-payload_counter, 0); // read bytes in each recv and then subtract when those bytes are read 
        if(connection_status > 0){
            payload_counter += connection_status;
        }
        else
            common::receive_failed(connection_status);
    }while(payload_counter != command_length);
    display_command_output(command_length, session_id, heartbeat, payload);
    close(client_fd);
}

