# raylib-miniscript

**Create videogames in [MiniScript](https://miniscript.org), powered by [Raylib](https://www.raylib.com/) graphics!**

This project is developing raylib bindings (API wrappers) for MiniScript, enabling you to write 2D and 3D games in the MiniScript language.  The build products are executables for desktop, as well a web (HTML/JS/emscripten) build.  You, the game developer, will be able to just download the build for the platform of interest, drop your MiniScript and asset files next to it, and run — no other compiler needed.


## Setup & Build

1. Clone this repo to your local machine.
2. Run `git submodule update --init` once after cloning, to pull down the raylib module.
3. Also clone the [miniscript repo](https://github.com/JoeStrout/miniscript).  (We'll change this to a submodule once we move to MiniScript 2.)
4. Symlink the miniscript C++ source into the directory of this repo, e.g.:
```
cd raylib-miniscript
ln -s ../miniscript/MiniScript-cpp/src/MiniScript MiniScript
```
5. Build raylib-miniscript with `scripts/build-desktop.sh` (this will also build raylib if needed).
6. Run with `build/raylib-miniscript`.  This will look for `assets/main.ms`, unless you specify some other script file for it to launch.

## Documentation

Most of the added intrinsics (i.e., ones not part of the standard MiniScript language) are direct wrappings of the Raylib APIs.

There are a few additional intrinsics also added, including a `file` module and corresponding `FileHandle` class, a `RawData` class, and an `http` module.

See [API_DOC.md](API_DOC.md) for the full list of Raylib and additional intrinsics.  Also see the [raylib-miniscript wiki](https://github.com/JoeStrout/raylib-miniscript/wiki) for a searchable database including both standard and added functions.

