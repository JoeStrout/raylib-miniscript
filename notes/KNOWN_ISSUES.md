# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

## Resource maps carry raw pointers as numbers

Every raylib resource map (Image, Texture, Font, Wave, Music, Sound,
AudioStream, RenderTexture, Shader, Mesh, Material, Model, ModelAnimation,
AutomationEventList, and rlRenderBatch) stores its C++ heap pointer in `_handle`
as a plain number.  `ValueToPointer` (`RaylibTypes.h`) casts that number
straight back to a pointer and every binding dereferences it unchecked.  About
36 call sites, across `RaylibTypes.cpp`, `RAudio.cpp`, `RModels.cpp`,
`RTextures.cpp`, `RCore.cpp`, `RText.cpp`, and `RLgl.cpp`.

So any map with a numeric `_handle` passes for a real resource: a script can
write `img._handle = 12345`, or just build `{"_handle": 12345}`, and the host
will read or write whatever memory that names.  That is a crash at best, and a
hole in the sandbox at worst.  Freezing the maps would not help, since a script
can always build a fresh one.

This is legacy code.  MiniScript 2 has an opaque handle type for exactly this:
`Value::NewHandle(ptr, finalizer)` and `Value::HandlePtr()`, which script cannot
forge.  `FileModule.cpp`, `MatrixCore.cpp`, and `RawData.cpp` already use it.
The raylib resource maps should be converted to the same thing, which would
also let the finalizer free the resource when the map is collected.

## Music ignores `looping`

`MusicToValue` copies `music.looping` into the map, but nothing reads the map's
`looping` back: `ValueToMusic` returns the struct behind `_handle` as it was
loaded.  raylib enables looping by default in `LoadMusicStream`, so a music
stream can never be made to stop looping from script.

This shows up in Soda: `Sound.play` and `Sound.adjust` set
`self._music.looping = self.loop`, so a streamed (.ogg or .mp3) `Sound` with
`loop = false` still loops.  Probably the simplest fix is for `ValueToMusic`
to apply the map's `looping` to the Music it returns, since `UpdateMusicStream`
gets a fresh copy through `ValueToMusic` every frame anyway.
