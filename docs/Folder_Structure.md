
## FOLDER STRUCTURE

```bash
GhostProtocol/
│
├── agent/                                  # Phase 2, 9, 11 | C++
│   └── agent.cpp                           # Agent implementation
│                                            # - Entry point (main)
│                                            # - TCP client
│                                            # - Heartbeat
│                                            # - Task execution
│                                            # - Reconnection
│                                            # - File transfer
│
├── server/                                 # Phase 1, 5, 6, 10, 11 | C++
│   └── server.cpp                          # Server implementation
│                                            # - Entry point (main)
│                                            # - TCP listener
│                                            # - Session management
│                                            # - Command dispatcher
│                                            # - Task queue
│                                            # - File transfer
│
├── operator/                               # Phase 7, 8, 10, 13, 14 | Python
│   ├── cli.py                              # Main CLI entry point
│   ├── commands.py                         # CLI command parser
│   ├── database.py                         # SQLite interface
│   ├── session.py                          # Session management
│   ├── tasks.py                            # Task creation & viewing
│   ├── reports.py                          # Reporting & history
│   ├── config.py                           # Configuration
│   ├── utils.py                            # Helper functions
│   ├── gui.py                              # Future GUI (optional)
│   └── requirements.txt
│
├── database/                               # Phase 8 | Python + SQLite
│   ├── schema.sql                          # Database schema
│   ├── migrations/                         # Migration scripts
│   ├── sqlite.db                           # Local database
│   └── seed.sql                            # Sample data
│
├── protocol/                               # Phase 4 | Shared Design
│   ├── protocol.md                         # Packet format documentation
│   ├── packet_format.md                    # Packet layout
│   ├── commands.md                         # Command IDs & meanings
│   └── notes.md                            # Protocol ideas/design
│
├── common/                                 # Phase 3, 12 | Future Shared C++
│   ├── logger/                             # Logging (future)
│   ├── networking/                         # Socket wrappers (future)
│   ├── config/                             # Config parser (future)
│   └── utils/                              # Helper functions (future)
│
├── docs/                                   # Phase 13
│   ├── architecture.md
│   ├── protocol.md
│   ├── design.md
│   ├── roadmap.md
│   └── diagrams/
│
├── tests/                                  # Phase 13
│   ├── unit/
│   ├── integration/
│   └── networking/
│
├── scripts/                                # Phase 13 | Python
│   ├── build.py                            # Build automation (future)
│   ├── clean.py                            # Cleanup script
│   ├── run_server.py                       # Launch server
│   └── run_agent.py                        # Launch agent
│
├── README.md
└── .gitignore
```