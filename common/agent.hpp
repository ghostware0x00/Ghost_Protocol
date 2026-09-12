#pragma once
#include<string>
#include<sstream>
class agent{
    public:
        void receive_commands(std::string);
        bool validate_ipaddress(std::string);
        void recv_all(int client_fd, void *buffer, size_t length);
        //void receive_chunks(int, );
};

// client common code header file
// receiving data in chunks left