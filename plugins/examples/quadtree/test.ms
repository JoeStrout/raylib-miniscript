// quadtree plugin -- tests.  Run from the repo root:
//   ./build/raylib-miniscript plugins/examples/quadtree/test.ms
// Prints a line per check and a summary; no window needed.

failures = 0
checks = 0

ok = function(cond, label)
	outer.checks = outer.checks + 1
	if cond then
		print "ok   " + label
	else
		outer.failures = outer.failures + 1
		print "FAIL " + label
	end if
end function

eq = function(got, want, label)
	ok got == want, label + "  (got " + got + ", want " + want + ")"
end function

// Same ids regardless of order.
sameSet = function(a, b)
	return a.sort == b.sort
end function

// The plugin list
ok plugins.hasIndex("quadtree"), "plugins lists quadtree"
eq plugins.quadtree.loaded, 1, "plugins.quadtree.loaded"
eq plugins.quadtree.version, "1.0", "plugins.quadtree.version"
ok not plugins.hasIndex("no_such_plugin"), "plugins does not list unknown plugins"

// create / count / insert
t = quadtree.create(0, 0, 100, 100)
eq quadtree.count(t), 0, "new tree is empty"
eq quadtree.insert(t, 1, 10, 10), 1, "insert inside bounds returns 1"
eq quadtree.insert(t, 2, 90, 90), 1, "insert a second point"
eq quadtree.count(t), 2, "count after two inserts"
eq quadtree.insert(t, 3, 150, 50), 0, "insert outside bounds returns 0"
eq quadtree.insert(t, 4, -1, 50), 0, "insert left of bounds returns 0"
eq quadtree.count(t), 2, "rejected points are not counted"
eq quadtree.insert(t, 5, 0, 0), 1, "insert on the min corner"
eq quadtree.insert(t, 6, 100, 100), 1, "insert on the max corner"

// query
t = quadtree.create(0, 0, 100, 100)
quadtree.insert t, 1, 10, 10
quadtree.insert t, 2, 20, 20
quadtree.insert t, 3, 80, 80
ok sameSet(quadtree.query(t, 0, 0, 30, 30), [1, 2]), "query lower-left"
ok sameSet(quadtree.query(t, 70, 70, 30, 30), [3]), "query upper-right"
eq quadtree.query(t, 40, 40, 10, 10).len, 0, "query an empty area returns an empty list"
ok sameSet(quadtree.query(t, 0, 0, 100, 100), [1, 2, 3]), "query everything"
ok sameSet(quadtree.query(t, 10, 10, 0, 0), [1]), "zero-size query hits a point exactly on it"
eq quadtree.query(t, 500, 500, 10, 10).len, 0, "query entirely outside the tree"

// Enough points to force splits, checked against brute force.
t = quadtree.create(0, 0, 1000, 1000)
pts = []
seed = 12345
rnd = function()
	outer.seed = (outer.seed * 1103515245 + 12345) % 2147483648
	return outer.seed / 2147483648
end function
for i in range(1, 500)
	p = {"id": i, "x": rnd * 1000, "y": rnd * 1000}
	pts.push p
	quadtree.insert t, p.id, p.x, p.y
end for
eq quadtree.count(t), 500, "500 points inserted"
allMatch = true
for q in range(1, 25)
	qx = rnd * 900; qy = rnd * 900; qw = rnd * 200; qh = rnd * 200
	want = []
	for p in pts
		if p.x >= qx and p.x <= qx + qw and p.y >= qy and p.y <= qy + qh then want.push p.id
	end for
	if not sameSet(quadtree.query(t, qx, qy, qw, qh), want) then allMatch = false
end for
ok allMatch, "25 random queries match brute force"

// Many points at one spot must not recurse forever.
t = quadtree.create(0, 0, 100, 100)
for i in range(1, 100)
	quadtree.insert t, i, 50, 50
end for
eq quadtree.count(t), 100, "100 coincident points are all stored"
eq quadtree.query(t, 49, 49, 2, 2).len, 100, "and all found"

// clear keeps the bounds
quadtree.clear t
eq quadtree.count(t), 0, "clear empties the tree"
eq quadtree.query(t, 0, 0, 100, 100).len, 0, "query after clear is empty"
eq quadtree.insert(t, 1, 50, 50), 1, "tree is reusable after clear"
eq quadtree.insert(t, 2, 500, 500), 0, "bounds survive clear"

// Two trees are independent.
a = quadtree.create(0, 0, 10, 10)
b = quadtree.create(0, 0, 10, 10)
quadtree.insert a, 1, 5, 5
eq quadtree.count(a), 1, "tree a has its point"
eq quadtree.count(b), 0, "tree b is unaffected"

// Bad arguments are errors, not crashes.
isErr = function(v)
	return v isa error
end function
ok isErr(quadtree.count(42)), "count(non-tree) is an error"
ok isErr(quadtree.insert(null, 1, 1, 1)), "insert(null) is an error"
ok isErr(quadtree.query("tree", 0, 0, 1, 1)), "query(string) is an error"
ok isErr(quadtree.clear({})), "clear(map) is an error"

// Dropped trees are collected without trouble.
for i in range(1, 200)
	g = quadtree.create(0, 0, 100, 100)
	quadtree.insert g, i, 1, 1
end for
g = null
gc.collect true
ok true, "200 discarded trees collected"

print
print checks + " checks, " + failures + " failures"
