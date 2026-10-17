# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

## Interp instances carry a raw pointer as a number

`InterpModule.cpp` keeps each `Interp` instance's `InterpHandle*` in `_handle`
as a plain number, and `GetHandle` casts any numeric `_handle` straight back to
a pointer.  So `{"_handle": 12345, "__isa": Interp}` (or writing over a real
instance's `_handle`) makes the host call into arbitrary memory, the same hole
the raylib resource maps had before they moved to `Value::NewHandle`.  An
`Interp` that is dropped without `dispose` also leaks its whole child VM.

The fix is the same: an opaque handle whose finalizer deletes the
`InterpHandle`.  `dispose` then has to tear the child down early without
deleting the object the handle still owns (or mark the handle disposed, as
`TakeResource` does in `RaylibTypes.h`).

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
