# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -Wextra -Icommon -Iprotocol

# Output directory
BIN = exe

# Common source files
SERVER_SRC = src/server.cpp
AGENT_SRC  = src/agent.cpp

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
# Clean
# -----------------------
clean:
	rm -f $(SERVER) $(AGENT)

# -----------------------
# Rebuild
# -----------------------
rebuild: clean all

.PHONY: all clean rebuild
