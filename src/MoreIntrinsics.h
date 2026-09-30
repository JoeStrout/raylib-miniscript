//
//  MoreIntrinsics.h
//  raylib-miniscript
//
//  Additional intrinsics (import, exit, env, shellArgs, run) for the MiniScript environment.
//

#ifndef MOREINTRINSICS_H
#define MOREINTRINSICS_H

#include "miniscript.h"
#include <vector>

namespace MiniScript { struct Interpreter; }

/// Add the import, exit, env, shellArgs, and run intrinsics to the MiniScript environment.
/// Call after the interpreter is created.
void AddMoreIntrinsics();

/// Remember the list for the shellArgs intrinsic: the script path, then the
/// arguments following it on the command line (as in MiniScript 1).  Call
/// before the script runs; if never called, shellArgs is an empty list.
void SetShellArgs(const std::vector<MiniScript::String>& args);

/// Update the MS_SCRIPT_DIR environment variable to the directory containing
/// the given file path.
void UpdateScriptDir(const char* path);

// Snapshot the import search directories, so that later writes to
// env.MS_IMPORT_PATH (or MS_EXE_DIR / MS_SCRIPT_DIR) cannot redirect `import`.
// Called when the sandbox latches; harmless and idempotent otherwise.
void FreezeImportPath();

/// Load new source code into the interpreter and run it: stops the current
/// program, then recompiles, preserving the current global variables.
void RunScriptSource(MiniScript::Interpreter interpreter, MiniScript::String source);

#endif // MOREINTRINSICS_H
