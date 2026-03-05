# raylib-miniscript

**Create videogames in [MiniScript](https://miniscript.org), powered by [Raylib](https://www.raylib.com/) graphics!**

This project is developing raylib bindings (API wrappers) for MiniScript, enabling you to write 2D and 3D games in the MiniScript language.  The build products are executables for desktop, as well a web (HTML/JS/emscripten) build.  You, the game developer, will be able to just download the build for the platform of interest, drop your MiniScript and asset files next to it, and run — no other compiler needed.


## Setup & Build

1. Clone this repo to your local machine.
2. Also clone the raylib and miniscript repos.
3. Symlink each of those into the directory of this repo, e.g.:
```
cd raylib-miniscript
ln -s ../raylib raylib
ln -s ../miniscript/MiniScript-cpp/src/MiniScript MiniScript
```
4. Make sure the raylib lib is built (e.g. `cd ../raylib/src; make`).
5. Build raylib-miniscript with `scripts/build-desktop.sh`
6. Run with `build/miniscript-raylib`.  This will look for `assets/main.ms`, unless you specify some other script file for it to launch.

