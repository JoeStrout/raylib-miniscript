//
//  InterpModule.h
//  raylib-miniscript
//
//  The `Interp` class: a MiniScript-visible handle on a second Interpreter,
//  which a script can seed, feed source to, run in time slices, and stop.
//  See notes/HOSTING_MS.md.
//

#ifndef INTERPMODULE_H
#define INTERPMODULE_H

/// Register the `Interp` class into the MiniScript environment.  A host that
/// does not call this sees no new globals, so an embedder with no use for child
/// interpreters (Soda) simply leaves it out.  raylib-miniscript itself calls it
/// unless the build sets MS_ENABLE_INTERP=OFF, because Mini Micro 2 runs on the
/// stock binary and its shell is built on `Interp`.
///
/// Note that this module needs no cooperation from the rest of the host.  In
/// particular `exit` needs no special case: the request is recorded on the VM
/// that made it (VM::RequestExit), so a child's `exit` ends the child and is
/// visible to its driver as `p.exitRequested`, while the process only shuts
/// down when the root interpreter's own VM asks for it.
void AddInterpIntrinsics();

#endif // INTERPMODULE_H
