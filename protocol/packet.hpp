#pragma once
#include <string>
#include <cstdint>
#include <vector>

struct packet{
    uint32_t message_type;
    uint32_t session_id;
    uint32_t payload_length;
    std::string payload;
};


std::vector<uint8_t> serialization(const packet &p1);
packet deserialization_payload_header(const uint8_t payload_header[]);
std::string deserialization_payload(const uint8_t *payload, size_t size);

// updated packet structure
//                  4 bytes          4 bytes          4 bytes
//               +-------------+----------------+----------------+
//               | Message Type|   Session ID    | Payload Length |
//               +-------------+----------------+----------------+
//               |                 Payload (N bytes)              |
//               +------------------------------------------------+

