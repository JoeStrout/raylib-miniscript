//
// macros.h
// raylib-miniscript
//
// Common defines used throughout raylib-miniscript
//

#pragma once
#include "miniscript.h"
#include <cstdio>

// Macro to reduce boilerplate for lambda intrinsics.
// In MiniScript 2.0 the intrinsic callback signature takes Context by value
// (it is a lightweight struct wrapping the VM + argument stack), not by pointer.
#define INTRINSIC_LAMBDA [](MiniScript::Context context, MiniScript::IntrinsicResult partialResult) -> MiniScript::IntrinsicResult

// Raylib version check
#define RAYLIB_VERSION_GT(maj, min) (RAYLIB_VERSION_MAJOR>maj || (RAYLIB_VERSION_MAJOR==maj && RAYLIB_VERSION_MINOR>min))

// Return an intrinsic's funcref with a note, which `info` shows just as it
// shows a MiniScript function's leading string.  The FuncDef behind the
// funcref is a handle shared by every VM, so setting it here sets it
// everywhere; and since GetFunc builds that FuncDef first, order of setup
// does not matter.  Use it where the funcref is registered:
//   raylibModule.SetValue("OpenURL", WithNote(i, "Open a URL in the browser."));
inline MiniScript::Value WithNote(MiniScript::Intrinsic& i, const char* note) {
	MiniScript::Value f = i.GetFunc();
	f.FunctionDef().set_Note(MiniScript::String(note));
	return f;
}

// The same, for an intrinsic created by name (a global, like `exit`), which
// is never passed to SetValue.  Call it LAST, after set_Code and every
// AddParam: getting the funcref builds its FuncDef, copying those as they
// stand, and nothing added afterward would reach it.
inline void SetNote(MiniScript::Intrinsic& i, const char* note) {
	if (!i.Code()) {
		fprintf(stderr, "SetNote(%s): call it after set_Code, not before\n", i.Name().c_str());
		return;
	}
	WithNote(i, note);
}
