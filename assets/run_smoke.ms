// Smoke test for the `run` intrinsic (MS2-TODO item 2).
// Chaining to another script must (a) preserve the globals of this script, and
// (b) stop this script -- nothing after the `run` line should execute.
// Run with:  build/raylib-miniscript assets/run_smoke.ms
// Expect a series of "ok:" lines and no "FAIL"/"ERROR" lines.

counter = 1
greeting = "hello"
untouched = [1, 2, 3]

double = function(x)
	return x * 2
end function

// A function that reads a global, to prove closures still resolve globals
// after the chain (a top-level function's closure is the globals map itself).
describe = function
	return greeting + " #" + counter
end function

print "first script: counter = " + counter

run env.MS_SCRIPT_DIR + "/run_smoke2.ms"

print "ERROR: first script kept running after `run`"
