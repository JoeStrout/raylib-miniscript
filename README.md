# raylib-miniscript

**Create videogames in [MiniScript](https://miniscript.org), powered by [Raylib](https://www.raylib.com/) graphics!**

This project provides raylib bindings (API wrappers) for MiniScript 2, letting you write 2D and 3D games in the MiniScript language.  The build products are executables for desktop, as well as a web (HTML/JS/emscripten) build.  You, the game developer, can just download the build for the platform of interest, drop your MiniScript and asset files next to it, and run — no other compiler needed.


## Setup & Build

1. Clone this repo to your local machine.
2. Run `git submodule update --init` once after cloning, to pull down the raylib module.
3. Also clone the [miniscript2 repo](https://github.com/JoeStrout/miniscript2) as a sibling of this repo (so that it sits at `../miniscript2`).
4. Transpile MiniScript 2's C# reference implementation to C++ (this produces the `generated/` sources that raylib-miniscript compiles):
```
cd ../miniscript2
./tools/build.sh transpile
cd ../raylib-miniscript
```
5. Symlink the miniscript2 repo into the directory of this repo, e.g.:
```
ln -s ../miniscript2 MiniScript2
```
6. Build raylib-miniscript with `scripts/build-desktop.sh` (this will also build raylib if needed).
7. Run with `build/raylib-miniscript`.  This will look for `assets/main.ms`, unless you specify some other script file for it to launch.

For the web build, install [Emscripten](https://emscripten.org/) and run `scripts/build-web.sh` instead; output goes to `build-web/`.

When run with no script argument, it looks for `assets/main.ms` in the current working directory first, and failing that, next to the executable itself.  The second case is what lets a packaged app (where the payload ships beside the binary) work when launched from the Finder or a desktop shortcut, since the working directory is then something unrelated.

## Documentation

Most of the added intrinsics (i.e., ones not part of the standard MiniScript language) are direct wrappings of the Raylib APIs.

There are a few additional intrinsics also added, including a `file` module and corresponding `FileHandle` class, a `RawData` class, a `Matrix` class (with physics helpers), an `http` module, and an `Interp` class for hosting a second, child interpreter from script.  (`Interp` can be disabled at build time with `-DMS_ENABLE_INTERP=OFF`.)

See [API_DOC.md](API_DOC.md) for the full list of Raylib and additional intrinsics.  Also see the [raylib-miniscript wiki](https://github.com/JoeStrout/raylib-miniscript/wiki) for a searchable database including both standard and added functions.

Design notes for the larger additions live in [notes/](notes/): `MATRIX_DESIGN.md`, `NN_INFERENCE.md`, `HOSTING_MS.md` (`Interp`), and `SANDBOXING.md` (the virtual file system).

### Resource handles

Native raylib objects (images, textures, fonts, sounds, models, and so on) are exposed to script as maps whose `_handle` field is an opaque handle.  Scripts can't forge a handle from a number, and a handle of the wrong type is rejected.  Resources are still released explicitly with the matching `Unload*` call; after that, every copy of the object is dead, and using it is a harmless no-op.

## Tests

Headless C++ tests live in `tests/`.  Build and run them via CMake, e.g.:
```
cmake --build build --target handle_tests interp_tests fs_tests
./build/handle_tests
```
