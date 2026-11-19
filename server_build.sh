#!/bin/bash

echo "Building TCP Server..."

g++ -std=c++17 -Wall -Wextra -Wpedantic src/server.cpp -o build/server

if [ $? -eq 0 ]; then
    echo "Build successful!"
else
    echo "Build failed!"
    exit 1
fi