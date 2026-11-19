#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

echo "Building TCP Server..."
make

echo "Starting TCP Server..."
./build/tcp_server.exe