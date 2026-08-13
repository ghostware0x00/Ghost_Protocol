## TODO

### WHAT TO DO

1. find a way to send message to a particular client first
    - for doing this implement operator.py something which will check what sessions are available and then based on that display it.

2. Implement the below stuff after you implement the above stuff
```bash
1. Server accepts agent
        ↓
2. Server assigns session ID
        ↓
3. Server sends command
        ↓
4. Agent receives command
        ↓
5. Agent executes command
        ↓
6. Agent captures stdout
        ↓
7. Agent sends result back
        ↓
8. Server receives result
        ↓
9. Server displays result
        ↓
10. Then add heartbeat
        ↓
11. Then add disconnect/session cleanup
```


### LATER STUFF TO IMPLEMENT

- THE AGENTS WHICH ARE DISCONNECTED THOSE MAPPINGS FROM UNORDERED MAP SHOULD BE REMOVED
- ALWAYS DISPLAY UPDATED HASH TABLE (UNORDERED MAP i.e. session_registry)
    - find the agent which disconnected using its client fd
    - find the client fd then remove that mapping from the UNORDERED MAP (session_registry)