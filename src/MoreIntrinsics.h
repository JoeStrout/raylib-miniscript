//
//  MoreIntrinsics.h
//  raylib-miniscript
//
//  Additional intrinsics (import, exit) for the MiniScript environment.
//

#ifndef MOREINTRINSICS_H
#define MOREINTRINSICS_H

/// Add the import and exit intrinsics to the MiniScript environment.
/// Call after the interpreter is created.
void AddMoreIntrinsics();

/// Returns true if the `exit` intrinsic has been called.
bool ExitRequested();

/// Returns the exit result code set by the `exit` intrinsic.
int ExitResultCode();

#endif // MOREINTRINSICS_H
