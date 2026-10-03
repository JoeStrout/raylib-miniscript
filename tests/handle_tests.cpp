//
//  handle_tests.cpp
//  raylib-miniscript
//
//  Regression tests for the native `_handle` fields of the raylib types
//  (Image, Texture, Sound, ...).  Those used to hold a raw C++ pointer as a
//  plain number, so a script could write `img._handle = <any number>` and then
//  draw into or free arbitrary memory.  They now hold a GC handle
//  (Value::NewHandle) which script cannot forge, tagged with its type and
//  killed as a whole by Unload*.  See NativeBox in src/RaylibTypes.h.
//
//  Each test runs a script in a real interpreter with the real raylib
//  intrinsics and checks that the hostile thing it does is a harmless no-op.
//  Success is simply that this process is still alive to report it.
//
//  It uses Images, which need neither a window nor an audio device.
//
//  Build:  cmake --build build --target handle_tests
//  Run:    ./build/handle_tests
//

#include "miniscript.h"
#include "RaylibTypes.h"
#include "RaylibIntrinsics.h"
#include "InterpModule.h"

#include <stdio.h>
#include <string.h>

using namespace MiniScript;

static int failures = 0;
static int checks = 0;

static void ok(bool condition, const char* what) {
	checks++;
	if (condition) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s\n", what);
		failures++;
	}
}

static void eqNum(Value actual, double expected, const char* what) {
	checks++;
	if (actual.IsNumber() && actual.DoubleValue() == expected) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s (got %s, expected %g)\n", what, actual.ToString().c_str(), expected);
		failures++;
	}
}

static String g_out;
static String g_err;

static void CaptureOut(String s, Boolean lineBreak) {
	g_out = g_out + s + (lineBreak ? String("\n") : String(""));
}

static void CaptureErr(String s, Boolean lineBreak) {
	g_err = g_err + s + (lineBreak ? String("\n") : String(""));
}

static bool errContains(const char* needle) {
	return strstr(g_err.c_str(), needle) != nullptr;
}

static Value readGlobal(Interpreter p, const char* name) {
	Value g = p.GetGlobals().AsMap();
	if (!g.IsMap()) return Value::Null;
	Value out;
	if (!g.TryGet(Value(name), &out)) return Value::Null;
	return out;
}

// Run a script to completion, bounded by iterations so a hang fails the test.
static Interpreter run(const char* source) {
	g_out = String("");
	g_err = String("");
	Interpreter a = Interpreter::New();
	a.set_standardOutput(&CaptureOut);
	a.set_implicitOutput(&CaptureOut);
	a.set_errorOutput(&CaptureErr);
	a.Reset(String(source));
	a.Compile();
	for (int i = 0; i < 500 && !a.Done(); i++) a.RunUntilDone(0.1, true);
	return a;
}

static void testHandleIsUnforgeable() {
	printf("\n-- _handle is a GC handle, not a number --\n");
	int before = rcImage;
	Interpreter a = run(
		"img = raylib.GenImageColor(4, 4, \"#FF0000\")\n");
	ok(g_err.empty(), "the script runs clean");
	Value img = readGlobal(a, "img");
	ok(img.IsMap(), "GenImageColor returns a map");
	ok(img.GetDict().Lookup(String("_handle"), Value::Null).IsHandle(),
		"...whose _handle is a handle Value");
	eqNum(Value(rcImage - before), 1, "...and counts as one live image");
	run("img = raylib.GenImageColor(4, 4, \"#FF0000\")\nraylib.UnloadImage img\n");
	eqNum(Value(rcImage - before), 1, "(the unloaded one is balanced out)");
	rcImage = before;
}

static void testForgedNumber() {
	printf("\n-- a forged numeric _handle is ignored --\n");
	int before = rcImage;
	Interpreter a = run(
		"img = raylib.GenImageColor(4, 4, \"#0000FF\")\n"
		"img._handle = 12345\n"
		"raylib.ImageDrawPixel img, 1, 1, \"#00FF00\"\n"
		"raylib.ImageClearBackground img, \"#000000\"\n"
		"raylib.UnloadImage img\n"
		"survived = 1\n");
	ok(g_err.empty(), "drawing and unloading with a forged number is not an error");
	eqNum(readGlobal(a, "survived"), 1, "...and the process survives");
	eqNum(Value(rcImage - before), 1, "...and nothing was freed (the real image is simply orphaned)");
	rcImage = before;
}

static void testCrossType() {
	printf("\n-- a handle of the wrong type is ignored --\n");
	int before = rcImage;
	Interpreter a = run(
		"img = raylib.GenImageColor(4, 4, \"#0000FF\")\n"
		"ip = Interp.create\n"
		"img._handle = ip._handle\n"
		"raylib.ImageDrawPixel img, 1, 1, \"#00FF00\"\n"
		"raylib.UnloadImage img\n"
		"ip.dispose\n"
		"survived = 1\n");
	ok(g_err.empty(), "an Interp handle in an Image is treated as no image");
	eqNum(readGlobal(a, "survived"), 1, "...and neither side is damaged");
	rcImage = before;

	run(
		"img = raylib.GenImageColor(4, 4, \"#0000FF\")\n"
		"ip = Interp.create\n"
		"ip._handle = img._handle\n"
		"ip.replLine \"x = 1\"\n");
	ok(errContains("disposed"), "an Image handle in an Interp is rejected");
	rcImage = before;
}

static void testUseAfterUnload() {
	printf("\n-- every copy of an unloaded handle goes dead --\n");
	int before = rcImage;
	Interpreter a = run(
		"img = raylib.GenImageColor(4, 4, \"#FF0000\")\n"
		"copy = {} + img\n"
		"raylib.UnloadImage img\n"
		"raylib.ImageDrawPixel copy, 1, 1, \"#00FF00\"\n"
		"raylib.UnloadImage copy\n"
		"raylib.UnloadImage img\n"
		"survived = 1\n");
	ok(g_err.empty(), "drawing on, or unloading, a dead copy is not an error");
	eqNum(readGlobal(a, "survived"), 1, "...and the process survives");
	eqNum(Value(rcImage - before), 0, "...and the image is released exactly once");
	ok(readGlobal(a, "img").GetDict().Lookup(String("_handle"), Value::Null).IsNull(),
		"the map unloaded through is left with a null _handle");
}

static void testLegitimateUse() {
	printf("\n-- normal use still works --\n");
	int before = rcImage;
	Interpreter a = run(
		"img = raylib.GenImageColor(4, 4, \"#FF0000\")\n"
		"raylib.ImageDrawPixel img, 1, 1, \"#00FF00\"\n"
		"px = raylib.GetImageColor(img, 1, 1)\n"
		"other = raylib.GetImageColor(img, 0, 0)\n"
		"raylib.UnloadImage img\n");
	ok(g_err.empty(), "the script runs clean");
	eqNum(readGlobal(a, "px").GetDict().Lookup(String("g"), Value::Null), 255, "a drawn pixel reads back green");
	eqNum(readGlobal(a, "px").GetDict().Lookup(String("r"), Value::Null), 0, "...with no red");
	eqNum(readGlobal(a, "other").GetDict().Lookup(String("r"), Value::Null), 255, "an untouched pixel is still red");
	eqNum(Value(rcImage - before), 0, "UnloadImage balances GenImageColor");
}

static void testInterpCopies() {
	printf("\n-- copies of an Interp handle --\n");
	Interpreter a = run(
		"ip = Interp.create\n"
		"cp = {} + ip\n"
		"ip.dispose\n"
		"cp.dispose\n"
		"survived = 1\n");
	ok(g_err.empty(), "disposing through a copy after dispose is a no-op");
	eqNum(readGlobal(a, "survived"), 1, "...and does not double-free");

	run(
		"ip = Interp.create\n"
		"cp = {} + ip\n"
		"ip.dispose\n"
		"cp.replLine \"x = 1\"\n");
	ok(errContains("disposed"), "using a copy after dispose is an error, not a dangling pointer");
}

int main() {
	GCManager::Init();
	value_init_constants();
	ErrorTypes::Init();
	AddRaylibIntrinsics();
	AddInterpIntrinsics();

	testHandleIsUnforgeable();
	testForgedNumber();
	testCrossType();
	testUseAfterUnload();
	testLegitimateUse();
	testInterpCopies();

	printf("\n%d checks, %d failures\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
