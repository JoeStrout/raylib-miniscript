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
        echo ""
        echo "This script expects the Emscripten SDK at '$EMSDK_DIR' (project root)."
        echo "If you already have emsdk installed elsewhere, create a symlink to it:"
        echo ""
        echo "    ln -s /path/to/your/emsdk \"$(cd "$(dirname "$0")/.." && pwd)/emsdk\""
        echo ""
        echo "Otherwise, install it into the project root:"
        echo ""
        echo "    git clone https://github.com/emscripten-core/emsdk.git"
        echo "    cd emsdk && ./emsdk install latest && ./emsdk activate latest"
        echo ""
        exit 1
    fi
fi

# Check if raylib web library exists; build it if not.
RAYLIB_LIB="raylib/src/libraylib.web.a"
if [ ! -f "$RAYLIB_LIB" ]; then
    echo -e "${YELLOW}Raylib web library not found; building raylib for web...${NC}"
    # Desktop and web builds share the raylib/src object directory, and their
    # .o files are not interchangeable. Remove any stray objects before building
    # (forcing a fresh web compile) and again afterward (so a later desktop build
    # can't pick up these web objects). We don't expect raylib sources to change
    # often, so the cost of a full rebuild is worth avoiding a bad link.
    rm -f raylib/src/*.o
    make -C raylib/src PLATFORM=PLATFORM_WEB
    rc=$?
    rm -f raylib/src/*.o
    if [ $rc -ne 0 ] || [ ! -f "$RAYLIB_LIB" ]; then
        echo -e "${RED}Failed to build raylib for web!${NC}"
        exit 1
    fi
    echo -e "${GREEN}Raylib web library built!${NC}"
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
