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

// Set once the script has called WindowShouldClose: it watches for a close
// request itself, so the host lets it finish rather than cutting it off.
extern bool scriptPollsWindowClose;

#endif // RAYLIBINTRINSICS_H
