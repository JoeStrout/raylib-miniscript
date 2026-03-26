//
//  RCore.cpp
//  raylib-miniscript
//
//  Raylib Core module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
#include "rlgl.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

static void SyncCamera3DValue(Value cameraValue, Camera3D camera) {
	if (cameraValue.type != ValueType::Map) return;
	ValueDict map = cameraValue.GetDict();

	map.SetValue(String("position"), Vector3ToValue(camera.position));
	map.SetValue(String("target"), Vector3ToValue(camera.target));
	map.SetValue(String("up"), Vector3ToValue(camera.up));
	map.SetValue(String("fovy"), Value(camera.fovy));
	map.SetValue(String("projection"), Value(camera.projection));

	map.SetValue(String("positionX"), Value(camera.position.x));
	map.SetValue(String("positionY"), Value(camera.position.y));
	map.SetValue(String("positionZ"), Value(camera.position.z));
	map.SetValue(String("targetX"), Value(camera.target.x));
	map.SetValue(String("targetY"), Value(camera.target.y));
	map.SetValue(String("targetZ"), Value(camera.target.z));
	map.SetValue(String("upX"), Value(camera.up.x));
	map.SetValue(String("upY"), Value(camera.up.y));
	map.SetValue(String("upZ"), Value(camera.up.z));
}

static Shader* GetShaderPtr(Value shaderValue) {
	if (shaderValue.type != ValueType::Map) return nullptr;
	ValueDict map = shaderValue.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	return (Shader*)ValueToPointer(handleVal);
}

#ifdef PLATFORM_WEB
static void PrintWebNotSupported(const char* functionName) {
	TraceLog(LOG_ERROR, "%s: Web not supported.", functionName);
}
#endif

static void SyncShaderValue(Value shaderValue, Shader shader) {
	if (shaderValue.type != ValueType::Map) return;
	ValueDict map = shaderValue.GetDict();
	map.SetValue(String("id"), Value((int)shader.id));
}

static bool GetBytesFromValue(Value value, int requestedSize, std::vector<unsigned char>& scratch,
		const unsigned char** outBytes, int* outSize) {
	if (outBytes == nullptr || outSize == nullptr) return false;

	*outBytes = nullptr;
	*outSize = 0;
	scratch.clear();

	if (value.type == ValueType::Map) {
		BinaryData* rawData = ValueToRawData(value);
		if (rawData == nullptr || rawData->bytes == nullptr || rawData->length <= 0) return false;

		*outBytes = rawData->bytes;
		*outSize = rawData->length;
	} else if (value.type == ValueType::String) {
		String text = value.ToString();
		int len = text.LengthB();
		if (len <= 0) return false;

		scratch.resize(len);
		memcpy(scratch.data(), text.c_str(), (size_t)len);
		*outBytes = scratch.data();
		*outSize = len;
	} else if (value.type == ValueType::List) {
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
	if (bytes == nullptr || length <= 0) return Value::null;

	BinaryData* raw = new BinaryData(length);
	memcpy(raw->bytes, bytes, (size_t)length);
	return RawDataToValue(raw);
}

static VrDeviceInfo ValueToVrDeviceInfo(Value value) {
	VrDeviceInfo result = {0};
	if (value.type != ValueType::Map) return result;

	ValueDict map = value.GetDict();
	result.hResolution = map.Lookup(String("hResolution"), Value::zero).IntValue();
	result.vResolution = map.Lookup(String("vResolution"), Value::zero).IntValue();
	result.hScreenSize = map.Lookup(String("hScreenSize"), Value::zero).FloatValue();
	result.vScreenSize = map.Lookup(String("vScreenSize"), Value::zero).FloatValue();
	result.eyeToScreenDistance = map.Lookup(String("eyeToScreenDistance"), Value::zero).FloatValue();
	result.lensSeparationDistance = map.Lookup(String("lensSeparationDistance"), Value::zero).FloatValue();
	result.interpupillaryDistance = map.Lookup(String("interpupillaryDistance"), Value::zero).FloatValue();

	Value lensValue = map.Lookup(String("lensDistortionValues"), Value::null);
	if (lensValue.type == ValueType::List) {
		ValueList lens = lensValue.GetList();
		for (int i = 0; i < 4 && i < lens.Count(); i++) result.lensDistortionValues[i] = lens[i].FloatValue();
	}

	Value chromaValue = map.Lookup(String("chromaAbCorrection"), Value::null);
	if (chromaValue.type == ValueType::List) {
		ValueList chroma = chromaValue.GetList();
		for (int i = 0; i < 4 && i < chroma.Count(); i++) result.chromaAbCorrection[i] = chroma[i].FloatValue();
	}

	return result;
}

static VrStereoConfig ValueToVrStereoConfig(Value value) {
	VrStereoConfig result{};
	if (value.type != ValueType::Map) return result;

	ValueDict map = value.GetDict();

	Value projectionValue = map.Lookup(String("projection"), Value::null);
	if (projectionValue.type == ValueType::List) {
		ValueList projection = projectionValue.GetList();
		if (projection.Count() > 0) result.projection[0] = ValueToMatrix(projection[0]);
		if (projection.Count() > 1) result.projection[1] = ValueToMatrix(projection[1]);
	}

	Value viewOffsetValue = map.Lookup(String("viewOffset"), Value::null);
	if (viewOffsetValue.type == ValueType::List) {
		ValueList viewOffset = viewOffsetValue.GetList();
		if (viewOffset.Count() > 0) result.viewOffset[0] = ValueToMatrix(viewOffset[0]);
		if (viewOffset.Count() > 1) result.viewOffset[1] = ValueToMatrix(viewOffset[1]);
	}

	auto ReadFloatPair = [](Value value, float out[2]) {
		if (value.type != ValueType::List) return;
		ValueList list = value.GetList();
		if (list.Count() > 0) out[0] = list[0].FloatValue();
		if (list.Count() > 1) out[1] = list[1].FloatValue();
	};

	ReadFloatPair(map.Lookup(String("leftLensCenter"), Value::null), result.leftLensCenter);
	ReadFloatPair(map.Lookup(String("rightLensCenter"), Value::null), result.rightLensCenter);
	ReadFloatPair(map.Lookup(String("leftScreenCenter"), Value::null), result.leftScreenCenter);
	ReadFloatPair(map.Lookup(String("rightScreenCenter"), Value::null), result.rightScreenCenter);
	ReadFloatPair(map.Lookup(String("scale"), Value::null), result.scale);
	ReadFloatPair(map.Lookup(String("scaleIn"), Value::null), result.scaleIn);

	return result;
}

static Value VrStereoConfigToValue(const VrStereoConfig& config) {
	ValueDict map;

	ValueList projection;
	projection.Add(MatrixToValue(config.projection[0]));
	projection.Add(MatrixToValue(config.projection[1]));
	map.SetValue(String("projection"), Value(projection));

	ValueList viewOffset;
	viewOffset.Add(MatrixToValue(config.viewOffset[0]));
	viewOffset.Add(MatrixToValue(config.viewOffset[1]));
	map.SetValue(String("viewOffset"), Value(viewOffset));

	ValueList leftLensCenter;
	leftLensCenter.Add(Value(config.leftLensCenter[0]));
	leftLensCenter.Add(Value(config.leftLensCenter[1]));
	map.SetValue(String("leftLensCenter"), Value(leftLensCenter));

	ValueList rightLensCenter;
	rightLensCenter.Add(Value(config.rightLensCenter[0]));
	rightLensCenter.Add(Value(config.rightLensCenter[1]));
	map.SetValue(String("rightLensCenter"), Value(rightLensCenter));

	ValueList leftScreenCenter;
	leftScreenCenter.Add(Value(config.leftScreenCenter[0]));
	leftScreenCenter.Add(Value(config.leftScreenCenter[1]));
	map.SetValue(String("leftScreenCenter"), Value(leftScreenCenter));

	ValueList rightScreenCenter;
	rightScreenCenter.Add(Value(config.rightScreenCenter[0]));
	rightScreenCenter.Add(Value(config.rightScreenCenter[1]));
	map.SetValue(String("rightScreenCenter"), Value(rightScreenCenter));

	ValueList scale;
	scale.Add(Value(config.scale[0]));
	scale.Add(Value(config.scale[1]));
	map.SetValue(String("scale"), Value(scale));

	ValueList scaleIn;
	scaleIn.Add(Value(config.scaleIn[0]));
	scaleIn.Add(Value(config.scaleIn[1]));
	map.SetValue(String("scaleIn"), Value(scaleIn));

	return Value(map);
}

static AutomationEvent ValueToAutomationEvent(Value value) {
	AutomationEvent result = {0};
	if (value.type != ValueType::Map) return result;

	ValueDict map = value.GetDict();
	result.frame = (unsigned int)map.Lookup(String("frame"), Value::zero).IntValue();
	result.type = (unsigned int)map.Lookup(String("type"), Value::zero).IntValue();

	Value paramsValue = map.Lookup(String("params"), Value::null);
	if (paramsValue.type == ValueType::List) {
		ValueList params = paramsValue.GetList();
		for (int i = 0; i < 4 && i < params.Count(); i++) result.params[i] = params[i].IntValue();
	} else {
		result.params[0] = map.Lookup(String("p0"), Value::zero).IntValue();
		result.params[1] = map.Lookup(String("p1"), Value::zero).IntValue();
		result.params[2] = map.Lookup(String("p2"), Value::zero).IntValue();
		result.params[3] = map.Lookup(String("p3"), Value::zero).IntValue();
	}

	return result;
}

static Value AutomationEventToValue(const AutomationEvent& event) {
	ValueDict map;
	map.SetValue(String("frame"), Value((double)event.frame));
	map.SetValue(String("type"), Value((double)event.type));

	ValueList params;
	for (int i = 0; i < 4; i++) params.Add(Value(event.params[i]));
	map.SetValue(String("params"), Value(params));

	return Value(map);
}

static AutomationEventList* GetAutomationEventListPtr(Value value) {
	if (value.type != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (AutomationEventList*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static Value AutomationEventListToValue(const AutomationEventList& list) {
	AutomationEventList* listPtr = new AutomationEventList(list);

	ValueDict map;
	map.SetValue(String("_handle"), PointerToValue(listPtr));
	map.SetValue(String("capacity"), Value((double)list.capacity));
	map.SetValue(String("count"), Value((double)list.count));

	ValueList events;
	for (unsigned int i = 0; i < list.count; i++) events.Add(AutomationEventToValue(list.events[i]));
	map.SetValue(String("events"), Value(events));

	return Value(map);
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

	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) out[i] = list[i].FloatValue();
		return;
	}

	if (value.type == ValueType::Map) {
		ValueDict map = value.GetDict();
		if (components > 0) out[0] = map.Lookup(String("x"), map.Lookup(String("r"), Value::zero)).FloatValue();
		if (components > 1) out[1] = map.Lookup(String("y"), map.Lookup(String("g"), Value::zero)).FloatValue();
		if (components > 2) out[2] = map.Lookup(String("z"), map.Lookup(String("b"), Value::zero)).FloatValue();
		if (components > 3) out[3] = map.Lookup(String("w"), map.Lookup(String("a"), Value::zero)).FloatValue();
		return;
	}

	if (components > 0) out[0] = value.FloatValue();
}

static void FillIntComponentsFromValue(Value value, int* out, int components) {
	for (int i = 0; i < components; i++) out[i] = 0;

	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) out[i] = list[i].IntValue();
		return;
	}

	if (value.type == ValueType::Map) {
		ValueDict map = value.GetDict();
		if (components > 0) out[0] = map.Lookup(String("x"), map.Lookup(String("r"), Value::zero)).IntValue();
		if (components > 1) out[1] = map.Lookup(String("y"), map.Lookup(String("g"), Value::zero)).IntValue();
		if (components > 2) out[2] = map.Lookup(String("z"), map.Lookup(String("b"), Value::zero)).IntValue();
		if (components > 3) out[3] = map.Lookup(String("w"), map.Lookup(String("a"), Value::zero)).IntValue();
		return;
	}

	if (components > 0) out[0] = value.IntValue();
}

static void FillUIntComponentsFromValue(Value value, unsigned int* out, int components) {
	for (int i = 0; i < components; i++) out[i] = 0;

	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) {
			int v = list[i].IntValue();
			out[i] = (unsigned int)(v < 0 ? 0 : v);
		}
		return;
	}

	if (value.type == ValueType::Map) {
		ValueDict map = value.GetDict();
		if (components > 0) { int v = map.Lookup(String("x"), map.Lookup(String("r"), Value::zero)).IntValue(); out[0] = (unsigned int)(v < 0 ? 0 : v); }
		if (components > 1) { int v = map.Lookup(String("y"), map.Lookup(String("g"), Value::zero)).IntValue(); out[1] = (unsigned int)(v < 0 ? 0 : v); }
		if (components > 2) { int v = map.Lookup(String("z"), map.Lookup(String("b"), Value::zero)).IntValue(); out[2] = (unsigned int)(v < 0 ? 0 : v); }
		if (components > 3) { int v = map.Lookup(String("w"), map.Lookup(String("a"), Value::zero)).IntValue(); out[3] = (unsigned int)(v < 0 ? 0 : v); }
		return;
	}

	if (components > 0) {
		int v = value.IntValue();
		out[0] = (unsigned int)(v < 0 ? 0 : v);
	}
}

static void PackFloatUniformData(Value value, int components, int& count, std::vector<float>& out) {
	out.clear();

	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		bool nested = list.Count() > 0 && (list[0].type == ValueType::List || list[0].type == ValueType::Map);

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

	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		bool nested = list.Count() > 0 && (list[0].type == ValueType::List || list[0].type == ValueType::Map);

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

	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		bool nested = list.Count() > 0 && (list[0].type == ValueType::List || list[0].type == ValueType::Map);

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

struct RaylibCallbackBridgeState {
	Interpreter* interpreter = nullptr;
	Value traceLogCallback = Value::null;
	Value loadFileDataCallback = Value::null;
	Value saveFileDataCallback = Value::null;
	Value loadFileTextCallback = Value::null;
	Value saveFileTextCallback = Value::null;
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
	g_callbackBridgeState.interpreter = nullptr;
	g_callbackBridgeState.traceLogCallback = Value::null;
	g_callbackBridgeState.loadFileDataCallback = Value::null;
	g_callbackBridgeState.saveFileDataCallback = Value::null;
	g_callbackBridgeState.loadFileTextCallback = Value::null;
	g_callbackBridgeState.saveFileTextCallback = Value::null;
	g_callbackBridgeState.invokingTraceLogCallback = false;
}

static bool IsFunctionOrNull(Value callback) {
	return callback.IsNull() || callback.type == ValueType::Function;
}

static FunctionStorage* BuildCallbackInvoker(Value callback, ValueList args) {
	if (callback.type != ValueType::Function) return nullptr;

	FunctionStorage* invoker = new FunctionStorage();
	for (int i = 0; i < args.Count(); i++) {
		invoker->code.Add(TACLine(TACLine::Op::PushParam, args[i]));
	}

	invoker->code.Add(TACLine(Value::Temp(0), TACLine::Op::CallFunctionA, callback, Value(args.Count())));
	return invoker;
}

static bool InvokeMiniScriptCallback(Value callback, ValueList args, Value* outResult) {
	if (outResult != nullptr) *outResult = Value::null;
	if (callback.type != ValueType::Function) return false;

	Interpreter* interpreter = g_callbackBridgeState.interpreter;
	if (interpreter == nullptr || interpreter->vm == nullptr) {
		ResetRaylibCallbackBridge();
		return false;
	}

	Machine* vm = interpreter->vm;
	Context* callerContext = vm->GetTopContext();
	if (callerContext == nullptr) {
		ResetRaylibCallbackBridge();
		return false;
	}

	FunctionStorage* invoker = BuildCallbackInvoker(callback, args);
	if (invoker == nullptr) return false;

	Value previousTemp0 = callerContext->GetTemp(0, Value::null);
	vm->ManuallyPushCall(invoker, Value::Temp(0));
	delete invoker;

	bool completed = false;
	try {
		while (vm->GetTopContext() != callerContext && !vm->yielding) vm->Step();
		completed = (vm->GetTopContext() == callerContext);
	} catch (...) {
		ResetRaylibCallbackBridge();
		completed = false;
	}

	Value callbackResult = callerContext->GetTemp(0, Value::null);
	callerContext->SetTemp(0, previousTemp0);

	if (outResult != nullptr) *outResult = callbackResult;
	return completed;
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

static bool MiniScriptSaveFileDataBridge(const char* fileName, void* data, int dataSize) {
	ValueList args;
	args.Add(Value(String(fileName == nullptr ? "" : fileName)));
	args.Add(BytesToRawDataValue((unsigned char*)data, dataSize));
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

void AddRCoreMethods(ValueDict raylibModule) {
	Intrinsic *i;

	// Drawing-related functions

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		BeginDrawing();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginDrawing", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndDrawing();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndDrawing", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		Value colorVal = context->GetVar(String("color"));
		Color color = ValueToColor(colorVal);
		ClearBackground(color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ClearBackground", i->GetFunc());

	// 3D camera and projection functions

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context->GetVar(String("camera")));
		BeginMode3D(camera);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginMode3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndMode3D();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndMode3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Camera3D camera = ValueToCamera3D(context->GetVar(String("camera")));
		Ray ray = GetScreenToWorldRay(position, camera);
		return IntrinsicResult(RayToValue(ray));
	};
	raylibModule.SetValue("GetScreenToWorldRay", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("camera");
	i->AddParam("width", Value::zero);
	i->AddParam("height", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Camera3D camera = ValueToCamera3D(context->GetVar(String("camera")));
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		if (width <= 0) width = GetScreenWidth();
		if (height <= 0) height = GetScreenHeight();
		Ray ray = GetScreenToWorldRayEx(position, camera, width, height);
		return IntrinsicResult(RayToValue(ray));
	};
	raylibModule.SetValue("GetScreenToWorldRayEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Camera3D camera = ValueToCamera3D(context->GetVar(String("camera")));
		Vector2 result = GetWorldToScreen(position, camera);
		return IntrinsicResult(Vector2ToValue(result));
	};
	raylibModule.SetValue("GetWorldToScreen", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("camera");
	i->AddParam("width", Value::zero);
	i->AddParam("height", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Camera3D camera = ValueToCamera3D(context->GetVar(String("camera")));
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		if (width <= 0) width = GetScreenWidth();
		if (height <= 0) height = GetScreenHeight();
		Vector2 result = GetWorldToScreenEx(position, camera, width, height);
		return IntrinsicResult(Vector2ToValue(result));
	};
	raylibModule.SetValue("GetWorldToScreenEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		Camera3D camera = ValueToCamera3D(context->GetVar(String("camera")));
		return IntrinsicResult(MatrixToValue(GetCameraMatrix(camera)));
	};
	raylibModule.SetValue("GetCameraMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->AddParam("mode", Value(CAMERA_CUSTOM));
	i->code = INTRINSIC_LAMBDA {
		Value cameraValue = context->GetVar(String("camera"));
		Camera3D camera = ValueToCamera3D(cameraValue);
		int mode = context->GetVar(String("mode")).IntValue();
		UpdateCamera((Camera*)&camera, mode);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateCamera", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->AddParam("movement", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("rotation", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("zoom", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value cameraValue = context->GetVar(String("camera"));
		Camera3D camera = ValueToCamera3D(cameraValue);
		Vector3 movement = ValueToVector3(context->GetVar(String("movement")));
		Vector3 rotation = ValueToVector3(context->GetVar(String("rotation")));
		float zoom = context->GetVar(String("zoom")).FloatValue();
		UpdateCameraPro((Camera*)&camera, movement, rotation, zoom);
		SyncCamera3DValue(cameraValue, camera);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateCameraPro", i->GetFunc());

	// Shader functions

	i = Intrinsic::Create("");
	i->AddParam("vsFileName", String());
	i->AddParam("fsFileName", String());
	i->code = INTRINSIC_LAMBDA {
		String vsFileName = context->GetVar(String("vsFileName")).ToString();
		String fsFileName = context->GetVar(String("fsFileName")).ToString();
		const char* vsPtr = vsFileName.LengthB() > 0 ? vsFileName.c_str() : nullptr;
		const char* fsPtr = fsFileName.LengthB() > 0 ? fsFileName.c_str() : nullptr;
		Shader shader = LoadShader(vsPtr, fsPtr);
		if (!IsShaderValid(shader)) return IntrinsicResult::Null;
		rcShader++;
		return IntrinsicResult(ShaderToValue(shader));
	};
	raylibModule.SetValue("LoadShader", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("vsCode", String());
	i->AddParam("fsCode", String());
	i->code = INTRINSIC_LAMBDA {
		String vsCode = context->GetVar(String("vsCode")).ToString();
		String fsCode = context->GetVar(String("fsCode")).ToString();
		const char* vsPtr = vsCode.LengthB() > 0 ? vsCode.c_str() : nullptr;
		const char* fsPtr = fsCode.LengthB() > 0 ? fsCode.c_str() : nullptr;
		Shader shader = LoadShaderFromMemory(vsPtr, fsPtr);
		if (!IsShaderValid(shader)) return IntrinsicResult::Null;
		rcShader++;
		return IntrinsicResult(ShaderToValue(shader));
	};
	raylibModule.SetValue("LoadShaderFromMemory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		return IntrinsicResult(IsShaderValid(shader));
	};
	raylibModule.SetValue("IsShaderValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		BeginShaderMode(shader);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginShaderMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndShaderMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndShaderMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->AddParam("uniformName");
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		String uniformName = context->GetVar(String("uniformName")).ToString();
		return IntrinsicResult(GetShaderLocation(shader, uniformName.c_str()));
	};
	raylibModule.SetValue("GetShaderLocation", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->AddParam("attribName");
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		String attribName = context->GetVar(String("attribName")).ToString();
		return IntrinsicResult(GetShaderLocationAttrib(shader, attribName.c_str()));
	};
	raylibModule.SetValue("GetShaderLocationAttrib", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->AddParam("locIndex");
	i->AddParam("mat");
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		SetShaderValueMatrix(shader, locIndex, mat);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetShaderValueMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->AddParam("locIndex");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		SetShaderValueTexture(shader, locIndex, texture);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetShaderValueTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->AddParam("locIndex");
	i->AddParam("value");
	i->AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Value value = context->GetVar(String("value"));
		int uniformType = context->GetVar(String("uniformType")).IntValue();

		BinaryData* rawData = nullptr;
		if (value.type == ValueType::Map) rawData = ValueToRawData(value);
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
	};
	raylibModule.SetValue("SetShaderValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->AddParam("locIndex");
	i->AddParam("value");
	i->AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i->AddParam("count", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Shader shader = ValueToShader(context->GetVar(String("shader")));
		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Value value = context->GetVar(String("value"));
		int uniformType = context->GetVar(String("uniformType")).IntValue();
		int count = context->GetVar(String("count")).IntValue();

		BinaryData* rawData = nullptr;
		if (value.type == ValueType::Map) rawData = ValueToRawData(value);
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
	};
	raylibModule.SetValue("SetShaderValueV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("shader");
	i->code = INTRINSIC_LAMBDA {
		Value shaderValue = context->GetVar(String("shader"));
		Shader shader = ValueToShader(shaderValue);
		UnloadShader(shader);

		Shader* shaderPtr = GetShaderPtr(shaderValue);
		if (shaderPtr != nullptr) {
			delete shaderPtr;
			rcShader--;
			ValueDict map = shaderValue.GetDict();
			map.SetValue(String("_handle"), Value::zero);
		}

		SyncShaderValue(shaderValue, Shader{0, NULL});
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadShader", i->GetFunc());

	// Timing functions

	i = Intrinsic::Create("");
	i->AddParam("fps");
	i->code = INTRINSIC_LAMBDA {
		SetTargetFPS(context->GetVar(String("fps")).IntValue());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetTargetFPS", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetFrameTime());
	};
	raylibModule.SetValue("GetFrameTime", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTime());
	};
	raylibModule.SetValue("GetTime", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetFPS());
	};
	raylibModule.SetValue("GetFPS", i->GetFunc());

	// Input-related functions: keyboard

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyPressed(context->GetVar(String("key")).IntValue()));
	};
	raylibModule.SetValue("IsKeyPressed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyPressedRepeat(context->GetVar(String("key")).IntValue()));
	};
	raylibModule.SetValue("IsKeyPressedRepeat", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyDown(context->GetVar(String("key")).IntValue()));
	};
	raylibModule.SetValue("IsKeyDown", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyReleased(context->GetVar(String("key")).IntValue()));
	};
	raylibModule.SetValue("IsKeyReleased", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsKeyUp(context->GetVar(String("key")).IntValue()));
	};
	raylibModule.SetValue("IsKeyUp", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetKeyPressed());
	};
	raylibModule.SetValue("GetKeyPressed", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetCharPressed());
	};
	raylibModule.SetValue("GetCharPressed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		const char* keyName = GetKeyName(context->GetVar(String("key")).IntValue());
		if (keyName == nullptr) return IntrinsicResult(String());
		return IntrinsicResult(String(keyName));
	};
	raylibModule.SetValue("GetKeyName", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("key");
	i->code = INTRINSIC_LAMBDA {
		SetExitKey(context->GetVar(String("key")).IntValue());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetExitKey", i->GetFunc());

	// Input-related functions: gamepad

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadAvailable(context->GetVar(String("gamepad")).IntValue()));
	};
	raylibModule.SetValue("IsGamepadAvailable", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadName(context->GetVar(String("gamepad")).IntValue()));
	};
	raylibModule.SetValue("GetGamepadName", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonPressed(
			context->GetVar(String("gamepad")).IntValue(),
			context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsGamepadButtonPressed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonDown(
			context->GetVar(String("gamepad")).IntValue(),
			context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsGamepadButtonDown", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonReleased(
			context->GetVar(String("gamepad")).IntValue(),
			context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsGamepadButtonReleased", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsGamepadButtonUp(
			context->GetVar(String("gamepad")).IntValue(),
			context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsGamepadButtonUp", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadButtonPressed());
	};
	raylibModule.SetValue("GetGamepadButtonPressed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadAxisCount(context->GetVar(String("gamepad")).IntValue()));
	};
	raylibModule.SetValue("GetGamepadAxisCount", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->AddParam("axis");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGamepadAxisMovement(
			context->GetVar(String("gamepad")).IntValue(),
			context->GetVar(String("axis")).IntValue()));
	};
	raylibModule.SetValue("GetGamepadAxisMovement", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mappings");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(SetGamepadMappings(context->GetVar(String("mappings")).ToString().c_str()));
	};
	raylibModule.SetValue("SetGamepadMappings", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gamepad", 0);
	i->AddParam("leftMotor", 0.0);
	i->AddParam("rightMotor", 0.0);
	i->AddParam("duration", 0.0);
	i->code = INTRINSIC_LAMBDA {
		SetGamepadVibration(
			context->GetVar(String("gamepad")).IntValue(),
			context->GetVar(String("leftMotor")).FloatValue(),
			context->GetVar(String("rightMotor")).FloatValue(),
			context->GetVar(String("duration")).FloatValue());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetGamepadVibration", i->GetFunc());

	// Input-related functions: mouse

	i = Intrinsic::Create("");
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonPressed(context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsMouseButtonPressed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonDown(context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsMouseButtonDown", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonReleased(context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsMouseButtonReleased", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("button");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsMouseButtonUp(context->GetVar(String("button")).IntValue()));
	};
	raylibModule.SetValue("IsMouseButtonUp", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetMouseX());
	};
	raylibModule.SetValue("GetMouseX", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetMouseY());
	};
	raylibModule.SetValue("GetMouseY", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Vector2 pos = GetMousePosition();
		ValueDict posMap;
		posMap.SetValue(String("x"), Value(pos.x));
		posMap.SetValue(String("y"), Value(pos.y));
		return IntrinsicResult(posMap);
	};
	raylibModule.SetValue("GetMousePosition", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Vector2 delta = GetMouseDelta();
		ValueDict deltaMap;
		deltaMap.SetValue(String("x"), Value(delta.x));
		deltaMap.SetValue(String("y"), Value(delta.y));
		return IntrinsicResult(deltaMap);
	};
	raylibModule.SetValue("GetMouseDelta", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetMouseWheelMove());
	};
	raylibModule.SetValue("GetMouseWheelMove", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("cursor");
	i->code = INTRINSIC_LAMBDA {
		SetMouseCursor(context->GetVar(String("cursor")).IntValue());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMouseCursor", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		ShowCursor();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ShowCursor", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		HideCursor();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("HideCursor", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsCursorHidden());
	};
	raylibModule.SetValue("IsCursorHidden", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsCursorOnScreen());
	};
	raylibModule.SetValue("IsCursorOnScreen", i->GetFunc());

	// Set window title/icon (platform-gated)
	i = Intrinsic::Create("");
	i->AddParam("caption", "raylib-miniscript");
	i->code = INTRINSIC_LAMBDA {
		String caption = context->GetVar(String("caption")).GetString();
#ifdef PLATFORM_WEB
		_SetWindowTitle_Web(caption.c_str());
#else
		SetWindowTitle(caption.c_str());
#endif
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowTitle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
#ifdef PLATFORM_WEB
		int size;
		unsigned char *data = ExportImageToMemory(image, ".png", &size);
		_SetWindowIcon_Web(data, size);
		free(data);
#else
		SetWindowIcon(image);
#endif
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowIcon", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("images");
	i->AddParam("count", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowIcons");
		return IntrinsicResult::Null;
	#endif
		Value imagesValue = context->GetVar(String("images"));
		int count = context->GetVar(String("count")).IntValue();

		if (imagesValue.type != ValueType::List) {
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
	};
	raylibModule.SetValue("SetWindowIcons", i->GetFunc());

	// Screen dimension functions

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetScreenWidth());
	};
	raylibModule.SetValue("GetScreenWidth", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetScreenHeight());
	};
	raylibModule.SetValue("GetScreenHeight", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetRenderWidth());
	};
	raylibModule.SetValue("GetRenderWidth", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetRenderHeight());
	};
	raylibModule.SetValue("GetRenderHeight", i->GetFunc());

	// Window state functions

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(WindowShouldClose());
	};
	raylibModule.SetValue("WindowShouldClose", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsWindowFullscreen());
	};
	raylibModule.SetValue("IsWindowFullscreen", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowHidden");
		return IntrinsicResult(false);
#endif
		return IntrinsicResult(IsWindowHidden());
	};
	raylibModule.SetValue("IsWindowHidden", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowMinimized");
		return IntrinsicResult(false);
#endif
		return IntrinsicResult(IsWindowMinimized());
	};
	raylibModule.SetValue("IsWindowMinimized", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowMaximized");
		return IntrinsicResult(false);
#endif
		return IntrinsicResult(IsWindowMaximized());
	};
	raylibModule.SetValue("IsWindowMaximized", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowResized");
		return IntrinsicResult(false);
#endif
		return IntrinsicResult(IsWindowResized());
	};
	raylibModule.SetValue("IsWindowResized", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("flags");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsWindowState");
		return IntrinsicResult(false);
	#endif
		unsigned int flags = (unsigned int)context->GetVar(String("flags")).IntValue();
		return IntrinsicResult(IsWindowState(flags));
	};
	raylibModule.SetValue("IsWindowState", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("flags");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowState");
		return IntrinsicResult::Null;
	#endif
		unsigned int flags = (unsigned int)context->GetVar(String("flags")).IntValue();
		SetWindowState(flags);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowState", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("flags");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ClearWindowState");
		return IntrinsicResult::Null;
	#endif
		unsigned int flags = (unsigned int)context->GetVar(String("flags")).IntValue();
		ClearWindowState(flags);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ClearWindowState", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		ToggleFullscreen();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ToggleFullscreen", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ToggleBorderlessWindowed");
		return IntrinsicResult::Null;
	#endif
		ToggleBorderlessWindowed();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ToggleBorderlessWindowed", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("MaximizeWindow");
		return IntrinsicResult::Null;
	#endif
		MaximizeWindow();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("MaximizeWindow", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("MinimizeWindow");
		return IntrinsicResult::Null;
	#endif
		MinimizeWindow();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("MinimizeWindow", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("RestoreWindow");
		return IntrinsicResult::Null;
	#endif
		RestoreWindow();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("RestoreWindow", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x");
	i->AddParam("y");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowPosition");
		return IntrinsicResult::Null;
	#endif
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		SetWindowPosition(x, y);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowPosition", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowMonitor");
		return IntrinsicResult::Null;
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		SetWindowMonitor(monitor);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowMonitor", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width");
	i->AddParam("height");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowMinSize");
		return IntrinsicResult::Null;
	#endif
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		SetWindowMinSize(width, height);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowMinSize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width");
	i->AddParam("height");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowMaxSize");
		return IntrinsicResult::Null;
	#endif
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		SetWindowMaxSize(width, height);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowMaxSize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width");
	i->AddParam("height");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowSize");
		return IntrinsicResult::Null;
	#endif
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		SetWindowSize(width, height);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowSize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("opacity");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowOpacity");
		return IntrinsicResult::Null;
	#endif
		float opacity = context->GetVar(String("opacity")).FloatValue();
		SetWindowOpacity(opacity);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowOpacity", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetWindowFocused");
		return IntrinsicResult::Null;
	#endif
		SetWindowFocused();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetWindowFocused", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWindowHandle");
		return IntrinsicResult(PointerToValue(nullptr));
	#endif
		return IntrinsicResult(PointerToValue(GetWindowHandle()));
	};
	raylibModule.SetValue("GetWindowHandle", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorCount");
		return IntrinsicResult(Value::zero);
	#endif
		return IntrinsicResult(GetMonitorCount());
	};
	raylibModule.SetValue("GetMonitorCount", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetCurrentMonitor");
		return IntrinsicResult(Value::zero);
	#endif
		return IntrinsicResult(GetCurrentMonitor());
	};
	raylibModule.SetValue("GetCurrentMonitor", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorPosition");
		return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		return IntrinsicResult(Vector2ToValue(GetMonitorPosition(monitor)));
	};
	raylibModule.SetValue("GetMonitorPosition", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorWidth");
		return IntrinsicResult(Value::zero);
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		return IntrinsicResult(GetMonitorWidth(monitor));
	};
	raylibModule.SetValue("GetMonitorWidth", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorHeight");
		return IntrinsicResult(Value::zero);
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		return IntrinsicResult(GetMonitorHeight(monitor));
	};
	raylibModule.SetValue("GetMonitorHeight", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorPhysicalWidth");
		return IntrinsicResult(Value::zero);
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		return IntrinsicResult(GetMonitorPhysicalWidth(monitor));
	};
	raylibModule.SetValue("GetMonitorPhysicalWidth", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorPhysicalHeight");
		return IntrinsicResult(Value::zero);
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		return IntrinsicResult(GetMonitorPhysicalHeight(monitor));
	};
	raylibModule.SetValue("GetMonitorPhysicalHeight", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorRefreshRate");
		return IntrinsicResult(Value::zero);
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		return IntrinsicResult(GetMonitorRefreshRate(monitor));
	};
	raylibModule.SetValue("GetMonitorRefreshRate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("monitor", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetMonitorName");
		return IntrinsicResult(String());
	#endif
		int monitor = context->GetVar(String("monitor")).IntValue();
		String name = GetMonitorName(monitor);
		return IntrinsicResult(name);
	};
	raylibModule.SetValue("GetMonitorName", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWindowPosition");
		return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));
	#endif
		return IntrinsicResult(Vector2ToValue(GetWindowPosition()));
	};
	raylibModule.SetValue("GetWindowPosition", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWindowScaleDPI");
		return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));
	#endif
		return IntrinsicResult(Vector2ToValue(GetWindowScaleDPI()));
	};
	raylibModule.SetValue("GetWindowScaleDPI", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsWindowFocused());
	};
	raylibModule.SetValue("IsWindowFocused", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(IsWindowReady());
	};
	raylibModule.SetValue("IsWindowReady", i->GetFunc());

	// Additional mouse functions

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Vector2 wheelMove = GetMouseWheelMoveV();
		ValueDict wheelMap;
		wheelMap.SetValue(String("x"), Value(wheelMove.x));
		wheelMap.SetValue(String("y"), Value(wheelMove.y));
		return IntrinsicResult(wheelMap);
	};
	raylibModule.SetValue("GetMouseWheelMoveV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x");
	i->AddParam("y");
	i->code = INTRINSIC_LAMBDA {
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		SetMousePosition(x, y);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMousePosition", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("offsetX");
	i->AddParam("offsetY");
	i->code = INTRINSIC_LAMBDA {
		int offsetX = context->GetVar(String("offsetX")).IntValue();
		int offsetY = context->GetVar(String("offsetY")).IntValue();
		SetMouseOffset(offsetX, offsetY);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMouseOffset", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("scaleX");
	i->AddParam("scaleY");
	i->code = INTRINSIC_LAMBDA {
		float scaleX = context->GetVar(String("scaleX")).FloatValue();
		float scaleY = context->GetVar(String("scaleY")).FloatValue();
		SetMouseScale(scaleX, scaleY);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMouseScale", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EnableCursor();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EnableCursor", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		DisableCursor();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DisableCursor", i->GetFunc());

	// Touch input functions

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTouchX());
	};
	raylibModule.SetValue("GetTouchX", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTouchY());
	};
	raylibModule.SetValue("GetTouchY", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("index", 0);
	i->code = INTRINSIC_LAMBDA {
		int index = context->GetVar(String("index")).IntValue();
		Vector2 pos = GetTouchPosition(index);
		ValueDict posMap;
		posMap.SetValue(String("x"), Value(pos.x));
		posMap.SetValue(String("y"), Value(pos.y));
		return IntrinsicResult(posMap);
	};
	raylibModule.SetValue("GetTouchPosition", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("index", 0);
	i->code = INTRINSIC_LAMBDA {
		int index = context->GetVar(String("index")).IntValue();
		return IntrinsicResult(GetTouchPointId(index));
	};
	raylibModule.SetValue("GetTouchPointId", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetTouchPointCount());
	};
	raylibModule.SetValue("GetTouchPointCount", i->GetFunc());

	// Gesture functions

	i = Intrinsic::Create("");
	i->AddParam("flags");
	i->code = INTRINSIC_LAMBDA {
		unsigned int flags = (unsigned int)context->GetVar(String("flags")).IntValue();
		SetGesturesEnabled(flags);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetGesturesEnabled", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("gesture");
	i->code = INTRINSIC_LAMBDA {
		int gesture = context->GetVar(String("gesture")).IntValue();
		return IntrinsicResult(IsGestureDetected(gesture));
	};
	raylibModule.SetValue("IsGestureDetected", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGestureDetected());
	};
	raylibModule.SetValue("GetGestureDetected", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGestureHoldDuration());
	};
	raylibModule.SetValue("GetGestureHoldDuration", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Vector2 dragVector = GetGestureDragVector();
		ValueDict dragMap;
		dragMap.SetValue(String("x"), Value(dragVector.x));
		dragMap.SetValue(String("y"), Value(dragVector.y));
		return IntrinsicResult(dragMap);
	};
	raylibModule.SetValue("GetGestureDragVector", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGestureDragAngle());
	};
	raylibModule.SetValue("GetGestureDragAngle", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Vector2 pinchVector = GetGesturePinchVector();
		ValueDict pinchMap;
		pinchMap.SetValue(String("x"), Value(pinchVector.x));
		pinchMap.SetValue(String("y"), Value(pinchVector.y));
		return IntrinsicResult(pinchMap);
	};
	raylibModule.SetValue("GetGesturePinchVector", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetGesturePinchAngle());
	};
	raylibModule.SetValue("GetGesturePinchAngle", i->GetFunc());

	// 2D rendering mode functions

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		ValueDict cameraMap = context->GetVar(String("camera")).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(String("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(String("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(String("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(String("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(String("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(String("zoom"), Value::one).FloatValue();
		BeginMode2D(camera);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginMode2D", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndMode2D();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndMode2D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		ValueDict cameraMap = context->GetVar(String("camera")).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(String("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(String("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(String("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(String("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(String("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(String("zoom"), Value::one).FloatValue();

		return IntrinsicResult(MatrixToValue(GetCameraMatrix2D(camera)));
	};
	raylibModule.SetValue("GetCameraMatrix2D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		ValueDict cameraMap = context->GetVar(String("camera")).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(String("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(String("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(String("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(String("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(String("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(String("zoom"), Value::one).FloatValue();

		Vector2 result = GetWorldToScreen2D(position, camera);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetWorldToScreen2D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("camera");
	i->code = INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		ValueDict cameraMap = context->GetVar(String("camera")).GetDict();
		Camera2D camera;
		camera.offset.x = cameraMap.Lookup(String("offsetX"), Value::zero).FloatValue();
		camera.offset.y = cameraMap.Lookup(String("offsetY"), Value::zero).FloatValue();
		camera.target.x = cameraMap.Lookup(String("targetX"), Value::zero).FloatValue();
		camera.target.y = cameraMap.Lookup(String("targetY"), Value::zero).FloatValue();
		camera.rotation = cameraMap.Lookup(String("rotation"), Value::zero).FloatValue();
		camera.zoom = cameraMap.Lookup(String("zoom"), Value::one).FloatValue();

		Vector2 result = GetScreenToWorld2D(position, camera);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetScreenToWorld2D", i->GetFunc());

	// Blend mode functions

	i = Intrinsic::Create("");
	i->AddParam("mode");
	i->code = INTRINSIC_LAMBDA {
		int mode = context->GetVar(String("mode")).IntValue();
		BeginBlendMode(mode);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginBlendMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndBlendMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndBlendMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("glSrcFactor");
	i->AddParam("glDstFactor");
	i->AddParam("glEquation");
	i->code = INTRINSIC_LAMBDA {
		int glSrcFactor = context->GetVar(String("glSrcFactor")).IntValue();
		int glDstFactor = context->GetVar(String("glDstFactor")).IntValue();
		int glEquation = context->GetVar(String("glEquation")).IntValue();
		rlSetBlendFactors(glSrcFactor, glDstFactor, glEquation);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlSetBlendFactors", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("glSrcRGB");
	i->AddParam("glDstRGB");
	i->AddParam("glSrcAlpha");
	i->AddParam("glDstAlpha");
	i->AddParam("glEqRGB");
	i->AddParam("glEqAlpha");
	i->code = INTRINSIC_LAMBDA {
		int glSrcRGB = context->GetVar(String("glSrcRGB")).IntValue();
		int glDstRGB = context->GetVar(String("glDstRGB")).IntValue();
		int glSrcAlpha = context->GetVar(String("glSrcAlpha")).IntValue();
		int glDstAlpha = context->GetVar(String("glDstAlpha")).IntValue();
		int glEqRGB = context->GetVar(String("glEqRGB")).IntValue();
		int glEqAlpha = context->GetVar(String("glEqAlpha")).IntValue();
		rlSetBlendFactorsSeparate(glSrcRGB, glDstRGB, glSrcAlpha, glDstAlpha, glEqRGB, glEqAlpha);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlSetBlendFactorsSeparate", i->GetFunc());

	// Matrix operations (rlgl)

	i = Intrinsic::Create("");
	i->AddParam("mode");
	i->code = INTRINSIC_LAMBDA {
		int mode = context->GetVar(String("mode")).IntValue();
		rlMatrixMode(mode);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlMatrixMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlPushMatrix();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlPushMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlPopMatrix();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlPopMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlLoadIdentity();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlLoadIdentity", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x", Value::zero);
	i->AddParam("y", Value::zero);
	i->AddParam("z", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float x = context->GetVar(String("x")).FloatValue();
		float y = context->GetVar(String("y")).FloatValue();
		float z = context->GetVar(String("z")).FloatValue();
		rlTranslatef(x, y, z);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlTranslatef", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("angle", Value::zero);
	i->AddParam("x", Value::zero);
	i->AddParam("y", Value::zero);
	i->AddParam("z", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float angle = context->GetVar(String("angle")).FloatValue();
		float x = context->GetVar(String("x")).FloatValue();
		float y = context->GetVar(String("y")).FloatValue();
		float z = context->GetVar(String("z")).FloatValue();
		rlRotatef(angle, x, y, z);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlRotatef", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x", Value(1));
	i->AddParam("y", Value(1));
	i->AddParam("z", Value(1));
	i->code = INTRINSIC_LAMBDA {
		float x = context->GetVar(String("x")).FloatValue();
		float y = context->GetVar(String("y")).FloatValue();
		float z = context->GetVar(String("z")).FloatValue();
		rlScalef(x, y, z);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlScalef", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("matf");
	i->code = INTRINSIC_LAMBDA {
		Value listVal = context->GetVar(String("matf"));
		ValueList list = listVal.GetList();
		float matf[16];
		for (int j = 0; j < 16; j++) {
			matf[j] = (j < list.Count()) ? list[j].FloatValue() : 0.0f;
		}
		rlMultMatrixf(matf);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlMultMatrixf", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left");
	i->AddParam("right");
	i->AddParam("bottom");
	i->AddParam("top");
	i->AddParam("znear");
	i->AddParam("zfar");
	i->code = INTRINSIC_LAMBDA {
		double left = context->GetVar(String("left")).FloatValue();
		double right = context->GetVar(String("right")).FloatValue();
		double bottom = context->GetVar(String("bottom")).FloatValue();
		double top = context->GetVar(String("top")).FloatValue();
		double znear = context->GetVar(String("znear")).FloatValue();
		double zfar = context->GetVar(String("zfar")).FloatValue();
		rlFrustum(left, right, bottom, top, znear, zfar);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlFrustum", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left");
	i->AddParam("right");
	i->AddParam("bottom");
	i->AddParam("top");
	i->AddParam("znear");
	i->AddParam("zfar");
	i->code = INTRINSIC_LAMBDA {
		double left = context->GetVar(String("left")).FloatValue();
		double right = context->GetVar(String("right")).FloatValue();
		double bottom = context->GetVar(String("bottom")).FloatValue();
		double top = context->GetVar(String("top")).FloatValue();
		double znear = context->GetVar(String("znear")).FloatValue();
		double zfar = context->GetVar(String("zfar")).FloatValue();
		rlOrtho(left, right, bottom, top, znear, zfar);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlOrtho", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x");
	i->AddParam("y");
	i->AddParam("width");
	i->AddParam("height");
	i->code = INTRINSIC_LAMBDA {
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		rlViewport(x, y, width, height);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlViewport", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("nearPlane");
	i->AddParam("farPlane");
	i->code = INTRINSIC_LAMBDA {
		double nearPlane = context->GetVar(String("nearPlane")).FloatValue();
		double farPlane = context->GetVar(String("farPlane")).FloatValue();
		rlSetClipPlanes(nearPlane, farPlane);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlSetClipPlanes", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetCullDistanceNear()));
	};
	raylibModule.SetValue("rlGetCullDistanceNear", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetCullDistanceFar()));
	};
	raylibModule.SetValue("rlGetCullDistanceFar", i->GetFunc());

	// Render state toggles (rlgl)

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlEnableBackfaceCulling();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlEnableBackfaceCulling", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlDisableBackfaceCulling();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlDisableBackfaceCulling", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlEnableDepthTest();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlEnableDepthTest", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlDisableDepthTest();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlDisableDepthTest", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlEnableDepthMask();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlEnableDepthMask", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlDisableDepthMask();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlDisableDepthMask", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlEnableWireMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlEnableWireMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlDisableWireMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlDisableWireMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlEnableSmoothLines();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlEnableSmoothLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlDisableSmoothLines();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlDisableSmoothLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(1));
	i->code = INTRINSIC_LAMBDA {
		float width = context->GetVar(String("width")).FloatValue();
		rlSetLineWidth(width);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlSetLineWidth", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetLineWidth()));
	};
	raylibModule.SetValue("rlGetLineWidth", i->GetFunc());

	// Render batch (rlgl)

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		rlDrawRenderBatchActive();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlDrawRenderBatchActive", i->GetFunc());

	// Get/set matrices (rlgl)

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixModelview()));
	};
	raylibModule.SetValue("rlGetMatrixModelview", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixProjection()));
	};
	raylibModule.SetValue("rlGetMatrixProjection", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("proj");
	i->code = INTRINSIC_LAMBDA {
		Matrix proj = ValueToMatrix(context->GetVar(String("proj")));
		rlSetMatrixProjection(proj);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlSetMatrixProjection", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("view");
	i->code = INTRINSIC_LAMBDA {
		Matrix view = ValueToMatrix(context->GetVar(String("view")));
		rlSetMatrixModelview(view);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("rlSetMatrixModelview", i->GetFunc());

	// Scissor mode functions

	i = Intrinsic::Create("");
	i->AddParam("x");
	i->AddParam("y");
	i->AddParam("width");
	i->AddParam("height");
	i->code = INTRINSIC_LAMBDA {
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		BeginScissorMode(x, y, width, height);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginScissorMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndScissorMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndScissorMode", i->GetFunc());

	// VR stereo functions

	i = Intrinsic::Create("");
	i->AddParam("config");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("BeginVrStereoMode");
		return IntrinsicResult::Null;
	#endif
		VrStereoConfig config = ValueToVrStereoConfig(context->GetVar(String("config")));
		BeginVrStereoMode(config);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginVrStereoMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("EndVrStereoMode");
		return IntrinsicResult::Null;
	#endif
		EndVrStereoMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndVrStereoMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("device");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadVrStereoConfig");
		return IntrinsicResult::Null;
	#endif
		VrDeviceInfo device = ValueToVrDeviceInfo(context->GetVar(String("device")));
		VrStereoConfig config = LoadVrStereoConfig(device);
		return IntrinsicResult(VrStereoConfigToValue(config));
	};
	raylibModule.SetValue("LoadVrStereoConfig", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("config");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("UnloadVrStereoConfig");
		return IntrinsicResult::Null;
	#endif
		VrStereoConfig config = ValueToVrStereoConfig(context->GetVar(String("config")));
		UnloadVrStereoConfig(config);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadVrStereoConfig", i->GetFunc());

	// Utility functions

	i = Intrinsic::Create("");
	i->AddParam("url");
	i->code = INTRINSIC_LAMBDA {
		String url = context->GetVar(String("url")).ToString();
		OpenURL(url.c_str());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("OpenURL", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("text");
	i->code = INTRINSIC_LAMBDA {
		String text = context->GetVar(String("text")).ToString();
		SetClipboardText(text.c_str());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetClipboardText", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(GetClipboardText());
	};
	raylibModule.SetValue("GetClipboardText", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("EnableEventWaiting");
		return IntrinsicResult::Null;
	#endif
		EnableEventWaiting();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EnableEventWaiting", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("DisableEventWaiting");
		return IntrinsicResult::Null;
	#endif
		DisableEventWaiting();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DisableEventWaiting", i->GetFunc());

#if RAYLIB_VERSION_GT(5, 5) || !defined(PLATFORM_WEB)
	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Image image = GetClipboardImage();
		rcImage++;
		return IntrinsicResult(ImageToValue(image));
	};
	raylibModule.SetValue("GetClipboardImage", i->GetFunc());
#endif

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("ext");
	i->code = INTRINSIC_LAMBDA {
		String fileName = context->GetVar(String("fileName")).ToString();
		String ext = context->GetVar(String("ext")).ToString();
		return IntrinsicResult(IsFileExtension(fileName.c_str(), ext.c_str()));
	};
	raylibModule.SetValue("IsFileExtension", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String fileName = context->GetVar(String("fileName")).ToString();
		TakeScreenshot(fileName.c_str());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("TakeScreenshot", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("flags");
	i->code = INTRINSIC_LAMBDA {
		unsigned int flags = (unsigned int)context->GetVar(String("flags")).IntValue();
		SetConfigFlags(flags);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetConfigFlags", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize");
	i->code = INTRINSIC_LAMBDA {
		Value dataVal = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

		const unsigned char* bytes = nullptr;
		String tempStr;

		if (dataVal.type == ValueType::String) {
			tempStr = dataVal.ToString();
			bytes = (const unsigned char*)tempStr.c_str();
			if (dataSize <= 0) dataSize = tempStr.LengthB();
		} else if (dataVal.type == ValueType::Map) {
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
	};
	raylibModule.SetValue("EncodeDataBase64", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("seconds", 1.0);
	i->code = INTRINSIC_LAMBDA {
		double seconds = context->GetVar(String("seconds")).DoubleValue();
		WaitTime(seconds);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("WaitTime", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SwapScreenBuffer");
		return IntrinsicResult::Null;
	#endif
		SwapScreenBuffer();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SwapScreenBuffer", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("PollInputEvents");
		return IntrinsicResult::Null;
	#endif
		PollInputEvents();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PollInputEvents", i->GetFunc());

	// Load text files
	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		const char *fileName = context->GetVar("fileName").GetString().c_str();
		char *text = LoadFileText(fileName);
		if (text == nullptr) return IntrinsicResult::Null;
		String ret(text);
		UnloadFileText(text);
		return IntrinsicResult(ret);
	};
	raylibModule.SetValue("LoadFileText", i->GetFunc());

	// File system management

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadFileData");
		return IntrinsicResult::Null;
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		int dataSize = 0;
		unsigned char* data = LoadFileData(fileName.c_str(), &dataSize);
		if (data == nullptr || dataSize <= 0) return IntrinsicResult::Null;

		Value result = BytesToRawDataValue(data, dataSize);
		UnloadFileData(data);
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("LoadFileData", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("UnloadFileData");
		return IntrinsicResult::Null;
	#endif
		Value dataValue = context->GetVar(String("data"));
		BinaryData* rawData = ValueToRawData(dataValue);
		if (rawData == nullptr || rawData->bytes == nullptr) return IntrinsicResult::Null;

		UnloadFileData(rawData->bytes);
		rawData->bytes = nullptr;
		rawData->length = 0;
		rawData->ownsBuffer = false;

		if (dataValue.type == ValueType::Map) {
			ValueDict map = dataValue.GetDict();
			map.SetValue(String("_handle"), Value::zero);
		}

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadFileData", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("data");
	i->AddParam("dataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SaveFileData");
		return IntrinsicResult(false);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult(false);

		return IntrinsicResult(SaveFileData(fileName.c_str(), (void*)bytes, byteCount));
	};
	raylibModule.SetValue("SaveFileData", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ExportDataAsCode");
		return IntrinsicResult(false);
	#endif
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();
		String fileName = context->GetVar(String("fileName")).ToString();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult(false);

		return IntrinsicResult(ExportDataAsCode(bytes, byteCount, fileName.c_str()));
	};
	raylibModule.SetValue("ExportDataAsCode", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("text");
	i->code = INTRINSIC_LAMBDA {
		// LoadFileText already returns a MiniScript String, so explicit unload is a no-op.
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadFileText", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("text");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SaveFileText");
		return IntrinsicResult(false);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		String text = context->GetVar(String("text")).ToString();
		return IntrinsicResult(SaveFileText(fileName.c_str(), text.c_str()));
	};
	raylibModule.SetValue("SaveFileText", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("callback", Value::null);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetLoadFileDataCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context->GetVar(String("callback"));
		if (callback.IsNull()) {
			g_callbackBridgeState.loadFileDataCallback = Value::null;
			SetLoadFileDataCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetLoadFileDataCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = (context->vm != nullptr) ? context->vm->interpreter : nullptr;
		if (g_callbackBridgeState.interpreter == nullptr || g_callbackBridgeState.interpreter->vm == nullptr) {
			TraceLog(LOG_ERROR, "SetLoadFileDataCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.loadFileDataCallback = callback;
		SetLoadFileDataCallback(MiniScriptLoadFileDataBridge);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetLoadFileDataCallback", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("callback", Value::null);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetSaveFileDataCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context->GetVar(String("callback"));
		if (callback.IsNull()) {
			g_callbackBridgeState.saveFileDataCallback = Value::null;
			SetSaveFileDataCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetSaveFileDataCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = (context->vm != nullptr) ? context->vm->interpreter : nullptr;
		if (g_callbackBridgeState.interpreter == nullptr || g_callbackBridgeState.interpreter->vm == nullptr) {
			TraceLog(LOG_ERROR, "SetSaveFileDataCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.saveFileDataCallback = callback;
		SetSaveFileDataCallback(MiniScriptSaveFileDataBridge);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetSaveFileDataCallback", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("callback", Value::null);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetLoadFileTextCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context->GetVar(String("callback"));
		if (callback.IsNull()) {
			g_callbackBridgeState.loadFileTextCallback = Value::null;
			SetLoadFileTextCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetLoadFileTextCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = (context->vm != nullptr) ? context->vm->interpreter : nullptr;
		if (g_callbackBridgeState.interpreter == nullptr || g_callbackBridgeState.interpreter->vm == nullptr) {
			TraceLog(LOG_ERROR, "SetLoadFileTextCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.loadFileTextCallback = callback;
		SetLoadFileTextCallback(MiniScriptLoadFileTextBridge);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetLoadFileTextCallback", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("callback", Value::null);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetSaveFileTextCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context->GetVar(String("callback"));
		if (callback.IsNull()) {
			g_callbackBridgeState.saveFileTextCallback = Value::null;
			SetSaveFileTextCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			TraceLog(LOG_ERROR, "SetSaveFileTextCallback: callback must be function or null.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = (context->vm != nullptr) ? context->vm->interpreter : nullptr;
		if (g_callbackBridgeState.interpreter == nullptr || g_callbackBridgeState.interpreter->vm == nullptr) {
			TraceLog(LOG_ERROR, "SetSaveFileTextCallback: interpreter not available.");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.saveFileTextCallback = callback;
		SetSaveFileTextCallback(MiniScriptSaveFileTextBridge);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetSaveFileTextCallback", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("fileRename");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileRename");
		return IntrinsicResult(-1);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		String fileRename = context->GetVar(String("fileRename")).ToString();
		return IntrinsicResult(FileRename(fileName.c_str(), fileRename.c_str()));
	};
	raylibModule.SetValue("FileRename", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileRemove");
		return IntrinsicResult(-1);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(FileRemove(fileName.c_str()));
	};
	raylibModule.SetValue("FileRemove", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("srcPath");
	i->AddParam("dstPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileCopy");
		return IntrinsicResult(-1);
	#endif
		String srcPath = context->GetVar(String("srcPath")).ToString();
		String dstPath = context->GetVar(String("dstPath")).ToString();
		return IntrinsicResult(FileCopy(srcPath.c_str(), dstPath.c_str()));
	};
	raylibModule.SetValue("FileCopy", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("srcPath");
	i->AddParam("dstPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileMove");
		return IntrinsicResult(-1);
	#endif
		String srcPath = context->GetVar(String("srcPath")).ToString();
		String dstPath = context->GetVar(String("dstPath")).ToString();
		return IntrinsicResult(FileMove(srcPath.c_str(), dstPath.c_str()));
	};
	raylibModule.SetValue("FileMove", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("search");
	i->AddParam("replacement");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileTextReplace");
		return IntrinsicResult(-1);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		String search = context->GetVar(String("search")).ToString();
		String replacement = context->GetVar(String("replacement")).ToString();
		return IntrinsicResult(FileTextReplace(fileName.c_str(), search.c_str(), replacement.c_str()));
	};
	raylibModule.SetValue("FileTextReplace", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("search");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileTextFindIndex");
		return IntrinsicResult(-1);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		String search = context->GetVar(String("search")).ToString();
		return IntrinsicResult(FileTextFindIndex(fileName.c_str(), search.c_str()));
	};
	raylibModule.SetValue("FileTextFindIndex", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("FileExists");
		return IntrinsicResult(false);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(FileExists(fileName.c_str()));
	};
	raylibModule.SetValue("FileExists", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dirPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("DirectoryExists");
		return IntrinsicResult(false);
	#endif
		String dirPath = context->GetVar(String("dirPath")).ToString();
		return IntrinsicResult(DirectoryExists(dirPath.c_str()));
	};
	raylibModule.SetValue("DirectoryExists", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetFileLength");
		return IntrinsicResult(Value::zero);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(GetFileLength(fileName.c_str()));
	};
	raylibModule.SetValue("GetFileLength", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetFileModTime");
		return IntrinsicResult(Value::zero);
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult((double)GetFileModTime(fileName.c_str()));
	};
	raylibModule.SetValue("GetFileModTime", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String fileName = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(String(GetFileExtension(fileName.c_str())));
	};
	raylibModule.SetValue("GetFileExtension", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("filePath");
	i->code = INTRINSIC_LAMBDA {
		String filePath = context->GetVar(String("filePath")).ToString();
		return IntrinsicResult(String(GetFileName(filePath.c_str())));
	};
	raylibModule.SetValue("GetFileName", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("filePath");
	i->code = INTRINSIC_LAMBDA {
		String filePath = context->GetVar(String("filePath")).ToString();
		return IntrinsicResult(String(GetFileNameWithoutExt(filePath.c_str())));
	};
	raylibModule.SetValue("GetFileNameWithoutExt", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("filePath");
	i->code = INTRINSIC_LAMBDA {
		String filePath = context->GetVar(String("filePath")).ToString();
		return IntrinsicResult(String(GetDirectoryPath(filePath.c_str())));
	};
	raylibModule.SetValue("GetDirectoryPath", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dirPath");
	i->code = INTRINSIC_LAMBDA {
		String dirPath = context->GetVar(String("dirPath")).ToString();
		return IntrinsicResult(String(GetPrevDirectoryPath(dirPath.c_str())));
	};
	raylibModule.SetValue("GetPrevDirectoryPath", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetWorkingDirectory");
		return IntrinsicResult(String());
	#endif
		return IntrinsicResult(String(GetWorkingDirectory()));
	};
	raylibModule.SetValue("GetWorkingDirectory", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetApplicationDirectory");
		return IntrinsicResult(String());
	#endif
		return IntrinsicResult(String(GetApplicationDirectory()));
	};
	raylibModule.SetValue("GetApplicationDirectory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dirPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("MakeDirectory");
		return IntrinsicResult(-1);
	#endif
		String dirPath = context->GetVar(String("dirPath")).ToString();
		return IntrinsicResult(MakeDirectory(dirPath.c_str()));
	};
	raylibModule.SetValue("MakeDirectory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dirPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ChangeDirectory");
		return IntrinsicResult(false);
	#endif
		String dirPath = context->GetVar(String("dirPath")).ToString();
		return IntrinsicResult(ChangeDirectory(dirPath.c_str()));
	};
	raylibModule.SetValue("ChangeDirectory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("path");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsPathFile");
		return IntrinsicResult(false);
	#endif
		String path = context->GetVar(String("path")).ToString();
		return IntrinsicResult(IsPathFile(path.c_str()));
	};
	raylibModule.SetValue("IsPathFile", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String fileName = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(IsFileNameValid(fileName.c_str()));
	};
	raylibModule.SetValue("IsFileNameValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dirPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadDirectoryFiles");
		return IntrinsicResult(Value(ValueList()));
	#endif
		String dirPath = context->GetVar(String("dirPath")).ToString();
		FilePathList files = LoadDirectoryFiles(dirPath.c_str());

		ValueList result;
		for (unsigned int n = 0; n < files.count; n++) result.Add(Value(String(files.paths[n])));
		UnloadDirectoryFiles(files);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadDirectoryFiles", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("basePath");
	i->AddParam("filter");
	i->AddParam("scanSubdirs", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadDirectoryFilesEx");
		return IntrinsicResult(Value(ValueList()));
	#endif
		String basePath = context->GetVar(String("basePath")).ToString();
		String filter = context->GetVar(String("filter")).ToString();
		bool scanSubdirs = context->GetVar(String("scanSubdirs")).IntValue() != 0;

		FilePathList files = LoadDirectoryFilesEx(basePath.c_str(), filter.c_str(), scanSubdirs);
		ValueList result;
		for (unsigned int n = 0; n < files.count; n++) result.Add(Value(String(files.paths[n])));
		UnloadDirectoryFiles(files);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadDirectoryFilesEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("files");
	i->code = INTRINSIC_LAMBDA {
		// LoadDirectoryFiles* wrappers return plain MiniScript lists and release native memory internally.
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadDirectoryFiles", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("IsFileDropped");
		return IntrinsicResult(false);
	#endif
		return IntrinsicResult(IsFileDropped());
	};
	raylibModule.SetValue("IsFileDropped", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadDroppedFiles");
		return IntrinsicResult(Value(ValueList()));
	#endif
		FilePathList files = LoadDroppedFiles();
		ValueList result;
		for (unsigned int n = 0; n < files.count; n++) result.Add(Value(String(files.paths[n])));
		UnloadDroppedFiles(files);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadDroppedFiles", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("files");
	i->code = INTRINSIC_LAMBDA {
		// LoadDroppedFiles wrapper returns a plain MiniScript list and releases native memory internally.
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadDroppedFiles", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dirPath");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetDirectoryFileCount");
		return IntrinsicResult(Value::zero);
	#endif
		String dirPath = context->GetVar(String("dirPath")).ToString();
		return IntrinsicResult((int)GetDirectoryFileCount(dirPath.c_str()));
	};
	raylibModule.SetValue("GetDirectoryFileCount", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("basePath");
	i->AddParam("filter");
	i->AddParam("scanSubdirs", Value::zero);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("GetDirectoryFileCountEx");
		return IntrinsicResult(Value::zero);
	#endif
		String basePath = context->GetVar(String("basePath")).ToString();
		String filter = context->GetVar(String("filter")).ToString();
		bool scanSubdirs = context->GetVar(String("scanSubdirs")).IntValue() != 0;
		return IntrinsicResult((int)GetDirectoryFileCountEx(basePath.c_str(), filter.c_str(), scanSubdirs));
	};
	raylibModule.SetValue("GetDirectoryFileCountEx", i->GetFunc());

	// Compression and encoding helpers

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

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
	};
	raylibModule.SetValue("CompressData", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("compData");
	i->AddParam("compDataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value compDataValue = context->GetVar(String("compData"));
		int compDataSize = context->GetVar(String("compDataSize")).IntValue();

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
	};
	raylibModule.SetValue("DecompressData", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("text");
	i->code = INTRINSIC_LAMBDA {
		String text = context->GetVar(String("text")).ToString();
		int outputSize = 0;
		unsigned char* decoded = DecodeDataBase64(text.c_str(), &outputSize);
		if (decoded == nullptr || outputSize <= 0) return IntrinsicResult::Null;

		Value result = BytesToRawDataValue(decoded, outputSize);
		MemFree(decoded);
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("DecodeDataBase64", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult(Value::zero);

		return IntrinsicResult((int)ComputeCRC32((unsigned char*)bytes, byteCount));
	};
	raylibModule.SetValue("ComputeCRC32", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		unsigned int* hash = ComputeMD5((unsigned char*)bytes, byteCount);
		if (hash == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < 4; n++) result.Add(Value((double)hash[n]));
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("ComputeMD5", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		unsigned int* hash = ComputeSHA1((unsigned char*)bytes, byteCount);
		if (hash == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < 5; n++) result.Add(Value((double)hash[n]));
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("ComputeSHA1", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("data");
	i->AddParam("dataSize", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Value dataValue = context->GetVar(String("data"));
		int dataSize = context->GetVar(String("dataSize")).IntValue();

		std::vector<unsigned char> scratch;
		const unsigned char* bytes = nullptr;
		int byteCount = 0;
		if (!GetBytesFromValue(dataValue, dataSize, scratch, &bytes, &byteCount)) return IntrinsicResult::Null;

		unsigned int* hash = ComputeSHA256((unsigned char*)bytes, byteCount);
		if (hash == nullptr) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < 8; n++) result.Add(Value((double)hash[n]));
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("ComputeSHA256", i->GetFunc());

	// Automation events functions

	i = Intrinsic::Create("");
	i->AddParam("fileName", String());
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("LoadAutomationEventList");
		return IntrinsicResult::Null;
	#endif
		String fileName = context->GetVar(String("fileName")).ToString();
		const char* fileNamePtr = fileName.LengthB() > 0 ? fileName.c_str() : nullptr;
		AutomationEventList list = LoadAutomationEventList(fileNamePtr);
		return IntrinsicResult(AutomationEventListToValue(list));
	};
	raylibModule.SetValue("LoadAutomationEventList", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("list");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("UnloadAutomationEventList");
		return IntrinsicResult::Null;
	#endif
		Value listValue = context->GetVar(String("list"));
		AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
		if (listPtr == nullptr) return IntrinsicResult::Null;

		UnloadAutomationEventList(*listPtr);
		delete listPtr;

		if (listValue.type == ValueType::Map) {
			ValueDict map = listValue.GetDict();
			map.SetValue(String("_handle"), Value::zero);
			map.SetValue(String("capacity"), Value::zero);
			map.SetValue(String("count"), Value::zero);
			map.SetValue(String("events"), Value(ValueList()));
		}

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadAutomationEventList", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("list");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("ExportAutomationEventList");
		return IntrinsicResult(false);
	#endif
		Value listValue = context->GetVar(String("list"));
		String fileName = context->GetVar(String("fileName")).ToString();

		AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
		if (listPtr != nullptr) return IntrinsicResult(ExportAutomationEventList(*listPtr, fileName.c_str()));

		if (listValue.type != ValueType::Map) return IntrinsicResult(false);

		ValueDict map = listValue.GetDict();
		Value eventsValue = map.Lookup(String("events"), Value::null);
		if (eventsValue.type != ValueType::List) return IntrinsicResult(false);

		ValueList eventsList = eventsValue.GetList();
		std::vector<AutomationEvent> events;
		events.reserve(eventsList.Count());
		for (int n = 0; n < eventsList.Count(); n++) events.push_back(ValueToAutomationEvent(eventsList[n]));

		AutomationEventList list = {0};
		list.count = (unsigned int)events.size();
		list.capacity = (unsigned int)events.size();
		list.events = events.empty() ? nullptr : events.data();
		return IntrinsicResult(ExportAutomationEventList(list, fileName.c_str()));
	};
	raylibModule.SetValue("ExportAutomationEventList", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("list", Value::null);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetAutomationEventList");
		return IntrinsicResult::Null;
	#endif
		Value listValue = context->GetVar(String("list"));
		if (listValue.IsNull()) {
			SetAutomationEventList(nullptr);
			return IntrinsicResult::Null;
		}

		AutomationEventList* listPtr = GetAutomationEventListPtr(listValue);
		if (listPtr == nullptr) return IntrinsicResult::Null;

		SetAutomationEventList(listPtr);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetAutomationEventList", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("frame");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetAutomationEventBaseFrame");
		return IntrinsicResult::Null;
	#endif
		int frame = context->GetVar(String("frame")).IntValue();
		SetAutomationEventBaseFrame(frame);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetAutomationEventBaseFrame", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("StartAutomationEventRecording");
		return IntrinsicResult::Null;
	#endif
		StartAutomationEventRecording();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("StartAutomationEventRecording", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("StopAutomationEventRecording");
		return IntrinsicResult::Null;
	#endif
		StopAutomationEventRecording();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("StopAutomationEventRecording", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("event");
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("PlayAutomationEvent");
		return IntrinsicResult::Null;
	#endif
		AutomationEvent event = ValueToAutomationEvent(context->GetVar(String("event")));
		PlayAutomationEvent(event);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("PlayAutomationEvent", i->GetFunc());

	// Random number generation
	i = Intrinsic::Create("");
	i->AddParam("seed");
	i->code = INTRINSIC_LAMBDA {
		unsigned int seed = (unsigned int)context->GetVar(String("seed")).IntValue();
		SetRandomSeed(seed);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetRandomSeed", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("min");
	i->AddParam("max");
	i->code = INTRINSIC_LAMBDA {
		int min = context->GetVar(String("min")).IntValue();
		int max = context->GetVar(String("max")).IntValue();
		return IntrinsicResult(GetRandomValue(min, max));
	};
	raylibModule.SetValue("GetRandomValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("count");
	i->AddParam("min");
	i->AddParam("max");
	i->code = INTRINSIC_LAMBDA {
		unsigned int count = (unsigned int)context->GetVar(String("count")).IntValue();
		int min = context->GetVar(String("min")).IntValue();
		int max = context->GetVar(String("max")).IntValue();

		int* sequence = LoadRandomSequence(count, min, max);
		if (sequence == nullptr) return IntrinsicResult(Value::null);

		ValueList result;
		for (unsigned int i = 0; i < count; i++) {
			result.Add(Value(sequence[i]));
		}
		UnloadRandomSequence(sequence);
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("LoadRandomSequence", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("sequence", Value::null);
	i->code = INTRINSIC_LAMBDA {
		// LoadRandomSequence wrapper returns a plain MiniScript list and unloads native memory immediately.
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadRandomSequence", i->GetFunc());

	// Logging and tracing
	i = Intrinsic::Create("");
	i->AddParam("logLevel");
	i->code = INTRINSIC_LAMBDA {
		int logLevel = context->GetVar(String("logLevel")).IntValue();
		SetTraceLogLevel(logLevel);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetTraceLogLevel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("callback", Value::null);
	i->code = INTRINSIC_LAMBDA {
	#ifdef PLATFORM_WEB
		PrintWebNotSupported("SetTraceLogCallback");
		return IntrinsicResult::Null;
	#endif
		Value callback = context->GetVar(String("callback"));
		if (callback.IsNull()) {
			g_callbackBridgeState.traceLogCallback = Value::null;
			g_callbackBridgeState.invokingTraceLogCallback = false;
			SetTraceLogCallback(nullptr);
			return IntrinsicResult::Null;
		}
		if (!IsFunctionOrNull(callback)) {
			fprintf(stderr, "SetTraceLogCallback: callback must be function or null.\n");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.interpreter = (context->vm != nullptr) ? context->vm->interpreter : nullptr;
		if (g_callbackBridgeState.interpreter == nullptr || g_callbackBridgeState.interpreter->vm == nullptr) {
			fprintf(stderr, "SetTraceLogCallback: interpreter not available.\n");
			return IntrinsicResult::Null;
		}

		g_callbackBridgeState.traceLogCallback = callback;
		g_callbackBridgeState.invokingTraceLogCallback = false;
		SetTraceLogCallback(MiniScriptTraceLogBridge);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetTraceLogCallback", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("logLevel");
	i->AddParam("text");
	i->code = INTRINSIC_LAMBDA {
		int logLevel = context->GetVar(String("logLevel")).IntValue();
		String text = context->GetVar(String("text")).ToString();
		TraceLog(logLevel, "%s", text.c_str());
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("TraceLog", i->GetFunc());
}
