# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -Wextra -Icommon -Iprotocol

# Output directory
BIN = exe

# Common source files
SERVER_SRC = src/server.cpp
AGENT_SRC  = src/agent.cpp src/PersistentShell.cpp

# Standalone local demo (not linked into agent or server)
DEMO_SRC   = src/process_demo.cpp src/PersistentShell.cpp
DEMO       = $(BIN)/process_demo

# Targets
SERVER = $(BIN)/server_main
AGENT  = $(BIN)/agent_main

all: $(SERVER) $(AGENT)

# -----------------------
# Server
# -----------------------
$(SERVER): server_main.cpp $(SERVER_SRC)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) server_main.cpp $(SERVER_SRC) -o $(SERVER)

# -----------------------
# Agent
# -----------------------
$(AGENT): agent_main.cpp $(AGENT_SRC)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) agent_main.cpp $(AGENT_SRC) -o $(AGENT)

# -----------------------
# Standalone process demo
# (local POSIX test only — no networking)
# -----------------------
demo: $(DEMO)
$(DEMO): $(DEMO_SRC)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) $(DEMO_SRC) -o $(DEMO)

# -----------------------
# Clean
# -----------------------
clean:
	rm -f $(SERVER) $(AGENT) $(DEMO)

# -----------------------
# Rebuild
# -----------------------
rebuild: clean all

.PHONY: all demo clean rebuild
