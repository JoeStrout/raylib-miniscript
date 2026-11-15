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
	raylibModule.SetValue("DrawPixel", WithNote(i,
		"Draw a pixel"));

	i = Intrinsic::Create("");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 position = ValueToVector2(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawPixelV(position, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPixelV", WithNote(i,
		"Draw a pixel (Vector version)"));

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
	raylibModule.SetValue("DrawLine", WithNote(i,
		"Draw a line (using gl lines)"));

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
	raylibModule.SetValue("DrawLineV", WithNote(i,
		"Draw a line (using gl lines)"));

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
	raylibModule.SetValue("DrawLineEx", WithNote(i,
		"Draw a line defining thickness"));

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
	raylibModule.SetValue("DrawCircle", WithNote(i,
		"Draw a color-filled circle"));

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
	raylibModule.SetValue("DrawCircleV", WithNote(i,
		"Draw a color-filled circle (Vector version) NOTE: On OpenGL 3.3 and ES2 using QUADS to "
		"avoid drawing order issues"));

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
	raylibModule.SetValue("DrawCircleLines", WithNote(i,
		"Draw circle outline"));

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
	raylibModule.SetValue("DrawEllipse", WithNote(i,
		"Draw ellipse"));

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
	raylibModule.SetValue("DrawEllipseLines", WithNote(i,
		"Draw ellipse outline"));

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
	raylibModule.SetValue("DrawRing", WithNote(i,
		"Draw ring"));

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
	raylibModule.SetValue("DrawRingLines", WithNote(i,
		"Draw ring outline"));

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("innerRadius", Value(20));
	i.AddParam("outerRadius", Value(32));
	i.AddParam("startAngle", Value::zero);
	i.AddParam("endAngle", Value(360));
	i.AddParam("segments", Value(36));
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float innerRadius = context.GetArg(1).FloatValue();
		float outerRadius = context.GetArg(2).FloatValue();
		float startAngle = context.GetArg(3).FloatValue();
		float endAngle = context.GetArg(4).FloatValue();
		int segments = context.GetArg(5).IntValue();
		float thick = context.GetArg(6).FloatValue();
		Color color = ValueToColor(context.GetArg(7));
		DrawRingLinesEx(center, innerRadius, outerRadius, startAngle, endAngle, segments, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRingLinesEx", WithNote(i,
		"Draw ring outline with line thickness"));

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
	raylibModule.SetValue("DrawRectangle", WithNote(i,
		"Draw a color-filled rectangle"));

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
	raylibModule.SetValue("DrawRectangleV", WithNote(i,
		"Draw a color-filled rectangle (Vector version) NOTE: On OpenGL 3.3 and ES2 using QUADS to "
		"avoid drawing order issues"));

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		Color color = ValueToColor(context.GetArg(1));
		DrawRectangleRec(rec, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleRec", WithNote(i,
		"Draw a color-filled rectangle"));

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
	raylibModule.SetValue("DrawRectanglePro", WithNote(i,
		"Draw a color-filled rectangle with pro parameters"));

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
	raylibModule.SetValue("DrawRectangleLines", WithNote(i,
		"Draw rectangle outline WARNING: All Draw*Lines() functions use RL_LINES for drawing, it "
		"implies flushing the current batch and changing draw mode to RL_LINES but it solves "
		"another issue: https://github.com/raysan5/raylib/issues/3884"));

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("thick", Value(1));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		float thick = context.GetArg(1).FloatValue();
		Color color = ValueToColor(context.GetArg(2));
		DrawRectangleLinesEx(rec, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleLinesEx", WithNote(i,
		"Draw rectangle outline with line thickness"));

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
	raylibModule.SetValue("DrawRectangleRounded", WithNote(i,
		"Draw rectangle with rounded edges"));

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
	raylibModule.SetValue("DrawRectangleRoundedLines", WithNote(i,
		"Draw rectangle with rounded edges"));

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
	raylibModule.SetValue("DrawRectangleGradientV", WithNote(i,
		"Draw a vertical-gradient-filled rectangle"));

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
	raylibModule.SetValue("DrawRectangleGradientH", WithNote(i,
		"Draw a horizontal-gradient-filled rectangle"));

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
	raylibModule.SetValue("DrawRectangleGradientEx", WithNote(i,
		"Draw a gradient-filled rectangle with custom vertex colors, counter-clockwise color order"));

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
	raylibModule.SetValue("DrawTriangle", WithNote(i,
		"Draw a triangle NOTE: Vertex must be provided in counter-clockwise order"));

	i = Intrinsic::Create("");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("c1");
	i.AddParam("c2");
	i.AddParam("c3");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		Vector2 v3 = ValueToVector2(context.GetArg(2));
		Color c1 = ValueToColor(context.GetArg(3));
		Color c2 = ValueToColor(context.GetArg(4));
		Color c3 = ValueToColor(context.GetArg(5));
		DrawTriangleGradient(v1, v2, v3, c1, c2, c3);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangleGradient", WithNote(i,
		"Draw triangle with interpolated colors (vertex in counter-clockwise order!)"));

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
	raylibModule.SetValue("DrawTriangleLines", WithNote(i,
		"Draw a triangle using lines NOTE: Vertex must be provided in counter-clockwise order"));

	i = Intrinsic::Create("");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 v1 = ValueToVector2(context.GetArg(0));
		Vector2 v2 = ValueToVector2(context.GetArg(1));
		Vector2 v3 = ValueToVector2(context.GetArg(2));
		float thick = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawTriangleLinesEx(v1, v2, v3, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTriangleLinesEx", WithNote(i,
		"Draw a triangle using lines with thickness NOTE: Vertex must be provided in "
		"counter-clockwise order"));

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
	raylibModule.SetValue("DrawPoly", WithNote(i,
		"Draw a polygon of n sides"));

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
	raylibModule.SetValue("DrawPolyLines", WithNote(i,
		"Draw a polygon outline of n sides"));

	i = Intrinsic::Create("");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("sides", Value(6));
	i.AddParam("radius", Value(32));
	i.AddParam("rotation", Value::zero);
	i.AddParam("thick", Value(1));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		int sides = context.GetArg(1).IntValue();
		float radius = context.GetArg(2).FloatValue();
		float rotation = context.GetArg(3).FloatValue();
		float thick = context.GetArg(4).FloatValue();
		Color color = ValueToColor(context.GetArg(5));
		DrawPolyLinesEx(center, sides, radius, rotation, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawPolyLinesEx", WithNote(i,
		"Draw the outline of a regular polygon of n sides, with line thickness"));

	// Collision detection

	i = Intrinsic::Create("");
	i.AddParam("rec1");
	i.AddParam("rec2");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec1 = ValueToRectangle(context.GetArg(0));
		Rectangle rec2 = ValueToRectangle(context.GetArg(1));
		return IntrinsicResult(CheckCollisionRecs(rec1, rec2));
	});
	raylibModule.SetValue("CheckCollisionRecs", WithNote(i,
		"Check collision between two rectangles"));

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
	raylibModule.SetValue("CheckCollisionCircles", WithNote(i,
		"Check collision between two circles"));

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
	raylibModule.SetValue("CheckCollisionCircleRec", WithNote(i,
		"Check collision between circle and rectangle NOTE: Reviewed version to take into account "
		"corner limit case"));

	i = Intrinsic::Create("");
	i.AddParam("point");
	i.AddParam("rec");
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 point = ValueToVector2(context.GetArg(0));
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		return IntrinsicResult(CheckCollisionPointRec(point, rec));
	});
	raylibModule.SetValue("CheckCollisionPointRec", WithNote(i,
		"Check if point is inside rectangle"));

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
	raylibModule.SetValue("CheckCollisionPointCircle", WithNote(i,
		"Check if point is inside circle"));

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
	raylibModule.SetValue("CheckCollisionPointTriangle", WithNote(i,
		"Check if point is inside a triangle defined by three points (p1, p2, p3)"));

	i = Intrinsic::Create("");
	i.AddParam("rec1");
	i.AddParam("rec2");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec1 = ValueToRectangle(context.GetArg(0));
		Rectangle rec2 = ValueToRectangle(context.GetArg(1));
		Rectangle result = GetCollisionRec(rec1, rec2);
		return IntrinsicResult(RectangleToValue(result));
	});
	raylibModule.SetValue("GetCollisionRec", WithNote(i,
		"Get collision rectangle for two rectangles collision"));

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
	raylibModule.SetValue("CheckCollisionCircleLine", WithNote(i,
		"Check if circle collides with a line created between two points [p1] and [p2]"));

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
	raylibModule.SetValue("CheckCollisionLines", WithNote(i,
		"Check the collision between two lines defined by two points each, returns collision point "
		"by reference REF: "
		"https://en.wikipedia.org/wiki/Line–line_intersection#Given_two_points_on_each_line_segment"));

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
	raylibModule.SetValue("CheckCollisionPointLine", WithNote(i,
		"Check if point belongs to line created between two points [p1] and [p2] with defined "
		"margin in pixels [threshold]"));

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
	raylibModule.SetValue("CheckCollisionPointPoly", WithNote(i,
		"Check if point is within a polygon described by array of vertices NOTE: Based on "
		"http://jeffreythompson.org/collision-detection/poly-point.php"));

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
	raylibModule.SetValue("DrawCircleGradient", WithNote(i,
		"Draw a gradient-filled circle"));

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
	raylibModule.SetValue("DrawCircleLinesV", WithNote(i,
		"Draw circle outline (Vector version)"));

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius", Value(10.0));
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		float thick = context.GetArg(2).FloatValue();
		Color color = ValueToColor(context.GetArg(3));
		DrawCircleLinesEx(center, radius, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleLinesEx", WithNote(i,
		"Draw circle outline with line thickness"));

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
	raylibModule.SetValue("DrawCircleSector", WithNote(i,
		"Draw a piece of a circle"));

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
	raylibModule.SetValue("DrawCircleSectorLines", WithNote(i,
		"Draw a piece of a circle outlines"));

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radius", Value(10.0));
	i.AddParam("startAngle", Value::zero);
	i.AddParam("endAngle", Value(90.0));
	i.AddParam("segments", Value(36));
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radius = context.GetArg(1).FloatValue();
		float startAngle = context.GetArg(2).FloatValue();
		float endAngle = context.GetArg(3).FloatValue();
		int segments = context.GetArg(4).IntValue();
		float thick = context.GetArg(5).FloatValue();
		Color color = ValueToColor(context.GetArg(6));
		DrawCircleSectorLinesEx(center, radius, startAngle, endAngle, segments, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawCircleSectorLinesEx", WithNote(i,
		"Draw a piece of a circle outlines with thickness"));

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
	raylibModule.SetValue("DrawEllipseV", WithNote(i,
		"Draw ellipse (Vector version)"));

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
	raylibModule.SetValue("DrawEllipseLinesV", WithNote(i,
		"Draw ellipse outline"));

	i = Intrinsic::Create("");
	i.AddParam("center");
	i.AddParam("radiusH", Value(10.0));
	i.AddParam("radiusV", Value(5.0));
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Vector2 center = ValueToVector2(context.GetArg(0));
		float radiusH = context.GetArg(1).FloatValue();
		float radiusV = context.GetArg(2).FloatValue();
		float thick = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawEllipseLinesEx(center, radiusH, radiusV, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawEllipseLinesEx", WithNote(i,
		"Draw ellipse outline with thickness"));

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
	raylibModule.SetValue("DrawLineBezier", WithNote(i,
		"Draw line using cubic-bezier spline, in-out interpolation, no control points"));

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
	raylibModule.SetValue("DrawLineDashed", WithNote(i,
		"Draw a dashed line"));

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
	raylibModule.SetValue("DrawLineStrip", WithNote(i,
		"Draw lines sequence (using gl lines)"));

	// Additional rectangle drawing

	i = Intrinsic::Create("");
	i.AddParam("rec");
	i.AddParam("roundness", Value(0.0));
	i.AddParam("segments", Value::zero);
	i.AddParam("thick", Value(1.0));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rec = ValueToRectangle(context.GetArg(0));
		float roundness = context.GetArg(1).FloatValue();
		int segments = context.GetArg(2).IntValue();
		float thick = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawRectangleRoundedLinesEx(rec, roundness, segments, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawRectangleRoundedLinesEx", WithNote(i,
		"Draw rectangle with rounded edges outline with line thickness"));

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
	raylibModule.SetValue("DrawSplineLinear", WithNote(i,
		"Draw spline: linear, minimum 2 points"));

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
	raylibModule.SetValue("DrawSplineBasis", WithNote(i,
		"Draw spline: B-Spline, minimum 4 points"));

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
	raylibModule.SetValue("DrawSplineCatmullRom", WithNote(i,
		"Draw spline: Catmull-Rom, minimum 4 points"));

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
	raylibModule.SetValue("DrawSplineBezierQuadratic", WithNote(i,
		"Draw spline: Quadratic Bezier, minimum 3 points (1 control point): [p1, c2, p3, c4...]"));

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
	raylibModule.SetValue("DrawSplineBezierCubic", WithNote(i,
		"Draw spline: Cubic Bezier, minimum 4 points (2 control points): [p1, c2, c3, p4, c5, "
		"c6...]"));

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
	raylibModule.SetValue("DrawSplineSegmentLinear", WithNote(i,
		"Draw spline segment: Linear, 2 points"));

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
	raylibModule.SetValue("DrawSplineSegmentBasis", WithNote(i,
		"Draw spline segment: B-Spline, 4 points"));

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
	raylibModule.SetValue("DrawSplineSegmentCatmullRom", WithNote(i,
		"Draw spline segment: Catmull-Rom, 4 points"));

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
	raylibModule.SetValue("DrawSplineSegmentBezierQuadratic", WithNote(i,
		"Draw spline segment: Quadratic Bezier, 2 points, 1 control point"));

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
	raylibModule.SetValue("DrawSplineSegmentBezierCubic", WithNote(i,
		"Draw spline segment: Cubic Bezier, 2 points, 2 control points"));

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
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointLinear", WithNote(i,
		"Get spline point for a given t [0.0f .. 1.0f], Linear"));

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
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointBasis", WithNote(i,
		"Get spline point for a given t [0.0f .. 1.0f], B-Spline"));

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
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointCatmullRom", WithNote(i,
		"Get spline point for a given t [0.0f .. 1.0f], Catmull-Rom"));

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
		Vector2 result = GetSplinePointBezierQuadratic(p1, c2, p3, t);
		ValueDict resultMap;
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointBezierQuadratic", WithNote(i,
		"Get spline point for a given t [0.0f .. 1.0f], Quadratic Bezier"));

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
		resultMap.SetValue(CACHED_KEY("x"), Value(result.x));
		resultMap.SetValue(CACHED_KEY("y"), Value(result.y));
		return IntrinsicResult(DynamicMap(resultMap));
	});
	raylibModule.SetValue("GetSplinePointBezierCubic", WithNote(i,
		"Get spline point for a given t [0.0f .. 1.0f], Cubic Bezier"));

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
	raylibModule.SetValue("DrawTriangleFan", WithNote(i,
		"Draw a triangle fan defined by points NOTE: First vertex provided is the center, shared "
		"by all triangles By default, following vertex should be provided in counter-clockwise "
		"order"));

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
	raylibModule.SetValue("DrawTriangleStrip", WithNote(i,
		"Draw a triangle strip defined by points NOTE: Every new vertex connects with previous two"));

	// Texture setup

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("rec");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture texture = ValueToTexture(context.GetArg(0));
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		SetShapesTexture(texture, rec);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetShapesTexture", WithNote(i,
		"Set texture and rectangle to be used on shapes drawing NOTE: It can be useful when using "
		"basic shapes and one single font, defining a font char white rectangle would allow "
		"drawing everything in a single draw call"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = GetShapesTexture();
		return IntrinsicResult(TextureToValue(texture));
	});
	raylibModule.SetValue("GetShapesTexture", WithNote(i,
		"Get texture that is used for shapes drawing"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Rectangle rect = GetShapesTextureRectangle();
		return IntrinsicResult(RectangleToValue(rect));
	});
	raylibModule.SetValue("GetShapesTextureRectangle", WithNote(i,
		"Get texture source rectangle that is used for shapes drawing"));
}
