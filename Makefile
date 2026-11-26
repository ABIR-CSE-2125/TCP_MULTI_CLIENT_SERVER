CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -pthread -Iinclude

TARGET = build/tcp_server.exe

SOURCES = src/server.cpp src/communication.cpp src/config.cpp src/thread_pool.cpp src/unique_socket.cpp src/active_client_guard.cpp
OBJECTS = $(patsubst src/%.cpp,build/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

-include $(DEPS)
all: $(TARGET)

build/%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

.PHONY: all clean

