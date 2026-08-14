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
        //void operator_control(std::unordered_map<uint32_t, int> session_registry, int session_id);
        void agent_listener();
        void operator_listener();
        void send_commands(int soc_fd, int session_id);
        int get_session_id();
        void detect_active_agents(int client_fd, int session_id, std::unordered_map<uint32_t, int> *session_registry); // passing the session_registry as address cuz threads store data in their own stack frame so we pass by reference so that we can update the original session hash table in real time
        void display_active_agents(std::unordered_map<uint32_t, int> *session_registry);
        int choose_session(std::unordered_map<uint32_t, int> *session_registry);
        void bind_failed(int server_fd);
        void setsockopt_failed(int server_fd);
};

//server common code header file
