#include "server.hpp"

int main(){
    server myserver;
    myserver.agent_listener();
    myserver.operator_listener();
    return 0;
}