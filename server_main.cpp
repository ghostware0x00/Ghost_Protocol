#include "server.hpp"
#include <thread>

int main(){
    server myserver;
    std::thread server_thread(&server::agent_listener, &myserver); // member function needs object to execute thats why &myserver is passed
    std::thread operator_thread(&server::operator_listener, &myserver);
    server_thread.join();// joining both threads so that main() thread doesnt terminate before execution of both these threads
    operator_thread.join();
    return 0;
}