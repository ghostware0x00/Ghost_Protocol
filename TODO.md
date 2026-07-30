## TODO

### PHASE 4 (Designing the Protocol)


#### WHAT I NEED TO DO

- create a packet structure and then use it to wrap the data sent from server to agent and vice versa.
- also find a way to encrypt the data sent from server to agent and vice versa.


#### WHAT TO FIX

- need to find a way to set a random value for session_id
- wrapping up of struct packet and sending it via 


#### DESIGN LOGIC AND QUESTIONS

Do the following for the phase 4
design the protocol
understand the strcuture
ask the communication rules whether the packet structure is following the stuff or not

- Packet structure — What fields does every message contain?
    - Command type
    - Command length
    - Session ID
    - Heartbeat

- Message types — What kinds of messages exist?
    - Register
    - Heartbeat
    - Execute command
    - Return output
    - Upload file
    - Download file
    - Disconnect

- Communication rules — What happens after each message?
    - Does the agent send an acknowledgment?
    - Does the server wait for a reply?
    - Can multiple requests be outstanding?
    - How are errors reported?
    - Encoding — How is data represented?
    - Plain text?
    - Binary?
    - JSON?
    - Custom serialized format?