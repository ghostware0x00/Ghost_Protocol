## TODO

### WHAT TO DO NOW


- optimize code with the `send_all()` function and then properly send the ip address and parse and display the ip address of each associating session_ids in the operator console first.






- LATER THE BELOW STUFF
    - after implementing the above stuff do this -> implement `use sessions` command (IMP!!!)
        - this is leading to targetted message input 


    - also find a way to take c2 server ip address as command line argument in agent.cpp and then attach the ip to the target ip (IMP!!!)



```c
// Define a temporary struct to hold just the data we need to send
struct AgentData {
    uint32_t id;
    std::string ip;
    uint16_t port;
};

void server::get_active_agents(int client_fd)
{
    std::vector<AgentData> snapshot;

    // 1. Lock, Copy, and Release instantly
    {
        std::lock_guard<std::mutex> lock(session_reg_mutex);
        
        snapshot.reserve(session_registry.size()); 
        for (const auto& session : session_registry) 
        {
            snapshot.push_back({
                session.first, 
                session.second.ip_address, 
                session.second.port
            });
        }
    } // Mutex unlocks completely right here!

    // 2. Send session count (Safe: map is unlocked)
    uint32_t sessionCount = htonl(snapshot.size());
    if(!send_all(client_fd, &sessionCount, sizeof(sessionCount)))
    {
        std::cout << "[!] operator disconnected" << std::endl;
        close(client_fd);
        return;
    }

    // 3. Loop through our local temporary copy (Safe: no network blocking issues)
    for(const auto& agent : snapshot)
    {
        uint32_t session_id = htonl(agent.id);
        uint16_t port = htons(agent.port);

        // Send session ID
        if(!send_all(client_fd, &session_id, sizeof(session_id)))
        {
            close(client_fd);
            return;
        }

        // Send IP length
        uint32_t ip_length = htonl(agent.ip.size());
        if(!send_all(client_fd, &ip_length, sizeof(ip_length)))
        {
            close(client_fd);
            return;
        }

        // Send IP data
        if(!send_all(client_fd, agent.ip.data(), agent.ip.size()))
        {
            close(client_fd);
            return;
        }

        // Send port
        if(!send_all(client_fd, &port, sizeof(port)))
        {
            close(client_fd);
            return;
        }
    }

    close(client_fd);
}
    
```