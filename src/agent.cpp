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


// void common::receive_failed(int connection_status){
//     if(connection_status < 0){
//         std::perror( "[*]connection failed to establish between c2 server and agent");
//         std::cout << std::endl;
//         return;
//     }
// }


void common::socket_check(int soc_fd){
    if(soc_fd < 0){
        std::perror("[*]socket creation failed");
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }
}


void common::connection_failed(int connection_status){
    if(connection_status == 0){
        std::perror("[-]server disconnected connection");
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }
    else{
        std::perror("[-]receive failed");
        std::cout << std::endl;
    }
}


void display_command_output(uint32_t command_length, uint32_t session_id, uint8_t heartbeat, std::string command){
    packet p1;
    p1.command_length = command_length;
    p1.session_id = session_id;
    p1.heartbeat = heartbeat + 1; //indicating agent is online
    p1.command = command;
    //std::cout << "$$$$$$$ Payload Details $$$$$$$" << std::endl;
    std::println("{:<20}{:<20}{:<20}{:<20}", "Session_ID", "Hearbeat", "Command_Length", "Command");
    std::println("{:<20}{:<20}{:<20}{:<20}", p1.session_id, p1.heartbeat, p1.command_length, p1.command);
    std::println();
}


packet deserialization_payload_header(uint8_t payload_header[]){
/*
+--------------+----------------+----------------+---------------+
| Command Length | Session ID     | Heartbeat | Payload          |
| 4 bytes        | 4 bytes        | 1 byte    | Variable bytes   |
+----------------+----------------+-----------+------------------+
*/
   packet p1;
   std::memcpy(&p1.command_length, payload_header+0, 4);
   p1.command_length = ntohl(p1.command_length); // fixing byte ordering using nthol cuz data received in big endian and nthol converts data to default endian of the system
   std::memcpy(&p1.session_id, payload_header+4, 4);
   p1.session_id = ntohl(p1.session_id);
   std::memcpy(&p1.heartbeat, payload_header+8, 1);
   return p1;
}


std::string deserialization_payload(const uint8_t* payload, int payload_size){
    packet p1;
    return std::string(reinterpret_cast<const char*>(payload), payload_size);
}


bool agent::validate_ipaddress(std::string server_ip){
    /*
    Check the basic format: Ensure the string is not empty and contains digits and dots.Split the string: Use a string stream or loop to break the address into four segments separated by the dot (.) character.Count the segments: Verify that you have exactly four segments.Check the ranges: Convert each segment to an integer and make sure its value is between 0 and 255 inclusive.Check for extra characters: Ensure there are no extra dots, letters, or invalid symbols.Check for leading zeros: Decide if your rules allow leading zeros like 01 (standard strict validation usually rejects them unless it is a single 0).
    */
   
}


void agent::receive_commands(){
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    common::socket_check(client_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // localhost
    server_address.sin_port = htons(AGENT_PORT); // port 1234 assigned
    //char server_response[100];
    // payload header storage
    // we using &byte_array 
    // although might seem byte_array would work cuz array is a pointer. but byte_array is vector object first so we need point to that vector object's address
    while(1){
        if(connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == 0){
            std::cout << "[+]connected to server" << std::endl;
            while(1){
                uint8_t payload_header[9];
                int payload_header_length = 9; // size of struct packet (command length, session_id, heartbeat)
                int connection_status = 0, recv_counter = 0;
                //int phi = 0;
                // receiving the payload header
                do{
                    connection_status = recv(client_fd, payload_header+recv_counter, payload_header_length-recv_counter, 0);  // read bytes in each recv and then subtract when those bytes are read 
                    if(connection_status > 0){ // > 0 means received something else nothing was received
                        recv_counter += connection_status;
                    }else if(connection_status <= 0){common::connection_failed(connection_status);}
                }while(recv_counter != payload_header_length);
                // now that payload header is stored
                // i need to need packet p1.command_length to understand how many bytes i need to read of command or payload
                packet p1 = deserialization_payload_header(payload_header);
                uint32_t payload_counter = 0;
                // now reading payload from the tcp buffer
                std::vector<uint8_t> payload(p1.command_length);
                do{
                    connection_status = recv(client_fd, payload.data()+payload_counter, p1.command_length-payload_counter, 0); // read bytes in each recv and then subtract when those bytes are read 
                    if(connection_status > 0){
                        payload_counter += connection_status;
                    }else if(connection_status <= 0){common::connection_failed(connection_status);}
                }while(payload_counter != p1.command_length);
                std::string payload_cmd = deserialization_payload(payload.data(), payload.size()); // a vector_array's.size() sends const <datatype>* pointer or address
                p1.command = payload_cmd;
                display_command_output(p1.command_length, p1.session_id, p1.heartbeat, p1.command);
            }
        }
        else{
            close(client_fd);
            std::cout << "[-]agent couldn't connect to server" << std::endl;
            break;
        }
    }
}

