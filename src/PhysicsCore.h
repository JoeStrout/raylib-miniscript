//
//  PhysicsCore.h
//  raylib-miniscript
//
//  The physicsCore module: the per-contact work of assets/physics.ms (the 2D
//  rigid-body engine), natively.  assets/physicsFallback.ms is the same API in
//  MiniScript, which physics.ms uses when this module isn't present.
//

#ifndef PHYSICSCORE_H
#define PHYSICSCORE_H

/// Register the `physicsCore` intrinsic (the module map) in the MiniScript environment.
void AddPhysicsCoreIntrinsics();

#endif // PHYSICSCORE_H
