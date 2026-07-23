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
if [ ! -e "MiniScript2" ]; then
    echo -e "${RED}Error: MiniScript2 symlink not found!${NC}"
    echo "Create a symlink to the miniscript2 repo:"
    echo "  ln -s /path/to/miniscript2 MiniScript2"
    exit 1
fi

# The generated/ tree is produced by MiniScript2's own transpile step, not here.
if [ ! -f "MiniScript2/generated/App.g.cpp" ]; then
    echo -e "${RED}Error: MiniScript2 generated sources not found!${NC}"
    echo "Transpile the C# reference implementation in the miniscript2 repo first:"
    echo "  (cd MiniScript2 && ./tools/build.sh transpile)"
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

if [ ! -f "raylib/src/libraylib.a" ] || [ "raylib/.git" -nt "raylib/src/libraylib.a" ]; then
    echo -e "${YELLOW}Building raylib...${NC}"
    # Desktop and web builds share the raylib/src object directory, and their
    # .o files are not interchangeable. Remove any stray objects before building
    # (forcing a fresh desktop compile) and again afterward (so a later web build
    # can't pick up these desktop objects). We only delete the .o files here, not
    # the resulting libraylib.a. We don't expect raylib sources to change often,
    # so the cost of a full rebuild is worth avoiding a bad link.
    rm -f raylib/src/*.o
    make -C raylib/src
    rc=$?
    rm -f raylib/src/*.o
    if [ $rc -ne 0 ]; then
        echo -e "${RED}Failed to build raylib!${NC}"
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
