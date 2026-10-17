# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

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
