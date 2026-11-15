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
#include "miniscript.h"
#include "macros.h"

using namespace MiniScript;

void AddRMathMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Utility math
	i = Intrinsic::Create("");
	i.AddParam("value", Value::zero);
	i.AddParam("min", Value::zero);
	i.AddParam("max", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float value = context.GetArg(0).FloatValue();
		float min = context.GetArg(1).FloatValue();
		float max = context.GetArg(2).FloatValue();
		return IntrinsicResult(Clamp(value, min, max));
	});
	raylibModule.SetValue("Clamp", WithNote(i,
		"Clamp value to the range [min, max]"));

	i = Intrinsic::Create("");
	i.AddParam("start", Value::zero);
	i.AddParam("end", Value::one);
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float start = context.GetArg(0).FloatValue();
		float end = context.GetArg(1).FloatValue();
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(Lerp(start, end, amount));
	});
	raylibModule.SetValue("Lerp", WithNote(i,
		"Linearly interpolate between start and end; amount 0 gives start, 1 gives end"));

	i = Intrinsic::Create("");
	i.AddParam("value", Value::zero);
	i.AddParam("start", Value::zero);
	i.AddParam("end", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float value = context.GetArg(0).FloatValue();
		float start = context.GetArg(1).FloatValue();
		float end = context.GetArg(2).FloatValue();
		return IntrinsicResult(Normalize(value, start, end));
	});
	raylibModule.SetValue("Normalize", WithNote(i,
		"Map value from the range [start, end] to the range [0, 1]"));

	i = Intrinsic::Create("");
	i.AddParam("value", Value::zero);
	i.AddParam("inputStart", Value::zero);
	i.AddParam("inputEnd", Value::one);
	i.AddParam("outputStart", Value::zero);
	i.AddParam("outputEnd", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float value = context.GetArg(0).FloatValue();
		float inputStart = context.GetArg(1).FloatValue();
		float inputEnd = context.GetArg(2).FloatValue();
		float outputStart = context.GetArg(3).FloatValue();
		float outputEnd = context.GetArg(4).FloatValue();
		return IntrinsicResult(Remap(value, inputStart, inputEnd, outputStart, outputEnd));
	});
	raylibModule.SetValue("Remap", WithNote(i,
		"Remap input value within input range to output range"));

	i = Intrinsic::Create("");
	i.AddParam("value", Value::zero);
	i.AddParam("min", Value::zero);
	i.AddParam("max", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float value = context.GetArg(0).FloatValue();
		float min = context.GetArg(1).FloatValue();
		float max = context.GetArg(2).FloatValue();
		return IntrinsicResult(Wrap(value, min, max));
	});
	raylibModule.SetValue("Wrap", WithNote(i,
		"Wrap value into the range [min, max)"));

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		return IntrinsicResult(FloatEquals(x, y));
	});
	raylibModule.SetValue("FloatEquals", WithNote(i,
		"Return true (1) if x and y are almost equal (within a small relative tolerance), "
		"else false (0)"));

	// Vector2 math
	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector2ToValue(Vector2Zero()));
	});
	raylibModule.SetValue("Vector2Zero", WithNote(i,
		"Get a Vector2 map {x, y} with all components 0"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector2ToValue(Vector2One()));
	});
	raylibModule.SetValue("Vector2One", WithNote(i,
		"Get a Vector2 map {x, y} with all components 1"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Add(v1, v2)));
	});
	raylibModule.SetValue("Vector2Add", WithNote(i,
		"Add two vectors (v1 + v2)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("add", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		float add = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2AddValue(v, add)));
	});
	raylibModule.SetValue("Vector2AddValue", WithNote(i,
		"Add a number to each component of vector v"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Subtract(v1, v2)));
	});
	raylibModule.SetValue("Vector2Subtract", WithNote(i,
		"Subtract two vectors (v1 - v2)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("sub", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		float sub = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2SubtractValue(v, sub)));
	});
	raylibModule.SetValue("Vector2SubtractValue", WithNote(i,
		"Subtract a number from each component of vector v"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		return IntrinsicResult(Vector2Length(v));
	});
	raylibModule.SetValue("Vector2Length", WithNote(i,
		"Calculate vector length"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		return IntrinsicResult(Vector2LengthSqr(v));
	});
	raylibModule.SetValue("Vector2LengthSqr", WithNote(i,
		"Calculate vector squared length"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2DotProduct(v1, v2));
	});
	raylibModule.SetValue("Vector2DotProduct", WithNote(i,
		"Calculate the dot product of two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2CrossProduct(v1, v2));
	});
	raylibModule.SetValue("Vector2CrossProduct", WithNote(i,
		"Calculate the 2D cross product of two vectors (v1.x*v2.y - v1.y*v2.x)"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2Distance(v1, v2));
	});
	raylibModule.SetValue("Vector2Distance", WithNote(i,
		"Calculate distance between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2DistanceSqr(v1, v2));
	});
	raylibModule.SetValue("Vector2DistanceSqr", WithNote(i,
		"Calculate squared distance between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2Angle(v1, v2));
	});
	raylibModule.SetValue("Vector2Angle", WithNote(i,
		"Calculate the signed angle (in radians) from v1 to v2, relative to the origin (0, 0). "
		"With positive X right and positive Y down, positive angles appear clockwise and "
		"negative angles appear counterclockwise"));

	i = Intrinsic::Create("");
	i.AddParam("start", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("end", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 start = ValueToVector2(context.GetArg(0));
		Vector2 end = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2LineAngle(start, end));
	});
	raylibModule.SetValue("Vector2LineAngle", WithNote(i,
		"Calculate the angle (in radians) of the line from start to end, "
		"i.e. -atan(end.y - start.y, end.x - start.x)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("scale", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		float scale = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Scale(v, scale)));
	});
	raylibModule.SetValue("Vector2Scale", WithNote(i,
		"Scale vector (multiply by value)"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Multiply(v1, v2)));
	});
	raylibModule.SetValue("Vector2Multiply", WithNote(i,
		"Multiply two vectors component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		return IntrinsicResult(Vector2ToValue(Vector2Negate(v)));
	});
	raylibModule.SetValue("Vector2Negate", WithNote(i,
		"Negate vector"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Divide(v1, v2)));
	});
	raylibModule.SetValue("Vector2Divide", WithNote(i,
		"Divide v1 by v2 component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		return IntrinsicResult(Vector2ToValue(Vector2Normalize(v)));
	});
	raylibModule.SetValue("Vector2Normalize", WithNote(i,
		"Scale vector to length 1 (a zero vector is returned unchanged)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		Matrix mat = ValueToMatrix(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Transform(v, mat)));
	});
	raylibModule.SetValue("Vector2Transform", WithNote(i,
		"Transforms a Vector2 by a given Matrix"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Lerp(v1, v2, amount)));
	});
	raylibModule.SetValue("Vector2Lerp", WithNote(i,
		"Calculate linear interpolation between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("normal", Vector2ToValue(Vector2{0, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		Vector2 normal = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Reflect(v, normal)));
	});
	raylibModule.SetValue("Vector2Reflect", WithNote(i,
		"Calculate the reflection of v off a surface with the given normal"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Min(v1, v2)));
	});
	raylibModule.SetValue("Vector2Min", WithNote(i,
		"Get min value for each pair of components"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("v2", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2ToValue(Vector2Max(v1, v2)));
	});
	raylibModule.SetValue("Vector2Max", WithNote(i,
		"Get max value for each pair of components"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		float angle = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Rotate(v, angle)));
	});
	raylibModule.SetValue("Vector2Rotate", WithNote(i,
		"Rotate vector by angle (in radians)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("target", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("maxDistance", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		Vector2 target = ValueToVector2(context.GetArg(1));
		float maxDistance = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2MoveTowards(v, target, maxDistance)));
	});
	raylibModule.SetValue("Vector2MoveTowards", WithNote(i,
		"Move vector v towards target by at most maxDistance"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		return IntrinsicResult(Vector2ToValue(Vector2Invert(v)));
	});
	raylibModule.SetValue("Vector2Invert", WithNote(i,
		"Get the component-wise reciprocal of the vector (1/x, 1/y)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("min", Vector2ToValue(Vector2{-1, -1}));
	i.AddParam("max", Vector2ToValue(Vector2{1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		Vector2 min = ValueToVector2(context.GetArg(1));
		Vector2 max = ValueToVector2(context.GetArg(2));
		return IntrinsicResult(Vector2ToValue(Vector2Clamp(v, min, max)));
	});
	raylibModule.SetValue("Vector2Clamp", WithNote(i,
		"Clamp the components of the vector between min and max values specified by the given "
		"vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("min", Value::zero);
	i.AddParam("max", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		float min = context.GetArg(1).FloatValue();
		float max = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2ClampValue(v, min, max)));
	});
	raylibModule.SetValue("Vector2ClampValue", WithNote(i,
		"Clamp the magnitude of the vector between two min and max values"));

	i = Intrinsic::Create("");
	i.AddParam("p", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("q", Vector2ToValue(Vector2{0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p = ValueToVector2(context.GetArg(0));
		Vector2 q = ValueToVector2(context.GetArg(1));
		return IntrinsicResult(Vector2Equals(p, q));
	});
	raylibModule.SetValue("Vector2Equals", WithNote(i,
		"Return true (1) if two vectors are almost equal (within a small relative tolerance), "
		"else false (0)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("n", Vector2ToValue(Vector2{0, 1}));
	i.AddParam("r", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v = ValueToVector2(context.GetArg(0));
		Vector2 n = ValueToVector2(context.GetArg(1));
		float r = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector2ToValue(Vector2Refract(v, n, r)));
	});
	raylibModule.SetValue("Vector2Refract", WithNote(i,
		"Compute the direction of a refracted ray. v: normalized direction of the incoming ray; "
		"n: normalized normal vector of the interface between the two media; r: ratio of the "
		"refractive index of the medium the ray comes from to that of the medium on the other side"));

	// Vector3 math (initial tranche)
	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector3ToValue(Vector3Zero()));
	});
	raylibModule.SetValue("Vector3Zero", WithNote(i,
		"Get a Vector3 map {x, y, z} with all components 0"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector3ToValue(Vector3One()));
	});
	raylibModule.SetValue("Vector3One", WithNote(i,
		"Get a Vector3 map {x, y, z} with all components 1"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Add(v1, v2)));
	});
	raylibModule.SetValue("Vector3Add", WithNote(i,
		"Add two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("add", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		float add = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3AddValue(v, add)));
	});
	raylibModule.SetValue("Vector3AddValue", WithNote(i,
		"Add a number to each component of vector v"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Subtract(v1, v2)));
	});
	raylibModule.SetValue("Vector3Subtract", WithNote(i,
		"Subtract two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("sub", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		float sub = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3SubtractValue(v, sub)));
	});
	raylibModule.SetValue("Vector3SubtractValue", WithNote(i,
		"Subtract a number from each component of vector v"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("scalar", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		float scalar = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3Scale(v, scalar)));
	});
	raylibModule.SetValue("Vector3Scale", WithNote(i,
		"Multiply vector by scalar"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Multiply(v1, v2)));
	});
	raylibModule.SetValue("Vector3Multiply", WithNote(i,
		"Multiply two vectors component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3CrossProduct(v1, v2)));
	});
	raylibModule.SetValue("Vector3CrossProduct", WithNote(i,
		"Calculate two vectors cross product"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(Vector3Perpendicular(v)));
	});
	raylibModule.SetValue("Vector3Perpendicular", WithNote(i,
		"Calculate a vector perpendicular to v"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(Vector3Length(v));
	});
	raylibModule.SetValue("Vector3Length", WithNote(i,
		"Calculate vector length"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(Vector3LengthSqr(v));
	});
	raylibModule.SetValue("Vector3LengthSqr", WithNote(i,
		"Calculate vector squared length"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3DotProduct(v1, v2));
	});
	raylibModule.SetValue("Vector3DotProduct", WithNote(i,
		"Calculate the dot product of two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3Distance(v1, v2));
	});
	raylibModule.SetValue("Vector3Distance", WithNote(i,
		"Calculate distance between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3DistanceSqr(v1, v2));
	});
	raylibModule.SetValue("Vector3DistanceSqr", WithNote(i,
		"Calculate squared distance between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3Angle(v1, v2));
	});
	raylibModule.SetValue("Vector3Angle", WithNote(i,
		"Calculate the angle (in radians) between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(Vector3Negate(v)));
	});
	raylibModule.SetValue("Vector3Negate", WithNote(i,
		"Negate provided vector (invert direction)"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Divide(v1, v2)));
	});
	raylibModule.SetValue("Vector3Divide", WithNote(i,
		"Divide v1 by v2 component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(Vector3Normalize(v)));
	});
	raylibModule.SetValue("Vector3Normalize", WithNote(i,
		"Scale vector to length 1 (a zero vector is returned unchanged)"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 1, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Project(v1, v2)));
	});
	raylibModule.SetValue("Vector3Project", WithNote(i,
		"Calculate the projection of v1 onto v2"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 1, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Reject(v1, v2)));
	});
	raylibModule.SetValue("Vector3Reject", WithNote(i,
		"Calculate the rejection of v1 from v2: the part of v1 perpendicular to v2"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{1, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 1, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		Vector3OrthoNormalize(&v1, &v2);
		ValueDict result;
		result.SetValue(CACHED_KEY("v1"), Vector3ToValue(v1));
		result.SetValue(CACHED_KEY("v2"), Vector3ToValue(v2));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("Vector3OrthoNormalize", WithNote(i,
		"Orthonormalize v1 and v2 (Gram-Schmidt); returns a map with keys v1 and v2, "
		"which are normalized and orthogonal to each other"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Matrix mat = ValueToMatrix(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Transform(v, mat)));
	});
	raylibModule.SetValue("Vector3Transform", WithNote(i,
		"Transforms a Vector3 by a given Matrix"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Quaternion q = ValueToQuaternion(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3RotateByQuaternion(v, q)));
	});
	raylibModule.SetValue("Vector3RotateByQuaternion", WithNote(i,
		"Transform a vector by quaternion rotation"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("axis", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Vector3 axis = ValueToVector3(context.GetArg(1));
		float angle = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3RotateByAxisAngle(v, axis, angle)));
	});
	raylibModule.SetValue("Vector3RotateByAxisAngle", WithNote(i,
		"Rotate vector v around axis by angle (in radians)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("target", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("maxDistance", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Vector3 target = ValueToVector3(context.GetArg(1));
		float maxDistance = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3MoveTowards(v, target, maxDistance)));
	});
	raylibModule.SetValue("Vector3MoveTowards", WithNote(i,
		"Move vector v towards target by at most maxDistance"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3Lerp(v1, v2, amount)));
	});
	raylibModule.SetValue("Vector3Lerp", WithNote(i,
		"Calculate linear interpolation between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("tangent1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("tangent2", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 tangent1 = ValueToVector3(context.GetArg(1));
		Vector3 v2 = ValueToVector3(context.GetArg(2));
		Vector3 tangent2 = ValueToVector3(context.GetArg(3));
		float amount = context.GetArg(4).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3CubicHermite(v1, tangent1, v2, tangent2, amount)));
	});
	raylibModule.SetValue("Vector3CubicHermite", WithNote(i,
		"Calculate cubic hermite interpolation between two vectors and their tangents as described "
		"in the GLTF 2.0 specification: "
		"https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("normal", Vector3ToValue(Vector3{0, 1, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Vector3 normal = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Reflect(v, normal)));
	});
	raylibModule.SetValue("Vector3Reflect", WithNote(i,
		"Calculate the reflection of v off a surface with the given normal"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Min(v1, v2)));
	});
	raylibModule.SetValue("Vector3Min", WithNote(i,
		"Get min value for each pair of components"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("v2", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v1 = ValueToVector3(context.GetArg(0));
		Vector3 v2 = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3ToValue(Vector3Max(v1, v2)));
	});
	raylibModule.SetValue("Vector3Max", WithNote(i,
		"Get max value for each pair of components"));

	i = Intrinsic::Create("");
	i.AddParam("p", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("a", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("b", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("c", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 p = ValueToVector3(context.GetArg(0));
		Vector3 a = ValueToVector3(context.GetArg(1));
		Vector3 b = ValueToVector3(context.GetArg(2));
		Vector3 c = ValueToVector3(context.GetArg(3));
		return IntrinsicResult(Vector3ToValue(Vector3Barycenter(p, a, b, c)));
	});
	raylibModule.SetValue("Vector3Barycenter", WithNote(i,
		"Compute barycentric coordinates (u, v, w), returned as {x, y, z}, for point p "
		"with respect to triangle (a, b, c). Assumes p lies in the plane of the triangle"));

	i = Intrinsic::Create("");
	i.AddParam("source", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("projection", MatrixToValue(MatrixIdentity()));
	i.AddParam("view", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 source = ValueToVector3(context.GetArg(0));
		Matrix projection = ValueToMatrix(context.GetArg(1));
		Matrix view = ValueToMatrix(context.GetArg(2));
		return IntrinsicResult(Vector3ToValue(Vector3Unproject(source, projection, view)));
	});
	raylibModule.SetValue("Vector3Unproject", WithNote(i,
		"Unproject a point from normalized device coordinates (-1 to 1) into world space, "
		"using the given projection and view matrices"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		float3 out = Vector3ToFloatV(v);
		ValueList result;
		result.Add(Value(out.v[0]));
		result.Add(Value(out.v[1]));
		result.Add(Value(out.v[2]));
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("Vector3ToFloatV", WithNote(i,
		"Get vector components as a list [x, y, z]"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(Vector3Invert(v)));
	});
	raylibModule.SetValue("Vector3Invert", WithNote(i,
		"Get the component-wise reciprocal of the vector (1/x, 1/y, 1/z)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("min", Vector3ToValue(Vector3{-1, -1, -1}));
	i.AddParam("max", Vector3ToValue(Vector3{1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Vector3 min = ValueToVector3(context.GetArg(1));
		Vector3 max = ValueToVector3(context.GetArg(2));
		return IntrinsicResult(Vector3ToValue(Vector3Clamp(v, min, max)));
	});
	raylibModule.SetValue("Vector3Clamp", WithNote(i,
		"Clamp the components of the vector between min and max values specified by the given "
		"vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("min", Value::zero);
	i.AddParam("max", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		float min = context.GetArg(1).FloatValue();
		float max = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3ClampValue(v, min, max)));
	});
	raylibModule.SetValue("Vector3ClampValue", WithNote(i,
		"Clamp the magnitude of the vector between min and max values"));

	i = Intrinsic::Create("");
	i.AddParam("p", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("q", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 p = ValueToVector3(context.GetArg(0));
		Vector3 q = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(Vector3Equals(p, q));
	});
	raylibModule.SetValue("Vector3Equals", WithNote(i,
		"Return true (1) if two vectors are almost equal (within a small relative tolerance), "
		"else false (0)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("n", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("r", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 v = ValueToVector3(context.GetArg(0));
		Vector3 n = ValueToVector3(context.GetArg(1));
		float r = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector3ToValue(Vector3Refract(v, n, r)));
	});
	raylibModule.SetValue("Vector3Refract", WithNote(i,
		"Compute the direction of a refracted ray. v: normalized direction of the incoming ray; "
		"n: normalized normal vector of the interface between the two media; r: ratio of the "
		"refractive index of the medium the ray comes from to that of the medium on the other side"));

	// Vector4 math
	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector4ToValue(Vector4Zero()));
	});
	raylibModule.SetValue("Vector4Zero", WithNote(i,
		"Get a Vector4 map {x, y, z, w} with all components 0"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Vector4ToValue(Vector4One()));
	});
	raylibModule.SetValue("Vector4One", WithNote(i,
		"Get a Vector4 map {x, y, z, w} with all components 1"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4ToValue(Vector4Add(v1, v2)));
	});
	raylibModule.SetValue("Vector4Add", WithNote(i,
		"Add two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("add", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		float add = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4AddValue(v, add)));
	});
	raylibModule.SetValue("Vector4AddValue", WithNote(i,
		"Add a number to each component of vector v"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4ToValue(Vector4Subtract(v1, v2)));
	});
	raylibModule.SetValue("Vector4Subtract", WithNote(i,
		"Subtract two vectors (v1 - v2)"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("sub", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		float sub = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4SubtractValue(v, sub)));
	});
	raylibModule.SetValue("Vector4SubtractValue", WithNote(i,
		"Subtract a number from each component of vector v"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		return IntrinsicResult(Vector4Length(v));
	});
	raylibModule.SetValue("Vector4Length", WithNote(i,
		"Calculate vector length"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		return IntrinsicResult(Vector4LengthSqr(v));
	});
	raylibModule.SetValue("Vector4LengthSqr", WithNote(i,
		"Calculate vector squared length"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4DotProduct(v1, v2));
	});
	raylibModule.SetValue("Vector4DotProduct", WithNote(i,
		"Calculate the dot product of two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4Distance(v1, v2));
	});
	raylibModule.SetValue("Vector4Distance", WithNote(i,
		"Calculate distance between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4DistanceSqr(v1, v2));
	});
	raylibModule.SetValue("Vector4DistanceSqr", WithNote(i,
		"Calculate squared distance between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("scale", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		float scale = context.GetArg(1).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4Scale(v, scale)));
	});
	raylibModule.SetValue("Vector4Scale", WithNote(i,
		"Scale vector components by value (multiply)"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4ToValue(Vector4Multiply(v1, v2)));
	});
	raylibModule.SetValue("Vector4Multiply", WithNote(i,
		"Multiply two vectors component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		return IntrinsicResult(Vector4ToValue(Vector4Negate(v)));
	});
	raylibModule.SetValue("Vector4Negate", WithNote(i,
		"Negate vector"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{1, 1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4ToValue(Vector4Divide(v1, v2)));
	});
	raylibModule.SetValue("Vector4Divide", WithNote(i,
		"Divide v1 by v2 component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		return IntrinsicResult(Vector4ToValue(Vector4Normalize(v)));
	});
	raylibModule.SetValue("Vector4Normalize", WithNote(i,
		"Scale vector to length 1 (a zero vector is returned unchanged)"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4ToValue(Vector4Min(v1, v2)));
	});
	raylibModule.SetValue("Vector4Min", WithNote(i,
		"Get min value for each pair of components"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4ToValue(Vector4Max(v1, v2)));
	});
	raylibModule.SetValue("Vector4Max", WithNote(i,
		"Get max value for each pair of components"));

	i = Intrinsic::Create("");
	i.AddParam("v1", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("v2", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v1 = ValueToVector4(context.GetArg(0));
		Vector4 v2 = ValueToVector4(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4Lerp(v1, v2, amount)));
	});
	raylibModule.SetValue("Vector4Lerp", WithNote(i,
		"Calculate linear interpolation between two vectors"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("target", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("maxDistance", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		Vector4 target = ValueToVector4(context.GetArg(1));
		float maxDistance = context.GetArg(2).FloatValue();
		return IntrinsicResult(Vector4ToValue(Vector4MoveTowards(v, target, maxDistance)));
	});
	raylibModule.SetValue("Vector4MoveTowards", WithNote(i,
		"Move vector v towards target by at most maxDistance"));

	i = Intrinsic::Create("");
	i.AddParam("v", Vector4ToValue(Vector4{1, 1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 v = ValueToVector4(context.GetArg(0));
		return IntrinsicResult(Vector4ToValue(Vector4Invert(v)));
	});
	raylibModule.SetValue("Vector4Invert", WithNote(i,
		"Get the component-wise reciprocal of the vector (1/x, 1/y, 1/z, 1/w)"));

	i = Intrinsic::Create("");
	i.AddParam("p", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.AddParam("q", Vector4ToValue(Vector4{0, 0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector4 p = ValueToVector4(context.GetArg(0));
		Vector4 q = ValueToVector4(context.GetArg(1));
		return IntrinsicResult(Vector4Equals(p, q));
	});
	raylibModule.SetValue("Vector4Equals", WithNote(i,
		"Return true (1) if two vectors are almost equal (within a small relative tolerance), "
		"else false (0)"));

	// Matrix math
	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		return IntrinsicResult(MatrixDeterminant(mat));
	});
	raylibModule.SetValue("MatrixDeterminant", WithNote(i,
		"Compute matrix determinant"));

	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		return IntrinsicResult(MatrixTrace(mat));
	});
	raylibModule.SetValue("MatrixTrace", WithNote(i,
		"Get the trace of the matrix (sum of the values along the diagonal)"));

	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(MatrixTranspose(mat)));
	});
	raylibModule.SetValue("MatrixTranspose", WithNote(i,
		"Transpose provided matrix"));

	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(MatrixInvert(mat)));
	});
	raylibModule.SetValue("MatrixInvert", WithNote(i,
		"Invert provided matrix"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(MatrixIdentity()));
	});
	raylibModule.SetValue("MatrixIdentity", WithNote(i,
		"Get identity matrix"));

	i = Intrinsic::Create("");
	i.AddParam("left", MatrixToValue(MatrixIdentity()));
	i.AddParam("right", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context.GetArg(0));
		Matrix right = ValueToMatrix(context.GetArg(1));
		return IntrinsicResult(MatrixToValue(MatrixAdd(left, right)));
	});
	raylibModule.SetValue("MatrixAdd", WithNote(i,
		"Add two matrices"));

	i = Intrinsic::Create("");
	i.AddParam("left", MatrixToValue(MatrixIdentity()));
	i.AddParam("right", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context.GetArg(0));
		Matrix right = ValueToMatrix(context.GetArg(1));
		return IntrinsicResult(MatrixToValue(MatrixSubtract(left, right)));
	});
	raylibModule.SetValue("MatrixSubtract", WithNote(i,
		"Subtract two matrices (left - right)"));

	i = Intrinsic::Create("");
	i.AddParam("left", MatrixToValue(MatrixIdentity()));
	i.AddParam("right", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context.GetArg(0));
		Matrix right = ValueToMatrix(context.GetArg(1));
		return IntrinsicResult(MatrixToValue(MatrixMultiply(left, right)));
	});
	raylibModule.SetValue("MatrixMultiply", WithNote(i,
		"Multiply two matrices; note that the order of the arguments matters"));

	i = Intrinsic::Create("");
	i.AddParam("left", MatrixToValue(MatrixIdentity()));
	i.AddParam("value", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix left = ValueToMatrix(context.GetArg(0));
		float value = context.GetArg(1).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixMultiplyValue(left, value)));
	});
	raylibModule.SetValue("MatrixMultiplyValue", WithNote(i,
		"Multiply matrix components by value"));

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("z", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixTranslate(x, y, z)));
	});
	raylibModule.SetValue("MatrixTranslate", WithNote(i,
		"Get translation matrix for offset (x, y, z)"));

	i = Intrinsic::Create("");
	i.AddParam("axis", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 axis = ValueToVector3(context.GetArg(0));
		float angle = context.GetArg(1).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotate(axis, angle)));
	});
	raylibModule.SetValue("MatrixRotate", WithNote(i,
		"Create rotation matrix from axis and angle (in radians)"));

	i = Intrinsic::Create("");
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float angle = context.GetArg(0).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotateX(angle)));
	});
	raylibModule.SetValue("MatrixRotateX", WithNote(i,
		"Get rotation matrix about the X axis; angle is in radians"));

	i = Intrinsic::Create("");
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float angle = context.GetArg(0).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotateY(angle)));
	});
	raylibModule.SetValue("MatrixRotateY", WithNote(i,
		"Get rotation matrix about the Y axis; angle is in radians"));

	i = Intrinsic::Create("");
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float angle = context.GetArg(0).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixRotateZ(angle)));
	});
	raylibModule.SetValue("MatrixRotateZ", WithNote(i,
		"Get rotation matrix about the Z axis; angle is in radians"));

	i = Intrinsic::Create("");
	i.AddParam("angle", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 angle = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(MatrixRotateXYZ(angle)));
	});
	raylibModule.SetValue("MatrixRotateXYZ", WithNote(i,
		"Get xyz-rotation matrix; angle is a Vector3 of rotations about each axis, in radians"));

	i = Intrinsic::Create("");
	i.AddParam("angle", Vector3ToValue(Vector3{0, 0, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 angle = ValueToVector3(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(MatrixRotateZYX(angle)));
	});
	raylibModule.SetValue("MatrixRotateZYX", WithNote(i,
		"Get zyx-rotation matrix; angle is a Vector3 of rotations about each axis, in radians"));

	i = Intrinsic::Create("");
	i.AddParam("x", Value::one);
	i.AddParam("y", Value::one);
	i.AddParam("z", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		return IntrinsicResult(MatrixToValue(MatrixScale(x, y, z)));
	});
	raylibModule.SetValue("MatrixScale", WithNote(i,
		"Get scaling matrix for scale factors (x, y, z)"));

	i = Intrinsic::Create("");
	i.AddParam("left", Value(-1));
	i.AddParam("right", Value(1));
	i.AddParam("bottom", Value(-1));
	i.AddParam("top", Value(1));
	i.AddParam("nearPlane", Value(0.01));
	i.AddParam("farPlane", Value(1000));
	i.set_Code(INTRINSIC_LAMBDA {
		double left = context.GetArg(0).DoubleValue();
		double right = context.GetArg(1).DoubleValue();
		double bottom = context.GetArg(2).DoubleValue();
		double top = context.GetArg(3).DoubleValue();
		double nearPlane = context.GetArg(4).DoubleValue();
		double farPlane = context.GetArg(5).DoubleValue();
		return IntrinsicResult(MatrixToValue(MatrixFrustum(left, right, bottom, top, nearPlane, farPlane)));
	});
	raylibModule.SetValue("MatrixFrustum", WithNote(i,
		"Get perspective projection matrix"));

	i = Intrinsic::Create("");
	i.AddParam("fovY", Value(45.0 * DEG2RAD));
	i.AddParam("aspect", Value(1));
	i.AddParam("nearPlane", Value(0.01));
	i.AddParam("farPlane", Value(1000));
	i.set_Code(INTRINSIC_LAMBDA {
		double fovY = context.GetArg(0).DoubleValue();
		double aspect = context.GetArg(1).DoubleValue();
		double nearPlane = context.GetArg(2).DoubleValue();
		double farPlane = context.GetArg(3).DoubleValue();
		return IntrinsicResult(MatrixToValue(MatrixPerspective(fovY, aspect, nearPlane, farPlane)));
	});
	raylibModule.SetValue("MatrixPerspective", WithNote(i,
		"Get perspective projection matrix; fovY is the vertical field of view in radians"));

	i = Intrinsic::Create("");
	i.AddParam("left", Value(-1));
	i.AddParam("right", Value(1));
	i.AddParam("bottom", Value(-1));
	i.AddParam("top", Value(1));
	i.AddParam("nearPlane", Value(0.01));
	i.AddParam("farPlane", Value(1000));
	i.set_Code(INTRINSIC_LAMBDA {
		double left = context.GetArg(0).DoubleValue();
		double right = context.GetArg(1).DoubleValue();
		double bottom = context.GetArg(2).DoubleValue();
		double top = context.GetArg(3).DoubleValue();
		double nearPlane = context.GetArg(4).DoubleValue();
		double farPlane = context.GetArg(5).DoubleValue();
		return IntrinsicResult(MatrixToValue(MatrixOrtho(left, right, bottom, top, nearPlane, farPlane)));
	});
	raylibModule.SetValue("MatrixOrtho", WithNote(i,
		"Get orthographic projection matrix"));

	i = Intrinsic::Create("");
	i.AddParam("eye", Vector3ToValue(Vector3{0, 0, 1}));
	i.AddParam("target", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("up", Vector3ToValue(Vector3{0, 1, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 eye = ValueToVector3(context.GetArg(0));
		Vector3 target = ValueToVector3(context.GetArg(1));
		Vector3 up = ValueToVector3(context.GetArg(2));
		return IntrinsicResult(MatrixToValue(MatrixLookAt(eye, target, up)));
	});
	raylibModule.SetValue("MatrixLookAt", WithNote(i,
		"Get camera look-at matrix (view matrix)"));

	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		float16 values = MatrixToFloatV(mat);
		ValueList result;
		for (int n = 0; n < 16; n++) result.Add(Value(values.v[n]));
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("MatrixToFloatV", WithNote(i,
		"Get matrix data as a flat list of 16 numbers, in column-major order"));

	i = Intrinsic::Create("");
	i.AddParam("translation", Vector3ToValue(Vector3{0, 0, 0}));
	i.AddParam("rotation", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("scale", Vector3ToValue(Vector3{1, 1, 1}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 translation = ValueToVector3(context.GetArg(0));
		Quaternion rotation = ValueToQuaternion(context.GetArg(1));
		Vector3 scale = ValueToVector3(context.GetArg(2));
		return IntrinsicResult(MatrixToValue(MatrixCompose(translation, rotation, scale)));
	});
	raylibModule.SetValue("MatrixCompose", WithNote(i,
		"Compose a transformation matrix from rotational, translational and scaling components"));

	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		Vector3 translation;
		Quaternion rotation;
		Vector3 scale;
		MatrixDecompose(mat, &translation, &rotation, &scale);

		ValueDict result;
		result.SetValue(CACHED_KEY("translation"), Vector3ToValue(translation));
		result.SetValue(CACHED_KEY("rotation"), QuaternionToValue(rotation));
		result.SetValue(CACHED_KEY("scale"), Vector3ToValue(scale));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("MatrixDecompose", WithNote(i,
		"Decompose a transformation matrix into translation, rotation and scale (removing "
		"any shear); returns a map with keys translation (Vector3), rotation (Quaternion) "
		"and scale (Vector3)"));

	// Quaternion math
	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		return IntrinsicResult(QuaternionToValue(QuaternionAdd(q1, q2)));
	});
	raylibModule.SetValue("QuaternionAdd", WithNote(i,
		"Add two quaternions"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("add", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		float add = context.GetArg(1).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionAddValue(q, add)));
	});
	raylibModule.SetValue("QuaternionAddValue", WithNote(i,
		"Add a number to each component of quaternion q"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		return IntrinsicResult(QuaternionToValue(QuaternionSubtract(q1, q2)));
	});
	raylibModule.SetValue("QuaternionSubtract", WithNote(i,
		"Subtract two quaternions"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("sub", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		float sub = context.GetArg(1).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionSubtractValue(q, sub)));
	});
	raylibModule.SetValue("QuaternionSubtractValue", WithNote(i,
		"Subtract a number from each component of quaternion q"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(QuaternionToValue(QuaternionIdentity()));
	});
	raylibModule.SetValue("QuaternionIdentity", WithNote(i,
		"Get identity quaternion"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		return IntrinsicResult(QuaternionLength(q));
	});
	raylibModule.SetValue("QuaternionLength", WithNote(i,
		"Compute the length of a quaternion"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		return IntrinsicResult(QuaternionToValue(QuaternionNormalize(q)));
	});
	raylibModule.SetValue("QuaternionNormalize", WithNote(i,
		"Normalize provided quaternion"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		return IntrinsicResult(QuaternionToValue(QuaternionInvert(q)));
	});
	raylibModule.SetValue("QuaternionInvert", WithNote(i,
		"Invert provided quaternion"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		return IntrinsicResult(QuaternionToValue(QuaternionMultiply(q1, q2)));
	});
	raylibModule.SetValue("QuaternionMultiply", WithNote(i,
		"Multiply two quaternions (q1 * q2)"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("mul", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		float mul = context.GetArg(1).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionScale(q, mul)));
	});
	raylibModule.SetValue("QuaternionScale", WithNote(i,
		"Scale quaternion by float value"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		return IntrinsicResult(QuaternionToValue(QuaternionDivide(q1, q2)));
	});
	raylibModule.SetValue("QuaternionDivide", WithNote(i,
		"Divide q1 by q2 component-wise"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionLerp(q1, q2, amount)));
	});
	raylibModule.SetValue("QuaternionLerp", WithNote(i,
		"Calculate linear interpolation between two quaternions"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionNlerp(q1, q2, amount)));
	});
	raylibModule.SetValue("QuaternionNlerp", WithNote(i,
		"Calculate slerp-optimized interpolation between two quaternions"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("amount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion q2 = ValueToQuaternion(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionSlerp(q1, q2, amount)));
	});
	raylibModule.SetValue("QuaternionSlerp", WithNote(i,
		"Calculate spherical linear interpolation between two quaternions"));

	i = Intrinsic::Create("");
	i.AddParam("q1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("outTangent1", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q2", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("inTangent2", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("t", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q1 = ValueToQuaternion(context.GetArg(0));
		Quaternion outTangent1 = ValueToQuaternion(context.GetArg(1));
		Quaternion q2 = ValueToQuaternion(context.GetArg(2));
		Quaternion inTangent2 = ValueToQuaternion(context.GetArg(3));
		float t = context.GetArg(4).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionCubicHermiteSpline(q1, outTangent1, q2, inTangent2, t)));
	});
	raylibModule.SetValue("QuaternionCubicHermiteSpline", WithNote(i,
		"Calculate quaternion cubic spline interpolation using Cubic Hermite Spline algorithm as "
		"described in the GLTF 2.0 specification: "
		"https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic"));

	i = Intrinsic::Create("");
	i.AddParam("from", Vector3ToValue(Vector3{1, 0, 0}));
	i.AddParam("to", Vector3ToValue(Vector3{0, 1, 0}));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 from = ValueToVector3(context.GetArg(0));
		Vector3 to = ValueToVector3(context.GetArg(1));
		return IntrinsicResult(QuaternionToValue(QuaternionFromVector3ToVector3(from, to)));
	});
	raylibModule.SetValue("QuaternionFromVector3ToVector3", WithNote(i,
		"Calculate quaternion based on the rotation from one vector to another"));

	i = Intrinsic::Create("");
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix mat = ValueToMatrix(context.GetArg(0));
		return IntrinsicResult(QuaternionToValue(QuaternionFromMatrix(mat)));
	});
	raylibModule.SetValue("QuaternionFromMatrix", WithNote(i,
		"Get a quaternion for a given rotation matrix"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		return IntrinsicResult(MatrixToValue(QuaternionToMatrix(q)));
	});
	raylibModule.SetValue("QuaternionToMatrix", WithNote(i,
		"Get a matrix for a given quaternion"));

	i = Intrinsic::Create("");
	i.AddParam("axis", Vector3ToValue(Vector3{0, 1, 0}));
	i.AddParam("angle", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Vector3 axis = ValueToVector3(context.GetArg(0));
		float angle = context.GetArg(1).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionFromAxisAngle(axis, angle)));
	});
	raylibModule.SetValue("QuaternionFromAxisAngle", WithNote(i,
		"Get rotation quaternion for an axis and angle (in radians)"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		Vector3 outAxis;
		float outAngle = 0;
		QuaternionToAxisAngle(q, &outAxis, &outAngle);
		ValueDict result;
		result.SetValue(CACHED_KEY("axis"), Vector3ToValue(outAxis));
		result.SetValue(CACHED_KEY("angle"), Value(outAngle));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("QuaternionToAxisAngle", WithNote(i,
		"Get the rotation axis and angle for a given quaternion; returns a map with "
		"keys axis (Vector3) and angle (in radians)"));

	i = Intrinsic::Create("");
	i.AddParam("pitch", Value::zero);
	i.AddParam("yaw", Value::zero);
	i.AddParam("roll", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float pitch = context.GetArg(0).FloatValue();
		float yaw = context.GetArg(1).FloatValue();
		float roll = context.GetArg(2).FloatValue();
		return IntrinsicResult(QuaternionToValue(QuaternionFromEuler(pitch, yaw, roll)));
	});
	raylibModule.SetValue("QuaternionFromEuler", WithNote(i,
		"Get the quaternion equivalent to Euler angles (in radians): pitch (about X), "
		"yaw (about Y) and roll (about Z). Rotation order is ZYX"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		return IntrinsicResult(Vector3ToValue(QuaternionToEuler(q)));
	});
	raylibModule.SetValue("QuaternionToEuler", WithNote(i,
		"Get the Euler angles equivalent to quaternion, as a Vector3 of rotations (in "
		"radians) about the X, Y and Z axes"));

	i = Intrinsic::Create("");
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("mat", MatrixToValue(MatrixIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion q = ValueToQuaternion(context.GetArg(0));
		Matrix mat = ValueToMatrix(context.GetArg(1));
		return IntrinsicResult(QuaternionToValue(QuaternionTransform(q, mat)));
	});
	raylibModule.SetValue("QuaternionTransform", WithNote(i,
		"Transform a quaternion given a transformation matrix"));

	i = Intrinsic::Create("");
	i.AddParam("p", QuaternionToValue(QuaternionIdentity()));
	i.AddParam("q", QuaternionToValue(QuaternionIdentity()));
	i.set_Code(INTRINSIC_LAMBDA {
		Quaternion p = ValueToQuaternion(context.GetArg(0));
		Quaternion q = ValueToQuaternion(context.GetArg(1));
		return IntrinsicResult(QuaternionEquals(p, q));
	});
	raylibModule.SetValue("QuaternionEquals", WithNote(i,
		"Return true (1) if two quaternions are almost equal (within a small relative tolerance), "
		"else false (0)"));
}
