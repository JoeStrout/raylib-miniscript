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
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"
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

static void SyncShaderValue(Value shaderValue, Shader shader) {
	if (shaderValue.type != ValueType::Map) return;
	ValueDict map = shaderValue.GetDict();
	map.SetValue(String("id"), Value((int)shader.id));
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

#if RAYLIB_VERSION_GT(5, 5) || !defined(PLATFORM_WEB)
	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Image image = GetClipboardImage();
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
		free(encoded);
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

	// Load text files
	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		const char *fileName = context->GetVar("fileName").GetString().c_str();
		char *text = LoadFileText(fileName);
		String ret(text);
		UnloadFileText(text);
		return IntrinsicResult(ret);
	};
	raylibModule.SetValue("LoadFileText", i->GetFunc());

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

	// Logging and tracing
	i = Intrinsic::Create("");
	i->AddParam("logLevel");
	i->code = INTRINSIC_LAMBDA {
		int logLevel = context->GetVar(String("logLevel")).IntValue();
		SetTraceLogLevel(logLevel);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetTraceLogLevel", i->GetFunc());
}
