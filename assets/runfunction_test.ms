// runfunction_test.ms
//
// Focused test of VM.RunFunction (Interpreter.RunFunction) — the synchronous,
// re-entrant "call a MiniScript funcref from native code and get its result
// right now" API that backs the raylib callback bridges (SetTraceLogCallback,
// SetLoadFileTextCallback, etc.).  See MS2-TODO.md item #1.
//
// callback_smoke.ms already covers the basic "did each bridge fire" path; this
// script instead stresses the re-entrancy correctness RunFunction is
// responsible for:
//   1. Safe callee base — heavy register/local work inside a callback must NOT
//      clobber the outer function's live locals (the _nativeFrameTop logic).
//   2. Arbitrary call depth inside a callback — nested calls and recursion must
//      run to completion before RunFunction returns (the sentinel-depth logic).
//   3. Closure capture — a callback that reads an upvalue must see it (OuterVars
//      threaded through the pushed frame).
//   4. Default parameters — when the bridge passes fewer args than the callback
//      declares, RunFunction must fill in the callee's parameter defaults.
//   5. Return value — the callee's result must flow back to the host unchanged.
//
// The trace-log callback is the easiest trigger: rl.TraceLog(level, text) fires
// it synchronously.  LoadFileText is used to check return values (its result is
// handed back to the host, unlike the trace callback whose result is ignored).

if not raylib.IsWindowReady then raylib.InitWindow

rl = raylib

fail = function(msg)
	print "FAIL: " + msg
	// Force a runtime error so the test fails fast and loudly.
	x = null[0]
end function

assert = function(ok, msg)
	if not ok then fail(msg)
end function

state = {}
state.traceCount = 0
state.lastReturn = null

// ---------------------------------------------------------------------------
// 2 & 3: nested calls + recursion + closure capture, all inside the callback.
// ---------------------------------------------------------------------------

// A recursive helper (deep re-entrant call stack while RunFunction drives the VM).
factorial = function(n)
	if n <= 1 then return 1
	return n * factorial(n - 1)
end function

// A plain helper the callback calls (one level of nesting).
doubler = function(x)
	return x * 2
end function

// Closure: captures `secret` from this scope; the callback reads it re-entrantly.
secret = 1234
makeTraceHandler = function
	// Returns a function that closes over `secret` and `state`.
	return function(logLevel, text)
		state.traceCount += 1
		// Do register-heavy work so a wrong callee base would corrupt caller regs.
		total = 0
		for i in range(1, 20)
			total += doubler(i)
		end for
		// Nested + recursive calls during the re-entrant run.
		f5 = factorial(5)          // 120
		// Closure upvalue must be visible here.
		state.lastReturn = secret + total + f5
		return state.lastReturn    // ignored by the trace bridge, but harmless
	end function
end function

// ---------------------------------------------------------------------------
// 4 & 5: default params + return value, via the LoadFileText bridge.
// The bridge calls this with ONE arg (fileName); `prefix` must take its default.
// ---------------------------------------------------------------------------
onLoadText = function(fileName, prefix = "[def]")
	return prefix + ":" + fileName
end function

run = function
	print "runfunction test: begin"

	// --- Re-entrancy while the OUTER function holds live locals. ---
	// These locals live in `run`'s frame.  Every rl.TraceLog below re-enters
	// the VM (via RunFunction) to run a register-heavy callback.  If RunFunction
	// picked an unsafe callee base, these would get clobbered.
	guardA = 111
	guardB = 222
	guardList = [1, 2, 3, 4, 5]
	guardStr = "untouched"

	traceHandler = makeTraceHandler()             // call factory -> inner closure
	rl.SetTraceLogCallback(@traceHandler)         // pass by reference (no invoke)

	// Fire the trace callback several times, checking guards survive each one.
	for k in range(1, 5)
		rl.TraceLog(rl.LOG_INFO, "reentrancy probe " + k)
		assert guardA == 111, "guardA clobbered by callback (bad callee base)"
		assert guardB == 222, "guardB clobbered by callback (bad callee base)"
		assert guardList.len == 5, "guardList clobbered by callback"
		assert guardList[2] == 3, "guardList contents clobbered by callback"
		assert guardStr == "untouched", "guardStr clobbered by callback"
	end for

	// Verify the callback actually ran, and that its nested/recursive/closure
	// math came out right.  range(1,20) is inclusive, so sum(1..20)=210:
	//   secret(1234) + sum(2*i for i in 1..20)=2*210=420 + factorial(5)=120
	//   = 1234 + 420 + 120 = 1774
	assert state.traceCount == 5, "trace callback should have fired 5 times"
	assert state.lastReturn == 1774, "callback nested/closure result wrong: " + state.lastReturn

	// --- Default params + return value flowing back to the host. ---
	rl.SetLoadFileTextCallback(@onLoadText)
	got = rl.LoadFileText("hello.txt")
	assert got == "[def]:hello.txt", "LoadFileText default-param/return wrong: " + got

	// --- Outer state fully restored: run() keeps executing normally after all
	//     the re-entrant calls (this line running at all is the proof). ---
	afterAll = guardA + guardB
	assert afterAll == 333, "outer arithmetic broken after re-entrant calls"

	// Clean up.
	rl.SetTraceLogCallback(null)
	rl.SetLoadFileTextCallback(null)

	print "runfunction test: PASS"
end function

if locals == globals then run
