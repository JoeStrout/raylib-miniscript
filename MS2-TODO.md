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

## 3. `env` map — script assignment no longer syncs to the OS environment ✅ NOT APPLICABLE (no code change)

**Where:** `src/MoreIntrinsics.cpp`, `intrinsic_env()` / `envMapRef()`.

**Behavior difference:** `env` returns a live map of environment variables, but assigning
to a member from script (`env.PATH = "..."`) updates only the in-memory map — it does
**not** call `setenv()`. MS1 installed a `ValueDict::SetAssignOverride(assignEnvVar)`
hook so map assignment called `setenv`; MS2 dropped the map assign-override mechanism.
(MS2's own ShellIntrinsics `env` has the same limitation.)

**Why it isn't observable in this host — investigated, no fix needed:**

1. **The in-memory world is self-consistent.** `StaticMap(envMapRef())` wraps the
   host's `envMap` ValueDict via `GCManager::NewMapFromDict`, which *shares storage*
   rather than copying ("later mutations to either are visible through the other").
   So a script write to `env.FOO` lands in `envMapRef()` itself. And every read the
   host does — notably `ExpandVariables`, which expands `$FOO`/`${FOO}`/`$(FOO)` in
   paths — goes through `getEnvMap()` → `envMapRef()`, the *same* dict, never
   `getenv()`. A script that sets `env.FOO` and later reads it, or uses it in a path,
   sees its own write.

2. **Nothing inherits the OS environment in a script-controllable way.** The lost
   `setenv()` only matters to a *child process*. The sole process spawn in the host is
   one hardcoded `system("cp -p …")` in `CopyFileHelper` (`src/FileModule.cpp`, Linux
   fallback only; Apple/Windows use `fcopyfile`/`CopyFile`). That command consults no
   script-set variable — `cp` is resolved via the OS PATH inherited at launch, which is
   untouched. There is no `exec`/`system`/`popen`/shell intrinsic exposed to scripts.

So the divergence between the in-memory map and the OS environment has no trigger here.
Left as-is.

**Reopen if:** an `exec`/`system`-style intrinsic is ever added that scripts can call
with a script-populated environment. Then the write-through gap becomes real, and the
fix is the one MS2's ShellIntrinsics already uses — apply the `env` map to the OS
environment at the point of the `exec` call — not a per-assignment hook.

---

## 4. Replace `GetVar(name)` with `GetArg(index)` throughout the intrinsics ✅ DONE (Pattern A)

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

**Resolution:** wrote a MiniScript code-updater, `scripts/update_code.ms`, that
rewrites `context.GetVar(String("name"))` → `context.GetArg(index)`, where
`index` is the name's position in that intrinsic's `AddParam` list (`self`, when
present, is a normal `AddParam("self")` at index 0, so it maps to `GetArg(0)`
naturally). Trailing/wrapping calls (`.IntValue()`, `ValueToColor(...)`, …) are
preserved. It backs each changed file up to `<name>.bak` (once, never
overwritten) and can `--revert` from those backups.

It handles the **inline-lambda style (Pattern A)** only — `Intrinsic::Create` →
`AddParam`s → `set_Code(INTRINSIC_LAMBDA { …GetVar… })` in one sequential block,
which is the whole `src/R*.cpp` family. It deliberately leaves the **named-impl
style (Pattern B)** alone: there the `GetVar` calls sit in a function defined
*above* the registration, out of param-context reach, and a `set_Code(&func)`
line switches conversion off. Those (`FileModule.cpp`, `MoreIntrinsics.cpp`,
`HttpModule.cpp`, …) are still to be hand-converted. New intrinsics should use
`GetArg` from the start.

**Verified:** ran it across the Pattern A modules (e.g. `RShapes.cpp`: all 271
`GetVar`s converted, indices matching `AddParam` order, zero left), rebuilt, and
it works.

---

### Notes

- All 14 host modules compile cleanly against MS2 with the above stubs in place.
- General MS1→MS2 host-porting patterns are documented separately in
  `MiniScript2/notes/CPP_HOST_UPDATE_GUIDE.md`.
