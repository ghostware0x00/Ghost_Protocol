# Ghost Protocol

Ghost Protocol is an educational client/server project for studying TCP socket programming, custom packet framing, session tracking, and controlled remote shell sessions on systems you own or are explicitly authorized to test.

The project consists of:

- A C++ server that accepts agent and operator connections.
- A C++ agent that connects to the server and owns a local persistent Bash process.
- A Python operator console that lists connected agents and opens a shell session for a selected agent.

> **Authorization required:** The agent can execute shell commands on its host. Use this project only in an isolated lab or on systems for which you have explicit permission. Do not expose it to untrusted networks or use it to access systems, accounts, or data without authorization.

## Features

- TCP-based agent and operator connections.
- Custom binary packet format using network byte order.
- Server-side registry of connected agent sessions.
- Interactive shell sessions for selected agents.
- One persistent Bash child process per active shell session.
- Shell state that persists between commands: working directory, exported variables, and shell variables.
- Non-blocking stdout and stderr collection.
- Explicit command-completion marker and graceful shell cleanup.

## Technology stack

| Area | Technology |
| --- | --- |
| Server and agent | C++23 |
| Local process management | POSIX (`fork`, `pipe`, `dup2`, `poll`, `waitpid`) |
| Operator console | Python 3 |
| Transport | TCP/IP sockets |
| Shell process | `/bin/bash -s` |
| Build | GNU Make and `g++` |

The agent and persistent shell require a POSIX-compatible system such as Linux. Native Windows is not supported.

## Project layout

```text
Ghost_Protocol/
├── agent_main.cpp                 Agent entry point
├── server_main.cpp                Server entry point
├── common/
│   ├── agent.hpp                  Agent state and message constants
│   ├── common.hpp                 Shared port configuration
│   └── PersistentShell.hpp        Persistent local Bash interface
├── protocol/packet.hpp            Packet definition and serialization API
├── src/
│   ├── agent.cpp                  Agent receive and dispatch loop
│   ├── server.cpp                 Server listeners and session routing
│   ├── PersistentShell.cpp        POSIX persistent-process implementation
│   └── process_demo.cpp           Standalone PersistentShell test
├── operator/                      Python operator console
└── Makefile
```

## Network model

The protocol has a 12-byte header followed by an optional payload:

```text
+----------------+----------------+----------------+-------------------+
| Message type    | Session ID     | Payload length | Payload           |
| 4 bytes         | 4 bytes        | 4 bytes        | N bytes           |
+----------------+----------------+----------------+-------------------+
```

All header fields are unsigned 32-bit values in network byte order.

| Message | Value | Purpose |
| --- | ---: | --- |
| `MESSAGE_COMMAND` | 1 | General operator command, including `sessions`. |
| `MESSAGE_SHELL_START` | 2 | Start an agent shell session. |
| `MESSAGE_SHELL_DATA` | 3 | Send input to the active shell. |
| `MESSAGE_SHELL_EXIT` | 4 | Stop the active shell. |
| `MESSAGE_OUTPUT` | 5 | Return shell output. |
| `MESSAGE_ERROR` | 6 | Return an error. |
| `MESSAGE_SHELL_ACK` | 7 | Confirm shell startup. |

Default ports, defined in `common/common.hpp`:

```text
Agent listener:    TCP 1234
Operator listener: TCP 9000
```

```text
Agent ── TCP/1234 ──> Server <── TCP/9000 ── Operator console
                              │
                              └── routes packets by session ID
```

The agent initiates its connection to the server. The server does not connect back to the agent.

## Persistent shell behavior

`agent` owns one `PersistentShell` instance. A shell command does not create another Bash process.

```text
MESSAGE_SHELL_START
        │
        ▼
PersistentShell.start()
        │
        ▼
One /bin/bash -s child process
        │
        ▼
MESSAGE_SHELL_DATA
        │
        ▼
PersistentShell.write_input(command)
        │
        ▼
PersistentShell.read_available_output()
        │
        ▼
MESSAGE_OUTPUT
        │
        ▼
MESSAGE_SHELL_EXIT → PersistentShell.stop()
```

For each command, `PersistentShell` writes input to the same Bash process and appends an internal completion marker. It polls non-blocking stdout and stderr pipes until the marker arrives, removes the marker from returned output, and sends the result through the existing packet flow. The persistent process is why `cd`, shell variables, and exported variables survive across inputs.

Shell input is intentionally interpreted by Bash on the authorized agent host. This is remote shell execution by design, not a safe command API. Never send untrusted input to an agent.

## Prerequisites

- Linux or another POSIX-compatible environment for the agent.
- `g++` with C++23 support.
- GNU Make.
- Python 3 for the operator console.
- `/bin/bash` available on the agent host.

On Debian-based lab systems:

```bash
sudo apt update
sudo apt install build-essential python3
```

## Build

Build the server and agent:

```bash
make all
```

Build and run the persistent-shell validation demo:

```bash
make demo
./exe/process_demo
```

Generated executables:

```text
exe/server_main
exe/agent_main
exe/process_demo
```

Remove generated executables:

```bash
make clean
```

## Controlled lab usage

1. On the server host, start the server:

   ```bash
   ./exe/server_main
   ```

2. Confirm it listens on both project ports:

   ```bash
   ss -ltn | grep -E ':(1234|9000)'
   ```

3. On the authorized agent host, connect to the server's reachable LAN address:

   ```bash
   ./exe/agent_main <SERVER_IP>
   ```

4. If the operator console runs on another machine, set `TARGET_IP` in `operator/connection.py` to the same server address. The default `127.0.0.1` works only when the console and server share a host.

5. Start the operator console:

   ```bash
   python3 operator/main.py
   ```

6. Use the console:

   ```text
   ghost$> sessions
   ghost$> use <SESSION_ID>
   ghost [<SESSION_ID>]$> shell
   agent$> pwd
   agent$> back
   ```

`back` or `exit` at the `agent$>` prompt sends `MESSAGE_SHELL_EXIT`, stops the Bash child, and clears the shell session.

## Firewall and network troubleshooting

If `sessions` shows no active agents, the server has not registered an agent TCP connection. Check the following:

- Start `server_main` before `agent_main`.
- Pass the server's bridged/LAN address to `agent_main`, not the agent VM's own address.
- Verify from the agent machine that the server port is reachable:

  ```bash
  ping -c 1 <SERVER_IP>
  nc -vz <SERVER_IP> 1234
  ```

- Permit inbound TCP `1234` and `9000` on the server host and on any firewall between the systems.
- For bridged VMs, ensure both guests are on a reachable network and guest-to-guest traffic is allowed.
- For NAT VMs, configure port forwarding or use a network mode that permits the agent to reach the server.
- Check server output for:

  ```text
  [+] an agent connected
  ```

- A failed initial agent connection ends its current connection loop. After fixing reachability, restart `agent_main`.

Useful server-side checks:

```bash
ip -br addr
ss -ltnp | grep -E ':(1234|9000)'
```

## Limitations

- This is an educational prototype, not a production remote-administration system.
- TCP traffic is plaintext; there is no encryption, authentication, authorization, or integrity protection.
- The agent executes shell input with the privileges of the account that starts it.
- The fixed completion marker can conflict with output containing that exact standalone marker line.
- Slow or long-running commands can return partial output after the polling timeout.
- stdout and stderr are separate pipes, so their relative ordering is not guaranteed.
- The agent does not continuously retry a failed initial server connection.
- There is no durable session storage, audit trail, file transfer control, or production-grade recovery.
- Shell state is reset when the session exits or the agent process terminates.

## Security guidance

- Run only in a private, disposable lab network.
- Use a dedicated unprivileged account for the agent.
- Restrict project ports to known lab hosts with host and network firewalls.
- Do not run the server or agent as root.
- Do not expose TCP ports `1234` or `9000` to the public internet.
- Treat operator input and agent output as sensitive.
- Add authentication, encryption, authorization, audit logging, message-size limits, and reconnect handling before any legitimate use outside a controlled lab.

## Disclaimer

Ghost Protocol is provided for educational and authorized testing purposes. The project authors and contributors are not responsible for misuse, unauthorized access, damage, data loss, or legal consequences resulting from its use. You are responsible for complying with applicable laws, organizational policies, and written authorization requirements.
