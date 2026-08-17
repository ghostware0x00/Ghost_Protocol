## TODO

### WHAT TO DO NOW

- python packet structure is done using python struct
- now deserialize or read the data sent in bytes from python in the server.cpp and understand the command sent and command length and based on that perform the operation

### WHAT TO DO AFTER THAT 

1. send sessions cmd to c2 server
2. receive the command from operator in c2 
3. form the display_active_agents() info
4. serialize it and send to operator_console
5. deserialize the c2 server info and display it


NOTE : based on the command we need to serialize the command packet
and then receive the data from c2 server