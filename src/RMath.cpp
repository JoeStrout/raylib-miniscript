//
//  RMath.cpp
//  raylib-miniscript
//
//  Raylib raymath module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "raymath.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"

using namespace MiniScript;

void AddRMathMethods(ValueDict raylibModule) {
	Intrinsic *i;

	// Utility math
	i = Intrinsic::Create("");
	i->AddParam("value", Value::zero);
	i->AddParam("min", Value::zero);
	i->AddParam("max", Value::one);
	i->code = INTRINSIC_LAMBDA {
		float value = context->GetVar(String("value")).FloatValue();
		float min = context->GetVar(String("min")).FloatValue();
		float max = context->GetVar(String("max")).FloatValue();
		return IntrinsicResult(Clamp(value, min, max));
	};
	raylibModule.SetValue("Clamp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("start", Value::zero);
	i->AddParam("end", Value::one);
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float start = context->GetVar(String("start")).FloatValue();
		float end = context->GetVar(String("end")).FloatValue();
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(Lerp(start, end, amount));
	};
	raylibModule.SetValue("Lerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("value", Value::zero);
	i->AddParam("start", Value::zero);
	i->AddParam("end", Value::one);
	i->code = INTRINSIC_LAMBDA {
		float value = context->GetVar(String("value")).FloatValue();
		float start = context->GetVar(String("start")).FloatValue();
		float end = context->GetVar(String("end")).FloatValue();
		return IntrinsicResult(Normalize(value, start, end));
	};
	raylibModule.SetValue("Normalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("value", Value::zero);
	i->AddParam("inputStart", Value::zero);
	i->AddParam("inputEnd", Value::one);
	i->AddParam("outputStart", Value::zero);
	i->AddParam("outputEnd", Value::one);
	i->code = INTRINSIC_LAMBDA {
		float value = context->GetVar(String("value")).FloatValue();
		float inputStart = context->GetVar(String("inputStart")).FloatValue();
		float inputEnd = context->GetVar(String("inputEnd")).FloatValue();
		float outputStart = context->GetVar(String("outputStart")).FloatValue();
		float outputEnd = context->GetVar(String("outputEnd")).FloatValue();
		return IntrinsicResult(Remap(value, inputStart, inputEnd, outputStart, outputEnd));
	};
	raylibModule.SetValue("Remap", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("value", Value::zero);
	i->AddParam("min", Value::zero);
	i->AddParam("max", Value::one);
	i->code = INTRINSIC_LAMBDA {
		float value = context->GetVar(String("value")).FloatValue();
		float min = context->GetVar(String("min")).FloatValue();
		float max = context->GetVar(String("max")).FloatValue();
		return IntrinsicResult(Wrap(value, min, max));
	};
	raylibModule.SetValue("Wrap", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x", Value::zero);
	i->AddParam("y", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float x = context->GetVar(String("x")).FloatValue();
		float y = context->GetVar(String("y")).FloatValue();
		return IntrinsicResult(FloatEquals(x, y));
	};
	raylibModule.SetValue("FloatEquals", i->GetFunc());

	// Vector2 math
	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector2ToValue(Vector2Zero()));
	};
	raylibModule.SetValue("Vector2Zero", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector2ToValue(Vector2One()));
	};
	raylibModule.SetValue("Vector2One", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2ToValue(Vector2Add(v1, v2)));
	};
	raylibModule.SetValue("Vector2Add", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("add", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		float add = context->GetVar(String("add")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2AddValue(v, add)));
	};
	raylibModule.SetValue("Vector2AddValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2ToValue(Vector2Subtract(v1, v2)));
	};
	raylibModule.SetValue("Vector2Subtract", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("sub", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		float sub = context->GetVar(String("sub")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2SubtractValue(v, sub)));
	};
	raylibModule.SetValue("Vector2SubtractValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		return IntrinsicResult(Vector2Length(v));
	};
	raylibModule.SetValue("Vector2Length", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		return IntrinsicResult(Vector2LengthSqr(v));
	};
	raylibModule.SetValue("Vector2LengthSqr", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2DotProduct(v1, v2));
	};
	raylibModule.SetValue("Vector2DotProduct", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2CrossProduct(v1, v2));
	};
	raylibModule.SetValue("Vector2CrossProduct", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2Distance(v1, v2));
	};
	raylibModule.SetValue("Vector2Distance", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2DistanceSqr(v1, v2));
	};
	raylibModule.SetValue("Vector2DistanceSqr", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2Angle(v1, v2));
	};
	raylibModule.SetValue("Vector2Angle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("start", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("end", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 start = ValueToVector2(context->GetVar(String("start")));
		Vector2 end = ValueToVector2(context->GetVar(String("end")));
		return IntrinsicResult(Vector2LineAngle(start, end));
	};
	raylibModule.SetValue("Vector2LineAngle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("scale", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		float scale = context->GetVar(String("scale")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Scale(v, scale)));
	};
	raylibModule.SetValue("Vector2Scale", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2ToValue(Vector2Multiply(v1, v2)));
	};
	raylibModule.SetValue("Vector2Multiply", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		return IntrinsicResult(Vector2ToValue(Vector2Negate(v)));
	};
	raylibModule.SetValue("Vector2Negate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2ToValue(Vector2Divide(v1, v2)));
	};
	raylibModule.SetValue("Vector2Divide", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		return IntrinsicResult(Vector2ToValue(Vector2Normalize(v)));
	};
	raylibModule.SetValue("Vector2Normalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(Vector2ToValue(Vector2Transform(v, mat)));
	};
	raylibModule.SetValue("Vector2Transform", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Lerp(v1, v2, amount)));
	};
	raylibModule.SetValue("Vector2Lerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("normal", Vector2ToValue(Vector2{0, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		Vector2 normal = ValueToVector2(context->GetVar(String("normal")));
		return IntrinsicResult(Vector2ToValue(Vector2Reflect(v, normal)));
	};
	raylibModule.SetValue("Vector2Reflect", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2ToValue(Vector2Min(v1, v2)));
	};
	raylibModule.SetValue("Vector2Min", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		return IntrinsicResult(Vector2ToValue(Vector2Max(v1, v2)));
	};
	raylibModule.SetValue("Vector2Max", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Rotate(v, angle)));
	};
	raylibModule.SetValue("Vector2Rotate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("target", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("maxDistance", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		Vector2 target = ValueToVector2(context->GetVar(String("target")));
		float maxDistance = context->GetVar(String("maxDistance")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2MoveTowards(v, target, maxDistance)));
	};
	raylibModule.SetValue("Vector2MoveTowards", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		return IntrinsicResult(Vector2ToValue(Vector2Invert(v)));
	};
	raylibModule.SetValue("Vector2Invert", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("min", Vector2ToValue(Vector2{-1, -1}));
	i->AddParam("max", Vector2ToValue(Vector2{1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		Vector2 min = ValueToVector2(context->GetVar(String("min")));
		Vector2 max = ValueToVector2(context->GetVar(String("max")));
		return IntrinsicResult(Vector2ToValue(Vector2Clamp(v, min, max)));
	};
	raylibModule.SetValue("Vector2Clamp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("min", Value::zero);
	i->AddParam("max", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		float min = context->GetVar(String("min")).FloatValue();
		float max = context->GetVar(String("max")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2ClampValue(v, min, max)));
	};
	raylibModule.SetValue("Vector2ClampValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("q", Vector2ToValue(Vector2{0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector2 p = ValueToVector2(context->GetVar(String("p")));
		Vector2 q = ValueToVector2(context->GetVar(String("q")));
		return IntrinsicResult(Vector2Equals(p, q));
	};
	raylibModule.SetValue("Vector2Equals", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("n", Vector2ToValue(Vector2{0, 1}));
	i->AddParam("r", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context->GetVar(String("v")));
		Vector2 n = ValueToVector2(context->GetVar(String("n")));
		float r = context->GetVar(String("r")).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Refract(v, n, r)));
	};
	raylibModule.SetValue("Vector2Refract", i->GetFunc());

	// Vector3 math (initial tranche)
	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector3ToValue(Vector3Zero()));
	};
	raylibModule.SetValue("Vector3Zero", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector3ToValue(Vector3One()));
	};
	raylibModule.SetValue("Vector3One", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Add(v1, v2)));
	};
	raylibModule.SetValue("Vector3Add", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("add", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		float add = context->GetVar(String("add")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3AddValue(v, add)));
	};
	raylibModule.SetValue("Vector3AddValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Subtract(v1, v2)));
	};
	raylibModule.SetValue("Vector3Subtract", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("sub", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		float sub = context->GetVar(String("sub")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3SubtractValue(v, sub)));
	};
	raylibModule.SetValue("Vector3SubtractValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("scalar", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		float scalar = context->GetVar(String("scalar")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3Scale(v, scalar)));
	};
	raylibModule.SetValue("Vector3Scale", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Multiply(v1, v2)));
	};
	raylibModule.SetValue("Vector3Multiply", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3CrossProduct(v1, v2)));
	};
	raylibModule.SetValue("Vector3CrossProduct", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		return IntrinsicResult(Vector3ToValue(Vector3Perpendicular(v)));
	};
	raylibModule.SetValue("Vector3Perpendicular", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		return IntrinsicResult(Vector3Length(v));
	};
	raylibModule.SetValue("Vector3Length", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		return IntrinsicResult(Vector3LengthSqr(v));
	};
	raylibModule.SetValue("Vector3LengthSqr", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3DotProduct(v1, v2));
	};
	raylibModule.SetValue("Vector3DotProduct", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3Distance(v1, v2));
	};
	raylibModule.SetValue("Vector3Distance", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3DistanceSqr(v1, v2));
	};
	raylibModule.SetValue("Vector3DistanceSqr", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3Angle(v1, v2));
	};
	raylibModule.SetValue("Vector3Angle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		return IntrinsicResult(Vector3ToValue(Vector3Negate(v)));
	};
	raylibModule.SetValue("Vector3Negate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Divide(v1, v2)));
	};
	raylibModule.SetValue("Vector3Divide", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		return IntrinsicResult(Vector3ToValue(Vector3Normalize(v)));
	};
	raylibModule.SetValue("Vector3Normalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 1, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Project(v1, v2)));
	};
	raylibModule.SetValue("Vector3Project", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 1, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Reject(v1, v2)));
	};
	raylibModule.SetValue("Vector3Reject", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{1, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 1, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		Vector3OrthoNormalize(&v1, &v2);
		ValueDict result;
		result.SetValue(String("v1"), Vector3ToValue(v1));
		result.SetValue(String("v2"), Vector3ToValue(v2));
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("Vector3OrthoNormalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(Vector3ToValue(Vector3Transform(v, mat)));
	};
	raylibModule.SetValue("Vector3Transform", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(Vector3ToValue(Vector3RotateByQuaternion(v, q)));
	};
	raylibModule.SetValue("Vector3RotateByQuaternion", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("axis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Vector3 axis = ValueToVector3(context->GetVar(String("axis")));
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3RotateByAxisAngle(v, axis, angle)));
	};
	raylibModule.SetValue("Vector3RotateByAxisAngle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("target", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("maxDistance", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Vector3 target = ValueToVector3(context->GetVar(String("target")));
		float maxDistance = context->GetVar(String("maxDistance")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3MoveTowards(v, target, maxDistance)));
	};
	raylibModule.SetValue("Vector3MoveTowards", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3Lerp(v1, v2, amount)));
	};
	raylibModule.SetValue("Vector3Lerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("tangent1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("tangent2", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 tangent1 = ValueToVector3(context->GetVar(String("tangent1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		Vector3 tangent2 = ValueToVector3(context->GetVar(String("tangent2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3CubicHermite(v1, tangent1, v2, tangent2, amount)));
	};
	raylibModule.SetValue("Vector3CubicHermite", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("normal", Vector3ToValue(Vector3{0, 1, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Vector3 normal = ValueToVector3(context->GetVar(String("normal")));
		return IntrinsicResult(Vector3ToValue(Vector3Reflect(v, normal)));
	};
	raylibModule.SetValue("Vector3Reflect", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Min(v1, v2)));
	};
	raylibModule.SetValue("Vector3Min", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context->GetVar(String("v1")));
		Vector3 v2 = ValueToVector3(context->GetVar(String("v2")));
		return IntrinsicResult(Vector3ToValue(Vector3Max(v1, v2)));
	};
	raylibModule.SetValue("Vector3Max", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("a", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("b", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("c", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 p = ValueToVector3(context->GetVar(String("p")));
		Vector3 a = ValueToVector3(context->GetVar(String("a")));
		Vector3 b = ValueToVector3(context->GetVar(String("b")));
		Vector3 c = ValueToVector3(context->GetVar(String("c")));
		return IntrinsicResult(Vector3ToValue(Vector3Barycenter(p, a, b, c)));
	};
	raylibModule.SetValue("Vector3Barycenter", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("source", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("projection", MatrixToValue(MatrixIdentity()));
	i->AddParam("view", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Vector3 source = ValueToVector3(context->GetVar(String("source")));
		Matrix projection = ValueToMatrix(context->GetVar(String("projection")));
		Matrix view = ValueToMatrix(context->GetVar(String("view")));
		return IntrinsicResult(Vector3ToValue(Vector3Unproject(source, projection, view)));
	};
	raylibModule.SetValue("Vector3Unproject", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		float3 out = Vector3ToFloatV(v);
		ValueList result;
		result.Add(Value(out.v[0]));
		result.Add(Value(out.v[1]));
		result.Add(Value(out.v[2]));
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("Vector3ToFloatV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		return IntrinsicResult(Vector3ToValue(Vector3Invert(v)));
	};
	raylibModule.SetValue("Vector3Invert", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("min", Vector3ToValue(Vector3{-1, -1, -1}));
	i->AddParam("max", Vector3ToValue(Vector3{1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Vector3 min = ValueToVector3(context->GetVar(String("min")));
		Vector3 max = ValueToVector3(context->GetVar(String("max")));
		return IntrinsicResult(Vector3ToValue(Vector3Clamp(v, min, max)));
	};
	raylibModule.SetValue("Vector3Clamp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("min", Value::zero);
	i->AddParam("max", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		float min = context->GetVar(String("min")).FloatValue();
		float max = context->GetVar(String("max")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3ClampValue(v, min, max)));
	};
	raylibModule.SetValue("Vector3ClampValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("q", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 p = ValueToVector3(context->GetVar(String("p")));
		Vector3 q = ValueToVector3(context->GetVar(String("q")));
		return IntrinsicResult(Vector3Equals(p, q));
	};
	raylibModule.SetValue("Vector3Equals", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("n", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("r", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context->GetVar(String("v")));
		Vector3 n = ValueToVector3(context->GetVar(String("n")));
		float r = context->GetVar(String("r")).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3Refract(v, n, r)));
	};
	raylibModule.SetValue("Vector3Refract", i->GetFunc());

	// Vector4 math
	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector4ToValue(Vector4Zero()));
	};
	raylibModule.SetValue("Vector4Zero", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector4ToValue(Vector4One()));
	};
	raylibModule.SetValue("Vector4One", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4ToValue(Vector4Add(v1, v2)));
	};
	raylibModule.SetValue("Vector4Add", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("add", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		float add = context->GetVar(String("add")).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4AddValue(v, add)));
	};
	raylibModule.SetValue("Vector4AddValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4ToValue(Vector4Subtract(v1, v2)));
	};
	raylibModule.SetValue("Vector4Subtract", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("sub", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		float sub = context->GetVar(String("sub")).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4SubtractValue(v, sub)));
	};
	raylibModule.SetValue("Vector4SubtractValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		return IntrinsicResult(Vector4Length(v));
	};
	raylibModule.SetValue("Vector4Length", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		return IntrinsicResult(Vector4LengthSqr(v));
	};
	raylibModule.SetValue("Vector4LengthSqr", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4DotProduct(v1, v2));
	};
	raylibModule.SetValue("Vector4DotProduct", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4Distance(v1, v2));
	};
	raylibModule.SetValue("Vector4Distance", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4DistanceSqr(v1, v2));
	};
	raylibModule.SetValue("Vector4DistanceSqr", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("scale", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		float scale = context->GetVar(String("scale")).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4Scale(v, scale)));
	};
	raylibModule.SetValue("Vector4Scale", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4ToValue(Vector4Multiply(v1, v2)));
	};
	raylibModule.SetValue("Vector4Multiply", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		return IntrinsicResult(Vector4ToValue(Vector4Negate(v)));
	};
	raylibModule.SetValue("Vector4Negate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{1, 1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4ToValue(Vector4Divide(v1, v2)));
	};
	raylibModule.SetValue("Vector4Divide", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		return IntrinsicResult(Vector4ToValue(Vector4Normalize(v)));
	};
	raylibModule.SetValue("Vector4Normalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4ToValue(Vector4Min(v1, v2)));
	};
	raylibModule.SetValue("Vector4Min", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		return IntrinsicResult(Vector4ToValue(Vector4Max(v1, v2)));
	};
	raylibModule.SetValue("Vector4Max", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context->GetVar(String("v1")));
		Vector4 v2 = ValueToVector4(context->GetVar(String("v2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4Lerp(v1, v2, amount)));
	};
	raylibModule.SetValue("Vector4Lerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("target", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("maxDistance", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		Vector4 target = ValueToVector4(context->GetVar(String("target")));
		float maxDistance = context->GetVar(String("maxDistance")).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4MoveTowards(v, target, maxDistance)));
	};
	raylibModule.SetValue("Vector4MoveTowards", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v", Vector4ToValue(Vector4{1, 1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context->GetVar(String("v")));
		return IntrinsicResult(Vector4ToValue(Vector4Invert(v)));
	};
	raylibModule.SetValue("Vector4Invert", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->AddParam("q", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector4 p = ValueToVector4(context->GetVar(String("p")));
		Vector4 q = ValueToVector4(context->GetVar(String("q")));
		return IntrinsicResult(Vector4Equals(p, q));
	};
	raylibModule.SetValue("Vector4Equals", i->GetFunc());

	// Matrix math
	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(MatrixDeterminant(mat));
	};
	raylibModule.SetValue("MatrixDeterminant", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(MatrixTrace(mat));
	};
	raylibModule.SetValue("MatrixTrace", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(MatrixToValue(MatrixTranspose(mat)));
	};
	raylibModule.SetValue("MatrixTranspose", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(MatrixToValue(MatrixInvert(mat)));
	};
	raylibModule.SetValue("MatrixInvert", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(MatrixIdentity()));
	};
	raylibModule.SetValue("MatrixIdentity", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left", MatrixToValue(MatrixIdentity()));
	i->AddParam("right", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context->GetVar(String("left")));
		Matrix right = ValueToMatrix(context->GetVar(String("right")));
		return IntrinsicResult(MatrixToValue(MatrixAdd(left, right)));
	};
	raylibModule.SetValue("MatrixAdd", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left", MatrixToValue(MatrixIdentity()));
	i->AddParam("right", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context->GetVar(String("left")));
		Matrix right = ValueToMatrix(context->GetVar(String("right")));
		return IntrinsicResult(MatrixToValue(MatrixSubtract(left, right)));
	};
	raylibModule.SetValue("MatrixSubtract", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left", MatrixToValue(MatrixIdentity()));
	i->AddParam("right", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context->GetVar(String("left")));
		Matrix right = ValueToMatrix(context->GetVar(String("right")));
		return IntrinsicResult(MatrixToValue(MatrixMultiply(left, right)));
	};
	raylibModule.SetValue("MatrixMultiply", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left", MatrixToValue(MatrixIdentity()));
	i->AddParam("value", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context->GetVar(String("left")));
		float value = context->GetVar(String("value")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixMultiplyValue(left, value)));
	};
	raylibModule.SetValue("MatrixMultiplyValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x", Value::zero);
	i->AddParam("y", Value::zero);
	i->AddParam("z", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float x = context->GetVar(String("x")).FloatValue();
		float y = context->GetVar(String("y")).FloatValue();
		float z = context->GetVar(String("z")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixTranslate(x, y, z)));
	};
	raylibModule.SetValue("MatrixTranslate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("axis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 axis = ValueToVector3(context->GetVar(String("axis")));
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotate(axis, angle)));
	};
	raylibModule.SetValue("MatrixRotate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotateX(angle)));
	};
	raylibModule.SetValue("MatrixRotateX", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotateY(angle)));
	};
	raylibModule.SetValue("MatrixRotateY", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotateZ(angle)));
	};
	raylibModule.SetValue("MatrixRotateZ", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("angle", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 angle = ValueToVector3(context->GetVar(String("angle")));
		return IntrinsicResult(MatrixToValue(MatrixRotateXYZ(angle)));
	};
	raylibModule.SetValue("MatrixRotateXYZ", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("angle", Vector3ToValue(Vector3{0, 0, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 angle = ValueToVector3(context->GetVar(String("angle")));
		return IntrinsicResult(MatrixToValue(MatrixRotateZYX(angle)));
	};
	raylibModule.SetValue("MatrixRotateZYX", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("x", Value::one);
	i->AddParam("y", Value::one);
	i->AddParam("z", Value::one);
	i->code = INTRINSIC_LAMBDA {
		float x = context->GetVar(String("x")).FloatValue();
		float y = context->GetVar(String("y")).FloatValue();
		float z = context->GetVar(String("z")).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixScale(x, y, z)));
	};
	raylibModule.SetValue("MatrixScale", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left", Value(-1));
	i->AddParam("right", Value(1));
	i->AddParam("bottom", Value(-1));
	i->AddParam("top", Value(1));
	i->AddParam("nearPlane", Value(0.01));
	i->AddParam("farPlane", Value(1000));
	i->code = INTRINSIC_LAMBDA {
		double left = context->GetVar(String("left")).DoubleValue();
		double right = context->GetVar(String("right")).DoubleValue();
		double bottom = context->GetVar(String("bottom")).DoubleValue();
		double top = context->GetVar(String("top")).DoubleValue();
		double nearPlane = context->GetVar(String("nearPlane")).DoubleValue();
		double farPlane = context->GetVar(String("farPlane")).DoubleValue();
		return IntrinsicResult(MatrixToValue(MatrixFrustum(left, right, bottom, top, nearPlane, farPlane)));
	};
	raylibModule.SetValue("MatrixFrustum", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fovY", Value(45.0 * DEG2RAD));
	i->AddParam("aspect", Value(1));
	i->AddParam("nearPlane", Value(0.01));
	i->AddParam("farPlane", Value(1000));
	i->code = INTRINSIC_LAMBDA {
		double fovY = context->GetVar(String("fovY")).DoubleValue();
		double aspect = context->GetVar(String("aspect")).DoubleValue();
		double nearPlane = context->GetVar(String("nearPlane")).DoubleValue();
		double farPlane = context->GetVar(String("farPlane")).DoubleValue();
		return IntrinsicResult(MatrixToValue(MatrixPerspective(fovY, aspect, nearPlane, farPlane)));
	};
	raylibModule.SetValue("MatrixPerspective", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("left", Value(-1));
	i->AddParam("right", Value(1));
	i->AddParam("bottom", Value(-1));
	i->AddParam("top", Value(1));
	i->AddParam("nearPlane", Value(0.01));
	i->AddParam("farPlane", Value(1000));
	i->code = INTRINSIC_LAMBDA {
		double left = context->GetVar(String("left")).DoubleValue();
		double right = context->GetVar(String("right")).DoubleValue();
		double bottom = context->GetVar(String("bottom")).DoubleValue();
		double top = context->GetVar(String("top")).DoubleValue();
		double nearPlane = context->GetVar(String("nearPlane")).DoubleValue();
		double farPlane = context->GetVar(String("farPlane")).DoubleValue();
		return IntrinsicResult(MatrixToValue(MatrixOrtho(left, right, bottom, top, nearPlane, farPlane)));
	};
	raylibModule.SetValue("MatrixOrtho", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("eye", Vector3ToValue(Vector3{0, 0, 1}));
	i->AddParam("target", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("up", Vector3ToValue(Vector3{0, 1, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 eye = ValueToVector3(context->GetVar(String("eye")));
		Vector3 target = ValueToVector3(context->GetVar(String("target")));
		Vector3 up = ValueToVector3(context->GetVar(String("up")));
		return IntrinsicResult(MatrixToValue(MatrixLookAt(eye, target, up)));
	};
	raylibModule.SetValue("MatrixLookAt", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		float16 values = MatrixToFloatV(mat);
		ValueList result;
		for (int n = 0; n < 16; n++) result.Add(Value(values.v[n]));
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("MatrixToFloatV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("translation", Vector3ToValue(Vector3{0, 0, 0}));
	i->AddParam("rotation", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 translation = ValueToVector3(context->GetVar(String("translation")));
		Quaternion rotation = ValueToQuaternion(context->GetVar(String("rotation")));
		Vector3 scale = ValueToVector3(context->GetVar(String("scale")));
		return IntrinsicResult(MatrixToValue(MatrixCompose(translation, rotation, scale)));
	};
	raylibModule.SetValue("MatrixCompose", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		Vector3 translation;
		Quaternion rotation;
		Vector3 scale;
		MatrixDecompose(mat, &translation, &rotation, &scale);

		ValueDict result;
		result.SetValue(String("translation"), Vector3ToValue(translation));
		result.SetValue(String("rotation"), QuaternionToValue(rotation));
		result.SetValue(String("scale"), Vector3ToValue(scale));
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("MatrixDecompose", i->GetFunc());

	// Quaternion math
	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		return IntrinsicResult(QuaternionToValue(QuaternionAdd(q1, q2)));
	};
	raylibModule.SetValue("QuaternionAdd", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("add", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		float add = context->GetVar(String("add")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionAddValue(q, add)));
	};
	raylibModule.SetValue("QuaternionAddValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		return IntrinsicResult(QuaternionToValue(QuaternionSubtract(q1, q2)));
	};
	raylibModule.SetValue("QuaternionSubtract", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("sub", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		float sub = context->GetVar(String("sub")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionSubtractValue(q, sub)));
	};
	raylibModule.SetValue("QuaternionSubtractValue", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		return IntrinsicResult(QuaternionToValue(QuaternionIdentity()));
	};
	raylibModule.SetValue("QuaternionIdentity", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(QuaternionLength(q));
	};
	raylibModule.SetValue("QuaternionLength", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(QuaternionToValue(QuaternionNormalize(q)));
	};
	raylibModule.SetValue("QuaternionNormalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(QuaternionToValue(QuaternionInvert(q)));
	};
	raylibModule.SetValue("QuaternionInvert", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		return IntrinsicResult(QuaternionToValue(QuaternionMultiply(q1, q2)));
	};
	raylibModule.SetValue("QuaternionMultiply", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("mul", Value::one);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		float mul = context->GetVar(String("mul")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionScale(q, mul)));
	};
	raylibModule.SetValue("QuaternionScale", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		return IntrinsicResult(QuaternionToValue(QuaternionDivide(q1, q2)));
	};
	raylibModule.SetValue("QuaternionDivide", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionLerp(q1, q2, amount)));
	};
	raylibModule.SetValue("QuaternionLerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionNlerp(q1, q2, amount)));
	};
	raylibModule.SetValue("QuaternionNlerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("amount", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionSlerp(q1, q2, amount)));
	};
	raylibModule.SetValue("QuaternionSlerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("outTangent1", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("inTangent2", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("t", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context->GetVar(String("q1")));
		Quaternion outTangent1 = ValueToQuaternion(context->GetVar(String("outTangent1")));
		Quaternion q2 = ValueToQuaternion(context->GetVar(String("q2")));
		Quaternion inTangent2 = ValueToQuaternion(context->GetVar(String("inTangent2")));
		float t = context->GetVar(String("t")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionCubicHermiteSpline(q1, outTangent1, q2, inTangent2, t)));
	};
	raylibModule.SetValue("QuaternionCubicHermiteSpline", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("from", Vector3ToValue(Vector3{1, 0, 0}));
	i->AddParam("to", Vector3ToValue(Vector3{0, 1, 0}));
	i->code = INTRINSIC_LAMBDA {
		Vector3 from = ValueToVector3(context->GetVar(String("from")));
		Vector3 to = ValueToVector3(context->GetVar(String("to")));
		return IntrinsicResult(QuaternionToValue(QuaternionFromVector3ToVector3(from, to)));
	};
	raylibModule.SetValue("QuaternionFromVector3ToVector3", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(QuaternionToValue(QuaternionFromMatrix(mat)));
	};
	raylibModule.SetValue("QuaternionFromMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(MatrixToValue(QuaternionToMatrix(q)));
	};
	raylibModule.SetValue("QuaternionToMatrix", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("axis", Vector3ToValue(Vector3{0, 1, 0}));
	i->AddParam("angle", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		Vector3 axis = ValueToVector3(context->GetVar(String("axis")));
		float angle = context->GetVar(String("angle")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionFromAxisAngle(axis, angle)));
	};
	raylibModule.SetValue("QuaternionFromAxisAngle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		Vector3 outAxis;
		float outAngle = 0;
		QuaternionToAxisAngle(q, &outAxis, &outAngle);
		ValueDict result;
		result.SetValue(String("axis"), Vector3ToValue(outAxis));
		result.SetValue(String("angle"), Value(outAngle));
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("QuaternionToAxisAngle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("pitch", Value::zero);
	i->AddParam("yaw", Value::zero);
	i->AddParam("roll", Value::zero);
	i->code = INTRINSIC_LAMBDA {
		float pitch = context->GetVar(String("pitch")).FloatValue();
		float yaw = context->GetVar(String("yaw")).FloatValue();
		float roll = context->GetVar(String("roll")).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionFromEuler(pitch, yaw, roll)));
	};
	raylibModule.SetValue("QuaternionFromEuler", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(Vector3ToValue(QuaternionToEuler(q)));
	};
	raylibModule.SetValue("QuaternionToEuler", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("mat", MatrixToValue(MatrixIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		Matrix mat = ValueToMatrix(context->GetVar(String("mat")));
		return IntrinsicResult(QuaternionToValue(QuaternionTransform(q, mat)));
	};
	raylibModule.SetValue("QuaternionTransform", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p", QuaternionToValue(QuaternionIdentity()));
	i->AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i->code = INTRINSIC_LAMBDA {
		Quaternion p = ValueToQuaternion(context->GetVar(String("p")));
		Quaternion q = ValueToQuaternion(context->GetVar(String("q")));
		return IntrinsicResult(QuaternionEquals(p, q));
	};
	raylibModule.SetValue("QuaternionEquals", i->GetFunc());
}
