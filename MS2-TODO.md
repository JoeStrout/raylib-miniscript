# MS2-TODO: deferred / degraded functionality

This file tracks places where the MiniScript 1.x → 2.0 migration left functionality
**stubbed, degraded, or behaving differently**, pending a new solution — usually a
small new MiniScript 2 host API. Each entry says what changed, where, why, what the
MS1 code did, and the proposed fix.

Ordered roughly by impact.

---

## 1. Raylib callback bridge — synchronous funcref invocation ✅ DONE

**Where:** `src/RCore.cpp`, `InvokeMiniScriptCallback()`. Affects the intrinsics
`SetLoadFileDataCallback`, `SetSaveFileDataCallback`, `SetLoadFileTextCallback`,
`SetSaveFileTextCallback`, and `SetTraceLogCallback`.

**Resolution:** Added a host-facing synchronous funcref-call API to MS2:
```cpp
Value VM::RunFunction(Value funcRef, ValueList args);          // cs/VM.cs
Value Interpreter::RunFunction(Value funcRef, ValueList args); // cs/Interpreter.cs (thin wrapper)
```
`InvokeMiniScriptCallback` is now a thin wrapper that calls
`g_callbackBridgeState.interpreter.RunFunction(callback, args)` and returns the result.
**Verified:** transpiled, rebuilt, and `assets/callback_smoke.ms` passes (exercises all
five callbacks, including null/missing-file returns and callback unregistration).

**Why it was hard:** These raylib callbacks are **synchronous C functions** — raylib
calls them mid-operation and needs the result immediately, so the bridge must invoke a
MiniScript funcref *re-entrantly, to completion, right now* (we're already nested inside
`vm.Run()` via the intrinsic that called the raylib function). Unlike `import` (which
can defer via a not-done `IntrinsicResult` and let the outer loop resume), a C callback
is stuck mid-operation and cannot unwind. MS1 did this by building a TAC
`FunctionStorage`, `ManuallyPushCall` + `while (GetTopContext() != caller) Step();`,
reading the result from a temp register. MS2 removed all of that surface (TAC/
`FunctionStorage` gone; no `GetTopContext`/`Step`; no `GetTemp`/`SetTemp`).

**How `RunFunction` works (implemented):** it reuses the same *manual-call sentinel*
the `import` path already uses to run a pushed frame to completion:
- Pick a callee base **above the active native frame** so we don't clobber the calling
  intrinsic's registers. The VM now tracks `_nativeFrameTop = calleeBase + callee.MaxRegs`
  for the duration of each `InvokeNativeCallback`; `RunFunction` uses
  `max(_nativeFrameTop, BaseIndex + CurrentFunction.MaxRegs)`.
- Clear the callee frame, bind args positionally (defaults for missing, error for extra),
  push a `CallInfo` carrying the funcref's closure `OuterVars` with `CopyResultToReg = -1`.
- Arm `_hasPendingManualCall` / `_pendingManualCallDepth = runDepth`; the existing
  `RETURN` handler stops `RunInner` the instant our frame returns (into `ManualCallResult`).
- Drive `RunInner(0)` (re-entrant; `_activeVM` set as in `Run`), then **restore** all
  saved outer execution state (PC, base, func, callStackTop, pending-manual-call fields,
  pending self/super, error). A nested runtime error is surfaced on the outer run.

Because the pre-sized register/`stack` array never reallocates during a run, the
suspended outer `RunInner`'s cached frame pointers stay valid across the nested run.

**Remaining caveats (not exercised by the smoke test):**
- Bound-method funcrefs: `RunFunction` does not inject `self` (callbacks are plain
  functions). Fine for the raylib hooks; revisit if a use case needs method callbacks.
- If a callback function *yields*, it can't (there's no one to yield to inside a C
  callback); such a callback would spin. The raylib file/trace callbacks are simple
  synchronous functions, so this shouldn't arise in practice.

---

## 2. `run` intrinsic — globals not preserved across reset

**Where:** `src/MoreIntrinsics.cpp`, `RunScriptSource()` (does `Reset(source)` +
`Compile()` only). Affects the `run` intrinsic on all platforms.

**Current behavior:** `run "otherScript"` chains to the new script but **discards the
current global variables**. MS1 preserved them.

**Why:** MS1 globals were a plain `ValueDict` (`vm->GetGlobalContext()->variables`), so
`run` snapshotted them, `Reset`+`Compile`d, and restored them. In MS2 globals live in
`@main`'s **named registers**; `SetGlobalValue` is a no-op outside REPL mode, and the
globals VarMap overflow path is still stubbed — so there is no working
snapshot/restore. (The cached type maps that MS1 also saved/restored now live in
`CoreIntrinsics` and persist across a reset, so those no longer need handling.)

**Proposed MS2 fix (agreed approach — inject at reset):** because a reset happens
*before* execution, `@main` is the only frame and its register block can be grown
safely. So:
1. `Value Interpreter::GetAllGlobals()` — snapshot the *old* program's globals by
   scanning `@main`'s `names[]`/`stack[]` (already populated post-run) into a map.
2. After `Reset(source)` + compile, **inject** the saved globals into the new `@main`:
   for each `name→value`, update the matching `@main` register or **append** a new one
   (bumping `@main.MaxRegs`). Pre-seed the name so pre-assignment reads see it; the new
   script's own assignments still win.

The one prerequisite: the global **name→register map isn't available at inject time**
(`FuncDef` stores only `ParamNames`; names are written to `names[]` at runtime by the
`NAME_rA_kBC`/`ASSIGN_rA_rB_kC` opcodes). Cleanest fix: have the code generator emit a
`@main` name→register table into its `FuncDef`. (Alternative: decode `@main`'s bytecode
at reset — no codegen change, but fragile.) Appending preserved globals as *named
registers* also means the register-backed globals VarMap will see them, sidestepping
the stubbed overflow-dict path.

Then `RunScriptSource` collapses to: snapshot → `Reset` → compile-with-injected-globals.

---

## 3. `env` map — script assignment no longer syncs to the OS environment

**Where:** `src/MoreIntrinsics.cpp`, `intrinsic_env()` / `envMapRef()`.

**Current behavior:** `env` returns a live map of environment variables, but assigning
to a member from script (`env.PATH = "..."`) updates only the in-memory map — it does
**not** call `setenv()`. Host code that changes the OS env still works (via
`setEnvVar()`); reads via `env` work. Only the script-driven write-through is lost.

**Why:** MS1 installed a `ValueDict::SetAssignOverride(assignEnvVar)` hook so map
assignment called `setenv`. MS2 dropped the map assign-override mechanism. (MS2's own
ShellIntrinsics `env` has the same limitation — it keeps a plain map and syncs to the
OS env only through explicit host calls.)

**Proposed fix / options:**
- Accept the limitation (document that `env` is read-mostly; provide an explicit
  `setEnv(name, value)` intrinsic for writes), **or**
- reintroduce an assignment hook in MS2 (a per-map "on assign" callback), if the
  write-through behavior is deemed important.

Lowest impact of the three; a small explicit `setEnv` intrinsic likely suffices.

---

## 4. Replace `GetVar(name)` with `GetArg(index)` throughout the intrinsics

**Where:** essentially every intrinsic in `src/R*.cpp`, `FileModule.cpp`,
`HttpModule.cpp`, `MoreIntrinsics.cpp`, etc. — they read arguments with
`context.GetVar(String("name"))`, the MS1 idiom.

**Status:** works, but it's the **slow path**.  `GetVar` does a linear,
string-compare search of the parameter names (and builds a `Value` string for
the name) on every call; `GetArg(i)` is a direct positional array index.  For
per-frame drawing/input intrinsics this overhead is pointless.

(Historical note: `GetVar` was also *broken* for intrinsics in an early MS2 build
— it resolved against the VM's current frame, which for a native call is the
caller, not the intrinsic — so every argument read back null.  That was fixed in
MS2 by giving `Context` the intrinsic's own parameter names; `GetVar` is correct
now, just slower than `GetArg`.)

**Proposed fix:** migrate each intrinsic to `context.GetArg(0)`, `GetArg(1)`, …
matching the order of its `AddParam` calls.  Mechanical but bulk; do it
per-module and test.  New intrinsics should use `GetArg` from the start.

---

### Notes

- All 14 host modules compile cleanly against MS2 with the above stubs in place.
- General MS1→MS2 host-porting patterns are documented separately in
  `MiniScript2/notes/CPP_HOST_UPDATE_GUIDE.md`.
