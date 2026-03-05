#!/bin/bash

# Build script for raylib-miniscript (web/Emscripten)

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

cd "$(dirname "$0")/.."

if [ "$1" == "clean" ]; then
    echo -e "${YELLOW}Cleaning build-web directory...${NC}"
    rm -rf build-web
    echo -e "${GREEN}Build directory cleaned!${NC}"
fi

echo "Building raylib-miniscript (web)..."

# Check if emcmake is available, if not try to activate emsdk
if ! command -v emcmake &> /dev/null; then
    echo -e "${YELLOW}emcmake not found, attempting to activate Emscripten...${NC}"

    EMSDK_DIR="raylib/../emsdk"
    EMSDK_PATH="$EMSDK_DIR/emsdk_env.sh"

    if [ -f "$EMSDK_PATH" ]; then
        echo -e "${YELLOW}Found emsdk at $EMSDK_PATH${NC}"

        EMSDK_ABS=$(cd "$EMSDK_DIR" && pwd)
        echo -e "${YELLOW}Reactivating emsdk...${NC}"
        cd "$EMSDK_ABS"
        ./emsdk activate latest

        if [ $? -ne 0 ]; then
            echo -e "${RED}Error: Failed to activate emsdk!${NC}"
            exit 1
        fi

        cd - > /dev/null
        source "$EMSDK_ABS/emsdk_env.sh"

        if ! command -v emcmake &> /dev/null; then
            echo -e "${RED}Error: Failed to activate Emscripten!${NC}"
            exit 1
        fi
        echo -e "${GREEN}Emscripten activated successfully!${NC}"
    else
        echo -e "${RED}Error: emcmake (Emscripten) not found!${NC}"
        echo "Please install Emscripten or update the path in this script"
        exit 1
    fi
fi

# Check if raylib web library exists
RAYLIB_LIB="raylib/src/libraylib.web.a"
if [ ! -f "$RAYLIB_LIB" ]; then
    echo -e "${YELLOW}Warning: Raylib web library not found at $RAYLIB_LIB${NC}"
    echo "You may need to build raylib for web first."
fi

mkdir -p build-web
cd build-web

echo -e "${YELLOW}Configuring with CMake...${NC}"
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

echo -e "${YELLOW}Building...${NC}"
cmake --build . --config Release

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo "Generated files in build-web/:"
    ls -lh raylib-miniscript.html raylib-miniscript.js raylib-miniscript.wasm 2>/dev/null || echo "  (output files)"
    echo ""
    echo "To run: cd build-web && python3 -m http.server 8000"
    echo "Then open: http://localhost:8000"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
