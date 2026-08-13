## TODO

### WHAT TO DO

- implementing or detecting which agent has disconnected and then updating the table


### HOW TO DO

- using mutex locking to prevent multiple threads to concurrently access hash table session_registry and change corrupt the session_registry causing segmentation fault

### WHY REQUIRED

- mutex is required so that only one thread can access and modify the hash table because simultaneous modification might corrupt the unordered map session_registry