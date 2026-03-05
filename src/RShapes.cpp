//
//  RShapes.cpp
//  MSRLWeb
//
//  Raylib Shapes module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"

using namespace MiniScript;

void AddRShapesMethods(ValueDict raylibModule) {
	Intrinsic *i;

	// Pixel drawing

	i = Intrinsic::Create("");
	i->AddParam("posX", Value::zero);
	i->AddParam("posY", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int posX = context->GetVar(String("posX")).IntValue();
		int posY = context->GetVar(String("posY")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPixel(posX, posY, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPixel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPixelV(position, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPixelV", i->GetFunc());

	// Line drawing

	i = Intrinsic::Create("");
	i->AddParam("startPosX", Value::zero);
	i->AddParam("startPosY", Value::zero);
	i->AddParam("endPosX", Value::zero);
	i->AddParam("endPosY", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int startPosX = context->GetVar(String("startPosX")).IntValue();
		int startPosY = context->GetVar(String("startPosY")).IntValue();
		int endPosX = context->GetVar(String("endPosX")).IntValue();
		int endPosY = context->GetVar(String("endPosY")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLine(startPosX, startPosY, endPosX, endPosY, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLine", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("endPos", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context->GetVar(String("startPos")));
		Vector2 endPos = ValueToVector2(context->GetVar(String("endPos")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLineV(startPos, endPos, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLineV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("endPos", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("thick", Value(1));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context->GetVar(String("startPos")));
		Vector2 endPos = ValueToVector2(context->GetVar(String("endPos")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLineEx(startPos, endPos, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLineEx", i->GetFunc());

	// Circle drawing

	i = Intrinsic::Create("");
	i->AddParam("centerX", Value(100));
	i->AddParam("centerY", Value(100));
	i->AddParam("radius", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircle(centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("radius", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircleV(center, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircleV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("centerX", Value(100));
	i->AddParam("centerY", Value(100));
	i->AddParam("radius", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircleLines(centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircleLines", i->GetFunc());

	// Ellipse drawing

	i = Intrinsic::Create("");
	i->AddParam("centerX", Value(100));
	i->AddParam("centerY", Value(100));
	i->AddParam("radiusH", Value(32));
	i->AddParam("radiusV", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		float radiusH = context->GetVar(String("radiusH")).FloatValue();
		float radiusV = context->GetVar(String("radiusV")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawEllipse(centerX, centerY, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawEllipse", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("centerX", Value(100));
	i->AddParam("centerY", Value(100));
	i->AddParam("radiusH", Value(32));
	i->AddParam("radiusV", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		float radiusH = context->GetVar(String("radiusH")).FloatValue();
		float radiusV = context->GetVar(String("radiusV")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawEllipseLines(centerX, centerY, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawEllipseLines", i->GetFunc());

	// Ring drawing

	i = Intrinsic::Create("");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("innerRadius", Value(20));
	i->AddParam("outerRadius", Value(32));
	i->AddParam("startAngle", Value::zero);
	i->AddParam("endAngle", Value(360));
	i->AddParam("segments", Value(36));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float innerRadius = context->GetVar(String("innerRadius")).FloatValue();
		float outerRadius = context->GetVar(String("outerRadius")).FloatValue();
		float startAngle = context->GetVar(String("startAngle")).FloatValue();
		float endAngle = context->GetVar(String("endAngle")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRing(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRing", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("innerRadius", Value(20));
	i->AddParam("outerRadius", Value(32));
	i->AddParam("startAngle", Value::zero);
	i->AddParam("endAngle", Value(360));
	i->AddParam("segments", Value(36));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float innerRadius = context->GetVar(String("innerRadius")).FloatValue();
		float outerRadius = context->GetVar(String("outerRadius")).FloatValue();
		float startAngle = context->GetVar(String("startAngle")).FloatValue();
		float endAngle = context->GetVar(String("endAngle")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRingLines(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRingLines", i->GetFunc());

	// Rectangle drawing

	i = Intrinsic::Create("");
	i->AddParam("x", Value::zero);
	i->AddParam("y", Value::zero);
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangle(x, y, width, height, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("size", Vector2ToValue(Vector2{256, 256}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Vector2 size = ValueToVector2(context->GetVar(String("size")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleV(position, size, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleRec(rec, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("rotation", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Vector2 origin = ValueToVector2(context->GetVar(String("origin")));
		float rotation = context->GetVar(String("rotation")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectanglePro(rec, origin, rotation, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectanglePro", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleLines(rec.x, rec.y, rec.width, rec.height, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("lineThick", Value(1));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		float lineThick = context->GetVar(String("lineThick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleLinesEx(rec, lineThick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleLinesEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("roundness", Value(0.5));
	i->AddParam("segments", Value(36));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		float roundness = context->GetVar(String("roundness")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleRounded(rec, roundness, segments, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleRounded", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("roundness", Value(0.5));
	i->AddParam("segments", Value(36));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		float roundness = context->GetVar(String("roundness")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleRoundedLines(rec, roundness, segments, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleRoundedLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("posX", Value::zero);
	i->AddParam("posY", Value::zero);
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("color1", ColorToValue(WHITE));
	i->AddParam("color2", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		int posX = context->GetVar(String("posX")).IntValue();
		int posY = context->GetVar(String("posY")).IntValue();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		Color color1 = ValueToColor(context->GetVar(String("color1")));
		Color color2 = ValueToColor(context->GetVar(String("color2")));
		DrawRectangleGradientV(posX, posY, width, height, color1, color2);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleGradientV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("posX", Value::zero);
	i->AddParam("posY", Value::zero);
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("color1", ColorToValue(WHITE));
	i->AddParam("color2", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		int posX = context->GetVar(String("posX")).IntValue();
		int posY = context->GetVar(String("posY")).IntValue();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		Color color1 = ValueToColor(context->GetVar(String("color1")));
		Color color2 = ValueToColor(context->GetVar(String("color2")));
		DrawRectangleGradientH(posX, posY, width, height, color1, color2);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleGradientH", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("col1");
	i->AddParam("col2");
	i->AddParam("col3");
	i->AddParam("col4");
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Color col1 = ValueToColor(context->GetVar(String("col1")));
		Color col2 = ValueToColor(context->GetVar(String("col2")));
		Color col3 = ValueToColor(context->GetVar(String("col3")));
		Color col4 = ValueToColor(context->GetVar(String("col4")));
		DrawRectangleGradientEx(rec, col1, col2, col3, col4);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleGradientEx", i->GetFunc());

	// Triangle drawing

	i = Intrinsic::Create("");
	i->AddParam("v1");
	i->AddParam("v2");
	i->AddParam("v3");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		Vector2 v3 = ValueToVector2(context->GetVar(String("v3")));
		Color color = ValueToColor(context->GetVar(String("color")));
		// Check winding order and ensure counter-clockwise (in screen coords where Y is down)
		float det = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);
		if (det > 0) {
			// Clockwise in screen space - swap v2 and v3 to make it counter-clockwise
			DrawTriangle(v1, v3, v2, color);
		} else {
			DrawTriangle(v1, v2, v3, color);
		}
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTriangle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("v1");
	i->AddParam("v2");
	i->AddParam("v3");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		Vector2 v3 = ValueToVector2(context->GetVar(String("v3")));
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawTriangleLines(v1, v2, v3, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTriangleLines", i->GetFunc());

	// Polygon drawing

	i = Intrinsic::Create("");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("sides", Value(6));
	i->AddParam("radius", Value(32));
	i->AddParam("rotation", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		int sides = context->GetVar(String("sides")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		float rotation = context->GetVar(String("rotation")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPoly(center, sides, radius, rotation, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPoly", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("sides", Value(6));
	i->AddParam("radius", Value(32));
	i->AddParam("rotation", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		int sides = context->GetVar(String("sides")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		float rotation = context->GetVar(String("rotation")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPolyLines(center, sides, radius, rotation, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPolyLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("sides", Value(6));
	i->AddParam("radius", Value(32));
	i->AddParam("rotation", Value::zero);
	i->AddParam("lineThick", Value(1));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		int sides = context->GetVar(String("sides")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		float rotation = context->GetVar(String("rotation")).FloatValue();
		float lineThick = context->GetVar(String("lineThick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawPolyLinesEx(center, sides, radius, rotation, lineThick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawPolyLinesEx", i->GetFunc());

	// Collision detection

	i = Intrinsic::Create("");
	i->AddParam("rec1");
	i->AddParam("rec2");
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec1 = ValueToRectangle(context->GetVar(String("rec1")));
		Rectangle rec2 = ValueToRectangle(context->GetVar(String("rec2")));
		return IntrinsicResult(CheckCollisionRecs(rec1, rec2));
	};
	raylibModule.SetValue("CheckCollisionRecs", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center1");
	i->AddParam("radius1");
	i->AddParam("center2");
	i->AddParam("radius2");
	i->code = INTRINSIC_LAMBDA {
		Vector2 center1 = ValueToVector2(context->GetVar(String("center1")));
		float radius1 = context->GetVar(String("radius1")).FloatValue();
		Vector2 center2 = ValueToVector2(context->GetVar(String("center2")));
		float radius2 = context->GetVar(String("radius2")).FloatValue();
		return IntrinsicResult(CheckCollisionCircles(center1, radius1, center2, radius2));
	};
	raylibModule.SetValue("CheckCollisionCircles", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radius");
	i->AddParam("rec");
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		return IntrinsicResult(CheckCollisionCircleRec(center, radius, rec));
	};
	raylibModule.SetValue("CheckCollisionCircleRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("point");
	i->AddParam("rec");
	i->code = INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context->GetVar(String("point")));
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		return IntrinsicResult(CheckCollisionPointRec(point, rec));
	};
	raylibModule.SetValue("CheckCollisionPointRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("point");
	i->AddParam("center");
	i->AddParam("radius");
	i->code = INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context->GetVar(String("point")));
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		return IntrinsicResult(CheckCollisionPointCircle(point, center, radius));
	};
	raylibModule.SetValue("CheckCollisionPointCircle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("point");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->code = INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context->GetVar(String("point")));
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		return IntrinsicResult(CheckCollisionPointTriangle(point, p1, p2, p3));
	};
	raylibModule.SetValue("CheckCollisionPointTriangle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("rec1");
	i->AddParam("rec2");
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec1 = ValueToRectangle(context->GetVar(String("rec1")));
		Rectangle rec2 = ValueToRectangle(context->GetVar(String("rec2")));
		Rectangle result = GetCollisionRec(rec1, rec2);
		return IntrinsicResult(RectangleToValue(result));
	};
	raylibModule.SetValue("GetCollisionRec", i->GetFunc());

	// Additional collision detection

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radius");
	i->AddParam("p1");
	i->AddParam("p2");
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		return IntrinsicResult(CheckCollisionCircleLine(center, radius, p1, p2));
	};
	raylibModule.SetValue("CheckCollisionCircleLine", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("startPos1");
	i->AddParam("endPos1");
	i->AddParam("startPos2");
	i->AddParam("endPos2");
	i->code = INTRINSIC_LAMBDA {
		Vector2 startPos1 = ValueToVector2(context->GetVar(String("startPos1")));
		Vector2 endPos1 = ValueToVector2(context->GetVar(String("endPos1")));
		Vector2 startPos2 = ValueToVector2(context->GetVar(String("startPos2")));
		Vector2 endPos2 = ValueToVector2(context->GetVar(String("endPos2")));
		Vector2 collisionPoint;
		bool result = CheckCollisionLines(startPos1, endPos1, startPos2, endPos2, &collisionPoint);
		if (!result) return IntrinsicResult::Null;
		return IntrinsicResult(Vector2ToValue(collisionPoint));
	};
	raylibModule.SetValue("CheckCollisionLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("point");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("threshold");
	i->code = INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context->GetVar(String("point")));
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		int threshold = context->GetVar(String("threshold")).IntValue();
		return IntrinsicResult(CheckCollisionPointLine(point, p1, p2, threshold));
	};
	raylibModule.SetValue("CheckCollisionPointLine", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("point");
	i->AddParam("points");
	i->code = INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context->GetVar(String("point")));
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult(Value::zero);

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		bool result = CheckCollisionPointPoly(point, points, pointCount);
		delete[] points;
		return IntrinsicResult(result);
	};
	raylibModule.SetValue("CheckCollisionPointPoly", i->GetFunc());

	// Additional circle drawing

	i = Intrinsic::Create("");
	i->AddParam("centerX", Value::zero);
	i->AddParam("centerY", Value::zero);
	i->AddParam("radius", Value(10.0));
	i->AddParam("colorInner", ColorToValue(WHITE));
	i->AddParam("colorOuter", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		float radius = context->GetVar(String("radius")).FloatValue();
		Color colorInner = ValueToColor(context->GetVar(String("colorInner")));
		Color colorOuter = ValueToColor(context->GetVar(String("colorOuter")));
		DrawCircleGradient(centerX, centerY, radius, colorInner, colorOuter);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircleGradient", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radius", Value(10.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircleLinesV(center, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircleLinesV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radius", Value(10.0));
	i->AddParam("startAngle", Value::zero);
	i->AddParam("endAngle", Value(90.0));
	i->AddParam("segments", Value(36));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		float startAngle = context->GetVar(String("startAngle")).FloatValue();
		float endAngle = context->GetVar(String("endAngle")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircleSector(center, radius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircleSector", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radius", Value(10.0));
	i->AddParam("startAngle", Value::zero);
	i->AddParam("endAngle", Value(90.0));
	i->AddParam("segments", Value(36));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radius = context->GetVar(String("radius")).FloatValue();
		float startAngle = context->GetVar(String("startAngle")).FloatValue();
		float endAngle = context->GetVar(String("endAngle")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawCircleSectorLines(center, radius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawCircleSectorLines", i->GetFunc());

#if RAYLIB_VERSION_GT(5, 5)
	// Additional ellipse drawing

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radiusH", Value(10.0));
	i->AddParam("radiusV", Value(5.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radiusH = context->GetVar(String("radiusH")).FloatValue();
		float radiusV = context->GetVar(String("radiusV")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawEllipseV(center, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawEllipseV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("center");
	i->AddParam("radiusH", Value(10.0));
	i->AddParam("radiusV", Value(5.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		float radiusH = context->GetVar(String("radiusH")).FloatValue();
		float radiusV = context->GetVar(String("radiusV")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawEllipseLinesV(center, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawEllipseLinesV", i->GetFunc());
#endif /* RAYLIB_VERSION_GT(5, 5) */

	// Additional line drawing

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context->GetVar(String("startPos")));
		Vector2 endPos = ValueToVector2(context->GetVar(String("endPos")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLineBezier(startPos, endPos, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLineBezier", i->GetFunc());

#if RAYLIB_VERSION_GT(5, 5)
	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("dashSize");
	i->AddParam("spaceSize");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context->GetVar(String("startPos")));
		Vector2 endPos = ValueToVector2(context->GetVar(String("endPos")));
		int dashSize = context->GetVar(String("dashSize")).IntValue();
		int spaceSize = context->GetVar(String("spaceSize")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLineDashed(startPos, endPos, dashSize, spaceSize, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLineDashed", i->GetFunc());
#endif /* RAYLIB_VERSION_GT(5, 5) */

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		Color color = ValueToColor(context->GetVar(String("color")));
		DrawLineStrip(points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawLineStrip", i->GetFunc());

	// Additional rectangle drawing

	i = Intrinsic::Create("");
	i->AddParam("rec");
	i->AddParam("roundness", Value(0.0));
	i->AddParam("segments", Value::zero);
	i->AddParam("lineThick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		float roundness = context->GetVar(String("roundness")).FloatValue();
		int segments = context->GetVar(String("segments")).IntValue();
		float lineThick = context->GetVar(String("lineThick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawRectangleRoundedLinesEx", i->GetFunc());

	// Spline drawing

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineLinear(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineLinear", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 4) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineBasis(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineBasis", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineCatmullRom(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineCatmullRom", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineBezierQuadratic(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineBezierQuadratic", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 4) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineBezierCubic(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineBezierCubic", i->GetFunc());

	// Spline segment drawing

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineSegmentLinear(p1, p2, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineSegmentLinear", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("p4");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		Vector2 p4 = ValueToVector2(context->GetVar(String("p4")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineSegmentBasis(p1, p2, p3, p4, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineSegmentBasis", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("p4");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		Vector2 p4 = ValueToVector2(context->GetVar(String("p4")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineSegmentCatmullRom(p1, p2, p3, p4, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineSegmentCatmullRom", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineSegmentBezierQuadratic(p1, p2, p3, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineSegmentBezierQuadratic", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("p4");
	i->AddParam("thick", Value(1.0));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		Vector2 p4 = ValueToVector2(context->GetVar(String("p4")));
		float thick = context->GetVar(String("thick")).FloatValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		DrawSplineSegmentBezierCubic(p1, p2, p3, p4, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawSplineSegmentBezierCubic", i->GetFunc());

	// Spline point evaluation functions

	i = Intrinsic::Create("");
	i->AddParam("startPos");
	i->AddParam("endPos");
	i->AddParam("t");
	i->code = INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context->GetVar(String("startPos")));
		Vector2 endPos = ValueToVector2(context->GetVar(String("endPos")));
		float t = context->GetVar(String("t")).FloatValue();
		Vector2 result = GetSplinePointLinear(startPos, endPos, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetSplinePointLinear", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("p4");
	i->AddParam("t");
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		Vector2 p4 = ValueToVector2(context->GetVar(String("p4")));
		float t = context->GetVar(String("t")).FloatValue();
		Vector2 result = GetSplinePointBasis(p1, p2, p3, p4, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetSplinePointBasis", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("p2");
	i->AddParam("p3");
	i->AddParam("p4");
	i->AddParam("t");
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 p2 = ValueToVector2(context->GetVar(String("p2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		Vector2 p4 = ValueToVector2(context->GetVar(String("p4")));
		float t = context->GetVar(String("t")).FloatValue();
		Vector2 result = GetSplinePointCatmullRom(p1, p2, p3, p4, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetSplinePointCatmullRom", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("c2");
	i->AddParam("p3");
	i->AddParam("t");
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 c2 = ValueToVector2(context->GetVar(String("c2")));
		Vector2 p3 = ValueToVector2(context->GetVar(String("p3")));
		float t = context->GetVar(String("t")).FloatValue();
		Vector2 result = GetSplinePointBezierQuad(p1, c2, p3, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetSplinePointBezierQuad", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("p1");
	i->AddParam("c2");
	i->AddParam("c3");
	i->AddParam("p4");
	i->AddParam("t");
	i->code = INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context->GetVar(String("p1")));
		Vector2 c2 = ValueToVector2(context->GetVar(String("c2")));
		Vector2 c3 = ValueToVector2(context->GetVar(String("c3")));
		Vector2 p4 = ValueToVector2(context->GetVar(String("p4")));
		float t = context->GetVar(String("t")).FloatValue();
		Vector2 result = GetSplinePointBezierCubic(p1, c2, c3, p4, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(resultMap);
	};
	raylibModule.SetValue("GetSplinePointBezierCubic", i->GetFunc());

	// Additional triangle drawing

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		Color color = ValueToColor(context->GetVar(String("color")));
		DrawTriangleFan(points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTriangleFan", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("points");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		Color color = ValueToColor(context->GetVar(String("color")));
		DrawTriangleStrip(points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTriangleStrip", i->GetFunc());

	// Texture setup

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("source");
	i->code = INTRINSIC_LAMBDA {
		Texture texture = ValueToTexture(context->GetVar(String("texture")));
		Rectangle source = ValueToRectangle(context->GetVar(String("source")));
		SetShapesTexture(texture, source);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetShapesTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Texture2D texture = GetShapesTexture();
		return IntrinsicResult(TextureToValue(texture));
	};
	raylibModule.SetValue("GetShapesTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Rectangle rect = GetShapesTextureRectangle();
		return IntrinsicResult(RectangleToValue(rect));
	};
	raylibModule.SetValue("GetShapesTextureRectangle", i->GetFunc());
}
