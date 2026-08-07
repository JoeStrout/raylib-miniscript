# Hosting MiniScript from MiniScript

A plan for an `Interp` class in **raylib-miniscript**: a MiniScript-visible
handle on a second `Interpreter`, which a script can seed, feed source to, run
in time slices, and stop.  Mini Micro 2 needs this to have a REPL at all, and
runs on the stock binary, so raylib-miniscript registers it by default; Soda,
embedding the same code, simply never calls `AddInterpIntrinsics()`.

## The problem

Mini Micro's shell is a read-eval-print loop.  The user types a line, it is
compiled and run, and whatever globals it left behind are still there for the
next line.  `run` executes an edited program the same way; ctrl-C stops a
runaway; an error prints a stack trace and drops back to the `]` prompt with
globals intact.

In Mini Micro 1 all of that is C#: `Shell.cs` owns an `Interpreter` and steps it
one 30 ms slice per frame, and `Console.cs` is a line editor drawing into a
`TextDisplay`.  Mini Micro 2 has no C# and no custom host — it is stock
raylib-miniscript plus a tree of MiniScript.  So the shell must be MiniScript,
and a MiniScript program cannot compile and run MiniScript.

Everything else in `Shell.cs` and `Console.cs` *can* be MiniScript, and should
be.  The line editor is `display.print`, `display.backup`, `display.setCursor`,
and a history list — nothing a host is needed for.  The shell commands (`run`,
`load`, `save`, `edit`, `dir`, `clear`) are already MiniScript in
`/sys/startup.ms`, calling one-line intrinsics that do nothing but set a flag.
What is genuinely missing is one thing: **compile this source and step it.**

## Why this has to be in the host

There is no way to build it in-language.  `Interpreter` is a C++ object; the
compiler, the bytecode emitter, and the VM are not reachable from script, and
writing a MiniScript interpreter in MiniScript would be far too slow to run
Mini Micro programs.

The `import` intrinsic already compiles source at runtime
(`Interpreter::CompileToFunc` plus `vm.ManuallyPushCall`, see
`MoreIntrinsics.cpp:301`), and an `eval` built the same way would be a much
smaller change.  It is not enough:

- the compiled code shares the shell's VM, so a runaway loop can never be
  interrupted — there is no time slice to return from;
- a runtime error in it is a runtime error in the shell;
- there is no per-program globals reset, and no isolation of any kind.

A REPL needs a *separate* interpreter that the shell drives.

## What MiniScript 2 already provides

Nearly the whole surface exists on `Interpreter` and just isn't exposed:

| Need | Already there |
|------|---------------|
| Feed one REPL line, accumulating incomplete blocks | `REPL(line, timeLimit)`, `NeedMoreInput()` |
| Echo the value of a bare expression | `lastImplicitResult` |
| Run a whole program | `Reset(source)` + `Compile()` |
| Run a program keeping globals | `ResetPreservingGlobals(source)` |
| Step for a bounded time | `RunUntilDone(timeLimit, returnEarly)` |
| Interrupt | `Stop()` |
| Errors | `Error`, `errorOutput` delegate, `vm.BuildStackTrace()` |
| Seed and read globals | `SetGlobalValue`, `GetGlobalValue` |

Two runtime facts make a *second* interpreter safe, rather than merely
plausible:

**Each VM registers its own GC mark callback** — `GCManager.RegisterMarkCallback`
in the VM constructor (`VM.cs:327`), unregistered in the destructor (`:367`).
Multiple live interpreters are collected correctly by construction; no root
bookkeeping is needed in this module beyond keeping the handle alive.

**Free-variable lookup falls through to the running VM's globals** —
`LookupVariable` (`VM.cs:2841`) checks the frame's locals, then its outer
VarMap, then the interpreter's global namespace, then the intrinsics table.  So a function
compiled in interpreter A, called from VM B, resolves its globals *in B*.  That
is what makes seeding work: give the child a globals map containing the same
`Display`, `text`, `gfx`, `key` values the parent holds, and shared library code
runs unmodified.  Because those are the same heap objects, display state is
genuinely shared, not copied — but rebinding a global name in the child does not
disturb the parent.

`ResetPreservingGlobals` (`Interpreter.cs:217`) deserves a note: it is exactly
the hand-rolled globals-and-type-map copy that `Shell.Break()` does in Mini
Micro 1 (`Shell.cs:552`), done properly inside the runtime.  Break becomes a
one-liner here.

## Architecture

### An `Interp` class, in the house style

A MiniScript map with a `_handle` pointing at a heap-allocated
`InterpHandle { Interpreter interp; String outBuf; }`, exactly as `Texture` and
`Sound` work today (`RaylibTypes.h`).  `Interp.create` returns a new instance;
the handle is freed by `dispose`.  There is no teardown sweep, so a child that
is never disposed lives until the process exits — harmless, but it does mean
`dispose` is the only thing that reclaims a child's VM.

Added by `AddInterpIntrinsics()`, called from `InitMiniScript`.  A host that
never calls it sees no new globals — Soda won't — but raylib-miniscript does,
because Mini Micro 2 is this stock binary plus a tree of MiniScript and its
shell cannot exist without `Interp`.  `-DMS_ENABLE_INTERP=OFF` builds a binary
with no way to spawn a second interpreter.

### The API

| Member | Purpose |
|--------|---------|
| `Interp.create` | New child interpreter, empty globals |
| `p.dispose` | Release the interpreter and its VM |
| `p.setGlobal(name, value)` | One global — `Interpreter::SetGlobalValue` |
| `p.getGlobal(name)` | Read one back — `Interpreter::GetGlobalValue` |
| `p.setGlobals(map)` | Bulk seed; the common case (see below).  Returns the number of names copied |
| `p.load(source, sourceName="")` | `Reset` + `Compile`, fresh globals |
| `p.loadKeepingGlobals(source)` | `ResetPreservingGlobals` |
| `p.replLine(line, seconds=0.03)` | Feed one REPL line, and run it for up to `seconds` |
| `p.needMoreInput` | True when the line left a block open |
| `p.runUntilDone(seconds=0.03)` | Run a slice; returns true if it finished |
| `p.done` / `p.running` | State |
| `p.stop` | Interrupt |
| `p.error` | Last error as a string, or null |
| `p.stackTrace` | List of strings, captured at the error |
| `p.exitRequested` | True if the child called `exit` |
| `p.exitCode` | The result code it passed, or 0 |
| `p.lastResult` | `lastImplicitResult`, for REPL echo |
| `p.takeOutput` | Drain and clear whatever reached stdout/stderr |

`runUntilDone` is the load-bearing one.  It calls
`RunUntilDone(seconds, /*returnEarly*/ true)`, which honors its time limit
regardless of what the child is doing — so the shell gets a frame back even from
`while true; end while`.

### Globals seeding

`setGlobals` copies each key of the given map into the map behind the child's
`Interpreter.GetGlobals()` (`SetGlobalValue` per name would do the same thing one
lookup at a time).  The parent passes its own `globals`; that is a few hundred
names, once per program launch, and the values are pointers.

The spike turned up four rough edges on this path, all of them consequences of
globals being a VarMap that viewed an `@main` register window.  MiniScript 2's
Globals rework removed the whole category: globals are now a namespace the
Interpreter owns (`Interpreter.GetGlobals`, `notes/GLOBALS.md` in MiniScript 2).
Seeding works before the first compile, so there is no bootstrap step; `Set` and
`Get` cannot disagree; no register scan can wander into a callee's locals; and
`Gather`/`Rebind` are gone from the runtime entirely.

What survives is the shape of the `setGlobals` loop: enumerate the source map
without mutating it.  That is now simply a complete enumeration, since every
global is a slot the map views — plain map iteration sees all of them, including
the names a seeded shell was itself seeded with, so an interpreter can seed a
child in turn.  `tests/interp_tests.cpp` covers the lot.

Seeding is the whole isolation story.  The child gets the library layer and
whatever else the shell chooses to publish, and nothing else.  A program that
does `clear`, or assigns over `text`, or dies with an error, cannot touch the
shell's own variables.  Mini Micro 1 has no such boundary and lives with the
consequences.

### Output

Errors need no buffering: `Interpreter.Error` is an error *Value* that already
carries its stack trace, and the runtime clears it on the next line or run,
which is exactly the lifetime the shell wants — so `p.error` and `p.stackTrace`
read that, and `errorOutput` is just a sink keeping a child's errors off the
host's stdout.  Ordinary printing needs no host support at all, because globals
are consulted before the intrinsics table (`LookupVariable`, above): seeding the
child with a MiniScript `print` that writes to the text display shadows it.  The
`write` and `writeLine` intrinsics Mini Micro 1 needed (`Shell.cs:990`) have no
counterpart here.

`standardOutput` is still buffered into `outBuf` and drained by `takeOutput`, so
that anything reaching real stdout — from a program that did *not* shadow
`print` — is visible rather than lost.

### The shared request map

`_run` and `_edit` in Mini Micro 1 are intrinsics that set a host flag which
`Shell.Update` notices (`Shell.cs:966`).  Here they need not be intrinsics at
all.  The shell seeds the child with a plain map that it also holds a reference
to; `_run` writes `req.action = "run"` into it and yields.  The slice ends, the
shell reads the map, and acts.  **`/sys/startup.ms` keeps working with `_run`
and `_edit` shimmed in `mm/`, with no C++ involved.**

### Yield, and why `input` is free

The child's `yield` ends its slice and hands the frame back to the shell, which
is where drawing belongs.  (The `globals.yield` shadow in Mini Micro 2's
`main.ms`, which renders a frame from inside a yield so a tight loop does not
freeze the app, is not needed for child code.)

That gives `input` for nothing.  In Mini Micro 1 it is an intrinsic returning
`Result.Waiting` while the C# console fills a buffer (`Shell.cs:913`).  Here it
is a seeded MiniScript function: set a flag on the request map, yield, and check
for a result.  The shell — still running normally, frame after frame — runs the
line editor for as long as it takes, drops the string in the map, and the
child's next slice returns it.  No partial-result intrinsic, no stashed
`inputReceived`.

`key.get` and any other blocking input work the same way.

### Break

The shell polls the keyboard itself every frame, because it is the thing running
the frame loop.  Ctrl-C is `p.stop`.  No `controlCHandler` threaded through the
host, no `allowControlCBreak` special case, and it works on an infinite loop
because `runUntilDone` always returns.

## Division of labor

Everything below stays in MiniScript, in Mini Micro 2's `assets/mm/`:

- **console.ms** — the port of `Console.cs`: key buffer, input buffer and
  cursor index, history, ghost-text autocomplete accepted with Tab, ctrl-A/E/K/U,
  word-wise motion, mouse selection and copy/paste.
- **shell.ms** — the port of `Shell.cs:465`'s state machine: prompt and
  `morePrompt`, dispatch of a committed line to `replLine`, the request map,
  break, and error reporting.
- **autocomplete.ms** — `Autocomplete.cs`, over `p.getGlobal`.

The host contributes `Interp` and nothing else.  No console, no shell policy, no
text rendering, no key handling.

## Sketch

```
p = Interp.create
p.setGlobals globals            // library layer, print, input, _run, req...

while true
	if console.inputReady then
		p.replLine console.takeLine
		if p.needMoreInput then
			console.startInput env.morePrompt
		end if
	end if

	if not p.done then
		p.runUntilDone 0.03      // returns every frame, whatever the child does
		if p.error then
			text.print p.error
			for line in p.stackTrace; text.print char(9) + line; end for
		end if
		if req.action then handleRequest
	else if not console.inputInProgress then
		console.startInput env.prompt
	end if

	if key.pressed("ctrl-c") then p.stop

	_update                       // draw the frame; the shell owns this
	yield
end while
```

## Rejected alternatives

**A custom host binary.**  Fork `main.cpp` and port `Shell.cs` and `Console.cs`
to C++.  Full parity and no runtime design questions, but Mini Micro 2's text
display is MiniScript, so a C++ console would have to draw through callbacks
into script anyway — and it means maintaining a second host and giving up the
"stock binary plus assets" property.  This is the fallback if `Interp` turns out
to be unworkable, and most of the MiniScript-side console survives either way.

**An `eval` intrinsic.**  Covered above: no interruption, no error containment,
no isolation.

**Host-driven REPL with MiniScript callbacks.**  Keep one interpreter, let the
host own the `Shell.Update` state machine, and have it call a MiniScript hook
via `RunFunction` on idle frames to run the console.  Faithful to Mini Micro 1
and `/sys/startup.ms` would need almost no changes — but the shell then lives in
the same globals as user code, clobberable exactly as in Mini Micro 1, and
control flow is split across the C++/MiniScript boundary in a way that is
unpleasant to debug.

## Risks and open questions

**Settled.**  *Seeding at scale*: 400 names seed and resolve as identifiers, a
parent-compiled function called from the child resolves its globals in the child
and finds the seeded values, mutation through a seeded map is visible on both
sides while a rebinding in the child is not, and all of it survives
`FullCollectGarbage`.  Error containment, time-slicing a runaway loop, `print`
shadowing, and the shared request map check out too.  *Nesting*: a child that
creates an `Interp` of its own works, and a value set two levels down reads back
— `runUntilDone` is called from inside an intrinsic invoked by the parent VM,
but it runs a *different* VM, so there is no re-entrant `VM.Run` on one machine.
*`exit` in a child* ends the child, not the process: `exitASAP`/`exitResult`
were host statics — process-wide state describing a per-machine event, wrong the
moment there are two interpreters — and are now `VM.ExitRequested` /
`VM.ExitCode`, set by `VM.RequestExit` and cleared by `VM.Reset`, with
`Interpreter.ExitRequested()` / `ExitCode()` as the host-facing accessors.  So
`exit` is unconditional in every host: record the request on the VM that made it
and stop, and whoever drives that VM decides what it means.

Still open:

- **Intrinsic-side state.**  Anything the host keeps per-interpreter rather than
  per-VM — the raylib callback bridge is the one to check
  (`ResetRaylibCallbackBridge`, `RCore.cpp:537`) — needs to know which
  interpreter a callback belongs to.
- **Sandboxing** is unaffected: mounts are process-wide and the latch is a
  static, so a child interpreter is sandboxed exactly as the parent is.  It
  gains nothing by being new.

## Departures from this plan (step 2 as built)

**`p.error` and `p.stackTrace` read `Interpreter.Error`, not a captured
buffer.**  The plan had `errorOutput` write into `errBuf` and call
`vm.BuildStackTrace()`.  There is no need: `Interpreter.Error` is an error
*Value* that already carries its trace (`ErrorTypes.RuntimeError` attaches
`value_current_stack_trace()`), it covers compile errors as well as runtime
ones, and the runtime clears it on the next line or run — which is exactly the
lifetime the shell wants.  So `errBuf` is gone; `errorOutput` is a sink that
only keeps a child's errors off the host's stdout.  `outBuf` and `takeOutput`
are as described.

### MiniScript 2 change candidates found here

Two are already fixed upstream — `exit` keeping its state in a host static
(`VM.ExitRequested`), and the globals regime behind `SetGlobalValue` being
REPL-only — which is the model for the rest of these: fix them where they belong
rather than working around them in the host.  What is left:

- `REPL(line, timeLimit)` captures implicit output whenever its loop exits,
  guarded only on `hasImplicitOutput && !hadRuntimeError`
  (`Interpreter.cs:516`).  A time-limit break sets neither, so a run cut short
  still captures r0 — whatever the half-finished computation left there.  In
  practice that reads as null, so a slow expression silently loses its echo
  rather than echoing garbage; a time-sliced REPL wants the capture deferred
  until the run ends.
- `REPL` should honor `SourceFile` the way `Compile` does.  This no longer
  affects `load` (which compiles), only `replLine`.
- Nothing marks `Interpreter.Error` or `Interpreter.lastImplicitResult` as GC
  roots.  They live outside the VM, and both are what a host reads *after* a
  slice, with collection possible in between.  `InterpHandle` registers its own
  mark callback to cover this; the runtime arguably should.

## Implementation order

1. **Done** — the seeding spike, in `tests/interp_tests.cpp`
   (`cmake --build build --target interp_tests && ./build/interp_tests`).
   The design holds; `seedGlobals` and `readGlobal` there are the prototypes of
   `p.setGlobals` and `p.getGlobal`.
2. **Done** — `src/InterpModule.{h,cpp}`, the class behind
   `AddInterpIntrinsics()`, registered by default (`MS_ENABLE_INTERP`, ON),
   since Mini Micro 2 runs on the stock binary.  The second half of
   `tests/interp_tests.cpp` drives it from MiniScript, the way the shell will.
   See **Departures from this plan** for the one place the implementation does
   not match what is written above.
3. `mm/console.ms` — the most mechanical port, testable against a stub that
   just echoes.
4. `mm/shell.ms` — the state machine, break, error reporting.
5. Seed `print`, `input`, `_run`, `_edit` and boot `/sys/startup.ms`.
6. Autocomplete and mouse selection.
