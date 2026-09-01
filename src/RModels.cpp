#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "FileSystem.h"
#include "RawData.h"
#include "raylib.h"
#include "raymath.h"
#include "miniscript.h"
#include "macros.h"
#include <vector>

using namespace MiniScript;

static Mesh* GetMeshPtr(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (Mesh*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static Material* GetMaterialPtr(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (Material*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static Model* GetModelPtr(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (Model*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

static ModelAnimation* GetModelAnimationArray(Value value, int* outCount) {
	*outCount = 0;

	Value first = value;
	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		if (list.Count() == 0) return nullptr;
		first = list[0];
	}

	if (first.Type() != ValueType::Map) return nullptr;

	ValueDict map = first.GetDict();
	ModelAnimation* arrayPtr = (ModelAnimation*)ValueToPointer(map.Lookup(String("_arrayHandle"), Value::zero));
	int arrayCount = map.Lookup(String("_arrayCount"), Value::zero).IntValue();
	if (arrayPtr == nullptr || arrayCount <= 0) return nullptr;

	*outCount = arrayCount;
	return arrayPtr;
}

static Value ModelAnimationArrayItemToValue(ModelAnimation* animations, int count, int index) {
	rcModelAnimation++;
	ValueDict map;
	map.SetValue(Value::magicIsA, ModelAnimationClass());
	map.SetValue(String("_handle"), PointerToValue(&animations[index]));
	map.SetValue(String("name"), Value(String(animations[index].name)));
	map.SetValue(String("boneCount"), Value(animations[index].boneCount));
	map.SetValue(String("keyframeCount"), Value(animations[index].keyframeCount));
	map.SetValue(String("_arrayHandle"), PointerToValue(animations));
	map.SetValue(String("_arrayCount"), Value(count));
	map.SetValue(String("_arrayIndex"), Value(index));
	return DynamicMap(map);
}

static void SyncMaterialShaderMetadata(Value materialValue, Shader shader) {
	if (materialValue.Type() != ValueType::Map) return;
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

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) out[i] = list[i].FloatValue();
		return;
	}

	if (value.Type() == ValueType::Map) {
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

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int n = list.Count();
		if (n > components) n = components;
		for (int i = 0; i < n; i++) out[i] = list[i].IntValue();
		return;
	}

	if (value.Type() == ValueType::Map) {
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

void AddRModelsMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Basic geometric 3D drawing

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context.GetArg(0));
		Vector3 endPos = ValueToVector3(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		DrawLine3D(startPos, endPos, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLine3D", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawPoint3D(position, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPoint3D", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius", Value(1.0));
	i.AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("rotationAngle", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 center = ValueToVector3(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		Vector3 rotationAxis = ValueToVector3(context.GetArg(2));
		float rotationAngle = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawCircle3D(center, radius, rotationAxis, rotationAngle, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircle3D", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		Vector3 v3 = ValueToVector3(context.GetArg(2));
		Color color = ValueToColor(context.GetArg(3));
		DrawTriangle3D(v1, v2, v3, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangle3D", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Value pointsValue = context.GetArg(0);
		if (pointsValue.Type() != ValueType::List) return IntrinsicResult::Null;
		ValueList pointsList = pointsValue.GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		std::vector<Vector3> points;
		points.reserve(pointCount);
		for (int n = 0; n < pointCount; n++) points.push_back(ValueToVector3(pointsList[n]));

		Color color = ValueToColor(context.GetArg(1));
		DrawTriangleStrip3D(points.data(), pointCount, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangleStrip3D", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("width", Value(1.0));
	i.AddParam("height", Value(1.0));
	i.AddParam("length", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		float width = context.GetArg(1).FloatValue();
		float height = context.GetArg(2).FloatValue();
		float length = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawCube(position, width, height, length, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCube", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("size", Vector3ToValue(Vector3{1, 1, 1}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		Vector3 size = ValueToVector3(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		DrawCubeV(position, size, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCubeV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("width", Value(1.0));
	i.AddParam("height", Value(1.0));
	i.AddParam("length", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		float width = context.GetArg(1).FloatValue();
		float height = context.GetArg(2).FloatValue();
		float length = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawCubeWires(position, width, height, length, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCubeWires", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("size", Vector3ToValue(Vector3{1, 1, 1}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		Vector3 size = ValueToVector3(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		DrawCubeWiresV(position, size, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCubeWiresV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("centerPos");
	i.AddParam("radius", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawSphere(centerPos, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSphere", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("centerPos");
	i.AddParam("radius", Value(1.0));
	i.AddParam("rings", Value(16));
	i.AddParam("slices", Value(16));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		int rings = context.GetArg(2).IntValue();
		int slices = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawSphereEx(centerPos, radius, rings, slices, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSphereEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("centerPos");
	i.AddParam("radius", Value(1.0));
	i.AddParam("rings", Value(16));
	i.AddParam("slices", Value(16));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		int rings = context.GetArg(2).IntValue();
		int slices = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawSphereWires(centerPos, radius, rings, slices, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSphereWires", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("radiusTop", Value(1.0));
	i.AddParam("radiusBottom", Value(1.0));
	i.AddParam("height", Value(2.0));
	i.AddParam("slices", Value(16));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		float radiusTop = context.GetArg(1).FloatValue();
		float radiusBottom = context.GetArg(2).FloatValue();
		float height = context.GetArg(3).FloatValue();
		int slices = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCylinder(position, radiusTop, radiusBottom, height, slices, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCylinder", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("startRadius", Value(1.0));
	i.AddParam("endRadius", Value(1.0));
	i.AddParam("sides", Value(16));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context.GetArg(0));
		Vector3 endPos = ValueToVector3(context.GetArg(1));
		float startRadius = context.GetArg(2).FloatValue();
		float endRadius = context.GetArg(3).FloatValue();
		int sides = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCylinderEx(startPos, endPos, startRadius, endRadius, sides, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCylinderEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position");
	i.AddParam("radiusTop", Value(1.0));
	i.AddParam("radiusBottom", Value(1.0));
	i.AddParam("height", Value(2.0));
	i.AddParam("slices", Value(16));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 position = ValueToVector3(context.GetArg(0));
		float radiusTop = context.GetArg(1).FloatValue();
		float radiusBottom = context.GetArg(2).FloatValue();
		float height = context.GetArg(3).FloatValue();
		int slices = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCylinderWires(position, radiusTop, radiusBottom, height, slices, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCylinderWires", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("startRadius", Value(1.0));
	i.AddParam("endRadius", Value(1.0));
	i.AddParam("sides", Value(16));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context.GetArg(0));
		Vector3 endPos = ValueToVector3(context.GetArg(1));
		float startRadius = context.GetArg(2).FloatValue();
		float endRadius = context.GetArg(3).FloatValue();
		int sides = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCylinderWiresEx(startPos, endPos, startRadius, endRadius, sides, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCylinderWiresEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("radius", Value(1.0));
	i.AddParam("slices", Value(16));
	i.AddParam("rings", Value(8));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context.GetArg(0));
		Vector3 endPos = ValueToVector3(context.GetArg(1));
		float radius = context.GetArg(2).FloatValue();
		int slices = context.GetArg(3).IntValue();
		int rings = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCapsule(startPos, endPos, radius, slices, rings, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCapsule", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("radius", Value(1.0));
	i.AddParam("slices", Value(16));
	i.AddParam("rings", Value(8));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 startPos = ValueToVector3(context.GetArg(0));
		Vector3 endPos = ValueToVector3(context.GetArg(1));
		float radius = context.GetArg(2).FloatValue();
		int slices = context.GetArg(3).IntValue();
		int rings = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCapsuleWires(startPos, endPos, radius, slices, rings, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCapsuleWires", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("centerPos");
	i.AddParam("size", Vector2ToValue(Vector2{1, 1}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 centerPos = ValueToVector3(context.GetArg(0));
		Vector2 size = ValueToVector2(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		DrawPlane(centerPos, size, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPlane", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("ray");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawRay(ray, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRay", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("slices", Value(10));
	i.AddParam("spacing", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		int slices = context.GetArg(0).IntValue();
		float spacing = context.GetArg(1).FloatValue();
		DrawGrid(slices, spacing);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawGrid", i.GetFunc());

	// Billboards

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("texture");
	i.AddParam("position");
	i.AddParam("scale", Value(1.0));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Camera camera = ValueToCamera3D(context.GetArg(0));
		Texture texture = ValueToTexture(context.GetArg(1));
		Vector3 position = ValueToVector3(context.GetArg(2));
		float scale = context.GetArg(3).FloatValue();
		Color tint = ValueToColor(context.GetArg(4));
		DrawBillboard(camera, texture, position, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawBillboard", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("texture");
	i.AddParam("source");
	i.AddParam("position");
	i.AddParam("size", Vector2ToValue(Vector2{1, 1}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Camera camera = ValueToCamera3D(context.GetArg(0));
		Texture texture = ValueToTexture(context.GetArg(1));
		Rectangle source = ValueToRectangle(context.GetArg(2));
		Vector3 position = ValueToVector3(context.GetArg(3));
		Vector2 size = ValueToVector2(context.GetArg(4));
		Color tint = ValueToColor(context.GetArg(5));
		DrawBillboardRec(camera, texture, source, position, size, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawBillboardRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("camera");
	i.AddParam("texture");
	i.AddParam("source");
	i.AddParam("position");
	i.AddParam("up", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("size", Vector2ToValue(Vector2{1, 1}));
	i.AddParam("origin", Vector2ToValue(Vector2{0.5, 0.5}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Camera camera = ValueToCamera3D(context.GetArg(0));
		Texture texture = ValueToTexture(context.GetArg(1));
		Rectangle source = ValueToRectangle(context.GetArg(2));
		Vector3 position = ValueToVector3(context.GetArg(3));
		Vector3 up = ValueToVector3(context.GetArg(4));
		Vector2 size = ValueToVector2(context.GetArg(5));
		Vector2 origin = ValueToVector2(context.GetArg(6));
		float rotation = context.GetArg(7).FloatValue();
		Color tint = ValueToColor(context.GetArg(8));
		DrawBillboardPro(camera, texture, source, position, up, size, origin, rotation, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawBillboardPro", i.GetFunc());

	// Model management

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Model model = LoadModel(path.c_str());
		if (!IsModelValid(model)) return IntrinsicResult::Null;
		rcModel++;
		return IntrinsicResult(ModelToValue(model));
	});
	raylibModule.SetValue("LoadModel", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context.GetArg(0));
		Model model = LoadModelFromMesh(mesh);
		if (!IsModelValid(model)) return IntrinsicResult::Null;
		rcModel++;
		return IntrinsicResult(ModelToValue(model));
	});
	raylibModule.SetValue("LoadModelFromMesh", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		return IntrinsicResult(IsModelValid(model));
	});
	raylibModule.SetValue("IsModelValid", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.set_Code(INTRINSIC_LAMBDA {
		Value modelValue = context.GetArg(0);
		Model model = ValueToModel(modelValue);
		UnloadModel(model);

		Model* modelPtr = GetModelPtr(modelValue);
		if (modelPtr != nullptr) { delete modelPtr; rcModel--; }

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadModel", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		BoundingBox bounds = GetModelBoundingBox(model);
		return IntrinsicResult(BoundingBoxToValue(bounds));
	});
	raylibModule.SetValue("GetModelBoundingBox", i.GetFunc());

	// Model drawing

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("scale", Value(1.0));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		Vector3 position = ValueToVector3(context.GetArg(1));
		float scale = context.GetArg(2).FloatValue();
		Color tint = ValueToColor(context.GetArg(3));
		DrawModel(model, position, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawModel", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("rotationAngle", Value::zero);
	i.AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		Vector3 position = ValueToVector3(context.GetArg(1));
		Vector3 rotationAxis = ValueToVector3(context.GetArg(2));
		float rotationAngle = context.GetArg(3).FloatValue();
		Vector3 scale = ValueToVector3(context.GetArg(4));
		Color tint = ValueToColor(context.GetArg(5));
		DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawModelEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("scale", Value(1.0));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		Vector3 position = ValueToVector3(context.GetArg(1));
		float scale = context.GetArg(2).FloatValue();
		Color tint = ValueToColor(context.GetArg(3));
		DrawModelWires(model, position, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawModelWires", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("position", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("rotationAxis", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("rotationAngle", Value::zero);
	i.AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		Vector3 position = ValueToVector3(context.GetArg(1));
		Vector3 rotationAxis = ValueToVector3(context.GetArg(2));
		float rotationAngle = context.GetArg(3).FloatValue();
		Vector3 scale = ValueToVector3(context.GetArg(4));
		Color tint = ValueToColor(context.GetArg(5));
		DrawModelWiresEx(model, position, rotationAxis, rotationAngle, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawModelWiresEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("box");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		BoundingBox box = ValueToBoundingBox(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawBoundingBox(box, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawBoundingBox", i.GetFunc());

	// Mesh management

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.AddParam("dynamic", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh* meshPtr = GetMeshPtr(context.GetArg(0));
		if (meshPtr == nullptr) return IntrinsicResult::Null;
		bool dynamic = context.GetArg(1).IntValue() != 0;
		UploadMesh(meshPtr, dynamic);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UploadMesh", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.AddParam("index");
	i.AddParam("data");
	i.AddParam("offset", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh* meshPtr = GetMeshPtr(context.GetArg(0));
		if (meshPtr == nullptr) return IntrinsicResult::Null;

		int index = context.GetArg(1).IntValue();
		int offset = context.GetArg(3).IntValue();
		Value dataVal = context.GetArg(2);

		if (dataVal.Type() == ValueType::Map) {
			BinaryData* rawData = ValueToRawData(dataVal);
			if (rawData == nullptr || rawData->bytes == nullptr || rawData->length <= 0) return IntrinsicResult::Null;
			UpdateMeshBuffer(*meshPtr, index, rawData->bytes, rawData->length, offset);
			return IntrinsicResult::Null;
		}

		if (dataVal.Type() == ValueType::List) {
			ValueList list = dataVal.GetList();
			if (list.Count() == 0) return IntrinsicResult::Null;

			std::vector<float> data;
			data.reserve(list.Count());
			for (int n = 0; n < list.Count(); n++) data.push_back(list[n].FloatValue());

			UpdateMeshBuffer(*meshPtr, index, data.data(), (int)(data.size()*sizeof(float)), offset);
			return IntrinsicResult::Null;
		}

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateMeshBuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.set_Code(INTRINSIC_LAMBDA {
		Value meshValue = context.GetArg(0);
		Mesh mesh = ValueToMesh(meshValue);
		UnloadMesh(mesh);

		Mesh* meshPtr = GetMeshPtr(meshValue);
		if (meshPtr != nullptr) { delete meshPtr; rcMesh--; }

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadMesh", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.AddParam("material");
	i.AddParam("transform", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context.GetArg(0));
		Material material = ValueToMaterial(context.GetArg(1));
		Matrix transform = ValueToMatrix(context.GetArg(2));
		DrawMesh(mesh, material, transform);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawMesh", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.AddParam("material");
	i.AddParam("transforms");
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context.GetArg(0));
		Material material = ValueToMaterial(context.GetArg(1));
		Value transformsValue = context.GetArg(2);
		if (transformsValue.Type() != ValueType::List) return IntrinsicResult::Null;

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
	});
	raylibModule.SetValue("DrawMeshInstanced", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context.GetArg(0));
		BoundingBox bounds = GetMeshBoundingBox(mesh);
		return IntrinsicResult(BoundingBoxToValue(bounds));
	});
	raylibModule.SetValue("GetMeshBoundingBox", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh* meshPtr = GetMeshPtr(context.GetArg(0));
		if (meshPtr == nullptr) return IntrinsicResult::Null;
		GenMeshTangents(meshPtr);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("GenMeshTangents", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult::Zero;
		return IntrinsicResult(ExportMesh(mesh, path.c_str()));
	});
	raylibModule.SetValue("ExportMesh", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mesh");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Mesh mesh = ValueToMesh(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult::Zero;
		return IntrinsicResult(ExportMeshAsCode(mesh, path.c_str()));
	});
	raylibModule.SetValue("ExportMeshAsCode", i.GetFunc());

	// Mesh generation

	i = Intrinsic::Create("");
	i.AddParam("sides", Value(6));
	i.AddParam("radius", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		int sides = context.GetArg(0).IntValue();
		float radius = context.GetArg(1).FloatValue();
		Mesh mesh = GenMeshPoly(sides, radius);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshPoly", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(1.0));
	i.AddParam("length", Value(1.0));
	i.AddParam("resX", Value(1));
	i.AddParam("resZ", Value(1));
	i.set_Code(INTRINSIC_LAMBDA {
		float width = context.GetArg(0).FloatValue();
		float length = context.GetArg(1).FloatValue();
		int resX = context.GetArg(2).IntValue();
		int resZ = context.GetArg(3).IntValue();
		Mesh mesh = GenMeshPlane(width, length, resX, resZ);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshPlane", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(1.0));
	i.AddParam("height", Value(1.0));
	i.AddParam("length", Value(1.0));
	i.set_Code(INTRINSIC_LAMBDA {
		float width = context.GetArg(0).FloatValue();
		float height = context.GetArg(1).FloatValue();
		float length = context.GetArg(2).FloatValue();
		Mesh mesh = GenMeshCube(width, height, length);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshCube", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("radius", Value(1.0));
	i.AddParam("rings", Value(16));
	i.AddParam("slices", Value(16));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(0).FloatValue();
		int rings = context.GetArg(1).IntValue();
		int slices = context.GetArg(2).IntValue();
		Mesh mesh = GenMeshSphere(radius, rings, slices);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshSphere", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("radius", Value(1.0));
	i.AddParam("rings", Value(16));
	i.AddParam("slices", Value(16));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(0).FloatValue();
		int rings = context.GetArg(1).IntValue();
		int slices = context.GetArg(2).IntValue();
		Mesh mesh = GenMeshHemiSphere(radius, rings, slices);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshHemiSphere", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("radius", Value(1.0));
	i.AddParam("height", Value(2.0));
	i.AddParam("slices", Value(16));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(0).FloatValue();
		float height = context.GetArg(1).FloatValue();
		int slices = context.GetArg(2).IntValue();
		Mesh mesh = GenMeshCylinder(radius, height, slices);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshCylinder", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("radius", Value(1.0));
	i.AddParam("height", Value(2.0));
	i.AddParam("slices", Value(16));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(0).FloatValue();
		float height = context.GetArg(1).FloatValue();
		int slices = context.GetArg(2).IntValue();
		Mesh mesh = GenMeshCone(radius, height, slices);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshCone", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("radius", Value(1.0));
	i.AddParam("size", Value(0.5));
	i.AddParam("radSeg", Value(16));
	i.AddParam("sides", Value(16));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(0).FloatValue();
		float size = context.GetArg(1).FloatValue();
		int radSeg = context.GetArg(2).IntValue();
		int sides = context.GetArg(3).IntValue();
		Mesh mesh = GenMeshTorus(radius, size, radSeg, sides);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshTorus", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("radius", Value(1.0));
	i.AddParam("size", Value(0.5));
	i.AddParam("radSeg", Value(16));
	i.AddParam("sides", Value(16));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(0).FloatValue();
		float size = context.GetArg(1).FloatValue();
		int radSeg = context.GetArg(2).IntValue();
		int sides = context.GetArg(3).IntValue();
		Mesh mesh = GenMeshKnot(radius, size, radSeg, sides);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshKnot", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("heightmap");
	i.AddParam("size", Vector3ToValue(Vector3{1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		Vector3 size = ValueToVector3(context.GetArg(1));
		Mesh mesh = GenMeshHeightmap(image, size);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshHeightmap", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("cubicmap");
	i.AddParam("cubeSize", Vector3ToValue(Vector3{1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		Vector3 cubeSize = ValueToVector3(context.GetArg(1));
		Mesh mesh = GenMeshCubicmap(image, cubeSize);
		rcMesh++;
		return IntrinsicResult(MeshToValue(mesh));
	});
	raylibModule.SetValue("GenMeshCubicmap", i.GetFunc());

	// Material management

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		int count = 0;
		Material* materials = LoadMaterials(path.c_str(), &count);
		if (materials == nullptr || count <= 0) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < count; n++) {
			rcMaterial++;
			result.Add(MaterialToValue(materials[n]));
		}

		MemFree(materials);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadMaterials", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Material material = LoadMaterialDefault();
		rcMaterial++;
		return IntrinsicResult(MaterialToValue(material));
	});
	raylibModule.SetValue("LoadMaterialDefault", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.set_Code(INTRINSIC_LAMBDA {
		Material material = ValueToMaterial(context.GetArg(0));
		return IntrinsicResult(IsMaterialValid(material));
	});
	raylibModule.SetValue("IsMaterialValid", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.set_Code(INTRINSIC_LAMBDA {
		Value materialValue = context.GetArg(0);
		Material material = ValueToMaterial(materialValue);
		UnloadMaterial(material);

		Material* materialPtr = GetMaterialPtr(materialValue);
		if (materialPtr != nullptr) { delete materialPtr; rcMaterial--; }

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadMaterial", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("mapType");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int mapType = context.GetArg(1).IntValue();
		Texture texture = ValueToTexture(context.GetArg(2));
		SetMaterialTexture(materialPtr, mapType, texture);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMaterialTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.set_Code(INTRINSIC_LAMBDA {
		Value materialValue = context.GetArg(0);
		Material* materialPtr = GetMaterialPtr(materialValue);
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		Value shaderValue = ShaderToValue(materialPtr->shader);
		SyncMaterialShaderMetadata(materialValue, materialPtr->shader);
		return IntrinsicResult(shaderValue);
	});
	raylibModule.SetValue("GetMaterialShader", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("shader");
	i.set_Code(INTRINSIC_LAMBDA {
		Value materialValue = context.GetArg(0);
		Material* materialPtr = GetMaterialPtr(materialValue);
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		Shader shader = ValueToShader(context.GetArg(1));
		materialPtr->shader = shader;
		SyncMaterialShaderMetadata(materialValue, shader);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMaterialShader", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("uniformName");
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult(-1);

		String uniformName = context.GetArg(1).ToString();
		return IntrinsicResult(GetShaderLocation(materialPtr->shader, uniformName.c_str()));
	});
	raylibModule.SetValue("GetMaterialShaderLocation", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("attribName");
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult(-1);

		String attribName = context.GetArg(1).ToString();
		return IntrinsicResult(GetShaderLocationAttrib(materialPtr->shader, attribName.c_str()));
	});
	raylibModule.SetValue("GetMaterialShaderLocationAttrib", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("locIndex");
	i.AddParam("value");
	i.AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context.GetArg(1).IntValue();
		Value value = context.GetArg(2);
		int uniformType = context.GetArg(3).IntValue();

		BinaryData* rawData = nullptr;
		if (value.Type() == ValueType::Map) rawData = ValueToRawData(value);
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
	});
	raylibModule.SetValue("SetMaterialShaderValue", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("locIndex");
	i.AddParam("value");
	i.AddParam("uniformType", Value(SHADER_UNIFORM_FLOAT));
	i.AddParam("count", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context.GetArg(1).IntValue();
		Value value = context.GetArg(2);
		int uniformType = context.GetArg(3).IntValue();
		int count = context.GetArg(4).IntValue();

		BinaryData* rawData = nullptr;
		if (value.Type() == ValueType::Map) rawData = ValueToRawData(value);
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
	});
	raylibModule.SetValue("SetMaterialShaderValueV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("locIndex");
	i.AddParam("mat");
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context.GetArg(1).IntValue();
		Matrix mat = ValueToMatrix(context.GetArg(2));
		SetShaderValueMatrix(materialPtr->shader, locIndex, mat);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMaterialShaderValueMatrix", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("material");
	i.AddParam("locIndex");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Material* materialPtr = GetMaterialPtr(context.GetArg(0));
		if (materialPtr == nullptr) return IntrinsicResult::Null;

		int locIndex = context.GetArg(1).IntValue();
		Texture2D texture = ValueToTexture(context.GetArg(2));
		SetShaderValueTexture(materialPtr->shader, locIndex, texture);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetMaterialShaderValueTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("meshId");
	i.AddParam("materialId");
	i.set_Code(INTRINSIC_LAMBDA {
		Model* modelPtr = GetModelPtr(context.GetArg(0));
		if (modelPtr == nullptr) return IntrinsicResult::Null;

		int meshId = context.GetArg(1).IntValue();
		int materialId = context.GetArg(2).IntValue();
		SetModelMeshMaterial(modelPtr, meshId, materialId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetModelMeshMaterial", i.GetFunc());

	// Model animations

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		int animCount = 0;
		ModelAnimation* animations = LoadModelAnimations(path.c_str(), &animCount);
		if (animations == nullptr || animCount <= 0) return IntrinsicResult::Null;

		ValueList result;
		for (int n = 0; n < animCount; n++) {
			result.Add(ModelAnimationArrayItemToValue(animations, animCount, n));
		}

		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadModelAnimations", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("animation");
	i.AddParam("frame", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		ModelAnimation animation = ValueToModelAnimation(context.GetArg(1));
		float frame = context.GetArg(2).FloatValue();
		UpdateModelAnimation(model, animation, frame);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateModelAnimation", i.GetFunc());

#ifndef PLATFORM_WEB
	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("animationA");
	i.AddParam("frameA", Value::zero);
	i.AddParam("animationB");
	i.AddParam("frameB", Value::zero);
	i.AddParam("blend", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		ModelAnimation animationA = ValueToModelAnimation(context.GetArg(1));
		float frameA = context.GetArg(2).FloatValue();
		ModelAnimation animationB = ValueToModelAnimation(context.GetArg(3));
		float frameB = context.GetArg(4).FloatValue();
		float blend = context.GetArg(5).FloatValue();
		UpdateModelAnimationEx(model, animationA, frameA, animationB, frameB, blend);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateModelAnimationEx", i.GetFunc());
#endif

	i = Intrinsic::Create("");
	i.AddParam("animations");
	i.set_Code(INTRINSIC_LAMBDA {
		Value animationsValue = context.GetArg(0);
		int animCount = 0;
		ModelAnimation* animations = GetModelAnimationArray(animationsValue, &animCount);
		if (animations == nullptr || animCount <= 0) return IntrinsicResult::Null;

		UnloadModelAnimations(animations, animCount);
		rcModelAnimation -= animCount;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadModelAnimations", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("model");
	i.AddParam("animation");
	i.set_Code(INTRINSIC_LAMBDA {
		Model model = ValueToModel(context.GetArg(0));
		ModelAnimation animation = ValueToModelAnimation(context.GetArg(1));
		return IntrinsicResult(IsModelAnimationValid(model, animation));
	});
	raylibModule.SetValue("IsModelAnimationValid", i.GetFunc());

	// Collision detection

	i = Intrinsic::Create("");
	i.AddParam("center1");
	i.AddParam("radius1");
	i.AddParam("center2");
	i.AddParam("radius2");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 center1 = ValueToVector3(context.GetArg(0));
		float radius1 = context.GetArg(1).FloatValue();
		Vector3 center2 = ValueToVector3(context.GetArg(2));
		float radius2 = context.GetArg(3).FloatValue();
		return IntrinsicResult(CheckCollisionSpheres(center1, radius1, center2, radius2));
	});
	raylibModule.SetValue("CheckCollisionSpheres", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("box1");
	i.AddParam("box2");
	i.set_Code(INTRINSIC_LAMBDA {
		BoundingBox box1 = ValueToBoundingBox(context.GetArg(0));
		BoundingBox box2 = ValueToBoundingBox(context.GetArg(1));
		return IntrinsicResult(CheckCollisionBoxes(box1, box2));
	});
	raylibModule.SetValue("CheckCollisionBoxes", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("box");
	i.AddParam("center");
	i.AddParam("radius");
	i.set_Code(INTRINSIC_LAMBDA {
		BoundingBox box = ValueToBoundingBox(context.GetArg(0));
		Vector3 center = ValueToVector3(context.GetArg(1));
		float radius = context.GetArg(2).FloatValue();
		return IntrinsicResult(CheckCollisionBoxSphere(box, center, radius));
	});
	raylibModule.SetValue("CheckCollisionBoxSphere", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("ray");
	i.AddParam("center");
	i.AddParam("radius");
	i.set_Code(INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context.GetArg(0));
		Vector3 center = ValueToVector3(context.GetArg(1));
		float radius = context.GetArg(2).FloatValue();
		RayCollision result = GetRayCollisionSphere(ray, center, radius);
		return IntrinsicResult(RayCollisionToValue(result));
	});
	raylibModule.SetValue("GetRayCollisionSphere", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("ray");
	i.AddParam("box");
	i.set_Code(INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context.GetArg(0));
		BoundingBox box = ValueToBoundingBox(context.GetArg(1));
		RayCollision result = GetRayCollisionBox(ray, box);
		return IntrinsicResult(RayCollisionToValue(result));
	});
	raylibModule.SetValue("GetRayCollisionBox", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("ray");
	i.AddParam("mesh");
	i.AddParam("transform", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context.GetArg(0));
		Mesh mesh = ValueToMesh(context.GetArg(1));
		Matrix transform = ValueToMatrix(context.GetArg(2));
		RayCollision result = GetRayCollisionMesh(ray, mesh, transform);
		return IntrinsicResult(RayCollisionToValue(result));
	});
	raylibModule.SetValue("GetRayCollisionMesh", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("ray");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.set_Code(INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context.GetArg(0));
		Vector3 p1 = ValueToVector3(context.GetArg(1));
		Vector3 p2 = ValueToVector3(context.GetArg(2));
		Vector3 p3 = ValueToVector3(context.GetArg(3));
		RayCollision result = GetRayCollisionTriangle(ray, p1, p2, p3);
		return IntrinsicResult(RayCollisionToValue(result));
	});
	raylibModule.SetValue("GetRayCollisionTriangle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("ray");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("p4");
	i.set_Code(INTRINSIC_LAMBDA {
		Ray ray = ValueToRay(context.GetArg(0));
		Vector3 p1 = ValueToVector3(context.GetArg(1));
		Vector3 p2 = ValueToVector3(context.GetArg(2));
		Vector3 p3 = ValueToVector3(context.GetArg(3));
		Vector3 p4 = ValueToVector3(context.GetArg(4));
		RayCollision result = GetRayCollisionQuad(ray, p1, p2, p3, p4);
		return IntrinsicResult(RayCollisionToValue(result));
	});
	raylibModule.SetValue("GetRayCollisionQuad", i.GetFunc());
}
