// video plugin -- tests.  Desktop, from the repo root:
//   ./build/raylib-miniscript plugins/video/test.ms
// Opens a small window (video frames are textures).  Exits with 0 if every check passes.
// Needs a build with libvpx; without it the plugin reports itself unavailable and this
// script says so and exits.
//
// Web: copy plugins/video/testdata/*.webm into the served assets/ folder (the browser
// fetches videos by URL) and use this script as assets/main.ms.  Checks that depend on
// the libvpx decoder (frame counts, container details, audio pacing) are skipped there.

if not raylib.IsWindowReady then raylib.InitWindow

failures = 0
checks = 0

ok = function(cond, label)
	outer.checks = outer.checks + 1
	if cond then
		print "ok   " + label
	else
		outer.failures = outer.failures + 1
		print "FAIL " + label
	end if
end function

eq = function(got, want, label)
	ok got == want, label + "  (got " + got + ", want " + want + ")"
end function

near = function(got, want, tol, label)
	ok abs(got - want) <= tol, label + "  (got " + got + ", want " + want + " +-" + tol + ")"
end function

finish = function
	print
	print checks + " checks, " + failures + " failures"
	if video.backend == "libvpx" then exit failures > 0
end function

if not plugins.hasIndex("video") or not plugins.video.loaded then
	print "video plugin unavailable in this build (no libvpx); nothing to test"
	exit 0
end if

libvpx = video.backend == "libvpx"     // else the browser decodes (web)
dir = "plugins/video/testdata/"
if not libvpx then dir = "assets/"
textures = function
	return resourceCounts.Texture
end function

// Run frames until cond() is true or `seconds` pass; returns true if cond() became true.
runUntil = function(v, seconds, cond)
	t0 = raylib.GetTime
	while raylib.GetTime - t0 < seconds
		video.update v
		raylib.BeginDrawing
		raylib.ClearBackground raylib.BLACK
		raylib.DrawTexture video.texture(v), 4, 4, raylib.WHITE
		raylib.EndDrawing
		if cond(v) then return true
		yield
	end while
	return false
end function

// ---- the plugin ----
eq plugins.video.version, "1.0", "plugins.video.version"
ok video.backend == "libvpx" or video.backend == "browser", "backend with no video names the decoder"

// ---- loading ----
baseTextures = textures
v = video.load(dir + "clip.webm")
ok v != null, "load clip.webm"
eq video.lastError, "", "no error after a good load"
eq v.width, 96, "width"
eq v.height, 54, "height"
near v.timeLength, 1.5, 0.05, "timeLength"
eq v.isPlaying, 0, "not playing after load"
eq video.isValid(v), 1, "isValid"
eq video.backend(v), video.backend, "backend(video) is the build's decoder"
eq textures, baseTextures + 1, "the video's texture is counted in resourceCounts"
if libvpx then
	eq v.frameCount, 15, "frameCount"
	eq v.frameRate, 10, "frameRate"
	info = video.info(v)
	eq info.container, "webm", "info.container"
	eq info.codec, "vp8", "info.codec"
	eq info.hasAudio, 1, "webm with sound reports hasAudio"
	eq info.audioCodec, "A_VORBIS", "info.audioCodec"
	eq info.audioChannels, 2, "info.audioChannels"
end if
tex = video.texture(v)
ok tex != null, "texture(v)"
eq tex.width, 96, "the texture is the video's size"

if libvpx then
	ivf = video.load(dir + "clip.ivf")
	ok ivf != null, "load clip.ivf"
	eq video.info(ivf).container, "ivf", "ivf container"
	eq video.info(ivf).hasAudio, 0, "ivf has no sound"
	eq ivf.frameCount, 15, "ivf frameCount"
	video.unload ivf
end if

silent = video.load(dir + "silent.webm")
ok silent != null, "load silent.webm"
if libvpx then eq video.info(silent).hasAudio, 0, "silent.webm has no sound"
eq silent.width, 64, "silent width"

missing = video.load(dir + "nope.webm")
eq missing, null, "a missing file loads as null"
ok video.lastError != "", "and lastError says why"
if libvpx then
	ok video.lastError.indexOf("could not open") != null, "(could not open file)"
	notVideo = video.load("plugins/video/test.ms")
	eq notVideo, null, "a file that is not video loads as null"
	ok video.lastError != "", "with an error message"
end if
eq video.load(""), null, "an empty name loads as null"

// ---- playback ----
video.play v
eq video.isPlaying(v), 1, "isPlaying after play"
eq v.isPlaying, 1, "the map's isPlaying follows"
ok runUntil(v, 0.5, function(x); return x.timePlayed > 0.2; end function), "time advances while playing"
// The picture: read the texture back and look for anything but black.
img = raylib.LoadImageFromTexture(video.texture(v))
lit = 0
for y in range(4, 50, 9)
	for x in range(4, 92, 11)
		c = raylib.GetImageColor(img, x, y)
		if c.r + c.g + c.b > 60 then lit = lit + 1
	end for
end for
raylib.UnloadImage img
ok lit > 10, "the texture holds a picture, not black  (" + lit + " lit samples)"
video.pause v
eq video.isPlaying(v), 0, "pause"
frozen = video.timePlayed(v)
for i in range(1, 5)
	video.update v
	yield
end for
eq video.timePlayed(v), frozen, "time holds while paused"
video.resume v
eq video.isPlaying(v), 1, "resume"
ok runUntil(v, 3, function(x); return x.isFinished; end function), "plays to the end"
eq video.didFinish(v), 1, "didFinish reports the end"
eq video.didFinish(v), 0, "and only once"
eq video.isPlaying(v), 0, "not playing once finished"
if libvpx then
	eq v.currentFrame, 15, "every frame was shown"
	d = video.diagnostics(v)
	ok d.totalFramesDecoded >= 15, "diagnostics counts decoded frames"
	eq d.syncMode, "audio-stream", "a video with sound is paced by its audio"
end if

video.stop v
eq video.timePlayed(v), 0, "stop rewinds"
eq v.isFinished, 0, "and clears finished"

// play again after finishing restarts from the top
video.play v
ok runUntil(v, 0.5, function(x); return x.timePlayed > 0.2; end function), "plays again after stop"
video.seek v, 1.0
near video.timePlayed(v), 1.0, 0.11, "seek moves the position"
video.pause v

// ---- rate and looping ----
eq video.rate(v), 1, "rate defaults to 1"
video.setRate v, 2
eq video.rate(v), 2, "setRate"
video.setRate v, 1
eq video.looping(v), 0, "looping defaults off"
video.setLooping silent, 1
eq video.looping(silent), 1, "setLooping"
video.play silent
ok runUntil(silent, 4, function(x); return video.didLoop(x); end function), "a looping video raises didLoop at the end"
ok video.isPlaying(silent), "and keeps playing"

// ---- bad handles ----
eq video.update(null), null, "update(null) is harmless"
eq video.update(42), null, "update(number) is harmless"
eq video.update("video"), null, "update(string) is harmless"
eq video.update({"_handle": 12345}), null, "a forged numeric _handle is rejected"
eq video.isValid({"_handle": 12345}), 0, "isValid(forged)"
eq video.isValid(null), 0, "isValid(null)"
eq video.play({}), null, "play({}) is harmless"
eq video.timePlayed(null), null, "timePlayed(null)"
eq video.texture(null), null, "texture(null)"
eq video.info({"_handle": 7}), null, "info(forged)"
tx = video.texture(v)
fake = {"_handle": tx._handle}   // a real handle, but not a video's
eq video.update(fake), null, "a handle of another type is rejected"
eq video.isValid(fake), 0, "isValid(handle of another type)"

// ---- unload ----
copy = {} + v          // a second map sharing v's handle
video.unload v
eq video.isValid(v), 0, "unload invalidates"
eq video.isValid(copy), 0, "and every copy of the map"
eq video.update(copy), null, "using a copy after unload is harmless"
video.unload v
video.unload copy
ok true, "unloading twice is harmless"
video.unload silent
eq textures, baseTextures, "unloading returns the texture count to where it started"

// ---- script unloads the texture out from under the player ----
w = video.load(dir + "silent.webm")
eq textures, baseTextures + 1, "a new video"
raylib.UnloadTexture video.texture(w)
eq textures, baseTextures, "script's UnloadTexture is counted once"
video.play w
ok runUntil(w, 0.6, function(x); return x.timePlayed > 0.2; end function), "playing on after script unloaded its texture does not crash"
video.unload w
eq textures, baseTextures, "unloading afterwards does not unload twice"

// ---- forgotten videos are collected ----
for i in range(1, 5)
	g = video.load(dir + "silent.webm")
	video.play g
	video.update g
end for
g = null
eq textures, baseTextures + 5, "five videos left loaded"
gc.collect true
for i in range(1, 3)
	yield
end for
eq textures, baseTextures, "forgotten videos are released after a collection"

finish
