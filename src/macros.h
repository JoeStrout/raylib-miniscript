//
// macros.h
// raylib-miniscript
//
// Common defines used throughout raylib-miniscript
//

// Macro to reduce boilerplate for lambda intrinsics.
// In MiniScript 2.0 the intrinsic callback signature takes Context by value
// (it is a lightweight struct wrapping the VM + argument stack), not by pointer.
#define INTRINSIC_LAMBDA [](MiniScript::Context context, MiniScript::IntrinsicResult partialResult) -> MiniScript::IntrinsicResult

// Raylib version check
#define RAYLIB_VERSION_GT(maj, min) (RAYLIB_VERSION_MAJOR>maj || (RAYLIB_VERSION_MAJOR==maj && RAYLIB_VERSION_MINOR>min))
