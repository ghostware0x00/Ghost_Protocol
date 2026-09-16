#pragma once
#include <string>
#include <sstream>
#include <cstdint>

#include "PersistentShell.hpp"

constexpr int MESSAGE_COMMAND = 1;
constexpr int MESSAGE_SHELL_START = 2;
constexpr int MESSAGE_SHELL_DATA = 3;
constexpr int MESSAGE_SHELL_EXIT = 4;
constexpr int MESSAGE_OUTPUT = 5;
constexpr int MESSAGE_ERROR = 6;
constexpr int MESSAGE_SHELL_ACK = 7;

struct InteractiveSession {
    bool active = false;
    uint32_t session_id = 0;
};

class agent{
    private:
        InteractiveSession session;   /* tracks active session id / state */
        PersistentShell    shell_;    /* single persistent bash process   */
    public:
        void receive_commands(std::string);
        bool validate_ipaddress(std::string);
        bool recv_all(int client_fd, void *buffer, size_t length);
        bool send_all(int client_fd, const void *data, size_t length);
        //void receive_chunks(int, );
};

// client common code header file
// receiving data in chunks left