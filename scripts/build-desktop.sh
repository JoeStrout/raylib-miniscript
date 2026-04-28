#!/bin/bash

# Build script for raylib-miniscript (desktop)

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

cd "$(dirname "$0")/.."

if [ "$1" == "clean" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf build
    echo -e "${GREEN}Build directory cleaned!${NC}"
fi

echo "Building raylib-miniscript (desktop)..."

# Check for symlinks
if [ ! -e "MiniScript" ]; then
    echo -e "${RED}Error: MiniScript symlink not found!${NC}"
    echo "Create a symlink to MiniScript-cpp/src/MiniScript:"
    echo "  ln -s /path/to/MiniScript-cpp/src/MiniScript MiniScript"
    exit 1
fi

if [ ! -e "raylib/src" ]; then
    echo -e "${YELLOW}Initializing raylib submodule...${NC}"
    git submodule update --init raylib
    if [ $? -ne 0 ]; then
        echo -e "${RED}Failed to initialize raylib submodule!${NC}"
        exit 1
    fi
fi

mkdir -p build
cd build

echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

echo -e "${YELLOW}Building...${NC}"
cmake --build . --config Release

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo "Run: ./build/raylib-miniscript [script.ms]"
    echo "Default script: assets/main.ms"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
