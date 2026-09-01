//
//  InterpModule.cpp
//  raylib-miniscript
//
//  Step 2 of notes/HOSTING_MS.md: the `Interp` class -- a MiniScript-visible
//  handle on a second Interpreter, which a script can seed, feed source to,
//  run in time slices, and stop.  Mini Micro 2's shell is built on this; see
//  that note for why an `eval` intrinsic is not enough.
//
//  Nothing here is registered unless the host calls AddInterpIntrinsics(), so a
//  host with no use for child interpreters sees no new globals.  This host does
//  call it (unless built with MS_ENABLE_INTERP=OFF): Mini Micro 2 is the stock
//  binary plus a tree of MiniScript, and its shell needs Interp.
//
//  This file deliberately depends on nothing but MiniScript -- no raylib, none
//  of the rest of src/ -- so that tests/interp_tests.cpp can exercise it.
//

#include "InterpModule.h"
#include "miniscript.h"

namespace MiniScript {

#define INTRINSIC_LAMBDA [](Context context, IntrinsicResult partialResult) -> IntrinsicResult

static const Value& kHandle() { static Value v("_handle"); return v; }

//--------------------------------------------------------------------------------
// InterpHandle: the C++ side of an Interp instance
//--------------------------------------------------------------------------------

// A child interpreter plus the small amount of state the host keeps for it.
//
// Two things here are not obvious:
//
// GC.  The child's VM registers its own mark callback (VM.cs:279), so the
// child's stack and globals are roots by construction.  What is *not* covered
// is the handful of Values the Interpreter itself holds outside the VM --
// `Error` (and the stack-trace list hanging off it) and `lastImplicitResult`.
// Nothing in the runtime marks those, and both are exactly what a shell reads
// *after* a slice, with parent code (and therefore allocation, and therefore
// collection) in between.  So the handle registers a mark callback of its own.
//
// Output.  TextOutputMethod is a bare function pointer with no user data, so
// the delegates cannot know which handle they belong to.  They don't have to:
// output only ever happens while we are running a child, and ActiveChild is set
// for exactly that window.
struct InterpHandle {
	Interpreter interp;
	String outBuf;			// drained by takeOutput

	InterpHandle();
	~InterpHandle();

	static void MarkRoots(void* userData) {
		InterpHandle* h = (InterpHandle*)userData;
		if (IsNull(h->interp)) return;
		GCManager::Mark(h->interp.Error());
		GCManager::Mark(h->interp.lastImplicitResult());
	}
};

InterpHandle::InterpHandle() {
	GCManager::RegisterMarkCallback(MarkRoots, this);
}

InterpHandle::~InterpHandle() {
	GCManager::UnregisterMarkCallback(MarkRoots, this);
}

// The child we are currently running, or null.  This exists only for the output
// delegates, which are bare function pointers with no user data and so have no
// other way to know whose output they are carrying.  Nothing else should reach
// for it: anything holding a Context can ask who is running, and anything
// holding a handle already knows.  Saved and restored, so a child that drives a
// child of its own nests correctly.
static InterpHandle* g_activeChild = nullptr;

struct ActiveChild {
	InterpHandle* prev;
	ActiveChild(InterpHandle* h) : prev(g_activeChild) { g_activeChild = h; }
	~ActiveChild() { g_activeChild = prev; }
};

//--------------------------------------------------------------------------------
// Output delegates
//--------------------------------------------------------------------------------

// Ordinary printing from a child usually never reaches here: the shell seeds
// the child with a MiniScript `print` that draws to the text display, and
// globals are consulted before the intrinsics table, so the shadow wins.  This
// buffer is for output from a program that did *not* shadow print -- visible
// via takeOutput rather than lost.
static void CaptureOut(String s, Boolean lineBreak) {
	if (g_activeChild == nullptr) return;
	g_activeChild->outBuf = g_activeChild->outBuf + s + (lineBreak ? String("\n") : String(""));
}

// REPL echo of a bare expression.  Only replLine can produce this: a loaded
// program is compiled and run rather than fed to REPL(), so one that happens to
// end in an expression does not echo it.
static void CaptureImplicit(String s, Boolean lineBreak) {
	if (g_activeChild == nullptr) return;
	CaptureOut(s, lineBreak);
}

// Errors are not buffered.  The Interpreter keeps the error as a Value
// (`Error`), with the stack trace hanging off it, until the next line or run
// clears it -- which is a better source for the shell than a formatted string,
// so `error` and `stackTrace` below read that instead.  Swallowing here just
// keeps a child's error off the host's stdout, where nobody asked for it.
static void SwallowErr(String, Boolean) {
}

//--------------------------------------------------------------------------------
// Interp instances
//--------------------------------------------------------------------------------

static InterpHandle* GetHandle(Context context) {
	Value self = context.GetArg(0);
	if (self.Type() != ValueType::Map) {
		context.vm.RaiseRuntimeError("Interp required for self parameter");
		return nullptr;
	}
	Value handleVal = self.GetDict().Lookup(kHandle(), Value::Null);
	if (handleVal.Type() != ValueType::Number) {
		context.vm.RaiseRuntimeError("Interp has been disposed");
		return nullptr;
	}
	return (InterpHandle*)(intptr_t)handleVal.DoubleValue();
}

// Point a child's output at our capture buffers.
//
// Seeding used to need a bootstrap step here as well: globals lived in a VarMap
// that only a compile created, so a host had to run REPL("") before it could
// call SetGlobalValue.  Globals are now a namespace the Interpreter owns and
// creates on demand (Interpreter.GetGlobals), so seeding works on a child that
// has never compiled anything, and nothing but the delegates is left to do.
static void Bootstrap(InterpHandle* h) {
	h->interp.set_standardOutput(&CaptureOut);
	h->interp.set_implicitOutput(&CaptureImplicit);
	h->interp.set_errorOutput(&SwallowErr);
}

// Compile source as a program, ready to run on the next runUntilDone.
//
// There is no program-vs-REPL split in where globals live: they are a namespace
// the Interpreter owns, which every compiled @main resolves against, so "run a
// program, it ends, you type at the prompt and its globals are still there"
// holds by construction.  What the two loads differ in is only whether that
// namespace is replaced: Reset builds a fresh one, and ResetPreservingGlobals
// is the same thing with the namespace carried over (and the outgoing program
// stopped, which matters if it was still running).
//
// Compiling here rather than at the first runUntilDone is what lets a load
// report a compile error: it lands in `error` before the caller has run
// anything, which is what a shell wants to print instead of starting a program
// that cannot run.  A failed compile leaves no VM, so the child reads as done.
static void LoadSource(InterpHandle* h, String source, bool freshGlobals) {
	ActiveChild guard(h);
	if (freshGlobals) {
		h->interp.Reset(source);
		h->interp.Compile();
	} else {
		h->interp.ResetPreservingGlobals(source);	// compiles, too
	}
}

// Copy every global of `src` into the child.
//
// This used to have to reach past the map it was handed: globals were a VarMap
// viewing a VM's registers, whose hash table held only the names that had been
// gathered out of them, so iterating it missed every global running code had
// assigned.  A globals map is now a complete view of the Globals table behind
// it -- every global is a slot, and iteration walks exactly the bound ones --
// so plain map iteration sees all of them, and reading the source cannot
// disturb it.
//
// The destination side needs no VM either: Interpreter.GetGlobals() creates the
// child's namespace if this is its first use, so a child can be seeded before
// it has been given any source.
static int SeedGlobals(InterpHandle* h, Value src) {
	if (!src.IsMap()) return 0;
	Value dst = h->interp.GetGlobals().AsMap();
	if (!dst.IsMap()) return 0;

	int n = 0;
	for (int it = src.IterNext(-1); it != Value::MAP_ITER_DONE; it = src.IterNext(it)) {
		Value entry = src.IterEntry(it);
		Value k = entry.MapGet(Value("key"));
		if (!k.IsString()) continue;
		dst.MapSet(k, entry.MapGet(Value("value")));
		n++;
	}
	return n;
}

//--------------------------------------------------------------------------------
// The Interp class
//--------------------------------------------------------------------------------

const Value& InterpClass() {
	static ValueDict interpClass;
	static Value classValue;   // wrapped and GC-rooted at the end of the build
	if (!classValue.IsNull()) return classValue;

	interpClass.SetValue(kHandle(), Value::Null);

	Intrinsic f;

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = new InterpHandle();
		h->interp = Interpreter::New();
		Bootstrap(h);

		ValueDict inst;
		inst.SetValue(Value::magicIsA, InterpClass());
		inst.SetValue(kHandle(), Value((double)(intptr_t)h));
		return IntrinsicResult(DynamicMap(inst));
	});
	// create a new child interpreter, with empty globals
	interpClass.SetValue(String("create"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value self = context.GetArg(0);
		if (self.Type() != ValueType::Map) return IntrinsicResult::Null;
		Value handleVal = self.GetDict().Lookup(kHandle(), Value::Null);
		if (handleVal.Type() != ValueType::Number) return IntrinsicResult::Null;
		InterpHandle* h = (InterpHandle*)(intptr_t)handleVal.DoubleValue();
		if (h == g_activeChild) {
			context.vm.RaiseRuntimeError("cannot dispose an Interp while it is running");
			return IntrinsicResult::Null;
		}
		delete h;
		self.MapSet(kHandle(), Value::Null);
		return IntrinsicResult::Null;
	});
	// release this interpreter and its VM
	interpClass.SetValue(String("dispose"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("name", "");
	f.AddParam("value");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		h->interp.SetGlobalValue(context.GetArg(1).ToString(), context.GetArg(2));
		return IntrinsicResult::Null;
	});
	// set one global in the child
	interpClass.SetValue(String("setGlobal"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("name", "");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		return IntrinsicResult(h->interp.GetGlobalValue(context.GetArg(1).ToString()));
	});
	// read one global back out of the child
	interpClass.SetValue(String("getGlobal"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("map");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		// The values are heap objects shared with the caller, not copies: a map
		// seeded this way is the *same* map, so library state is genuinely
		// shared.  Rebinding a name in the child, though, is the child's alone.
		return IntrinsicResult(Value(SeedGlobals(h, context.GetArg(1))));
	});
	// bulk-seed the child's globals from a map (typically the caller's `globals`);
	// returns the number of names copied
	interpClass.SetValue(String("setGlobals"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("source", "");
	f.AddParam("sourceName", "");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		h->interp.set_SourceFile(context.GetArg(2).ToString());
		LoadSource(h, context.GetArg(1).ToString(), /*freshGlobals*/ true);
		return IntrinsicResult::Null;
	});
	// load and compile a program into the child, with empty globals; check
	// `error` for a compile error, and runUntilDone to run it
	interpClass.SetValue(String("load"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("source", "");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		LoadSource(h, context.GetArg(1).ToString(), /*freshGlobals*/ false);
		return IntrinsicResult::Null;
	});
	// as load, but the child keeps the globals it already has
	interpClass.SetValue(String("loadKeepingGlobals"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("line", "");
	f.AddParam("seconds", 0.03);
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		ActiveChild guard(h);
		h->interp.REPL(context.GetArg(1).ToString(), context.GetArg(2).DoubleValue());
		return IntrinsicResult::Null;
	});
	// feed one REPL line to the child, and run it for up to `seconds`; if it is
	// not finished by then, keep calling runUntilDone
	interpClass.SetValue(String("replLine"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		return IntrinsicResult(h->interp.NeedMoreInput() ? Value::one : Value::zero);
	});
	// true when the last replLine left a block open
	interpClass.SetValue(String("needMoreInput"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("seconds", 0.03);
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		double seconds = context.GetArg(1).DoubleValue();
		ActiveChild guard(h);
		// RunUntilDone honors its time limit whatever the child is doing, so
		// the shell gets its frame back even from `while true; end while`.
		h->interp.RunUntilDone(seconds, /*returnEarly*/ true);
		return IntrinsicResult(h->interp.Done() ? Value::one : Value::zero);
	});
	// run the child for up to `seconds`; returns true if it finished
	interpClass.SetValue(String("runUntilDone"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		// A loaded program is compiled at load, so the child is already
		// "running" (at the first instruction) before its first slice, and
		// this needs no state of its own to say so.
		return IntrinsicResult(h->interp.Done() ? Value::one : Value::zero);
	});
	// true when the child has nothing left to run
	interpClass.SetValue(String("done"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		return IntrinsicResult(h->interp.Running() ? Value::one : Value::zero);
	});
	// the inverse of done
	interpClass.SetValue(String("running"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		h->interp.Stop();
		return IntrinsicResult::Null;
	});
	// interrupt the child (this is what ctrl-C is)
	interpClass.SetValue(String("stop"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		// `exit` in a child is recorded on the child's own VM and goes no
		// further -- the host process is not the child's to end.  This is how a
		// shell tells a program that exited from one that reached its end;
		// both leave the child done.
		return IntrinsicResult(h->interp.ExitRequested() ? Value::one : Value::zero);
	});
	// true if the child called `exit`
	interpClass.SetValue(String("exitRequested"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		return IntrinsicResult(Value(h->interp.ExitCode()));
	});
	// the result code the child passed to `exit`, or 0
	interpClass.SetValue(String("exitCode"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		Value err = h->interp.Error();
		if (err.IsNull()) return IntrinsicResult::Null;
		return IntrinsicResult(Value(ErrorTypes::DescribeError(err)));
	});
	// the child's last error as a string, or null
	interpClass.SetValue(String("error"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		Value err = h->interp.Error();
		if (err.IsNull()) return IntrinsicResult::Null;
		// Runtime errors carry the trace they were raised with; compiler errors
		// have none, and answer null.
		return IntrinsicResult(err.Stack());
	});
	// list of strings, captured where the error was raised
	interpClass.SetValue(String("stackTrace"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		return IntrinsicResult(h->interp.lastImplicitResult());
	});
	// value of the last bare expression a replLine evaluated, for REPL echo
	interpClass.SetValue(String("lastResult"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		InterpHandle* h = GetHandle(context);
		if (h == nullptr) return IntrinsicResult::Null;
		if (h->outBuf.empty()) return IntrinsicResult::Null;
		String out = h->outBuf;
		h->outBuf = nullptr;
		return IntrinsicResult(Value(out));
	});
	// drain and clear whatever the child printed to stdout
	interpClass.SetValue(String("takeOutput"), f.GetFunc());

	classValue = GCManager::NewMapFromDict(interpClass);   // shares interpClass's storage
	GCManager::AddRoot(classValue);
	Intrinsic::AddShortName(classValue, String("Interp"));

	return classValue;
}

} // namespace MiniScript

void AddInterpIntrinsics() {
	MiniScript::Intrinsic f = MiniScript::Intrinsic::Create("Interp");
	f.set_Code([](MiniScript::Context context, MiniScript::IntrinsicResult partialResult)
			-> MiniScript::IntrinsicResult {
		return MiniScript::IntrinsicResult(MiniScript::InterpClass());
	});
}

//--------------------------------------------------------------------------------
// Two things this module does NOT do, both recorded in notes/HOSTING_MS.md
// under "Departures from this plan":
//
// - It does not work around the one MiniScript 2 rough edge it still runs into:
//   REPL capturing implicit output after a cut-short run, which affects
//   replLine.  The others are gone.  The Globals rework fixed SetGlobalValue
//   being REPL-only and Reset not clearing the REPL globals; with one namespace
//   for both modes, `load` became Reset(source) + Compile(), which reports
//   compile errors at load and honors SourceFile (REPL ignores it).
//
// - It does not address the raylib callback bridge
//   (ResetRaylibCallbackBridge, RCore.cpp:537), which is per-host rather than
//   per-interpreter, so a child and a parent that both install a raylib
//   callback share one slot.  Mini Micro 2's shell doesn't, but it is the open
//   question the note flags under "Intrinsic-side state".
//--------------------------------------------------------------------------------
