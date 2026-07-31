#pragma once
#define PORT 1234

class common{
    public:
        static void socket_check(int socket_fd);
        static void send_failed(int send_val);
        static void code_exit();
};
// common socket file descriptor error handling header file
// this will be almost used everythere socket programming will be used in c++