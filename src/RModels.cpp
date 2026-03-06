#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
#include "raymath.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"
#include <vector>

using namespace MiniScript;

static Mesh* GetMeshPtr(Value value) {
	if (value.type != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (Mesh*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static Material* GetMaterialPtr(Value value) {
	if (value.type != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (Material*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static Model* GetModelPtr(Value value) {
	if (value.type != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (Model*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static ModelAnimation* GetModelAnimationArray(Value value, int* outCount) {
	*outCount = 0;

	Value first = value;
	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		if (list.Count() == 0) return nullptr;
		first = list[0];
	}

	if (first.type != ValueType::Map) return nullptr;

	ValueDict map = first.GetDict();
	ModelAnimation* arrayPtr = (ModelAnimation*)ValueToPointer(map.Lookup(String("_arrayHandle"), Value::zero));
	int arrayCount = map.Lookup(String("_arrayCount"), Value::zero).IntValue();
	if (arrayPtr == nullptr || arrayCount <= 0) return nullptr;

	*outCount = arrayCount;
	return arrayPtr;
}

static Value ModelAnimationArrayItemToValue(ModelAnimation* animations, int count, int index) {
	ValueDict map;
	map.SetValue(Value::magicIsA, ModelAnimationClass());
	map.SetValue(String("_handle"), PointerToValue(&animations[index]));
	map.SetValue(String("name"), Value(String(animations[index].name)));
	map.SetValue(String("boneCount"), Value(animations[index].boneCount));
	map.SetValue(String("keyframeCount"), Value(animations[index].keyframeCount));
	map.SetValue(String("_arrayHandle"), PointerToValue(animations));
	map.SetValue(String("_arrayCount"), Value(count));
	map.SetValue(String("_arrayIndex"), Value(index));
	return Value(map);
}

static void SyncMaterialShaderMetadata(Value materialValue, Shader shader) {
	if (materialValue.type != ValueType::Map) return;
	ValueDict map = materialValue.GetDict();
	map.SetValue(String("shaderId"), Value((int)shader.id));
	map.SetValue(String("shader"), ShaderToValue(shader));
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

void AddRModelsMethods(ValueDict raylibModule) {
	Intrinsic *i;

	// Basic geometric 3D drawing

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context->GetVar(String("startPos")));
		Vector3 endPos = ValueToVector3(context->GetVar(String("endPos")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLine3D(startPos, endPos, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLine3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPoint3D(position, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPoint3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radius", Value(1.0));
	i->AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("rotationAngle", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 center = ValueToVector3(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		Vector3 rotationAxis = ValueToVector3(context->GetVar(String("rotationAxis")));
		float rotationAngle = context->GetVar(String("rotationAngle")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircle3D(center, radius, rotationAxis, rotationAngle, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircle3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1");
	i->AddParam("v2");
	i->AddParam("v3");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		Vector3 v3 = ValueToVector3(context->GetVar(String("v3")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawTriangle3D(v1, v2, v3, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTriangle3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Value pointsValue = context->GetVar(String("points"));
		if (pointsValue.type != ValueType::List) return IntrinsicResult::Null;
		ValueList pointsList = pointsValue.GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		std::vector<Vector3> points;
		points.reserve(pointCount);
		for (int n = 0; n < pointCount; n++) points.push_back(ValueToVector3(pointsList[n]));

		Color color = ValueToColor(context->GetVar(String("color")));
		DrawTriangleStrip3D(points.data(), pointCount, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTriangleStrip3D", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("width", Value(1.0));
	i->AddParam("height", Value(1.0));
	i->AddParam("length", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float width = context->GetVar(String("width")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		float length = context->GetVar(String("length")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCube(position, width, height, length, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCube", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("size", Vector3ToValue(Vector3{1, 1, 1}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector3 size = ValueToVector3(context->GetVar(String("size")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCubeV(position, size, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCubeV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("width", Value(1.0));
	i->AddParam("height", Value(1.0));
	i->AddParam("length", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float width = context->GetVar(String("width")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		float length = context->GetVar(String("length")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCubeWires(position, width, height, length, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCubeWires", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("size", Vector3ToValue(Vector3{1, 1, 1}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector3 size = ValueToVector3(context->GetVar(String("size")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCubeWiresV(position, size, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCubeWiresV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("centerPos");
	i->AddParam("radius", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context->GetVar(String("centerPos")));
		float radius = context->GetVar(String("radius")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSphere(centerPos, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSphere", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("centerPos");
	i->AddParam("radius", Value(1.0));
	i->AddParam("rings", Value(16));
	i->AddParam("slices", Value(16));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context->GetVar(String("centerPos")));
		float radius = context->GetVar(String("radius")).FloatValue();
		int rings = context->GetVar(String("rings")).IntValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSphereEx(centerPos, radius, rings, slices, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSphereEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("centerPos");
	i->AddParam("radius", Value(1.0));
	i->AddParam("rings", Value(16));
	i->AddParam("slices", Value(16));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context->GetVar(String("centerPos")));
		float radius = context->GetVar(String("radius")).FloatValue();
		int rings = context->GetVar(String("rings")).IntValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSphereWires(centerPos, radius, rings, slices, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSphereWires", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("radiusTop", Value(1.0));
	i->AddParam("radiusBottom", Value(1.0));
	i->AddParam("height", Value(2.0));
	i->AddParam("slices", Value(16));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float radiusTop = context->GetVar(String("radiusTop")).FloatValue();
		float radiusBottom = context->GetVar(String("radiusBottom")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCylinder(position, radiusTop, radiusBottom, height, slices, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCylinder", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("startRadius", Value(1.0));
	i->AddParam("endRadius", Value(1.0));
	i->AddParam("sides", Value(16));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context->GetVar(String("startPos")));
		Vector3 endPos = ValueToVector3(context->GetVar(String("endPos")));
		float startRadius = context->GetVar(String("startRadius")).FloatValue();
		float endRadius = context->GetVar(String("endRadius")).FloatValue();
		int sides = context->GetVar(String("sides")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCylinderEx(startPos, endPos, startRadius, endRadius, sides, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCylinderEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position");
	i->AddParam("radiusTop", Value(1.0));
	i->AddParam("radiusBottom", Value(1.0));
	i->AddParam("height", Value(2.0));
	i->AddParam("slices", Value(16));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float radiusTop = context->GetVar(String("radiusTop")).FloatValue();
		float radiusBottom = context->GetVar(String("radiusBottom")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCylinderWires(position, radiusTop, radiusBottom, height, slices, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCylinderWires", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("startRadius", Value(1.0));
	i->AddParam("endRadius", Value(1.0));
	i->AddParam("sides", Value(16));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context->GetVar(String("startPos")));
		Vector3 endPos = ValueToVector3(context->GetVar(String("endPos")));
		float startRadius = context->GetVar(String("startRadius")).FloatValue();
		float endRadius = context->GetVar(String("endRadius")).FloatValue();
		int sides = context->GetVar(String("sides")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCylinderWiresEx(startPos, endPos, startRadius, endRadius, sides, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCylinderWiresEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("radius", Value(1.0));
	i->AddParam("slices", Value(16));
	i->AddParam("rings", Value(8));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context->GetVar(String("startPos")));
		Vector3 endPos = ValueToVector3(context->GetVar(String("endPos")));
		float radius = context->GetVar(String("radius")).FloatValue();
		int slices = context->GetVar(String("slices")).IntValue();
		int rings = context->GetVar(String("rings")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCapsule(startPos, endPos, radius, slices, rings, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCapsule", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("radius", Value(1.0));
	i->AddParam("slices", Value(16));
	i->AddParam("rings", Value(8));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context->GetVar(String("startPos")));
		Vector3 endPos = ValueToVector3(context->GetVar(String("endPos")));
		float radius = context->GetVar(String("radius")).FloatValue();
		int slices = context->GetVar(String("slices")).IntValue();
		int rings = context->GetVar(String("rings")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCapsuleWires(startPos, endPos, radius, slices, rings, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCapsuleWires", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("centerPos");
	i->AddParam("size", Vector2ToValue(Vector2{1, 1}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context->GetVar(String("centerPos")));
		Vector2 size = ValueToVector2(context->GetVar(String("size")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPlane(centerPos, size, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPlane", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("ray");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context->GetVar(String("ray")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRay(ray, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRay", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("slices", Value(10));
	i->AddParam("spacing", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		int slices = context->GetVar(String("slices")).IntValue();
		float spacing = context->GetVar(String("spacing")).FloatValue();
		DrawGrid(slices, spacing);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawGrid", i->GetFunc());

	// Billboards

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->AddParam("texture");
	i->AddParam("position");
	i->AddParam("scale", Value(1.0));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Camera camera = ValueToCamera3D(context->GetVar(String("camera")));
		Texture texture = ValueToTexture(context->GetVar(String("texture")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float scale = context->GetVar(String("scale")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawBillboard(camera, texture, position, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawBillboard", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->AddParam("texture");
	i->AddParam("source");
	i->AddParam("position");
	i->AddParam("size", Vector2ToValue(Vector2{1, 1}));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Camera camera = ValueToCamera3D(context->GetVar(String("camera")));
		Texture texture = ValueToTexture(context->GetVar(String("texture")));
		Rectangle source = ValueToRectangle(context->GetVar(String("source")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector2 size = ValueToVector2(context->GetVar(String("size")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawBillboardRec(camera, texture, source, position, size, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawBillboardRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("camera");
	i->AddParam("texture");
	i->AddParam("source");
	i->AddParam("position");
	i->AddParam("up", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("size", Vector2ToValue(Vector2{1, 1}));
	i->AddParam("origin", Vector2ToValue(Vector2{0.5, 0.5}));
	i->AddParam("rotation", Value::zero);
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Camera camera = ValueToCamera3D(context->GetVar(String("camera")));
		Texture texture = ValueToTexture(context->GetVar(String("texture")));
		Rectangle source = ValueToRectangle(context->GetVar(String("source")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector3 up = ValueToVector3(context->GetVar(String("up")));
		Vector2 size = ValueToVector2(context->GetVar(String("size")));
		Vector2 origin = ValueToVector2(context->GetVar(String("origin")));
		float rotation = context->GetVar(String("rotation")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawBillboardPro(camera, texture, source, position, up, size, origin, rotation, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawBillboardPro", i->GetFunc());

	// Model management

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		Model model = LoadModel(path.c_str());
		if (!IsModelValid(model)) return IntrinsicResult::Null;
		return IntrinsicResult(ModelToValue(model));
	};
	raylibModule.SetValue("LoadModel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->code = INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		Model model = LoadModelFromMesh(mesh);
		if (!IsModelValid(model)) return IntrinsicResult::Null;
		return IntrinsicResult(ModelToValue(model));
	};
	raylibModule.SetValue("LoadModelFromMesh", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		return IntrinsicResult(IsModelValid(model));
	};
	raylibModule.SetValue("IsModelValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->code = INTRINSIC_LAMBDA {
		Value modelValue = context->GetVar(String("model"));
		Model model = ValueToModel(modelValue);
		UnloadModel(model);

		Model* modelPtr = GetModelPtr(modelValue);
		if (modelPtr != nullptr) delete modelPtr;

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadModel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		BoundingBox bounds = GetModelBoundingBox(model);
		return IntrinsicResult(BoundingBoxToValue(bounds));
	};
	raylibModule.SetValue("GetModelBoundingBox", i->GetFunc());

	// Model drawing

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("scale", Value(1.0));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float scale = context->GetVar(String("scale")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawModel(model, position, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawModel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("rotationAngle", Value::zero);
	i->AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector3 rotationAxis = ValueToVector3(context->GetVar(String("rotationAxis")));
		float rotationAngle = context->GetVar(String("rotationAngle")).FloatValue();
		Vector3 scale = ValueToVector3(context->GetVar(String("scale")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawModelEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("scale", Value(1.0));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float scale = context->GetVar(String("scale")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawModelWires(model, position, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawModelWires", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("rotationAngle", Value::zero);
	i->AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector3 rotationAxis = ValueToVector3(context->GetVar(String("rotationAxis")));
		float rotationAngle = context->GetVar(String("rotationAngle")).FloatValue();
		Vector3 scale = ValueToVector3(context->GetVar(String("scale")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawModelWiresEx(model, position, rotationAxis, rotationAngle, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawModelWiresEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("scale", Value(1.0));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		float scale = context->GetVar(String("scale")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawModelPoints(model, position, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawModelPoints", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("rotationAngle", Value::zero);
	i->AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		Vector3 position = ValueToVector3(context->GetVar(String("position")));
		Vector3 rotationAxis = ValueToVector3(context->GetVar(String("rotationAxis")));
		float rotationAngle = context->GetVar(String("rotationAngle")).FloatValue();
		Vector3 scale = ValueToVector3(context->GetVar(String("scale")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawModelPointsEx(model, position, rotationAxis, rotationAngle, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawModelPointsEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("box");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		BoundingBox box = ValueToBoundingBox(context->GetVar(String("box")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawBoundingBox(box, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawBoundingBox", i->GetFunc());

	// Mesh management

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->AddParam("dynamic", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Mesh* meshPtr = GetMeshPtr(context->GetVar(String("mesh")));
		if (meshPtr == nullptr) return IntrinsicResult::Null;
		bool dynamic = context->GetVar(String("dynamic")).IntValue() != 0;
		UploadMesh(meshPtr, dynamic);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UploadMesh", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->AddParam("index");
	i->AddParam("data");
	i->AddParam("offset", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Mesh* meshPtr = GetMeshPtr(context->GetVar(String("mesh")));
		if (meshPtr == nullptr) return IntrinsicResult::Null;

		int index = context->GetVar(String("index")).IntValue();
		int offset = context->GetVar(String("offset")).IntValue();
		Value dataVal = context->GetVar(String("data"));

		if (dataVal.type == ValueType::Map) {
			BinaryData* rawData = ValueToRawData(dataVal);
			if (rawData == nullptr || rawData->bytes == nullptr || rawData->length <= 0) return IntrinsicResult::Null;
			UpdateMeshBuffer(*meshPtr, index, rawData->bytes, rawData->length, offset);
			return IntrinsicResult::Null;
		}

		if (dataVal.type == ValueType::List) {
			ValueList list = dataVal.GetList();
			if (list.Count() == 0) return IntrinsicResult::Null;

			std::vector<float> data;
			data.reserve(list.Count());
			for (int n = 0; n < list.Count(); n++) data.push_back(list[n].FloatValue());

			UpdateMeshBuffer(*meshPtr, index, data.data(), (int)(data.size()*sizeof(float)), offset);
			return IntrinsicResult::Null;
		}

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateMeshBuffer", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->code = INTRINSIC_LAMBDA {
		Value meshValue = context->GetVar(String("mesh"));
		Mesh mesh = ValueToMesh(meshValue);
		UnloadMesh(mesh);

		Mesh* meshPtr = GetMeshPtr(meshValue);
		if (meshPtr != nullptr) delete meshPtr;

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadMesh", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->AddParam("material");
	i->AddParam("transform", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		Material material = ValueToMaterial(context->GetVar(String("material")));
		Matrix transform = ValueToMatrix(context->GetVar(String("transform")));
		DrawMesh(mesh, material, transform);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawMesh", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->AddParam("material");
	i->AddParam("transforms");
	i->code = INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		Material material = ValueToMaterial(context->GetVar(String("material")));
		Value transformsValue = context->GetVar(String("transforms"));
		if (transformsValue.type != ValueType::List) return IntrinsicResult::Null;

		ValueList transformsList = transformsValue.GetList();
		int instances = transformsList.Count();
		if (instances <= 0) return IntrinsicResult::Null;

		std::vector<Matrix> transforms;
		transforms.reserve(instances);
		for (int n = 0; n < instances; n++) {
			transforms.push_back(ValueToMatrix(transformsList[n]));
		}

		DrawMeshInstanced(mesh, material, transforms.data(), instances);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawMeshInstanced", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->code = INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		BoundingBox bounds = GetMeshBoundingBox(mesh);
		return IntrinsicResult(BoundingBoxToValue(bounds));
	};
	raylibModule.SetValue("GetMeshBoundingBox", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->code = INTRINSIC_LAMBDA {
		Mesh* meshPtr = GetMeshPtr(context->GetVar(String("mesh")));
		if (meshPtr == nullptr) return IntrinsicResult::Null;
		GenMeshTangents(meshPtr);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("GenMeshTangents", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		String path = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(ExportMesh(mesh, path.c_str()));
	};
	raylibModule.SetValue("ExportMesh", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mesh");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		String path = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(ExportMeshAsCode(mesh, path.c_str()));
	};
	raylibModule.SetValue("ExportMeshAsCode", i->GetFunc());

	// Mesh generation

	i = Intrinsic::Create("");
	i->AddParam("sides", Value(6));
	i->AddParam("radius", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		int sides = context->GetVar(String("sides")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		Mesh mesh = GenMeshPoly(sides, radius);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshPoly", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(1.0));
	i->AddParam("length", Value(1.0));
	i->AddParam("resX", Value(1));
	i->AddParam("resZ", Value(1));
	i->code = INTRINSIC_LAMBDA {
		float width = context->GetVar(String("width")).FloatValue();
		float length = context->GetVar(String("length")).FloatValue();
		int resX = context->GetVar(String("resX")).IntValue();
		int resZ = context->GetVar(String("resZ")).IntValue();
		Mesh mesh = GenMeshPlane(width, length, resX, resZ);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshPlane", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(1.0));
	i->AddParam("height", Value(1.0));
	i->AddParam("length", Value(1.0));
	i->code = INTRINSIC_LAMBDA {
		float width = context->GetVar(String("width")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		float length = context->GetVar(String("length")).FloatValue();
		Mesh mesh = GenMeshCube(width, height, length);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshCube", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("radius", Value(1.0));
	i->AddParam("rings", Value(16));
	i->AddParam("slices", Value(16));
	i->code = INTRINSIC_LAMBDA {
		float radius = context->GetVar(String("radius")).FloatValue();
		int rings = context->GetVar(String("rings")).IntValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Mesh mesh = GenMeshSphere(radius, rings, slices);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshSphere", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("radius", Value(1.0));
	i->AddParam("rings", Value(16));
	i->AddParam("slices", Value(16));
	i->code = INTRINSIC_LAMBDA {
		float radius = context->GetVar(String("radius")).FloatValue();
		int rings = context->GetVar(String("rings")).IntValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Mesh mesh = GenMeshHemiSphere(radius, rings, slices);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshHemiSphere", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("radius", Value(1.0));
	i->AddParam("height", Value(2.0));
	i->AddParam("slices", Value(16));
	i->code = INTRINSIC_LAMBDA {
		float radius = context->GetVar(String("radius")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Mesh mesh = GenMeshCylinder(radius, height, slices);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshCylinder", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("radius", Value(1.0));
	i->AddParam("height", Value(2.0));
	i->AddParam("slices", Value(16));
	i->code = INTRINSIC_LAMBDA {
		float radius = context->GetVar(String("radius")).FloatValue();
		float height = context->GetVar(String("height")).FloatValue();
		int slices = context->GetVar(String("slices")).IntValue();
		Mesh mesh = GenMeshCone(radius, height, slices);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshCone", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("radius", Value(1.0));
	i->AddParam("size", Value(0.5));
	i->AddParam("radSeg", Value(16));
	i->AddParam("sides", Value(16));
	i->code = INTRINSIC_LAMBDA {
		float radius = context->GetVar(String("radius")).FloatValue();
		float size = context->GetVar(String("size")).FloatValue();
		int radSeg = context->GetVar(String("radSeg")).IntValue();
		int sides = context->GetVar(String("sides")).IntValue();
		Mesh mesh = GenMeshTorus(radius, size, radSeg, sides);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshTorus", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("radius", Value(1.0));
	i->AddParam("size", Value(0.5));
	i->AddParam("radSeg", Value(16));
	i->AddParam("sides", Value(16));
	i->code = INTRINSIC_LAMBDA {
		float radius = context->GetVar(String("radius")).FloatValue();
		float size = context->GetVar(String("size")).FloatValue();
		int radSeg = context->GetVar(String("radSeg")).IntValue();
		int sides = context->GetVar(String("sides")).IntValue();
		Mesh mesh = GenMeshKnot(radius, size, radSeg, sides);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshKnot", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("heightmap");
	i->AddParam("size", Vector3ToValue(Vector3{1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("heightmap")));
		Vector3 size = ValueToVector3(context->GetVar(String("size")));
		Mesh mesh = GenMeshHeightmap(image, size);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshHeightmap", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("cubicmap");
	i->AddParam("cubeSize", Vector3ToValue(Vector3{1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("cubicmap")));
		Vector3 cubeSize = ValueToVector3(context->GetVar(String("cubeSize")));
		Mesh mesh = GenMeshCubicmap(image, cubeSize);
		return IntrinsicResult(MeshToValue(mesh));
	};
	raylibModule.SetValue("GenMeshCubicmap", i->GetFunc());

	// Material management

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		int count = 0;
		Material* materials = LoadMaterials(path.c_str(), &count);
		if (materials == nullptr || count <= 0) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < count; n++) {
			result.Add(MaterialToValue(materials[n]));
		}

		MemFree(materials);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadMaterials", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Material material = LoadMaterialDefault();
		return IntrinsicResult(MaterialToValue(material));
	};
	raylibModule.SetValue("LoadMaterialDefault", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->code = INTRINSIC_LAMBDA {
		Material material = ValueToMaterial(context->GetVar(String("material")));
		return IntrinsicResult(IsMaterialValid(material));
	};
	raylibModule.SetValue("IsMaterialValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->code = INTRINSIC_LAMBDA {
		Value materialValue = context->GetVar(String("material"));
		Material material = ValueToMaterial(materialValue);
		UnloadMaterial(material);

		Material* materialPtr = GetMaterialPtr(materialValue);
		if (materialPtr != nullptr) delete materialPtr;

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadMaterial", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("mapType");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int mapType = context->GetVar(String("mapType")).IntValue();
		Texture texture = ValueToTexture(context->GetVar(String("texture")));
		SetMaterialTexture(materialPtr, mapType, texture);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMaterialTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->code = INTRINSIC_LAMBDA {
		Value materialValue = context->GetVar(String("material"));
		Material* materialPtr = GetMaterialPtr(materialValue);
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		Value shaderValue = ShaderToValue(materialPtr->shader);
		SyncMaterialShaderMetadata(materialValue, materialPtr->shader);
		return IntrinsicResult(shaderValue);
	};
	raylibModule.SetValue("GetMaterialShader", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("shader");
	i->code = INTRINSIC_LAMBDA {
		Value materialValue = context->GetVar(String("material"));
		Material* materialPtr = GetMaterialPtr(materialValue);
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		Shader shader = ValueToShader(context->GetVar(String("shader")));
		materialPtr->shader = shader;
		SyncMaterialShaderMetadata(materialValue, shader);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMaterialShader", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("uniformName");
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult(-1);

		String uniformName = context->GetVar(String("uniformName")).ToString();
		return IntrinsicResult(GetShaderLocation(materialPtr->shader, uniformName.c_str()));
	};
	raylibModule.SetValue("GetMaterialShaderLocation", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("attribName");
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult(-1);

		String attribName = context->GetVar(String("attribName")).ToString();
		return IntrinsicResult(GetShaderLocationAttrib(materialPtr->shader, attribName.c_str()));
	};
	raylibModule.SetValue("GetMaterialShaderLocationAttrib", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("locIndex");
	i->AddParam("value");
	i->AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Value value = context->GetVar(String("value"));
		int uniformType = context->GetVar(String("uniformType")).IntValue();

		BinaryData* rawData = nullptr;
		if (value.type == ValueType::Map) rawData = ValueToRawData(value);
		if (rawData != nullptr && rawData->bytes != nullptr && rawData->length > 0) {
			SetShaderValue(materialPtr->shader, locIndex, rawData->bytes, uniformType);
			return IntrinsicResult::Null;
		}

		int components = ShaderUniformComponentCount(uniformType);
		if (IsShaderUniformFloatType(uniformType)) {
			std::vector<float> packed;
			int count = 1;
			PackFloatUniformData(value, components, count, packed);
			SetShaderValue(materialPtr->shader, locIndex, packed.data(), uniformType);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformIntType(uniformType)) {
			std::vector<int> packed;
			int count = 1;
			PackIntUniformData(value, components, count, packed);
			SetShaderValue(materialPtr->shader, locIndex, packed.data(), uniformType);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformUIntType(uniformType)) {
			std::vector<unsigned int> packed;
			int count = 1;
			PackUIntUniformData(value, components, count, packed);
			SetShaderValue(materialPtr->shader, locIndex, packed.data(), uniformType);
			return IntrinsicResult::Null;
		}

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMaterialShaderValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("locIndex");
	i->AddParam("value");
	i->AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i->AddParam("count", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Value value = context->GetVar(String("value"));
		int uniformType = context->GetVar(String("uniformType")).IntValue();
		int count = context->GetVar(String("count")).IntValue();

		BinaryData* rawData = nullptr;
		if (value.type == ValueType::Map) rawData = ValueToRawData(value);
		if (rawData != nullptr && rawData->bytes != nullptr && rawData->length > 0) {
			if (count <= 0) count = 1;
			SetShaderValueV(materialPtr->shader, locIndex, rawData->bytes, uniformType, count);
			return IntrinsicResult::Null;
		}

		int components = ShaderUniformComponentCount(uniformType);
		if (IsShaderUniformFloatType(uniformType)) {
			std::vector<float> packed;
			PackFloatUniformData(value, components, count, packed);
			SetShaderValueV(materialPtr->shader, locIndex, packed.data(), uniformType, count);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformIntType(uniformType)) {
			std::vector<int> packed;
			PackIntUniformData(value, components, count, packed);
			SetShaderValueV(materialPtr->shader, locIndex, packed.data(), uniformType, count);
			return IntrinsicResult::Null;
		}

		if (IsShaderUniformUIntType(uniformType)) {
			std::vector<unsigned int> packed;
			PackUIntUniformData(value, components, count, packed);
			SetShaderValueV(materialPtr->shader, locIndex, packed.data(), uniformType, count);
			return IntrinsicResult::Null;
		}

		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMaterialShaderValueV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("locIndex");
	i->AddParam("mat");
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		SetShaderValueMatrix(materialPtr->shader, locIndex, mat);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMaterialShaderValueMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("material");
	i->AddParam("locIndex");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context->GetVar(String("material")));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context->GetVar(String("locIndex")).IntValue();
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		SetShaderValueTexture(materialPtr->shader, locIndex, texture);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetMaterialShaderValueTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("meshId");
	i->AddParam("materialId");
	i->code = INTRINSIC_LAMBDA {
		Model* modelPtr = GetModelPtr(context->GetVar(String("model")));
		if (modelPtr == nullptr) return IntrinsicResult::Null;

		int meshId = context->GetVar(String("meshId")).IntValue();
		int materialId = context->GetVar(String("materialId")).IntValue();
		SetModelMeshMaterial(modelPtr, meshId, materialId);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetModelMeshMaterial", i->GetFunc());

	// Model animations

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		int animCount = 0;
		ModelAnimation* animations = LoadModelAnimations(path.c_str(), &animCount);
		if (animations == nullptr || animCount <= 0) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < animCount; n++) {
			result.Add(ModelAnimationArrayItemToValue(animations, animCount, n));
		}

		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadModelAnimations", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("animation");
	i->AddParam("frame", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		ModelAnimation animation = ValueToModelAnimation(context->GetVar(String("animation")));
		float frame = context->GetVar(String("frame")).FloatValue();
		UpdateModelAnimation(model, animation, frame);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateModelAnimation", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("animationA");
	i->AddParam("frameA", Value::zero);
	i->AddParam("animationB");
	i->AddParam("frameB", Value::zero);
	i->AddParam("blend", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		ModelAnimation animationA = ValueToModelAnimation(context->GetVar(String("animationA")));
		float frameA = context->GetVar(String("frameA")).FloatValue();
		ModelAnimation animationB = ValueToModelAnimation(context->GetVar(String("animationB")));
		float frameB = context->GetVar(String("frameB")).FloatValue();
		float blend = context->GetVar(String("blend")).FloatValue();
		UpdateModelAnimationEx(model, animationA, frameA, animationB, frameB, blend);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateModelAnimationEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("animations");
	i->code = INTRINSIC_LAMBDA {
		Value animationsValue = context->GetVar(String("animations"));
		int animCount = 0;
		ModelAnimation* animations = GetModelAnimationArray(animationsValue, &animCount);
		if (animations == nullptr || animCount <= 0) return IntrinsicResult::Null;

		UnloadModelAnimations(animations, animCount);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadModelAnimations", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("model");
	i->AddParam("animation");
	i->code = INTRINSIC_LAMBDA {
		Model model = ValueToModel(context->GetVar(String("model")));
		ModelAnimation animation = ValueToModelAnimation(context->GetVar(String("animation")));
		return IntrinsicResult(IsModelAnimationValid(model, animation));
	};
	raylibModule.SetValue("IsModelAnimationValid", i->GetFunc());

	// Collision detection

	i = Intrinsic::Create("");
	i->AddParam("center1");
	i->AddParam("radius1");
	i->AddParam("center2");
	i->AddParam("radius2");
	i->code = INTRINSIC_LAMBDA {
		Vector3 center1 = ValueToVector3(context->GetVar(String("center1")));
		float radius1 = context->GetVar(String("radius1")).FloatValue();
		Vector3 center2 = ValueToVector3(context->GetVar(String("center2")));
		float radius2 = context->GetVar(String("radius2")).FloatValue();
		return IntrinsicResult(CheckCollisionSpheres(center1, radius1, center2, radius2));
	};
	raylibModule.SetValue("CheckCollisionSpheres", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("box1");
	i->AddParam("box2");
	i->code = INTRINSIC_LAMBDA {
		BoundingBox box1 = ValueToBoundingBox(context->GetVar(String("box1")));
		BoundingBox box2 = ValueToBoundingBox(context->GetVar(String("box2")));
		return IntrinsicResult(CheckCollisionBoxes(box1, box2));
	};
	raylibModule.SetValue("CheckCollisionBoxes", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("box");
	i->AddParam("center");
	i->AddParam("radius");
	i->code = INTRINSIC_LAMBDA {
		BoundingBox box = ValueToBoundingBox(context->GetVar(String("box")));
		Vector3 center = ValueToVector3(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		return IntrinsicResult(CheckCollisionBoxSphere(box, center, radius));
	};
	raylibModule.SetValue("CheckCollisionBoxSphere", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("ray");
	i->AddParam("center");
	i->AddParam("radius");
	i->code = INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context->GetVar(String("ray")));
		Vector3 center = ValueToVector3(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		RayCollision result = GetRayCollisionSphere(ray, center, radius);
		return IntrinsicResult(RayCollisionToValue(result));
	};
	raylibModule.SetValue("GetRayCollisionSphere", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("ray");
	i->AddParam("box");
	i->code = INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context->GetVar(String("ray")));
		BoundingBox box = ValueToBoundingBox(context->GetVar(String("box")));
		RayCollision result = GetRayCollisionBox(ray, box);
		return IntrinsicResult(RayCollisionToValue(result));
	};
	raylibModule.SetValue("GetRayCollisionBox", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("ray");
	i->AddParam("mesh");
	i->AddParam("transform", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context->GetVar(String("ray")));
		Mesh mesh = ValueToMesh(context->GetVar(String("mesh")));
		Matrix transform = ValueToMatrix(context->GetVar(String("transform")));
		RayCollision result = GetRayCollisionMesh(ray, mesh, transform);
		return IntrinsicResult(RayCollisionToValue(result));
	};
	raylibModule.SetValue("GetRayCollisionMesh", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("ray");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->code = INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context->GetVar(String("ray")));
		Vector3 p1 = ValueToVector3(context->GetVar(String("p1")));
		Vector3 p2 = ValueToVector3(context->GetVar(String("p2")));
		Vector3 p3 = ValueToVector3(context->GetVar(String("p3")));
		RayCollision result = GetRayCollisionTriangle(ray, p1, p2, p3);
		return IntrinsicResult(RayCollisionToValue(result));
	};
	raylibModule.SetValue("GetRayCollisionTriangle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("ray");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("p4");
	i->code = INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context->GetVar(String("ray")));
		Vector3 p1 = ValueToVector3(context->GetVar(String("p1")));
		Vector3 p2 = ValueToVector3(context->GetVar(String("p2")));
		Vector3 p3 = ValueToVector3(context->GetVar(String("p3")));
		Vector3 p4 = ValueToVector3(context->GetVar(String("p4")));
		RayCollision result = GetRayCollisionQuad(ray, p1, p2, p3, p4);
		return IntrinsicResult(RayCollisionToValue(result));
	};
	raylibModule.SetValue("GetRayCollisionQuad", i->GetFunc());
}