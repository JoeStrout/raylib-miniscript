# Plugins

Drop a folder here to add native intrinsics to the engine.  Everything under
`plugins/` is compiled into the one engine binary; adding a plugin needs no
change to `CMakeLists.txt` or `src/main.cpp`.  Re-run CMake (`cmake -S . -B build`)
after adding or removing a folder.

```
plugins/mything/
  plugin.cpp     required: defines the hooks below
  plugin.cmake   optional: include dirs, defines, libraries
  *.cpp *.h      any other sources are picked up automatically
```

The folder name must be a valid C identifier and unique (`plugins` is reserved). `plugin.cpp`
contains one `MS_PLUGIN(name, version, init, update, reset, shutdown)` line naming its hooks;
pass `nullptr` for any you don't need (only `init` is required):

```cpp
#include "Plugin.h"

static void Fill(MiniScript::ValueDict& m) { /* m.SetValue(String("fn"), f.GetFunc()); */ }

static bool Init() {                       // once, at interpreter creation
    return PluginAddModule<&Fill>("mything");   // false: name taken, or return false yourself
                                           // when unavailable on this run
}
static void Update()   { }                 // every frame, main thread
static void Reset()    { }                 // before the script (re)starts
static void Shutdown() { }                 // app closing (reverse load order)

MS_PLUGIN(mything, "1.0", Init, Update, Reset, Shutdown)
```

To make a hook conditional, choose its name with `#define` *outside* the macro; a preprocessor
directive inside the macro's arguments is undefined behavior and not portable:

```cpp
#ifdef PLATFORM_DESKTOP
  #define MYTHING_UPDATE Update
#else
  #define MYTHING_UPDATE nullptr
#endif
MS_PLUGIN(mything, "1.0", Init, MYTHING_UPDATE, nullptr, Shutdown)
```
(To leave a whole plugin out on a platform, set `MS_PLUGIN_SKIP` in `plugin.cmake` instead.)

### When init fails
An SDK often can't start (Steam isn't running, the app wasn't launched from Steam).  Have `init`
return `false`: the engine logs a warning, never calls the plugin's other hooks, and marks it
unavailable.  Release any SDK resources you acquired.  Intrinsics cannot be unregistered, so pick
one convention: call `PluginAddModule` last, only on success (the module then doesn't exist when
unavailable), or register it always and have its functions fail safely (return `false`/null) when
the SDK is unavailable, so a script that forgets to check doesn't crash.  Scripts should use
`plugins.<name>.loaded` as the authoritative check.  `PluginAddModule` itself returns false (and
logs an error) if the module name collides with an existing intrinsic; return that from `init`.

### Telling scripts what loaded
A global `plugins` map lists every plugin that was built in, so one script can ship to Steam, itch
and web and branch cleanly:
```
if plugins.hasIndex("steam") and plugins.steam.loaded then steam.unlock "FIRST_WIN"
print plugins.quadtree.version        // "1.0"
```
Each entry is `{"loaded": 1 or 0, "version": "..."}`.  A plugin left out of the build isn't listed.

See `quadtree/plugin.cpp` for a complete example, and `quadtree/test.ms` for a test script
(`./build/raylib-miniscript plugins/quadtree/test.ms`).

## plugin.cmake

Runs in the top-level scope, with `PLUGIN_NAME` and `PLUGIN_DIR` set, after the
platform sections of `CMakeLists.txt`:

```cmake
if(EMSCRIPTEN)
    set(MS_PLUGIN_SKIP TRUE)                  # leave this plugin out on web
else()
    target_compile_definitions(raylib-miniscript PRIVATE MYTHING_ENABLED)
    target_include_directories(raylib-miniscript PRIVATE ${PLUGIN_DIR}/sdk/include)
    target_link_libraries(raylib-miniscript ${PLUGIN_DIR}/sdk/libmything.a)  # no PRIVATE/PUBLIC
endif()
```

Leave plugins out of a build with `-DMS_DISABLE_PLUGINS="mything;other"`.

## Optional dependencies

A plugin that needs a system library finds it in `plugin.cmake` and degrades gracefully: define a macro when
the library is there (`target_compile_definitions(raylib-miniscript PRIVATE HAVE_FOO=1)`), link it with
`target_link_libraries(raylib-miniscript PkgConfig::FOO)` (use `pkg_check_modules(FOO IMPORTED_TARGET foo)`),
and have `init` return `false` when it is missing so the build never fails and scripts can check
`plugins.<name>.loaded`.  `plugins/video` does this for libvpx and libvorbis.  A web-only JavaScript half goes in
a `web/` folder and is linked with `target_link_options(raylib-miniscript PRIVATE "SHELL:--pre-js ${PLUGIN_DIR}/web/<file>.js")`.

## Plugins outside this repo

Some SDKs (Steamworks, most ad and purchase SDKs) can't be redistributed in a public tree.  Keep
those plugins in a private repo or git submodule, laid out like `plugins/` (a folder per plugin), and
point the build at it:

```
cmake -S . -B build -DMS_EXTRA_PLUGIN_DIRS="/path/to/private-plugins;../another"
```
(or set the `MS_EXTRA_PLUGIN_DIRS` environment variable; relative paths start at the repo root).  Names
must be unique across all directories.  A Steam-style `plugin.cmake` would copy the redistributable
(`steam_api64.dll`, `libsteam_api.so`, `libsteam_api.dylib`) next to the executable in a POST_BUILD
step, set the rpath on Linux and macOS, and write `steam_appid.txt` for development builds.

`scripts/gen_doc.ms` documents external plugins too (run it with the same `MS_EXTRA_PLUGIN_DIRS` set),
writing each one's `API.md` but adding no link to the public `API_DOC.md`.

## Documenting your plugin

`scripts/gen_doc.ms` writes your plugin's API into `plugins/<name>/API.md` (and adds a link
to it in `API_DOC.md`).  It finds your functions from the `SetValue(String("fn"), f.GetFunc())`
lines in the function you pass to `PluginAddModule`, so keep that shape.  The "Purpose" column
comes from the comment directly above each `Intrinsic::Create`; for more than one line, or text
that must be exact, put `// API: text` lines there instead (several are joined with spaces).

The generated table sits between `<!-- BEGIN GENERATED API -->` and `<!-- END GENERATED API -->`
markers.  Write your own notes (setup, caveats, examples) anywhere outside them: reruns replace
only what is between the markers.  Run it from the repo root: `./build/raylib-miniscript scripts/gen_doc.ms`.

## Guidance

- Register under your own module name (`PluginAddModule`), so you can't clobber core intrinsics.
- Batch work: let a script make one native call for many items, not one per item.
- Keep native objects in GC handles with a finalizer (see `NewNativeHandle` in `src/RaylibTypes.h`).
- Async SDK results (purchases, ads, Steam callbacks) must never touch the VM from another thread.
  Push plain C++ structs onto a `PluginEventQueue<T>` (`src/PluginEvents.h`; thread-safe, bounded) from the
  SDK's callback, and have a script-callable `poll` return them with `PluginDrainEvents`, which builds
  the MiniScript maps on the main thread.  Pump the SDK itself (e.g. `SteamAPI_RunCallbacks`) in your update hook.
- The queue's default (drop oldest when full) suits lossy events (overlay, ad status).  For purchases
  and entitlements use `PluginEventQueue<T>(0)` (unbounded), expose `Dropped()` to scripts (e.g.
  `iap.droppedEvents`), and don't finish a store transaction until the script acknowledges it;
  StoreKit and Play Billing redeliver unfinished transactions, so a lost event is recoverable.
- A `Value` you keep across frames (a stored callback, a cached map) must be GC-rooted: hold it in a
  `PluginRooted` (also in `src/PluginEvents.h`), or call `GCManager::AddRoot` yourself.
  Call `Clear()` on each `PluginRooted` in your shutdown hook, and in your reset hook if it holds a
  script's closure (so a dead script's callback isn't kept alive).
- Never grant an in-app purchase entitlement on the client's "purchase succeeded" callback alone;
  verify the receipt on a server (the `http` module can make that call).
- Log through raylib (`TraceLog(LOG_INFO / LOG_WARNING / LOG_ERROR, "MYTHING: ...", ...)`), not `printf`, so plugin messages follow the engine's log level and callback.
- Guard platform-specific code with `PLATFORM_DESKTOP` / `PLATFORM_WEB`.
- Plugins are trusted code: they run in-process with full access, outside the file-system sandbox.
