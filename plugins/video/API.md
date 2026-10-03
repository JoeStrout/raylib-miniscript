# video plugin

Play video in a script: `video.load` a file, `video.play` it, and each frame call `video.update`
and draw `video.texture`.  Desktop builds decode VP8 (WebM or IVF) with libvpx, and play the Vorbis
soundtrack through raylib's audio device.  Web builds hand the file to the browser, so anything the
browser plays works.

```
if not raylib.IsWindowReady then raylib.InitWindow
v = video.load("clip.webm")
if v == null then
    print "can't play: " + video.lastError
    exit
end if
video.setLooping v
video.play v
while not raylib.WindowShouldClose
    video.update v
    raylib.BeginDrawing
    raylib.ClearBackground raylib.BLACK
    raylib.DrawTexture video.texture(v), 20, 20, raylib.WHITE
    raylib.EndDrawing
    yield
end while
video.unload v
```

Check `plugins.video.loaded` (or `video.backend`) to find out whether this build can play video at all.

<!-- BEGIN GENERATED API (scripts/gen_doc.ms; edits between the markers are lost) -->

## video

|Name | Parameters | Purpose |
|-----|------------|---------|
|load |**fileName** |Load a video (VP8 in WebM or IVF on desktop, with Vorbis sound; anything the browser plays on web) and return it, or null if it can't be loaded -- see lastError.  Desktop paths go through the file system like the file module's, so a sandboxed program sees only its mounts; the whole file is read into memory (limit 512 MB).  On web the name is a URL relative to the page.  The video owns its texture: show it with texture(video), and don't call UnloadTexture on it |
|isValid |**video** |Return 1 if this is a loaded video that has not been unloaded, else 0 |
|play |**video** |Start (or restart, if it has finished) playing the video.  Nothing advances until you call update every frame |
|pause |**video** |Pause playback, keeping the position |
|resume |**video** |Continue a paused video from where it stopped |
|stop |**video** |Stop playback and rewind to the start |
|seek |**video**, **position**=0 |Jump to the given position in seconds |
|update |**video** |Advance the video to where it should be now and refresh its texture.  Call this once per frame, before drawing the texture |
|isPlaying |**video** |Return 1 if the video is playing, 0 if paused, stopped, or finished |
|timeLength |**video** |Total length of the video, in seconds |
|timePlayed |**video** |Current position of the video, in seconds |
|texture |**video** |The Texture holding the current frame; draw it with any Texture function.  The same texture is returned every time, and it belongs to the video: it is released by unload, not by UnloadTexture |
|info |**video** |Return a map describing the video: path, container, codec, width, height, frameRate, frameCount, timeLength, playbackRate, looping, hasAudio, audioCodec, audioSampleRate, audioChannels (and a few more) |
|backend |**video**=null |Name of the decoder in use: "libvpx" (desktop), "browser" (web), or "none" if this build has no video support.  Pass a video to get the one that video uses (null if it is not a loaded video) |
|lastError |**video**=null |Text of the most recent failure: the last load's, or, given a video, that video's most recent decode error.  Empty if there was none |
|diagnostics |**video** |Return a map of read-only playback statistics: sync mode, audio/video skew, frames decoded and skipped, frames dropped, audio buffer fill.  For tuning and debugging |
|setLooping |**video**, **enabled**=1 |Turn looping on or off (default 1).  A looping video starts over at the end instead of finishing |
|looping |**video** |Return 1 if the video is set to loop |
|setRate |**video**, **rate**=1.0 |Set the playback speed: 1 is normal, 2 is double speed, 0.5 is half |
|rate |**video** |Return the playback speed set by setRate |
|didLoop |**video** |Return 1 if a looping video has wrapped around since the last time you asked, then clear it (an event, not a state) |
|didFinish |**video** |Return 1 if the video has reached its end since the last time you asked, then clear it (an event, not a state) |
|unload |**video** |Release the video, its texture, and its sound.  Every copy of the video map becomes invalid.  Safe to call twice.  Videos you forget to unload are released when garbage collected |
<!-- END GENERATED API -->

## Notes

- **Files.**  On desktop the name goes through the file system, exactly like `file.*`: relative host paths
  until a program enters the sandbox, then only its mounts (`/usr/clip.webm`).  The whole file is held in
  memory, so keep clips modest; files over 512 MB are refused.  On web the name is a URL relative to the page,
  so the video must be deployed with the web build (put it in `assets/`).
- **Formats.**  Desktop: VP8 video in WebM (with Vorbis sound) or IVF (no sound).  Web: whatever the browser plays.
  VP9, H.264 and Opus are not supported on desktop.
- **The texture** belongs to the video.  Don't `UnloadTexture` it; `video.unload` releases it.  If a program does
  unload it, the video keeps playing without picture rather than crashing.
- **Timing.**  With sound, the picture follows the audio clock; without, the wall clock.  Call `update` every
  frame: frames are only decoded there.  `video.diagnostics` shows how the sync is doing.
- **Web.**  Browsers refuse to start un-muted playback until the user has clicked or pressed a key on the page; the
  video then starts muted.  `seek` only works if the web server supports HTTP range requests (real hosts do;
  Python's `http.server` does not).
- **Build.**  libvpx, libvorbis and libogg are found through pkg-config (macOS: `brew install libvpx libvorbis libogg`).
  Without libvpx the engine still builds, and `plugins.video.loaded` is 0; without libvorbis, videos play silently.
