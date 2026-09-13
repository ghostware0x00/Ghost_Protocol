## TODO

### WHAT TO DO NOW


- implement the below flow for reverse shell

```bash
ghost$> sessions

[*]Listing sessions
SESSION_ID     IP_ADDRESS      PORT
____________________________________
1              127.0.0.1      12345

ghost$> use 1
[+]session id : 1 selected

ghost [1]$> shell
[+] starting shell...

agent$> whoami
user

agent$> pwd
/home/user

agent$> ls
...

agent$> back
[*]exiting shell

ghost [1]$>
```