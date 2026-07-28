#pragma once

class server{
    public:
        void listener();
        void send_commands(int soc_fd);
};

//server common code header file
