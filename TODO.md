## TODO

1. implement tcp server (done)
2. implement tcp client(AGENT) (done)

3. IMPLEMENT THIS VERY IMPORTANT
    - include the header files with the common code
    - so that we can use this code elsewhere too

```bash
GhostProtocol/
│
├── include/
│   ├── server.hpp
│   ├── client.hpp
│   └── common.hpp
│
├── src/
│   ├── server.cpp
│   ├── client.cpp
│   └── common.cpp
│
├── server_main.cpp
└── client_main.cpp
```


4. make the tcp client to initiate connection with the tcp server in repeated timeouts when connection failed(AGENT)
5. encrypt the raw socket communication using openssl 
