//
//  RaylibConstants.cpp
//  MSRLWeb
//
//  Raylib constants
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "MiniscriptTypes.h"

using namespace MiniScript;

void AddConstants(ValueDict raylibModule) {
	// Add color constants (all colors from raylib.h)
	raylibModule.SetValue("LIGHTGRAY", ColorToValue(LIGHTGRAY));
	raylibModule.SetValue("GRAY", ColorToValue(GRAY));
	raylibModule.SetValue("DARKGRAY", ColorToValue(DARKGRAY));
	raylibModule.SetValue("YELLOW", ColorToValue(YELLOW));
	raylibModule.SetValue("GOLD", ColorToValue(GOLD));
	raylibModule.SetValue("ORANGE", ColorToValue(ORANGE));
	raylibModule.SetValue("PINK", ColorToValue(PINK));
	raylibModule.SetValue("RED", ColorToValue(RED));
	raylibModule.SetValue("MAROON", ColorToValue(MAROON));
	raylibModule.SetValue("GREEN", ColorToValue(GREEN));
	raylibModule.SetValue("LIME", ColorToValue(LIME));
	raylibModule.SetValue("DARKGREEN", ColorToValue(DARKGREEN));
	raylibModule.SetValue("SKYBLUE", ColorToValue(SKYBLUE));
	raylibModule.SetValue("BLUE", ColorToValue(BLUE));
	raylibModule.SetValue("DARKBLUE", ColorToValue(DARKBLUE));
	raylibModule.SetValue("PURPLE", ColorToValue(PURPLE));
	raylibModule.SetValue("VIOLET", ColorToValue(VIOLET));
	raylibModule.SetValue("DARKPURPLE", ColorToValue(DARKPURPLE));
	raylibModule.SetValue("BEIGE", ColorToValue(BEIGE));
	raylibModule.SetValue("BROWN", ColorToValue(BROWN));
	raylibModule.SetValue("DARKBROWN", ColorToValue(DARKBROWN));
	raylibModule.SetValue("WHITE", ColorToValue(WHITE));
	raylibModule.SetValue("BLACK", ColorToValue(BLACK));
	raylibModule.SetValue("BLANK", ColorToValue(BLANK));
	raylibModule.SetValue("MAGENTA", ColorToValue(MAGENTA));
	raylibModule.SetValue("RAYWHITE", ColorToValue(RAYWHITE));

	// Add keyboard key constants
	raylibModule.SetValue("KEY_NULL", Value(KEY_NULL));

	// Alphanumeric keys
	raylibModule.SetValue("KEY_APOSTROPHE", Value(KEY_APOSTROPHE));
	raylibModule.SetValue("KEY_COMMA", Value(KEY_COMMA));
	raylibModule.SetValue("KEY_MINUS", Value(KEY_MINUS));
	raylibModule.SetValue("KEY_PERIOD", Value(KEY_PERIOD));
	raylibModule.SetValue("KEY_SLASH", Value(KEY_SLASH));
	raylibModule.SetValue("KEY_ZERO", Value(KEY_ZERO));
	raylibModule.SetValue("KEY_ONE", Value(KEY_ONE));
	raylibModule.SetValue("KEY_TWO", Value(KEY_TWO));
	raylibModule.SetValue("KEY_THREE", Value(KEY_THREE));
	raylibModule.SetValue("KEY_FOUR", Value(KEY_FOUR));
	raylibModule.SetValue("KEY_FIVE", Value(KEY_FIVE));
	raylibModule.SetValue("KEY_SIX", Value(KEY_SIX));
	raylibModule.SetValue("KEY_SEVEN", Value(KEY_SEVEN));
	raylibModule.SetValue("KEY_EIGHT", Value(KEY_EIGHT));
	raylibModule.SetValue("KEY_NINE", Value(KEY_NINE));
	raylibModule.SetValue("KEY_SEMICOLON", Value(KEY_SEMICOLON));
	raylibModule.SetValue("KEY_EQUAL", Value(KEY_EQUAL));
	raylibModule.SetValue("KEY_A", Value(KEY_A));
	raylibModule.SetValue("KEY_B", Value(KEY_B));
	raylibModule.SetValue("KEY_C", Value(KEY_C));
	raylibModule.SetValue("KEY_D", Value(KEY_D));
	raylibModule.SetValue("KEY_E", Value(KEY_E));
	raylibModule.SetValue("KEY_F", Value(KEY_F));
	raylibModule.SetValue("KEY_G", Value(KEY_G));
	raylibModule.SetValue("KEY_H", Value(KEY_H));
	raylibModule.SetValue("KEY_I", Value(KEY_I));
	raylibModule.SetValue("KEY_J", Value(KEY_J));
	raylibModule.SetValue("KEY_K", Value(KEY_K));
	raylibModule.SetValue("KEY_L", Value(KEY_L));
	raylibModule.SetValue("KEY_M", Value(KEY_M));
	raylibModule.SetValue("KEY_N", Value(KEY_N));
	raylibModule.SetValue("KEY_O", Value(KEY_O));
	raylibModule.SetValue("KEY_P", Value(KEY_P));
	raylibModule.SetValue("KEY_Q", Value(KEY_Q));
	raylibModule.SetValue("KEY_R", Value(KEY_R));
	raylibModule.SetValue("KEY_S", Value(KEY_S));
	raylibModule.SetValue("KEY_T", Value(KEY_T));
	raylibModule.SetValue("KEY_U", Value(KEY_U));
	raylibModule.SetValue("KEY_V", Value(KEY_V));
	raylibModule.SetValue("KEY_W", Value(KEY_W));
	raylibModule.SetValue("KEY_X", Value(KEY_X));
	raylibModule.SetValue("KEY_Y", Value(KEY_Y));
	raylibModule.SetValue("KEY_Z", Value(KEY_Z));
	raylibModule.SetValue("KEY_LEFT_BRACKET", Value(KEY_LEFT_BRACKET));
	raylibModule.SetValue("KEY_BACKSLASH", Value(KEY_BACKSLASH));
	raylibModule.SetValue("KEY_RIGHT_BRACKET", Value(KEY_RIGHT_BRACKET));
	raylibModule.SetValue("KEY_GRAVE", Value(KEY_GRAVE));

	// Function keys
	raylibModule.SetValue("KEY_SPACE", Value(KEY_SPACE));
	raylibModule.SetValue("KEY_ESCAPE", Value(KEY_ESCAPE));
	raylibModule.SetValue("KEY_ENTER", Value(KEY_ENTER));
	raylibModule.SetValue("KEY_TAB", Value(KEY_TAB));
	raylibModule.SetValue("KEY_BACKSPACE", Value(KEY_BACKSPACE));
	raylibModule.SetValue("KEY_INSERT", Value(KEY_INSERT));
	raylibModule.SetValue("KEY_DELETE", Value(KEY_DELETE));
	raylibModule.SetValue("KEY_RIGHT", Value(KEY_RIGHT));
	raylibModule.SetValue("KEY_LEFT", Value(KEY_LEFT));
	raylibModule.SetValue("KEY_DOWN", Value(KEY_DOWN));
	raylibModule.SetValue("KEY_UP", Value(KEY_UP));
	raylibModule.SetValue("KEY_PAGE_UP", Value(KEY_PAGE_UP));
	raylibModule.SetValue("KEY_PAGE_DOWN", Value(KEY_PAGE_DOWN));
	raylibModule.SetValue("KEY_HOME", Value(KEY_HOME));
	raylibModule.SetValue("KEY_END", Value(KEY_END));
	raylibModule.SetValue("KEY_CAPS_LOCK", Value(KEY_CAPS_LOCK));
	raylibModule.SetValue("KEY_SCROLL_LOCK", Value(KEY_SCROLL_LOCK));
	raylibModule.SetValue("KEY_NUM_LOCK", Value(KEY_NUM_LOCK));
	raylibModule.SetValue("KEY_PRINT_SCREEN", Value(KEY_PRINT_SCREEN));
	raylibModule.SetValue("KEY_PAUSE", Value(KEY_PAUSE));
	raylibModule.SetValue("KEY_F1", Value(KEY_F1));
	raylibModule.SetValue("KEY_F2", Value(KEY_F2));
	raylibModule.SetValue("KEY_F3", Value(KEY_F3));
	raylibModule.SetValue("KEY_F4", Value(KEY_F4));
	raylibModule.SetValue("KEY_F5", Value(KEY_F5));
	raylibModule.SetValue("KEY_F6", Value(KEY_F6));
	raylibModule.SetValue("KEY_F7", Value(KEY_F7));
	raylibModule.SetValue("KEY_F8", Value(KEY_F8));
	raylibModule.SetValue("KEY_F9", Value(KEY_F9));
	raylibModule.SetValue("KEY_F10", Value(KEY_F10));
	raylibModule.SetValue("KEY_F11", Value(KEY_F11));
	raylibModule.SetValue("KEY_F12", Value(KEY_F12));

	// Modifier keys
	raylibModule.SetValue("KEY_LEFT_SHIFT", Value(KEY_LEFT_SHIFT));
	raylibModule.SetValue("KEY_LEFT_CONTROL", Value(KEY_LEFT_CONTROL));
	raylibModule.SetValue("KEY_LEFT_ALT", Value(KEY_LEFT_ALT));
	raylibModule.SetValue("KEY_LEFT_SUPER", Value(KEY_LEFT_SUPER));
	raylibModule.SetValue("KEY_RIGHT_SHIFT", Value(KEY_RIGHT_SHIFT));
	raylibModule.SetValue("KEY_RIGHT_CONTROL", Value(KEY_RIGHT_CONTROL));
	raylibModule.SetValue("KEY_RIGHT_ALT", Value(KEY_RIGHT_ALT));
	raylibModule.SetValue("KEY_RIGHT_SUPER", Value(KEY_RIGHT_SUPER));
	raylibModule.SetValue("KEY_KB_MENU", Value(KEY_KB_MENU));

	// Keypad keys
	raylibModule.SetValue("KEY_KP_0", Value(KEY_KP_0));
	raylibModule.SetValue("KEY_KP_1", Value(KEY_KP_1));
	raylibModule.SetValue("KEY_KP_2", Value(KEY_KP_2));
	raylibModule.SetValue("KEY_KP_3", Value(KEY_KP_3));
	raylibModule.SetValue("KEY_KP_4", Value(KEY_KP_4));
	raylibModule.SetValue("KEY_KP_5", Value(KEY_KP_5));
	raylibModule.SetValue("KEY_KP_6", Value(KEY_KP_6));
	raylibModule.SetValue("KEY_KP_7", Value(KEY_KP_7));
	raylibModule.SetValue("KEY_KP_8", Value(KEY_KP_8));
	raylibModule.SetValue("KEY_KP_9", Value(KEY_KP_9));
	raylibModule.SetValue("KEY_KP_DECIMAL", Value(KEY_KP_DECIMAL));
	raylibModule.SetValue("KEY_KP_DIVIDE", Value(KEY_KP_DIVIDE));
	raylibModule.SetValue("KEY_KP_MULTIPLY", Value(KEY_KP_MULTIPLY));
	raylibModule.SetValue("KEY_KP_SUBTRACT", Value(KEY_KP_SUBTRACT));
	raylibModule.SetValue("KEY_KP_ADD", Value(KEY_KP_ADD));
	raylibModule.SetValue("KEY_KP_ENTER", Value(KEY_KP_ENTER));
	raylibModule.SetValue("KEY_KP_EQUAL", Value(KEY_KP_EQUAL));

	// Android keys
	raylibModule.SetValue("KEY_BACK", Value(KEY_BACK));
	raylibModule.SetValue("KEY_MENU", Value(KEY_MENU));
	raylibModule.SetValue("KEY_VOLUME_UP", Value(KEY_VOLUME_UP));
	raylibModule.SetValue("KEY_VOLUME_DOWN", Value(KEY_VOLUME_DOWN));

	// Add gamepad button constants
	raylibModule.SetValue("GAMEPAD_BUTTON_UNKNOWN", Value(GAMEPAD_BUTTON_UNKNOWN));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_FACE_UP", Value(GAMEPAD_BUTTON_LEFT_FACE_UP));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_FACE_RIGHT", Value(GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_FACE_DOWN", Value(GAMEPAD_BUTTON_LEFT_FACE_DOWN));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_FACE_LEFT", Value(GAMEPAD_BUTTON_LEFT_FACE_LEFT));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_FACE_UP", Value(GAMEPAD_BUTTON_RIGHT_FACE_UP));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_FACE_RIGHT", Value(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_FACE_DOWN", Value(GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_FACE_LEFT", Value(GAMEPAD_BUTTON_RIGHT_FACE_LEFT));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_TRIGGER_1", Value(GAMEPAD_BUTTON_LEFT_TRIGGER_1));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_TRIGGER_2", Value(GAMEPAD_BUTTON_LEFT_TRIGGER_2));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_TRIGGER_1", Value(GAMEPAD_BUTTON_RIGHT_TRIGGER_1));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_TRIGGER_2", Value(GAMEPAD_BUTTON_RIGHT_TRIGGER_2));
	raylibModule.SetValue("GAMEPAD_BUTTON_MIDDLE_LEFT", Value(GAMEPAD_BUTTON_MIDDLE_LEFT));
	raylibModule.SetValue("GAMEPAD_BUTTON_MIDDLE", Value(GAMEPAD_BUTTON_MIDDLE));
	raylibModule.SetValue("GAMEPAD_BUTTON_MIDDLE_RIGHT", Value(GAMEPAD_BUTTON_MIDDLE_RIGHT));
	raylibModule.SetValue("GAMEPAD_BUTTON_LEFT_THUMB", Value(GAMEPAD_BUTTON_LEFT_THUMB));
	raylibModule.SetValue("GAMEPAD_BUTTON_RIGHT_THUMB", Value(GAMEPAD_BUTTON_RIGHT_THUMB));

	// Add gamepad axis constants
	raylibModule.SetValue("GAMEPAD_AXIS_LEFT_X", Value(GAMEPAD_AXIS_LEFT_X));
	raylibModule.SetValue("GAMEPAD_AXIS_LEFT_Y", Value(GAMEPAD_AXIS_LEFT_Y));
	raylibModule.SetValue("GAMEPAD_AXIS_RIGHT_X", Value(GAMEPAD_AXIS_RIGHT_X));
	raylibModule.SetValue("GAMEPAD_AXIS_RIGHT_Y", Value(GAMEPAD_AXIS_RIGHT_Y));
	raylibModule.SetValue("GAMEPAD_AXIS_LEFT_TRIGGER", Value(GAMEPAD_AXIS_LEFT_TRIGGER));
	raylibModule.SetValue("GAMEPAD_AXIS_RIGHT_TRIGGER", Value(GAMEPAD_AXIS_RIGHT_TRIGGER));

	// Add mouse button constants
	raylibModule.SetValue("MOUSE_BUTTON_LEFT", Value(MOUSE_BUTTON_LEFT));
	raylibModule.SetValue("MOUSE_BUTTON_RIGHT", Value(MOUSE_BUTTON_RIGHT));
	raylibModule.SetValue("MOUSE_BUTTON_MIDDLE", Value(MOUSE_BUTTON_MIDDLE));

	// Add mouse cursor constants
	raylibModule.SetValue("MOUSE_CURSOR_DEFAULT", Value(MOUSE_CURSOR_DEFAULT));
	raylibModule.SetValue("MOUSE_CURSOR_ARROW", Value(MOUSE_CURSOR_ARROW));
	raylibModule.SetValue("MOUSE_CURSOR_IBEAM", Value(MOUSE_CURSOR_IBEAM));
	raylibModule.SetValue("MOUSE_CURSOR_CROSSHAIR", Value(MOUSE_CURSOR_CROSSHAIR));
	raylibModule.SetValue("MOUSE_CURSOR_POINTING_HAND", Value(MOUSE_CURSOR_POINTING_HAND));
	raylibModule.SetValue("MOUSE_CURSOR_RESIZE_EW", Value(MOUSE_CURSOR_RESIZE_EW));
	raylibModule.SetValue("MOUSE_CURSOR_RESIZE_NS", Value(MOUSE_CURSOR_RESIZE_NS));
	raylibModule.SetValue("MOUSE_CURSOR_RESIZE_NWSE", Value(MOUSE_CURSOR_RESIZE_NWSE));
	raylibModule.SetValue("MOUSE_CURSOR_RESIZE_NESW", Value(MOUSE_CURSOR_RESIZE_NESW));
	raylibModule.SetValue("MOUSE_CURSOR_RESIZE_ALL", Value(MOUSE_CURSOR_RESIZE_ALL));
	raylibModule.SetValue("MOUSE_CURSOR_NOT_ALLOWED", Value(MOUSE_CURSOR_NOT_ALLOWED));

	// Add texture filter mode constants
	raylibModule.SetValue("TEXTURE_FILTER_POINT", Value(TEXTURE_FILTER_POINT));
	raylibModule.SetValue("TEXTURE_FILTER_BILINEAR", Value(TEXTURE_FILTER_BILINEAR));
	raylibModule.SetValue("TEXTURE_FILTER_TRILINEAR", Value(TEXTURE_FILTER_TRILINEAR));
	raylibModule.SetValue("TEXTURE_FILTER_ANISOTROPIC_4X", Value(TEXTURE_FILTER_ANISOTROPIC_4X));
	raylibModule.SetValue("TEXTURE_FILTER_ANISOTROPIC_8X", Value(TEXTURE_FILTER_ANISOTROPIC_8X));
	raylibModule.SetValue("TEXTURE_FILTER_ANISOTROPIC_16X", Value(TEXTURE_FILTER_ANISOTROPIC_16X));

	// Add texture wrap mode constants
	raylibModule.SetValue("TEXTURE_WRAP_REPEAT", Value(TEXTURE_WRAP_REPEAT));
	raylibModule.SetValue("TEXTURE_WRAP_CLAMP", Value(TEXTURE_WRAP_CLAMP));
	raylibModule.SetValue("TEXTURE_WRAP_MIRROR_REPEAT", Value(TEXTURE_WRAP_MIRROR_REPEAT));
	raylibModule.SetValue("TEXTURE_WRAP_MIRROR_CLAMP", Value(TEXTURE_WRAP_MIRROR_CLAMP));

	// Add trace log level constants
	raylibModule.SetValue("LOG_ALL", Value(LOG_ALL));
	raylibModule.SetValue("LOG_TRACE", Value(LOG_TRACE));
	raylibModule.SetValue("LOG_DEBUG", Value(LOG_DEBUG));
	raylibModule.SetValue("LOG_INFO", Value(LOG_INFO));
	raylibModule.SetValue("LOG_WARNING", Value(LOG_WARNING));
	raylibModule.SetValue("LOG_ERROR", Value(LOG_ERROR));
	raylibModule.SetValue("LOG_FATAL", Value(LOG_FATAL));
	raylibModule.SetValue("LOG_NONE", Value(LOG_NONE));
}
