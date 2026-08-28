// Matrix intrinsic class -- tests for the storage/handle slice.
// Covers: construction, element access, bounds and type errors, the null-handle
// rule for `new Matrix`, handle independence between instances, and cleanup.

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

// MS2 errors are values; `isa error` is how script tests for one.
isErr = function(v)
	return v isa error
end function

// Compare a whole matrix at once: shape plus contents, in one check.
matches = function(m, r, c, flat, label)
	if m.rows != r or m.columns != c then
		ok false, label + "  (shape " + m.rows + "x" + m.columns + ", want " + r + "x" + c + ")"
		return
	end if
	got = m.toFlatList
	if got != flat then
		ok false, label + "  (got " + got + ", want " + flat + ")"
		return
	end if
	ok true, label
end function

print "-- construction --"
m = Matrix.ofSize(3, 4)
eq m.rows, 3, "rows recorded"
eq m.columns, 4, "columns recorded"
eq m.capacity, 12, "capacity is rows*columns elements"
eq m.getElem(0, 0), 0, "zero-filled by default"
eq m.getElem(2, 3), 0, "...including the last element"

f = Matrix.ofSize(2, 2, 7.5)
eq f.getElem(0, 0), 7.5, "initialValue fills"
eq f.getElem(1, 1), 7.5, "...every element"

print
print "-- element access --"
m.setElem 1, 2, 42
eq m.getElem(1, 2), 42, "setElem then getElem round-trips"
eq m.getElem(1, 1), 0, "neighbors untouched"
eq m.getElem(2, 2), 0, "...in both directions"

// Row-major layout: writing along a row must not disturb other rows.
for c in range(0, 3)
	m.setElem 0, c, c + 1
end for
eq m.getElem(0, 3), 4, "row writes land in order"
eq m.getElem(1, 3), 0, "next row unaffected by row-major writes"

print
print "-- zero-size matrices --"
z = Matrix.ofSize(0, 0)
eq z.rows, 0, "0x0 has zero rows"
eq z.columns, 0, "0x0 has zero columns"
eq z.capacity, 0, "0x0 allocates nothing"
zr = Matrix.ofSize(0, 5)
eq zr.rows, 0, "0x5 has zero rows"
eq zr.columns, 5, "0x5 keeps its column count"
eq zr.capacity, 0, "0x5 allocates nothing"

print
print "-- shape inspectors --"
s34 = Matrix.ofSize(3, 4)
eq s34.size.len, 2, "size is a 2-element list"
eq s34.size[0], 3, "size[0] is rows"
eq s34.size[1], 4, "size[1] is columns"
eq s34.size[0], s34.rows, "size agrees with .rows"
eq s34.size[1], s34.columns, "size agrees with .columns"
eq Matrix.ofSize(0, 5).size[1], 5, "size works on a zero-row matrix"

print
print "-- negative indices count from the end --"
n = Matrix.ofSize(2, 3)
n.setElem -1, -1, 99
eq n.getElem(1, 2), 99, "(-1,-1) is the bottom-right element"
eq n.getElem(-1, -1), 99, "...and reads back the same way"
n.setElem 0, -3, 5
eq n.getElem(0, 0), 5, "-3 of 3 columns is column 0"
eq n.getElem(-2, 0), 5, "-2 of 2 rows is row 0"

print
print "-- errors are values, not crashes --"
ok isErr(m.getElem(3, 0)), "row past the end is an error"
ok isErr(m.getElem(0, 4)), "column past the end is an error"
ok isErr(z.getElem(0, 0)), "any index into a 0x0 is an error"
ok isErr(z.getElem(-1, -1)), "...including a negative one"
ok isErr(m.getElem(-4, 0)), "row too negative is an error"
ok isErr(m.getElem(0, -5)), "column too negative is an error"
ok isErr(m.setElem(9, 9, 1)), "setElem bounds-checks too"
ok isErr(m.getElem("x", 0)), "non-numeric index is an error"
ok isErr(m.setElem(0, 0, "x")), "non-numeric value is an error"
ok isErr(Matrix.ofSize(-1, 3)), "negative size is an error"
ok isErr(Matrix.ofSize(3, 3, @print)), "function initialValue is refused, not ignored"
ok isErr(Matrix.ofSize(3, 3, null)), "explicit null initialValue is refused (0 is the default)"
ok isErr(Matrix.ofSize(100000, 100000)), "oversized request is an error, not an OOM"

// The whole point of the null handle on the class.
print
print "-- the null-handle rule --"
bare = new Matrix
ok isErr(bare.getElem(0, 0)), "`new Matrix` has no storage and says so"
ok isErr(bare.capacity), "...for every accessor"

print
print "-- handles are per-instance --"
a = Matrix.ofSize(2, 2)
b = Matrix.ofSize(2, 2)
a.setElem 0, 0, 111
eq b.getElem(0, 0), 0, "instances do not share a buffer"
eq a.getElem(0, 0), 111, "...and the write did land"

// A map-level copy shares the handle: same storage, deliberately.
c = a
c.setElem 1, 1, 222
eq a.getElem(1, 1), 222, "assignment shares the handle (no deep copy)"

print
print "-- fromList / toFlatList --"
matches Matrix.fromList([1,2,3]), 1, 3, [1,2,3], "1D list becomes a row vector"
matches Matrix.fromList([[1,2],[3,4]]), 2, 2, [1,2,3,4], "2D list becomes n x m, row-major"
matches Matrix.fromList([[1,2,3]]), 1, 3, [1,2,3], "a single nested row is 1 x n"
matches Matrix.fromList([]), 0, 0, [], "empty list is a clean 0x0"
matches Matrix.fromList([[],[]]), 2, 0, [], "list of empty lists is n x 0"
eq Matrix.ofSize(2,2,3).toFlatList.len, 4, "toFlatList length is rows*columns"
ok isErr(Matrix.fromList([[1,2],[3]])), "ragged rows are an error"
ok isErr(Matrix.fromList([[1,2],3])), "mixed list/non-list rows are an error"
ok isErr(Matrix.fromList([1,"x"])), "non-numeric element is an error"
ok isErr(Matrix.fromList(3)), "a non-list is an error"

print
print "-- reshape (O(1), same elements) --"
r = Matrix.fromList([[1,2,3],[4,5,6]])
r.reshape 3, 2
matches r, 3, 2, [1,2,3,4,5,6], "2x3 -> 3x2 keeps row-major order"
eq r.getElem(0,0), 1, "reshape is not a transpose: (0,0) is still 1"
eq r.getElem(0,1), 2, "...and (0,1) is 2, not 4"
r.reshape 1, null
matches r, 1, 6, [1,2,3,4,5,6], "null infers columns"
r.reshape null, 2
matches r, 3, 2, [1,2,3,4,5,6], "null infers rows"
ok isErr(r.reshape(4, 4)), "a shape with a different element count is an error"
ok isErr(r.reshape(null, null)), "both dimensions null is an error"
ok isErr(r.reshape(null, 4)), "a non-divisible inference is an error"
matches r, 3, 2, [1,2,3,4,5,6], "a failed reshape leaves the matrix alone"

print
print "-- resize (preserve overlap, zero-fill growth) --"
g = Matrix.fromList([[1,2],[3,4]])
g.resize 3, 2
matches g, 3, 2, [1,2,3,4,0,0], "growing rows zero-fills the new row"
g.resize 2, 2
matches g, 2, 2, [1,2,3,4], "shrinking rows keeps the top-left block"
g.resize 3, 2
matches g, 3, 2, [1,2,3,4,0,0], "shrink-then-grow re-zeroes, not resurrects"

w = Matrix.fromList([[1,2],[3,4]])
w.resize 2, 3
matches w, 2, 3, [1,2,0,3,4,0], "growing columns re-lays out every row"
w.resize 2, 1
matches w, 2, 1, [1,3], "shrinking columns keeps the left block"
w.resize null, 2
matches w, 2, 2, [1,0,3,0], "null keeps a dimension unchanged"
z2 = Matrix.fromList([[1,2],[3,4]])
z2.resize 0, 2
matches z2, 0, 2, [], "resizing to zero rows empties it"
ok isErr(g.resize(-1, 2)), "a negative size is an error"

print
print "-- reserve (capacity without a visible change) --"
p = Matrix.ofSize(2, 3, 9)
before = p.capacity
p.reserve 100
ok p.capacity >= 300, "reserve grows capacity  (now " + p.capacity + ")"
matches p, 2, 3, [9,9,9,9,9,9], "reserve changes nothing visible"
capAfter = p.capacity
p.resize 100, 3
eq p.capacity, capAfter, "growing into reserved capacity does not reallocate"
eq p.getElem(99, 2), 0, "...and the new rows are zeroed"
eq p.getElem(0, 0), 9, "...and the old rows survived"
p.reserve 1
eq p.capacity, capAfter, "reserve never shrinks capacity"
ok isErr(p.reserve(-1)), "a negative reserve is an error"

print
print "-- getRow / getColumn --"
rc = Matrix.fromList([[1,2,3],[4,5,6]])
eq rc.getRow(0), [1,2,3], "getRow reads a row, contiguously"
eq rc.getRow(1), [4,5,6], "...and the next one"
eq rc.getRow(-1), [4,5,6], "negative row index counts from the end"
eq rc.getColumn(0), [1,4], "getColumn reads a strided column"
eq rc.getColumn(2), [3,6], "...at the far edge"
eq rc.getColumn(-1), [3,6], "negative column index counts from the end"
// getRow returns a plain list, deliberately unlike getSub which returns a Matrix.
ok rc.getRow(0) isa list, "getRow returns a plain list, not a Matrix"
ok rc.getColumn(0) isa list, "getColumn returns a plain list too"
eq Matrix.ofSize(2,0).getRow(0), [], "a row of a zero-column matrix is empty"
ok isErr(rc.getRow(2)), "a row past the end is an error"
ok isErr(rc.getColumn(3)), "a column past the end is an error"
ok isErr(Matrix.ofSize(0,3).getRow(0)), "any row of a zero-row matrix is an error"

print
print "-- setRow / setColumn --"
sc = Matrix.ofSize(2, 3)
sc.setRow 0, [1,2,3]
matches sc, 2, 3, [1,2,3,0,0,0], "setRow writes one row"
sc.setRow -1, [4,5,6]
matches sc, 2, 3, [1,2,3,4,5,6], "negative row index writes from the end"
sc.setColumn 1, [9,9]
matches sc, 2, 3, [1,9,3,4,9,6], "setColumn writes a strided column"
eq sc.getRow(0), [1,9,3], "...and getRow agrees afterward"
ok isErr(sc.setRow(0, [1,2])), "a short row is an error, not a partial write"
ok isErr(sc.setRow(0, [1,2,3,4])), "a long row is an error too"
ok isErr(sc.setColumn(0, [1])), "a wrong-length column is an error"
ok isErr(sc.setRow(0, [1,"x",3])), "a non-numeric element is an error"
matches sc, 2, 3, [1,9,3,4,9,6], "every refused write left the matrix untouched"
// Scalar broadcast, as numpy does for `a[0] = 5`.
bc = Matrix.ofSize(2, 3)
bc.setRow 0, 7
matches bc, 2, 3, [7,7,7,0,0,0], "setRow broadcasts a scalar across the row"
bc.setColumn 2, 4
matches bc, 2, 3, [7,7,4,0,0,4], "setColumn broadcasts a scalar down the column"
bc.setRow 1, 0
matches bc, 2, 3, [7,7,4,0,0,0], "broadcasting zero works too"
ok isErr(bc.setRow(0, "x")), "a non-numeric scalar is still an error"
ok isErr(bc.setRow(0, [1,2])), "a wrong-length list is still an error"

print
print "-- getSub --"
b = Matrix.fromList([[1,2,3,4],[5,6,7,8],[9,10,11,12]])
matches b.getSub(0,0,3,3), 3, 3, [1,2,3,5,6,7,9,10,11], "the 3x3 block out of a 3x4"
matches b.getSub(1,1,2,2), 2, 2, [6,7,10,11], "an interior block"
matches b.getSub(1,0), 2, 4, [5,6,7,8,9,10,11,12], "null counts run through the end"
matches b.getSub(0,2), 3, 2, [3,4,7,8,11,12], "...in both dimensions"
matches b.getSub(), 3, 4, [1,2,3,4,5,6,7,8,9,10,11,12], "no arguments copies the whole thing"
matches b.getSub(-1,0), 1, 4, [9,10,11,12], "a negative offset counts from the end"
matches b.getSub(0,0,0,0), 0, 0, [], "a zero-size block is a clean 0x0"
matches Matrix.ofSize(0,0).getSub(), 0, 0, [], "getSub on a 0x0 works"
// The copy must be independent: this is the "no views" rule.
sub = b.getSub(0,0,2,2)
sub.setElem 0, 0, 999
eq b.getElem(0,0), 1, "getSub returns a copy, not a view"
ok isErr(b.getSub(0,0,4,4)), "a block that overruns is an error, not a clip"
ok isErr(b.getSub(5,0)), "an offset past the end is an error"

print
print "-- setSub --"
d = Matrix.ofSize(3, 4)
d.setSub 0, 0, Matrix.fromList([[1,2],[3,4]])
matches d, 3, 4, [1,2,0,0,3,4,0,0,0,0,0,0], "a block lands at the origin"
d.setSub 1, 2, Matrix.fromList([[7,8],[9,10]])
matches d, 3, 4, [1,2,0,0,3,4,7,8,0,0,9,10], "...and at an offset"
d.setSub -1, 0, Matrix.fromList([[5,5,5,5]])
matches d, 3, 4, [1,2,0,0,3,4,7,8,5,5,5,5], "a negative offset counts from the end"
ok isErr(d.setSub(2, 0, Matrix.fromList([[1,1],[2,2]]))), "a block that does not fit is an error"
ok isErr(d.setSub(0, 3, Matrix.fromList([[1,1]]))), "...horizontally too"
ok isErr(d.setSub(0, 0, [[1,2],[3,4]])), "a plain list is not accepted as a block"
matches d, 3, 4, [1,2,0,0,3,4,7,8,5,5,5,5], "a refused setSub changes nothing"

// The motivating round trip from the design notes: pull the rotation block out
// of a 4x4 transform, then assemble a new 4x4 from it.
xf = Matrix.fromList([[1,2,3,10],[4,5,6,11],[7,8,9,12],[0,0,0,1]])
rot = xf.getSub(0,0,3,3)
matches rot, 3, 3, [1,2,3,4,5,6,7,8,9], "rotation block extracted from a 4x4"
rebuilt = Matrix.ofSize(4,4)
rebuilt.setSub 0, 0, rot
rebuilt.setSub 0, 3, xf.getSub(0,3,3,1)
rebuilt.setElem 3, 3, 1
matches rebuilt, 4, 4, [1,2,3,10,4,5,6,11,7,8,9,12,0,0,0,1], "4x4 reassembled from block plus column"

// Aliasing: self as its own source must not read what it already wrote.
al = Matrix.fromList([[1,2],[3,4]])
al.setSub 0, 0, al
matches al, 2, 2, [1,2,3,4], "setSub with self as source is a no-op, not corruption"

print
print "-- equals --"
e1 = Matrix.fromList([[1,2],[3,4]])
e2 = Matrix.fromList([[1,2],[3,4]])
ok e1.equals(e2), "identical contents compare equal"
ok not (e1 == e2), "...even though == compares maps and says otherwise"
ok e1.equals(e1), "a matrix equals itself"
ok e1.equals([[1,2],[3,4]]), "a 2D list operand works"
ok Matrix.fromList([1,2,3]).equals([1,2,3]), "a 1D list means what fromList would make of it"
ok not e1.equals(Matrix.fromList([[1,2],[3,5]])), "a differing element compares unequal"
ok not e1.equals(Matrix.fromList([[1,2,0],[3,4,0]])), "a differing shape compares unequal"
ok not e1.equals([[1,2]]), "...for lists too"
ok Matrix.ofSize(0,0).equals(Matrix.ofSize(0,0)), "two empty matrices are equal"
// Tolerance.
t1 = Matrix.fromList([1.0])
t2 = Matrix.fromList([1.0 + 1e-12])
ok t1.equals(t2), "a tiny difference is within the default tolerance"
ok not t1.equals(Matrix.fromList([1.1])), "a large difference is not"
ok t1.equals(Matrix.fromList([1.1]), 0.5), "...unless the tolerance says so"
ok not t1.equals(t2, 0), "a zero tolerance demands exactness"
ok isErr(e1.equals(3)), "a non-Matrix, non-list operand is an error"
ok isErr(e1.equals([[1,2],[3]])), "a ragged list operand is an error"

print
print "-- trim --"
tm = Matrix.ofSize(2, 3, 1)
tm.reserve 100
ok tm.capacity >= 300, "reserved capacity is there  (" + tm.capacity + ")"
tm.trim
eq tm.capacity, 6, "trim drops capacity to the live size"
matches tm, 2, 3, [1,1,1,1,1,1], "trim changes nothing visible"
tm.trim
eq tm.capacity, 6, "trim again is a no-op"
tz = Matrix.ofSize(0, 0)
tz.trim
eq tz.capacity, 0, "trimming an empty matrix is fine"

print
print "-- removeRow (order-preserving) --"
rr = Matrix.fromList([[1,1],[2,2],[3,3],[4,4]])
rr.removeRow 1
matches rr, 3, 2, [1,1,3,3,4,4], "the row is gone and order is kept"
rr.removeRow -1
matches rr, 2, 2, [1,1,3,3], "a negative index removes from the end"
rr.removeRow 0
matches rr, 1, 2, [3,3], "removing the first row slides the rest up"
rr.removeRow 0
matches rr, 0, 2, [], "removing the last row leaves 0 rows"
ok isErr(rr.removeRow(0)), "removing from an empty matrix is an error"

// Removing must not resurrect stale rows when the matrix grows again.
rg = Matrix.fromList([[1,1],[2,2],[3,3]])
rg.removeRow 0
rg.resize 3, 2
matches rg, 3, 2, [2,2,3,3,0,0], "growing after a remove zero-fills, not resurrects"

print
print "-- removeRowFast (reorders) --"
rf = Matrix.fromList([[1,1],[2,2],[3,3],[4,4]])
rf.removeRowFast 0
matches rf, 3, 2, [4,4,2,2,3,3], "the last row is moved into the hole"
rf.removeRowFast -1
matches rf, 2, 2, [4,4,2,2], "removing the last row needs no move"
rf2 = Matrix.fromList([[1,1]])
rf2.removeRowFast 0
matches rf2, 0, 2, [], "removing the only row works"
ok isErr(rf2.removeRowFast(0)), "removing from an empty matrix is an error"

print
print "-- storage is reclaimed --"
before = hostMemory
big = []
for i in range(1, 20)
	big.push Matrix.ofSize(256, 256)    // 512KB each, 10MB total
end for
grew = hostMemory - before
ok grew > 8000000, "20 x 256x256 allocated about 10MB  (grew " + grew + ")"
big = []
gc.collect true
held = hostMemory - before
ok held < grew / 4, "dropping them returns most of it  (held " + held + ")"

print
print checks + " checks, " + failures + " failures"
