#pragma once
#include<string>
class agent{
    public:
        void receive_commands();
        bool validate_ipaddress(std::string);
        //void receive_chunks(int, );
};

// client common code header file
// receiving data in chunks left