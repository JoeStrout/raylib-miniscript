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

		Matrix mat = GetCameraMatrix2D(camera);
		ValueDict result;
		result.SetValue(String("m0"), Value(mat.m0));
		result.SetValue(String("m1"), Value(mat.m1));
		result.SetValue(String("m2"), Value(mat.m2));
		result.SetValue(String("m3"), Value(mat.m3));
		result.SetValue(String("m4"), Value(mat.m4));
		result.SetValue(String("m5"), Value(mat.m5));
		result.SetValue(String("m6"), Value(mat.m6));
		result.SetValue(String("m7"), Value(mat.m7));
		result.SetValue(String("m8"), Value(mat.m8));
		result.SetValue(String("m9"), Value(mat.m9));
		result.SetValue(String("m10"), Value(mat.m10));
		result.SetValue(String("m11"), Value(mat.m11));
		result.SetValue(String("m12"), Value(mat.m12));
		result.SetValue(String("m13"), Value(mat.m13));
		result.SetValue(String("m14"), Value(mat.m14));
		result.SetValue(String("m15"), Value(mat.m15));
		return IntrinsicResult(result);
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
