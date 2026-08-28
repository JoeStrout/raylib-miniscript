//
//  RShapes.cpp
//  MSRLWeb
//
//  Raylib Shapes module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "miniscript.h"
#include "macros.h"

using namespace MiniScript;

void AddRShapesMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Pixel drawing

	i = Intrinsic::Create("");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int posX = context.GetArg(0).IntValue();
		int posY = context.GetArg(1).IntValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawPixel(posX, posY, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPixel", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawPixelV(position, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPixelV", i.GetFunc());

	// Line drawing

	i = Intrinsic::Create("");
	i.AddParam("startPosX", Value::zero);
	i.AddParam("startPosY", Value::zero);
	i.AddParam("endPosX", Value::zero);
	i.AddParam("endPosY", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int startPosX = context.GetArg(0).IntValue();
		int startPosY = context.GetArg(1).IntValue();
		int endPosX = context.GetArg(2).IntValue();
		int endPosY = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawLine(startPosX, startPosY, endPosX, endPosY, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLine", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("endPos", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context.GetArg(0));
		Vector2 endPos = ValueToVector2(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		DrawLineV(startPos, endPos, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLineV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("endPos", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("thick", Value(1));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context.GetArg(0));
		Vector2 endPos = ValueToVector2(context.GetArg(1));
		float thick = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawLineEx(startPos, endPos, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLineEx", i.GetFunc());

	// Circle drawing

	i = Intrinsic::Create("");
	i.AddParam("centerX", Value(100));
	i.AddParam("centerY", Value(100));
	i.AddParam("radius", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int centerX = context.GetArg(0).IntValue();
		int centerY = context.GetArg(1).IntValue();
		float radius = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawCircle(centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("radius", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawCircleV(center, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("centerX", Value(100));
	i.AddParam("centerY", Value(100));
	i.AddParam("radius", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int centerX = context.GetArg(0).IntValue();
		int centerY = context.GetArg(1).IntValue();
		float radius = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawCircleLines(centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleLines", i.GetFunc());

	// Ellipse drawing

	i = Intrinsic::Create("");
	i.AddParam("centerX", Value(100));
	i.AddParam("centerY", Value(100));
	i.AddParam("radiusH", Value(32));
	i.AddParam("radiusV", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int centerX = context.GetArg(0).IntValue();
		int centerY = context.GetArg(1).IntValue();
		float radiusH = context.GetArg(2).FloatValue();
		float radiusV = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawEllipse(centerX, centerY, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawEllipse", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("centerX", Value(100));
	i.AddParam("centerY", Value(100));
	i.AddParam("radiusH", Value(32));
	i.AddParam("radiusV", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int centerX = context.GetArg(0).IntValue();
		int centerY = context.GetArg(1).IntValue();
		float radiusH = context.GetArg(2).FloatValue();
		float radiusV = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawEllipseLines(centerX, centerY, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawEllipseLines", i.GetFunc());

	// Ring drawing

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("innerRadius", Value(20));
	i.AddParam("outerRadius", Value(32));
	i.AddParam("startAngle", Value::zero);
	i.AddParam("endAngle", Value(360));
	i.AddParam("segments", Value(36));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float innerRadius = context.GetArg(1).FloatValue();
		float outerRadius = context.GetArg(2).FloatValue();
		float startAngle = context.GetArg(3).FloatValue();
		float endAngle = context.GetArg(4).FloatValue();
		int segments = context.GetArg(5).IntValue();
		Color color = ValueToColor(context.GetArg(6));
		DrawRing(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRing", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("innerRadius", Value(20));
	i.AddParam("outerRadius", Value(32));
	i.AddParam("startAngle", Value::zero);
	i.AddParam("endAngle", Value(360));
	i.AddParam("segments", Value(36));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float innerRadius = context.GetArg(1).FloatValue();
		float outerRadius = context.GetArg(2).FloatValue();
		float startAngle = context.GetArg(3).FloatValue();
		float endAngle = context.GetArg(4).FloatValue();
		int segments = context.GetArg(5).IntValue();
		Color color = ValueToColor(context.GetArg(6));
		DrawRingLines(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRingLines", i.GetFunc());

	// Rectangle drawing

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawRectangle(x, y, width, height, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("size", Vector2ToValue(Vector2{256, 256}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		Vector2 size = ValueToVector2(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		DrawRectangleV(position, size, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawRectangleRec(rec, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		Vector2 origin = ValueToVector2(context.GetArg(1));
		float rotation = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawRectanglePro(rec, origin, rotation, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectanglePro", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawRectangleLines(x, y, width, height, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("lineThick", Value(1));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		float lineThick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawRectangleLinesEx(rec, lineThick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleLinesEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("roundness", Value(0.5));
	i.AddParam("segments", Value(36));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		float roundness = context.GetArg(1).FloatValue();
		int segments = context.GetArg(2).IntValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawRectangleRounded(rec, roundness, segments, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleRounded", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("roundness", Value(0.5));
	i.AddParam("segments", Value(36));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		float roundness = context.GetArg(1).FloatValue();
		int segments = context.GetArg(2).IntValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawRectangleRoundedLines(rec, roundness, segments, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleRoundedLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("color1", ColorToValue(WHITE));
	i.AddParam("color2", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int posX = context.GetArg(0).IntValue();
		int posY = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		Color color1 = ValueToColor(context.GetArg(4));
		Color color2 = ValueToColor(context.GetArg(5));
		DrawRectangleGradientV(posX, posY, width, height, color1, color2);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleGradientV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("color1", ColorToValue(WHITE));
	i.AddParam("color2", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int posX = context.GetArg(0).IntValue();
		int posY = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		Color color1 = ValueToColor(context.GetArg(4));
		Color color2 = ValueToColor(context.GetArg(5));
		DrawRectangleGradientH(posX, posY, width, height, color1, color2);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleGradientH", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("col1");
	i.AddParam("col2");
	i.AddParam("col3");
	i.AddParam("col4");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		Color col1 = ValueToColor(context.GetArg(1));
		Color col2 = ValueToColor(context.GetArg(2));
		Color col3 = ValueToColor(context.GetArg(3));
		Color col4 = ValueToColor(context.GetArg(4));
		DrawRectangleGradientEx(rec, col1, col2, col3, col4);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleGradientEx", i.GetFunc());

	// Triangle drawing

	i = Intrinsic::Create("");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		Vector2 v3 = ValueToVector2(context.GetArg(2));
		Color color = ValueToColor(context.GetArg(3));
		// Check winding order and ensure counter-clockwise (in screen coords where Y is down)
		float det = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);
		if (det > 0) {
			// Clockwise in screen space - swap v2 and v3 to make it counter-clockwise
			DrawTriangle(v1, v3, v2, color);
		} else {
			DrawTriangle(v1, v2, v3, color);
		}
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		Vector2 v3 = ValueToVector2(context.GetArg(2));
		Color color = ValueToColor(context.GetArg(3));
		DrawTriangleLines(v1, v2, v3, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangleLines", i.GetFunc());

	// Polygon drawing

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("sides", Value(6));
	i.AddParam("radius", Value(32));
	i.AddParam("rotation", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		int sides = context.GetArg(1).IntValue();
		float radius = context.GetArg(2).FloatValue();
		float rotation = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawPoly(center, sides, radius, rotation, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPoly", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("sides", Value(6));
	i.AddParam("radius", Value(32));
	i.AddParam("rotation", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		int sides = context.GetArg(1).IntValue();
		float radius = context.GetArg(2).FloatValue();
		float rotation = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawPolyLines(center, sides, radius, rotation, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPolyLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("sides", Value(6));
	i.AddParam("radius", Value(32));
	i.AddParam("rotation", Value::zero);
	i.AddParam("lineThick", Value(1));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		int sides = context.GetArg(1).IntValue();
		float radius = context.GetArg(2).FloatValue();
		float rotation = context.GetArg(3).FloatValue();
		float lineThick = context.GetArg(4).FloatValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawPolyLinesEx(center, sides, radius, rotation, lineThick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPolyLinesEx", i.GetFunc());

	// Collision detection

	i = Intrinsic::Create("");
	i.AddParam("rec1");
	i.AddParam("rec2");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec1 = ValueToRectangle(context.GetArg(0));
		Rectangle rec2 = ValueToRectangle(context.GetArg(1));
		return IntrinsicResult(CheckCollisionRecs(rec1, rec2));
	});
	raylibModule.SetValue("CheckCollisionRecs", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center1");
	i.AddParam("radius1");
	i.AddParam("center2");
	i.AddParam("radius2");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center1 = ValueToVector2(context.GetArg(0));
		float radius1 = context.GetArg(1).FloatValue();
		Vector2 center2 = ValueToVector2(context.GetArg(2));
		float radius2 = context.GetArg(3).FloatValue();
		return IntrinsicResult(CheckCollisionCircles(center1, radius1, center2, radius2));
	});
	raylibModule.SetValue("CheckCollisionCircles", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius");
	i.AddParam("rec");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		Rectangle rec = ValueToRectangle(context.GetArg(2));
		return IntrinsicResult(CheckCollisionCircleRec(center, radius, rec));
	});
	raylibModule.SetValue("CheckCollisionCircleRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("point");
	i.AddParam("rec");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context.GetArg(0));
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		return IntrinsicResult(CheckCollisionPointRec(point, rec));
	});
	raylibModule.SetValue("CheckCollisionPointRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("point");
	i.AddParam("center");
	i.AddParam("radius");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context.GetArg(0));
		Vector2 center = ValueToVector2(context.GetArg(1));
		float radius = context.GetArg(2).FloatValue();
		return IntrinsicResult(CheckCollisionPointCircle(point, center, radius));
	});
	raylibModule.SetValue("CheckCollisionPointCircle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("point");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context.GetArg(0));
		Vector2 p1 = ValueToVector2(context.GetArg(1));
		Vector2 p2 = ValueToVector2(context.GetArg(2));
		Vector2 p3 = ValueToVector2(context.GetArg(3));
		return IntrinsicResult(CheckCollisionPointTriangle(point, p1, p2, p3));
	});
	raylibModule.SetValue("CheckCollisionPointTriangle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("rec1");
	i.AddParam("rec2");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec1 = ValueToRectangle(context.GetArg(0));
		Rectangle rec2 = ValueToRectangle(context.GetArg(1));
		Rectangle result = GetCollisionRec(rec1, rec2);
		return IntrinsicResult(RectangleToValue(result));
	});
	raylibModule.SetValue("GetCollisionRec", i.GetFunc());

	// Additional collision detection

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius");
	i.AddParam("p1");
	i.AddParam("p2");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		Vector2 p1 = ValueToVector2(context.GetArg(2));
		Vector2 p2 = ValueToVector2(context.GetArg(3));
		return IntrinsicResult(CheckCollisionCircleLine(center, radius, p1, p2));
	});
	raylibModule.SetValue("CheckCollisionCircleLine", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos1");
	i.AddParam("endPos1");
	i.AddParam("startPos2");
	i.AddParam("endPos2");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 startPos1 = ValueToVector2(context.GetArg(0));
		Vector2 endPos1 = ValueToVector2(context.GetArg(1));
		Vector2 startPos2 = ValueToVector2(context.GetArg(2));
		Vector2 endPos2 = ValueToVector2(context.GetArg(3));
		Vector2 collisionPoint;
		bool result = CheckCollisionLines(startPos1, endPos1, startPos2, endPos2, &collisionPoint);
		if (!result) return IntrinsicResult::Null;
		return IntrinsicResult(Vector2ToValue(collisionPoint));
	});
	raylibModule.SetValue("CheckCollisionLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("point");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("threshold");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context.GetArg(0));
		Vector2 p1 = ValueToVector2(context.GetArg(1));
		Vector2 p2 = ValueToVector2(context.GetArg(2));
		int threshold = context.GetArg(3).IntValue();
		return IntrinsicResult(CheckCollisionPointLine(point, p1, p2, threshold));
	});
	raylibModule.SetValue("CheckCollisionPointLine", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("point");
	i.AddParam("points");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context.GetArg(0));
		ValueList pointsList = context.GetArg(1).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Zero;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		bool result = CheckCollisionPointPoly(point, points, pointCount);
		delete[] points;
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("CheckCollisionPointPoly", i.GetFunc());

	// Additional circle drawing

	i = Intrinsic::Create("");
	i.AddParam("centerX", Value::zero);
	i.AddParam("centerY", Value::zero);
	i.AddParam("radius", Value(10.0));
	i.AddParam("colorInner", ColorToValue(WHITE));
	i.AddParam("colorOuter", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		float radius = context.GetArg(2).FloatValue();
		Color colorInner = ValueToColor(context.GetArg(3));
		Color colorOuter = ValueToColor(context.GetArg(4));
		float centerX = context.GetArg(0).FloatValue();
		float centerY = context.GetArg(1).FloatValue();
		DrawCircleGradient(CLITERAL(Vector2){centerX, centerY}, radius, colorInner, colorOuter);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleGradient", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius", Value(10.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawCircleLinesV(center, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleLinesV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius", Value(10.0));
	i.AddParam("startAngle", Value::zero);
	i.AddParam("endAngle", Value(90.0));
	i.AddParam("segments", Value(36));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		float startAngle = context.GetArg(2).FloatValue();
		float endAngle = context.GetArg(3).FloatValue();
		int segments = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCircleSector(center, radius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleSector", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius", Value(10.0));
	i.AddParam("startAngle", Value::zero);
	i.AddParam("endAngle", Value(90.0));
	i.AddParam("segments", Value(36));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		float startAngle = context.GetArg(2).FloatValue();
		float endAngle = context.GetArg(3).FloatValue();
		int segments = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawCircleSectorLines(center, radius, startAngle, endAngle, segments, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleSectorLines", i.GetFunc());

	// Additional ellipse drawing

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radiusH", Value(10.0));
	i.AddParam("radiusV", Value(5.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radiusH = context.GetArg(1).FloatValue();
		float radiusV = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawEllipseV(center, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawEllipseV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radiusH", Value(10.0));
	i.AddParam("radiusV", Value(5.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radiusH = context.GetArg(1).FloatValue();
		float radiusV = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawEllipseLinesV(center, radiusH, radiusV, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawEllipseLinesV", i.GetFunc());

	// Additional line drawing

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context.GetArg(0));
		Vector2 endPos = ValueToVector2(context.GetArg(1));
		float thick = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawLineBezier(startPos, endPos, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLineBezier", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("dashSize");
	i.AddParam("spaceSize");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context.GetArg(0));
		Vector2 endPos = ValueToVector2(context.GetArg(1));
		int dashSize = context.GetArg(2).IntValue();
		int spaceSize = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawLineDashed(startPos, endPos, dashSize, spaceSize, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLineDashed", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		Color color = ValueToColor(context.GetArg(1));
		DrawLineStrip(points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawLineStrip", i.GetFunc());

	// Additional rectangle drawing

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("roundness", Value(0.0));
	i.AddParam("segments", Value::zero);
	i.AddParam("lineThick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		float roundness = context.GetArg(1).FloatValue();
		int segments = context.GetArg(2).IntValue();
		float lineThick = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawRectangleRoundedLinesEx(rec, roundness, segments, lineThick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleRoundedLinesEx", i.GetFunc());

	// Spline drawing

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawSplineLinear(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineLinear", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 4) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawSplineBasis(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineBasis", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawSplineCatmullRom(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineCatmullRom", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawSplineBezierQuadratic(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineBezierQuadratic", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 4) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		float thick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawSplineBezierCubic(points, pointCount, thick, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineBezierCubic", i.GetFunc());

	// Spline segment drawing

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		float thick = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawSplineSegmentLinear(p1, p2, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineSegmentLinear", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("p4");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		Vector2 p4 = ValueToVector2(context.GetArg(3));
		float thick = context.GetArg(4).FloatValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawSplineSegmentBasis(p1, p2, p3, p4, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineSegmentBasis", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("p4");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		Vector2 p4 = ValueToVector2(context.GetArg(3));
		float thick = context.GetArg(4).FloatValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawSplineSegmentCatmullRom(p1, p2, p3, p4, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineSegmentCatmullRom", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		float thick = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawSplineSegmentBezierQuadratic(p1, p2, p3, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineSegmentBezierQuadratic", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("p4");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		Vector2 p4 = ValueToVector2(context.GetArg(3));
		float thick = context.GetArg(4).FloatValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawSplineSegmentBezierCubic(p1, p2, p3, p4, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawSplineSegmentBezierCubic", i.GetFunc());

	// Spline point evaluation functions

	i = Intrinsic::Create("");
	i.AddParam("startPos");
	i.AddParam("endPos");
	i.AddParam("t");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 startPos = ValueToVector2(context.GetArg(0));
		Vector2 endPos = ValueToVector2(context.GetArg(1));
		float t = context.GetArg(2).FloatValue();
		Vector2 result = GetSplinePointLinear(startPos, endPos, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointLinear", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("p4");
	i.AddParam("t");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		Vector2 p4 = ValueToVector2(context.GetArg(3));
		float t = context.GetArg(4).FloatValue();
		Vector2 result = GetSplinePointBasis(p1, p2, p3, p4, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointBasis", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("p2");
	i.AddParam("p3");
	i.AddParam("p4");
	i.AddParam("t");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 p2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		Vector2 p4 = ValueToVector2(context.GetArg(3));
		float t = context.GetArg(4).FloatValue();
		Vector2 result = GetSplinePointCatmullRom(p1, p2, p3, p4, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointCatmullRom", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("c2");
	i.AddParam("p3");
	i.AddParam("t");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 c2 = ValueToVector2(context.GetArg(1));
		Vector2 p3 = ValueToVector2(context.GetArg(2));
		float t = context.GetArg(3).FloatValue();
		Vector2 result = GetSplinePointBezierQuad(p1, c2, p3, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointBezierQuad", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("p1");
	i.AddParam("c2");
	i.AddParam("c3");
	i.AddParam("p4");
	i.AddParam("t");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 p1 = ValueToVector2(context.GetArg(0));
		Vector2 c2 = ValueToVector2(context.GetArg(1));
		Vector2 c3 = ValueToVector2(context.GetArg(2));
		Vector2 p4 = ValueToVector2(context.GetArg(3));
		float t = context.GetArg(4).FloatValue();
		Vector2 result = GetSplinePointBezierCubic(p1, c2, c3, p4, t);
		ValueDict resultMap;
		resultMap.SetValue(String("x"), Value(result.x));
		resultMap.SetValue(String("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointBezierCubic", i.GetFunc());

	// Additional triangle drawing

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		Color color = ValueToColor(context.GetArg(1));
		DrawTriangleFan(points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangleFan", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("points");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList pointsList = context.GetArg(0).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;

		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}

		Color color = ValueToColor(context.GetArg(1));
		DrawTriangleStrip(points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangleStrip", i.GetFunc());

	// Texture setup

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("source");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture texture = ValueToTexture(context.GetArg(0));
		Rectangle source = ValueToRectangle(context.GetArg(1));
		SetShapesTexture(texture, source);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShapesTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = GetShapesTexture();
		return IntrinsicResult(TextureToValue(texture));
	});
	raylibModule.SetValue("GetShapesTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rect = GetShapesTextureRectangle();
		return IntrinsicResult(RectangleToValue(rect));
	});
	raylibModule.SetValue("GetShapesTextureRectangle", i.GetFunc());
}
