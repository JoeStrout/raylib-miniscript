//
//  MoreIntrinsics.h
//  raylib-miniscript
//
//  Additional intrinsics (import, exit, env, run) for the MiniScript environment.
//

#ifndef MOREINTRINSICS_H
#define MOREINTRINSICS_H

#include "miniscript.h"

namespace MiniScript { struct Interpreter; }

/// Add the import, exit, env, and run intrinsics to the MiniScript environment.
/// Call after the interpreter is created.
void AddMoreIntrinsics();

/// Returns true if the `exit` intrinsic has been called.
bool ExitRequested();

/// Returns the exit result code set by the `exit` intrinsic.
int ExitResultCode();

/// Update the MS_SCRIPT_DIR environment variable to the directory containing
/// the given file path.
void UpdateScriptDir(const char* path);

/// Load new source code into the interpreter and run it: stops the current
/// program, then recompiles, preserving the current global variables.
void RunScriptSource(MiniScript::Interpreter interpreter, MiniScript::String source);

#endif // MOREINTRINSICS_H
