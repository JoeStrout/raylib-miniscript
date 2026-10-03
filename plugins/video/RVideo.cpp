//
//  RVideo.cpp
//  raylib-miniscript video plugin
//
//  The player behind the `video` module (see API.md): a WebM/IVF container parser,
//  libvpx VP8 decoding, libvorbis audio feeding a raylib AudioStream, and the browser
//  <video> backend on web (web/video.js).  plugin.cpp wires it into the plugin system.
//
//  A video is a map holding a GC handle to a VideoBox, which points at a VideoPlayerState.
//  The file is read whole through the virtual file system (fs::ReadBinary) so sandboxed
//  hosts see only their mounts.  The player's texture is a normal raylib Texture handle
//  that the player keeps rooted, so it can tell when a script unloads it.
//

#include "Plugin.h"
#include "PluginEvents.h"
#include "FileSystem.h"
#include "RawData.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "miniscript.h"
#include "macros.h"

#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>
#include <cstring>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#ifndef HAVE_LIBVPX
#define HAVE_LIBVPX 0
#endif

#ifndef HAVE_LIBVORBIS
#define HAVE_LIBVORBIS 0
#endif

#if HAVE_LIBVPX
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#endif

#if HAVE_LIBVORBIS
#include <vorbis/codec.h>
#endif

using namespace MiniScript;

// Some helpers are only used in some build configurations (no libvpx, no libvorbis, web).
#define VIDEO_MAYBE_UNUSED __attribute__((unused))

namespace {

struct EncodedFrame {
	long offset = 0;
	uint32_t size = 0;
	double pts = 0.0;
};

struct VideoBox;

struct VideoPlayerState {
	VideoBox* box = nullptr;   // the script-side handle, while one exists (see VideoBox)
	std::string path;
	std::string container;
	std::string codecName;
	std::string lastError;
	std::string audioCodecName;
	std::string audioDecodePath;
	bool valid = false;
	bool playing = false;
	bool finished = false;
	bool hasAudio = false;
	bool webPlayer = false;
	bool audioDecodeScaffoldReady = false;
	bool vorbisHeaderParseAttempted = false;
	bool vorbisIdentificationSeen = false;
	bool vorbisCommentSeen = false;
	bool vorbisSetupSeen = false;
	bool vorbisHeadersFromCodecPrivate = false;
	bool vorbisHeadersFromPacketStream = false;
	bool vorbisSeedHeaderParseAttempted = false;
	bool vorbisSeedIdentificationSeen = false;
	bool vorbisSeedCommentSeen = false;
	bool vorbisSeedSetupSeen = false;
	bool vorbisSeedHeadersFromCodecPrivate = false;
	int webHandle = 0;
	int width = 0;
	int height = 0;
	int audioChannels = 0;
	int vorbisParsedChannels = 0;
	int vorbisSeedParsedChannels = 0;
	uint32_t audioDecodeSessionId = 0;
	uint32_t audioPacketsRead = 0;
	uint64_t audioBytesRead = 0;
	uint64_t decodedPcmFramesAvailable = 0;
	uint64_t decodedPcmFramesTotal = 0;
	uint64_t decodedPcmFramesConsumed = 0;
	uint64_t decodedPcmFramesClockEstimated = 0;
	uint64_t decodedPcmFramesClockConsumed = 0;
	uint32_t vorbisPacketsDecoded = 0;
	uint32_t autoRefillTriggerCount = 0;
	uint32_t autoRefillPacketsDecoded = 0;
	uint32_t autoRefillTargetHitCount = 0;
	double autoRefillLastLatencyMs = 0.0;
	double autoRefillTargetLatencyMs = 0.0;
	double autoRefillLatencyGainMsTotal = 0.0;
	uint32_t audioPacketCount = 0;
	double audioFirstPacketTime = 0.0;
	double audioLastPacketTime = 0.0;
	double audioLastReadPacketTime = 0.0;
	double decodedPcmDrainRemainder = 0.0;
	double audioSampleRate = 0.0;
	double vorbisParsedSampleRate = 0.0;
	double vorbisSeedParsedSampleRate = 0.0;
	uint32_t frameCount = 0;
	double frameRate = 0.0;
	double timeLength = 0.0;
	double timePlayed = 0.0;
	double playbackRate = 1.0;
	uint32_t currentFrame = 0;
	double playBaseClock = 0.0;
	double playBaseTime = 0.0;
	double lastObservedTime = 0.0;
	std::string syncMode = "wall-clock";
	bool audioLedSyncEnabled = true;
	bool audioSyncOffsetPrimed = false;
	bool audioSyncClampAdaptive = true;
	double audioSyncClampWindowMs = 120.0;
	double audioSyncClampManualWindowMs = 120.0;
	double audioSyncClampAutoWindowMs = 120.0;
	double audioSyncClampRawWindowMs = 120.0;
	double audioSyncClampSmoothingAlpha = 0.20;
	double audioSyncClampMaxStepMs = 12.0;
	double audioSyncOffsetSec = 0.0;
	double lastAudioClockSec = 0.0;
	double lastAudioLedTargetSec = 0.0;
	double audioStreamMediaBaseSec = 0.0;
	double videoAudioSyncSkewMs = 0.0;    // video clock ahead of audio-consumed position (ms); negative = video behind
	double lastDecodedFramePts = 0.0;    // PTS of most recently VP8-decoded frame
	double lastUpdateTargetPts = 0.0;    // targetTime computed in last video.update
	uint32_t totalFramesDecoded = 0;     // cumulative VP8 decode calls (never resets on rewind)
	uint32_t totalFramesSkipped = 0;     // stale frames advanced past without decoding (catch-up)
	uint32_t totalFrameDropEvents = 0;   // times decode budget was exhausted with pending frames
	int lastDecodeBudgetUsed = 0;        // frames decoded in the most recent video.update tick
	bool lastDecodeBudgetExhausted = false; // budget saturated AND frames still pending after last tick
	bool looping = false;
	bool loopEventPending = false;
	bool finishEventPending = false;
	bool finishedPrev = false;
	std::vector<unsigned char> data;   // the whole file, read through the virtual file system
	// The texture handed to script (see VideoPlayerToValue).  Rooted for as long as the
	// player lives, so its NativeBox outlives every script-side copy of the map: that is
	// how we learn that script called UnloadTexture on it.
	PluginRooted textureMap;
	PluginRooted textureHandle;
	Texture2D texture = {0};
	std::vector<unsigned char> rgba;
#if HAVE_LIBVPX
	vpx_codec_ctx_t codec;
	bool codecInited = false;
	std::vector<EncodedFrame> frames;
	std::vector<EncodedFrame> audioPackets;
	size_t nextFrameIndex = 0;
	size_t nextAudioPacketIndex = 0;
#endif
	// Raw Vorbis header packets extracted from CodecPrivate (always three: id, comment, setup)
	std::vector<std::vector<unsigned char>> vorbisCodecPrivateHeaders;

#if HAVE_LIBVORBIS
	vorbis_info      vorbisInfo;
	vorbis_comment   vorbisComment;
	vorbis_dsp_state vorbisState;
	vorbis_block     vorbisBlock;
	bool vorbisDecoderInited = false;
	bool vorbisBlockInited = false;
	// Audio stream for real playback
	AudioStream audioStream = {0};
	bool audioStreamInited = false;
	bool audioStreamPlaying = false;
	// Interleaved float PCM buffer awaiting consumption by the audio callback
	std::vector<float> pcmPlaybackBuffer;
	size_t pcmPlaybackHead = 0;
	uint64_t pcmFramesFedToStream = 0;
	std::mutex pcmPlaybackMutex;
	int audioStreamBufferFrames = 2048;  // used for refill sizing; actual playback is callback-driven
#endif
};

// False once script has called UnloadTexture on the player's texture: we must not
// upload to a texture that is gone.  True before the texture is handed out.
static VIDEO_MAYBE_UNUSED bool TextureStillOurs(const VideoPlayerState* state) {
	Value handle = state->textureHandle.Get();
	if (handle.IsNull()) return true;
	return NativeHandlePtr<Texture>(handle) != nullptr;
}

static std::string gLastVideoLoadError;

#if HAVE_LIBVORBIS
static std::atomic<VideoPlayerState*> gActiveAudioCallbackState { nullptr };

static uint64_t GetBufferedPcmFramesLocked(const VideoPlayerState* state) {
	if (!state) return 0;
	int ch = state->audioChannels > 0 ? state->audioChannels : 1;
	if (ch <= 0) ch = 1;
	size_t availableFloats = (state->pcmPlaybackHead < state->pcmPlaybackBuffer.size())
		? state->pcmPlaybackBuffer.size() - state->pcmPlaybackHead : 0;
	return (uint64_t)(availableFloats / (size_t)ch);
}

static void CompactPcmPlaybackBufferLocked(VideoPlayerState* state) {
	if (!state) return;
	if (state->pcmPlaybackHead >= state->pcmPlaybackBuffer.size() / 2) {
		state->pcmPlaybackBuffer.erase(
			state->pcmPlaybackBuffer.begin(),
			state->pcmPlaybackBuffer.begin() + (ptrdiff_t)state->pcmPlaybackHead);
		state->pcmPlaybackHead = 0;
	}
	state->decodedPcmFramesAvailable = GetBufferedPcmFramesLocked(state);
}

static void RefreshDecodedPcmFramesAvailable(VideoPlayerState* state) {
	if (!state) return;
	std::lock_guard<std::mutex> lock(state->pcmPlaybackMutex);
	state->decodedPcmFramesAvailable = GetBufferedPcmFramesLocked(state);
}

static void SetActiveAudioCallbackState(VideoPlayerState* state) {
	gActiveAudioCallbackState.store(state, std::memory_order_release);
}

static void ClearActiveAudioCallbackState(VideoPlayerState* state) {
	VideoPlayerState* expected = state;
	gActiveAudioCallbackState.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
}

static void VideoAudioStreamCallback(void *bufferData, unsigned int frames) {
	float* out = (float*)bufferData;
	VideoPlayerState* state = gActiveAudioCallbackState.load(std::memory_order_acquire);
	unsigned int channels = 2;
	if (state && state->audioChannels > 0) channels = (unsigned int)state->audioChannels;
	if (channels == 0) channels = 1;
	size_t requestedFloats = (size_t)frames * (size_t)channels;
	memset(out, 0, requestedFloats * sizeof(float));
	if (!state || !state->valid || !state->audioStreamPlaying) return;

	std::lock_guard<std::mutex> lock(state->pcmPlaybackMutex);
	size_t availableFloats = (state->pcmPlaybackHead < state->pcmPlaybackBuffer.size())
		? state->pcmPlaybackBuffer.size() - state->pcmPlaybackHead : 0;
	size_t copyFloats = std::min(requestedFloats, availableFloats);
	if (copyFloats > 0) {
		memcpy(out, state->pcmPlaybackBuffer.data() + state->pcmPlaybackHead, copyFloats * sizeof(float));
		state->pcmPlaybackHead += copyFloats;
		uint64_t copiedFrames = (uint64_t)(copyFloats / (size_t)channels);
		state->pcmFramesFedToStream += copiedFrames;
		state->decodedPcmFramesConsumed += copiedFrames;
		CompactPcmPlaybackBufferLocked(state);
	}
}
#endif

#ifdef PLATFORM_WEB
EM_ASYNC_JS(int, WebVideoLoad, (const char* urlPtr, int* outW, int* outH, double* outDuration), {
	const url = UTF8ToString(urlPtr);
	if (!Module.vpxVideoLoad) return 0;
	try {
		const meta = await Module.vpxVideoLoad(url);
		HEAP32[outW >> 2] = meta.width | 0;
		HEAP32[outH >> 2] = meta.height | 0;
		HEAPF64[outDuration >> 3] = +meta.duration || 0;
		return meta.id | 0;
	} catch (e) {
		console.error("WebVideoLoad failed:", e);
		return 0;
	}
});

EM_JS(void, WebVideoDestroy, (int id), {
	if (Module.vpxVideoDestroy) Module.vpxVideoDestroy(id);
});

EM_JS(void, WebVideoPlay, (int id), {
	if (Module.vpxVideoPlay) Module.vpxVideoPlay(id);
});

EM_JS(void, WebVideoPause, (int id), {
	if (Module.vpxVideoPause) Module.vpxVideoPause(id);
});

EM_JS(void, WebVideoStop, (int id), {
	if (Module.vpxVideoStop) Module.vpxVideoStop(id);
});

EM_JS(void, WebVideoSeek, (int id, double timeSec), {
	if (Module.vpxVideoSeek) Module.vpxVideoSeek(id, timeSec);
});

EM_JS(int, WebVideoIsPlaying, (int id), {
	if (!Module.vpxVideoIsPlaying) return 0;
	return Module.vpxVideoIsPlaying(id) | 0;
});

EM_JS(int, WebVideoIsFinished, (int id), {
	if (!Module.vpxVideoIsFinished) return 1;
	return Module.vpxVideoIsFinished(id) | 0;
});

EM_JS(double, WebVideoGetTimePlayed, (int id), {
	if (!Module.vpxVideoGetTimePlayed) return 0;
	return +Module.vpxVideoGetTimePlayed(id);
});

EM_JS(double, WebVideoGetTimeLength, (int id), {
	if (!Module.vpxVideoGetTimeLength) return 0;
	return +Module.vpxVideoGetTimeLength(id);
});

EM_JS(void, WebVideoSetLooping, (int id, int enabled), {
	if (Module.vpxVideoSetLooping) Module.vpxVideoSetLooping(id, enabled);
});

EM_JS(int, WebVideoGetLooping, (int id), {
	if (!Module.vpxVideoGetLooping) return 0;
	return Module.vpxVideoGetLooping(id) | 0;
});

EM_JS(void, WebVideoSetPlaybackRate, (int id, double rate), {
	if (Module.vpxVideoSetPlaybackRate) Module.vpxVideoSetPlaybackRate(id, rate);
});

EM_JS(double, WebVideoGetPlaybackRate, (int id), {
	if (!Module.vpxVideoGetPlaybackRate) return 1.0;
	return +Module.vpxVideoGetPlaybackRate(id);
});

EM_JS(int, WebVideoCopyFrameRGBA, (int id, uint8_t* dstPtr, int maxBytes), {
	if (!Module.vpxVideoCopyFrameRGBA) return 0;
	return Module.vpxVideoCopyFrameRGBA(id, dstPtr, maxBytes, HEAPU8) | 0;
});
#endif

static VIDEO_MAYBE_UNUSED uint16_t ReadLE16(const unsigned char* p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static VIDEO_MAYBE_UNUSED uint32_t ReadLE32(const unsigned char* p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static VIDEO_MAYBE_UNUSED uint64_t ReadLE64(const unsigned char* p) {
	return (uint64_t)p[0] |
		((uint64_t)p[1] << 8) |
		((uint64_t)p[2] << 16) |
		((uint64_t)p[3] << 24) |
		((uint64_t)p[4] << 32) |
		((uint64_t)p[5] << 40) |
		((uint64_t)p[6] << 48) |
		((uint64_t)p[7] << 56);
}

static uint32_t ReadBE32(const unsigned char* p) {
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint64_t ReadBE64(const unsigned char* p) {
	return ((uint64_t)p[0] << 56) |
		((uint64_t)p[1] << 48) |
		((uint64_t)p[2] << 40) |
		((uint64_t)p[3] << 32) |
		((uint64_t)p[4] << 24) |
		((uint64_t)p[5] << 16) |
		((uint64_t)p[6] << 8) |
		(uint64_t)p[7];
}

static VIDEO_MAYBE_UNUSED double ReadBigEndianFloat(const unsigned char* p, size_t sz) {
	if (sz == 4) {
		uint32_t bits = ReadBE32(p);
		float f = 0.0f;
		std::memcpy(&f, &bits, sizeof(float));
		return (double)f;
	}
	if (sz == 8) {
		uint64_t bits = ReadBE64(p);
		double d = 0.0;
		std::memcpy(&d, &bits, sizeof(double));
		return d;
	}
	return 0.0;
}

static VIDEO_MAYBE_UNUSED unsigned char ClampByte(int v) {
	if (v < 0) return 0;
	if (v > 255) return 255;
	return (unsigned char)v;
}

#if HAVE_LIBVPX
// Refuse files bigger than this: the whole file is held in memory.
static const long kMaxVideoFileBytes = 512L * 1024 * 1024;

// Read a whole file through the virtual file system, so a sandboxed host's mounts
// and path rules apply exactly as they do for the `file` module.
static bool ReadFileBytes(const char* path, std::vector<unsigned char>* out, std::string* error) {
	BinaryData* bd = fs::ReadBinary(String(path));
	if (bd == nullptr) {
		if (error) *error = std::string("could not open file: ") + path;
		return false;
	}
	bool ok = bd->length > 0 && bd->length <= kMaxVideoFileBytes;
	if (ok) {
		out->assign(bd->bytes, bd->bytes + bd->length);
	} else if (error) {
		*error = bd->length <= 0 ? std::string("file is empty: ") + path
			: std::string("file too large to load (limit ") + std::to_string(kMaxVideoFileBytes / (1024 * 1024)) + " MB): " + path;
	}
	delete bd;
	return ok;
}

static bool ReadEBMLVint(const std::vector<unsigned char>& data, size_t* pos, int maxLen, bool stripMarker, uint64_t* outValue, int* outLen, bool* outUnknown) {
	if (*pos >= data.size()) return false;
	unsigned char first = data[*pos];
	unsigned char mask = 0x80;
	int len = 1;
	while (len <= maxLen && (first & mask) == 0) {
		mask >>= 1;
		len += 1;
	}
	if (len > maxLen) return false;
	if (*pos + (size_t)len > data.size()) return false;

	uint64_t val = stripMarker ? (uint64_t)(first & (mask - 1)) : (uint64_t)first;
	for (int i = 1; i < len; i++) {
		val = (val << 8) | (uint64_t)data[*pos + (size_t)i];
	}

	bool unknown = false;
	if (stripMarker) {
		uint64_t maxVal = (len == 8) ? UINT64_MAX : ((1ULL << (7 * len)) - 1ULL);
		unknown = (val == maxVal);
	}

	*pos += (size_t)len;
	*outValue = val;
	if (outLen) *outLen = len;
	if (outUnknown) *outUnknown = unknown;
	return true;
}

static bool ParseSimpleBlock(const std::vector<unsigned char>& fileData,
							 size_t payloadStart,
							 size_t payloadSize,
							 uint64_t clusterTimecode,
							 uint64_t timecodeScale,
							 uint64_t wantedTrack,
							 std::vector<EncodedFrame>* outFrames,
							 size_t* outPushedFrames,
							 bool* outUsedLacing) {
	if (outPushedFrames) *outPushedFrames = 0;
	if (outUsedLacing) *outUsedLacing = false;

	size_t p = payloadStart;
	size_t payloadEnd = payloadStart + payloadSize;
	if (payloadEnd > fileData.size()) return false;

	uint64_t trackNum = 0;
	if (!ReadEBMLVint(fileData, &p, 8, true, &trackNum, nullptr, nullptr)) return false;
	if (p + 3 > payloadEnd) return false;

	int16_t relTc = (int16_t)(((int)fileData[p] << 8) | (int)fileData[p + 1]);
	p += 2;
	unsigned char flags = fileData[p++];

	unsigned char lacing = (unsigned char)((flags >> 1) & 0x03);
	if (trackNum != wantedTrack) return true;
	if (p >= payloadEnd) return true;
	if (lacing != 0 && outUsedLacing) *outUsedLacing = true;

	uint64_t tc = clusterTimecode + (int64_t)relTc;
	double pts = ((double)tc * (double)timecodeScale) / 1000000000.0;
	size_t pushed = 0;

	if (lacing == 0) {
		EncodedFrame frame;
		frame.offset = (long)p;
		frame.size = (uint32_t)(payloadEnd - p);
		frame.pts = pts;
		outFrames->push_back(frame);
		if (outPushedFrames) *outPushedFrames = 1;
		return true;
	}

	if (p >= payloadEnd) return false;
	uint8_t laceCount = (uint8_t)(fileData[p++] + 1);
	if (laceCount == 0) return false;

	std::vector<size_t> frameSizes;
	frameSizes.reserve((size_t)laceCount);

	if (lacing == 1) {
		for (uint8_t i = 0; i + 1 < laceCount; i++) {
			size_t sz = 0;
			while (true) {
				if (p >= payloadEnd) return false;
				unsigned char b = fileData[p++];
				sz += (size_t)b;
				if (b != 255) break;
			}
			frameSizes.push_back(sz);
		}
	} else if (lacing == 2) {
		size_t remaining = payloadEnd - p;
		if (remaining < (size_t)laceCount) return false;
		if ((remaining % (size_t)laceCount) != 0) return false;
		size_t each = remaining / (size_t)laceCount;
		for (uint8_t i = 0; i < laceCount; i++) frameSizes.push_back(each);
	} else if (lacing == 3) {
		uint64_t firstSize = 0;
		if (!ReadEBMLVint(fileData, &p, 8, true, &firstSize, nullptr, nullptr)) return false;
		frameSizes.push_back((size_t)firstSize);

		for (uint8_t i = 1; i + 1 < laceCount; i++) {
			if (p >= payloadEnd) return false;
			unsigned char first = fileData[p];
			unsigned char mask = 0x80;
			int len = 1;
			while (len <= 8 && (first & mask) == 0) {
				mask >>= 1;
				len += 1;
			}
			if (len > 8) return false;

			uint64_t vint = 0;
			if (!ReadEBMLVint(fileData, &p, 8, true, &vint, nullptr, nullptr)) return false;
			int64_t bias = ((int64_t)1 << (7 * len - 1)) - 1;
			int64_t delta = (int64_t)vint - bias;
			int64_t nextSize = (int64_t)frameSizes.back() + delta;
			if (nextSize < 0) return false;
			frameSizes.push_back((size_t)nextSize);
		}
	} else {
		return false;
	}

	size_t remaining = payloadEnd - p;
	size_t sumKnown = 0;
	for (size_t i = 0; i < frameSizes.size(); i++) {
		sumKnown += frameSizes[i];
		if (sumKnown > remaining) return false;
	}
	if ((size_t)laceCount < frameSizes.size()) return false;
	if (remaining < sumKnown) return false;
	frameSizes.push_back(remaining - sumKnown);

	size_t cursor = 0;
	for (size_t i = 0; i < frameSizes.size(); i++) {
		size_t sz = frameSizes[i];
		if (cursor + sz > remaining) return false;
		if (sz > 0) {
			EncodedFrame frame;
			frame.offset = (long)(p + cursor);
			frame.size = (uint32_t)sz;
			frame.pts = pts + ((double)i * 1e-6);
			outFrames->push_back(frame);
			pushed += 1;
		}
		cursor += sz;
	}

	if (outPushedFrames) *outPushedFrames = pushed;
	return true;
}

static bool ParseVorbisCodecPrivate(const unsigned char* data, size_t size,
	bool* outIdentificationSeen, bool* outCommentSeen, bool* outSetupSeen,
	int* outChannels, double* outSampleRate,
	std::vector<std::vector<unsigned char>>* outHeaders = nullptr) {
	if (outIdentificationSeen) *outIdentificationSeen = false;
	if (outCommentSeen) *outCommentSeen = false;
	if (outSetupSeen) *outSetupSeen = false;
	if (outChannels) *outChannels = 0;
	if (outSampleRate) *outSampleRate = 0.0;
	if (outHeaders) outHeaders->clear();

	if (!data || size < 1) return false;

	int headerCount = (int)data[0] + 1;
	if (headerCount != 3) return false;

	size_t cursor = 1;
	size_t headerSizes[3] = {0, 0, 0};
	for (int i = 0; i < 2; i++) {
		size_t sz = 0;
		while (cursor < size && data[cursor] == 0xFF) {
			sz += 255;
			cursor += 1;
		}
		if (cursor >= size) return false;
		sz += (size_t)data[cursor++];
		headerSizes[i] = sz;
	}

	if (size < cursor + headerSizes[0] + headerSizes[1]) return false;
	headerSizes[2] = size - cursor - headerSizes[0] - headerSizes[1];

	const unsigned char* h1 = data + cursor;
	const unsigned char* h2 = h1 + headerSizes[0];
	const unsigned char* h3 = h2 + headerSizes[1];

	auto hasVorbisSig = [](const unsigned char* p, size_t n, unsigned char expectedType) -> bool {
		if (!p || n < 7) return false;
		if (p[0] != expectedType) return false;
		return p[1] == 'v' && p[2] == 'o' && p[3] == 'r' && p[4] == 'b' && p[5] == 'i' && p[6] == 's';
	};

	bool idSeen = hasVorbisSig(h1, headerSizes[0], 0x01);
	bool commentSeen = hasVorbisSig(h2, headerSizes[1], 0x03);
	bool setupSeen = hasVorbisSig(h3, headerSizes[2], 0x05);

	if (idSeen && headerSizes[0] >= 16) {
		if (outChannels) *outChannels = (int)h1[11];
		if (outSampleRate) *outSampleRate = (double)ReadLE32(&h1[12]);
	}

	if (outHeaders) {
		outHeaders->resize(3);
		(*outHeaders)[0].assign(h1, h1 + headerSizes[0]);
		(*outHeaders)[1].assign(h2, h2 + headerSizes[1]);
		(*outHeaders)[2].assign(h3, h3 + headerSizes[2]);
	}

	if (outIdentificationSeen) *outIdentificationSeen = idSeen;
	if (outCommentSeen) *outCommentSeen = commentSeen;
	if (outSetupSeen) *outSetupSeen = setupSeen;
	return true;
}

static bool ParseWebMFrames(const std::vector<unsigned char>& fileData, int* outW, int* outH, double* outFps, std::vector<EncodedFrame>* outFrames, double* outDuration,
	bool* outHasAudio, std::string* outAudioCodec, double* outAudioSampleRate, int* outAudioChannels,
	std::vector<EncodedFrame>* outAudioPackets,
	bool* outVorbisHeaderParseAttempted, bool* outVorbisIdentificationSeen,
	bool* outVorbisCommentSeen, bool* outVorbisSetupSeen,
	int* outVorbisParsedChannels, double* outVorbisParsedSampleRate,
	bool* outVorbisHeadersFromCodecPrivate,
	std::vector<std::vector<unsigned char>>* outVorbisCodecPrivateHeaders = nullptr) {
	size_t pos = 0;
	size_t segmentStart = 0;
	size_t segmentEnd = 0;
	bool foundSegment = false;

	while (pos < fileData.size()) {
		size_t elemPos = pos;
		uint64_t elemId = 0;
		uint64_t elemSize = 0;
		bool sizeUnknown = false;
		if (!ReadEBMLVint(fileData, &pos, 4, false, &elemId, nullptr, nullptr)) return false;
		if (!ReadEBMLVint(fileData, &pos, 8, true, &elemSize, nullptr, &sizeUnknown)) return false;

		size_t payloadStart = pos;
		size_t payloadEnd = sizeUnknown ? fileData.size() : (payloadStart + (size_t)elemSize);
		if (payloadEnd > fileData.size()) return false;

		if (elemId == 0x18538067ULL) {
			segmentStart = payloadStart;
			segmentEnd = payloadEnd;
			foundSegment = true;
			break;
		}
		if (payloadEnd <= elemPos) break;
		pos = payloadEnd;
	}

	if (!foundSegment) return false;

	uint64_t timecodeScale = 1000000ULL;
	uint64_t videoTrack = 0;
	uint64_t audioTrack = 0;
	double fps = 0.0;
	int width = 0;
	int height = 0;
	bool hasAudio = false;
	std::string audioCodec;
	double audioSampleRate = 0.0;
	int audioChannels = 0;
	bool vorbisHeaderParseAttempted = false;
	bool vorbisIdentificationSeen = false;
	bool vorbisCommentSeen = false;
	bool vorbisSetupSeen = false;
	bool vorbisHeadersFromCodecPrivate = false;
	int vorbisParsedChannels = 0;
	double vorbisParsedSampleRate = 0.0;
	double audioFirstPacketTime = 0.0;
	double audioLastPacketTime = 0.0;
	bool sawAudioPacket = false;
	std::vector<EncodedFrame> frames;
	std::vector<EncodedFrame> audioPackets;
	size_t malformedBlocks = 0;
	size_t lacedBlocks = 0;
	size_t lacedFrames = 0;

	pos = segmentStart;
	while (pos < segmentEnd) {
		size_t elemStart = pos;
		uint64_t elemId = 0;
		uint64_t elemSize = 0;
		bool sizeUnknown = false;
		if (!ReadEBMLVint(fileData, &pos, 4, false, &elemId, nullptr, nullptr)) break;
		if (!ReadEBMLVint(fileData, &pos, 8, true, &elemSize, nullptr, &sizeUnknown)) break;
		size_t payloadStart = pos;
		size_t payloadEnd = sizeUnknown ? segmentEnd : (payloadStart + (size_t)elemSize);
		if (payloadEnd > segmentEnd) break;

		if (elemId == 0x1549A966ULL) {
			size_t p = payloadStart;
			while (p < payloadEnd) {
				uint64_t id = 0;
				uint64_t sz = 0;
				bool unk = false;
				if (!ReadEBMLVint(fileData, &p, 4, false, &id, nullptr, nullptr)) break;
				if (!ReadEBMLVint(fileData, &p, 8, true, &sz, nullptr, &unk)) break;
				size_t s = p;
				size_t e = unk ? payloadEnd : (s + (size_t)sz);
				if (e > payloadEnd) break;
				if (id == 0x2AD7B1ULL && sz > 0 && sz <= 8) {
					uint64_t v = 0;
					for (size_t i = 0; i < (size_t)sz; i++) v = (v << 8) | fileData[s + i];
					timecodeScale = v;
				}
				p = e;
			}
		} else if (elemId == 0x1654AE6BULL) {
			size_t p = payloadStart;
			while (p < payloadEnd) {
				uint64_t id = 0;
				uint64_t sz = 0;
				bool unk = false;
				if (!ReadEBMLVint(fileData, &p, 4, false, &id, nullptr, nullptr)) break;
				if (!ReadEBMLVint(fileData, &p, 8, true, &sz, nullptr, &unk)) break;
				size_t s = p;
				size_t e = unk ? payloadEnd : (s + (size_t)sz);
				if (e > payloadEnd) break;

				if (id == 0xAEULL) {
					uint64_t tn = 0;
					uint64_t tt = 0;
					std::string codec;
					uint64_t defaultDuration = 0;
					int tw = 0;
					int th = 0;
					double asr = 0.0;
					int ach = 0;
					std::vector<unsigned char> codecPrivate;

					size_t tp = s;
					while (tp < e) {
						uint64_t tid = 0;
						uint64_t tsz = 0;
						bool tunk = false;
						if (!ReadEBMLVint(fileData, &tp, 4, false, &tid, nullptr, nullptr)) break;
						if (!ReadEBMLVint(fileData, &tp, 8, true, &tsz, nullptr, &tunk)) break;
						size_t ts = tp;
						size_t te = tunk ? e : (ts + (size_t)tsz);
						if (te > e) break;

						if (tid == 0xD7ULL && tsz > 0 && tsz <= 8) {
							for (size_t i = 0; i < (size_t)tsz; i++) tn = (tn << 8) | fileData[ts + i];
						} else if (tid == 0x83ULL && tsz > 0 && tsz <= 8) {
							for (size_t i = 0; i < (size_t)tsz; i++) tt = (tt << 8) | fileData[ts + i];
						} else if (tid == 0x86ULL) {
							codec.assign((const char*)&fileData[ts], (size_t)tsz);
						} else if (tid == 0x63A2ULL) {
							codecPrivate.assign(fileData.begin() + (ptrdiff_t)ts, fileData.begin() + (ptrdiff_t)te);
						} else if (tid == 0x23E383ULL && tsz > 0 && tsz <= 8) {
							for (size_t i = 0; i < (size_t)tsz; i++) defaultDuration = (defaultDuration << 8) | fileData[ts + i];
						} else if (tid == 0xE0ULL) {
							size_t vp = ts;
							while (vp < te) {
								uint64_t vid = 0;
								uint64_t vsz = 0;
								bool vunk = false;
								if (!ReadEBMLVint(fileData, &vp, 4, false, &vid, nullptr, nullptr)) break;
								if (!ReadEBMLVint(fileData, &vp, 8, true, &vsz, nullptr, &vunk)) break;
								size_t vps = vp;
								size_t vpe = vunk ? te : (vps + (size_t)vsz);
								if (vpe > te) break;
								if (vid == 0xB0ULL && vsz > 0 && vsz <= 8) {
									uint64_t v = 0;
									for (size_t i = 0; i < (size_t)vsz; i++) v = (v << 8) | fileData[vps + i];
									tw = (int)v;
								} else if (vid == 0xBAULL && vsz > 0 && vsz <= 8) {
									uint64_t v = 0;
									for (size_t i = 0; i < (size_t)vsz; i++) v = (v << 8) | fileData[vps + i];
									th = (int)v;
								}
								vp = vpe;
							}
						} else if (tid == 0xE1ULL) {
							size_t ap = ts;
							while (ap < te) {
								uint64_t aid = 0;
								uint64_t asz = 0;
								bool aunk = false;
								if (!ReadEBMLVint(fileData, &ap, 4, false, &aid, nullptr, nullptr)) break;
								if (!ReadEBMLVint(fileData, &ap, 8, true, &asz, nullptr, &aunk)) break;
								size_t aps = ap;
								size_t ape = aunk ? te : (aps + (size_t)asz);
								if (ape > te) break;
								if (aid == 0xB5ULL && (asz == 4 || asz == 8)) {
									asr = ReadBigEndianFloat(&fileData[aps], (size_t)asz);
								} else if (aid == 0x9FULL && asz > 0 && asz <= 8) {
									uint64_t v = 0;
									for (size_t i = 0; i < (size_t)asz; i++) v = (v << 8) | fileData[aps + i];
									ach = (int)v;
								}
								ap = ape;
							}
						}

						tp = te;
					}

					if (tt == 1 && codec == "V_VP8") {
						videoTrack = tn;
						if (tw > 0) width = tw;
						if (th > 0) height = th;
						if (defaultDuration > 0) fps = 1000000000.0 / (double)defaultDuration;
					} else if (tt == 2) {
						hasAudio = true;
						if (audioTrack == 0) audioTrack = tn;
						if (!codec.empty()) audioCodec = codec;
						if (codec == "A_VORBIS" && !codecPrivate.empty()) {
							bool idSeen = false;
							bool commentSeen = false;
							bool setupSeen = false;
							int parsedChannels = 0;
							double parsedSampleRate = 0.0;
							vorbisHeaderParseAttempted = true;
							if (ParseVorbisCodecPrivate(codecPrivate.data(), codecPrivate.size(),
								&idSeen, &commentSeen, &setupSeen, &parsedChannels, &parsedSampleRate,
								outVorbisCodecPrivateHeaders)) {
								vorbisIdentificationSeen = vorbisIdentificationSeen || idSeen;
								vorbisCommentSeen = vorbisCommentSeen || commentSeen;
								vorbisSetupSeen = vorbisSetupSeen || setupSeen;
								if (parsedChannels > 0) vorbisParsedChannels = parsedChannels;
								if (parsedSampleRate > 0.0) vorbisParsedSampleRate = parsedSampleRate;
								if (ach <= 0 && parsedChannels > 0) ach = parsedChannels;
								if (asr <= 0.0 && parsedSampleRate > 0.0) asr = parsedSampleRate;
								if (idSeen && commentSeen && setupSeen) vorbisHeadersFromCodecPrivate = true;
							}
						}
						if (asr > 0.0) audioSampleRate = asr;
						if (ach > 0) audioChannels = ach;
					}
				}
				p = e;
			}
		} else if (elemId == 0x1F43B675ULL) {
			if (videoTrack == 0) {
				pos = payloadEnd;
				continue;
			}
			uint64_t clusterTimecode = 0;
			size_t p = payloadStart;
			while (p < payloadEnd) {
				uint64_t id = 0;
				uint64_t sz = 0;
				bool unk = false;
				if (!ReadEBMLVint(fileData, &p, 4, false, &id, nullptr, nullptr)) break;
				if (!ReadEBMLVint(fileData, &p, 8, true, &sz, nullptr, &unk)) break;
				size_t s = p;
				size_t e = unk ? payloadEnd : (s + (size_t)sz);
				if (e > payloadEnd) break;

				if (id == 0xE7ULL && sz > 0 && sz <= 8) {
					clusterTimecode = 0;
					for (size_t i = 0; i < (size_t)sz; i++) clusterTimecode = (clusterTimecode << 8) | fileData[s + i];
				} else if (id == 0xA3ULL) {
					size_t pushed = 0;
					bool usedLacing = false;
					if (!ParseSimpleBlock(fileData, s, (size_t)sz, clusterTimecode, timecodeScale, videoTrack, &frames, &pushed, &usedLacing)) {
						malformedBlocks += 1;
					} else if (usedLacing) {
						lacedBlocks += 1;
						lacedFrames += pushed;
					}
					if (audioTrack != 0) {
						size_t apushed = 0;
						bool alacing = false;
						if (ParseSimpleBlock(fileData, s, (size_t)sz, clusterTimecode, timecodeScale, audioTrack, &audioPackets, &apushed, &alacing)) {
							if (apushed > 0) {
								size_t first = audioPackets.size() - apushed;
								if (!sawAudioPacket) {
									audioFirstPacketTime = audioPackets[first].pts;
									sawAudioPacket = true;
								}
								audioLastPacketTime = audioPackets.back().pts;
							}
						}
					}
				} else if (id == 0xA0ULL) {
					size_t bg = s;
					while (bg < e) {
						uint64_t bid = 0;
						uint64_t bsz = 0;
						bool bunk = false;
						if (!ReadEBMLVint(fileData, &bg, 4, false, &bid, nullptr, nullptr)) break;
						if (!ReadEBMLVint(fileData, &bg, 8, true, &bsz, nullptr, &bunk)) break;
						size_t bs = bg;
						size_t be = bunk ? e : (bs + (size_t)bsz);
						if (be > e) break;
						if (bid == 0xA1ULL) {
							size_t pushed = 0;
							bool usedLacing = false;
							if (!ParseSimpleBlock(fileData, bs, (size_t)bsz, clusterTimecode, timecodeScale, videoTrack, &frames, &pushed, &usedLacing)) {
								malformedBlocks += 1;
							} else if (usedLacing) {
								lacedBlocks += 1;
								lacedFrames += pushed;
							}
							if (audioTrack != 0) {
								size_t apushed = 0;
								bool alacing = false;
								if (ParseSimpleBlock(fileData, bs, (size_t)bsz, clusterTimecode, timecodeScale, audioTrack, &audioPackets, &apushed, &alacing)) {
									if (apushed > 0) {
										size_t first = audioPackets.size() - apushed;
										if (!sawAudioPacket) {
											audioFirstPacketTime = audioPackets[first].pts;
											sawAudioPacket = true;
										}
										audioLastPacketTime = audioPackets.back().pts;
									}
								}
							}
						}
						bg = be;
					}
				}

				p = e;
			}
		}

		if (payloadEnd <= elemStart) break;
		pos = payloadEnd;
	}

	if (videoTrack == 0) {
		TraceLog(LOG_WARNING, "VIDEO: WebM parse failed: no VP8 video track found");
		return false;
	}
	if (width <= 0 || height <= 0) {
		TraceLog(LOG_WARNING, "VIDEO: WebM parse failed: invalid video dimensions");
		return false;
	}
	if (frames.empty()) {
		TraceLog(LOG_WARNING, "VIDEO: WebM parse failed: no VP8 frames (malformed blocks: %zu)", malformedBlocks);
		return false;
	}

	std::sort(frames.begin(), frames.end(), [](const EncodedFrame& a, const EncodedFrame& b) {
		return a.pts < b.pts;
	});

	if (fps <= 0.0 && frames.size() >= 2) {
		double span = frames.back().pts - frames.front().pts;
		if (span > 0.0) fps = (double)(frames.size() - 1) / span;
	}
	if (fps <= 0.0) fps = 30.0;
	if (!audioPackets.empty()) {
		std::sort(audioPackets.begin(), audioPackets.end(), [](const EncodedFrame& a, const EncodedFrame& b) {
			return a.pts < b.pts;
		});
	}

	*outW = width;
	*outH = height;
	*outFps = fps;
	*outFrames = frames;
	*outDuration = frames.back().pts + (1.0 / fps);
	if (outHasAudio) *outHasAudio = hasAudio;
	if (outAudioCodec) *outAudioCodec = audioCodec;
	if (outAudioSampleRate) *outAudioSampleRate = audioSampleRate;
	if (outAudioChannels) *outAudioChannels = audioChannels;
	if (outAudioPackets) *outAudioPackets = audioPackets;
	if (outVorbisHeaderParseAttempted) *outVorbisHeaderParseAttempted = vorbisHeaderParseAttempted;
	if (outVorbisIdentificationSeen) *outVorbisIdentificationSeen = vorbisIdentificationSeen;
	if (outVorbisCommentSeen) *outVorbisCommentSeen = vorbisCommentSeen;
	if (outVorbisSetupSeen) *outVorbisSetupSeen = vorbisSetupSeen;
	if (outVorbisParsedChannels) *outVorbisParsedChannels = vorbisParsedChannels;
	if (outVorbisParsedSampleRate) *outVorbisParsedSampleRate = vorbisParsedSampleRate;
	if (outVorbisHeadersFromCodecPrivate) *outVorbisHeadersFromCodecPrivate = vorbisHeadersFromCodecPrivate;
	if (outHasAudio && *outHasAudio && outAudioSampleRate && *outAudioSampleRate <= 0.0 && fps > 0.0) {
		// Keep fields consistent for scripts even when container omits explicit audio sampling metadata.
		*outAudioSampleRate = 0.0;
	}
	if (lacedBlocks > 0) {
		TraceLog(LOG_INFO, "VIDEO: WebM parser accepted %zu laced blocks (%zu decoded frame packets)", lacedBlocks, lacedFrames);
	}
	if (hasAudio && !audioPackets.empty()) {
		TraceLog(LOG_INFO, "VIDEO: WebM parser indexed %zu audio packets (first=%.3f, last=%.3f)", audioPackets.size(), audioFirstPacketTime, audioLastPacketTime);
	}
	return true;
}

static bool BuildIVFFrameIndex(const std::vector<unsigned char>& data, std::vector<EncodedFrame>* outFrames, uint32_t* outWidth, uint32_t* outHeight, double* outFps, double* outDuration) {
	const size_t kHeaderSize = 32;
	if (data.size() < kHeaderSize) return false;
	const unsigned char* header = data.data();

	if (!(header[0] == 'D' && header[1] == 'K' && header[2] == 'I' && header[3] == 'F')) return false;

	uint32_t fourcc = ReadLE32(&header[8]);
	if (fourcc != 0x30385056u) return false;

	uint32_t width = ReadLE16(&header[12]);
	uint32_t height = ReadLE16(&header[14]);
	uint32_t rate = ReadLE32(&header[16]);
	uint32_t scale = ReadLE32(&header[20]);
	if (rate == 0) rate = 30;
	if (scale == 0) scale = 1;
	double fps = (double)rate / (double)scale;
	if (fps <= 0.0) fps = 30.0;

	std::vector<EncodedFrame> frames;
	size_t pos = kHeaderSize;
	while (pos + 12 <= data.size()) {
		const unsigned char* frameHeader = &data[pos];
		uint32_t frameSize = ReadLE32(frameHeader);
		uint64_t ts = ReadLE64(&frameHeader[4]);
		size_t payloadOffset = pos + 12;
		if (frameSize == 0) break;
		if (payloadOffset + (size_t)frameSize > data.size()) break;   // truncated

		EncodedFrame frame;
		frame.offset = (long)payloadOffset;
		frame.size = frameSize;
		frame.pts = ((double)ts * (double)scale) / (double)rate;
		frames.push_back(frame);
		pos = payloadOffset + frameSize;
	}

	if (frames.empty()) return false;
	if (frames.back().pts <= 0.0) {
		for (size_t i = 0; i < frames.size(); i++) frames[i].pts = (double)i / fps;
	}

	*outFrames = frames;
	*outWidth = width;
	*outHeight = height;
	*outFps = fps;
	*outDuration = frames.back().pts + (1.0 / fps);
	return true;
}

static void ConvertI420ToRGBA(const vpx_image_t* img, std::vector<unsigned char>& outRGBA) {
	const int w = img->d_w;
	const int h = img->d_h;
	outRGBA.resize((size_t)w * (size_t)h * 4u);

	const unsigned char* yPlane = img->planes[VPX_PLANE_Y];
	const unsigned char* uPlane = img->planes[VPX_PLANE_U];
	const unsigned char* vPlane = img->planes[VPX_PLANE_V];
	const int yStride = img->stride[VPX_PLANE_Y];
	const int uStride = img->stride[VPX_PLANE_U];
	const int vStride = img->stride[VPX_PLANE_V];

	for (int y = 0; y < h; y++) {
		unsigned char* dstRow = &outRGBA[(size_t)y * (size_t)w * 4u];
		const unsigned char* yRow = yPlane + y * yStride;
		const unsigned char* uRow = uPlane + (y >> 1) * uStride;
		const unsigned char* vRow = vPlane + (y >> 1) * vStride;
		for (int x = 0; x < w; x++) {
			const int Y = (int)yRow[x];
			const int U = (int)uRow[x >> 1] - 128;
			const int V = (int)vRow[x >> 1] - 128;
			const int C = Y - 16;
			const int D = U;
			const int E = V;
			dstRow[x * 4 + 0] = ClampByte((298 * C + 409 * E + 128) >> 8);
			dstRow[x * 4 + 1] = ClampByte((298 * C - 100 * D - 208 * E + 128) >> 8);
			dstRow[x * 4 + 2] = ClampByte((298 * C + 516 * D + 128) >> 8);
			dstRow[x * 4 + 3] = 255;
		}
	}
}

// Copy a packet out of the in-memory file, checking that it lies inside it.
static bool CopyPacket(const VideoPlayerState* state, const EncodedFrame& f, std::vector<unsigned char>* out) {
	if (f.offset < 0 || (size_t)f.offset + (size_t)f.size > state->data.size()) return false;
	out->assign(state->data.begin() + f.offset, state->data.begin() + f.offset + f.size);
	return true;
}

static bool InitDecoder(VideoPlayerState* state) {
	if (state->codecInited) {
		vpx_codec_destroy(&state->codec);
		state->codecInited = false;
	}
	if (vpx_codec_dec_init(&state->codec, vpx_codec_vp8_dx(), nullptr, 0) != VPX_CODEC_OK) return false;
	state->codecInited = true;
	return true;
}

static void ResetAudioDecodeSessionState(VideoPlayerState* state, bool keepSeededHeaders, bool advanceSessionId);
static double GetEffectiveAudioSampleRate(const VideoPlayerState* state);
static bool DecodeDesktopAudioPacketStub(VideoPlayerState* state, uint32_t* outPacketIndex, double* outPacketPts, size_t* outPacketBytes,
	int* outDecodedSamples, int* outDecodedChannels, double* outDecodedSampleRate);
static int AutoRefillDecodedQueue(VideoPlayerState* state, double lowLatencyMs, double targetLatencyMs, int maxPacketsPerTick);

static bool RewindDesktopVideo(VideoPlayerState* state) {
	if (!state || state->data.empty()) return false;
	if (!InitDecoder(state)) return false;
	state->nextFrameIndex = 0;
	state->currentFrame = 0;
	state->timePlayed = 0.0;
	state->finished = false;
	state->playBaseClock = GetTime();
	state->playBaseTime = 0.0;
	state->lastObservedTime = 0.0;
	state->finishedPrev = false;
	state->syncMode = "wall-clock";
	state->audioSyncOffsetPrimed = false;
	state->audioSyncOffsetSec = 0.0;
	state->lastAudioClockSec = 0.0;
	state->lastAudioLedTargetSec = 0.0;
	state->audioStreamMediaBaseSec = 0.0;
	// Reset per-tick sync fields (lifetime counters totalFramesDecoded/Skipped/DropEvents are preserved)
	state->videoAudioSyncSkewMs = 0.0;
	state->lastDecodedFramePts = 0.0;
	state->lastUpdateTargetPts = 0.0;
	state->lastDecodeBudgetUsed = 0;
	state->lastDecodeBudgetExhausted = false;
#if HAVE_LIBVORBIS
	ClearActiveAudioCallbackState(state);
	if (state->audioStreamInited && state->audioStreamPlaying) {
		StopAudioStream(state->audioStream);
		state->audioStreamPlaying = false;
	}
#endif
	if (state->hasAudio && state->audioDecodeScaffoldReady) {
		ResetAudioDecodeSessionState(state, true, false);
	}
	return true;
}

static bool DecodeFrameAtIndex(VideoPlayerState* state, size_t idx, bool presentFrame = true) {
	if (!state || state->data.empty() || !state->codecInited || idx >= state->frames.size()) return false;
	const EncodedFrame& f = state->frames[idx];

	std::vector<unsigned char> packet;
	if (!CopyPacket(state, f, &packet)) {
		state->lastError = "video frame lies outside the file";
		return false;
	}
	vpx_codec_err_t err = vpx_codec_decode(&state->codec, packet.data(), (unsigned int)packet.size(), nullptr, 0);
	if (err != VPX_CODEC_OK) {
		const char* detail = vpx_codec_error_detail(&state->codec);
		TraceLog(LOG_WARNING, "VIDEO: VP8 decode error at frame %zu: %s%s%s",
			idx,
			vpx_codec_err_to_string(err),
			detail ? " - " : "",
			detail ? detail : "");
		state->lastError = std::string("VP8 decode error at frame ") + std::to_string(idx) + ": " + vpx_codec_err_to_string(err) + (detail ? std::string(" - ") + detail : std::string());
		return false;
	}

	vpx_codec_iter_t iter = nullptr;
	const vpx_image_t* img = vpx_codec_get_frame(&state->codec, &iter);
	if (!img) {
		TraceLog(LOG_WARNING, "VIDEO: VP8 decode produced no frame at index %zu", idx);
		state->lastError = std::string("VP8 decode produced no frame at index ") + std::to_string(idx);
		return false;
	}
	state->lastError.clear();

	ConvertI420ToRGBA(img, state->rgba);
	if (presentFrame && state->texture.id != 0 && !state->rgba.empty() && TextureStillOurs(state)) UpdateTexture(state->texture, state->rgba.data());

	state->currentFrame = (uint32_t)(idx + 1);
	state->timePlayed = f.pts;
	state->nextFrameIndex = idx + 1;
	state->totalFramesDecoded++;
	state->lastDecodedFramePts = f.pts;
	// A looping video does not finish here: update() wraps it when the clock passes the end.
	if (!state->looping && state->nextFrameIndex >= state->frames.size()) {
		state->finished = true;
		state->playing = false;
	}
	return true;
}

#if HAVE_LIBVORBIS
static void ClearVorbisDecoder(VideoPlayerState* state) {
	if (!state) return;
	if (state->vorbisBlockInited) {
		vorbis_block_clear(&state->vorbisBlock);
		state->vorbisBlockInited = false;
	}
	if (state->vorbisDecoderInited) {
		vorbis_dsp_clear(&state->vorbisState);
		vorbis_comment_clear(&state->vorbisComment);
		vorbis_info_clear(&state->vorbisInfo);
		state->vorbisDecoderInited = false;
	}
}

static bool InitVorbisDecoderFromHeaders(VideoPlayerState* state) {
	if (!state) return false;
	if (state->vorbisCodecPrivateHeaders.size() != 3) return false;

	ClearVorbisDecoder(state);

	vorbis_info_init(&state->vorbisInfo);
	vorbis_comment_init(&state->vorbisComment);

	for (int i = 0; i < 3; i++) {
		const auto& hdr = state->vorbisCodecPrivateHeaders[i];
		ogg_packet op;
		memset(&op, 0, sizeof(op));
		op.packet    = (unsigned char*)hdr.data();
		op.bytes     = (long)hdr.size();
		op.b_o_s     = (i == 0) ? 1 : 0;
		op.e_o_s     = 0;
		op.granulepos = 0;
		op.packetno  = (ogg_int64_t)i;
		int ret = vorbis_synthesis_headerin(&state->vorbisInfo, &state->vorbisComment, &op);
		if (ret != 0) {
			TraceLog(LOG_WARNING, "InitVorbisDecoderFromHeaders: header packet %d rejected (err=%d)", i, ret);
			vorbis_comment_clear(&state->vorbisComment);
			vorbis_info_clear(&state->vorbisInfo);
			return false;
		}
	}

	if (vorbis_synthesis_init(&state->vorbisState, &state->vorbisInfo) != 0) {
		TraceLog(LOG_WARNING, "InitVorbisDecoderFromHeaders: vorbis_synthesis_init failed");
		vorbis_comment_clear(&state->vorbisComment);
		vorbis_info_clear(&state->vorbisInfo);
		return false;
	}

	vorbis_block_init(&state->vorbisState, &state->vorbisBlock);
	state->vorbisDecoderInited = true;
	state->vorbisBlockInited = true;
	return true;
}

static void FeedAudioStreamFromBuffer(VideoPlayerState* state) {
#if HAVE_LIBVORBIS
	RefreshDecodedPcmFramesAvailable(state);
#else
	(void)state;
#endif
}

static void PrimeAudioPlaybackBuffer(VideoPlayerState* state, double lowLatencyMs = 220.0, double targetLatencyMs = 520.0, int maxPacketsPerTick = 32) {
#if HAVE_LIBVORBIS
	if (!state || !state->audioStreamInited) return;
	if (!state->hasAudio || !state->audioDecodeScaffoldReady) return;
	AutoRefillDecodedQueue(state, lowLatencyMs, targetLatencyMs, maxPacketsPerTick);
	FeedAudioStreamFromBuffer(state);
#else
	(void)state;
	(void)lowLatencyMs;
	(void)targetLatencyMs;
	(void)maxPacketsPerTick;
#endif
}

static void DiscardBufferedAudioFrames(VideoPlayerState* state, uint64_t framesToDiscard) {
#if HAVE_LIBVORBIS
	if (!state || framesToDiscard == 0) return;
	int ch = state->audioChannels > 0 ? state->audioChannels : 1;
	if (ch <= 0) ch = 1;
	std::lock_guard<std::mutex> lock(state->pcmPlaybackMutex);
	size_t availableFloats = (state->pcmPlaybackHead < state->pcmPlaybackBuffer.size())
		? state->pcmPlaybackBuffer.size() - state->pcmPlaybackHead : 0;
	uint64_t availableFrames = (uint64_t)(availableFloats / (size_t)ch);
	if (framesToDiscard > availableFrames) framesToDiscard = availableFrames;
	state->pcmPlaybackHead += (size_t)framesToDiscard * (size_t)ch;
	CompactPcmPlaybackBufferLocked(state);
#else
	(void)state;
	(void)framesToDiscard;
#endif
}

static void SeekAudioDecodeToTime(VideoPlayerState* state, double targetTime) {
	if (!state || !state->hasAudio || !state->audioDecodeScaffoldReady) return;
	if (targetTime < 0.0) targetTime = 0.0;

	ResetAudioDecodeSessionState(state, true, false);
	state->audioStreamMediaBaseSec = targetTime;
	if (targetTime <= 0.0) return;

	double sampleRate = GetEffectiveAudioSampleRate(state);
	if (sampleRate <= 0.0) return;
	uint64_t discardFrames = (uint64_t)llround(targetTime * sampleRate);

	while (state->nextAudioPacketIndex < state->audioPackets.size()
	       && state->audioPackets[state->nextAudioPacketIndex].pts + 0.0005 < targetTime) {
		uint32_t packetIndex = 0;
		double packetPts = 0.0;
		size_t packetBytes = 0;
		int decodedSamples = 0;
		int decodedChannels = 0;
		double decodedSampleRate = 0.0;
		if (!DecodeDesktopAudioPacketStub(state, &packetIndex, &packetPts, &packetBytes,
				&decodedSamples, &decodedChannels, &decodedSampleRate)) {
			break;
		}
		if (decodedSamples > 0 && discardFrames > 0) {
			uint64_t discardNow = (uint64_t)decodedSamples;
			if (discardNow > discardFrames) discardNow = discardFrames;
			DiscardBufferedAudioFrames(state, discardNow);
			discardFrames -= discardNow;
		}
	}
	if (discardFrames > 0) DiscardBufferedAudioFrames(state, discardFrames);
	AutoRefillDecodedQueue(state, 150.0, 400.0, 16);
}

#endif

static void InitializeDesktopAudioDecodeScaffold(VideoPlayerState* state) {
	if (!state) return;
	if (state->audioDecodeSessionId == 0) state->audioDecodeSessionId = 1;
	state->audioDecodeScaffoldReady = false;
	state->audioDecodePath.clear();
	state->vorbisHeaderParseAttempted = false;
	state->vorbisIdentificationSeen = false;
	state->vorbisCommentSeen = false;
	state->vorbisSetupSeen = false;
	state->vorbisHeadersFromCodecPrivate = false;
	state->vorbisHeadersFromPacketStream = false;
	state->vorbisParsedChannels = 0;
	state->vorbisParsedSampleRate = 0.0;
	state->audioPacketsRead = 0;
	state->audioBytesRead = 0;
	state->decodedPcmFramesAvailable = 0;
	state->decodedPcmFramesTotal = 0;
	state->decodedPcmFramesConsumed = 0;
	state->decodedPcmFramesClockEstimated = 0;
	state->decodedPcmFramesClockConsumed = 0;
	state->vorbisPacketsDecoded = 0;
	state->autoRefillTriggerCount = 0;
	state->autoRefillPacketsDecoded = 0;
	state->autoRefillTargetHitCount = 0;
	state->autoRefillLastLatencyMs = 0.0;
	state->autoRefillTargetLatencyMs = 0.0;
	state->autoRefillLatencyGainMsTotal = 0.0;
	state->audioLastReadPacketTime = 0.0;
	state->decodedPcmDrainRemainder = 0.0;
#if HAVE_LIBVPX
	state->nextAudioPacketIndex = 0;
	if (state->webPlayer) return;
	if (!state->hasAudio) return;
	if (state->audioPackets.empty()) return;
	if (state->audioCodecName == "A_VORBIS") {
		state->audioDecodeScaffoldReady = true;
		state->audioDecodePath = "vorbis-packet-reader";
	}
#endif
}

static bool ReadDesktopAudioPacketAtIndex(VideoPlayerState* state, size_t idx, std::vector<unsigned char>* outPacket) {
	if (!state || !outPacket || state->data.empty()) return false;
	if (idx >= state->audioPackets.size()) return false;
	const EncodedFrame& p = state->audioPackets[idx];
	if (!CopyPacket(state, p, outPacket)) {
		state->lastError = "audio packet lies outside the file";
		return false;
	}
	state->lastError.clear();
	return true;
}

static void UpdateVorbisHeaderScaffold(VideoPlayerState* state, const std::vector<unsigned char>& packet) {
	if (!state) return;
	if (state->audioCodecName != "A_VORBIS") return;
	state->vorbisHeaderParseAttempted = true;
	if (packet.size() < 7) return;
	if (!(packet[1] == 'v' && packet[2] == 'o' && packet[3] == 'r' && packet[4] == 'b' && packet[5] == 'i' && packet[6] == 's')) return;
	state->vorbisHeadersFromPacketStream = true;

	uint8_t headerType = packet[0];
	if (headerType == 0x01) {
		state->vorbisIdentificationSeen = true;
		if (packet.size() >= 16) {
			state->vorbisParsedChannels = (int)packet[11];
			state->vorbisParsedSampleRate = (double)ReadLE32(&packet[12]);
		}
	} else if (headerType == 0x03) {
		state->vorbisCommentSeen = true;
	} else if (headerType == 0x05) {
		state->vorbisSetupSeen = true;
	}
}

static bool IsVorbisHeaderPacket(const std::vector<unsigned char>& packet, uint8_t* outHeaderType = nullptr) {
	if (outHeaderType) *outHeaderType = 0;
	if (packet.size() < 7) return false;
	if (!(packet[1] == 'v' && packet[2] == 'o' && packet[3] == 'r' && packet[4] == 'b' && packet[5] == 'i' && packet[6] == 's')) return false;
	if (outHeaderType) *outHeaderType = packet[0];
	return packet[0] == 0x01 || packet[0] == 0x03 || packet[0] == 0x05;
}

static double GetEffectiveAudioSampleRate(const VideoPlayerState* state) {
	if (!state) return 0.0;
	if (state->audioSampleRate > 0.0) return state->audioSampleRate;
	if (state->vorbisParsedSampleRate > 0.0) return state->vorbisParsedSampleRate;
	return 0.0;
}

static double GetAudioStreamClockSec(const VideoPlayerState* state) {
	if (!state) return 0.0;
	double sampleRate = GetEffectiveAudioSampleRate(state);
	if (sampleRate <= 0.0) return state->audioStreamMediaBaseSec;
#if HAVE_LIBVORBIS
	return state->audioStreamMediaBaseSec + ((double)state->pcmFramesFedToStream / sampleRate);
#else
	return state->audioStreamMediaBaseSec;
#endif
}

static VIDEO_MAYBE_UNUSED void PrimeAudioSyncOffset(VideoPlayerState* state) {
	if (!state) return;
	double audioClockSec = GetAudioStreamClockSec(state);
	state->audioSyncOffsetSec = state->timePlayed - audioClockSec;
	state->audioSyncOffsetPrimed = true;
	state->lastAudioClockSec = audioClockSec;
	state->lastAudioLedTargetSec = audioClockSec + state->audioSyncOffsetSec;
}

static int EstimateVorbisPacketDecodedSamples(const VideoPlayerState* state, size_t packetIndex) {
	if (!state) return 0;
	double sampleRate = GetEffectiveAudioSampleRate(state);
	if (sampleRate <= 0.0) return 0;

	double delta = 0.0;
	if (packetIndex + 1 < state->audioPackets.size()) {
		delta = state->audioPackets[packetIndex + 1].pts - state->audioPackets[packetIndex].pts;
	} else if (packetIndex > 0 && packetIndex < state->audioPackets.size()) {
		delta = state->audioPackets[packetIndex].pts - state->audioPackets[packetIndex - 1].pts;
	}

	if (delta <= 0.0) delta = 0.02;
	if (delta < 0.001) delta = 0.001;
	if (delta > 0.2) delta = 0.2;

	int samples = (int)llround(delta * sampleRate);
	if (samples < 1) samples = 1;
	if (samples > 16384) samples = 16384;
	return samples;
}

static bool IsAudioDecodeReady(const VideoPlayerState* state) {
	if (!state) return false;
	if (!state->audioDecodeScaffoldReady) return false;
	if (state->audioCodecName == "A_VORBIS") {
		return state->vorbisIdentificationSeen && state->vorbisCommentSeen && state->vorbisSetupSeen;
	}
	return false;
}

static void ResetAudioDecodeSessionState(VideoPlayerState* state, bool keepSeededHeaders, bool advanceSessionId = true) {
	if (!state) return;
	if (advanceSessionId) {
		state->audioDecodeSessionId += 1;
	}
	state->audioPacketsRead = 0;
	state->audioBytesRead = 0;
	state->decodedPcmFramesAvailable = 0;
	state->decodedPcmFramesTotal = 0;
	state->decodedPcmFramesConsumed = 0;
	state->decodedPcmFramesClockEstimated = 0;
	state->decodedPcmFramesClockConsumed = 0;
	state->vorbisPacketsDecoded = 0;
	state->autoRefillTriggerCount = 0;
	state->autoRefillPacketsDecoded = 0;
	state->autoRefillTargetHitCount = 0;
	state->autoRefillLastLatencyMs = 0.0;
	state->autoRefillTargetLatencyMs = 0.0;
	state->autoRefillLatencyGainMsTotal = 0.0;
	state->audioLastReadPacketTime = 0.0;
	state->decodedPcmDrainRemainder = 0.0;
	state->nextAudioPacketIndex = 0;
#if HAVE_LIBVORBIS
	{
		std::lock_guard<std::mutex> lock(state->pcmPlaybackMutex);
		state->pcmPlaybackBuffer.clear();
		state->pcmPlaybackHead = 0;
		state->pcmFramesFedToStream = 0;
	}
	state->audioStreamMediaBaseSec = 0.0;
#endif
	state->vorbisHeaderParseAttempted = false;
	state->vorbisIdentificationSeen = false;
	state->vorbisCommentSeen = false;
	state->vorbisSetupSeen = false;
	state->vorbisHeadersFromCodecPrivate = false;
	state->vorbisHeadersFromPacketStream = false;
	state->vorbisParsedChannels = 0;
	state->vorbisParsedSampleRate = 0.0;

	if (keepSeededHeaders && state->audioCodecName == "A_VORBIS") {
		state->vorbisHeaderParseAttempted = state->vorbisSeedHeaderParseAttempted;
		state->vorbisIdentificationSeen = state->vorbisSeedIdentificationSeen;
		state->vorbisCommentSeen = state->vorbisSeedCommentSeen;
		state->vorbisSetupSeen = state->vorbisSeedSetupSeen;
		state->vorbisHeadersFromCodecPrivate = state->vorbisSeedHeadersFromCodecPrivate;
		if (state->vorbisSeedParsedChannels > 0) state->vorbisParsedChannels = state->vorbisSeedParsedChannels;
		if (state->vorbisSeedParsedSampleRate > 0.0) state->vorbisParsedSampleRate = state->vorbisSeedParsedSampleRate;
	}
#if HAVE_LIBVORBIS
	// Reinitialize the Vorbis decoder so the DSP state is fresh for the new session.
	if (state->vorbisCodecPrivateHeaders.size() == 3) {
		if (InitVorbisDecoderFromHeaders(state)) {
			state->audioDecodePath = "vorbis-libvorbis";
		}
	}
#endif
}

static bool DecodeDesktopAudioPacketStub(VideoPlayerState* state, uint32_t* outPacketIndex, double* outPacketPts, size_t* outPacketBytes,
	int* outDecodedSamples, int* outDecodedChannels, double* outDecodedSampleRate) {
	if (!state) return false;
	if (state->nextAudioPacketIndex >= state->audioPackets.size()) return false;
	std::vector<unsigned char> packet;
	if (!ReadDesktopAudioPacketAtIndex(state, state->nextAudioPacketIndex, &packet)) return false;

	int decodedSamples = 0;
	int decodedChannels = state->audioChannels;
	double decodedSampleRate = state->audioSampleRate;

	if (state->audioCodecName == "A_VORBIS") {
		uint8_t headerType = 0;
		bool isHeaderPacket = IsVorbisHeaderPacket(packet, &headerType);
		UpdateVorbisHeaderScaffold(state, packet);
		if (!isHeaderPacket && IsAudioDecodeReady(state)) {
#if HAVE_LIBVORBIS
			if (state->vorbisDecoderInited) {
				ogg_packet op;
				memset(&op, 0, sizeof(op));
				op.packet    = packet.data();
				op.bytes     = (long)packet.size();
				op.b_o_s     = 0;
				op.e_o_s     = 0;
				op.granulepos = -1;
				op.packetno  = (ogg_int64_t)state->nextAudioPacketIndex;
				if (vorbis_synthesis(&state->vorbisBlock, &op) == 0) {
					vorbis_synthesis_blockin(&state->vorbisState, &state->vorbisBlock);
					float** pcm = nullptr;
					int samples;
					int ch = state->vorbisInfo.channels > 0 ? state->vorbisInfo.channels : 1;
					while ((samples = vorbis_synthesis_pcmout(&state->vorbisState, &pcm)) > 0) {
						// Interleave float PCM channels into playback buffer
						{
							std::lock_guard<std::mutex> lock(state->pcmPlaybackMutex);
							for (int s = 0; s < samples; s++) {
								for (int c = 0; c < ch; c++) {
									state->pcmPlaybackBuffer.push_back(pcm[c][s]);
								}
							}
							state->decodedPcmFramesAvailable = GetBufferedPcmFramesLocked(state);
						}
						decodedSamples += samples;
						vorbis_synthesis_read(&state->vorbisState, samples);
					}
				}
			} else {
				decodedSamples = EstimateVorbisPacketDecodedSamples(state, state->nextAudioPacketIndex);
			}
#else
			decodedSamples = EstimateVorbisPacketDecodedSamples(state, state->nextAudioPacketIndex);
#endif
			if (decodedSamples > 0) {
#if HAVE_LIBVORBIS
				// When real PCM is buffered in pcmPlaybackBuffer, decodedPcmFramesAvailable
				// is kept accurate by FeedAudioStreamFromBuffer; only bump the estimate-path total here.
				if (!state->vorbisDecoderInited) {
					state->decodedPcmFramesAvailable += (uint64_t)decodedSamples;
				}
#else
				state->decodedPcmFramesAvailable += (uint64_t)decodedSamples;
#endif
				state->decodedPcmFramesTotal += (uint64_t)decodedSamples;
				state->vorbisPacketsDecoded += 1;
			}
		}
		if (decodedChannels <= 0 && state->vorbisParsedChannels > 0) decodedChannels = state->vorbisParsedChannels;
		if (decodedSampleRate <= 0.0 && state->vorbisParsedSampleRate > 0.0) decodedSampleRate = state->vorbisParsedSampleRate;
	}

	if (outPacketIndex) *outPacketIndex = (uint32_t)state->nextAudioPacketIndex;
	if (outPacketPts) *outPacketPts = state->audioPackets[state->nextAudioPacketIndex].pts;
	if (outPacketBytes) *outPacketBytes = packet.size();
	if (outDecodedSamples) *outDecodedSamples = decodedSamples;
	if (outDecodedChannels) *outDecodedChannels = decodedChannels;
	if (outDecodedSampleRate) *outDecodedSampleRate = decodedSampleRate;
	state->audioPacketsRead += 1;
	state->audioBytesRead += (uint64_t)packet.size();
	state->audioLastReadPacketTime = state->audioPackets[state->nextAudioPacketIndex].pts;
	state->nextAudioPacketIndex += 1;
	return true;
}

static uint64_t ConsumeDecodedPcmFrames(VideoPlayerState* state, uint64_t wantedFrames) {
	if (!state) return 0;
	if (wantedFrames == 0 || wantedFrames > state->decodedPcmFramesAvailable) wantedFrames = state->decodedPcmFramesAvailable;
	state->decodedPcmFramesAvailable -= wantedFrames;
	state->decodedPcmFramesConsumed += wantedFrames;
	return wantedFrames;
}

static uint64_t ConsumeDecodedPcmFramesByMediaDelta(VideoPlayerState* state, double mediaDeltaSeconds) {
	if (!state) return 0;
	if (mediaDeltaSeconds <= 0.0) return 0;

	double sampleRate = GetEffectiveAudioSampleRate(state);
	if (sampleRate <= 0.0) return 0;

	double exactFrames = mediaDeltaSeconds * sampleRate + state->decodedPcmDrainRemainder;
	if (exactFrames < 1.0) {
		state->decodedPcmDrainRemainder = exactFrames;
		return 0;
	}

	uint64_t wanted = (uint64_t)floor(exactFrames);
	state->decodedPcmDrainRemainder = exactFrames - (double)wanted;
	state->decodedPcmFramesClockEstimated += wanted;
	uint64_t consumed = ConsumeDecodedPcmFrames(state, wanted);
	state->decodedPcmFramesClockConsumed += consumed;
	return consumed;
}

static double GetDecodedQueueLatencyMs(const VideoPlayerState* state) {
	if (!state) return 0.0;
	double sampleRate = GetEffectiveAudioSampleRate(state);
	if (sampleRate <= 0.0) return 0.0;
	return ((double)state->decodedPcmFramesAvailable / sampleRate) * 1000.0;
}

static double ComputeAdaptiveAudioSyncClampWindowMs(const VideoPlayerState* state) {
	if (!state) return 120.0;
	double latencyMs = GetDecodedQueueLatencyMs(state);
	double targetMs = state->autoRefillTargetLatencyMs;
	if (targetMs <= 0.0) targetMs = 140.0;
	double marginMs = latencyMs - targetMs;
	double hitRate = 0.7;
	if (state->autoRefillTriggerCount > 0) {
		hitRate = (double)state->autoRefillTargetHitCount / (double)state->autoRefillTriggerCount;
		if (hitRate < 0.0) hitRate = 0.0;
		if (hitRate > 1.0) hitRate = 1.0;
	}

	double clampMs = targetMs * 0.50;
	if (clampMs < 30.0) clampMs = 30.0;
	if (clampMs > 180.0) clampMs = 180.0;

	if (marginMs < 0.0) {
		clampMs += (-marginMs) * 0.50;
	} else {
		clampMs -= marginMs * 0.10;
	}

	if (hitRate < 0.5) {
		clampMs += 25.0;
	} else if (hitRate > 0.9) {
		clampMs -= 10.0;
	}

	double skewMs = fabs(state->videoAudioSyncSkewMs);
	if (skewMs > 100.0) clampMs += (skewMs - 100.0) * 0.25;

	if (clampMs < 20.0) clampMs = 20.0;
	if (clampMs > 250.0) clampMs = 250.0;
	return clampMs;
}

static VIDEO_MAYBE_UNUSED double ResolveAudioSyncClampWindowMs(VideoPlayerState* state) {
	if (!state) return 120.0;
	if (!state->audioSyncClampAdaptive) {
		state->audioSyncClampRawWindowMs = state->audioSyncClampManualWindowMs;
		state->audioSyncClampAutoWindowMs = state->audioSyncClampManualWindowMs;
		state->audioSyncClampWindowMs = state->audioSyncClampManualWindowMs;
		return state->audioSyncClampManualWindowMs;
	}

	double rawMs = ComputeAdaptiveAudioSyncClampWindowMs(state);
	state->audioSyncClampRawWindowMs = rawMs;

	double prevMs = state->audioSyncClampAutoWindowMs;
	if (prevMs <= 0.0) prevMs = rawMs;

	double alpha = state->audioSyncClampSmoothingAlpha;
	if (alpha < 0.01) alpha = 0.01;
	if (alpha > 1.00) alpha = 1.00;

	double smoothedMs = prevMs + alpha * (rawMs - prevMs);
	double deltaMs = smoothedMs - prevMs;
	double maxStepMs = state->audioSyncClampMaxStepMs;
	if (maxStepMs < 1.0) maxStepMs = 1.0;
	if (maxStepMs > 100.0) maxStepMs = 100.0;
	if (deltaMs > maxStepMs) deltaMs = maxStepMs;
	if (deltaMs < -maxStepMs) deltaMs = -maxStepMs;

	double effectiveMs = prevMs + deltaMs;
	if (effectiveMs < 20.0) effectiveMs = 20.0;
	if (effectiveMs > 250.0) effectiveMs = 250.0;

	state->audioSyncClampAutoWindowMs = effectiveMs;
	state->audioSyncClampWindowMs = effectiveMs;
	return effectiveMs;
}

static int AutoRefillDecodedQueue(VideoPlayerState* state, double lowLatencyMs, double targetLatencyMs, int maxPacketsPerTick) {
	if (!state) return 0;
	if (maxPacketsPerTick <= 0) return 0;
	if (!state->audioDecodeScaffoldReady) return 0;
	if (state->nextAudioPacketIndex >= state->audioPackets.size()) return 0;

	double latencyMs = GetDecodedQueueLatencyMs(state);
	state->autoRefillLastLatencyMs = latencyMs;
	state->autoRefillTargetLatencyMs = targetLatencyMs;
	if (latencyMs >= lowLatencyMs) return 0;
	state->autoRefillTriggerCount += 1;

	int consumedPackets = 0;
	while (consumedPackets < maxPacketsPerTick && state->nextAudioPacketIndex < state->audioPackets.size()) {
		uint32_t packetIndex = 0;
		double packetPts = 0.0;
		size_t packetBytes = 0;
		int decodedSamples = 0;
		int decodedChannels = 0;
		double decodedSampleRate = 0.0;
		if (!DecodeDesktopAudioPacketStub(state, &packetIndex, &packetPts, &packetBytes, &decodedSamples, &decodedChannels, &decodedSampleRate)) {
			break;
		}
		consumedPackets += 1;
		if (targetLatencyMs > 0.0 && GetDecodedQueueLatencyMs(state) >= targetLatencyMs) break;
	}
	state->autoRefillPacketsDecoded += (uint32_t)consumedPackets;
	double postRefillLatencyMs = GetDecodedQueueLatencyMs(state);
	state->autoRefillLastLatencyMs = postRefillLatencyMs;
	if (postRefillLatencyMs > latencyMs) {
		state->autoRefillLatencyGainMsTotal += (postRefillLatencyMs - latencyMs);
	}
	bool hitTarget = false;
	if (targetLatencyMs > 0.0) {
		hitTarget = (postRefillLatencyMs >= targetLatencyMs);
	} else {
		hitTarget = (consumedPackets > 0);
	}
	if (hitTarget) state->autoRefillTargetHitCount += 1;

	return consumedPackets;
}

static VideoPlayerState* LoadDesktopVideo(const char* path) {
	std::string readError;
	std::vector<unsigned char> fileData;
	if (!ReadFileBytes(path, &fileData, &readError)) {
		gLastVideoLoadError = readError;
		return nullptr;
	}
	gLastVideoLoadError.clear();

	VideoPlayerState* state = new VideoPlayerState();
	state->path = path;
	state->data.swap(fileData);

	uint32_t ivfW = 0;
	uint32_t ivfH = 0;
	double ivfFps = 0.0;
	double ivfDuration = 0.0;
	std::vector<EncodedFrame> ivfFrames;

	int webmW = 0;
	int webmH = 0;
	double webmFps = 0.0;
	double webmDuration = 0.0;
	bool webmHasAudio = false;
	std::string webmAudioCodec;
	double webmAudioSampleRate = 0.0;
	int webmAudioChannels = 0;
	bool webmVorbisHeaderParseAttempted = false;
	bool webmVorbisIdentificationSeen = false;
	bool webmVorbisCommentSeen = false;
	bool webmVorbisSetupSeen = false;
	int webmVorbisParsedChannels = 0;
	double webmVorbisParsedSampleRate = 0.0;
	bool webmVorbisHeadersFromCodecPrivate = false;
	std::vector<std::vector<unsigned char>> webmVorbisCodecPrivateHeaders;
	std::vector<EncodedFrame> webmFrames;
	std::vector<EncodedFrame> webmAudioPackets;

	bool loaded = BuildIVFFrameIndex(state->data, &ivfFrames, &ivfW, &ivfH, &ivfFps, &ivfDuration);
	if (loaded) {
		state->container = "ivf";
		state->codecName = "vp8";
		state->width = (int)ivfW;
		state->height = (int)ivfH;
		state->frameRate = ivfFps;
		state->timeLength = ivfDuration;
		state->frames = ivfFrames;
	} else if (ParseWebMFrames(state->data, &webmW, &webmH, &webmFps, &webmFrames, &webmDuration,
		&webmHasAudio, &webmAudioCodec, &webmAudioSampleRate, &webmAudioChannels, &webmAudioPackets,
		&webmVorbisHeaderParseAttempted, &webmVorbisIdentificationSeen, &webmVorbisCommentSeen,
		&webmVorbisSetupSeen, &webmVorbisParsedChannels, &webmVorbisParsedSampleRate,
		&webmVorbisHeadersFromCodecPrivate, &webmVorbisCodecPrivateHeaders)) {
		state->container = "webm";
		state->codecName = "vp8";
		state->width = webmW;
		state->height = webmH;
		state->frameRate = webmFps;
		state->timeLength = webmDuration;
		state->frames = webmFrames;
		state->hasAudio = webmHasAudio;
		state->audioCodecName = webmAudioCodec;
		state->audioSampleRate = webmAudioSampleRate;
		state->audioChannels = webmAudioChannels;
		state->audioPackets = webmAudioPackets;
		state->audioPacketCount = (uint32_t)state->audioPackets.size();
		if (!state->audioPackets.empty()) {
			state->audioFirstPacketTime = state->audioPackets.front().pts;
			state->audioLastPacketTime = state->audioPackets.back().pts;
		}
	} else {
		TraceLog(LOG_WARNING, "VIDEO: desktop expects VP8 in IVF or WebM (V_VP8) container");
		gLastVideoLoadError = "desktop expects VP8 in IVF or WebM (V_VP8) container";
		delete state;
		return nullptr;
	}

	Image blank = GenImageColor(state->width, state->height, BLACK);
	state->texture = LoadTextureFromImage(blank);
	UnloadImage(blank);
	if (state->texture.id == 0) {
		gLastVideoLoadError = "failed to create video texture";
		delete state;
		return nullptr;
	}

	if (!InitDecoder(state)) {
		gLastVideoLoadError = "failed to initialize VP8 decoder";
		UnloadTexture(state->texture);
		delete state;
		return nullptr;
	}

	state->frameCount = (uint32_t)state->frames.size();
	state->rgba.resize((size_t)state->width * (size_t)state->height * 4u);
	state->playBaseClock = GetTime();
	state->playBaseTime = 0.0;
	state->lastError.clear();
	InitializeDesktopAudioDecodeScaffold(state);
	if (state->audioCodecName == "A_VORBIS") {
		state->vorbisSeedHeaderParseAttempted = webmVorbisHeaderParseAttempted;
		state->vorbisSeedIdentificationSeen = webmVorbisIdentificationSeen;
		state->vorbisSeedCommentSeen = webmVorbisCommentSeen;
		state->vorbisSeedSetupSeen = webmVorbisSetupSeen;
		state->vorbisSeedHeadersFromCodecPrivate = webmVorbisHeadersFromCodecPrivate;
		state->vorbisSeedParsedChannels = webmVorbisParsedChannels;
		state->vorbisSeedParsedSampleRate = webmVorbisParsedSampleRate;
		state->vorbisCodecPrivateHeaders = webmVorbisCodecPrivateHeaders;
		ResetAudioDecodeSessionState(state, true, false);
		if (state->audioChannels <= 0 && state->vorbisParsedChannels > 0) state->audioChannels = state->vorbisParsedChannels;
		if (state->audioSampleRate <= 0.0 && state->vorbisParsedSampleRate > 0.0) state->audioSampleRate = state->vorbisParsedSampleRate;
	}
#if HAVE_LIBVORBIS
	if (state->vorbisDecoderInited && state->audioChannels > 0 && state->audioSampleRate > 0.0) {
		state->audioStream = LoadAudioStream(
			(unsigned int)state->audioSampleRate, 32u, (unsigned int)state->audioChannels);
		if (IsAudioStreamValid(state->audioStream)) {
			SetAudioStreamCallback(state->audioStream, VideoAudioStreamCallback);
			state->audioStreamInited = true;
		}
	}
#endif
	state->valid = true;
	return state;
}
#endif

// Every live player, so Shutdown can release what script forgot to unload.
static std::vector<VideoPlayerState*> gPlayers;

static void DestroyVideoPlayer(VideoPlayerState* state);

// ---- The script-visible handle ------------------------------------------------
//
// A video is a map with a `_handle` key holding a GC handle (which script cannot
// forge from a number), plus read-only-by-convention fields.  Unloading kills the
// whole box, so every copy of the map dies together.  When the GC collects an
// unreachable video the finalizer only queues the player: tearing down a GL texture
// and an audio stream is not something to do in the middle of a sweep, so Update()
// does it (PluginDrainVideos).
struct VideoBox { VideoPlayerState* state; };

static PluginEventQueue<VideoPlayerState*> gPendingDestroy(0);

static void FreeVideoBox(void* p) {
	VideoBox* box = (VideoBox*)p;
	if (box->state) {
		box->state->box = nullptr;   // the handle is going away; the player will be torn down by Update
		gPendingDestroy.Push(box->state);
	}
	delete box;
}

static VideoBox* VideoBoxOf(Value videoVal) {
	if (videoVal.Type() != ValueType::Map) return nullptr;
	Value h = videoVal.GetDict().Lookup(kHandleKey(), Value::Null);
	if (!h.IsHandle()) return nullptr;
	GCHandle gh = GCManager::GetHandle(h);
	if (gh.Callback != &FreeVideoBox) return nullptr;
	return (VideoBox*)gh.UserData;
}

// The live player behind a video map, or null (not a video, or already unloaded).
static VIDEO_MAYBE_UNUSED VideoPlayerState* PlayerFromValue(Value videoVal) {
	VideoBox* box = VideoBoxOf(videoVal);
	return box ? box->state : nullptr;
}

static void SyncVideoStateValue(Value videoVal, VideoPlayerState* state) {
	if (videoVal.Type() != ValueType::Map) return;
	ValueDict map = videoVal.GetDict();
	map.SetValue(String("currentFrame"), Value((int)state->currentFrame));
	map.SetValue(String("timePlayed"), Value(state->timePlayed));
	map.SetValue(String("isPlaying"), Value(state->playing ? 1 : 0));
	map.SetValue(String("isFinished"), Value(state->finished ? 1 : 0));
}

// Hand a freshly loaded player to script: gives it its texture (registered with
// resourceCounts) and returns the video map.
static VIDEO_MAYBE_UNUSED Value VideoPlayerToValue(VideoPlayerState* state) {
	Value texMap = TextureToValue(state->texture);
	state->textureMap.Set(texMap);
	state->textureHandle.Set(texMap.GetDict().Lookup(kHandleKey(), Value::Null));
	rcTexture++;
	gPlayers.push_back(state);

	ValueDict map;
	state->box = new VideoBox{state};
	map.SetValue(kHandleKey(), Value::NewHandle(state->box, &FreeVideoBox));
	map.SetValue(String("width"), Value(state->width));
	map.SetValue(String("height"), Value(state->height));
	map.SetValue(String("frameCount"), Value((int)state->frameCount));
	map.SetValue(String("frameRate"), Value(state->frameRate));
	map.SetValue(String("timeLength"), Value(state->timeLength));
	map.SetValue(String("texture"), texMap);
	Value videoVal = DynamicMap(map);
	SyncVideoStateValue(videoVal, state);
	return videoVal;
}

static void DestroyVideoPlayer(VideoPlayerState* state) {
	if (!state) return;
	if (state->box) {
		state->box->state = nullptr;   // every copy of the video map is now dead
		state->box = nullptr;
	}
#ifdef PLATFORM_WEB
	if (state->webPlayer && state->webHandle > 0) {
		WebVideoDestroy(state->webHandle);
		state->webHandle = 0;
	}
#endif
#if HAVE_LIBVPX
	if (state->codecInited) {
		vpx_codec_destroy(&state->codec);
		state->codecInited = false;
	}
#endif
#if HAVE_LIBVORBIS
	ClearActiveAudioCallbackState(state);
	ClearVorbisDecoder(state);
	if (state->audioStreamInited) {
		if (state->audioStreamPlaying) StopAudioStream(state->audioStream);
		UnloadAudioStream(state->audioStream);
		state->audioStreamInited = false;
		state->audioStreamPlaying = false;
	}
#endif
	if (state->texture.id != 0) {
		Value handle = state->textureHandle.Get();
		if (handle.IsNull()) {
			UnloadTexture(state->texture);   // never handed to script
		} else {
			// Unload it unless script already did (UnloadTexture), and kill every copy.
			NativeBox<Texture>* box = NativeBoxOf<Texture>(handle);
			if (box && box->live) {
				UnloadTexture(box->obj);
				box->live = false;
				rcTexture--;
			}
		}
		state->texture = Texture2D{0};
	}
	state->textureHandle.Clear();
	state->textureMap.Clear();
	for (size_t i = 0; i < gPlayers.size(); i++) {
		if (gPlayers[i] == state) { gPlayers.erase(gPlayers.begin() + (ptrdiff_t)i); break; }
	}
	delete state;
}

#ifdef PLATFORM_WEB
static VideoPlayerState* LoadWebVideo(const char* path) {
	int width = 0;
	int height = 0;
	double duration = 0.0;
	int handle = WebVideoLoad(path, &width, &height, &duration);
	if (handle <= 0 || width <= 0 || height <= 0) {
		gLastVideoLoadError = "web backend failed to load video metadata";
		return nullptr;
	}
	gLastVideoLoadError.clear();

	VideoPlayerState* state = new VideoPlayerState();
	state->path = path;
	state->container = "webm";
	state->codecName = "browser";
	state->webPlayer = true;
	state->webHandle = handle;
	state->width = width;
	state->height = height;
	state->frameRate = 30.0;
	state->timeLength = duration > 0.0 ? duration : 0.0;
	state->frameCount = (state->timeLength > 0.0) ? (uint32_t)(state->timeLength * state->frameRate) : 0;

	Image blank = GenImageColor(state->width, state->height, BLACK);
	state->texture = LoadTextureFromImage(blank);
	UnloadImage(blank);
	if (state->texture.id == 0) {
		gLastVideoLoadError = "failed to create video texture";
		DestroyVideoPlayer(state);
		return nullptr;
	}

	state->rgba.resize((size_t)state->width * (size_t)state->height * 4u);
	state->lastError.clear();
	state->valid = true;
	return state;
}
#endif

}  // namespace

// Called from the plugin's Update hook: tear down players whose video map the GC collected.
void VideoDrainPending() {
	VideoPlayerState* state = nullptr;
	while (gPendingDestroy.Pop(state)) DestroyVideoPlayer(state);
}

// Called from the plugin's Shutdown hook: release whatever script never unloaded.
void VideoShutdownAll() {
	VideoDrainPending();
	while (!gPlayers.empty()) DestroyVideoPlayer(gPlayers.back());
}

void AddRVideoMethods(ValueDict& videoModule) {
#if !defined(PLATFORM_WEB) && !HAVE_LIBVPX
	// Desktop without libvpx: plugin.cpp reports the plugin unavailable and never
	// registers the module, so there is nothing to add.
	(void)videoModule;
#else
	Intrinsic i;

	// API: Load a video (VP8 in WebM or IVF on desktop, with Vorbis sound; anything the browser plays on web) and return it, or null if it can't be loaded -- see lastError.  Desktop paths go through the file system like the file module's, so a sandboxed program sees only its mounts; the whole file is read into memory (limit 512 MB).  On web the name is a URL relative to the page.  The video owns its texture: show it with texture(video), and don't call UnloadTexture on it
	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path = context.GetVar(String("fileName")).ToString();
#if !defined(PLATFORM_WEB) && !HAVE_LIBVPX
		TraceLog(LOG_WARNING, "VIDEO: libvpx support not enabled in this build");
		(void)path;
		return IntrinsicResult::Null;
#else
		VideoPlayerState* state = nullptr;
#ifdef PLATFORM_WEB
		state = LoadWebVideo(path.c_str());
#else
		state = LoadDesktopVideo(path.c_str());
#endif
		if (!state || !state->valid) return IntrinsicResult::Null;
		return IntrinsicResult(VideoPlayerToValue(state));
#endif
	});
	videoModule.SetValue("load", i.GetFunc());

	// API: Return 1 if this is a loaded video that has not been unloaded, else 0
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		return IntrinsicResult(state != nullptr && state->valid);
	});
	videoModule.SetValue("isValid", i.GetFunc());

	// API: Start (or restart, if it has finished) playing the video.  Nothing advances until you call update every frame
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		if (state->webPlayer && state->finished) {
			WebVideoSeek(state->webHandle, 0.0);
			state->finished = false;
		}
		if (state->webPlayer) WebVideoPlay(state->webHandle);
#else
		if (state->finished && !RewindDesktopVideo(state)) return IntrinsicResult::Null;
#endif
		state->playing = true;
		state->syncMode = "wall-clock";
		state->audioSyncOffsetPrimed = false;
#if HAVE_LIBVORBIS
		if (state->audioStreamInited && !state->audioStreamPlaying) {
			PrimeAudioPlaybackBuffer(state);
			SetActiveAudioCallbackState(state);
			PlayAudioStream(state->audioStream);
			state->audioStreamPlaying = true;
			if (state->audioLedSyncEnabled) PrimeAudioSyncOffset(state);
		}
#endif
		state->playBaseClock = GetTime();
		state->playBaseTime = state->timePlayed;
		state->lastObservedTime = state->timePlayed;
		state->finishedPrev = state->finished;
		SyncVideoStateValue(videoVal, state);
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("play", i.GetFunc());

	// API: Pause playback, keeping the position
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		if (state->webPlayer) WebVideoPause(state->webHandle);
#endif
		state->playing = false;
		state->syncMode = "wall-clock";
#if HAVE_LIBVORBIS
		state->audioSyncOffsetPrimed = false;
#endif
#if HAVE_LIBVORBIS
		if (state->audioStreamPlaying) {
			PauseAudioStream(state->audioStream);
			state->audioStreamPlaying = false;
			ClearActiveAudioCallbackState(state);
		}
#endif
		SyncVideoStateValue(videoVal, state);
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("pause", i.GetFunc());

	// API: Continue a paused video from where it stopped
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;
		if (!state->finished) {
#ifdef PLATFORM_WEB
			if (state->webPlayer) WebVideoPlay(state->webHandle);
#endif
			state->playing = true;
			state->syncMode = "wall-clock";
			state->audioSyncOffsetPrimed = false;
#if HAVE_LIBVORBIS
			if (state->audioStreamInited && !state->audioStreamPlaying) {
				PrimeAudioPlaybackBuffer(state);
				SetActiveAudioCallbackState(state);
				ResumeAudioStream(state->audioStream);
				state->audioStreamPlaying = true;
				if (state->audioLedSyncEnabled) PrimeAudioSyncOffset(state);
			}
#endif
			state->playBaseClock = GetTime();
			state->playBaseTime = state->timePlayed;
			state->lastObservedTime = state->timePlayed;
		}
		state->finishedPrev = state->finished;
		SyncVideoStateValue(videoVal, state);
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("resume", i.GetFunc());

	// API: Stop playback and rewind to the start
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;
		state->playing = false;
		state->syncMode = "wall-clock";
		state->audioSyncOffsetPrimed = false;
#ifdef PLATFORM_WEB
		if (state->webPlayer) {
			WebVideoStop(state->webHandle);
			state->timePlayed = 0.0;
			state->currentFrame = 0;
			state->finished = false;
		}
#else
		if (!RewindDesktopVideo(state)) return IntrinsicResult::Null;
#endif
#if HAVE_LIBVORBIS
		ClearActiveAudioCallbackState(state);
#endif
		state->loopEventPending = false;
		state->finishEventPending = false;
		SyncVideoStateValue(videoVal, state);
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("stop", i.GetFunc());

	// API: Jump to the given position in seconds
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.AddParam("position", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;

		double pos = context.GetVar(String("position")).DoubleValue();
		if (pos < 0.0) pos = 0.0;
		if (state->timeLength > 0.0 && pos > state->timeLength) pos = state->timeLength;

#ifdef PLATFORM_WEB
		if (state->webPlayer) {
			WebVideoSeek(state->webHandle, pos);
			state->timePlayed = pos;
			state->currentFrame = (state->frameRate > 0.0) ? (uint32_t)(pos * state->frameRate) : 0;
			state->finished = false;
		}
#else
		if (!RewindDesktopVideo(state)) return IntrinsicResult::Null;
		for (size_t idx = 0; idx < state->frames.size(); idx++) {
			if (state->frames[idx].pts > pos) break;
			if (!DecodeFrameAtIndex(state, idx)) break;
		}
#if HAVE_LIBVORBIS
		SeekAudioDecodeToTime(state, pos);
#endif
		state->timePlayed = pos;
		state->finished = false;
#endif
		state->audioSyncOffsetPrimed = false;
		state->playBaseClock = GetTime();
		state->playBaseTime = pos;
		state->lastObservedTime = pos;
		state->finishedPrev = state->finished;
		SyncVideoStateValue(videoVal, state);
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("seek", i.GetFunc());

	// API: Advance the video to where it should be now and refresh its texture.  Call this once per frame, before drawing the texture
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;

#ifdef PLATFORM_WEB
		if (state->webPlayer && state->texture.id != 0) {
			if (state->rgba.empty()) state->rgba.resize((size_t)state->width * (size_t)state->height * 4u);
			int copied = WebVideoCopyFrameRGBA(state->webHandle, state->rgba.data(), (int)state->rgba.size());
			if (copied > 0 && TextureStillOurs(state)) UpdateTexture(state->texture, state->rgba.data());
			double prevTime = state->timePlayed;
			state->timePlayed = WebVideoGetTimePlayed(state->webHandle);
			double duration = WebVideoGetTimeLength(state->webHandle);
			if (duration > 0.0) state->timeLength = duration;
			state->playing = WebVideoIsPlaying(state->webHandle) != 0;
			state->finished = WebVideoIsFinished(state->webHandle) != 0;
			if (state->looping && state->timePlayed + 0.05 < prevTime) {
				state->loopEventPending = true;
			}
			if (state->finished && !state->finishedPrev) {
				state->finishEventPending = true;
			}
			state->finishedPrev = state->finished;
			if (state->frameRate > 0.0) state->currentFrame = (uint32_t)(state->timePlayed * state->frameRate);
		}
#else
		if (state->playing && !state->finished) {
			double prevMediaTime = state->timePlayed;
			double wallClockTarget = state->playBaseTime + (GetTime() - state->playBaseClock) * state->playbackRate;
			if (wallClockTarget < 0.0) wallClockTarget = 0.0;
			double targetTime = wallClockTarget;
			state->syncMode = "wall-clock";
			state->lastAudioClockSec = 0.0;
			state->lastAudioLedTargetSec = 0.0;

#if HAVE_LIBVORBIS
			if (state->audioLedSyncEnabled && state->audioStreamInited && state->audioStreamPlaying) {
				double syncSampleRate = GetEffectiveAudioSampleRate(state);
				if (syncSampleRate > 0.0) {
					double audioClockSec = GetAudioStreamClockSec(state);
					if (!state->audioSyncOffsetPrimed) PrimeAudioSyncOffset(state);
					double audioLedTarget = audioClockSec + state->audioSyncOffsetSec;
					double clampWindowMs = ResolveAudioSyncClampWindowMs(state);
					double clampWindowSec = clampWindowMs / 1000.0;
					if (clampWindowSec > 0.0) {
						if (audioLedTarget > wallClockTarget + clampWindowSec) audioLedTarget = wallClockTarget + clampWindowSec;
						if (audioLedTarget < wallClockTarget - clampWindowSec) audioLedTarget = wallClockTarget - clampWindowSec;
					}
					if (audioLedTarget < 0.0) audioLedTarget = 0.0;
					targetTime = audioLedTarget;
					state->syncMode = "audio-stream";
					state->lastAudioClockSec = audioClockSec;
					state->lastAudioLedTargetSec = audioLedTarget;
				}
			}
#endif

			if (state->looping && state->timeLength > 0.0) {
				double wrapped = fmod(targetTime, state->timeLength);
				if (wrapped < 0.0) wrapped += state->timeLength;
				if (wrapped + 0.0005 < state->timePlayed) {
					state->loopEventPending = true;
					if (!RewindDesktopVideo(state)) return IntrinsicResult::Null;
#if HAVE_LIBVORBIS
					if (state->audioStreamInited) {
						PrimeAudioPlaybackBuffer(state);
						SetActiveAudioCallbackState(state);
						PlayAudioStream(state->audioStream);
						state->audioStreamPlaying = true;
						if (state->audioLedSyncEnabled) PrimeAudioSyncOffset(state);
					}
#endif
				}
				targetTime = wrapped;
			}

			double oneFrameInterval = (state->frameRate > 1.0) ? (1.0 / state->frameRate) : (1.0 / 30.0);

			int decoded = 0;
			const int decodeBudget = 8;
			while (state->nextFrameIndex < state->frames.size() && state->frames[state->nextFrameIndex].pts <= targetTime + 0.0005 && decoded < decodeBudget) {
				bool staleFrame = state->frames[state->nextFrameIndex].pts < targetTime - oneFrameInterval;
				if (!DecodeFrameAtIndex(state, state->nextFrameIndex, !staleFrame)) {
					state->finished = true;
					state->playing = false;
					break;
				}
				if (staleFrame) state->totalFramesSkipped++;
				decoded += 1;
			}
			state->lastDecodeBudgetUsed = decoded;
			state->lastDecodeBudgetExhausted = (decoded >= decodeBudget)
				&& (state->nextFrameIndex < state->frames.size())
				&& (state->frames[state->nextFrameIndex].pts <= targetTime + 0.0005);
			if (state->lastDecodeBudgetExhausted) state->totalFrameDropEvents++;
			state->lastUpdateTargetPts = targetTime;
			state->timePlayed = targetTime;
			if (state->timeLength > 0.0 && state->timePlayed > state->timeLength) state->timePlayed = state->timeLength;
			double mediaDelta = state->timePlayed - prevMediaTime;
#if HAVE_LIBVORBIS
			if (state->audioStreamInited) {
				if (state->hasAudio && state->audioDecodeScaffoldReady) {
					AutoRefillDecodedQueue(state, 220.0, 520.0, 24);
				}
				FeedAudioStreamFromBuffer(state);
				state->videoAudioSyncSkewMs = (state->timePlayed - GetAudioStreamClockSec(state)) * 1000.0;
			} else {
#endif
			if (mediaDelta > 0.0 && state->decodedPcmFramesAvailable > 0) {
				ConsumeDecodedPcmFramesByMediaDelta(state, mediaDelta);
			}
			// Compute audio-sync skew: how far ahead the video clock is vs audio-consumed position.
			// Positive = video leads audio, 0 = in sync, negative = video behind audio.
			{
				double syncSampleRate = GetEffectiveAudioSampleRate(state);
				if (syncSampleRate > 0.0 && state->decodedPcmFramesConsumed > 0) {
					double audioPositionSec = (double)state->decodedPcmFramesConsumed / syncSampleRate;
					state->videoAudioSyncSkewMs = (state->timePlayed - audioPositionSec) * 1000.0;
				}
			}
			if (state->hasAudio && state->audioDecodeScaffoldReady) {
				// Keep a small decoded queue buffered to emulate real decode+drain stabilization.
				AutoRefillDecodedQueue(state, 60.0, 140.0, 12);
			}
#if HAVE_LIBVORBIS
			}
#endif
			if (!state->looping && state->nextFrameIndex >= state->frames.size() && state->timePlayed >= state->timeLength - 0.0005) {
				state->finished = true;
				state->playing = false;
			}
			if (state->finished && !state->finishedPrev) {
				state->finishEventPending = true;
			}
			state->finishedPrev = state->finished;
		}
#endif

		SyncVideoStateValue(videoVal, state);
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("update", i.GetFunc());

	// API: Return 1 if the video is playing, 0 if paused, stopped, or finished
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		return IntrinsicResult(state && state->valid && state->playing && !state->finished);
	});
	videoModule.SetValue("isPlaying", i.GetFunc());

	// API: Total length of the video, in seconds
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		if (state->webPlayer) state->timeLength = WebVideoGetTimeLength(state->webHandle);
#endif
		return IntrinsicResult(state->timeLength);
	});
	videoModule.SetValue("timeLength", i.GetFunc());

	// API: Current position of the video, in seconds
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		if (state->webPlayer) state->timePlayed = WebVideoGetTimePlayed(state->webHandle);
#endif
		return IntrinsicResult(state->timePlayed);
	});
	videoModule.SetValue("timePlayed", i.GetFunc());

	// API: The Texture holding the current frame; draw it with any Texture function.  The same texture is returned every time, and it belongs to the video: it is released by unload, not by UnloadTexture
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		if (videoVal.Type() != ValueType::Map) return IntrinsicResult::Null;
		ValueDict map = videoVal.GetDict();
		return IntrinsicResult(map.Lookup(String("texture"), Value::null));
	});
	videoModule.SetValue("texture", i.GetFunc());

	// API: Return a map describing the video: path, container, codec, width, height, frameRate, frameCount, timeLength, playbackRate, looping, hasAudio, audioCodec, audioSampleRate, audioChannels (and a few more)
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;

		ValueDict info;
		info.SetValue(String("path"), Value(String(state->path.c_str())));
		info.SetValue(String("container"), Value(String(state->container.c_str())));
		info.SetValue(String("codec"), Value(String(state->codecName.c_str())));
		info.SetValue(String("width"), Value(state->width));
		info.SetValue(String("height"), Value(state->height));
		info.SetValue(String("frameRate"), Value(state->frameRate));
		info.SetValue(String("frameCount"), Value((int)state->frameCount));
		info.SetValue(String("timeLength"), Value(state->timeLength));
		info.SetValue(String("playbackRate"), Value(state->playbackRate));
		info.SetValue(String("looping"), Value(state->looping ? 1 : 0));
		info.SetValue(String("hasAudio"), Value(state->hasAudio ? 1 : 0));
		info.SetValue(String("audioCodec"), Value(String(state->audioCodecName.c_str())));
		info.SetValue(String("audioSampleRate"), Value(state->audioSampleRate));
		info.SetValue(String("audioChannels"), Value(state->audioChannels));
		info.SetValue(String("audioPacketCount"), Value((int)state->audioPacketCount));
		info.SetValue(String("audioFirstPacketTime"), Value(state->audioFirstPacketTime));
		info.SetValue(String("audioLastPacketTime"), Value(state->audioLastPacketTime));
		info.SetValue(String("isWebBackend"), Value(state->webPlayer ? 1 : 0));
		return IntrinsicResult(DynamicMap(info));
	});
	videoModule.SetValue("info", i.GetFunc());

	// API: Name of the decoder in use: "libvpx" (desktop), "browser" (web), or "none" if this build has no video support.  Pass a video to get the one that video uses (null if it is not a loaded video)
	i = Intrinsic::Create("");
	i.AddParam("video", Value::null);
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		if (videoVal.IsNull()) {
#ifdef PLATFORM_WEB
			return IntrinsicResult(Value(String("browser")));
#elif HAVE_LIBVPX
			return IntrinsicResult(Value(String("libvpx")));
#else
			return IntrinsicResult(Value(String("none")));
#endif
		}
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (!state || !state->valid) return IntrinsicResult::Null;
		return IntrinsicResult(Value(String(state->webPlayer ? "browser" : "libvpx")));
	});
	videoModule.SetValue("backend", i.GetFunc());

	// API: Text of the most recent failure: the last load's, or, given a video, that video's most recent decode error.  Empty if there was none
	i = Intrinsic::Create("");
	i.AddParam("video", Value::null);
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoPlayerState* state = PlayerFromValue(videoVal);
		if (state && state->valid) {
			return IntrinsicResult(Value(String(state->lastError.c_str())));
		}
		return IntrinsicResult(Value(String(gLastVideoLoadError.c_str())));
	});
	videoModule.SetValue("lastError", i.GetFunc());

		// API: Return a map of read-only playback statistics: sync mode, audio/video skew, frames decoded and skipped, frames dropped, audio buffer fill.  For tuning and debugging
		i = Intrinsic::Create("");
		i.AddParam("video");
		i.set_Code(INTRINSIC_LAMBDA {
			VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
			if (!state || !state->valid) return IntrinsicResult::Null;
			ValueDict info;
			info.SetValue(String("syncMode"), Value(String(state->syncMode.c_str())));
			info.SetValue(String("videoAudioSyncSkewMs"), Value(state->videoAudioSyncSkewMs));
			info.SetValue(String("totalFramesDecoded"), Value((int)state->totalFramesDecoded));
			info.SetValue(String("totalFramesSkipped"), Value((int)state->totalFramesSkipped));
			info.SetValue(String("totalFrameDropEvents"), Value((int)state->totalFrameDropEvents));
			info.SetValue(String("lastDecodeBudgetUsed"), Value(state->lastDecodeBudgetUsed));
			info.SetValue(String("lastDecodeBudgetExhausted"), Value(state->lastDecodeBudgetExhausted ? 1 : 0));
			info.SetValue(String("lastDecodedFramePts"), Value(state->lastDecodedFramePts));
			info.SetValue(String("lastUpdateTargetPts"), Value(state->lastUpdateTargetPts));
			info.SetValue(String("audioLedSyncEnabled"), Value(state->audioLedSyncEnabled ? 1 : 0));
			info.SetValue(String("audioSyncClampWindowMs"), Value(state->audioSyncClampWindowMs));
			info.SetValue(String("audioSyncClampAdaptive"), Value(state->audioSyncClampAdaptive ? 1 : 0));
			info.SetValue(String("audioSyncClampManualWindowMs"), Value(state->audioSyncClampManualWindowMs));
			info.SetValue(String("audioSyncClampAutoWindowMs"), Value(state->audioSyncClampAutoWindowMs));
			info.SetValue(String("audioSyncClampRawWindowMs"), Value(state->audioSyncClampRawWindowMs));
			info.SetValue(String("audioSyncClampSmoothingAlpha"), Value(state->audioSyncClampSmoothingAlpha));
			info.SetValue(String("audioSyncClampMaxStepMs"), Value(state->audioSyncClampMaxStepMs));
			info.SetValue(String("audioSyncOffsetSec"), Value(state->audioSyncOffsetSec));
			info.SetValue(String("audioClockSec"), Value(state->lastAudioClockSec));
			info.SetValue(String("audioLedTargetSec"), Value(state->lastAudioLedTargetSec));
			int framesBuffered = 0;
#if HAVE_LIBVPX
			if (!state->webPlayer && state->nextFrameIndex < state->frames.size()) {
				framesBuffered = (int)(state->frames.size() - state->nextFrameIndex);
			}
#endif
			info.SetValue(String("framesBuffered"), Value(framesBuffered));
			info.SetValue(String("audioBufferedFrames"), Value((double)state->decodedPcmFramesAvailable));
#if HAVE_LIBVPX
			info.SetValue(String("audioQueueLatencyMs"), Value(GetDecodedQueueLatencyMs(state)));
#else
			info.SetValue(String("audioQueueLatencyMs"), Value::zero);
#endif
			return IntrinsicResult(DynamicMap(info));
		});
		videoModule.SetValue("diagnostics", i.GetFunc());

	// API: Turn looping on or off (default 1).  A looping video starts over at the end instead of finishing
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.AddParam("enabled", Value(1));
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
		int enabled = context.GetVar(String("enabled")).IntValue();
		state->looping = (enabled != 0);
#ifdef PLATFORM_WEB
		if (state->webPlayer) WebVideoSetLooping(state->webHandle, state->looping ? 1 : 0);
#endif
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("setLooping", i.GetFunc());

	// API: Return 1 if the video is set to loop
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		if (state->webPlayer) state->looping = (WebVideoGetLooping(state->webHandle) != 0);
#endif
		return IntrinsicResult(state->looping ? 1 : 0);
	});
	videoModule.SetValue("looping", i.GetFunc());

	// API: Set the playback speed: 1 is normal, 2 is double speed, 0.5 is half
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.AddParam("rate", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
		double rate = context.GetVar(String("rate")).DoubleValue();
		if (rate < 0.05) rate = 0.05;
		if (rate > 4.0) rate = 4.0;
		state->playBaseTime = state->timePlayed;
		state->playBaseClock = GetTime();
		state->playbackRate = rate;
#ifdef PLATFORM_WEB
		if (state->webPlayer) WebVideoSetPlaybackRate(state->webHandle, rate);
#endif
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("setRate", i.GetFunc());

	// API: Return the playback speed set by setRate
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		if (state->webPlayer) state->playbackRate = WebVideoGetPlaybackRate(state->webHandle);
#endif
		return IntrinsicResult(state->playbackRate);
	});
	videoModule.SetValue("rate", i.GetFunc());

	// API: Return 1 if a looping video has wrapped around since the last time you asked, then clear it (an event, not a state)
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
		int raised = state->loopEventPending ? 1 : 0;
		state->loopEventPending = false;
		return IntrinsicResult(raised);
	});
	videoModule.SetValue("didLoop", i.GetFunc());

	// API: Return 1 if the video has reached its end since the last time you asked, then clear it (an event, not a state)
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		VideoPlayerState* state = PlayerFromValue(context.GetVar(String("video")));
		if (!state || !state->valid) return IntrinsicResult::Null;
		int raised = state->finishEventPending ? 1 : 0;
		state->finishEventPending = false;
		return IntrinsicResult(raised);
	});
	videoModule.SetValue("didFinish", i.GetFunc());

	// API: Release the video, its texture, and its sound.  Every copy of the video map becomes invalid.  Safe to call twice.  Videos you forget to unload are released when garbage collected
	i = Intrinsic::Create("");
	i.AddParam("video");
	i.set_Code(INTRINSIC_LAMBDA {
		Value videoVal = context.GetVar(String("video"));
		VideoBox* box = VideoBoxOf(videoVal);
		if (!box || !box->state) return IntrinsicResult::Null;   // not a video, or already unloaded
		DestroyVideoPlayer(box->state);   // also kills every copy of the map
		ValueDict map = videoVal.GetDict();
		map.SetValue(String("isPlaying"), Value::zero);
		map.SetValue(String("isFinished"), Value(1));
		return IntrinsicResult::Null;
	});
	videoModule.SetValue("unload", i.GetFunc());
#endif  // PLATFORM_WEB || HAVE_LIBVPX
}
