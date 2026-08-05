#pragma once
#include <string>
#include <cstdint>
#include <vector>

typedef struct packet{
    uint32_t command_length; // length of the command sent
    uint32_t session_id; // set session id to manage multiple agents
    uint8_t heartbeat; // variable to check whether agent is still alive or not (0 = offline) and (1 = online) if 1 is sent by agent then online else offline
    std::string command; // command sent by the server to agent
    //std::string command_type[4] = {"EXEC", "RESULT", "UPLOAD", "DOWNLOAD"}; // based on the command type categorise it
}packet;

packet packet_wrapping(std::string command, int session_id);
std::vector<uint8_t> serialization(packet p1);
packet deserialization_payload_header(uint8_t payload_header[]);
std::string deserializtion_payload(const uint8_t *payload, int size);
//void display_command_output(packet);


// deserialization
// displaying output left


