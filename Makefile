CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -Iinclude

SRC_DIR = src
BIN_DIR = bin

# Lista de executáveis a serem gerados
TARGETS = manager sensor actuator client

all: directories $(TARGETS)

directories:
	@mkdir -p $(BIN_DIR)

manager: $(SRC_DIR)/manager.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

sensor: $(SRC_DIR)/sensor.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

actuator: $(SRC_DIR)/actuator.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

client: $(SRC_DIR)/client.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all directories clean