#pragma once

typedef struct session_registry{
    uint32_t session_id;
    int client_fd;
}session_registry;


class server{
    public:
        void listener();
        void send_commands(int soc_fd, int session_id);
        int get_session_id();
        void handle_multiple_clients(int client_fd, int session_id);
};

//server common code header file
