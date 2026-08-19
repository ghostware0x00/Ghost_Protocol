#pragma once
#include <string>
#include <cstdint>
#include <unordered_map> // used for session handling (store key value pair of session_ids(key) and client_fd(value))

class server{
    private:
        // create a session_registry structure
        // so that we can map a new agent using session_id and client fd 
        // targetting an individual agent will help us to send specific commands to that client without overlapping with other agents
        std::unordered_map<uint32_t, int> session_registry;
    public:
        //void operator_control(std::unordered_map<uint32_t, int> session_registry, int session_id);
        void agent_listener();
        void operator_listener();
        void send_commands_agent(int soc_fd, int session_id);
        uint32_t deserialize_commandLenBytes(uint8_t command_length_bytes[]);
        void command_dispatcher(std::string command, int client_fd); // function will be used to call the associated function based on the command supplied by the operator
        int get_session_id();
        void detect_active_agents(int client_fd, int session_id); // passing the session_registry as address cuz threads store data in their own stack frame so we pass by reference so that we can update the original session hash table in real time
        void display_active_agents();
        void get_active_agents(int client_fd); // function for operator console
        int choose_session();
        void bind_failed(int server_fd);
        void setsockopt_failed(int server_fd);
};

//server common code header file
