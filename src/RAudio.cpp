//
//  RAudio.cpp
//  MSRLWeb
//
//  Raylib Audio module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "FileSystem.h"
#include "RawData.h"
#include "raylib.h"
#include "miniscript.h"
#include "macros.h"
#include "Profiler.h"

using namespace MiniScript;

void AddRAudioMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Audio device management

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		bool wasOpen = IsAudioDeviceReady();
		InitAudioDevice();
		if (!wasOpen && IsAudioDeviceReady()) BumpAudioGeneration();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("InitAudioDevice", WithNote(i,
		"Initialize audio device"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		CloseAudioDevice();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CloseAudioDevice", WithNote(i,
		"Close the audio device for all contexts"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsAudioDeviceReady());
	});
	raylibModule.SetValue("IsAudioDeviceReady", WithNote(i,
		"Check if device has been initialized successfully"));

	i = Intrinsic::Create("");
	i.AddParam("volume", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		float volume = context.GetArg(0).FloatValue();
		SetMasterVolume(volume);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMasterVolume", WithNote(i,
		"Set master volume (listener)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		float volume = GetMasterVolume();
		return IntrinsicResult(volume);
	});
	raylibModule.SetValue("GetMasterVolume", WithNote(i,
		"Get master volume (listener)"));

	// Wave loading

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Wave wave = LoadWave(path.c_str());
		if (!IsWaveValid(wave)) return IntrinsicResult::Null;
		rcWave++;
		return IntrinsicResult(WaveToValue(wave));
	});
	raylibModule.SetValue("LoadWave", WithNote(i,
		"Load wave data from file"));

	i = Intrinsic::Create("");
	i.AddParam("fileType");
	i.AddParam("fileData");
	i.AddParam("dataSize");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileType = context.GetArg(0).ToString();
		BinaryData* data = ValueToRawData(context.GetArg(1));
		if (!data) return IntrinsicResult::Null;
		int dataSize = context.GetArg(2).IntValue();
		if (dataSize <= 0 || dataSize > data->length) dataSize = data->length;
		Wave wave = LoadWaveFromMemory(fileType.c_str(), data->bytes, dataSize);
		if (!IsWaveValid(wave)) return IntrinsicResult::Null;
		rcWave++;
		return IntrinsicResult(WaveToValue(wave));
	});
	raylibModule.SetValue("LoadWaveFromMemory", WithNote(i,
		"Load wave from memory buffer, fileType refers to extension: i.e. \".wav\" WARNING: File "
		"extension must be provided in lower-case"));

	i = Intrinsic::Create("");
	i.AddParam("frameCount");
	i.AddParam("sampleRate");
	i.AddParam("sampleSize");
	i.AddParam("channels");
	i.AddParam("samples");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int frameCount = (unsigned int)context.GetArg(0).IntValue();
		unsigned int sampleRate = (unsigned int)context.GetArg(1).IntValue();
		unsigned int sampleSize = (unsigned int)context.GetArg(2).IntValue();
		unsigned int channels = (unsigned int)context.GetArg(3).IntValue();
		Value samplesVal = context.GetArg(4);

		// Validate parameters
		if (sampleSize != 8 && sampleSize != 16 && sampleSize != 32) {
			return IntrinsicResult::Null;  // Invalid sample size
		}
		if (channels < 1) return IntrinsicResult::Null;

		// Calculate required buffer size
		unsigned int bytesPerSample = sampleSize / 8;
		unsigned int totalSamples = frameCount * channels;
		unsigned int bufferSize = totalSamples * bytesPerSample;

		void* data = nullptr;

		// Handle RawData case
		if (samplesVal.Type() == ValueType::Map) {
			BinaryData* rawData = ValueToRawData(samplesVal);
			if (rawData != nullptr && rawData->length == bufferSize) {
				// Copy the data from RawData
				data = MemAlloc(bufferSize);
				memcpy(data, rawData->bytes, bufferSize);
			} else {
				return IntrinsicResult::Null;  // Size mismatch or invalid RawData
			}
		}
		// Handle list case
		else if (samplesVal.Type() == ValueType::List) {
			ValueList samplesList = samplesVal.GetList();
			if (samplesList.Count() != (int)totalSamples) {
				return IntrinsicResult::Null;  // Wrong number of samples
			}

			data = MemAlloc(bufferSize);

			if (sampleSize == 8) {
				unsigned char* bytes = (unsigned char*)data;
				for (int i = 0; i < (int)totalSamples; i++) {
					bytes[i] = (unsigned char)samplesList[i].IntValue();
				}
			} else if (sampleSize == 16) {
				short* shorts = (short*)data;
				for (int i = 0; i < (int)totalSamples; i++) {
					shorts[i] = (short)samplesList[i].IntValue();
				}
			} else if (sampleSize == 32) {
				float* floats = (float*)data;
				for (int i = 0; i < (int)totalSamples; i++) {
					floats[i] = samplesList[i].FloatValue();
				}
			}
		} else {
			return IntrinsicResult::Null;  // Invalid samples parameter
		}

		if (data == nullptr) return IntrinsicResult::Null;

		// Create the Wave structure
		Wave wave;
		wave.frameCount = frameCount;
		wave.sampleRate = sampleRate;
		wave.sampleSize = sampleSize;
		wave.channels = channels;
		wave.data = data;

		rcWave++;
		return IntrinsicResult(WaveToValue(wave));
	});
	raylibModule.SetValue("CreateWave", WithNote(i,
		"Create a Wave from samples: frameCount frames at sampleRate Hz, with sampleSize 8, 16 or "
		"32 (float) bits and the given number of channels.  samples is a RawData of exactly that "
		"size, or a list of frameCount * channels values.  Returns null if they don't agree"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.set_Code(INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context.GetArg(0));
		return IntrinsicResult(IsWaveValid(wave));
	});
	raylibModule.SetValue("IsWaveValid", WithNote(i,
		"Check if wave data is valid (data loaded and parameters)"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.set_Code(INTRINSIC_LAMBDA {
		Wave wave;
		if (!TakeResource(context.GetArg(0), &wave)) return IntrinsicResult::Null;
		UnloadWave(wave);
		rcWave--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadWave", WithNote(i,
		"Unload wave data"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.set_Code(INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context.GetArg(0));
		if (!IsWaveValid(wave)) return IntrinsicResult::Null;

		// Load the samples as a float array
		float* samples = LoadWaveSamples(wave);
		if (samples == nullptr) return IntrinsicResult::Null;

		// Calculate the number of samples
		int sampleCount = wave.frameCount * wave.channels;
		int byteSize = sampleCount * sizeof(float);

		// Wrap in a BinaryData object (takes ownership)
		BinaryData* data = new BinaryData((unsigned char*)samples, byteSize, true);

		return IntrinsicResult(RawDataToValue(data));
	});
	raylibModule.SetValue("LoadWaveSamples", WithNote(i,
		"Load samples data from wave as a floats array NOTE 1: Returned sample values are "
		"normalized to range [-1..1] NOTE 2: Sample data allocated should be freed with "
		"UnloadWaveSamples()"));

	i = Intrinsic::Create("");
	i.AddParam("samples");
	i.set_Code(INTRINSIC_LAMBDA {
		BinaryData* data = ValueToRawData(context.GetArg(0));
		if (data == nullptr) return IntrinsicResult::Null;

		// Get the raw buffer and free it using raylib's UnloadWaveSamples
		float* samples = (float*)data->bytes;
		if (samples != nullptr) {
			UnloadWaveSamples(samples);
			// Release ownership so we don't double-free
			data->ReleaseOwnership();
		}

		// Leave the wrapper for the GC.  Now that the buffer lives in a
		// GCHandle, deleting it here would leave that handle dangling and
		// delete it a second time on sweep.  Emptied out, it is harmless: the
		// script's RawData reads as zero-length until it drops the reference.
		data->bytes = nullptr;
		data->length = 0;

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadWaveSamples", WithNote(i,
		"Unload samples data loaded with LoadWaveSamples()"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult::Zero;
		return IntrinsicResult(ExportWave(wave, path.c_str()));
	});
	raylibModule.SetValue("ExportWave", WithNote(i,
		"Export wave data to file"));

	// Wave manipulation

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.set_Code(INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context.GetArg(0));
		Wave copy = WaveCopy(wave);
		rcWave++;
		return IntrinsicResult(WaveToValue(copy));
	});
	raylibModule.SetValue("WaveCopy", WithNote(i,
		"Copy a wave to a new wave"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.AddParam("initFrame", Value::zero);
	i.AddParam("finalFrame", Value(100));
	i.set_Code(INTRINSIC_LAMBDA {
		// Work on the handle's Wave itself: raylib replaces its sample buffer.
		Value waveValue = context.GetArg(0);
		Wave* wavePtr = ResourcePtr<Wave>(waveValue);
		if (wavePtr == nullptr) return IntrinsicResult::Null;
		int initFrame = context.GetArg(1).IntValue();
		int finalFrame = context.GetArg(2).IntValue();
		WaveCrop(wavePtr, initFrame, finalFrame);
		UpdateWaveValue(waveValue);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("WaveCrop", WithNote(i,
		"Crop a wave to defined frames range NOTE: Security check in case of out-of-range"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.AddParam("sampleRate", Value(44100));
	i.AddParam("sampleSize", Value(16));
	i.AddParam("channels", Value(2));
	i.set_Code(INTRINSIC_LAMBDA {
		// Work on the handle's Wave itself: raylib replaces its sample buffer.
		Value waveValue = context.GetArg(0);
		Wave* wavePtr = ResourcePtr<Wave>(waveValue);
		if (wavePtr == nullptr) return IntrinsicResult::Null;
		int sampleRate = context.GetArg(1).IntValue();
		int sampleSize = context.GetArg(2).IntValue();
		int channels = context.GetArg(3).IntValue();
		WaveFormat(wavePtr, sampleRate, sampleSize, channels);
		UpdateWaveValue(waveValue);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("WaveFormat", WithNote(i,
		"Convert wave data to desired format"));

	// Music loading and control

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Music music = LoadMusicStream(path.c_str());
		if (!IsMusicValid(music)) return IntrinsicResult::Null;
		rcMusic++;
		return IntrinsicResult(MusicToValue(music));
	});
	raylibModule.SetValue("LoadMusicStream", WithNote(i,
		"Load music stream from file"));

	i = Intrinsic::Create("");
	i.AddParam("fileType");
	i.AddParam("data");
	i.AddParam("dataSize");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileType = context.GetArg(0).ToString();
		// Note: This would need a byte array type in MiniScript to be fully useful
		// For now, we'll skip implementing this
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("LoadMusicStreamFromMemory", WithNote(i,
		"Load music stream from memory buffer, fileType refers to extension: i.e. \".wav\" "
		"WARNING: File extension must be provided in lower-case"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		return IntrinsicResult(IsMusicValid(music));
	});
	raylibModule.SetValue("IsMusicValid", WithNote(i,
		"Check if music stream is valid (context and buffers initialized)"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music;
		if (!TakeResource(context.GetArg(0), &music)) return IntrinsicResult::Null;
		UnloadMusicStream(music);
		rcMusic--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadMusicStream", WithNote(i,
		"Unload music stream"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		PlayMusicStream(music);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PlayMusicStream", WithNote(i,
		"Start music playing (open stream) from beginning"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		return IntrinsicResult(IsMusicStreamPlaying(music));
	});
	raylibModule.SetValue("IsMusicStreamPlaying", WithNote(i,
		"Check if any music is playing"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		// Decodes the next chunk of audio whenever the stream's buffer runs low.
		profiler::TimedScope timed("raylib.UpdateMusicStream");
		UpdateMusicStream(music);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateMusicStream", WithNote(i,
		"Update (re-fill) music buffers if data already processed"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		StopMusicStream(music);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("StopMusicStream", WithNote(i,
		"Stop music playing (close stream)"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		PauseMusicStream(music);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PauseMusicStream", WithNote(i,
		"Pause music playing"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		ResumeMusicStream(music);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ResumeMusicStream", WithNote(i,
		"Resume music playing"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.AddParam("position", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		float position = context.GetArg(1).FloatValue();
		SeekMusicStream(music, position);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SeekMusicStream", WithNote(i,
		"Seek music to a certain position (in seconds)"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.AddParam("volume", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		float volume = context.GetArg(1).FloatValue();
		SetMusicVolume(music, volume);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMusicVolume", WithNote(i,
		"Set volume for music"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.AddParam("pitch", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		float pitch = context.GetArg(1).FloatValue();
		SetMusicPitch(music, pitch);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMusicPitch", WithNote(i,
		"Set pitch for music"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.AddParam("pan", Value(0.0));
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		float pan = context.GetArg(1).FloatValue();
		SetMusicPan(music, pan);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMusicPan", WithNote(i,
		"Set pan for music"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		float length = GetMusicTimeLength(music);
		return IntrinsicResult(Value(length));
	});
	raylibModule.SetValue("GetMusicTimeLength", WithNote(i,
		"Get music time length (in seconds)"));

	i = Intrinsic::Create("");
	i.AddParam("music");
	i.set_Code(INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context.GetArg(0));
		float timePlayed = GetMusicTimePlayed(music);
		return IntrinsicResult(Value(timePlayed));
	});
	raylibModule.SetValue("GetMusicTimePlayed", WithNote(i,
		"Get current music time played (in seconds)"));

	// Sound loading and control

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Sound sound = LoadSound(path.c_str());
		if (!IsSoundValid(sound)) return IntrinsicResult::Null;
		rcSound++;
		return IntrinsicResult(SoundToValue(sound));
	});
	raylibModule.SetValue("LoadSound", WithNote(i,
		"Load sound from file NOTE: The entire file is loaded to memory to be played (no-streaming)"));

	i = Intrinsic::Create("");
	i.AddParam("wave");
	i.set_Code(INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context.GetArg(0));
		Sound sound = LoadSoundFromWave(wave);
		rcSound++;
		return IntrinsicResult(SoundToValue(sound));
	});
	raylibModule.SetValue("LoadSoundFromWave", WithNote(i,
		"Load sound from wave data NOTE: Wave data must be unallocated manually"));

	i = Intrinsic::Create("");
	i.AddParam("source");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound source = ValueToSound(context.GetArg(0));
		Sound alias = LoadSoundAlias(source);
		rcSound++;
		Value result = SoundToValue(alias);
		AnyResourceBoxOf(result)->soundAlias = true;
		// The alias plays the source's samples, so it keeps the source alive:
		// an auto-unloaded source must not be unloaded out from under it.
		result.GetDict().SetValue(CACHED_KEY("_source"), context.GetArg(0));
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("LoadSoundAlias", WithNote(i,
		"Load sound alias, clone sound from existing sound data, clone does not own wave data "
		"NOTE: Wave data must be unallocated manually and will be shared across all clones"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		return IntrinsicResult(IsSoundValid(sound));
	});
	raylibModule.SetValue("IsSoundValid", WithNote(i,
		"Check if sound is valid (data loaded and buffers initialized)"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound;
		if (!TakeResource(context.GetArg(0), &sound)) return IntrinsicResult::Null;
		UnloadSound(sound);
		rcSound--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadSound", WithNote(i,
		"Unload sound"));

	i = Intrinsic::Create("");
	i.AddParam("alias");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound alias;
		if (!TakeResource(context.GetArg(0), &alias)) return IntrinsicResult::Null;
		UnloadSoundAlias(alias);
		rcSound--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadSoundAlias", WithNote(i,
		"Unload a sound alias (does not deallocate sample data)"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		PlaySound(sound);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PlaySound", WithNote(i,
		"Play a sound"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		StopSound(sound);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("StopSound", WithNote(i,
		"Stop reproducing a sound"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		PauseSound(sound);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PauseSound", WithNote(i,
		"Pause a sound"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		ResumeSound(sound);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ResumeSound", WithNote(i,
		"Resume a paused sound"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		return IntrinsicResult(IsSoundPlaying(sound));
	});
	raylibModule.SetValue("IsSoundPlaying", WithNote(i,
		"Check if sound is playing"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.AddParam("data");
	i.AddParam("frameCount");
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		BinaryData* data = ValueToRawData(context.GetArg(1));
		int frameCount = context.GetArg(2).IntValue();

		if (data == nullptr || data->bytes == nullptr) {
			context.vm.RaiseRuntimeError("UpdateSound: RawData required for data parameter"); return IntrinsicResult::Null;
		}

		if (frameCount <= 0) {
			context.vm.RaiseRuntimeError("UpdateSound: frameCount must be > 0"); return IntrinsicResult::Null;
		}

		// Update the sound buffer with the raw data
		UpdateSound(sound, data->bytes, frameCount);

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateSound", WithNote(i,
		"Update sound buffer with new data PARAMS: [data], format must match "
		"sound.stream.sampleSize, default 32 bit float - stereo PARAMS: [frameCount] must not "
		"exceed sound.frameCount"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.AddParam("volume", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		float volume = context.GetArg(1).FloatValue();
		SetSoundVolume(sound, volume);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetSoundVolume", WithNote(i,
		"Set volume for a sound"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.AddParam("pitch", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		float pitch = context.GetArg(1).FloatValue();
		SetSoundPitch(sound, pitch);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetSoundPitch", WithNote(i,
		"Set pitch for a sound"));

	i = Intrinsic::Create("");
	i.AddParam("sound");
	i.AddParam("pan", Value(0.0));
	i.set_Code(INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context.GetArg(0));
		float pan = context.GetArg(1).FloatValue();
		SetSoundPan(sound, pan);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetSoundPan", WithNote(i,
		"Set pan for a sound"));

	// AudioStream management

	i = Intrinsic::Create("");
	i.AddParam("sampleRate", Value(44100));
	i.AddParam("sampleSize", Value(32));
	i.AddParam("channels", Value(1));
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = LoadAudioStream(context.GetArg(0).IntValue(), context.GetArg(1).IntValue(), context.GetArg(2).IntValue());
		rcAudioStream++;
		return IntrinsicResult(AudioStreamToValue(stream));
	});
	raylibModule.SetValue("LoadAudioStream", WithNote(i,
		"Load audio stream (to stream audio pcm data)"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		return IntrinsicResult(IsAudioStreamValid(stream));
	});
	raylibModule.SetValue("IsAudioStreamValid", WithNote(i,
		"Check if an audio stream is valid (buffers initialized)"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream;
		if (!TakeResource(context.GetArg(0), &stream)) return IntrinsicResult::Null;
		UnloadAudioStream(stream);
		rcAudioStream--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadAudioStream", WithNote(i,
		"Unload audio stream and free memory"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.AddParam("data");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		ValueList data = context.GetArg(1).GetList();

#define PROCESS_DATA(TYPE, VALUE) \
		TYPE *buffer = new TYPE[data.Count()]; \
		for (long i=0;i<data.Count();++i) { \
			buffer[i] = static_cast<TYPE>(data[i].VALUE()); \
		}; \
		UpdateAudioStream(stream, buffer, data.Count());

		if (stream.sampleSize==8) {
			PROCESS_DATA(unsigned char, IntValue)
		} else if (stream.sampleSize==16) {
			PROCESS_DATA(signed short, IntValue)
		} else {
			PROCESS_DATA(float, FloatValue)
		}

#undef PROCESS_DATA

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateAudioStream", WithNote(i,
		"Update audio stream buffers with data NOTE 1: Only updates one buffer of the stream "
		"source: dequeue -> update -> queue NOTE 2: To dequeue a buffer it needs to be processed: "
		"IsAudioStreamProcessed()"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		return IntrinsicResult(IsAudioStreamProcessed(stream));
	});
	raylibModule.SetValue("IsAudioStreamProcessed", WithNote(i,
		"Check if any audio stream buffers requires refill"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		PlayAudioStream(stream);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PlayAudioStream", WithNote(i,
		"Play audio stream"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		PauseAudioStream(stream);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PauseAudioStream", WithNote(i,
		"Play audio stream"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		ResumeAudioStream(stream);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ResumeAudioStream", WithNote(i,
		"Resume audio stream playing"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		return IntrinsicResult(IsAudioStreamPlaying(stream));
	});
	raylibModule.SetValue("IsAudioStreamPlaying", WithNote(i,
		"Check if audio stream is playing"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		StopAudioStream(stream);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("StopAudioStream", WithNote(i,
		"Stop audio stream"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.AddParam("volume", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		float volume = context.GetArg(1).FloatValue();
		SetAudioStreamVolume(stream, volume);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetAudioStreamVolume", WithNote(i,
		"Set volume for audio stream (1.0 is max level)"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.AddParam("pitch", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		float pitch = context.GetArg(1).FloatValue();
		SetAudioStreamPitch(stream, pitch);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetAudioStreamPitch", WithNote(i,
		"Set pitch for audio stream (1.0 is base level)"));

	i = Intrinsic::Create("");
	i.AddParam("stream");
	i.AddParam("pan", Value(0.0));
	i.set_Code(INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context.GetArg(0));
		float pan = context.GetArg(1).FloatValue();
		SetAudioStreamPan(stream, pan);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetAudioStreamPan", WithNote(i,
		"Set pan for audio stream"));

	i = Intrinsic::Create("");
	i.AddParam("size", Value(4096));
	i.set_Code(INTRINSIC_LAMBDA {
		int size = context.GetArg(0).IntValue();
		SetAudioStreamBufferSizeDefault(size);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetAudioStreamBufferSizeDefault", WithNote(i,
		"Default size for new audio streams"));
}
