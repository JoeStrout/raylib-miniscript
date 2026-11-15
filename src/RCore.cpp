//
//  RCore.cpp
//  raylib-miniscript
//
//  Raylib Core module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "FileSystem.h"
#include "RawData.h"
#include "raylib.h"
#include "rcamera.h"
#include "miniscript.h"
#include "macros.h"
#include "Profiler.h"
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef PLATFORM_WEB
#include <emscripten.h>

// Helper: Set window title (web version updates document title and h1)
EM_JS(void, _SetWindowTitle_Web, (const char *title), {
	const _title = UTF8ToString(title);
	document.title = _title;
	document.querySelector("h1").textContent = _title;
});

// Helper: Set window icon (web version updates favicon)
EM_ASYNC_JS(void, _SetWindowIcon_Web, (unsigned char *data, long size), {
	await new Promise((resolve, reject)=>{
		const _data = new Uint8Array(HEAP8.buffer, data, size);
		const blob = new Blob([_data], {type:"image/png"});
		const reader = new FileReader();
		reader.onloadend = () => {
			const dataURL = reader.result;
			let link = document.querySelector('link[rel="icon"]');
			if (link===null) {
				link = document.createElement("link");
				link.setAttribute("rel", "icon");
				document.head.appendChild(link);
			}
			link.href = dataURL;
			resolve();
		};
		reader.onerror = reject;
		reader.readAsDataURL(blob);
	});
});
#endif

using namespace MiniScript;

// Size of every Shader's locs table (set in rlgl.h, not included here).
#ifndef RL_MAX_SHADER_LOCATIONS
#define RL_MAX_SHADER_LOCATIONS 32
#endif

static void SyncCamera3DValue(Value cameraValue, Camera3D camera) {
	if (cameraValue.Type() != ValueType::Map) return;
	ValueDict map = cameraValue.GetDict();

	map.SetValue(CACHED_KEY("position"), Vector3ToValue(camera.position));
	map.SetValue(CACHED_KEY("target"), Vector3ToValue(camera.target));
	map.SetValue(CACHED_KEY("up"), Vector3ToValue(camera.up));
	map.SetValue(CACHED_KEY("fovy"), Value(camera.fovy));
	map.SetValue(CACHED_KEY("projection"), Value(camera.projection));

	map.SetValue(CACHED_KEY("positionX"), Value(camera.position.x));
	map.SetValue(CACHED_KEY("positionY"), Value(camera.position.y));
	map.SetValue(CACHED_KEY("positionZ"), Value(camera.position.z));
	map.SetValue(CACHED_KEY("targetX"), Value(camera.target.x));
	map.SetValue(CACHED_KEY("targetY"), Value(camera.target.y));
	map.SetValue(CACHED_KEY("targetZ"), Value(camera.target.z));
	map.SetValue(CACHED_KEY("upX"), Value(camera.up.x));
	map.SetValue(CACHED_KEY("upY"), Value(camera.up.y));
	map.SetValue(CACHED_KEY("upZ"), Value(camera.up.z));
}

#ifdef PLATFORM_WEB
static void PrintWebNotSupported(const char* functionName) {
	TraceLog(LOG_ERROR, "%s: Web not supported.", functionName);
}
#endif

static void SyncShaderValue(Value shaderValue, Shader shader) {
	if (shaderValue.Type() != ValueType::Map) return;
	ValueDict map = shaderValue.GetDict();
	map.SetValue(CACHED_KEY("id"), Value((int)shader.id));
}

static bool GetBytesFromValue(Value value, int requestedSize, std::vector<unsigned char>& scratch,
		const unsigned char** outBytes, int* outSize) {
	if (outBytes == nullptr || outSize == nullptr) return false;

	*outBytes = nullptr;
	*outSize = 0;
	scratch.clear();

	if (value.Type() == ValueType::Map) {
		BinaryData* rawData = ValueToRawData(value);
		if (rawData == nullptr || rawData->bytes == nullptr || rawData->length <= 0) return false;

		*outBytes = rawData->bytes;
		*outSize = rawData->length;
	} else if (value.Type() == ValueType::String) {
		String text = value.ToString();
		int len = text.LengthB();
		if (len <= 0) return false;

		scratch.resize(len);
		memcpy(scratch.data(), text.c_str(), (size_t)len);
		*outBytes = scratch.data();
		*outSize = len;
	} else if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int count = list.Count();
		if (count <= 0) return false;

		scratch.resize(count);
		for (int n = 0; n < count; n++) {
			int component = list[n].IntValue();
			if (component < 0) component = 0;
			if (component > 255) component = 255;
			scratch[(size_t)n] = (unsigned char)component;
		}

		*outBytes = scratch.data();
		*outSize = count;
	} else {
		return false;
	}

	if (requestedSize > 0 && requestedSize < *outSize) *outSize = requestedSize;
	return *outBytes != nullptr && *outSize > 0;
}

static Value BytesToRawDataValue(const unsigned char* bytes, int length) {
	if (bytes == nullptr || length <= 0) return Value::Null;

	BinaryData* raw = BinaryData::New(length);
	if (raw == nullptr) return Value::Null;
	memcpy(raw->bytes, bytes, (size_t)length);
	return RawDataToValue(raw);
}

static VrDeviceInfo ValueToVrDeviceInfo(Value value) {
	VrDeviceInfo result = {0};
	if (value.Type() != ValueType::Map) return result;

	ValueDict map = value.GetDict();
	result.hResolution = map.Lookup(CACHED_KEY("hResolution"), Value::zero).IntValue();
	result.vResolution = map.Lookup(CACHED_KEY("vResolution"), Value::zero).IntValue();
	result.hScreenSize = map.Lookup(CACHED_KEY("hScreenSize"), Value::zero).FloatValue();
	result.vScreenSize = map.Lookup(CACHED_KEY("vScreenSize"), Value::zero).FloatValue();
	result.eyeToScreenDistance = map.Lookup(CACHED_KEY("eyeToScreenDistance"), Value::zero).FloatValue();
	result.lensSeparationDistance = map.Lookup(CACHED_KEY("lensSeparationDistance"), Value::zero).FloatValue();
	result.interpupillaryDistance = map.Lookup(CACHED_KEY("interpupillaryDistance"), Value::zero).FloatValue();

	Value lensValue = map.Lookup(CACHED_KEY("lensDistortionValues"), Value::Null);
	if (lensValue.Type() == ValueType::List) {
		ValueList lens = lensValue.GetList();
		for (int i = 0; i < 4 && i < lens.Count(); i++) result.lensDistortionValues[i] = lens[i].FloatValue();
	}

	Value chromaValue = map.Lookup(CACHED_KEY("chromaAbCorrection"), Value::Null);
	if (chromaValue.Type() == ValueType::List) {
		ValueList chroma = chromaValue.GetList();
		for (int i = 0; i < 4 && i < chroma.Count(); i++) result.chromaAbCorrection[i] = chroma[i].FloatValue();
	}

	return result;
}

static VrStereoConfig ValueToVrStereoConfig(Value value) {
	VrStereoConfig result{};
	if (value.Type() != ValueType::Map) return result;

	ValueDict map = value.GetDict();

	Value projectionValue = map.Lookup(CACHED_KEY("projection"), Value::Null);
	if (projectionValue.Type() == ValueType::List) {
		ValueList projection = projectionValue.GetList();
		if (projection.Count() > 0) result.projection[0] = ValueToMatrix(projection[0]);
		if (projection.Count() > 1) result.projection[1] = ValueToMatrix(projection[1]);
	}

	Value viewOffsetValue = map.Lookup(CACHED_KEY("viewOffset"), Value::Null);
	if (viewOffsetValue.Type() == ValueType::List) {
		ValueList viewOffset = viewOffsetValue.GetList();
		if (viewOffset.Count() > 0) result.viewOffset[0] = ValueToMatrix(viewOffset[0]);
		if (viewOffset.Count() > 1) result.viewOffset[1] = ValueToMatrix(viewOffset[1]);
	}

	auto ReadFloatPair = [](Value value, float out[2]) {
		if (value.Type() != ValueType::List) return;
		ValueList list = value.GetList();
		if (list.Count() > 0) out[0] = list[0].FloatValue();
		if (list.Count() > 1) out[1] = list[1].FloatValue();
	};

	ReadFloatPair(map.Lookup(CACHED_KEY("leftLensCenter"), Value::Null), result.leftLensCenter);
	ReadFloatPair(map.Lookup(CACHED_KEY("rightLensCenter"), Value::Null), result.rightLensCenter);
	ReadFloatPair(map.Lookup(CACHED_KEY("leftScreenCenter"), Value::Null), result.leftScreenCenter);
	ReadFloatPair(map.Lookup(CACHED_KEY("rightScreenCenter"), Value::Null), result.rightScreenCenter);
	ReadFloatPair(map.Lookup(CACHED_KEY("scale"), Value::Null), result.scale);
	ReadFloatPair(map.Lookup(CACHED_KEY("scaleIn"), Value::Null), result.scaleIn);

	return result;
}

static Value VrStereoConfigToValue(const VrStereoConfig& config) {
	ValueDict map;

	ValueList projection;
	projection.Add(MatrixToValue(config.projection[0]));
	projection.Add(MatrixToValue(config.projection[1]));
	map.SetValue(CACHED_KEY("projection"), DynamicList(projection));

	ValueList viewOffset;
	viewOffset.Add(MatrixToValue(config.viewOffset[0]));
	viewOffset.Add(MatrixToValue(config.viewOffset[1]));
	map.SetValue(CACHED_KEY("viewOffset"), DynamicList(viewOffset));

	ValueList leftLensCenter;
	leftLensCenter.Add(Value(config.leftLensCenter[0]));
	leftLensCenter.Add(Value(config.leftLensCenter[1]));
	map.SetValue(CACHED_KEY("leftLensCenter"), DynamicList(leftLensCenter));

	ValueList rightLensCenter;
	rightLensCenter.Add(Value(config.rightLensCenter[0]));
	rightLensCenter.Add(Value(config.rightLensCenter[1]));
	map.SetValue(CACHED_KEY("rightLensCenter"), DynamicList(rightLensCenter));

	ValueList leftScreenCenter;
	leftScreenCenter.Add(Value(config.leftScreenCenter[0]));
	leftScreenCenter.Add(Value(config.leftScreenCenter[1]));
	map.SetValue(CACHED_KEY("leftScreenCenter"), DynamicList(leftScreenCenter));

	ValueList rightScreenCenter;
	rightScreenCenter.Add(Value(config.rightScreenCenter[0]));
	rightScreenCenter.Add(Value(config.rightScreenCenter[1]));
	map.SetValue(CACHED_KEY("rightScreenCenter"), DynamicList(rightScreenCenter));

	ValueList scale;
	scale.Add(Value(config.scale[0]));
	scale.Add(Value(config.scale[1]));
	map.SetValue(CACHED_KEY("scale"), DynamicList(scale));

	ValueList scaleIn;
	scaleIn.Add(Value(config.scaleIn[0]));
	scaleIn.Add(Value(config.scaleIn[1]));
	map.SetValue(CACHED_KEY("scaleIn"), DynamicList(scaleIn));

	return DynamicMap(map);
}

static AutomationEvent ValueToAutomationEvent(Value value) {
	AutomationEvent result = {0};
	if (value.Type() != ValueType::Map) return result;

	ValueDict map = value.GetDict();
	result.frame = (unsigned int)map.Lookup(CACHED_KEY("frame"), Value::zero).IntValue();
	result.type = (unsigned int)map.Lookup(CACHED_KEY("type"), Value::zero).IntValue();

	Value paramsValue = map.Lookup(CACHED_KEY("params"), Value::Null);
	if (paramsValue.Type() == ValueType::List) {
		ValueList params = paramsValue.GetList();
		for (int i = 0; i < 4 && i < params.Count(); i++) result.params[i] = params[i].IntValue();
	} else {
		result.params[0] = map.Lookup(CACHED_KEY("p0"), Value::zero).IntValue();
		result.params[1] = map.Lookup(CACHED_KEY("p1"), Value::zero).IntValue();
		result.params[2] = map.Lookup(CACHED_KEY("p2"), Value::zero).IntValue();
		result.params[3] = map.Lookup(CACHED_KEY("p3"), Value::zero).IntValue();
	}

	return result;
}

static Value AutomationEventToValue(const AutomationEvent& event) {
	ValueDict map;
	map.SetValue(CACHED_KEY("frame"), Value((double)event.frame));
	map.SetValue(CACHED_KEY("type"), Value((double)event.type));

	ValueList params;
	for (int i = 0; i < 4; i++) params.Add(Value(event.params[i]));
	map.SetValue(CACHED_KEY("params"), DynamicList(params));

	return DynamicMap(map);
}

static AutomationEventList* GetAutomationEventListPtr(Value value) {
	return ResourcePtr<AutomationEventList>(value);
}

// raylib records into the list SetAutomationEventList was given, through a
// pointer into its resource box; this keeps that box alive meanwhile.
bool scriptPollsWindowClose = false;

static PinnedResource activeAutomationEventList;
static AutomationEventList* activeAutomationEventListPtr = nullptr;
// The script's map for that list (a GC root while set), so the events raylib
// records into the list can be copied into the map's `count` and `events`.
static Value activeAutomationEventListMap;
static bool automationEventRecording = false;

static Value AutomationEventListToValue(const AutomationEventList& list) {
	ValueDict map;
	map.SetValue(CACHED_KEY("_handle"), NewResourceHandle(list));
	map.SetValue(CACHED_KEY("capacity"), Value((double)list.capacity));
	map.SetValue(CACHED_KEY("count"), Value((double)list.count));

	ValueList events;
	for (unsigned int i = 0; i < list.count; i++) events.Add(AutomationEventToValue(list.events[i]));
	map.SetValue(CACHED_KEY("events"), DynamicList(events));

	return DynamicMap(map);
}

// Bring a list map's `capacity`, `count` and `events` up to date with the list
// behind its handle, which raylib appends to while recording.  New events are
// appended to the existing `events` list; it is rebuilt only if it is longer
// than the list (which raylib never shrinks while it is set).
static void SyncAutomationEventListValue(Value listValue) {
	if (listValue.Type() != ValueType::Map) return;
	AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
	if (listPtr == nullptr) return;

	ValueDict map = listValue.GetDict();
	map.SetValue(CACHED_KEY("capacity"), Value((double)listPtr->capacity));
	map.SetValue(CACHED_KEY("count"), Value((double)listPtr->count));

	Value eventsValue = map.Lookup(CACHED_KEY("events"), Value::Null);
	if (eventsValue.Type() != ValueType::List || eventsValue.GetList().Count() > (int)listPtr->count) {
		ValueList fresh;
		eventsValue = DynamicList(fresh);
		map.SetValue(CACHED_KEY("events"), eventsValue);
	}
	ValueList events = eventsValue.GetList();
	for (unsigned int i = (unsigned int)events.Count(); i < listPtr->count; i++) {
		events.Add(AutomationEventToValue(listPtr->events[i]));
	}
}

// Make listValue (a list map, or null) the list raylib records into.
static void SetActiveAutomationEventList(Value listValue, AutomationEventList* listPtr) {
	// Catch the old list's map up before letting go of it.
	SyncAutomationEventListValue(activeAutomationEventListMap);
	if (!activeAutomationEventListMap.IsNull()) GCManager::RemoveRoot(activeAutomationEventListMap);
	activeAutomationEventListMap = Value::Null;

	SetAutomationEventList(listPtr);
	activeAutomationEventListPtr = listPtr;
	if (listPtr == nullptr) {
		activeAutomationEventList.Clear();
		return;
	}
	activeAutomationEventList.Set(listValue);
	activeAutomationEventListMap = listValue;
	GCManager::AddRoot(activeAutomationEventListMap);
}

static int ShaderUniformComponentCount(int uniformType) {
	switch (uniformType) {
		case SHADER_UNIFORM_VEC2:
		case SHADER_UNIFORM_IVEC2:
		case SHADER_UNIFORM_UIVEC2: return 2;
		case SHADER_UNIFORM_VEC3:
		case SHADER_UNIFORM_IVEC3:
		case SHADER_UNIFORM_UIVEC3: return 3;
		case SHADER_UNIFORM_VEC4:
		case SHADER_UNIFORM_IVEC4:
		case SHADER_UNIFORM_UIVEC4: return 4;
		default: return 1;
	}
}

static bool IsShaderUniformFloatType(int uniformType) {
	return uniformType == SHADER_UNIFORM_FLOAT
		|| uniformType == SHADER_UNIFORM_VEC2
		|| uniformType == SHADER_UNIFORM_VEC3
		|| uniformType == SHADER_UNIFORM_VEC4;
}

static bool IsShaderUniformIntType(int uniformType) {
	return uniformType == SHADER_UNIFORM_INT
		|| uniformType == SHADER_UNIFORM_IVEC2
		|| uniformType == SHADER_UNIFORM_IVEC3
		|| uniformType == SHADER_UNIFORM_IVEC4
		|| uniformType == SHADER_UNIFORM_SAMPLER2D;
}

static bool IsShaderUniformUIntType(int uniformType) {
	return uniformType == SHADER_UNIFORM_UINT
		|| uniformType == SHADER_UNIFORM_UIVEC2
		|| uniformType == SHADER_UNIFORM_UIVEC3
		|| uniformType == SHADER_UNIFORM_UIVEC4;
}

static void FillFloatComponentsFromValue(Value value, float* out, int components) {
	for (int i = 0; i < components; i++) out[i] = 0.0f;

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) out[i] = list[i].FloatValue();
		return;
	}

	if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();
		if (components > 0) out[0] = map.Lookup(CACHED_KEY("x"), map.Lookup(CACHED_KEY("r"), Value::zero)).FloatValue();
		if (components > 1) out[1] = map.Lookup(CACHED_KEY("y"), map.Lookup(CACHED_KEY("g"), Value::zero)).FloatValue();
		if (components > 2) out[2] = map.Lookup(CACHED_KEY("z"), map.Lookup(CACHED_KEY("b"), Value::zero)).FloatValue();
		if (components > 3) out[3] = map.Lookup(CACHED_KEY("w"), map.Lookup(CACHED_KEY("a"), Value::zero)).FloatValue();
		return;
	}

	if (components > 0) out[0] = value.FloatValue();
}

static void FillIntComponentsFromValue(Value value, int* out, int components) {
	for (int i = 0; i < components; i++) out[i] = 0;

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) out[i] = list[i].IntValue();
		return;
	}

	if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();
		if (components > 0) out[0] = map.Lookup(CACHED_KEY("x"), map.Lookup(CACHED_KEY("r"), Value::zero)).IntValue();
		if (components > 1) out[1] = map.Lookup(CACHED_KEY("y"), map.Lookup(CACHED_KEY("g"), Value::zero)).IntValue();
		if (components > 2) out[2] = map.Lookup(CACHED_KEY("z"), map.Lookup(CACHED_KEY("b"), Value::zero)).IntValue();
		if (components > 3) out[3] = map.Lookup(CACHED_KEY("w"), map.Lookup(CACHED_KEY("a"), Value::zero)).IntValue();
		return;
	}

	if (components > 0) out[0] = value.IntValue();
}

static void FillUIntComponentsFromValue(Value value, unsigned int* out, int components) {
	for (int i = 0; i < components; i++) out[i] = 0;

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) {
			int v = list[i].IntValue();
			out[i] = (unsigned int)(v < 0 ? 0 : v);
		}
		return;
	}

	if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();
		if (components > 0) { int v = map.Lookup(CACHED_KEY("x"), map.Lookup(CACHED_KEY("r"), Value::zero)).IntValue(); out[0] = (unsigned int)(v < 0 ? 0 : v); }
		if (components > 1) { int v = map.Lookup(CACHED_KEY("y"), map.Lookup(CACHED_KEY("g"), Value::zero)).IntValue(); out[1] = (unsigned int)(v < 0 ? 0 : v); }
		if (components > 2) { int v = map.Lookup(CACHED_KEY("z"), map.Lookup(CACHED_KEY("b"), Value::zero)).IntValue(); out[2] = (unsigned int)(v < 0 ? 0 : v); }
		if (components > 3) { int v = map.Lookup(CACHED_KEY("w"), map.Lookup(CACHED_KEY("a"), Value::zero)).IntValue(); out[3] = (unsigned int)(v < 0 ? 0 : v); }
		return;
	}

	if (components > 0) {
		int v = value.IntValue();
		out[0] = (unsigned int)(v < 0 ? 0 : v);
	}
}

static void PackFloatUniformData(Value value, int components, int& count, std::vector<float>& out) {
	out.clear();

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		bool nested = list.Count() > 0 && (list[0].Type() == ValueType::List || list[0].Type() == ValueType::Map);

		if (nested) {
			for (int n = 0; n < list.Count(); n++) {
				float tmp[4] = {0, 0, 0, 0};
				FillFloatComponentsFromValue(list[n], tmp, components);
				for (int c = 0; c < components; c++) out.push_back(tmp[c]);
			}
			if (count <= 0) count = list.Count();
		} else {
			for (int n = 0; n < list.Count(); n++) out.push_back(list[n].FloatValue());
			if (count <= 0) count = (list.Count() + components - 1) / components;
		}
	} else {
		float tmp[4] = {0, 0, 0, 0};
		FillFloatComponentsFromValue(value, tmp, components);
		for (int c = 0; c < components; c++) out.push_back(tmp[c]);
		if (count <= 0) count = 1;
	}

	if (count <= 0) count = 1;
	int needed = count * components;
	if ((int)out.size() < needed) out.resize(needed, 0.0f);
	if ((int)out.size() > needed) out.resize(needed);
}

static void PackIntUniformData(Value value, int components, int& count, std::vector<int>& out) {
	out.clear();

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		bool nested = list.Count() > 0 && (list[0].Type() == ValueType::List || list[0].Type() == ValueType::Map);

		if (nested) {
			for (int n = 0; n < list.Count(); n++) {
				int tmp[4] = {0, 0, 0, 0};
				FillIntComponentsFromValue(list[n], tmp, components);
				for (int c = 0; c < components; c++) out.push_back(tmp[c]);
			}
			if (count <= 0) count = list.Count();
		} else {
			for (int n = 0; n < list.Count(); n++) out.push_back(list[n].IntValue());
			if (count <= 0) count = (list.Count() + components - 1) / components;
		}
	} else {
		int tmp[4] = {0, 0, 0, 0};
		FillIntComponentsFromValue(value, tmp, components);
		for (int c = 0; c < components; c++) out.push_back(tmp[c]);
		if (count <= 0) count = 1;
	}

	if (count <= 0) count = 1;
	int needed = count * components;
	if ((int)out.size() < needed) out.resize(needed, 0);
	if ((int)out.size() > needed) out.resize(needed);
}

static void PackUIntUniformData(Value value, int components, int& count, std::vector<unsigned int>& out) {
	out.clear();

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		bool nested = list.Count() > 0 && (list[0].Type() == ValueType::List || list[0].Type() == ValueType::Map);

		if (nested) {
			for (int n = 0; n < list.Count(); n++) {
				unsigned int tmp[4] = {0, 0, 0, 0};
				FillUIntComponentsFromValue(list[n], tmp, components);
				for (int c = 0; c < components; c++) out.push_back(tmp[c]);
			}
			if (count <= 0) count = list.Count();
		} else {
			for (int n = 0; n < list.Count(); n++) {
				int v = list[n].IntValue();
				out.push_back((unsigned int)(v < 0 ? 0 : v));
			}
			if (count <= 0) count = (list.Count() + components - 1) / components;
		}
	} else {
		unsigned int tmp[4] = {0, 0, 0, 0};
		FillUIntComponentsFromValue(value, tmp, components);
		for (int c = 0; c < components; c++) out.push_back(tmp[c]);
		if (count <= 0) count = 1;
	}

	if (count <= 0) count = 1;
	int needed = count * components;
	if ((int)out.size() < needed) out.resize(needed, 0);
	if ((int)out.size() > needed) out.resize(needed);
}

// Make shader source written for desktop GL work on the web build too.  Web
// runs WebGL2 (GLSL ES 3.00), which is nearly the same language as desktop
// GLSL 3.30, but it insists on "#version 300 es" as the very first line and on
// an explicit float precision in fragment shaders.  So on web, a desktop
// "#version 330" (or later, non-"es") directive is replaced by that header;
// anything else passes through unchanged.  On desktop this is a no-op.
// (Also used by rlLoadShader and rlLoadShaderProgram, in RLgl.cpp.)
String AdaptShaderCode(String code) {
#ifdef PLATFORM_WEB
	const char* src = code.c_str();
	// Find the first token, skipping whitespace and comments.
	const char* p = src;
	while (*p) {
		if (isspace((unsigned char)*p)) p++;
		else if (p[0] == '/' && p[1] == '/') { while (*p && *p != '\n') p++; }
		else if (p[0] == '/' && p[1] == '*') {
			const char* end = strstr(p + 2, "*/");
			p = end ? end + 2 : p + strlen(p);
		}
		else break;
	}
	if (strncmp(p, "#version", 8) != 0) return code;
	const char* lineEnd = strchr(p, '\n');
	if (!lineEnd) lineEnd = p + strlen(p);
	int version = 0;
	char profile[16] = "";
	sscanf(p + 8, "%d %15s", &version, profile);
	if (version < 330 || strcmp(profile, "es") == 0) return code;
	// Keep the comments before the directive, and put the precision statements
	// where the directive was, so line numbers in errors are off by just one.
	std::string result = "#version 300 es\n";
	result.append(src, p - src);
	result += "precision highp float; precision highp int;";
	result += lineEnd;
	return String(result.c_str());
#else
	return code;
#endif
}

// Get the bytes for `count` uniforms of the given type from a script value:
// RawData is used as-is; a number, vector map, or (nested) list is packed into
// `storage`.  If count <= 0 it is inferred.  Returns null for an unknown type.
// (Also used by rlSetUniform, in RLgl.cpp.)
const void* PackUniformValue(Value value, int uniformType, int& count, std::vector<unsigned char>& storage) {
	BinaryData* rawData = nullptr;
	if (value.Type() == ValueType::Map) rawData = ValueToRawData(value);
	if (rawData != nullptr && rawData->bytes != nullptr && rawData->length > 0) {
		if (count <= 0) count = 1;
		return rawData->bytes;
	}

	int components = ShaderUniformComponentCount(uniformType);
	if (IsShaderUniformFloatType(uniformType)) {
		std::vector<float> packed;
		PackFloatUniformData(value, components, count, packed);
		storage.assign((unsigned char*)packed.data(), (unsigned char*)(packed.data() + packed.size()));
		return storage.data();
	}
	if (IsShaderUniformIntType(uniformType)) {
		std::vector<int> packed;
		PackIntUniformData(value, components, count, packed);
		storage.assign((unsigned char*)packed.data(), (unsigned char*)(packed.data() + packed.size()));
		return storage.data();
	}
	if (IsShaderUniformUIntType(uniformType)) {
		std::vector<unsigned int> packed;
		PackUIntUniformData(value, components, count, packed);
		storage.assign((unsigned char*)packed.data(), (unsigned char*)(packed.data() + packed.size()));
		return storage.data();
	}
	return nullptr;
}

struct RaylibCallbackBridgeState {
	Interpreter interpreter;
	Value traceLogCallback = Value::Null;
	Value loadFileDataCallback = Value::Null;
	Value saveFileDataCallback = Value::Null;
	Value loadFileTextCallback = Value::Null;
	Value saveFileTextCallback = Value::Null;
	bool invokingTraceLogCallback = false;
};

static RaylibCallbackBridgeState g_callbackBridgeState;

void ResetRaylibCallbackBridge() {
#ifndef PLATFORM_WEB
	SetLoadFileDataCallback(nullptr);
	SetSaveFileDataCallback(nullptr);
	SetLoadFileTextCallback(nullptr);
	SetSaveFileTextCallback(nullptr);
	SetTraceLogCallback(nullptr);
#endif
	g_callbackBridgeState.interpreter = Interpreter();
	g_callbackBridgeState.traceLogCallback = Value::Null;
	g_callbackBridgeState.loadFileDataCallback = Value::Null;
	g_callbackBridgeState.saveFileDataCallback = Value::Null;
	g_callbackBridgeState.loadFileTextCallback = Value::Null;
	g_callbackBridgeState.saveFileTextCallback = Value::Null;
	g_callbackBridgeState.invokingTraceLogCallback = false;
}

static bool IsFunctionOrNull(Value callback) {
	return callback.IsNull() || callback.Type() == ValueType::Function;
}

static bool InvokeMiniScriptCallback(Value callback, ValueList args, Value* outResult) {
	if (outResult != nullptr) *outResult = Value::Null;
	if (callback.Type() != ValueType::Function) return false;
	if (IsNull(g_callbackBridgeState.interpreter)) return false;

	// Synchronously call the MiniScript funcref with `args` and run it to
	// completion, re-entrantly from inside this C callback.  MS2's
	// Interpreter::RunFunction (-> VM::RunFunction) pushes a frame for the
	// callee above the active native frame and drives the VM until it returns,
	// then restores the interrupted outer execution state.  If the callback
	// raises a runtime error, RunFunction surfaces it on the outer run (which
	// then stops and reports it); we still return true here, having handled the
	// call, so raylib does not also run its C fallback path.
	Value result = g_callbackBridgeState.interpreter.RunFunction(callback, args);
	if (outResult != nullptr) *outResult = result;
	return true;
}

static unsigned char* MiniScriptLoadFileDataBridge(const char* fileName, int* dataSize) {
	if (dataSize != nullptr) *dataSize = 0;

	ValueList args;
	args.Add(Value(String(fileName == nullptr ? "" : fileName)));

	Value callbackResult;
	if (!InvokeMiniScriptCallback(g_callbackBridgeState.loadFileDataCallback, args, &callbackResult)) return nullptr;

	std::vector<unsigned char> scratch;
	const unsigned char* bytes = nullptr;
	int byteCount = 0;
	if (!GetBytesFromValue(callbackResult, 0, scratch, &bytes, &byteCount)) return nullptr;

	unsigned char* result = (unsigned char*)MemAlloc((unsigned int)byteCount);
	if (result == nullptr) return nullptr;

	memcpy(result, bytes, (size_t)byteCount);
	if (dataSize != nullptr) *dataSize = byteCount;
	return result;
}

static bool MiniScriptSaveFileDataBridge(const char* fileName, const void* data, int dataSize) {
	ValueList args;
	args.Add(Value(String(fileName == nullptr ? "" : fileName)));
	args.Add(BytesToRawDataValue((const unsigned char*)data, dataSize));
	args.Add(Value(dataSize));

	Value callbackResult;
	if (!InvokeMiniScriptCallback(g_callbackBridgeState.saveFileDataCallback, args, &callbackResult)) return false;
	return callbackResult.BoolValue();
}

static char* MiniScriptLoadFileTextBridge(const char* fileName) {
	ValueList args;
	args.Add(Value(String(fileName == nullptr ? "" : fileName)));

	Value callbackResult;
	if (!InvokeMiniScriptCallback(g_callbackBridgeState.loadFileTextCallback, args, &callbackResult)) return nullptr;
	if (callbackResult.IsNull()) return nullptr;

	String text = callbackResult.ToString();
	size_t size = text.LengthB() + 1;
	char* result = (char*)MemAlloc((unsigned int)size);
	if (result == nullptr) return nullptr;

	memcpy(result, text.c_str(), size);
	return result;
}

static bool MiniScriptSaveFileTextBridge(const char* fileName, const char* text) {
	ValueList args;
	args.Add(Value(String(fileName == nullptr ? "" : fileName)));
	args.Add(Value(String(text == nullptr ? "" : text)));

	Value callbackResult;
	if (!InvokeMiniScriptCallback(g_callbackBridgeState.saveFileTextCallback, args, &callbackResult)) return false;
	return callbackResult.BoolValue();
}

static void MiniScriptTraceLogBridge(int logLevel, const char* text, va_list args) {
	if (g_callbackBridgeState.invokingTraceLogCallback) return;

	char stackBuffer[2048];
	stackBuffer[0] = '\0';

	if (text != nullptr) {
		va_list argsCopy;
		va_copy(argsCopy, args);
		int written = vsnprintf(stackBuffer, sizeof(stackBuffer), text, argsCopy);
		va_end(argsCopy);

		if (written < 0) {
			stackBuffer[0] = '\0';
		} else if ((size_t)written >= sizeof(stackBuffer)) {
			size_t needed = (size_t)written + 1;
			char* dynamicBuffer = (char*)malloc(needed);
			if (dynamicBuffer != nullptr) {
				va_list argsCopy2;
				va_copy(argsCopy2, args);
				vsnprintf(dynamicBuffer, needed, text, argsCopy2);
				va_end(argsCopy2);

				ValueList argsList;
				argsList.Add(Value(logLevel));
				argsList.Add(Value(String(dynamicBuffer)));

				g_callbackBridgeState.invokingTraceLogCallback = true;
				Value ignored;
				bool ok = InvokeMiniScriptCallback(g_callbackBridgeState.traceLogCallback, argsList, &ignored);
				g_callbackBridgeState.invokingTraceLogCallback = false;

				if (!ok) fprintf(stderr, "[raylib:%d] %s\n", logLevel, dynamicBuffer);
				free(dynamicBuffer);
				return;
			}
		}
	}

	ValueList argsList;
	argsList.Add(Value(logLevel));
	argsList.Add(Value(String(stackBuffer)));

	g_callbackBridgeState.invokingTraceLogCallback = true;
	Value ignored;
	bool ok = InvokeMiniScriptCallback(g_callbackBridgeState.traceLogCallback, argsList, &ignored);
	g_callbackBridgeState.invokingTraceLogCallback = false;

	if (!ok) fprintf(stderr, "[raylib:%d] %s\n", logLevel, stackBuffer);
}

void AddRCoreMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Drawing-related functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		BeginDrawing();
		SetDrawingActive(true);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginDrawing", WithNote(i,
		"Begin canvas (framebuffer) drawing"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		{
			// EndDrawing also waits out the frame for SetTargetFPS; in profile
			// mode, charge that to this call rather than to the code after it.
			profiler::TimedScope timed("raylib.EndDrawing");
			EndDrawing();
		}
		// The batch is flushed, so nothing refers to a collected resource now:
		// the best moment to unload any the collector has queued.
		SetDrawingActive(false);
		ProcessPendingUnloads();
		// EndDrawing is where raylib records input events into the active list.
		if (automationEventRecording) SyncAutomationEventListValue(activeAutomationEventListMap);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndDrawing", WithNote(i,
		"End canvas (framebuffer) drawing and swap buffers (double buffering)"));

	i = Intrinsic::Create("");
	i.AddParam("color", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Value colorVal = context.GetArg(0);
		Color color = ValueToColor(colorVal);
		ClearBackground(color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ClearBackground", WithNote(i,
		"Clear background (framebuffer) to color"));

	// 3D camera and projection functions

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		profiler::TimedScope timed("raylib.BeginMode3D");
		BeginMode3D(camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginMode3D", WithNote(i,
		"Initializes 3D mode with custom camera (3D)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		profiler::TimedScope timed("raylib.EndMode3D");
		EndMode3D();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndMode3D", WithNote(i,
		"End 3D mode and returns to default 2D orthographic mode"));

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		Camera3D camera = ValueToCamera3D(context.GetArg(1));
		Ray ray = GetScreenToWorldRay(position, camera);
		return IntrinsicResult(RayToValue(ray));
	});
	raylibModule.SetValue("GetScreenToWorldRay", WithNote(i,
		"Get a ray trace from screen position (i.e mouse)"));

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("camera");
	i.AddParam("width", Value::zero);
	i.AddParam("height", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		Camera3D camera = ValueToCamera3D(context.GetArg(1));
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		if (width <= 0) width = GetScreenWidth();
		if (height <= 0) height = GetScreenHeight();
		Ray ray = GetScreenToWorldRayEx(position, camera, width, height);
		return IntrinsicResult(RayToValue(ray));
	});
	raylibModule.SetValue("GetScreenToWorldRayEx", WithNote(i,
		"Get a ray trace from the screen position (i.e mouse) within a specific section of the "
		"screen"));

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		Camera3D camera = ValueToCamera3D(context.GetArg(1));
		Vector2 result = GetWorldToScreen(position, camera);
		return IntrinsicResult(Vector2ToValue(result));
	});
	raylibModule.SetValue("GetWorldToScreen", WithNote(i,
		"Get screen space position from a 3d world space position"));

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("camera");
	i.AddParam("width", Value::zero);
	i.AddParam("height", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		Camera3D camera = ValueToCamera3D(context.GetArg(1));
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		if (width <= 0) width = GetScreenWidth();
		if (height <= 0) height = GetScreenHeight();
		Vector2 result = GetWorldToScreenEx(position, camera, width, height);
		return IntrinsicResult(Vector2ToValue(result));
	});
	raylibModule.SetValue("GetWorldToScreenEx", WithNote(i,
		"Get sized screen space position for a 3d world space position (useful for texture drawing)"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(GetCameraMatrix(camera)));
	});
	raylibModule.SetValue("GetCameraMatrix", WithNote(i,
		"Get transform matrix for camera"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("mode", Value(CAMERA_CUSTOM));
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		int mode = context.GetArg(1).IntValue();
		UpdateCamera((Camera*)&camera, mode);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateCamera", WithNote(i,
		"Update camera position for selected mode Camera mode: CAMERA_FREE, CAMERA_FIRST_PERSON, "
		"CAMERA_THIRD_PERSON, CAMERA_ORBITAL or CUSTOM"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("movement", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("rotation", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("zoom", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		Vector3 movement = ValueToVector3(context.GetArg(1));
		Vector3 rotation = ValueToVector3(context.GetArg(2));
		float zoom = context.GetArg(3).FloatValue();
		UpdateCameraPro((Camera*)&camera, movement, rotation, zoom);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateCameraPro", WithNote(i,
		"Update camera movement, movement/rotation values should be provided by user"));

	// rcamera: camera vectors, movement and rotation.  Functions that modify
	// the camera update the given camera map in place.

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(GetCameraForward(&camera)));
	});
	raylibModule.SetValue("GetCameraForward", WithNote(i,
		"Returns the cameras forward vector (normalized)"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(GetCameraUp(&camera)));
	});
	raylibModule.SetValue("GetCameraUp", WithNote(i,
		"Returns the cameras up vector (normalized) Note: The up vector might not be perpendicular "
		"to the forward vector"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(GetCameraRight(&camera)));
	});
	raylibModule.SetValue("GetCameraRight", WithNote(i,
		"Returns the cameras right vector (normalized)"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("distance");
	i.AddParam("moveInWorldPlane", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float distance = context.GetArg(1).FloatValue();
		bool moveInWorldPlane = context.GetArg(2).IntValue() != 0;
		CameraMoveForward(&camera, distance, moveInWorldPlane);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraMoveForward", WithNote(i,
		"Moves the camera in its forward direction"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("distance");
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float distance = context.GetArg(1).FloatValue();
		CameraMoveUp(&camera, distance);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraMoveUp", WithNote(i,
		"Moves the camera in its up direction"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("distance");
	i.AddParam("moveInWorldPlane", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float distance = context.GetArg(1).FloatValue();
		bool moveInWorldPlane = context.GetArg(2).IntValue() != 0;
		CameraMoveRight(&camera, distance, moveInWorldPlane);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraMoveRight", WithNote(i,
		"Moves the camera target in its current right direction"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("delta");
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float delta = context.GetArg(1).FloatValue();
		CameraMoveToTarget(&camera, delta);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraMoveToTarget", WithNote(i,
		"Moves the camera position closer/farther to/from the camera target"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("angle");
	i.AddParam("rotateAroundTarget", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float angle = context.GetArg(1).FloatValue();
		bool rotateAroundTarget = context.GetArg(2).IntValue() != 0;
		CameraYaw(&camera, angle, rotateAroundTarget);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraYaw", WithNote(i,
		"Rotates the camera around its up vector Yaw is \"looking left and right\" If "
		"rotateAroundTarget is false, the camera rotates around its position Note: angle must be "
		"provided in radians"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("angle");
	i.AddParam("lockView", Value::one);
	i.AddParam("rotateAroundTarget", Value::zero);
	i.AddParam("rotateUp", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float angle = context.GetArg(1).FloatValue();
		bool lockView = context.GetArg(2).IntValue() != 0;
		bool rotateAroundTarget = context.GetArg(3).IntValue() != 0;
		bool rotateUp = context.GetArg(4).IntValue() != 0;
		CameraPitch(&camera, angle, lockView, rotateAroundTarget, rotateUp);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraPitch", WithNote(i,
		"Rotates the camera around its right vector, pitch is \"looking up and down\"  - lockView "
		"prevents camera overrotation (aka \"somersaults\")  - rotateAroundTarget defines if "
		"rotation is around target or around its position  - rotateUp rotates the up direction as "
		"well (typically only useful in CAMERA_FREE) NOTE: [angle] must be provided in radians"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("angle");
	i.set_Code(INTRINSIC_LAMBDA {
		Value cameraValue = context.GetArg(0);
		Camera3D camera = ValueToCamera3D(cameraValue);
		float angle = context.GetArg(1).FloatValue();
		CameraRoll(&camera, angle);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CameraRoll", WithNote(i,
		"Rotates the camera around its forward vector Roll is \"turning your head sideways to the "
		"left or right\" Note: angle must be provided in radians"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(GetCameraViewMatrix(&camera)));
	});
	raylibModule.SetValue("GetCameraViewMatrix", WithNote(i,
		"Returns the camera view matrix"));

	// aspect <= 0 means use the screen's aspect ratio
	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("aspect", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context.GetArg(0));
		float aspect = context.GetArg(1).FloatValue();
		if (aspect <= 0) aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
		return IntrinsicResult(MatrixToValue(GetCameraProjectionMatrix(&camera, aspect)));
	});
	raylibModule.SetValue("GetCameraProjectionMatrix", WithNote(i,
		"Returns the camera projection matrix"));

	// Shader functions

	i = Intrinsic::Create("");
	i.AddParam("vsFileName", String());
	i.AddParam("fsFileName", String());
	i.set_Code(INTRINSIC_LAMBDA {
		// Either name may be empty, meaning "use raylib's default shader"; only
		// the ones actually given are resolved.
		String vsArg = context.GetArg(0).ToString();
		String fsArg = context.GetArg(1).ToString();
		String vsFileName, fsFileName;
		if (vsArg.LengthB() > 0 && !fs::HostPath(vsArg, vsFileName)) return IntrinsicResult::Null;
		if (fsArg.LengthB() > 0 && !fs::HostPath(fsArg, fsFileName)) return IntrinsicResult::Null;
#ifdef PLATFORM_WEB
		// Load the text ourselves, so it can go through AdaptShaderCode.
		String vsCode, fsCode;
		if (vsFileName.LengthB() > 0) {
			char* text = LoadFileText(vsFileName.c_str());
			if (!text) return IntrinsicResult::Null;
			vsCode = AdaptShaderCode(String(text));
			UnloadFileText(text);
		}
		if (fsFileName.LengthB() > 0) {
			char* text = LoadFileText(fsFileName.c_str());
			if (!text) return IntrinsicResult::Null;
			fsCode = AdaptShaderCode(String(text));
			UnloadFileText(text);
		}
		const char* vsPtr = vsCode.LengthB() > 0 ? vsCode.c_str() : nullptr;
		const char* fsPtr = fsCode.LengthB() > 0 ? fsCode.c_str() : nullptr;
		Shader shader = LoadShaderFromMemory(vsPtr, fsPtr);
#else
		const char* vsPtr = vsFileName.LengthB() > 0 ? vsFileName.c_str() : nullptr;
		const char* fsPtr = fsFileName.LengthB() > 0 ? fsFileName.c_str() : nullptr;
		Shader shader = LoadShader(vsPtr, fsPtr);
#endif
		if (!IsShaderValid(shader)) return IntrinsicResult::Null;
		rcShader++;
		return IntrinsicResult(ShaderToValue(shader));
	});
	raylibModule.SetValue("LoadShader", WithNote(i,
		"Load shader from files and bind default locations NOTE: If shader filename is NULL, using "
		"default vertex/fragment shaders"));

	i = Intrinsic::Create("");
	i.AddParam("vsCode", String());
	i.AddParam("fsCode", String());
	i.set_Code(INTRINSIC_LAMBDA {
		String vsCode = AdaptShaderCode(context.GetArg(0).ToString());
		String fsCode = AdaptShaderCode(context.GetArg(1).ToString());
		const char* vsPtr = vsCode.LengthB() > 0 ? vsCode.c_str() : nullptr;
		const char* fsPtr = fsCode.LengthB() > 0 ? fsCode.c_str() : nullptr;
		Shader shader = LoadShaderFromMemory(vsPtr, fsPtr);
		if (!IsShaderValid(shader)) return IntrinsicResult::Null;
		rcShader++;
		return IntrinsicResult(ShaderToValue(shader));
	});
	raylibModule.SetValue("LoadShaderFromMemory", WithNote(i,
		"Load shader from code strings and bind default locations"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		return IntrinsicResult(IsShaderValid(shader));
	});
	raylibModule.SetValue("IsShaderValid", WithNote(i,
		"Check if shader is valid (loaded on GPU)"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		profiler::TimedScope timed("raylib.BeginShaderMode");
		BeginShaderMode(shader);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginShaderMode", WithNote(i,
		"Begin custom shader mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		profiler::TimedScope timed("raylib.EndShaderMode");
		EndShaderMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndShaderMode", WithNote(i,
		"End custom shader mode (returns to default shader)"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("uniformName");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		String uniformName = context.GetArg(1).ToString();
		return IntrinsicResult(GetShaderLocation(shader, uniformName.c_str()));
	});
	raylibModule.SetValue("GetShaderLocation", WithNote(i,
		"Get shader uniform location"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("attribName");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		String attribName = context.GetArg(1).ToString();
		return IntrinsicResult(GetShaderLocationAttrib(shader, attribName.c_str()));
	});
	raylibModule.SetValue("GetShaderLocationAttrib", WithNote(i,
		"Get shader attribute location"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("locIndex");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		int locIndex = context.GetArg(1).IntValue();
		if (shader.locs == nullptr || locIndex < 0 || locIndex >= RL_MAX_SHADER_LOCATIONS) return IntrinsicResult(-1);
		return IntrinsicResult(shader.locs[locIndex]);
	});
	raylibModule.SetValue("GetShaderLoc", WithNote(i,
		"Get one entry of a shader's location table (SHADER_LOC_MATRIX_MODEL, "
		"SHADER_LOC_VECTOR_VIEW...): the uniform or attribute location raylib uses for it, or -1 "
		"if none"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("locIndex");
	i.AddParam("location");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		int locIndex = context.GetArg(1).IntValue();
		if (shader.locs == nullptr || locIndex < 0 || locIndex >= RL_MAX_SHADER_LOCATIONS) return IntrinsicResult::Null;
		shader.locs[locIndex] = context.GetArg(2).IntValue();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShaderLoc", WithNote(i,
		"Set one entry of a shader's location table (SHADER_LOC_MATRIX_MODEL, "
		"SHADER_LOC_VECTOR_VIEW...), usually to a location from GetShaderLocation, so raylib feeds "
		"that uniform or attribute when drawing. Every copy of the shader shares the table"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("locIndex");
	i.AddParam("mat");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		int locIndex = context.GetArg(1).IntValue();
		Matrix mat = ValueToMatrix(context.GetArg(2));
		SetShaderValueMatrix(shader, locIndex, mat);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShaderValueMatrix", WithNote(i,
		"Set shader uniform value (matrix 4x4)"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("locIndex");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		int locIndex = context.GetArg(1).IntValue();
		Texture2D texture = ValueToTexture(context.GetArg(2));
		SetShaderValueTexture(shader, locIndex, texture);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShaderValueTexture", WithNote(i,
		"Set shader uniform value for texture"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("locIndex");
	i.AddParam("value");
	i.AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		int locIndex = context.GetArg(1).IntValue();
		Value value = context.GetArg(2);
		int uniformType = context.GetArg(3).IntValue();

		BinaryData* rawData = nullptr;
		if (value.Type() == ValueType::Map) rawData = ValueToRawData(value);
		if (rawData != nullptr && rawData->bytes != nullptr && rawData->length > 0) {
			SetShaderValue(shader, locIndex, rawData->bytes, uniformType);
			return IntrinsicResult::Null;
		}

		int components = ShaderUniformComponentCount(uniformType);
		if (IsShaderUniformFloatType(uniformType)) {
			std::vector<float> packed;
			int count = 1;
			PackFloatUniformData(value, components, count, packed);
			SetShaderValue(shader, locIndex, packed.data(), uniformType);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformIntType(uniformType)) {
			std::vector<int> packed;
			int count = 1;
			PackIntUniformData(value, components, count, packed);
			SetShaderValue(shader, locIndex, packed.data(), uniformType);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformUIntType(uniformType)) {
			std::vector<unsigned int> packed;
			int count = 1;
			PackUIntUniformData(value, components, count, packed);
			SetShaderValue(shader, locIndex, packed.data(), uniformType);
			return IntrinsicResult::Null;
		}

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShaderValue", WithNote(i,
		"Set shader uniform value"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.AddParam("locIndex");
	i.AddParam("value");
	i.AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i.AddParam("count", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context.GetArg(0));
		int locIndex = context.GetArg(1).IntValue();
		Value value = context.GetArg(2);
		int uniformType = context.GetArg(3).IntValue();
		int count = context.GetArg(4).IntValue();

		BinaryData* rawData = nullptr;
		if (value.Type() == ValueType::Map) rawData = ValueToRawData(value);
		if (rawData != nullptr && rawData->bytes != nullptr && rawData->length > 0) {
			if (count <= 0) count = 1;
			SetShaderValueV(shader, locIndex, rawData->bytes, uniformType, count);
			return IntrinsicResult::Null;
		}

		int components = ShaderUniformComponentCount(uniformType);
		if (IsShaderUniformFloatType(uniformType)) {
			std::vector<float> packed;
			PackFloatUniformData(value, components, count, packed);
			SetShaderValueV(shader, locIndex, packed.data(), uniformType, count);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformIntType(uniformType)) {
			std::vector<int> packed;
			PackIntUniformData(value, components, count, packed);
			SetShaderValueV(shader, locIndex, packed.data(), uniformType, count);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformUIntType(uniformType)) {
			std::vector<unsigned int> packed;
			PackUIntUniformData(value, components, count, packed);
			SetShaderValueV(shader, locIndex, packed.data(), uniformType, count);
			return IntrinsicResult::Null;
		}

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShaderValueV", WithNote(i,
		"Set shader uniform value vector"));

	i = Intrinsic::Create("");
	i.AddParam("shader");
	i.set_Code(INTRINSIC_LAMBDA {
		Value shaderValue = context.GetArg(0);
		Shader shader;
		if (TakeResource(shaderValue, &shader)) {
			UnloadShader(shader);
			rcShader--;
		}

		SyncShaderValue(shaderValue, Shader{0, NULL});
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadShader", WithNote(i,
		"Unload shader from GPU memory (VRAM)"));

	// Timing functions

	i = Intrinsic::Create("");
	i.AddParam("fps");
	i.set_Code(INTRINSIC_LAMBDA {
		SetTargetFPS(context.GetArg(0).IntValue());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTargetFPS", WithNote(i,
		"Set target FPS (maximum)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetFrameTime());
	});
	raylibModule.SetValue("GetFrameTime", WithNote(i,
		"Get time in seconds for last frame drawn (delta time)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTime());
	});
	raylibModule.SetValue("GetTime", WithNote(i,
		"Get elapsed time measure in seconds since InitTimer()"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetFPS());
	});
	raylibModule.SetValue("GetFPS", WithNote(i,
		"Get current FPS NOTE: Calculating an average framerate"));

	// Input-related functions: keyboard

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyPressed(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsKeyPressed", WithNote(i,
		"Check if key has been pressed once"));

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyPressedRepeat(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsKeyPressedRepeat", WithNote(i,
		"Check if key has been pressed again"));

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyDown(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsKeyDown", WithNote(i,
		"Check if key is being pressed (key held down)"));

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyReleased(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsKeyReleased", WithNote(i,
		"Check if key has been released once"));

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyUp(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsKeyUp", WithNote(i,
		"Check if key is NOT being pressed (key not held down)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetKeyPressed());
	});
	raylibModule.SetValue("GetKeyPressed", WithNote(i,
		"Get the last key pressed"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetCharPressed());
	});
	raylibModule.SetValue("GetCharPressed", WithNote(i,
		"Get the last char pressed"));

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		const char* keyName = GetKeyName(context.GetArg(0).IntValue());
		if (keyName == nullptr) return IntrinsicResult(String());
		return IntrinsicResult(String(keyName));
	});
	raylibModule.SetValue("GetKeyName", WithNote(i,
		"Get the character a key types on the current keyboard layout (e.g. \"q\" for KEY_A on an "
		"AZERTY keyboard), or \"\" if none"));

	i = Intrinsic::Create("");
	i.AddParam("key");
	i.set_Code(INTRINSIC_LAMBDA {
		SetExitKey(context.GetArg(0).IntValue());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetExitKey", WithNote(i,
		"Set a custom key to exit program NOTE: default exitKey is set to ESCAPE"));

	// Input-related functions: gamepad

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadAvailable(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsGamepadAvailable", WithNote(i,
		"Check if gamepad is available"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadName(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("GetGamepadName", WithNote(i,
		"Get the internal name of a gamepad"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonPressed(
			context.GetArg(0).IntValue(),
			context.GetArg(1).IntValue()));
	});
	raylibModule.SetValue("IsGamepadButtonPressed", WithNote(i,
		"Check if gamepad button has been pressed once"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonDown(
			context.GetArg(0).IntValue(),
			context.GetArg(1).IntValue()));
	});
	raylibModule.SetValue("IsGamepadButtonDown", WithNote(i,
		"Check if gamepad button is being pressed"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonReleased(
			context.GetArg(0).IntValue(),
			context.GetArg(1).IntValue()));
	});
	raylibModule.SetValue("IsGamepadButtonReleased", WithNote(i,
		"Check if gamepad button has NOT been pressed once"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonUp(
			context.GetArg(0).IntValue(),
			context.GetArg(1).IntValue()));
	});
	raylibModule.SetValue("IsGamepadButtonUp", WithNote(i,
		"Check if gamepad button is NOT being pressed"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadButtonPressed());
	});
	raylibModule.SetValue("GetGamepadButtonPressed", WithNote(i,
		"Get the last gamepad button pressed NOTE: Returns last gamepad button down, down->up "
		"change not considered"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadAxisCount(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("GetGamepadAxisCount", WithNote(i,
		"Get gamepad axis count"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.AddParam("axis");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadAxisMovement(
			context.GetArg(0).IntValue(),
			context.GetArg(1).IntValue()));
	});
	raylibModule.SetValue("GetGamepadAxisMovement", WithNote(i,
		"Get axis movement vector for a gamepad"));

	i = Intrinsic::Create("");
	i.AddParam("mappings");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(SetGamepadMappings(context.GetArg(0).ToString().c_str()));
	});
	raylibModule.SetValue("SetGamepadMappings", WithNote(i,
		"Set internal gamepad mappings"));

	i = Intrinsic::Create("");
	i.AddParam("gamepad", 0);
	i.AddParam("leftMotor", 0.0);
	i.AddParam("rightMotor", 0.0);
	i.AddParam("duration", 0.0);
	i.set_Code(INTRINSIC_LAMBDA {
		SetGamepadVibration(
			context.GetArg(0).IntValue(),
			context.GetArg(1).FloatValue(),
			context.GetArg(2).FloatValue(),
			context.GetArg(3).FloatValue());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetGamepadVibration", WithNote(i,
		"Set gamepad vibration"));

	// Input-related functions: mouse

	i = Intrinsic::Create("");
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonPressed(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsMouseButtonPressed", WithNote(i,
		"Check if mouse button has been pressed once"));

	i = Intrinsic::Create("");
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonDown(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsMouseButtonDown", WithNote(i,
		"Check if mouse button is being pressed"));

	i = Intrinsic::Create("");
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonReleased(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsMouseButtonReleased", WithNote(i,
		"Check if mouse button has been released once"));

	i = Intrinsic::Create("");
	i.AddParam("button");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonUp(context.GetArg(0).IntValue()));
	});
	raylibModule.SetValue("IsMouseButtonUp", WithNote(i,
		"Check if mouse button is NOT being pressed"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetMouseX());
	});
	raylibModule.SetValue("GetMouseX", WithNote(i,
		"Get mouse position X"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetMouseY());
	});
	raylibModule.SetValue("GetMouseY", WithNote(i,
		"Get mouse position Y"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 pos = GetMousePosition();
		ValueDict posMap;
		posMap.SetValue(CACHED_KEY("x"), Value(pos.x));
		posMap.SetValue(CACHED_KEY("y"), Value(pos.y));
		return IntrinsicResult(DynamicMap(posMap));
	});
	raylibModule.SetValue("GetMousePosition", WithNote(i,
		"Get mouse position XY"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 delta = GetMouseDelta();
		ValueDict deltaMap;
		deltaMap.SetValue(CACHED_KEY("x"), Value(delta.x));
		deltaMap.SetValue(CACHED_KEY("y"), Value(delta.y));
		return IntrinsicResult(DynamicMap(deltaMap));
	});
	raylibModule.SetValue("GetMouseDelta", WithNote(i,
		"Get mouse delta between frames"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetMouseWheelMove());
	});
	raylibModule.SetValue("GetMouseWheelMove", WithNote(i,
		"Get mouse wheel movement Y"));

	i = Intrinsic::Create("");
	i.AddParam("cursor");
	i.set_Code(INTRINSIC_LAMBDA {
		SetMouseCursor(context.GetArg(0).IntValue());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMouseCursor", WithNote(i,
		"Set mouse cursor"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		ShowCursor();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ShowCursor", WithNote(i,
		"Show mouse cursor"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		HideCursor();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("HideCursor", WithNote(i,
		"Hide mouse cursor"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsCursorHidden());
	});
	raylibModule.SetValue("IsCursorHidden", WithNote(i,
		"Check if cursor is not visible"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsCursorOnScreen());
	});
	raylibModule.SetValue("IsCursorOnScreen", WithNote(i,
		"Check if cursor is on the current screen"));

	// Set window title/icon (platform-gated)
	i = Intrinsic::Create("");
	i.AddParam("caption", "raylib-miniscript");
	i.set_Code(INTRINSIC_LAMBDA {
		String caption = context.GetArg(0).ToString();
#ifdef PLATFORM_WEB
		_SetWindowTitle_Web(caption.c_str());
#else
		SetWindowTitle(caption.c_str());
#endif
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowTitle", WithNote(i,
		"Set title for window"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
#ifdef PLATFORM_WEB
		int size;
		unsigned char *data = ExportImageToMemory(image, ".png", &size);
		_SetWindowIcon_Web(data, size);
		free(data);
#else
		SetWindowIcon(image);
#endif
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowIcon", WithNote(i,
		"Set icon for window NOTE 1: Image must be in RGBA format, 8bit per channel NOTE 2: Image "
		"is scaled by the OS for all required sizes"));

	i = Intrinsic::Create("");
	i.AddParam("images");
	i.AddParam("count", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowIcons");
		return IntrinsicResult::Null;
	#endif
		Value imagesValue = context.GetArg(0);
		int count = context.GetArg(1).IntValue();

		if (imagesValue.Type() != ValueType::List) {
			Image image = ValueToImage(imagesValue);
			SetWindowIcon(image);
			return IntrinsicResult::Null;
		}

		ValueList imageList = imagesValue.GetList();
		if (imageList.Count() <= 0) return IntrinsicResult::Null;
		if (count <= 0 || count > imageList.Count()) count = imageList.Count();

		std::vector<Image> images;
		images.reserve(count);
		for (int i = 0; i < count; i++) images.push_back(ValueToImage(imageList[i]));

		SetWindowIcons(images.data(), count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowIcons", WithNote(i,
		"Set icon for window, multiple images NOTE 1: Images must be in RGBA format, 8bit per "
		"channel NOTE 2: The multiple images are used depending on provided sizes Standard Windows "
		"icon sizes: 256, 128, 96, 64, 48, 32, 24, 16"));

	// Screen dimension functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetScreenWidth());
	});
	raylibModule.SetValue("GetScreenWidth", WithNote(i,
		"Get current screen width"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetScreenHeight());
	});
	raylibModule.SetValue("GetScreenHeight", WithNote(i,
		"Get current screen height"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetRenderWidth());
	});
	raylibModule.SetValue("GetRenderWidth", WithNote(i,
		"Get current render width which is equal to screen width*dpi scale"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetRenderHeight());
	});
	raylibModule.SetValue("GetRenderHeight", WithNote(i,
		"Get current screen height which is equal to screen height*dpi scale"));

	i = Intrinsic::Create("");
	i.AddParam("width", Value(960));
	i.AddParam("height", Value(640));
	i.AddParam("title", Value("raylib-miniscript"));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		String title = context.GetArg(2).ToString();
		bool wasOpen = IsWindowReady();
		InitWindow(width, height, title.c_str());
		if (!wasOpen && IsWindowReady()) BumpGraphicsGeneration();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("InitWindow", WithNote(i,
		"Initialize window and OpenGL context"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		CloseWindow();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("CloseWindow", WithNote(i,
		"Close window and unload OpenGL context"));

	// Window state functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		scriptPollsWindowClose = true;
#ifdef PLATFORM_WEB
		// An exception to our rule of doing exactly what raylib does.  raylib's
		// web WindowShouldClose is just emscripten_sleep(12) followed by "return
		// false": the sleep hands control to the browser, so that a program
		// written as one endless loop still gets its frames shown and its input
		// delivered.  We never need that -- the host runs scripts from
		// emscripten_set_main_loop, and a script's `yield` already returns to
		// the browser every frame -- so the sleep only takes 12+ ms out of each
		// 16.7 ms frame, and opens a second point mid-frame where input arrives
		// (which the next EndDrawing then marks as old, so presses get missed).
		// The answer is the same without it: there's no window to close.
		return IntrinsicResult(false);
#else
		return IntrinsicResult(WindowShouldClose());
#endif
	});
	raylibModule.SetValue("WindowShouldClose", WithNote(i,
		"Check if application should close NOTE: By default, if KEY_ESCAPE pressed or window close "
		"icon clicked"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsWindowFullscreen());
	});
	raylibModule.SetValue("IsWindowFullscreen", WithNote(i,
		"Check if window is currently fullscreen"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowHidden");
		return IntrinsicResult::Zero;
#endif
		return IntrinsicResult(IsWindowHidden());
	});
	raylibModule.SetValue("IsWindowHidden", WithNote(i,
		"Check if window is currently hidden"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowMinimized");
		return IntrinsicResult::Zero;
#endif
		return IntrinsicResult(IsWindowMinimized());
	});
	raylibModule.SetValue("IsWindowMinimized", WithNote(i,
		"Check if window has been minimized"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowMaximized");
		return IntrinsicResult::Zero;
#endif
		return IntrinsicResult(IsWindowMaximized());
	});
	raylibModule.SetValue("IsWindowMaximized", WithNote(i,
		"Check if window has been maximized"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowResized");
		return IntrinsicResult::Zero;
#endif
		return IntrinsicResult(IsWindowResized());
	});
	raylibModule.SetValue("IsWindowResized", WithNote(i,
		"Check if window has been resizedLastFrame"));

	i = Intrinsic::Create("");
	i.AddParam("flags");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowState");
		return IntrinsicResult::Zero;
	#endif
		unsigned int flags = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(IsWindowState(flags));
	});
	raylibModule.SetValue("IsWindowState", WithNote(i,
		"Check if one specific window flag is enabled"));

	i = Intrinsic::Create("");
	i.AddParam("flags");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowState");
		return IntrinsicResult::Null;
	#endif
		unsigned int flags = (unsigned int)context.GetArg(0).IntValue();
		SetWindowState(flags);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowState", WithNote(i,
		"Set window configuration state using flags"));

	i = Intrinsic::Create("");
	i.AddParam("flags");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ClearWindowState");
		return IntrinsicResult::Null;
	#endif
		unsigned int flags = (unsigned int)context.GetArg(0).IntValue();
		ClearWindowState(flags);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ClearWindowState", WithNote(i,
		"Clear window configuration state flags"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		ToggleFullscreen();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ToggleFullscreen", WithNote(i,
		"Toggle fullscreen mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ToggleBorderlessWindowed");
		return IntrinsicResult::Null;
	#endif
		ToggleBorderlessWindowed();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ToggleBorderlessWindowed", WithNote(i,
		"Toggle borderless windowed mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("MaximizeWindow");
		return IntrinsicResult::Null;
	#endif
		MaximizeWindow();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("MaximizeWindow", WithNote(i,
		"Set window state: maximized, if resizable"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("MinimizeWindow");
		return IntrinsicResult::Null;
	#endif
		MinimizeWindow();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("MinimizeWindow", WithNote(i,
		"Set window state: minimized"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("RestoreWindow");
		return IntrinsicResult::Null;
	#endif
		RestoreWindow();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("RestoreWindow", WithNote(i,
		"Restore window from being minimized/maximized"));

	i = Intrinsic::Create("");
	i.AddParam("x");
	i.AddParam("y");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowPosition");
		return IntrinsicResult::Null;
	#endif
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		SetWindowPosition(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowPosition", WithNote(i,
		"Set window position on screen (windowed mode)"));

	i = Intrinsic::Create("");
	i.AddParam("monitor");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowMonitor");
		return IntrinsicResult::Null;
	#endif
		int monitor = context.GetArg(0).IntValue();
		SetWindowMonitor(monitor);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowMonitor", WithNote(i,
		"Set monitor for the current window"));

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowMinSize");
		return IntrinsicResult::Null;
	#endif
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		SetWindowMinSize(width, height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowMinSize", WithNote(i,
		"Set window minimum dimensions (FLAG_WINDOW_RESIZABLE)"));

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowMaxSize");
		return IntrinsicResult::Null;
	#endif
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		SetWindowMaxSize(width, height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowMaxSize", WithNote(i,
		"Set window maximum dimensions (FLAG_WINDOW_RESIZABLE)"));

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowSize");
		return IntrinsicResult::Null;
	#endif
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		SetWindowSize(width, height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowSize", WithNote(i,
		"Set window dimensions"));

	i = Intrinsic::Create("");
	i.AddParam("opacity");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowOpacity");
		return IntrinsicResult::Null;
	#endif
		float opacity = context.GetArg(0).FloatValue();
		SetWindowOpacity(opacity);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowOpacity", WithNote(i,
		"Set window opacity, value opacity is between 0.0 and 1.0"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowFocused");
		return IntrinsicResult::Null;
	#endif
		SetWindowFocused();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetWindowFocused", WithNote(i,
		"Set window focused"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWindowHandle");
		return IntrinsicResult::Zero;
	#endif
		// Only for display: nothing takes this number back as a pointer
		return IntrinsicResult(Value((double)(intptr_t)GetWindowHandle()));
	});
	raylibModule.SetValue("GetWindowHandle", WithNote(i,
		"Get native window handle"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorCount");
		return IntrinsicResult::Zero;
	#endif
		return IntrinsicResult(GetMonitorCount());
	});
	raylibModule.SetValue("GetMonitorCount", WithNote(i,
		"Get number of monitors"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetCurrentMonitor");
		return IntrinsicResult::Zero;
	#endif
		return IntrinsicResult(GetCurrentMonitor());
	});
	raylibModule.SetValue("GetCurrentMonitor", WithNote(i,
		"Get current monitor where window is placed"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorPosition");
		return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));
	#endif
		int monitor = context.GetArg(0).IntValue();
		return IntrinsicResult(Vector2ToValue(GetMonitorPosition(monitor)));
	});
	raylibModule.SetValue("GetMonitorPosition", WithNote(i,
		"Get selected monitor position"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorWidth");
		return IntrinsicResult::Zero;
	#endif
		int monitor = context.GetArg(0).IntValue();
		return IntrinsicResult(GetMonitorWidth(monitor));
	});
	raylibModule.SetValue("GetMonitorWidth", WithNote(i,
		"Get selected monitor width (currently used by monitor)"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorHeight");
		return IntrinsicResult::Zero;
	#endif
		int monitor = context.GetArg(0).IntValue();
		return IntrinsicResult(GetMonitorHeight(monitor));
	});
	raylibModule.SetValue("GetMonitorHeight", WithNote(i,
		"Get selected monitor height (currently used by monitor)"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorPhysicalWidth");
		return IntrinsicResult::Zero;
	#endif
		int monitor = context.GetArg(0).IntValue();
		return IntrinsicResult(GetMonitorPhysicalWidth(monitor));
	});
	raylibModule.SetValue("GetMonitorPhysicalWidth", WithNote(i,
		"Get selected monitor physical width in millimetres"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorPhysicalHeight");
		return IntrinsicResult::Zero;
	#endif
		int monitor = context.GetArg(0).IntValue();
		return IntrinsicResult(GetMonitorPhysicalHeight(monitor));
	});
	raylibModule.SetValue("GetMonitorPhysicalHeight", WithNote(i,
		"Get selected monitor physical height in millimetres"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorRefreshRate");
		return IntrinsicResult::Zero;
	#endif
		int monitor = context.GetArg(0).IntValue();
		return IntrinsicResult(GetMonitorRefreshRate(monitor));
	});
	raylibModule.SetValue("GetMonitorRefreshRate", WithNote(i,
		"Get selected monitor refresh rate"));

	i = Intrinsic::Create("");
	i.AddParam("monitor", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorName");
		return IntrinsicResult(String());
	#endif
		int monitor = context.GetArg(0).IntValue();
		String name = GetMonitorName(monitor);
		return IntrinsicResult(name);
	});
	raylibModule.SetValue("GetMonitorName", WithNote(i,
		"Get the human-readable name of a monitor (\"\" on web)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWindowPosition");
		return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));
	#endif
		return IntrinsicResult(Vector2ToValue(GetWindowPosition()));
	});
	raylibModule.SetValue("GetWindowPosition", WithNote(i,
		"Get window position XY on monitor"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWindowScaleDPI");
		return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));
	#endif
		return IntrinsicResult(Vector2ToValue(GetWindowScaleDPI()));
	});
	raylibModule.SetValue("GetWindowScaleDPI", WithNote(i,
		"Get window scale DPI factor for current monitor"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsWindowFocused());
	});
	raylibModule.SetValue("IsWindowFocused", WithNote(i,
		"Check if window has the focus"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(IsWindowReady());
	});
	raylibModule.SetValue("IsWindowReady", WithNote(i,
		"Check if window has been initialized successfully"));

	// Additional mouse functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 wheelMove = GetMouseWheelMoveV();
		ValueDict wheelMap;
		wheelMap.SetValue(CACHED_KEY("x"), Value(wheelMove.x));
		wheelMap.SetValue(CACHED_KEY("y"), Value(wheelMove.y));
		return IntrinsicResult(DynamicMap(wheelMap));
	});
	raylibModule.SetValue("GetMouseWheelMoveV", WithNote(i,
		"Get mouse wheel movement X/Y as a vector"));

	i = Intrinsic::Create("");
	i.AddParam("x");
	i.AddParam("y");
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		SetMousePosition(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMousePosition", WithNote(i,
		"Set mouse position XY"));

	i = Intrinsic::Create("");
	i.AddParam("offsetX");
	i.AddParam("offsetY");
	i.set_Code(INTRINSIC_LAMBDA {
		int offsetX = context.GetArg(0).IntValue();
		int offsetY = context.GetArg(1).IntValue();
		SetMouseOffset(offsetX, offsetY);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMouseOffset", WithNote(i,
		"Set mouse offset NOTE: Useful when rendering to different size targets"));

	i = Intrinsic::Create("");
	i.AddParam("scaleX");
	i.AddParam("scaleY");
	i.set_Code(INTRINSIC_LAMBDA {
		float scaleX = context.GetArg(0).FloatValue();
		float scaleY = context.GetArg(1).FloatValue();
		SetMouseScale(scaleX, scaleY);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMouseScale", WithNote(i,
		"Set mouse scaling NOTE: Useful when rendering to different size targets"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		EnableCursor();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EnableCursor", WithNote(i,
		"Enable cursor (unlock cursor)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		DisableCursor();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DisableCursor", WithNote(i,
		"Disable cursor (lock cursor)"));

	// Touch input functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTouchX());
	});
	raylibModule.SetValue("GetTouchX", WithNote(i,
		"Get touch position X for touch point 0 (relative to screen size)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTouchY());
	});
	raylibModule.SetValue("GetTouchY", WithNote(i,
		"Get touch position Y for touch point 0 (relative to screen size)"));

	i = Intrinsic::Create("");
	i.AddParam("index", 0);
	i.set_Code(INTRINSIC_LAMBDA {
		int index = context.GetArg(0).IntValue();
		Vector2 pos = GetTouchPosition(index);
		ValueDict posMap;
		posMap.SetValue(CACHED_KEY("x"), Value(pos.x));
		posMap.SetValue(CACHED_KEY("y"), Value(pos.y));
		return IntrinsicResult(DynamicMap(posMap));
	});
	raylibModule.SetValue("GetTouchPosition", WithNote(i,
		"Get touch position XY for a touch point index (relative to screen size)"));

	i = Intrinsic::Create("");
	i.AddParam("index", 0);
	i.set_Code(INTRINSIC_LAMBDA {
		int index = context.GetArg(0).IntValue();
		return IntrinsicResult(GetTouchPointId(index));
	});
	raylibModule.SetValue("GetTouchPointId", WithNote(i,
		"Get touch point identifier for provided index"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTouchPointCount());
	});
	raylibModule.SetValue("GetTouchPointCount", WithNote(i,
		"Get number of touch points"));

	// Gesture functions

	i = Intrinsic::Create("");
	i.AddParam("flags");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int flags = (unsigned int)context.GetArg(0).IntValue();
		SetGesturesEnabled(flags);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetGesturesEnabled", WithNote(i,
		"Enable only desired gestures to be detected"));

	i = Intrinsic::Create("");
	i.AddParam("gesture");
	i.set_Code(INTRINSIC_LAMBDA {
		int gesture = context.GetArg(0).IntValue();
		return IntrinsicResult(IsGestureDetected(gesture));
	});
	raylibModule.SetValue("IsGestureDetected", WithNote(i,
		"Check if gesture have been detected"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGestureDetected());
	});
	raylibModule.SetValue("GetGestureDetected", WithNote(i,
		"Get latest detected gesture"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGestureHoldDuration());
	});
	raylibModule.SetValue("GetGestureHoldDuration", WithNote(i,
		"Hold time measured in seconds"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 dragVector = GetGestureDragVector();
		ValueDict dragMap;
		dragMap.SetValue(CACHED_KEY("x"), Value(dragVector.x));
		dragMap.SetValue(CACHED_KEY("y"), Value(dragVector.y));
		return IntrinsicResult(DynamicMap(dragMap));
	});
	raylibModule.SetValue("GetGestureDragVector", WithNote(i,
		"Get drag vector (between initial touch point to current)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGestureDragAngle());
	});
	raylibModule.SetValue("GetGestureDragAngle", WithNote(i,
		"Get drag angle NOTE: Angle in degrees, horizontal-right is 0, counterclockwise"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 pinchVector = GetGesturePinchVector();
		ValueDict pinchMap;
		pinchMap.SetValue(CACHED_KEY("x"), Value(pinchVector.x));
		pinchMap.SetValue(CACHED_KEY("y"), Value(pinchVector.y));
		return IntrinsicResult(DynamicMap(pinchMap));
	});
	raylibModule.SetValue("GetGesturePinchVector", WithNote(i,
		"Get distance between two pinch points"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGesturePinchAngle());
	});
	raylibModule.SetValue("GetGesturePinchAngle", WithNote(i,
		"Get angle between two pinch points NOTE: Angle in degrees, horizontal-right is 0, "
		"counterclockwise"));

	// 2D rendering mode functions

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		ValueDict cameraMap = context.GetArg(0).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(CACHED_KEY("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(CACHED_KEY("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(CACHED_KEY("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(CACHED_KEY("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(CACHED_KEY("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(CACHED_KEY("zoom"), Value::one).FloatValue();
		profiler::TimedScope timed("raylib.BeginMode2D");
		BeginMode2D(camera);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginMode2D", WithNote(i,
		"Initialize 2D mode with custom camera (2D)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		profiler::TimedScope timed("raylib.EndMode2D");
		EndMode2D();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndMode2D", WithNote(i,
		"End 2D mode with custom camera"));

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		ValueDict cameraMap = context.GetArg(0).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(CACHED_KEY("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(CACHED_KEY("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(CACHED_KEY("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(CACHED_KEY("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(CACHED_KEY("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(CACHED_KEY("zoom"), Value::one).FloatValue();

		return IntrinsicResult(MatrixToValue(GetCameraMatrix2D(camera)));
	});
	raylibModule.SetValue("GetCameraMatrix2D", WithNote(i,
		"Get camera 2d transform matrix"));

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		ValueDict cameraMap = context.GetArg(1).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(CACHED_KEY("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(CACHED_KEY("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(CACHED_KEY("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(CACHED_KEY("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(CACHED_KEY("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(CACHED_KEY("zoom"), Value::one).FloatValue();

		Vector2 result = GetWorldToScreen2D(position, camera);
		ValueDict resultMap;
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetWorldToScreen2D", WithNote(i,
		"Get screen space position for a 2d camera world space position"));

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("camera");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		ValueDict cameraMap = context.GetArg(1).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(CACHED_KEY("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(CACHED_KEY("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(CACHED_KEY("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(CACHED_KEY("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(CACHED_KEY("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(CACHED_KEY("zoom"), Value::one).FloatValue();

		Vector2 result = GetScreenToWorld2D(position, camera);
		ValueDict resultMap;
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetScreenToWorld2D", WithNote(i,
		"Get world space position for a 2d camera screen space position"));

	// Blend mode functions

	i = Intrinsic::Create("");
	i.AddParam("mode");
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		profiler::TimedScope timed("raylib.BeginBlendMode");
		BeginBlendMode(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginBlendMode", WithNote(i,
		"Begin blending mode (alpha, additive, multiplied, subtract, custom) NOTE: Blend modes "
		"supported are enumerated in BlendMode enum"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		profiler::TimedScope timed("raylib.EndBlendMode");
		EndBlendMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndBlendMode", WithNote(i,
		"End blending mode (reset to default: alpha blending)"));

	// Scissor mode functions

	i = Intrinsic::Create("");
	i.AddParam("x");
	i.AddParam("y");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		profiler::TimedScope timed("raylib.BeginScissorMode");
		BeginScissorMode(x, y, width, height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginScissorMode", WithNote(i,
		"Begin scissor mode (define screen area for following drawing) NOTE: Scissor rec refers to "
		"bottom-left corner, changing it to upper-left"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		profiler::TimedScope timed("raylib.EndScissorMode");
		EndScissorMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndScissorMode", WithNote(i,
		"End scissor mode"));

	// VR stereo functions

	i = Intrinsic::Create("");
	i.AddParam("config");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("BeginVrStereoMode");
		return IntrinsicResult::Null;
	#endif
		VrStereoConfig config = ValueToVrStereoConfig(context.GetArg(0));
		BeginVrStereoMode(config);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginVrStereoMode", WithNote(i,
		"Begin VR drawing configuration"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("EndVrStereoMode");
		return IntrinsicResult::Null;
	#endif
		EndVrStereoMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndVrStereoMode", WithNote(i,
		"End VR drawing process (and desktop mirror)"));

	i = Intrinsic::Create("");
	i.AddParam("device");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadVrStereoConfig");
		return IntrinsicResult::Null;
	#endif
		VrDeviceInfo device = ValueToVrDeviceInfo(context.GetArg(0));
		VrStereoConfig config = LoadVrStereoConfig(device);
		return IntrinsicResult(VrStereoConfigToValue(config));
	});
	raylibModule.SetValue("LoadVrStereoConfig", WithNote(i,
		"Load VR stereo config for VR simulator device parameters"));

	i = Intrinsic::Create("");
	i.AddParam("config");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("UnloadVrStereoConfig");
		return IntrinsicResult::Null;
	#endif
		VrStereoConfig config = ValueToVrStereoConfig(context.GetArg(0));
		UnloadVrStereoConfig(config);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadVrStereoConfig", WithNote(i,
		"Unload VR stereo config properties"));

	// Utility functions

	i = Intrinsic::Create("");
	i.AddParam("url");
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("OpenURL")) return IntrinsicResult::Null;
		String url = context.GetArg(0).ToString();
		OpenURL(url.c_str());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("OpenURL", WithNote(i,
		"Open a URL in the default browser.  Refused (does nothing) once sandboxed."));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		SetClipboardText(text.c_str());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetClipboardText", WithNote(i,
		"Set clipboard text content"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(GetClipboardText());
	});
	raylibModule.SetValue("GetClipboardText", WithNote(i,
		"Get the clipboard's text content"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("EnableEventWaiting");
		return IntrinsicResult::Null;
	#endif
		EnableEventWaiting();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EnableEventWaiting", WithNote(i,
		"Enable waiting for events on EndDrawing(), no automatic event polling"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("DisableEventWaiting");
		return IntrinsicResult::Null;
	#endif
		DisableEventWaiting();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DisableEventWaiting", WithNote(i,
		"Disable waiting for events on EndDrawing(), automatic events polling"));

#if !defined(PLATFORM_WEB)
	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = GetClipboardImage();
		rcImage++;
		return IntrinsicResult(ImageToValue(image));
	});
	raylibModule.SetValue("GetClipboardImage", WithNote(i,
		"Get clipboard image"));
#endif

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("ext");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileName = context.GetArg(0).ToString();
		String ext = context.GetArg(1).ToString();
		return IntrinsicResult(IsFileExtension(fileName.c_str(), ext.c_str()));
	});
	raylibModule.SetValue("IsFileExtension", WithNote(i,
		"Check file extension"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("TakeScreenshot")) return IntrinsicResult::Null;
		String fileName = context.GetArg(0).ToString();
		TakeScreenshot(fileName.c_str());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("TakeScreenshot", WithNote(i,
		"Takes a screenshot of current screen"));

	i = Intrinsic::Create("");
	i.AddParam("flags");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int flags = (unsigned int)context.GetArg(0).IntValue();
		SetConfigFlags(flags);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetConfigFlags", WithNote(i,
		"Set up window configuration flags (view FLAGS) NOTE: This function is expected to be "
		"called before window creation, because it sets up some flags for the window creation "
		"process To configure window states after creation, use SetWindowState()"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize");
	i.set_Code(INTRINSIC_LAMBDA {
		Value dataVal = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();

		const unsigned char* bytes = nullptr;
		String tempStr;

		if (dataVal.Type() == ValueType::String) {
			tempStr = dataVal.ToString();
			bytes = (const unsigned char*)tempStr.c_str();
			if (dataSize <= 0) dataSize = tempStr.LengthB();
		} else if (dataVal.Type() == ValueType::Map) {
			BinaryData* rawData = ValueToRawData(dataVal);
			if (rawData != nullptr) {
				bytes = rawData->bytes;
				if (dataSize <= 0) dataSize = rawData->length;
			}
		}

		if (bytes == nullptr || dataSize <= 0) {
			return IntrinsicResult(String());
		}

		int outputSize = 0;
		char* encoded = EncodeDataBase64(bytes, dataSize, &outputSize);
		String result(encoded);
		if (encoded != nullptr) MemFree(encoded);
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("EncodeDataBase64", WithNote(i,
		"Encode data to Base64 string NOTE: Returned string includes NULL terminator, considered "
		"on outputSize"));

	i = Intrinsic::Create("");
	i.AddParam("seconds", 1.0);
	i.set_Code(INTRINSIC_LAMBDA {
		double seconds = context.GetArg(0).DoubleValue();
		profiler::TimedScope timed("raylib.WaitTime");
		WaitTime(seconds);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("WaitTime", WithNote(i,
		"Wait for some time (stop program execution) NOTE: Sleep() granularity could be around 10 "
		"ms, it means, Sleep() could take longer than expected... for that reason a busy wait loop "
		"is used REF: "
		"http://stackoverflow.com/questions/43057578/c-programming-win32-games-sleep-taking-longer-than-expected "
		"REF: http://www.geisswerks.com/ryan/FAQS/timing.html --> All about timing on Win32!"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SwapScreenBuffer");
		return IntrinsicResult::Null;
	#endif
		profiler::TimedScope timed("raylib.SwapScreenBuffer");
		SwapScreenBuffer();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SwapScreenBuffer", WithNote(i,
		"Swap back buffer with front buffer (screen drawing)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("PollInputEvents");
		return IntrinsicResult::Null;
	#endif
		PollInputEvents();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PollInputEvents", WithNote(i,
		"Register all input events"));

	// Load text files
	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		// Hold the String in a named local: ToString() returns a temporary whose
		// c_str() would otherwise dangle before LoadFileText reads it.
		String fileName;
		if (!fs::HostPath(context.GetArg(0), fileName)) return IntrinsicResult::Null;
		char *text = LoadFileText(fileName.c_str());
		if (text == nullptr) return IntrinsicResult::Null;
		String ret(text);
		UnloadFileText(text);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("LoadFileText", WithNote(i,
		"Load text data from file, returns a '\\0' terminated string NOTE: text chars array should "
		"be freed manually"));

	// File system management

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		// (On web, raylib reaches this through the fetch hook installed by
		// InstallLoadFileHooks, as LoadFileText does.)
		String fileName;
		if (!fs::HostPath(context.GetArg(0), fileName)) return IntrinsicResult::Null;
		int dataSize = 0;
		unsigned char* data = LoadFileData(fileName.c_str(), &dataSize);
		if (data == nullptr || dataSize <= 0) return IntrinsicResult::Null;

		Value result = BytesToRawDataValue(data, dataSize);
		UnloadFileData(data);
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("LoadFileData", WithNote(i,
		"Load a file's contents as a RawData, or null if it can't be read"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("UnloadFileData");
		return IntrinsicResult::Null;
	#endif
		Value dataValue = context.GetArg(0);
		BinaryData* rawData = ValueToRawData(dataValue);
		if (rawData == nullptr || rawData->bytes == nullptr) return IntrinsicResult::Null;

		UnloadFileData(rawData->bytes);
		rawData->bytes = nullptr;
		rawData->length = 0;
		rawData->ownsBuffer = false;

		if (dataValue.Type() == ValueType::Map) {
			ValueDict map = dataValue.GetDict();
			map.SetValue(CACHED_KEY("_handle"), Value::Null);   // a handle Value now, not a number
		}

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadFileData", WithNote(i,
		"Unload file data allocated by LoadFileData()"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SaveFileData");
		return IntrinsicResult::Zero;
	#endif
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(0), fileName)) return IntrinsicResult::Zero;
		Value dataValue = context.GetArg(1);
		int dataSize = context.GetArg(2).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Zero;

		return IntrinsicResult(SaveFileData(fileName.c_str(), (void*)bytes, byteCount));
	});
	raylibModule.SetValue("SaveFileData", WithNote(i,
		"Save data to file from buffer"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ExportDataAsCode");
		return IntrinsicResult::Zero;
	#endif
		Value dataValue = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(2), fileName)) return IntrinsicResult::Zero;

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Zero;

		return IntrinsicResult(ExportDataAsCode(bytes, byteCount, fileName.c_str()));
	});
	raylibModule.SetValue("ExportDataAsCode", WithNote(i,
		"Export data to code (.h), returns true on success"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		// LoadFileText already returns a MiniScript String, so explicit unload is a no-op.
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadFileText", WithNote(i,
		"Unload file text data allocated by LoadFileText()"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SaveFileText");
		return IntrinsicResult::Zero;
	#endif
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(0), fileName)) return IntrinsicResult::Zero;
		String text = context.GetArg(1).ToString();
		return IntrinsicResult(SaveFileText(fileName.c_str(), text.c_str()));
	});
	raylibModule.SetValue("SaveFileText", WithNote(i,
		"Save text data to file (write), string must be '\\0' terminated"));

	i = Intrinsic::Create("");
	i.AddParam("callback", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("SetLoadFileDataCallback")) return IntrinsicResult::Null;
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetLoadFileDataCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context.GetArg(0);
		if (callback.IsNull()) {
			g_callbackBridgeState.loadFileDataCallback = Value::Null;
			SetLoadFileDataCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetLoadFileDataCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = context.vm.GetInterpreter();
		if (IsNull(g_callbackBridgeState.interpreter)) {
			TraceLog(LOG_ERROR, "SetLoadFileDataCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.loadFileDataCallback = callback;
		SetLoadFileDataCallback(MiniScriptLoadFileDataBridge);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetLoadFileDataCallback", WithNote(i,
		"Set custom file binary data loader"));

	i = Intrinsic::Create("");
	i.AddParam("callback", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("SetSaveFileDataCallback")) return IntrinsicResult::Null;
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetSaveFileDataCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context.GetArg(0);
		if (callback.IsNull()) {
			g_callbackBridgeState.saveFileDataCallback = Value::Null;
			SetSaveFileDataCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetSaveFileDataCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = context.vm.GetInterpreter();
		if (IsNull(g_callbackBridgeState.interpreter)) {
			TraceLog(LOG_ERROR, "SetSaveFileDataCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.saveFileDataCallback = callback;
		SetSaveFileDataCallback(MiniScriptSaveFileDataBridge);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetSaveFileDataCallback", WithNote(i,
		"Set custom file binary data saver"));

	i = Intrinsic::Create("");
	i.AddParam("callback", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("SetLoadFileTextCallback")) return IntrinsicResult::Null;
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetLoadFileTextCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context.GetArg(0);
		if (callback.IsNull()) {
			g_callbackBridgeState.loadFileTextCallback = Value::Null;
			SetLoadFileTextCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetLoadFileTextCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = context.vm.GetInterpreter();
		if (IsNull(g_callbackBridgeState.interpreter)) {
			TraceLog(LOG_ERROR, "SetLoadFileTextCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.loadFileTextCallback = callback;
		SetLoadFileTextCallback(MiniScriptLoadFileTextBridge);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetLoadFileTextCallback", WithNote(i,
		"Set custom file text data loader"));

	i = Intrinsic::Create("");
	i.AddParam("callback", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("SetSaveFileTextCallback")) return IntrinsicResult::Null;
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetSaveFileTextCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context.GetArg(0);
		if (callback.IsNull()) {
			g_callbackBridgeState.saveFileTextCallback = Value::Null;
			SetSaveFileTextCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetSaveFileTextCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = context.vm.GetInterpreter();
		if (IsNull(g_callbackBridgeState.interpreter)) {
			TraceLog(LOG_ERROR, "SetSaveFileTextCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.saveFileTextCallback = callback;
		SetSaveFileTextCallback(MiniScriptSaveFileTextBridge);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetSaveFileTextCallback", WithNote(i,
		"Set custom file text data saver"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("fileRename");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileRename");
		return IntrinsicResult(-1);
	#endif
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(0), fileName)) return IntrinsicResult(-1);
		String fileRename;
		if (!fs::HostPathForWrite(context.GetArg(1), fileRename)) return IntrinsicResult(-1);
		return IntrinsicResult(FileRename(fileName.c_str(), fileRename.c_str()));
	});
	raylibModule.SetValue("FileRename", WithNote(i,
		"Rename file (if exists) NOTE: Only rename file name required, not full path"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileRemove");
		return IntrinsicResult(-1);
	#endif
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(0), fileName)) return IntrinsicResult(-1);
		return IntrinsicResult(FileRemove(fileName.c_str()));
	});
	raylibModule.SetValue("FileRemove", WithNote(i,
		"Remove file (if exists)"));

	i = Intrinsic::Create("");
	i.AddParam("srcPath");
	i.AddParam("dstPath");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileCopy");
		return IntrinsicResult(-1);
	#endif
		String srcPath;
		if (!fs::HostPath(context.GetArg(0), srcPath)) return IntrinsicResult(-1);
		String dstPath;
		if (!fs::HostPathForWrite(context.GetArg(1), dstPath)) return IntrinsicResult(-1);
		return IntrinsicResult(FileCopy(srcPath.c_str(), dstPath.c_str()));
	});
	raylibModule.SetValue("FileCopy", WithNote(i,
		"Copy file from one path to another NOTE: If destination path does not exist, it is "
		"created!"));

	i = Intrinsic::Create("");
	i.AddParam("srcPath");
	i.AddParam("dstPath");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileMove");
		return IntrinsicResult(-1);
	#endif
		String srcPath;
		if (!fs::HostPathForWrite(context.GetArg(0), srcPath)) return IntrinsicResult(-1);
		String dstPath;
		if (!fs::HostPathForWrite(context.GetArg(1), dstPath)) return IntrinsicResult(-1);
		return IntrinsicResult(FileMove(srcPath.c_str(), dstPath.c_str()));
	});
	raylibModule.SetValue("FileMove", WithNote(i,
		"Move file from one directory to another NOTE: If dst directories do not exists they are "
		"created"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("search");
	i.AddParam("replacement");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileTextReplace");
		return IntrinsicResult(-1);
	#endif
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(0), fileName)) return IntrinsicResult(-1);
		String search = context.GetArg(1).ToString();
		String replacement = context.GetArg(2).ToString();
		return IntrinsicResult(FileTextReplace(fileName.c_str(), search.c_str(), replacement.c_str()));
	});
	raylibModule.SetValue("FileTextReplace", WithNote(i,
		"Replace text in an existing file WARNING: DEPENDENCY: [rtext] module"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("search");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileTextFindIndex");
		return IntrinsicResult(-1);
	#endif
		String fileName;
		if (!fs::HostPath(context.GetArg(0), fileName)) return IntrinsicResult(-1);
		String search = context.GetArg(1).ToString();
		return IntrinsicResult(FileTextFindIndex(fileName.c_str(), search.c_str()));
	});
	raylibModule.SetValue("FileTextFindIndex", WithNote(i,
		"Find text index position in existing file WARNING: DEPENDENCY: [rtext] module"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		// (On web this sees the in-memory file system, i.e. a preloaded file package.)
		String fileName;
		if (!fs::HostPath(context.GetArg(0), fileName)) return IntrinsicResult::Zero;
		return IntrinsicResult(FileExists(fileName.c_str()));
	});
	raylibModule.SetValue("FileExists", WithNote(i,
		"Check if the file exists"));

	i = Intrinsic::Create("");
	i.AddParam("dirPath");
	i.set_Code(INTRINSIC_LAMBDA {
		// (On web this sees the in-memory file system, i.e. a preloaded file package.)
		String dirPath;
		if (!fs::HostPath(context.GetArg(0), dirPath)) return IntrinsicResult::Zero;
		return IntrinsicResult(DirectoryExists(dirPath.c_str()));
	});
	raylibModule.SetValue("DirectoryExists", WithNote(i,
		"Check if directory path exists"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetFileLength");
		return IntrinsicResult::Zero;
	#endif
		String fileName;
		if (!fs::HostPath(context.GetArg(0), fileName)) return IntrinsicResult::Zero;
		return IntrinsicResult(GetFileLength(fileName.c_str()));
	});
	raylibModule.SetValue("GetFileLength", WithNote(i,
		"Get file length in bytes NOTE: GetFileSize() conflicts with windows.h"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetFileModTime");
		return IntrinsicResult::Zero;
	#endif
		String fileName;
		if (!fs::HostPath(context.GetArg(0), fileName)) return IntrinsicResult::Zero;
		return IntrinsicResult((double)GetFileModTime(fileName.c_str()));
	});
	raylibModule.SetValue("GetFileModTime", WithNote(i,
		"Get file modification time (last write time)"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileName = context.GetArg(0).ToString();
		return IntrinsicResult(String(GetFileExtension(fileName.c_str())));
	});
	raylibModule.SetValue("GetFileExtension", WithNote(i,
		"Get the extension of a file name, including the dot (e.g. \".png\"), or \"\" if it has "
		"none"));

	i = Intrinsic::Create("");
	i.AddParam("filePath");
	i.set_Code(INTRINSIC_LAMBDA {
		String filePath = context.GetArg(0).ToString();
		return IntrinsicResult(String(GetFileName(filePath.c_str())));
	});
	raylibModule.SetValue("GetFileName", WithNote(i,
		"Get the file name at the end of a path"));

	i = Intrinsic::Create("");
	i.AddParam("filePath");
	i.set_Code(INTRINSIC_LAMBDA {
		String filePath = context.GetArg(0).ToString();
		return IntrinsicResult(String(GetFileNameWithoutExt(filePath.c_str())));
	});
	raylibModule.SetValue("GetFileNameWithoutExt", WithNote(i,
		"Get the file name at the end of a path, without its last extension"));

	i = Intrinsic::Create("");
	i.AddParam("filePath");
	i.set_Code(INTRINSIC_LAMBDA {
		String filePath = context.GetArg(0).ToString();
		return IntrinsicResult(String(GetDirectoryPath(filePath.c_str())));
	});
	raylibModule.SetValue("GetDirectoryPath", WithNote(i,
		"Get the directory part of a file path"));

	i = Intrinsic::Create("");
	i.AddParam("dirPath");
	i.set_Code(INTRINSIC_LAMBDA {
		String dirPath = context.GetArg(0).ToString();
		return IntrinsicResult(String(GetPrevDirectoryPath(dirPath.c_str())));
	});
	raylibModule.SetValue("GetPrevDirectoryPath", WithNote(i,
		"Get the parent of a directory path"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("GetWorkingDirectory")) return IntrinsicResult(String());
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWorkingDirectory");
		return IntrinsicResult(String());
	#endif
		return IntrinsicResult(String(GetWorkingDirectory()));
	});
	raylibModule.SetValue("GetWorkingDirectory", WithNote(i,
		"Get the current working directory (\"\" once sandboxed, and on web)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("GetApplicationDirectory")) return IntrinsicResult(String());
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetApplicationDirectory");
		return IntrinsicResult(String());
	#endif
		return IntrinsicResult(String(GetApplicationDirectory()));
	});
	raylibModule.SetValue("GetApplicationDirectory", WithNote(i,
		"Get the directory of the running application (\"\" once sandboxed, and on web)"));

	i = Intrinsic::Create("");
	i.AddParam("dirPath");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("MakeDirectory");
		return IntrinsicResult(-1);
	#endif
		String dirPath;
		if (!fs::HostPathForWrite(context.GetArg(0), dirPath)) return IntrinsicResult(-1);
		return IntrinsicResult(MakeDirectory(dirPath.c_str()));
	});
	raylibModule.SetValue("MakeDirectory", WithNote(i,
		"Create directories (including full path requested), returns 0 on success"));

	i = Intrinsic::Create("");
	i.AddParam("dirPath");
	i.set_Code(INTRINSIC_LAMBDA {
		// Returns 0 on success (raylib 6.2 convention), nonzero on failure.
		if (fs::RefuseWhenSandboxed("ChangeDirectory")) return IntrinsicResult(-1);
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ChangeDirectory");
		return IntrinsicResult(-1);
	#endif
		String dirPath = context.GetArg(0).ToString();
		return IntrinsicResult(ChangeDirectory(dirPath.c_str()));
	});
	raylibModule.SetValue("ChangeDirectory", WithNote(i,
		"Change working directory, returns 0 on success (nonzero on failure)"));

	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsPathFile");
		return IntrinsicResult::Zero;
	#endif
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Zero;
		return IntrinsicResult(IsPathFile(path.c_str()));
	});
	raylibModule.SetValue("IsPathFile", WithNote(i,
		"Check if provided path point to a file"));

	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsPathDirectory");
		return IntrinsicResult::Zero;
	#endif
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Zero;
		// raylib 6.2's IsPathDirectory returns true for any path that is not a
		// regular file, including paths that don't exist; require existence too.
		return IntrinsicResult(IsPathDirectory(path.c_str()) && DirectoryExists(path.c_str()));
	});
	raylibModule.SetValue("IsPathDirectory", WithNote(i,
		"Check if provided path points to an existing directory"));

	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(INTRINSIC_LAMBDA {
		// Pure string check on the path as given; no host-path translation.
		String path = context.GetArg(0).ToString();
		return IntrinsicResult(IsPathAbsolute(path.c_str()));
	});
	raylibModule.SetValue("IsPathAbsolute", WithNote(i,
		"Check if provided path is an absolute path"));

	i = Intrinsic::Create("");
	i.AddParam("filePath");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsFileHidden");
		return IntrinsicResult::Zero;
	#endif
		String filePath;
		if (!fs::HostPath(context.GetArg(0), filePath)) return IntrinsicResult::Zero;
		return IntrinsicResult(IsFileHidden(filePath.c_str()));
	});
	raylibModule.SetValue("IsFileHidden", WithNote(i,
		"Check if file path (file or directory) is hidden by OS"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileName = context.GetArg(0).ToString();
		return IntrinsicResult(IsFileNameValid(fileName.c_str()));
	});
	raylibModule.SetValue("IsFileNameValid", WithNote(i,
		"Check if fileName is valid for the platform/OS"));

	i = Intrinsic::Create("");
	i.AddParam("dirPath");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadDirectoryFiles");
		return IntrinsicResult(Value::make_list(0));
	#endif
		String dirPath = context.GetArg(0).ToString();
		if (fs::IsSandboxed()) {
			// raylib's own version returns real host paths, which must never
			// reach script -- so sandboxed we list through fs and hand back
			// virtual ones.  This is also the only listing that can see /hw.
			String virtualDir;
			ValueList result;
			std::vector<String> names;
			if (fs::ResolvePath(dirPath, virtualDir) && fs::ListDir(virtualDir, names)) {
				for (size_t n = 0; n < names.size(); n++) {
					result.Add(fs::PathCombine(virtualDir, names[n]));
				}
			}
			return IntrinsicResult(DynamicList(result));
		}
		FilePathList files = LoadDirectoryFiles(dirPath.c_str());

		ValueList result;
		for (unsigned int n = 0; n < files.count; n++) result.Add(Value(String(files.paths[n])));
		UnloadDirectoryFiles(files);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadDirectoryFiles", WithNote(i,
		"Load directory filepaths NOTE: Base path is prepended to the scanned filepaths WARNING: "
		"Directory is scanned twice, first time to get paths count Scanneed files and directories, "
		"no recursive/subdirs scanning"));

	i = Intrinsic::Create("");
	i.AddParam("basePath");
	i.AddParam("filter");
	i.AddParam("scanSubdirs", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("LoadDirectoryFilesEx")) return IntrinsicResult(Value::make_list(0));
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadDirectoryFilesEx");
		return IntrinsicResult(Value::make_list(0));
	#endif
		String basePath = context.GetArg(0).ToString();
		String filter = context.GetArg(1).ToString();
		bool scanSubdirs = context.GetArg(2).IntValue() != 0;

		FilePathList files = LoadDirectoryFilesEx(basePath.c_str(), filter.c_str(), scanSubdirs);
		ValueList result;
		for (unsigned int n = 0; n < files.count; n++) result.Add(Value(String(files.paths[n])));
		UnloadDirectoryFiles(files);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadDirectoryFilesEx", WithNote(i,
		"Load directory filepaths with extension filtering and recursive directory scan Use "
		"\"*.*\" to include all files and directories on scan Use \"FILES*\" to include only files "
		"on scan Use \"DIRS*\" to include only directories on scan WARNING: Directory is scanned "
		"twice, first time to get paths count"));

	i = Intrinsic::Create("");
	i.AddParam("files");
	i.set_Code(INTRINSIC_LAMBDA {
		// LoadDirectoryFiles* wrappers return plain MiniScript lists and release native memory internally.
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadDirectoryFiles", WithNote(i,
		"Unload directory filepaths WARNING: files.count is not reseted to 0 after unloading"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsFileDropped");
		return IntrinsicResult::Zero;
	#endif
		return IntrinsicResult(IsFileDropped());
	});
	raylibModule.SetValue("IsFileDropped", WithNote(i,
		"Check if file has been dropped into window"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("LoadDroppedFiles")) return IntrinsicResult(Value::make_list(0));
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadDroppedFiles");
		return IntrinsicResult(Value::make_list(0));
	#endif
		FilePathList files = LoadDroppedFiles();
		ValueList result;
		for (unsigned int n = 0; n < files.count; n++) result.Add(Value(String(files.paths[n])));
		UnloadDroppedFiles(files);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadDroppedFiles", WithNote(i,
		"Load dropped filepaths"));

	i = Intrinsic::Create("");
	i.AddParam("files");
	i.set_Code(INTRINSIC_LAMBDA {
		// LoadDroppedFiles wrapper returns a plain MiniScript list and releases native memory internally.
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadDroppedFiles", WithNote(i,
		"Unload dropped filepaths"));

	i = Intrinsic::Create("");
	i.AddParam("dirPath");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetDirectoryFileCount");
		return IntrinsicResult::Zero;
	#endif
		String dirPath = context.GetArg(0).ToString();
		if (fs::IsSandboxed()) {
			String virtualDir;
			std::vector<String> names;
			if (!fs::ResolvePath(dirPath, virtualDir)) return IntrinsicResult::Zero;
			if (!fs::ListDir(virtualDir, names)) return IntrinsicResult::Zero;
			return IntrinsicResult((int)names.size());
		}
		return IntrinsicResult((int)GetDirectoryFileCount(dirPath.c_str()));
	});
	raylibModule.SetValue("GetDirectoryFileCount", WithNote(i,
		"Get the number of files in a directory (0 on web)"));

	i = Intrinsic::Create("");
	i.AddParam("basePath");
	i.AddParam("filter");
	i.AddParam("scanSubdirs", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		if (fs::RefuseWhenSandboxed("GetDirectoryFileCountEx")) return IntrinsicResult::Zero;
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetDirectoryFileCountEx");
		return IntrinsicResult::Zero;
	#endif
		String basePath = context.GetArg(0).ToString();
		String filter = context.GetArg(1).ToString();
		bool scanSubdirs = context.GetArg(2).IntValue() != 0;
		return IntrinsicResult((int)GetDirectoryFileCountEx(basePath.c_str(), filter.c_str(), scanSubdirs));
	});
	raylibModule.SetValue("GetDirectoryFileCountEx", WithNote(i,
		"Count the files in a directory with the given extensions (e.g. \".png;.jpg\"; include "
		"\"DIR\" to count directories too), optionally searching subdirectories.  Returns 0 once "
		"sandboxed, and on web"));

	// Compression and encoding helpers

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value dataValue = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		int compSize = 0;
		unsigned char* compressed = CompressData(bytes, byteCount, &compSize);
		if (compressed == nullptr || compSize <= 0) return IntrinsicResult::Null;

		Value result = BytesToRawDataValue(compressed, compSize);
		MemFree(compressed);
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("CompressData", WithNote(i,
		"Compress data with DEFLATE: a RawData, a string, or a list of byte values; dataSize, if "
		"given, uses only that many bytes.  Returns a RawData, or null"));

	i = Intrinsic::Create("");
	i.AddParam("compData");
	i.AddParam("compDataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value compDataValue = context.GetArg(0);
		int compDataSize = context.GetArg(1).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(compDataValue, compDataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		int outSize = 0;
		unsigned char* decompressed = DecompressData(bytes, byteCount, &outSize);
		if (decompressed == nullptr || outSize <= 0) return IntrinsicResult::Null;

		Value result = BytesToRawDataValue(decompressed, outSize);
		MemFree(decompressed);
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("DecompressData", WithNote(i,
		"Decompress DEFLATE data (as from CompressData): a RawData, a string, or a list of byte "
		"values; compDataSize, if given, uses only that many bytes.  Returns a RawData, or null"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int outputSize = 0;
		unsigned char* decoded = DecodeDataBase64(text.c_str(), &outputSize);
		if (decoded == nullptr || outputSize <= 0) return IntrinsicResult::Null;

		Value result = BytesToRawDataValue(decoded, outputSize);
		MemFree(decoded);
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("DecodeDataBase64", WithNote(i,
		"Decode a Base64 string into a RawData, or null"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value dataValue = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Zero;

		return IntrinsicResult((int)ComputeCRC32((unsigned char*)bytes, byteCount));
	});
	raylibModule.SetValue("ComputeCRC32", WithNote(i,
		"Compute the CRC32 checksum of data: a RawData, a string, or a list of byte values; "
		"dataSize, if given, uses only that many bytes"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value dataValue = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		unsigned int* hash = ComputeMD5((unsigned char*)bytes, byteCount);
		if (hash == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < 4; n++) result.Add(Value((double)hash[n]));
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("ComputeMD5", WithNote(i,
		"Compute the MD5 hash of data (as for ComputeCRC32), as a list of four 32-bit integers"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value dataValue = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		unsigned int* hash = ComputeSHA1((unsigned char*)bytes, byteCount);
		if (hash == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < 5; n++) result.Add(Value((double)hash[n]));
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("ComputeSHA1", WithNote(i,
		"Compute the SHA-1 hash of data (as for ComputeCRC32), as a list of five 32-bit integers"));

	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Value dataValue = context.GetArg(0);
		int dataSize = context.GetArg(1).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		unsigned int* hash = ComputeSHA256((unsigned char*)bytes, byteCount);
		if (hash == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < 8; n++) result.Add(Value((double)hash[n]));
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("ComputeSHA256", WithNote(i,
		"Compute the SHA-256 hash of data (as for ComputeCRC32), as a list of eight 32-bit integers"));

	// Automation events functions

	i = Intrinsic::Create("");
	i.AddParam("fileName", String());
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadAutomationEventList");
		return IntrinsicResult::Null;
	#endif
		// An empty name means "start a new empty list", so only resolve a name
		// that was actually given.
		String nameArg = context.GetArg(0).ToString();
		String fileName;
		if (nameArg.LengthB() > 0 && !fs::HostPath(nameArg, fileName)) return IntrinsicResult::Null;
		const char* fileNamePtr = fileName.LengthB() > 0 ? fileName.c_str() : nullptr;
		AutomationEventList list = LoadAutomationEventList(fileNamePtr);
		return IntrinsicResult(AutomationEventListToValue(list));
	});
	raylibModule.SetValue("LoadAutomationEventList", WithNote(i,
		"Load automation events list from file, NULL for empty list, capacity = "
		"MAX_AUTOMATION_EVENTS"));

	i = Intrinsic::Create("");
	i.AddParam("list");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("UnloadAutomationEventList");
		return IntrinsicResult::Null;
	#endif
		Value listValue = context.GetArg(0);
		AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
		if (listPtr == nullptr) return IntrinsicResult::Null;
		if (listPtr == activeAutomationEventListPtr) SetActiveAutomationEventList(Value::Null, nullptr);
		AutomationEventList list;
		TakeResource(listValue, &list);

		UnloadAutomationEventList(list);

		if (listValue.Type() == ValueType::Map) {
			ValueDict map = listValue.GetDict();
			map.SetValue(CACHED_KEY("capacity"), Value::zero);
			map.SetValue(CACHED_KEY("count"), Value::zero);
			map.SetValue(CACHED_KEY("events"), Value::make_list(0));
		}

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadAutomationEventList", WithNote(i,
		"Unload automation events list from file"));

	i = Intrinsic::Create("");
	i.AddParam("list");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ExportAutomationEventList");
		return IntrinsicResult::Zero;
	#endif
		Value listValue = context.GetArg(0);
		String fileName;
		if (!fs::HostPathForWrite(context.GetArg(1), fileName)) return IntrinsicResult::Zero;

		AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
		if (listPtr != nullptr) return IntrinsicResult(ExportAutomationEventList(*listPtr, fileName.c_str()));

		if (listValue.Type() != ValueType::Map) return IntrinsicResult::Zero;

		ValueDict map = listValue.GetDict();
		Value eventsValue = map.Lookup(CACHED_KEY("events"), Value::Null);
		if (eventsValue.Type() != ValueType::List) return IntrinsicResult::Zero;

		ValueList eventsList = eventsValue.GetList();
		std::vector<AutomationEvent> events;
		events.reserve(eventsList.Count());
		for (int n = 0; n < eventsList.Count(); n++) events.push_back(ValueToAutomationEvent(eventsList[n]));

		AutomationEventList list = {0};
		list.count = (unsigned int)events.size();
		list.capacity = (unsigned int)events.size();
		list.events = events.empty() ? nullptr : events.data();
		return IntrinsicResult(ExportAutomationEventList(list, fileName.c_str()));
	});
	raylibModule.SetValue("ExportAutomationEventList", WithNote(i,
		"Export automation events list as text file"));

	i = Intrinsic::Create("");
	i.AddParam("list", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetAutomationEventList");
		return IntrinsicResult::Null;
	#endif
		Value listValue = context.GetArg(0);
		if (listValue.IsNull()) {
			SetActiveAutomationEventList(Value::Null, nullptr);
			return IntrinsicResult::Null;
		}

		AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
		if (listPtr == nullptr) return IntrinsicResult::Null;

		SetActiveAutomationEventList(listValue, listPtr);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetAutomationEventList", WithNote(i,
		"Setup automation event list to record to"));

	i = Intrinsic::Create("");
	i.AddParam("frame");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetAutomationEventBaseFrame");
		return IntrinsicResult::Null;
	#endif
		int frame = context.GetArg(0).IntValue();
		SetAutomationEventBaseFrame(frame);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetAutomationEventBaseFrame", WithNote(i,
		"Set automation event internal base frame to start recording"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("StartAutomationEventRecording");
		return IntrinsicResult::Null;
	#endif
		StartAutomationEventRecording();
		automationEventRecording = true;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("StartAutomationEventRecording", WithNote(i,
		"Start recording automation events (AutomationEventList must be set)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("StopAutomationEventRecording");
		return IntrinsicResult::Null;
	#endif
		StopAutomationEventRecording();
		automationEventRecording = false;
		SyncAutomationEventListValue(activeAutomationEventListMap);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("StopAutomationEventRecording", WithNote(i,
		"Stop recording automation events"));

	i = Intrinsic::Create("");
	i.AddParam("event");
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("PlayAutomationEvent");
		return IntrinsicResult::Null;
	#endif
		AutomationEvent event = ValueToAutomationEvent(context.GetArg(0));
		PlayAutomationEvent(event);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("PlayAutomationEvent", WithNote(i,
		"Play a recorded automation event"));

	// Random number generation
	i = Intrinsic::Create("");
	i.AddParam("seed");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int seed = (unsigned int)context.GetArg(0).IntValue();
		SetRandomSeed(seed);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetRandomSeed", WithNote(i,
		"Set the seed for the random number generator"));

	i = Intrinsic::Create("");
	i.AddParam("min");
	i.AddParam("max");
	i.set_Code(INTRINSIC_LAMBDA {
		int min = context.GetArg(0).IntValue();
		int max = context.GetArg(1).IntValue();
		return IntrinsicResult(GetRandomValue(min, max));
	});
	raylibModule.SetValue("GetRandomValue", WithNote(i,
		"Get a random value between min and max included"));

	i = Intrinsic::Create("");
	i.AddParam("count");
	i.AddParam("min");
	i.AddParam("max");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int count = (unsigned int)context.GetArg(0).IntValue();
		int min = context.GetArg(1).IntValue();
		int max = context.GetArg(2).IntValue();

		int* sequence = LoadRandomSequence(count, min, max);
		if (sequence == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (unsigned int i = 0; i < count; i++) {
			result.Add(Value(sequence[i]));
		}
		UnloadRandomSequence(sequence);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadRandomSequence", WithNote(i,
		"Load random values sequence, no values repeated, min and max included"));

	i = Intrinsic::Create("");
	i.AddParam("sequence", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		// LoadRandomSequence wrapper returns a plain MiniScript list and unloads native memory immediately.
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadRandomSequence", WithNote(i,
		"Unload random values sequence"));

	// Logging and tracing

	i = Intrinsic::Create("");
	i.AddParam("logLevel");
	i.set_Code(INTRINSIC_LAMBDA {
		int logLevel = context.GetArg(0).IntValue();
		SetTraceLogLevel(logLevel);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTraceLogLevel", WithNote(i,
		"Set the lowest level of raylib log message to print: LOG_ALL, LOG_TRACE, LOG_DEBUG, LOG_INFO "
		"(the default), LOG_WARNING, LOG_ERROR, LOG_FATAL or LOG_NONE"));

	i = Intrinsic::Create("");
	i.AddParam("callback", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetTraceLogCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context.GetArg(0);
		if (callback.IsNull()) {
			g_callbackBridgeState.traceLogCallback = Value::Null;
			g_callbackBridgeState.invokingTraceLogCallback = false;
			SetTraceLogCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			fprintf(stderr, "SetTraceLogCallback: callback must be function or null.\n");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = context.vm.GetInterpreter();
		if (IsNull(g_callbackBridgeState.interpreter)) {
			fprintf(stderr, "SetTraceLogCallback: interpreter not available.\n");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.traceLogCallback = callback;
		g_callbackBridgeState.invokingTraceLogCallback = false;
		SetTraceLogCallback(MiniScriptTraceLogBridge);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTraceLogCallback", WithNote(i,
		"Set custom trace log"));

	i = Intrinsic::Create("");
	i.AddParam("logLevel");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		int logLevel = context.GetArg(0).IntValue();
		String text = context.GetArg(1).ToString();
		TraceLog(logLevel, "%s", text.c_str());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("TraceLog", WithNote(i,
		"Show trace log messages (LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG)"));
}
