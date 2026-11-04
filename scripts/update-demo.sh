#!/bin/bash

# Update the live demo hosted on GitHub Pages (https://joestrout.github.io/raylib-miniscript/).
#
# Pages serves the docs/ folder of the main branch.  This script rebuilds the
# web version, then replaces docs/ with the engine page plus a file package of
# the current assets folder.  Review and commit docs/, then push; Pages
# republishes a minute or so later.
#
# Usage: scripts/update-demo.sh

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

cd -P "$(dirname "$0")/.."

scripts/build-web.sh || exit 1

DOCS=docs
echo -e "${YELLOW}Refreshing $DOCS/...${NC}"
rm -rf "$DOCS"
mkdir -p "$DOCS"
for f in index.html raylib-miniscript.html raylib-miniscript.js raylib-miniscript.wasm; do
    if [ ! -f "build-web/$f" ]; then
        echo -e "${RED}Error: build-web/$f is missing.${NC}"
        exit 1
    fi
    cp "build-web/$f" "$DOCS/"
done

# Bundle assets into game.data (and add its loader to the page), so the demo
# starts with one download instead of a request per file.
scripts/package-web.sh assets "$DOCS" || exit 1

# Serve the files as they are, without running them through Jekyll.
touch "$DOCS/.nojekyll"

echo ""
ls -lh "$DOCS"
echo ""
echo -e "${GREEN}Demo updated in $DOCS/.${NC}"
echo "To test locally: cd $DOCS && python3 -m http.server 8000, then open http://localhost:8000"
echo "To publish: commit $DOCS/ and push."
