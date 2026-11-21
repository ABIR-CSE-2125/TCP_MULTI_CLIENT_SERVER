CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude

TARGET = build/tcp_server.exe

SOURCES = src/server.cpp src/communication.cpp
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

