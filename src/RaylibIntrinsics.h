//
//  RaylibIntrinsics.h
//  raylib-miniscript
//
//  Raylib intrinsics for MiniScript
//

#ifndef RAYLIBINTRINSICS_H
#define RAYLIBINTRINSICS_H

namespace MiniScript {
	struct Interpreter;
}

// Add Raylib intrinsics to the global state
void AddRaylibIntrinsics();

// Clear native callback hooks and callback bridge state.
void ResetRaylibCallbackBridge();

#endif // RAYLIBINTRINSICS_H
