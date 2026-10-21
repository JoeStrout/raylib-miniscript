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

When run with no script argument, it looks for `assets/main.ms` in the current working directory first, and failing that, next to the executable itself.  The second case is what lets a packaged app (where the payload ships beside the binary) work when launched from the Finder or a desktop shortcut, since the working directory is then something unrelated.

## Profiling

To find out where a script spends its time, run it with `--profile` and a name for the results:

```
build/raylib-miniscript --profile myprofile path/to/game.ms
```

Play as usual, then close the window, let the script end, or press Ctrl-C. Two files are written:

- `myprofile.txt` is a readable report: each function's *self* time (its own code, including the intrinsics it calls) and *total* time (also counting the MiniScript functions it calls), followed by the hottest source lines.
- `myprofile.folded` is a flame graph. Drag it onto [speedscope.app](https://www.speedscope.app), or feed it to `flamegraph.pl`, to see which call paths the time goes to.

The profiler samples: it runs the script a few hundred instructions at a time and records the full call stack and the time taken after each batch. Time is wall time.

- Most intrinsics are charged to the line where their batch ends, which is the line that called them or one shortly after it.
- Calls that can take long are timed exactly and charged to the line that made them, listed in brackets such as `[raylib.EndTextureMode]`: `EndDrawing`, `WaitTime`, `SwapScreenBuffer`, and the Begin/End mode calls that flush raylib's draw batch. `[raylib.EndDrawing]` includes the wait for `SetTargetFPS`, so most of it is usually idle time.
- Garbage collection, which MiniScript runs inside `yield` and `wait`, is timed and charged to that line as `(garbage collection)`.

`--profile-interval <n>` changes the average batch size (default 250). Smaller batches charge time to the right line more often, at a cost: on a CPU-bound benchmark, profiling adds about 20% to the run time at 250, 50% at 100, and 90% at 50. Profiling is desktop only.

## Documentation

Most of the added intrinsics (i.e., ones not part of the standard MiniScript language) are direct wrappings of the Raylib APIs.

There are a few additional intrinsics also added, including a `file` module and corresponding `FileHandle` class, a `RawData` class, and an `http` module.

See [API_DOC.md](API_DOC.md) for the full list of Raylib and additional intrinsics.  Also see the [raylib-miniscript wiki](https://github.com/JoeStrout/raylib-miniscript/wiki) for a searchable database including both standard and added functions.

