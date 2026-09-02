#pragma once
#define AGENT_PORT 1234
#define OPERATOR_PORT 9000

class common{
    public:
        static void socket_check(int socket_fd);
        static void send_failed(int client_fd);
        //static void receive_failed(int conn);
        static void accept_failed(int client_fd);
        static void connection_failed(int client_fd);
        static void getpeername_failed(int client_fd);
        static void inet_ntop_failed(int client_fd);
        static void code_exit();
};
// common socket file descriptor error handling header file
// this will be almost used everythere socket programming will be used in c++