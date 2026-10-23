#!/bin/bash

# Build script for raylib-miniscript (web/Emscripten)

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# -P resolves symlinks, so the project path CMake records (in every include
# flag) is the same however the script was reached; a different spelling
# would change the flags and force a full rebuild.
cd -P "$(dirname "$0")/.."

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

# Build the raylib web library if it is missing or was built from a different
# raylib commit (e.g. after moving the submodule to a new release) or for a
# different graphics API. The stamp file records which commit and API the
# current libraylib.web.a came from.  We use OpenGL ES 3 (WebGL2), whose GLSL
# ES 3.00 is close enough to desktop GLSL 3.30 that one shader source can serve
# both (see AdaptShaderCode in src/RCore.cpp).  Keep this in sync with the
# WebGL link options in CMakeLists.txt.
RAYLIB_GRAPHICS="GRAPHICS_API_OPENGL_ES3"
RAYLIB_LIB="raylib/src/libraylib.web.a"
RAYLIB_COMMIT="$(git -C raylib rev-parse HEAD 2>/dev/null) $RAYLIB_GRAPHICS"
RAYLIB_STAMP="build-web/raylib-commit.txt"
if [ ! -f "$RAYLIB_LIB" ] || [ "$(cat "$RAYLIB_STAMP" 2>/dev/null)" != "$RAYLIB_COMMIT" ]; then
    echo -e "${YELLOW}Raylib web library missing or out of date; building raylib for web...${NC}"
    # Desktop and web builds share the raylib/src object directory, and their
    # .o files are not interchangeable. Remove any stray objects before building
    # (forcing a fresh web compile) and again afterward (so a later desktop build
    # can't pick up these web objects). We don't expect raylib sources to change
    # often, so the cost of a full rebuild is worth avoiding a bad link.
    rm -f raylib/src/*.o
    make -C raylib/src PLATFORM=PLATFORM_WEB GRAPHICS=$RAYLIB_GRAPHICS
    rc=$?
    rm -f raylib/src/*.o
    if [ $rc -ne 0 ] || [ ! -f "$RAYLIB_LIB" ]; then
        echo -e "${RED}Failed to build raylib for web!${NC}"
        exit 1
    fi
    mkdir -p build-web
    echo "$RAYLIB_COMMIT" > "$RAYLIB_STAMP"
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
    # A relink rewrites the page without package-web.sh's loader tag, leaving
    # any file package here unused.  Remove it then, so it can't be mistaken
    # for current files.  (If nothing relinked, the page still loads it; keep it.)
    if { [ -f game.data ] || [ -f game.data.js ]; } && ! grep -q "game.data.js" raylib-miniscript.html; then
        rm -f game.data game.data.js
        echo -e "${YELLOW}Removed the old file package (game.data); run scripts/package-web.sh to make a fresh one.${NC}"
    fi
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
