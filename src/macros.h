//
// macros.h
// raylib-miniscript
//
// Common defines used throughout raylib-miniscript
//

// Macro to reduce boilerplate for lambda intrinsics
#define INTRINSIC_LAMBDA [](Context *context, IntrinsicResult partialResult) -> IntrinsicResult

// Raylib version check
#define RAYLIB_VERSION_GT(maj, min) (RAYLIB_VERSION_MAJOR>maj || (RAYLIB_VERSION_MAJOR==maj && RAYLIB_VERSION_MINOR>min))
