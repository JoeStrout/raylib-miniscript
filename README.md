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

Play as usual, then close the window, let the script end, or press Ctrl-C. Three files are written:

- `myprofile.txt` is a readable report: each function's *self* time (its own code, including the intrinsics it calls) and *total* time (also counting the MiniScript functions it calls), followed by the hottest source lines.
- `myprofile.speedscope.json` is for [speedscope.app](https://www.speedscope.app): drag it onto the page (nothing is uploaded). The menu at the top switches between two profiles of the same run. *By function* gives one bar per function; *By line* splits each function by line, so the Sandwich view lists the hottest lines and the flame graph shows which call site each call came through. Selecting a frame shows its file and line.
- `myprofile.folded` holds the same data as plain folded stacks, for `flamegraph.pl` and other tools that read that format.

The profiler samples: it runs the script a few hundred instructions at a time and records the full call stack and the time taken after each batch. Time is wall time.

- Most intrinsics are charged to the line where their batch ends, which is the line that called them or one shortly after it.
- Calls that can take long are timed exactly and charged to the line that made them, listed in brackets such as `[raylib.EndTextureMode]`: `EndDrawing`, `WaitTime`, `SwapScreenBuffer`, `UpdateMusicStream`, and the Begin/End mode calls that flush raylib's draw batch. `[raylib.EndDrawing]` includes the wait for `SetTargetFPS`, so most of it is usually idle time.
- Garbage collection, which MiniScript runs inside `yield` and `wait`, is timed and charged to that line as `(garbage collection)`.
- A `yield` always ends a sample, so the yield's line also gets the work done since the previous sample: the last few hundred instructions before it, and any untimed calls among them.

`--profile-interval <n>` changes the average batch size (default 250). Smaller batches charge time to the right line more often, at a cost: on a CPU-bound benchmark, profiling adds about 20% to the run time at 250, 50% at 100, and 90% at 50. Profiling is desktop only.

### Viewing a profile

raylib-miniscript comes with a profile viewer written in MiniScript:

```
build/raylib-miniscript tools/profview.ms myprofile.speedscope.json
```

It shows a flame graph above a table of functions. Each bar in the flame graph is a function, as wide as its share of the time, with the functions it called beneath it; MiniScript functions are warm colors, timed raylib calls blue and garbage collection green. Hover over a bar for its file, line and times. Click a bar to zoom in, and right-click, Escape or Backspace to zoom back out (Home goes all the way out). Click a table row to light that function up wherever it appears, and click a column heading to sort by self or total time. Tab switches between the *By function* and *By line* views, and the mouse wheel scrolls whichever panel it is over.

## Documentation

Most of the added intrinsics (i.e., ones not part of the standard MiniScript language) are direct wrappings of the Raylib APIs.

There are a few additional intrinsics also added, including a `file` module and corresponding `FileHandle` class, a `RawData` class, and an `http` module.

See [API_DOC.md](API_DOC.md) for the full list of Raylib and additional intrinsics.  Also see the [raylib-miniscript wiki](https://github.com/JoeStrout/raylib-miniscript/wiki) for a searchable database including both standard and added functions.

