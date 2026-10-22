#!/bin/bash

# Package a game's files for the web build (raylib-miniscript).
#
# Bundles a folder (default: assets) into an Emscripten file package: game.data
# (every file, in one download) and game.data.js (its loader).  The page loads
# the package into the in-memory file system before the engine starts, and the
# engine then reads every file from there instead of fetching each one.
#
# Usage: scripts/package-web.sh [assets-dir] [output-dir]
#   assets-dir   folder to package; it appears to the game as assets/ (default: assets)
#   output-dir   where to write the package; normally your web build (default: build-web)
#
# If output-dir holds the engine page (raylib-miniscript.html), the loader's
# <script> tag is added to it.  Run this again after every web build, since
# the build rewrites the page.  See "Publishing to the Web" in the wiki.

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

cd "$(dirname "$0")/.."

ASSETS_DIR="${1:-assets}"
OUT_DIR="${2:-build-web}"
PAGE="$OUT_DIR/raylib-miniscript.html"

if [ ! -d "$ASSETS_DIR" ]; then
    echo -e "${RED}Error: no such folder: $ASSETS_DIR${NC}"
    exit 1
fi
if [ ! -f "$ASSETS_DIR/main.ms" ]; then
    echo -e "${YELLOW}Warning: $ASSETS_DIR has no main.ms; the engine runs assets/main.ms.${NC}"
fi
mkdir -p "$OUT_DIR"

# Find file_packager.py: the active Emscripten first, then the emsdk in the
# project root (the same one build-web.sh uses).
PACKAGER=""
if [ -n "$EMSDK" ] && [ -f "$EMSDK/upstream/emscripten/tools/file_packager.py" ]; then
    PACKAGER="$EMSDK/upstream/emscripten/tools/file_packager.py"
elif [ -f "emsdk/upstream/emscripten/tools/file_packager.py" ]; then
    PACKAGER="emsdk/upstream/emscripten/tools/file_packager.py"
else
    echo -e "${RED}Error: can't find Emscripten's file_packager.py.${NC}"
    echo "Activate Emscripten (source emsdk_env.sh) or put emsdk in the project root, as for build-web.sh."
    exit 1
fi

echo -e "${YELLOW}Packaging $ASSETS_DIR...${NC}"
# --exclude drops dotfiles such as .DS_Store; --no-node leaves out the Node.js
# support the page doesn't need.
python3 "$PACKAGER" "$OUT_DIR/game.data" \
    --preload "$ASSETS_DIR@/assets" \
    --exclude '*/.*' \
    --no-node --quiet \
    --js-output="$OUT_DIR/game.data.js"
if [ $? -ne 0 ]; then
    echo -e "${RED}Packaging failed!${NC}"
    exit 1
fi
ls -lh "$OUT_DIR/game.data" "$OUT_DIR/game.data.js"

TAG='<script src="game.data.js"></script>'
if [ -f "$PAGE" ]; then
    # The loader must run before the engine script, so it goes just ahead of it.
    python3 -I - "$PAGE" <<'EOF'
import re, sys
path = sys.argv[1]
page = open(path).read()
if "game.data.js" in page:
    print("The page already loads game.data.js.")
    sys.exit(0)
engine = re.search(r'<script[^>]*\bsrc="?raylib-miniscript\.js"?[^>]*>', page)
if not engine:
    print("Couldn't find the engine <script> tag in " + path + "; add the tag by hand.")
    sys.exit(1)
page = page[:engine.start()] + '<script src="game.data.js"></script>' + page[engine.start():]
open(path, "w").write(page)
print("Added the package loader to " + path + ".")
EOF
    if [ $? -ne 0 ]; then
        echo "  $TAG"
        exit 1
    fi
else
    echo "No $PAGE here; in your page, add this after the Module definition and before the engine script:"
    echo "  $TAG"
fi

echo -e "${GREEN}Done!${NC}  Deploy game.data and game.data.js with the page; the loose $ASSETS_DIR folder isn't needed."
