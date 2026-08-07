//
//  interp_tests.cpp
//  raylib-miniscript
//
//  Step 1 of notes/HOSTING_MS.md: the seeding spike.
//
//  The whole `Interp` plan rests on one thing being true -- that a second
//  Interpreter can be handed the first one's globals, run library code the
//  first one compiled, and survive a collection.  Nothing in the runtime is
//  built for that specifically: SetGlobalValue writes into the interpreter's
//  own global namespace, and free-variable lookup falls through to whichever VM
//  is *running* (VM.cs:2646), which is what should make a shared function
//  resolve its globals in the child.
//
//  So this is a spike, not a unit test of shipped code.  It exercises the
//  runtime directly to find out whether the design holds up, and it stays
//  afterwards as the regression test for the module built on top of it.
//
//  Build:  cmake --build build --target interp_tests
//  Run:    ./build/interp_tests
//

#include "miniscript.h"
#include "InterpModule.h"

#include <stdio.h>
#include <string.h>

using namespace MiniScript;

static int failures = 0;
static int checks = 0;

static void ok(bool condition, const char* what) {
	checks++;
	if (condition) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s\n", what);
		failures++;
	}
}

static void eqNum(Value actual, double expected, const char* what) {
	checks++;
	if (actual.IsNumber() && actual.DoubleValue() == expected) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s (got %s, expected %g)\n", what, actual.ToString().c_str(), expected);
		failures++;
	}
}

static void eqStr(Value actual, const char* expected, const char* what) {
	checks++;
	if (strcmp(actual.ToString().c_str(), expected) == 0) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s (got \"%s\", expected \"%s\")\n", what,
			actual.ToString().c_str(), expected);
		failures++;
	}
}

//--------------------------------------------------------------------------------
// Output capture
//
// Two interpreters run here, and the point of several tests is that output from
// one does not come from the other.  A single sink plus a marker set before each
// step keeps that visible without a per-interpreter delegate (which the real
// module will need, but which is not what is under test).

static String g_out;
static String g_err;

static void CaptureOut(String s, Boolean lineBreak) {
	g_out = g_out + s + (lineBreak ? String("\n") : String(""));
}

static void CaptureErr(String s, Boolean lineBreak) {
	g_err = g_err + s + (lineBreak ? String("\n") : String(""));
}

static void clearCapture() {
	g_out = String("");
	g_err = String("");
}

static bool outContains(const char* needle) {
	return strstr(g_out.c_str(), needle) != nullptr;
}

static bool errContains(const char* needle) {
	return strstr(g_err.c_str(), needle) != nullptr;
}

//--------------------------------------------------------------------------------
// Helpers under test
//
// These two are the prototype of what InterpModule will do.  If they hold up,
// `p.setGlobals` and `p.create` are these functions with a handle around them.

// A child interpreter with its output routed here.
//
// This used to have to run REPL("") first: globals lived in a VarMap that only
// a compile created, so SetGlobalValue was a no-op until then.  An Interpreter
// now owns its namespace and makes it on demand, so a brand-new child can be
// seeded as it stands.
static Interpreter newChild() {
	Interpreter p = Interpreter::New();
	p.set_standardOutput(&CaptureOut);
	p.set_implicitOutput(&CaptureOut);
	p.set_errorOutput(&CaptureErr);
	return p;
}

// Read a global straight out of the globals table, bypassing GetGlobalValue.
//
// The two agree in every case these tests exercise -- there is one namespace
// now, and both routes reach the same slot -- but reading the table directly is
// what several tests below are actually asserting about.
static Value readGlobal(Interpreter p, const char* name) {
	Value g = p.GetGlobals().AsMap();
	if (!g.IsMap()) return Value::Null;
	Value out;
	if (!g.TryGet(Value(name), &out)) return Value::Null;
	return out;
}

// Copy every global of `src` into `dst`.  Values are heap objects shared
// between the two, not copies: a map seeded this way is the *same* map.
//
// This used to have to reach past the globals map: it was a view over the VM's
// registers whose hash table held only the entries gathered out of them, so
// enumerating it missed everything running code had assigned, and the scan had
// to take in @main's named registers as well.  Every global is now a slot in
// the namespace the map views, and iteration walks exactly the bound ones, so
// one pass over the map sees all of them -- and reading the source cannot
// disturb it.
static int seedGlobals(Interpreter dst, Interpreter src) {
	Value g = src.GetGlobals().AsMap();
	if (!g.IsMap()) return -1;

	int n = 0;
	for (int it = g.IterNext(-1); it != Value::MAP_ITER_DONE; it = g.IterNext(it)) {
		Value entry = g.IterEntry(it);
		Value k = entry.MapGet(Value("key"));
		if (!k.IsString()) continue;
		dst.SetGlobalValue(k.ToString(), entry.MapGet(Value("value")));
		n++;
	}
	return n;
}

//--------------------------------------------------------------------------------
// The library layer stands in for Soda's lib/: maps and functions defined in the
// parent, which the child must be able to use without recompiling them.

static const char* kLibSource =
	"libState = {\"count\": 0}\n"
	"bump = function(n)\n"
	"	libState.count = libState.count + n\n"
	"	return libState.count\n"
	"end function\n"
	"greet = function(who)\n"
	"	return \"hello \" + who\n"
	"end function\n";

static Interpreter makeParent() {
	Interpreter a = Interpreter::New();
	a.set_standardOutput(&CaptureOut);
	a.set_implicitOutput(&CaptureOut);
	a.set_errorOutput(&CaptureErr);
	a.REPL(String(kLibSource));
	return a;
}

//--------------------------------------------------------------------------------

// The bootstrap assumption above, checked rather than trusted.
static void testBootstrap() {
	printf("\n-- bootstrap --\n");
	clearCapture();

	Interpreter p = Interpreter::New();
	p.set_errorOutput(&CaptureErr);
	ok(IsNull(p.vm()), "a fresh interpreter has no VM");

	p.SetGlobalValue(String("early"), Value(1.0));
	eqNum(readGlobal(p, "early"), 1, "seeding works before anything has been compiled");
	ok(IsNull(p.vm()), "...and did not have to make a VM to do it");
	ok(g_err.empty(), "...without an error");

	p.SetGlobalValue(String("x"), Value(42.0));
	eqNum(p.GetGlobalValue(String("x")), 42, "GetGlobalValue reads a seeded name back");

	clearCapture();
	p.REPL(String("y = x + 1"));
	ok(g_err.empty(), "the first compiled line runs");
	eqNum(p.GetGlobalValue(String("y")), 43, "...and sees what was seeded");

	// A blank line is a no-op, and in particular does not replace the namespace.
	p.REPL(String(""));
	eqNum(p.GetGlobalValue(String("y")), 43, "a REPL(\"\") disturbs nothing");
	eqNum(readGlobal(p, "early"), 1, "...and the pre-compile seeding is still there");
}

// A global used to reach the host by two routes -- a register in the @main
// frame if compiled code assigned it, the globals VarMap's hash table if the
// host seeded it -- and running code could not tell them apart.  Both are now
// the same slot in the one namespace, which is what makes that indistinguishable
// by construction; this checks it, since the two routes are what the whole
// seeding story rests on.
static void testGlobalReadPath() {
	printf("\n-- global read path --\n");
	clearCapture();

	Interpreter p = newChild();
	p.SetGlobalValue(String("seeded"), Value(7.0));
	p.REPL(String("assigned = 9"));

	eqNum(readGlobal(p, "assigned"), 9, "the globals map holds a name assigned by running code");
	eqNum(readGlobal(p, "seeded"), 7, "...and a seeded name");

	eqNum(p.GetGlobalValue(String("assigned")), 9, "GetGlobalValue sees the assigned name");
	eqNum(p.GetGlobalValue(String("seeded")), 7, "GetGlobalValue sees the seeded name too");

	// And running code resolves both alike.
	clearCapture();
	p.REPL(String("sum = seeded + assigned"));
	ok(g_err.empty(), "running code resolves both kinds of global");
	eqNum(p.GetGlobalValue(String("sum")), 16, "...to the same values");
}

// One namespace across compilations.
//
// This was the Gather/Rebind test: globals were a VarMap over @main's
// registers, so every new @main meant re-binding one program's globals onto the
// next one's register window, and Gather (which detaches that backing) left the
// map answering with whatever was true at the moment it ran.  None of that
// machinery is left -- the namespace is the Interpreter's, each REPL line is
// just another @main resolving against it -- so what is worth checking now is
// simply that globals do not care where they came from or how many
// compilations ago.
static void testNamespaceAcrossCompiles() {
	printf("\n-- namespace across compiles --\n");
	clearCapture();

	Interpreter p = newChild();
	p.SetGlobalValue(String("seeded"), Value(1.0));
	p.REPL(String("before = 2"));
	p.REPL(String("after = before + seeded"));
	ok(g_err.empty(), "each line compiles and runs against the same namespace");

	eqNum(readGlobal(p, "seeded"), 1, "a seeded name survives later compilations");
	eqNum(readGlobal(p, "before"), 2, "...and so does one an earlier line assigned");
	eqNum(readGlobal(p, "after"), 3, "...which the later line could read");

	// A Reset is the one thing that does replace the namespace.
	p.Reset();
	ok(readGlobal(p, "before").IsNull(), "Reset drops the globals");
}

// A host reads globals while a program is stopped mid-call -- which is exactly
// what a shell does at a breakpoint, or after ctrl-C.  What it must get is the
// global, never whatever the interrupted function happened to call its local.
static void testGlobalsAreNotLocals() {
	printf("\n-- globals are not locals --\n");
	clearCapture();

	Interpreter r = Interpreter::New();
	r.set_standardOutput(&CaptureOut);
	r.set_errorOutput(&CaptureErr);
	r.Reset(String(
		"shared = \"global value\"\n"
		"f = function\n"
		"	hidden = \"a local\"\n"
		"	shared = \"a local too\"\n"
		"	while true\n"
		"		yield\n"
		"	end while\n"
		"end function\n"
		"f\n"));
	r.Compile();
	r.RunUntilDone(0.05, true);
	ok(r.Running(), "the program is stopped inside a call");

	ok(r.GetGlobalValue(String("hidden")).IsNull(), "a callee's local is not a global");
	eqStr(r.GetGlobalValue(String("shared")), "global value",
		"a global shadowed by a callee's local still reads as the global");
	ok(!r.GetGlobalValue(String("f")).IsNull(), "ordinary globals still read back");

	r.Stop();
}

// The central question: does a function compiled in A, called from B, resolve
// its globals in B -- and find the seeded values there?
static void testSeededLibraryCall() {
	printf("\n-- seeded library call --\n");
	clearCapture();

	Interpreter a = makeParent();
	ok(g_err.empty(), "library compiles clean in the parent");

	Interpreter b = newChild();
	int seeded = seedGlobals(b, a);
	ok(seeded > 0, "seedGlobals copied something");
	printf("     (%d globals seeded)\n", seeded);

	ok(!readGlobal(b, "bump").IsNull(), "child can see the seeded funcref");

	clearCapture();
	b.REPL(String("result = bump(5)"));
	ok(g_err.empty(), "calling a parent-compiled function from the child raises no error");
	eqNum(readGlobal(b, "result"), 5, "...and it returns the right value");

	clearCapture();
	b.REPL(String("greeting = greet(\"world\")"));
	eqStr(readGlobal(b, "greeting"), "hello world", "a second seeded function works too");
}

// Seeded values are shared objects, so library *state* is shared -- but a name
// rebound in the child must not disturb the parent.  That split is the whole
// isolation story in HOSTING_MS.md.
static void testSharingAndIsolation() {
	printf("\n-- sharing vs. rebinding --\n");
	clearCapture();

	Interpreter a = makeParent();
	Interpreter b = newChild();
	seedGlobals(b, a);

	b.REPL(String("bump 7"));
	a.REPL(String("parentSees = libState.count"));
	eqNum(readGlobal(a, "parentSees"), 7, "mutation through a seeded map is visible in the parent");

	// Rebinding the name in the child must not reach the parent's binding.
	b.REPL(String("libState = \"clobbered\""));
	a.REPL(String("stillAMap = libState isa map"));
	eqNum(readGlobal(a, "stillAMap"), 1, "child rebinding a global leaves the parent's binding alone");
	eqStr(readGlobal(b, "libState"), "clobbered", "...and the child does see its own");

	// The parent's own functions still work after the child clobbered its copy.
	clearCapture();
	a.REPL(String("afterward = bump(1)"));
	ok(g_err.empty(), "parent library still runs after the child clobbered its binding");
	eqNum(readGlobal(a, "afterward"), 8, "...with state intact");
}

// Mini Micro seeds the whole library layer: a few hundred names, once per
// program launch -- far more than any @main's register window would have held
// back when globals lived in one.
static void testSeedingAtScale() {
	printf("\n-- seeding at scale --\n");
	clearCapture();

	Interpreter b = newChild();
	const int kCount = 400;
	for (int i = 0; i < kCount; i++) {
		char name[32];
		snprintf(name, sizeof(name), "seed%d", i);
		b.SetGlobalValue(String(name), Value((double)i));
	}

	eqNum(readGlobal(b, "seed0"), 0, "first of 400 seeded names");
	eqNum(readGlobal(b, "seed399"), 399, "last of 400 seeded names");

	// Reading them back through GetGlobalValue is one thing; a running program
	// resolving them as identifiers is the case that matters.
	clearCapture();
	b.REPL(String("total = seed1 + seed2 + seed399"));
	ok(g_err.empty(), "running code resolves seeded names as identifiers");
	eqNum(readGlobal(b, "total"), 402, "...to the right values");
}

// Each VM registers its own GC mark callback (VM.cs:279), so both interpreters
// should be roots.  A seeded value reachable only from the child's globals is
// the interesting case: nothing on either stack names it.
static void testGCSurvival() {
	printf("\n-- gc survival --\n");
	clearCapture();

	Interpreter a = makeParent();
	Interpreter b = newChild();
	seedGlobals(b, a);

	// A map the child alone will hold, dropped from the parent's namespace.
	a.REPL(String("orphan = {\"tag\": \"kept alive by the child\"}"));
	b.SetGlobalValue(String("orphan"), readGlobal(a, "orphan"));
	a.REPL(String("orphan = null"));

	GCManager::FullCollectGarbage();
	GCManager::FullCollectGarbage();

	clearCapture();
	b.REPL(String("tag = orphan.tag"));
	ok(g_err.empty(), "child globals survive a full collection");
	eqStr(readGlobal(b, "tag"), "kept alive by the child", "...with contents intact");

	clearCapture();
	b.REPL(String("afterGC = bump(3)"));
	ok(g_err.empty(), "seeded funcrefs still callable after a full collection");
	eqNum(readGlobal(b, "afterGC"), 3, "...and library state survived too");

	// And the parent is still usable from its side.
	clearCapture();
	a.REPL(String("parentAfterGC = bump(4)"));
	ok(g_err.empty(), "parent still runs after a full collection");
	eqNum(readGlobal(a, "parentAfterGC"), 7, "...sharing the same library state");
}

// The shell gets a frame back whatever the child is doing.  Without this,
// ctrl-C needs host support; with it, break is p.stop.
static void testTimeSliceAndStop() {
	printf("\n-- time slice and stop --\n");
	clearCapture();

	Interpreter b = newChild();
	b.Reset(String("i = 0\nwhile true\n	i = i + 1\nend while\n"));
	b.Compile();

	b.RunUntilDone(0.05, true);
	ok(!b.Done(), "an infinite loop is still running after a slice");
	ok(g_err.empty(), "...with no error");

	Value progress = readGlobal(b, "i");
	ok(progress.IsNumber() && progress.DoubleValue() > 0, "...and it made progress");

	b.RunUntilDone(0.05, true);
	ok(!b.Done(), "a second slice also returns");

	b.Stop();
	ok(b.Done(), "Stop ends it");
}

// A child that dies must not take the shell with it.
static void testErrorContainment() {
	printf("\n-- error containment --\n");
	clearCapture();

	Interpreter a = makeParent();
	Interpreter b = newChild();
	seedGlobals(b, a);

	clearCapture();
	b.REPL(String("boom = 1 / 0\nnoSuchFunction"));
	ok(!g_err.empty(), "the child reports its error");
	ok(errContains("Undefined") || errContains("undefined") || errContains("Identifier"),
		"...as an undefined-identifier error");

	// The parent is untouched: still compiles, still runs, still has its state.
	clearCapture();
	a.REPL(String("survived = bump(2)"));
	ok(g_err.empty(), "parent unaffected by the child's error");
	eqNum(readGlobal(a, "survived"), 2, "...with its globals intact");

	// And the child itself recovers, as a REPL must.
	clearCapture();
	b.REPL(String("recovered = greet(\"again\")"));
	ok(g_err.empty(), "child recovers and takes the next line");
	eqStr(readGlobal(b, "recovered"), "hello again", "...running seeded code still");
}

// Print in the child must be routable to the shell's text display.  The plan
// leans on globals being consulted before the intrinsics table
// (LookupVariable, VM.cs:2646), so a seeded MiniScript `print` shadows the
// intrinsic and no host output routing is needed for ordinary output.
static void testPrintShadowing() {
	printf("\n-- print shadowing --\n");
	clearCapture();

	Interpreter a = makeParent();
	a.REPL(String(
		"printed = []\n"
		"print = function(s=\"\")\n"
		"	printed.push s\n"
		"end function\n"));
	ok(g_err.empty(), "a MiniScript print shadow compiles");

	Interpreter b = newChild();
	seedGlobals(b, a);

	clearCapture();
	b.REPL(String("print \"to the display\""));
	ok(g_err.empty(), "child print raises no error");
	ok(!outContains("to the display"), "child print did NOT reach the host stdout delegate");

	a.REPL(String("captured = printed[0]"));
	eqStr(readGlobal(a, "captured"), "to the display",
		"...it reached the shadowing function instead");
}

// The request-map trick that replaces Mini Micro 1's _run / _edit intrinsics:
// a plain map both sides hold, written by the child, read by the shell.
static void testSharedRequestMap() {
	printf("\n-- shared request map --\n");
	clearCapture();

	Interpreter a = makeParent();
	a.REPL(String("req = {\"action\": null}"));
	Value req = readGlobal(a, "req");
	ok(req.IsMap(), "the shell holds a request map");

	Interpreter b = newChild();
	b.SetGlobalValue(String("req"), req);
	b.REPL(String(
		"_run = function\n"
		"	req.action = \"run\"\n"
		"end function\n"
		"_run"));
	ok(g_err.empty(), "child writes the request without error");

	Value action = req.MapGet(Value("action"));
	eqStr(action, "run", "the shell sees the child's request through the shared map");
}

//--------------------------------------------------------------------------------
// The Interp class (src/InterpModule.cpp)
//
// Step 1 above establishes that the runtime supports a second interpreter.
// What follows drives the module built on that, the way the shell will: from
// MiniScript, in a parent interpreter, with the child reached only through the
// class.  Source is written with MiniScript's doubled-quote escape, so
// `""x""` in a C string is a quoted `"x"` in the script.

// Run a parent program to completion.  The bound is on iterations rather than
// time so that a child which fails to make progress fails the test instead of
// hanging it.
static Interpreter runParent(const char* source) {
	Interpreter a = Interpreter::New();
	a.set_standardOutput(&CaptureOut);
	a.set_implicitOutput(&CaptureOut);
	a.set_errorOutput(&CaptureErr);
	a.Reset(String(source));
	a.Compile();
	for (int i = 0; i < 500 && !a.Done(); i++) a.RunUntilDone(0.1, true);
	return a;
}

// create, seed, feed a line, read a global back, dispose.
static void testInterpBasics() {
	printf("\n-- Interp: basics --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.setGlobal \"x\", 21\n"
		"p.replLine \"y = x * 2\"\n"
		"got = p.getGlobal(\"y\")\n"
		"err = p.error\n"
		"isDone = p.done\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");
	ok(readGlobal(a, "err").IsNull(), "the child reports no error");
	eqNum(readGlobal(a, "got"), 42, "a seeded global is visible to a REPL line");
	eqNum(readGlobal(a, "isDone"), 1, "the child is done after a short line");
}

// The case the shell actually uses: seed from the parent's own `globals`.  The
// parent here is in program mode, which used to mean every one of its globals
// was a named register that iterating the map would miss entirely; now `globals`
// is a complete view either way, and this is what proves it.
static void testInterpSeedFromGlobals() {
	printf("\n-- Interp: setGlobals from the parent's globals --\n");
	clearCapture();

	Interpreter a = runParent(
		"libState = {\"count\": 0}\n"
		"bump = function(n)\n"
		"	libState.count = libState.count + n\n"
		"	return libState.count\n"
		"end function\n"
		"p = Interp.create\n"
		"seeded = p.setGlobals(globals)\n"
		"p.replLine \"r = bump(5)\"\n"
		"err = p.error\n"
		"childR = p.getGlobal(\"r\")\n"
		"parentCount = libState.count\n"
		"p.replLine \"libState = \"\"clobbered\"\"\"\n"
		"stillAMap = libState isa map\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");
	Value seeded = readGlobal(a, "seeded");
	ok(seeded.IsNumber() && seeded.DoubleValue() > 0, "setGlobals copied register-backed globals");
	ok(readGlobal(a, "err").IsNull(), "the child calls a parent-compiled function without error");
	eqNum(readGlobal(a, "childR"), 5, "...and gets the right value");
	eqNum(readGlobal(a, "parentCount"), 5, "mutation through a seeded map reaches the parent");
	eqNum(readGlobal(a, "stillAMap"), 1, "a name the child rebound is still the parent's own");
}

// The load-bearing one: the shell gets its frame back whatever the child does.
static void testInterpSliceAndStop() {
	printf("\n-- Interp: time slice and stop --\n");
	clearCapture();

	Interpreter a = runParent(
		"nl = char(10)\n"
		"p = Interp.create\n"
		"p.load \"i = 0\" + nl + \"while true\" + nl + \"i = i + 1\" + nl + \"end while\"\n"
		"d1 = p.runUntilDone(0.02)\n"
		"d2 = p.runUntilDone(0.02)\n"
		"progress = p.getGlobal(\"i\")\n"
		"err = p.error\n"
		"p.stop\n"
		"d3 = p.done\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");
	eqNum(readGlobal(a, "d1"), 0, "an infinite loop is unfinished after a slice");
	eqNum(readGlobal(a, "d2"), 0, "...and after a second slice");
	ok(readGlobal(a, "err").IsNull(), "...with no error");
	Value progress = readGlobal(a, "progress");
	ok(progress.IsNumber() && progress.DoubleValue() > 0, "...having made progress");
	eqNum(readGlobal(a, "d3"), 1, "stop ends it");
}

// A child that dies reports the error and its trace, and takes the next line.
static void testInterpErrors() {
	printf("\n-- Interp: error and stack trace --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.replLine \"noSuchThing\"\n"
		"e = p.error\n"
		"st = p.stackTrace\n"
		"traceIsList = st isa list\n"
		"p.replLine \"ok = 1 + 1\"\n"
		"recovered = p.getGlobal(\"ok\")\n"
		"afterErr = p.error\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent is untouched by the child's error");
	Value e = readGlobal(a, "e");
	ok(!e.IsNull() && strstr(e.ToString().c_str(), "nknown") != nullptr,
		"p.error describes the undefined identifier");
	eqNum(readGlobal(a, "traceIsList"), 1, "p.stackTrace is a list");
	eqNum(readGlobal(a, "recovered"), 2, "the child takes the next line");
	ok(readGlobal(a, "afterErr").IsNull(), "...and the error is cleared");
}

// Globals must survive from a program to the prompt that follows it, and `load`
// must start clean while `loadKeepingGlobals` does not.  This is the whole
// reason LoadSource runs programs through REPL rather than Reset + Compile.
static void testInterpLoadAndGlobals() {
	printf("\n-- Interp: load, and globals across programs --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.load \"a = 10\"\n"
		"p.runUntilDone 1\n"
		"afterRun = p.getGlobal(\"a\")\n"
		"p.replLine \"b = a + 5\"\n"
		"atPrompt = p.getGlobal(\"b\")\n"
		"p.load \"c = 1\"\n"
		"p.runUntilDone 1\n"
		"goneA = p.getGlobal(\"a\")\n"
		"p.loadKeepingGlobals \"d = c + 1\"\n"
		"p.runUntilDone 1\n"
		"keptC = p.getGlobal(\"d\")\n"
		"err = p.error\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");
	ok(readGlobal(a, "err").IsNull(), "no errors along the way");
	eqNum(readGlobal(a, "afterRun"), 10, "a loaded program's globals are readable when it ends");
	eqNum(readGlobal(a, "atPrompt"), 15, "...and a REPL line afterwards still sees them");
	ok(readGlobal(a, "goneA").IsNull(), "load starts the next program with empty globals");
	eqNum(readGlobal(a, "keptC"), 2, "loadKeepingGlobals does not");
}

// load compiles, so it can answer for the program before anything has run --
// which is what a shell prints instead of starting a program that cannot run.
static void testInterpLoadErrors() {
	printf("\n-- Interp: compile errors at load --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.load(\"if true then\", \"unfinished.ms\")\n"	// no end if: not a REPL, so this is an error
		"errAtLoad = p.error\n"
		"doneAfterError = p.done\n"
		"p.load \"x = 1 + \"\n"
		"errBadExpr = p.error\n"
		"p.load \"good = 21 * 2\"\n"
		"errAfterGoodLoad = p.error\n"
		"runningBeforeSlice = p.running\n"
		"p.runUntilDone 1\n"
		"val = p.getGlobal(\"good\")\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");

	Value err = readGlobal(a, "errAtLoad");
	ok(!err.IsNull(), "an unterminated block is an error at load, not at the first slice");
	ok(strstr(err.ToString().c_str(), "unfinished.ms") != nullptr,
		"...reported against the source name load was given");
	eqNum(readGlobal(a, "doneAfterError"), 1, "a program that did not compile leaves the child done");

	ok(!readGlobal(a, "errBadExpr").IsNull(), "a syntax error is caught at load too");

	ok(readGlobal(a, "errAfterGoodLoad").IsNull(), "a later good load clears the error");
	eqNum(readGlobal(a, "runningBeforeSlice"), 1, "a loaded program reads as running before its first slice");
	eqNum(readGlobal(a, "val"), 42, "...and runs when given one");
}

// Implicit output (for REPL echo) and buffered stdout (for a program that did
// not shadow print).
static void testInterpOutput() {
	printf("\n-- Interp: lastResult and takeOutput --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.replLine \"2 + 3\"\n"
		"lr = p.lastResult\n"
		"echo = p.takeOutput\n"
		"p.replLine \"print \"\"hello\"\"\"\n"
		"printed = p.takeOutput\n"
		"drained = p.takeOutput\n"
		"p.load \"7 + 7\"\n"
		"p.runUntilDone 1\n"
		"programEcho = p.takeOutput\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");
	eqNum(readGlobal(a, "lr"), 5, "lastResult is the value of a bare expression");
	eqStr(readGlobal(a, "echo"), "5\n", "...which is also echoed into the output buffer");
	eqStr(readGlobal(a, "printed"), "hello\n", "an unshadowed print lands in the buffer");
	ok(readGlobal(a, "drained").IsNull(), "takeOutput clears what it returns, and answers null when empty");
	ok(readGlobal(a, "programEcho").IsNull(), "a loaded program does not echo its last expression");
}

// A child that drives a child of its own.  Nothing re-enters one VM -- each
// runUntilDone runs a different machine -- but the note asks for it to be
// tested rather than assumed.
static void testInterpNesting() {
	printf("\n-- Interp: nesting --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.replLine \"q = Interp.create\"\n"
		"p.replLine \"q.replLine \"\"z = 7\"\"\"\n"
		"p.replLine \"zz = q.getGlobal(\"\"z\"\")\"\n"
		"err = p.error\n"
		"deep = p.getGlobal(\"zz\")\n"
		"p.replLine \"q.dispose\"\n"
		"p.dispose\n");

	ok(g_err.empty(), "the parent script runs clean");
	ok(readGlobal(a, "err").IsNull(), "a grandchild raises no error");
	eqNum(readGlobal(a, "deep"), 7, "a value set two levels down reads back");
}

// The Interpreter holds Error (with its stack trace) and lastImplicitResult
// outside the VM, where nothing in the runtime marks them.  The handle's own
// mark callback is what keeps them alive across the collection that parent code
// will inevitably trigger before the shell gets around to reading them.
static void testInterpGCRoots() {
	printf("\n-- Interp: handle GC roots --\n");
	clearCapture();

	Interpreter a = newChild();
	a.REPL(String("p = Interp.create"));
	a.REPL(String("p.replLine \"noSuchThing\""));
	a.REPL(String("q = Interp.create"));
	a.REPL(String("q.replLine \"[1, 2, 3]\""));

	GCManager::FullCollectGarbage();
	GCManager::FullCollectGarbage();

	clearCapture();
	a.REPL(String("e = p.error"));
	a.REPL(String("traceLen = p.stackTrace.len"));
	a.REPL(String("n = q.lastResult.len"));
	ok(g_err.empty(), "reading a child's error state after a collection raises nothing");

	Value e = readGlobal(a, "e");
	ok(!e.IsNull() && strstr(e.ToString().c_str(), "nknown") != nullptr,
		"the error message survives a full collection");
	Value traceLen = readGlobal(a, "traceLen");
	ok(traceLen.IsNumber() && traceLen.DoubleValue() > 0, "...and so does its stack trace");
	eqNum(readGlobal(a, "n"), 3, "lastImplicitResult survives too");

	a.REPL(String("p.dispose"));
	a.REPL(String("q.dispose"));
}

// `exit` is recorded on the VM that ran it, so a child's exit is the child's
// alone: it ends that program, tells its driver so, and reaches no further.
// Nothing in this module has to special-case it.
static void testInterpExit() {
	printf("\n-- Interp: exit --\n");
	clearCapture();

	Interpreter a = runParent(
		"nl = char(10)\n"
		"p = Interp.create\n"
		"p.load \"before = 1\" + nl + \"exit 7\" + nl + \"after = 2\"\n"
		"while not p.done\n"
		"	p.runUntilDone 0.05\n"
		"end while\n"
		"ranFirstPart = p.getGlobal(\"before\")\n"
		"stoppedAtExit = p.getGlobal(\"after\") == null\n"
		"exited = p.exitRequested\n"
		"code = p.exitCode\n"
		"err = p.error\n"
		// A program that just ends is not an exit, and the state is per-run.
		"p.load \"plain = 1\"\n"
		"p.runUntilDone 1\n"
		"exitedAgain = p.exitRequested\n"
		"codeAgain = p.exitCode\n"
		"p.dispose\n"
		"parentAlive = 1\n");

	ok(g_err.empty(), "the parent script runs clean");
	eqNum(readGlobal(a, "parentAlive"), 1, "a child's exit does not stop the parent");
	eqNum(readGlobal(a, "ranFirstPart"), 1, "the child ran up to the exit");
	eqNum(readGlobal(a, "stoppedAtExit"), 1, "...and no further");
	ok(readGlobal(a, "err").IsNull(), "exit is not an error");
	eqNum(readGlobal(a, "exited"), 1, "p.exitRequested reports the exit");
	eqNum(readGlobal(a, "code"), 7, "...with the result code");
	eqNum(readGlobal(a, "exitedAgain"), 0, "the next program starts out not exiting");
	eqNum(readGlobal(a, "codeAgain"), 0, "...with the code reset");
}

// dispose releases the handle; using the instance afterwards is an error, not a
// jump through a dangling pointer.
static void testInterpDispose() {
	printf("\n-- Interp: dispose --\n");
	clearCapture();

	Interpreter a = runParent(
		"p = Interp.create\n"
		"p.replLine \"x = 1\"\n"
		"p.dispose\n"
		"after = p.getGlobal(\"x\")\n");

	ok(!g_err.empty(), "using a disposed Interp is a runtime error");
	ok(errContains("disposed"), "...saying so");
}

//--------------------------------------------------------------------------------

int main() {
	GCManager::Init();
	value_init_constants();
	ErrorTypes::Init();
	AddInterpIntrinsics();

	// This binary links neither MoreIntrinsics.cpp nor MS2's ShellIntrinsics, so
	// it has no `exit` of its own.  Register the same one-liner both of those
	// are: exit records the request on the VM that ran it and stops there.  That
	// the request then surfaces as p.exitRequested, and reaches no further, is
	// what testInterpExit is about.
	Intrinsic f = Intrinsic::Create("exit");
	f.AddParam("resultCode", Value::Null);
	f.set_Code([](Context ctx, IntrinsicResult partialResult) -> IntrinsicResult {
		Value resultCode = ctx.GetArg(0);
		ctx.vm.RequestExit(resultCode.IsNull() ? 0 : (Int32)resultCode.IntValue());
		return IntrinsicResult::Null;
	});

	testBootstrap();
	testGlobalReadPath();
	testNamespaceAcrossCompiles();
	testGlobalsAreNotLocals();
	testSeededLibraryCall();
	testSharingAndIsolation();
	testSeedingAtScale();
	testGCSurvival();
	testTimeSliceAndStop();
	testErrorContainment();
	testPrintShadowing();
	testSharedRequestMap();

	testInterpBasics();
	testInterpSeedFromGlobals();
	testInterpSliceAndStop();
	testInterpErrors();
	testInterpLoadAndGlobals();
	testInterpLoadErrors();
	testInterpOutput();
	testInterpNesting();
	testInterpGCRoots();
	testInterpExit();
	testInterpDispose();

	printf("\n%d checks, %d failures\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
