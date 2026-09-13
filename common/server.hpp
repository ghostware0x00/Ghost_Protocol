#pragma once
#include <string>
#include <cstdint>
#include <packet.hpp>
#include <unordered_map> // used for session handling (store key value pair of session_ids(key) and client_fd(value))


// message type codes
// constexpr can improve efficiency and allow values to be used during compile time
constexpr int MESSAGE_COMMAND = 1;
constexpr int MESSAGE_SHELL_START = 2;
constexpr int MESSAGE_SHELL_DATA = 3;
constexpr int MESSAGE_SHELL_EXIT = 4;
constexpr int MESSAGE_OUTPUT = 5;
constexpr int MESSAGE_ERROR = 6;


class server{
    private:
        // create a session_registry structure
        // so that we can map a new agent using session_id and client fd 
        // targetting an individual agent will help us to send specific commands to that client without overlapping with other agents
        struct SessionInfo{
            int client_fd;
            std::string ip_address;
            uint16_t port;
        };
        uint32_t session_id = 0;
        std::unordered_map<uint32_t, SessionInfo> session_registry;
    public:
        //void operator_control(std::unordered_map<uint32_t, int> session_registry, int session_id);
        void agent_listener();
        void operator_listener();
        void send_commands_agent(int soc_fd, int session_id);
        void operator_data_recvHandling(int received_bytes, int client_fd);
        bool send_all(int client_fd, const void *data, size_t length);
        bool recv_all(int client_fd, void *buffer, size_t length);
        void command_dispatcher(const packet &received_packet, int client_fd); // used to execute the corresponding function based on the payload received
        void handle_shell_session(int operator_fd, int agent_fd, uint32_t session_id); // handle shell command packet strcuture
        int get_session_id();
        int agentLookup(uint32_t session_id); // function to use the session_id received from operator console to lookup the agent by using session_id to find the agent's client_fd in the session_registry
        void detect_active_agents(int client_fd, int session_id); // passing the session_registry as address cuz threads store data in their own stack frame so we pass by reference so that we can update the original session hash table in real time
        void display_active_agents();
        void get_active_agents(int client_fd); // function for operator console
        int choose_session();
        void bind_failed(int server_fd);
        void setsockopt_failed(int server_fd);
};

//server common code header file
