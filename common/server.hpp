#pragma once
#include <cstdint>
#include <unordered_map> // used for session handling (store key value pair of session_ids(key) and client_fd(value))
class server{
    private:
        // create a session_registry structure
        // so that we can map a new agent using session_id and client fd 
        // targetting an individual agent will help us to send specific commands to that client without overlapping with other agents
        std::unordered_map<uint32_t, int> session_registry;
    public:
        void listener();
        void send_commands(int soc_fd, int session_id);
        int get_session_id();
        void handle_multiple_clients(int client_fd, int session_id);
};

//server common code header file
