#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
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

static Matrix IdentityMatrix() {
	return Matrix{1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};
}

void AddRModelsMethods(ValueDict raylibModule) {
	Intrinsic *i;

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
	i->AddParam("transform", MatrixToValue(IdentityMatrix()));
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
	i->AddParam("transform", MatrixToValue(IdentityMatrix()));
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