//
//  plugins/video/plugin.cpp
//
//  Video playback: VP8 (WebM or IVF) with Vorbis audio on desktop, through libvpx and
//  libvorbis; the browser's own decoder on web.  Adds the `video` module -- see API.md.
//
//  Needs libvpx (and libvorbis, for sound) at build time; plugin.cmake finds them.
//  Without libvpx on desktop the plugin reports itself unavailable (plugins.video.loaded
//  is 0) rather than failing the build.
//

#include "Plugin.h"
#include "RVideo.h"
#include "raylib.h"

static void Fill(MiniScript::ValueDict& m) { AddRVideoMethods(m); }

static bool Init() {
#if !defined(PLATFORM_WEB) && !HAVE_LIBVPX
	TraceLog(LOG_WARNING, "PLUGIN: video: built without libvpx; video playback is unavailable");
	return false;
#else
#if !defined(PLATFORM_WEB) && !HAVE_LIBVORBIS
	TraceLog(LOG_WARNING, "PLUGIN: video: built without libvorbis; videos will play without sound");
#endif
	return PluginAddModule<&Fill>("video");
#endif
}

static void Update() { VideoDrainPending(); }

static void Shutdown() { VideoShutdownAll(); }

MS_PLUGIN(video, "1.0", Init, Update, nullptr, Shutdown)
