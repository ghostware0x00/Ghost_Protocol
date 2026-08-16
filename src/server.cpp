#include <iostream>
#include <cstring> // for memcpy()
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <thread> // used to implement multithreading so that tcp server can handle multiple clients
#include <random>
#include <mutex> // will be used for locking session_registry cuz concurrent access might corrupt it
#include <print>
#include "server.hpp"
#include "packet.hpp"
#include "common.hpp"
// #include <openssl/ssl.h> // using openssl to encrypt the open socket communication
// #include <openssl/err.h>

std::mutex session_reg_mutex; // global locker variable created

void common::code_exit(){
    std::cout << "[*]Corrupted data exiting program...." << std::endl;
    exit(EXIT_FAILURE);
}


void common::socket_check(int soc_fd){ // socket failure error handling
    if(soc_fd < 0){
        std::perror("[*]connection failed");
        std::cout << std::endl;
        close(soc_fd);
        exit(EXIT_FAILURE);
    }
}


void common::accept_failed(int client_fd){
    // client_fd < 0
    std::perror("[*]couldn't accept connection");
    std::cout << std::endl;
    close(client_fd);
}


void server::setsockopt_failed(int server_fd){ 
    // this function is executed when setsockopt fails
    std::perror("[-]setsockopt failed");
    close(server_fd);
    std::cout << std::endl;
    exit(EXIT_FAILURE);
}


void server::bind_failed(int server_fd){
    std::perror("[-]server bind failed");
    std::cout << std::endl;
    close(server_fd);
    exit(EXIT_FAILURE);
}


std::vector<uint8_t> serialization(packet p1){
    size_t total_size = sizeof(p1.command_length) + sizeof(p1.session_id) + sizeof(p1.heartbeat) + p1.command.length();
    // doing sizeof(p1) causes padding of bytes so we will get wrong data 
    // since we are sending this through network we need to calculate their correct size 
    std::vector<uint8_t> byte_array(total_size); // creating a dynamic array of total_size bytes and it will have 1 byte continguous blocks of memory storage. arrays in vectors can shrink and increase since they are dynamic
    uint8_t *byte_array_ptr = byte_array.data(); // pointer of byte_array so that we can move it accordingly and writes bytes in the byte_array. also byte_array.data() produces the starting address of byte_array
    // since these 2 variables are 4 bytes so using htonl we set them to big endian order, the correct byte order for network communication
    if(byte_array_ptr == NULL){
        common::code_exit();
    }
    uint32_t cl = htonl(p1.command_length);
    uint32_t sid = htonl(p1.session_id);
    //copying bytes in memory (byte_array)
    byte_array_ptr = byte_array.data(); // setting the starting address for 
    std::memcpy(byte_array_ptr, &cl, sizeof(cl)); // copying command length bytes
    // incrementing pointer with respect to the bytes of cl(command_length) and sid(session_id)
    byte_array_ptr += sizeof(cl); 
    std::memcpy(byte_array_ptr, &sid, sizeof(sid));
    byte_array_ptr += sizeof(sid);
    std::memcpy(byte_array_ptr, &p1.heartbeat, sizeof(p1.heartbeat));
    byte_array_ptr += sizeof(p1.heartbeat);
    std::memcpy(byte_array_ptr, p1.command.data(), p1.command.length());
    return byte_array;
}


// DEMO FUNCTION FOR DEBUGGIN PURPORSES
void server::display_active_agents(std::unordered_map<uint32_t, int> *session_registry){
    // display function here is kept inside mutex because if one agent is connected and another one disconnects
    // the display will cause problems so mutex is applied so that display is shown properly without any issue cuz iterating over the session_registry is still reading.
    std::unique_lock<std::mutex> lock(session_reg_mutex);
    if(session_registry->empty()){
        std::cout << "[*] no active agents are available" << std::endl;
        return;
    }
    std::cout << "___________________________________________" << std::endl;
    std::println("{:<20}{:<20}", "Session ID", "Client FD");
    for(auto session_reg : *session_registry){
        std::println("{:<20}{:<20}", session_reg.first, session_reg.second);
    }
    std::cout << "___________________________________________" << std::endl;
}



int server::get_session_id(){
    srand(time(0)); // setting the current time as seed value so that rand() number is new everytime
    int session_id = rand();
    return session_id;
}


packet packet_wrapping(std::string command, int session_id){
    //srand(time(0)); // setting the current time as seed value so that rand() number is new everytime
    packet p1;
    p1.command_length = command.length();
    p1.session_id = session_id;
    p1.heartbeat = 0;
    p1.command = command;
    return p1;
}


int server::choose_session(std::unordered_map<uint32_t, int> *session_registry){
    uint32_t session_id;
    std::cout << "Choose session_id : ";
    std::cin >> session_id;
    if(session_registry->contains(session_id)){
        return session_id;
    }    
    else{
        std::cout << "[*] invalid session input" << std::endl;
        return -1;
    }
}


void server::send_commands_agent(int soc_fd, int session_id){ // send message to client
    std::string command;
    packet p1;
    // std::cout << "Enter command : " << std::endl;
    // std::getline(std::cin >> std::ws, command);
    p1 = packet_wrapping(command, session_id);
    std::vector<uint8_t> byte_array = serialization(p1);
    // instead of soc_fd we need to send session_registry[session_id] to send commands to that particular data only
    if(send(soc_fd, byte_array.data(), byte_array.size(), 0) < 0){
        common::code_exit();
    }
}


void server::detect_active_agents(int client_fd, int session_id, std::unordered_map<uint32_t, int> *session_registry){
    /*
                        SERVER
                      |
                listener thread
                      |
          +-----------+-----------+
          |           |           |
       accept()    accept()    accept()
          |           |           |
       client_fd   client_fd   client_fd
          |           |           |
       thread 1    thread 2    thread 3
          |           |           |
       session A   session B   session C

       using the threading we are able to implement this
    */
    char temp[1024];
    while(true){
        int received = recv(client_fd, temp, sizeof(temp), 0);
        if(received == 0){ // received becomes 0 when agent disconnects and client_fd becomes invalid
            close(client_fd);
            {
                std::unique_lock<std::mutex> lock_session_reg(session_reg_mutex); // locks the below code and automatically performs lock_session.unlock() when goes out of function scope unless explicitly called. This lock makes sure when multiple agents don't access session_registry at the same time. only when one finishes the other can modify it. Without locking multiple modifications of the session_registry at the same time might result in program crash or segmentation faults. []
                session_registry->erase(session_id); // since session_registry is a pointer so we use arrow operator
                std::cout << "[-]agent id : " << session_id << " is disconnected" << std::endl;
                break;
            }
        }
    }
    display_active_agents(session_registry);
}


// void server::receive_commands_operator(int client_fd){ // function to receive commands 
//     /* TO DO */
//     uint32_t command_length = 32;
//     int receive_counter = 0;
//     int bytes_received = 0;
//     char bytearray_command[32] = {}; // array where command data received in bytes
//     do{
//         bytes_received = recv(client_fd, bytearray_command + receive_counter, command_length-receive_counter, 0);
//         if(bytes_received > 0){
//             receive_counter += bytes_received;
//         }
//     }while(bytes_received != command_length);
// }



void server::operator_listener(){
    std::println("[+] server listening on 0.0.0.0 port {} for OPERATOR",OPERATOR_PORT);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    common::socket_check(server_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(OPERATOR_PORT);
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        setsockopt_failed(server_fd);
    }
    if(bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        bind_failed(server_fd);
    }
    listen(server_fd, 1);
    while(true){
        std::cout << "[*] waiting for connections (operator)" << std::endl;
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd < 0){
            common::accept_failed(client_fd);
            continue;
        }
        std::cout << "[+] operator connected" << std::endl;
        //receive_commands_operator(client_fd); 
        // read the command length and understand how many bytes to read and then based on that call the respective function
        // need to deserialize the packet strcuture sent by the python operator
    }
}



void server::agent_listener(){
    /*
    listener of tcp requires to perform the below functions
    1. socket creation
    2. struct sockaddr_in values configuration
    3. bind
    4. listen
    5. accept
    */
    std::println("[+] server listening on 0.0.0.0 port {} FOR AGENT", AGENT_PORT);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // creating socket
    common::socket_check(server_fd);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4 address
    server_address.sin_addr.s_addr = INADDR_ANY; //  accept conncetions from all network interfaces 0.0.0.0
    server_address.sin_port = htons(AGENT_PORT); // listen to port 1234
    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        // the server cant connect because the operating system holds the ports in a TIME_WAIT STATE
        // in this state the ports are in use so when the server tries to bind to this port the operating system refuses it
        // hence neither the server nor the agent can connect
        setsockopt_failed(server_fd);
    }
    if(bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){ // binding
        bind_failed(server_fd);
    }
    
    listen(server_fd, 1); // listening (max connection in queue = 2)
    while(true){ // kept in loop so that multiple clients can be handled
        // like if one agent connection is lost or connected the loop will start from while(true) again and try to accept new agent connection
        std::cout << "[*] waiting for connections (agent)" << std::endl;
        int client_fd = accept(server_fd, NULL, NULL); // accept connections from client
        if(client_fd < 0){
            common::accept_failed(client_fd);
            continue; // skips the rest of the below logic and retries accept()
        }
        std::cout << "[+] an agent connected" << std::endl;
        int session_id = get_session_id();
        // the locking is required because multiple agents might insert data at the same time
        {
            std::unique_lock<std::mutex> lock(session_reg_mutex);
            session_registry[session_id] = client_fd; // mapping session_id to client_fd using unorderd_map 
            // after this lock will be unlocked automatically cuz out of scope
        }
        display_active_agents(&session_registry);
        std::thread client(&server::detect_active_agents, this, client_fd, session_id, &session_registry); // pass the address of original session_registry hash table
        // handle_multiple_clients() is a member function of the server class.
        // &server::handle_multiple_clients gives a pointer to that member function.
        // It identifies which member function the new thread should execute.
        // this keyword is used to tell the pointer which server object should it point to. In this case the current server object 
        client.detach();
    }
    std::cout << "[-]server exiting..." << std::endl;
    close(server_fd); // close server_socket created for listening
}
