// Second half of the `run` smoke test; see run_smoke.ms.

check = function(label, condition)
	if condition then
		print "ok: " + label
	else
		print "FAIL: " + label
	end if
end function

print "second script running"

// Globals this script never assigns are still in scope...
check "number global preserved", counter == 1
check "string global preserved", greeting == "hello"
check "list global preserved", untouched.len == 3

// ...including functions, which still see the globals through their closure.
check "function global callable", double(21) == 42
check "closure sees globals", describe == "hello #1"

// Reading a global before assigning it in this script gets the old value;
// assigning it then wins from here on.
counter = counter + 1
check "global reassigned", counter == 2
check "closure sees new value", describe == "hello #2"

// The globals map itself sees the carried-over variables.
check "globals map has old entry", globals.hasIndex("untouched")

// And a global defined only here works normally.
fresh = "new"
check "new global works", fresh == "new"

print "run smoke test complete"
