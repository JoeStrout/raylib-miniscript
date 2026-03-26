//
//  RAudio.cpp
//  MSRLWeb
//
//  Raylib Audio module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"

using namespace MiniScript;

void AddRAudioMethods(ValueDict raylibModule) {
	Intrinsic *i;

	// Audio device management

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		InitAudioDevice();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("InitAudioDevice", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		CloseAudioDevice();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("CloseAudioDevice", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsAudioDeviceReady());
	};
	raylibModule.SetValue("IsAudioDeviceReady", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("volume", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		float volume = context->GetVar(String("volume")).FloatValue();
		SetMasterVolume(volume);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMasterVolume", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		float volume = GetMasterVolume();
		return IntrinsicResult(volume);
	};
	raylibModule.SetValue("GetMasterVolume", i->GetFunc());

	// Wave loading

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		Wave wave = LoadWave(path.c_str());
		if (!IsWaveValid(wave)) return IntrinsicResult::Null;
		rcWave++;
		return IntrinsicResult(WaveToValue(wave));
	};
	raylibModule.SetValue("LoadWave", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileType");
	i->AddParam("fileData");
	i->AddParam("dataSize");
	i->code = INTRINSIC_LAMBDA {
		String fileType = context->GetVar(String("fileType")).ToString();
		BinaryData* data = ValueToRawData(context->GetVar(String("fileData")));
		if (!data) return IntrinsicResult::Null;
		int dataSize = context->GetVar(String("dataSize")).IntValue();
		if (dataSize <= 0 || dataSize > data->length) dataSize = data->length;
		Wave wave = LoadWaveFromMemory(fileType.c_str(), data->bytes, dataSize);
		if (!IsWaveValid(wave)) return IntrinsicResult::Null;
		rcWave++;
		return IntrinsicResult(WaveToValue(wave));
	};
	raylibModule.SetValue("LoadWaveFromMemory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("frameCount");
	i->AddParam("sampleRate");
	i->AddParam("sampleSize");
	i->AddParam("channels");
	i->AddParam("samples");
	i->code = INTRINSIC_LAMBDA {
		unsigned int frameCount = (unsigned int)context->GetVar(String("frameCount")).IntValue();
		unsigned int sampleRate = (unsigned int)context->GetVar(String("sampleRate")).IntValue();
		unsigned int sampleSize = (unsigned int)context->GetVar(String("sampleSize")).IntValue();
		unsigned int channels = (unsigned int)context->GetVar(String("channels")).IntValue();
		Value samplesVal = context->GetVar(String("samples"));

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
		if (samplesVal.type == ValueType::Map) {
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
		else if (samplesVal.type == ValueType::List) {
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
	};
	raylibModule.SetValue("CreateWave", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
		return IntrinsicResult(IsWaveValid(wave));
	};
	raylibModule.SetValue("IsWaveValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
		UnloadWave(wave);
		// Also delete the heap-allocated Wave
		ValueDict map = context->GetVar(String("wave")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Wave* wavePtr = (Wave*)ValueToPointer(handleVal);
		if (wavePtr != nullptr) {
			delete wavePtr;
			rcWave--;
		}
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadWave", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
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
	};
	raylibModule.SetValue("LoadWaveSamples", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("samples");
	i->code = INTRINSIC_LAMBDA {
		BinaryData* data = ValueToRawData(context->GetVar(String("samples")));
		if (data == nullptr) return IntrinsicResult::Null;

		// Get the raw buffer and free it using raylib's UnloadWaveSamples
		float* samples = (float*)data->bytes;
		if (samples != nullptr) {
			UnloadWaveSamples(samples);
			// Release ownership so we don't double-free
			data->ReleaseOwnership();
		}

		// Delete the BinaryData wrapper
		delete data;

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadWaveSamples", i->GetFunc());

	// Wave manipulation

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
		Wave copy = WaveCopy(wave);
		rcWave++;
		return IntrinsicResult(WaveToValue(copy));
	};
	raylibModule.SetValue("WaveCopy", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->AddParam("initFrame", Value::zero);
	i->AddParam("finalFrame", Value(100));
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
		int initFrame = context->GetVar(String("initFrame")).IntValue();
		int finalFrame = context->GetVar(String("finalFrame")).IntValue();
		WaveCrop(&wave, initFrame, finalFrame);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("WaveCrop", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->AddParam("sampleRate", Value(44100));
	i->AddParam("sampleSize", Value(16));
	i->AddParam("channels", Value(2));
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
		int sampleRate = context->GetVar(String("sampleRate")).IntValue();
		int sampleSize = context->GetVar(String("sampleSize")).IntValue();
		int channels = context->GetVar(String("channels")).IntValue();
		WaveFormat(&wave, sampleRate, sampleSize, channels);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("WaveFormat", i->GetFunc());

	// Music loading and control

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		Music music = LoadMusicStream(path.c_str());
		if (!IsMusicValid(music)) return IntrinsicResult::Null;
		rcMusic++;
		return IntrinsicResult(MusicToValue(music));
	};
	raylibModule.SetValue("LoadMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileType");
	i->AddParam("data");
	i->AddParam("dataSize");
	i->code = INTRINSIC_LAMBDA {
		String fileType = context->GetVar(String("fileType")).ToString();
		// Note: This would need a byte array type in MiniScript to be fully useful
		// For now, we'll skip implementing this
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("LoadMusicStreamFromMemory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		return IntrinsicResult(IsMusicValid(music));
	};
	raylibModule.SetValue("IsMusicValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		UnloadMusicStream(music);
		// Also delete the heap-allocated Music
		ValueDict map = context->GetVar(String("music")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Music* musicPtr = (Music*)ValueToPointer(handleVal);
		if (musicPtr != nullptr) {
			delete musicPtr;
			rcMusic--;
		}
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		PlayMusicStream(music);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PlayMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		return IntrinsicResult(IsMusicStreamPlaying(music));
	};
	raylibModule.SetValue("IsMusicStreamPlaying", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		UpdateMusicStream(music);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		StopMusicStream(music);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("StopMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		PauseMusicStream(music);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PauseMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		ResumeMusicStream(music);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ResumeMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->AddParam("position", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		float position = context->GetVar(String("position")).FloatValue();
		SeekMusicStream(music, position);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SeekMusicStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->AddParam("volume", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		float volume = context->GetVar(String("volume")).FloatValue();
		SetMusicVolume(music, volume);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMusicVolume", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->AddParam("pitch", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		float pitch = context->GetVar(String("pitch")).FloatValue();
		SetMusicPitch(music, pitch);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMusicPitch", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->AddParam("pan", Value(0.5));
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		float pan = context->GetVar(String("pan")).FloatValue();
		SetMusicPan(music, pan);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMusicPan", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		float length = GetMusicTimeLength(music);
		return IntrinsicResult(Value(length));
	};
	raylibModule.SetValue("GetMusicTimeLength", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("music");
	i->code = INTRINSIC_LAMBDA {
		Music music = ValueToMusic(context->GetVar(String("music")));
		float timePlayed = GetMusicTimePlayed(music);
		return IntrinsicResult(Value(timePlayed));
	};
	raylibModule.SetValue("GetMusicTimePlayed", i->GetFunc());

	// Sound loading and control

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		Sound sound = LoadSound(path.c_str());
		if (!IsSoundValid(sound)) return IntrinsicResult::Null;
		rcSound++;
		return IntrinsicResult(SoundToValue(sound));
	};
	raylibModule.SetValue("LoadSound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("wave");
	i->code = INTRINSIC_LAMBDA {
		Wave wave = ValueToWave(context->GetVar(String("wave")));
		Sound sound = LoadSoundFromWave(wave);
		rcSound++;
		return IntrinsicResult(SoundToValue(sound));
	};
	raylibModule.SetValue("LoadSoundFromWave", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("source");
	i->code = INTRINSIC_LAMBDA {
		Sound source = ValueToSound(context->GetVar(String("source")));
		Sound alias = LoadSoundAlias(source);
		rcSound++;
		return IntrinsicResult(SoundToValue(alias));
	};
	raylibModule.SetValue("LoadSoundAlias", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		return IntrinsicResult(IsSoundValid(sound));
	};
	raylibModule.SetValue("IsSoundValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		UnloadSound(sound);
		// Also delete the heap-allocated Sound
		ValueDict map = context->GetVar(String("sound")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Sound* soundPtr = (Sound*)ValueToPointer(handleVal);
		if (soundPtr != nullptr) {
			delete soundPtr;
			rcSound--;
		}
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadSound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("alias");
	i->code = INTRINSIC_LAMBDA {
		Sound alias = ValueToSound(context->GetVar(String("alias")));
		UnloadSoundAlias(alias);
		// Also delete the heap-allocated Sound
		ValueDict map = context->GetVar(String("alias")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Sound* soundPtr = (Sound*)ValueToPointer(handleVal);
		if (soundPtr != nullptr) {
			delete soundPtr;
			rcSound--;
		}
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadSoundAlias", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		PlaySound(sound);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PlaySound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		StopSound(sound);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("StopSound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		PauseSound(sound);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PauseSound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		ResumeSound(sound);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ResumeSound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		return IntrinsicResult(IsSoundPlaying(sound));
	};
	raylibModule.SetValue("IsSoundPlaying", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->AddParam("data");
	i->AddParam("sampleCount");
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		BinaryData* data = ValueToRawData(context->GetVar(String("data")));
		int sampleCount = context->GetVar(String("sampleCount")).IntValue();

		if (data == nullptr || data->bytes == nullptr) {
			RuntimeException("UpdateSound: RawData required for data parameter").raise();
		}

		if (sampleCount <= 0) {
			RuntimeException("UpdateSound: sampleCount must be > 0").raise();
		}

		// Update the sound buffer with the raw data
		UpdateSound(sound, data->bytes, sampleCount);

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateSound", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->AddParam("volume", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		float volume = context->GetVar(String("volume")).FloatValue();
		SetSoundVolume(sound, volume);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetSoundVolume", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->AddParam("pitch", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		float pitch = context->GetVar(String("pitch")).FloatValue();
		SetSoundPitch(sound, pitch);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetSoundPitch", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sound");
	i->AddParam("pan", Value(0.5));
	i->code = INTRINSIC_LAMBDA {
		Sound sound = ValueToSound(context->GetVar(String("sound")));
		float pan = context->GetVar(String("pan")).FloatValue();
		SetSoundPan(sound, pan);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetSoundPan", i->GetFunc());

	// AudioStream management

	i = Intrinsic::Create("");
	i->AddParam("sampleRate", Value(44100));
	i->AddParam("sampleSize", Value(32));
	i->AddParam("channels", Value(1));
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = LoadAudioStream(context->GetVar(String("sampleRate")).IntValue(), context->GetVar(String("sampleSize")).IntValue(), context->GetVar(String("channels")).IntValue());
		rcAudioStream++;
		return IntrinsicResult(AudioStreamToValue(stream));
	};
	raylibModule.SetValue("LoadAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		return IntrinsicResult(IsAudioStreamValid(stream));
	};
	raylibModule.SetValue("IsAudioStreamValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		UnloadAudioStream(stream);
		// Also delete the heap-allocated AudioStream
		ValueDict map = context->GetVar(String("stream")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		AudioStream* streamPtr = (AudioStream*)ValueToPointer(handleVal);
		if (streamPtr != nullptr) {
			delete streamPtr;
			rcAudioStream--;
		}
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->AddParam("data");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		ValueList data = context->GetVar(String("data")).GetList();

#define PROCESS_DATA(TYPE, VALUE) \
		TYPE *buffer = new TYPE[data.Count()]; \
		for (long i=0;i<data.Count();++i) { \
			buffer[i] = static_cast<TYPE>(data.Item(i).VALUE()); \
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
	};
	raylibModule.SetValue("UpdateAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		return IntrinsicResult(IsAudioStreamProcessed(stream));
	};
	raylibModule.SetValue("IsAudioStreamProcessed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		PlayAudioStream(stream);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PlayAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		PauseAudioStream(stream);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PauseAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		ResumeAudioStream(stream);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ResumeAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		return IntrinsicResult(IsAudioStreamPlaying(stream));
	};
	raylibModule.SetValue("IsAudioStreamPlaying", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		StopAudioStream(stream);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("StopAudioStream", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->AddParam("volume", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		float volume = context->GetVar(String("volume")).FloatValue();
		SetAudioStreamVolume(stream, volume);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetAudioStreamVolume", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->AddParam("pitch", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		float pitch = context->GetVar(String("pitch")).FloatValue();
		SetAudioStreamPitch(stream, pitch);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetAudioStreamPitch", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("stream");
	i->AddParam("pan", Value(0.5));
	i->code = INTRINSIC_LAMBDA {
		AudioStream stream = ValueToAudioStream(context->GetVar(String("stream")));
		float pan = context->GetVar(String("pan")).FloatValue();
		SetAudioStreamPan(stream, pan);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetAudioStreamPan", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("size", Value(4096));
	i->code = INTRINSIC_LAMBDA {
		int size = context->GetVar(String("size")).IntValue();
		SetAudioStreamBufferSizeDefault(size);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetAudioStreamBufferSizeDefault", i->GetFunc());
}
