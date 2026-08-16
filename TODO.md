## TODO

### WHAT TO DO

- keeping building operator console design
- after proper design assign session id and send command input to server
- design the command dispatcher


### WHAT TO DO NOW

1. send sessions cmd to c2 server
2. receive the command from operator in c2 
3. form the display_active_agents() info
4. serialize it and send to operator_console
5. deserialize the c2 server info and display it


NOTE : based on the command we need to serialize the command packet
and then receive the data from c2 server



### HOW TO GET STARTED

- first build python command sending packet strcture
- this is important cuz it will help server.cpp undestand the command length and then based on that know how much to read and then call that particular function

- LEARN PYTHON STRUCT