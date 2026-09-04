#include<iostream>
#include "agent.hpp"


int main(int argc, char *argv[]){
    if(argc < 2 || argc > 2){
        std::cout << "[!]server address not provided" << std::endl;
        return 0;
    }
    agent myagent;
    if(myagent.validate_ipaddress(argv[1]))
        myagent.receive_commands();
    else
        std::cout << "[!] invalid server ip address" << std::endl;
    return 0;
}