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

## 2. `run` intrinsic — globals not preserved across reset ✅ DONE

**Where:** `src/MoreIntrinsics.cpp`, `RunScriptSource()`. Affects the `run` intrinsic
on all platforms.

**Old behavior:** `run "otherScript"` chained to the new script but **discarded the
current global variables** (MS1 preserved them) — and, separately, **let the old script
keep running**: the `run` intrinsic returns into the old VM's still-active `Run` loop,
so the remainder of the old `@main` executed *before* the new script started.

**Resolution:** MS2 already had the machinery, in the REPL rather than where a host
could reach it. A VarMap over `@main`'s registers *is* the globals map, and
`VM.Reset(functions, globalsMap)` rebinds it to the new program: `Rebind` gathers the
old register values into the map's hash table, and as the new program runs,
`NAME`/`ASSIGN` at base 0 call `MapToRegister`, which pulls each preserved value back
out of the table into the new register. Globals the new script never names stay as
hash entries and are found by `LookupVariable`'s globals fallback. So no codegen
change and no `@main` name→register table are needed after all. MS2 additions:

```csharp
Value VM.GetGlobalsVarMap();                        // was private
void Interpreter.ResetPreservingGlobals(String src); // capture + Reset + Compile
```
plus two supporting fixes in `cs/VM.cs`: `Reset` now builds the intrinsics table when
the VM doesn't have one (it was keyed on "full reset", so a *fresh* VM given preserved
globals got an empty table), and `MarkRoots` now marks `ReplGlobals` — the gathered
entries live only in that map, and nothing else was keeping it alive (a latent GC bug
in REPL mode too).

A top-level function carried across the chain keeps working, and still sees the
globals: its closure captured `callStack[0].LocalVarMap`, which is that same map.

`ResetPreservingGlobals` also **stops the outgoing VM** — that's the second half of
the old behavior. Replacing `interpreter.vm` does not stop the VM we're called from
(we're inside its `Run` loop, in the `run` intrinsic), so the rest of the abandoned
script used to execute before the new one started. It lives in MS2 rather than in
`RunScriptSource` because every host chaining scripts needs it and the failure is
silent. So `RunScriptSource` is now a single call.

**Verified:** transpiled, rebuilt, and `assets/run_smoke.ms` (which chains to
`run_smoke2.ms`) passes with all `ok:` lines — number/string/list globals preserved,
a carried-over function still callable, its closure seeing both the old and the
reassigned value, the `globals` map holding the carried-over entries, and the old
script *not* continuing past the `run`. On the MS2 side,
`TestResetPreservingGlobals` in `cs/UnitTests.cs` passes (it collects garbage between
the two programs, so it also covers the `ReplGlobals` root), and the 684 integration
tests still pass.

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
