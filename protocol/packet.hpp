#pragma once
#include <string>

typedef struct packet{
    std::string command_length; // length of the command sent
    std::string command; // command sent by the server to agent
    //std::string command_type[4] = {"EXEC", "RESULT", "UPLOAD", "DOWNLOAD"}; // based on the command type categorise it
    int session_id; // set session id to manage multiple agents
    int heartbeat; // variable to check whether agent is still alive or not
}packet;

packet packet_wrapping(std::string);