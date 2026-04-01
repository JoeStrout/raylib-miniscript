// rtext_smoke.ms
// Smoke checks for raylib 6 text mutation intrinsics and their Alloc variants.

rl = raylib
if not rl.IsWindowReady then rl.InitWindow 

fail = function(msg)
	print "FAIL: " + msg
	// Force a runtime error so smoke runs fail fast.
	x = null[0]
end function

assert = function(ok, msg)
	if not ok then fail(msg)
end function

run = function
	print "rtext smoke: begin"

	replaced = rl.TextReplace("foo bar foo", "foo", "baz")
	assert replaced == "baz bar baz", "TextReplace"

	replacedAlloc = rl.TextReplaceAlloc("foo bar foo", "foo", "baz")
	assert replacedAlloc == "baz bar baz", "TextReplaceAlloc"

	between = rl.TextReplaceBetween("a[old]b", "[", "]", "new")
	assert between == "a[new]b", "TextReplaceBetween"

	betweenAlloc = rl.TextReplaceBetweenAlloc("a[old]b", "[", "]", "new")
	assert betweenAlloc == "a[new]b", "TextReplaceBetweenAlloc"

	inserted = rl.TextInsert("abcd", "XY", 0)
	assert inserted == "XYcd", "TextInsert"

	insertedAlloc = rl.TextInsertAlloc("abcd", "XY", 0)
	assert insertedAlloc == "XYcd", "TextInsertAlloc"

	// Ensure prior results remain stable after additional static-buffer calls.
	keep = rl.TextReplace("123-123", "123", "X")
	_ = rl.TextReplace("hello", "l", "L")
	assert keep == "X-X", "TextReplace result copy stability"

	print "rtext smoke: PASS"
end function

if locals == globals then run
