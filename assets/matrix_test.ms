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
print "-- gemm: plain matrix product --"
A23 = Matrix.fromList([[1,2,3],[4,5,6]])
B34 = Matrix.fromList([[1,0,0,1],[0,1,0,2],[0,0,1,3]])
matches A23.gemm(B34), 2, 4, [1,2,3,14,4,5,6,32], "2x3 times 3x4"
I3 = Matrix.fromList([[1,0,0],[0,1,0],[0,0,1]])
ok A23.gemm(I3).equals(A23), "times identity is a no-op"
matches Matrix.fromList([[1,2],[3,4]]).gemm(Matrix.fromList([[5,6],[7,8]])), 2, 2, [19,22,43,50], "the classic 2x2"
// Vector shapes.
matches Matrix.fromList([[1,2,3]]).gemm(Matrix.fromList([[4],[5],[6]])), 1, 1, [32], "row times column is a dot product"
matches Matrix.fromList([[1],[2]]).gemm(Matrix.fromList([[3,4]])), 2, 2, [3,4,6,8], "column times row is an outer product"
ok isErr(A23.gemm(A23)), "mismatched inner dimensions are an error"

print
print "-- gemm: transpose flags (all four combinations) --"
P = Matrix.fromList([[1,2],[3,4],[5,6]])      // 3x2
Q = Matrix.fromList([[1,2,3],[4,5,6]])        // 2x3
// NN: 3x2 * 2x3 -> 3x3
matches P.gemm(Q), 3, 3, [9,12,15,19,26,33,29,40,51], "NN"
// TN: Pt(2x3) * P(3x2) -> 2x2
matches P.gemm(P, null, null, true, false), 2, 2, [35,44,44,56], "TN: transposedTimes"
// NT: Q(2x3) * Qt(3x2) -> 2x2
matches Q.gemm(Q, null, null, false, true), 2, 2, [14,32,32,77], "NT: timesTransposed"
// TT: Pt(2x3) * Qt(3x2) -> 2x2 ; equals (Q*P)t
QP = Q.gemm(P)
matches P.gemm(Q, null, null, true, true), 2, 2, [QP.getElem(0,0),QP.getElem(1,0),QP.getElem(0,1),QP.getElem(1,1)], "TT equals (Q*P) transposed"

print
print "-- gemm: null B is the level-1 path --"
X = Matrix.fromList([[1,2],[3,4]])
matches X.gemm(null), 2, 2, [1,2,3,4], "clone: gemm(null)"
ok X.gemm(null) != X, "...returns a distinct matrix"
matches X.gemm(null, null, null, true), 2, 2, [1,3,2,4], "transposed: gemm(null,...,transA)"
R32 = Matrix.fromList([[1,2,3],[4,5,6]]).gemm(null, null, null, true)
matches R32, 3, 2, [1,4,2,5,3,6], "transpose of a non-square matrix"
matches X.gemm(null, null, null, false, false, -1), 2, 2, [-1,-2,-3,-4], "negated: alpha = -1"
matches X.gemm(null, null, null, false, false, 2), 2, 2, [2,4,6,8], "scaled: alpha = 2"
matches X.gemm(null, X), 2, 2, [2,4,6,8], "plus: A + A"
matches X.gemm(null, X, null, false, false, 1, -1), 2, 2, [0,0,0,0], "minus: A - A"
matches X.gemm(null, X, null, false, false, 1, 3), 2, 2, [4,8,12,16], "addScaled: A + 3A"

print
print "-- gemm: addend forms and broadcasting --"
C22 = Matrix.fromList([[10,20],[30,40]])
matches X.gemm(null, C22), 2, 2, [11,22,33,44], "a full m x n addend"
matches X.gemm(null, 100), 2, 2, [101,102,103,104], "a scalar addend"
matches X.gemm(null, Matrix.fromList([[10,20]])), 2, 2, [11,22,13,24], "a 1 x n addend broadcasts down the rows"
matches X.gemm(null, Matrix.fromList([[10],[20]])), 2, 2, [11,12,23,24], "an m x 1 addend broadcasts across the columns"
matches X.gemm(null, Matrix.fromList([[7]])), 2, 2, [8,9,10,11], "a 1 x 1 addend acts as a scalar"
matches X.gemm(null, [[10,20],[30,40]]), 2, 2, [11,22,33,44], "a 2D list addend"
matches X.gemm(null, [10,20]), 2, 2, [11,22,13,24], "a 1D list addend broadcasts as a row"
ok isErr(X.gemm(null, Matrix.fromList([[1,2,3]]))), "an addend that cannot broadcast is an error"
// beta must not read the addend when it is zero.
matches X.gemm(null, C22, null, false, false, 1, 0), 2, 2, [1,2,3,4], "beta = 0 ignores the addend entirely"

print
print "-- gemm: the dense NN layer, in one call --"
// y = x*W + b, with b broadcast over the batch.
xb = Matrix.fromList([[1,2],[3,4],[5,6]])     // batch 3, 2 features
W = Matrix.fromList([[1,0,2],[0,1,3]])        // 2 -> 3
bias = Matrix.fromList([[10,20,30]])          // 1 x 3
matches xb.gemm(W, bias), 3, 3, [11,22,38,13,24,48,15,26,58], "x*W + b in one kernel pass"

print
print "-- gemm: the out parameter --"
dest = Matrix.ofSize(1, 1)
res = X.gemm(null, null, dest)
ok refEquals(res, dest), "gemm returns the same map it was given as out"
matches dest, 2, 2, [1,2,3,4], "out is resized to fit the result"
eq dest.rows, 2, "...and its rows entry is updated"
eq dest.columns, 2, "...and its columns entry too"
big = Matrix.ofSize(10, 10)
cap = big.capacity
X.gemm null, null, big
matches big, 2, 2, [1,2,3,4], "out shrinks to the result shape"
eq big.capacity, cap, "...without giving back capacity"
ok isErr(X.gemm(null, null, 5)), "a non-Matrix out is an error"

print
print "-- gemm: aliasing --"
// out aliases the addend: the accumulate case.
acc = Matrix.fromList([[1,1],[1,1]])
X.gemm null, acc, acc
matches acc, 2, 2, [2,3,4,5], "out == addend accumulates in place"
// out aliases A.
sa = Matrix.fromList([[1,2],[3,4]])
sa.gemm null, null, sa, false, false, 3
matches sa, 2, 2, [3,6,9,12], "out == A scales in place"
// out aliases A with a transpose, which also changes the shape.
st = Matrix.fromList([[1,2,3],[4,5,6]])
st.gemm null, null, st, true
matches st, 3, 2, [1,4,2,5,3,6], "out == A transposes in place"
// out aliases A in a real product.
sq = Matrix.fromList([[1,2],[3,4]])
sq.gemm sq, null, sq
matches sq, 2, 2, [7,10,15,22], "out == A == B squares in place"
// out aliases B only.
lb = Matrix.fromList([[1,2],[3,4]])
Matrix.fromList([[2,0],[0,2]]).gemm lb, null, lb
matches lb, 2, 2, [2,4,6,8], "out == B works too"
// addend aliases A (read-only, no copy needed).
ra = Matrix.fromList([[1,2],[3,4]])
matches ra.gemm(null, ra), 2, 2, [2,4,6,8], "addend == A is fine"

print
print "-- gemm: empty and degenerate shapes --"
matches Matrix.ofSize(0,3).gemm(Matrix.ofSize(3,4)), 0, 4, [], "zero rows gives a zero-row result"
matches Matrix.ofSize(2,0).gemm(Matrix.ofSize(0,3)), 2, 3, [0,0,0,0,0,0], "a zero-length inner dimension sums to zero"
matches Matrix.ofSize(2,0).gemm(Matrix.ofSize(0,3), 5), 2, 3, [5,5,5,5,5,5], "...and still takes the addend"
matches Matrix.ofSize(0,0).gemm(null), 0, 0, [], "gemm on a 0x0 works"

print
print "-- gemm: consistency between paths --"
// A*B computed three ways must agree.
G = Matrix.fromList([[1,2,3],[4,5,6],[7,8,9]])
H = Matrix.fromList([[9,8,7],[6,5,4],[3,2,1]])
direct = G.gemm(H)
viaT = G.gemm(H.gemm(null, null, null, true), null, null, false, true)
ok direct.equals(viaT), "A*B == A*(Bt)t via the NT path"
Gt = G.gemm(null, null, null, true)
viaTN = Gt.gemm(H, null, null, true)
ok direct.equals(viaTN), "A*B == (At)t*B via the TN path"

print
print "-- elementwise --"
E = Matrix.fromList([[1,2],[3,4]])
E.elemMultiplyBy 2
matches E, 2, 2, [2,4,6,8], "elemMultiplyBy a scalar"
E.elemDivideBy 2
matches E, 2, 2, [1,2,3,4], "elemDivideBy a scalar"
E.elemMultiplyBy Matrix.fromList([[10,100],[1000,10000]])
matches E, 2, 2, [10,200,3000,40000], "elemMultiplyBy a Matrix is the Hadamard product"
H = Matrix.fromList([[1,2],[3,4]])
H.elemMultiplyBy [[2,3],[4,5]]
matches H, 2, 2, [2,6,12,20], "a 2D list operand works"
// Broadcasting, same rules as gemm's addend.
B1 = Matrix.fromList([[1,2,3],[4,5,6]])
B1.elemMultiplyBy Matrix.fromList([[10,20,30]])
matches B1, 2, 3, [10,40,90,40,100,180], "a 1 x n operand broadcasts down the rows"
B2 = Matrix.fromList([[1,2,3],[4,5,6]])
B2.elemMultiplyBy Matrix.fromList([[10],[100]])
matches B2, 2, 3, [10,20,30,400,500,600], "an m x 1 operand broadcasts across the columns"
ok isErr(B2.elemMultiplyBy(Matrix.fromList([[1,2]]))), "an operand that cannot broadcast is an error"
ok isErr(B2.elemMultiplyBy("x")), "a non-numeric operand is an error"
// Division by zero follows IEEE rather than erroring.
dz = Matrix.fromList([[1,0]])
dz.elemDivideBy 0
ok dz.getElem(0,0) > 1e300, "1/0 is +inf, not an error"
// MiniScript follows IEEE here: NaN is the one value not equal to itself.
isNaN = function(v)
	return v != v
end function
ok isNaN(dz.getElem(0,1)), "0/0 is NaN"

print
print "-- pow / abs / sqrt --"
P2 = Matrix.fromList([[2,3],[4,5]])
P2.pow 2
matches P2, 2, 2, [4,9,16,25], "pow squares"
P2.sqrt
matches P2, 2, 2, [2,3,4,5], "sqrt undoes it"
AB = Matrix.fromList([[-1,2],[-3,-4]])
AB.abs
matches AB, 2, 2, [1,2,3,4], "abs"
PB = Matrix.fromList([[2,2],[2,2]])
PB.pow Matrix.fromList([[1,2],[3,4]])
matches PB, 2, 2, [2,4,8,16], "pow broadcasts an exponent matrix"
ng = Matrix.fromList([[-4]])
ng.sqrt
ok isNaN(ng.getElem(0,0)), "sqrt of a negative is NaN, as in numpy"

print
print "-- round --"
R1 = Matrix.fromList([[1.234, 5.678],[-1.235, -5.6]])
R1.round 2
matches R1, 2, 2, [1.23, 5.68, -1.24, -5.6], "round to 2 places"
R2 = Matrix.fromList([[1.5, 2.5],[-1.5, -2.5]])
R2.round
// MiniScript rounds half away from zero; numpy would give 2 and 2 here.
matches R2, 2, 2, [2, 3, -2, -3], "half rounds away from zero, matching MiniScript's round()"
eq round(2.5), 3, "...which is what the language's own round() does"
R3 = Matrix.fromList([[1234, 5678]])
R3.round -2
matches R3, 1, 2, [1200, 5700], "negative places round to tens/hundreds"

print
print "-- clamp --"
C1 = Matrix.fromList([[-5,0],[5,10]])
C1.clamp 0, 6
matches C1, 2, 2, [0,0,5,6], "clamp both ends"
C2 = Matrix.fromList([[-5,0],[5,10]])
C2.clamp 0, null
matches C2, 2, 2, [0,0,5,10], "null hi clamps from below only (this is relu)"
C3 = Matrix.fromList([[-5,0],[5,10]])
C3.clamp null, 1
matches C3, 2, 2, [-5,0,1,1], "null lo clamps from above only"
ok isErr(Matrix.fromList([[1]]).clamp(5, 1)), "lo above hi is an error"

print
print "-- reductions: whole matrix --"
S = Matrix.fromList([[1,2,3],[4,5,6]])
eq S.sum, 21, "sum of everything"
eq S.sumOfSquares, 91, "sum of squares"
eq S.max, 6, "max"
eq S.min, 1, "min"
eq S.argmax, 5, "argmax is a FLAT index, as in numpy"
eq S.argmin, 0, "argmin likewise"
eq Matrix.ofSize(0,0).sum, 0, "sum of an empty matrix is 0"
eq Matrix.ofSize(0,0).sumOfSquares, 0, "...and so is sumOfSquares"
ok isErr(Matrix.ofSize(0,0).max), "max of an empty matrix is an error"
ok isErr(Matrix.ofSize(0,0).argmax), "...and so is argmax"

print
print "-- reductions: by axis --"
matches S.sum(0), 1, 3, [5,7,9], "sum(0) reduces down the columns to 1 x n"
matches S.sum(1), 2, 1, [6,15], "sum(1) reduces across the rows to m x 1"
matches S.max(0), 1, 3, [4,5,6], "max(0)"
matches S.max(1), 2, 1, [3,6], "max(1)"
matches S.min(0), 1, 3, [1,2,3], "min(0)"
matches S.min(1), 2, 1, [1,4], "min(1)"
matches S.argmax(0), 1, 3, [1,1,1], "argmax(0) gives row indices"
matches S.argmax(1), 2, 1, [2,2], "argmax(1) gives column indices"
matches S.sumOfSquares(1), 2, 1, [14,77], "sumOfSquares by row"
ok isErr(S.sum(2)), "an axis other than 0, 1 or null is an error"
ok isErr(S.sum("x")), "a non-numeric axis is an error"

print
print "-- reductions: ties and NaN --"
T = Matrix.fromList([[1,5,5,2]])
eq T.argmax, 1, "argmax breaks ties toward the first, as numpy does"
T2 = Matrix.fromList([[3,1,1,4]])
eq T2.argmin, 1, "argmin likewise"
N = Matrix.fromList([[0,2],[3,4]])
N.elemDivideBy Matrix.fromList([[0,1],[1,1]])    // 0/0 at (0,0) is NaN
ok isNaN(N.getElem(0,0)), "the test matrix really does hold a NaN"
ok isNaN(N.max), "a NaN makes max NaN, as in numpy (nanmax is separate there)"
ok isNaN(N.min), "...and min too"
ok not isNaN(N.getElem(0,1)), "a non-NaN element is not reported as NaN"
eq N.argmax, 0, "argmax reports the first NaN's index, as in numpy"

print
print "-- reductions: degenerate shapes --"
Z1 = Matrix.ofSize(0, 3)
matches Z1.sum(0), 1, 3, [0,0,0], "sum down a zero-length axis is zeros"
ok isErr(Z1.max(0)), "max down a zero-length axis is an error"
matches Matrix.ofSize(2,0).sum(1), 2, 1, [0,0], "sum across a zero-length axis is zeros"

print
print "-- format --"
F = Matrix.fromList([[1,2],[3,4]])
ok F.format isa string, "format returns a string rather than printing"
eq F.format(0, 0, ",", ";"), "1,2;3,4", "no padding, explicit separators"
eq F.format(3, 0, "", ";"), "  1  2;  3  4", "fieldWidth right-aligns"
eq F.format(0, 2, ",", ";"), "1.00,2.00;3.00,4.00", "precision gives fixed-point"
eq F.format(0, 0, "", "\n"), "12\n34", "an explicit newline rowSep"
eq Matrix.fromList([[1.5]]).format(0, 0, "", ""), "2", "precision 0 rounds"
// Default rowSep is a newline; check by splitting.
eq F.format(0, 0, ",").split(char(10)).len, 2, "the default rowSep is a newline"
eq F.format(0, 0, ",").split(char(10))[1], "3,4", "...and rows come out in order"

print
print "-- format: small values (the old E- hack is gone) --"
tiny = Matrix.fromList([[0.000000000000001, 1.5]])
// Without a precision the true magnitude is reported, not silently zeroed.
ok tiny.format(0).indexOf("E-") != null, "without precision, a tiny value keeps its exponent"
// With a precision, fixed-point renders it as zero to that many places, which
// is the honest answer rather than a string-matching hack.
eq tiny.format(0, 3, ",", ""), "0.000,1.500", "with precision, a tiny value renders as 0.000"
// The old hack would have zeroed this legitimately small value too.
small = Matrix.fromList([[0.000015]])
eq small.format(0, 6, "", ""), "0.000015", "a legitimately small value survives"

print
print "-- format: special values --"
sp = Matrix.fromList([[1,2],[3,4]])
sp.setElem 0, 0, 0
sp.elemDivideBy Matrix.fromList([[0,1],[0,1]])
eq sp.format(0, 1, ",", ";").split(";")[0].split(",")[0], "NaN", "NaN is spelled the way MiniScript spells it"
eq sp.format(0, 1, ",", ";").split(";")[1].split(",")[0], "Inf", "and so is Inf"
neg = Matrix.fromList([[-1]])
neg.elemDivideBy 0
eq neg.format(0, 1, "", ""), "-Inf", "negative infinity too"

print
print "-- format: edges and errors --"
eq Matrix.ofSize(0,0).format, "", "a 0x0 formats as the empty string"
eq Matrix.ofSize(2,0).format(0,0,"",";"), ";", "a zero-column matrix is just the row separators"
eq Matrix.fromList([[1]]).format(4, 0, "", ""), "   1", "single element padded to fieldWidth"
// A value that would fill or overflow the field is truncated to fieldWidth-1
// and then padded back out, so there is always at least one space between
// columns even with the default empty columnSep.  Only values with a fractional
// part are ever truncated, so an integer magnitude is never falsified.
wide = Matrix.fromList([[123.456789, 1]])
r = wide.format(6, 4, "", "")
eq r.len, 12, "each field occupies exactly fieldWidth characters"
eq r[0:6], " 123.4", "a too-wide value truncates and keeps a separating space"
eq r[6:12], " 1.000", "...as does one that would exactly fill the field"
// Integers are never truncated, however wide.
eq Matrix.fromList([[1234567]]).format(4, 0, "", ""), "1234567", "an integer is never truncated"
ok isErr(F.format(-1)), "a negative fieldWidth is an error"
ok isErr(F.format(10, -1)), "a negative precision is an error"
ok isErr(F.format(10, 20)), "a precision above 15 is an error"
ok isErr(F.format("x")), "a non-numeric fieldWidth is an error"

print
print "-- str(m) no longer dumps the class --"
s = str(Matrix.ofSize(2,3))
ok s.indexOf("Matrix") != null, "str names the class"
ok s.indexOf("getElem") == null, "...without listing every method"
ok s.len < 100, "...and stays short  (" + s.len + " chars)"

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
