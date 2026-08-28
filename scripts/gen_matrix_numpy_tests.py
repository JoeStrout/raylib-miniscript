#!/usr/bin/env python3
"""Generate assets/matrix_numpy_test.ms from numpy-computed expected values.

numpy is used as an INDEPENDENT oracle.  The hand-written checks in
assets/matrix_test.ms and any reference loop written in MiniScript share their
author's assumptions with the kernel under test -- if the row-major indexing or
the meaning of a transpose flag were misunderstood, both would be wrong the same
way and would agree.  numpy was written by other people from the same maths.

The generated file has the expected values baked in, so running the tests needs
no Python.  Regenerate with:

    python3 scripts/gen_matrix_numpy_tests.py
"""
import numpy as np

rng = np.random.default_rng(20260828)
OUT = "assets/matrix_numpy_test.ms"
lines, checks = [], 0


def num(x):
    x = float(x)
    if x != x or x in (float("inf"), float("-inf")):
        raise ValueError("non-finite value in fixture: %r" % x)
    return repr(x)


def lit(a):
    """A numpy 2-D array as a MiniScript nested-list literal."""
    a = np.atleast_2d(a)
    return "[" + ",".join("[" + ",".join(num(v) for v in row) + "]" for row in a) + "]"


def flat(a):
    return "[" + ",".join(num(v) for v in np.atleast_2d(a).ravel()) + "]"


def emit(label, expr, expected):
    global checks
    checks += 1
    e = np.atleast_2d(expected)
    lines.append('cmp %s, %d, %d, %s, "%s"' % (expr, e.shape[0], e.shape[1], flat(e), label))


def rand(r, c, lo=-5, hi=5):
    return np.round(rng.uniform(lo, hi, size=(r, c)), 4)


lines.append("// GENERATED FILE -- do not edit by hand.")
lines.append("// Expected values computed by numpy; see scripts/gen_matrix_numpy_tests.py")
lines.append("// Regenerate:  python3 scripts/gen_matrix_numpy_tests.py")
lines.append("")
lines.append("failures = 0")
lines.append("checks = 0")
lines.append("""
cmp = function(m, r, c, want, label)
	outer.checks = outer.checks + 1
	bad = null
	if m isa error then
		bad = "error: " + m
	else if m.rows != r or m.columns != c then
		bad = "shape " + m.rows + "x" + m.columns + ", want " + r + "x" + c
	else
		got = m.toFlatList
		for i in indexes(want)
			d = got[i] - want[i]
			if d < 0 then d = -d
			if d > 1e-9 then
				bad = "element " + i + ": got " + got[i] + ", want " + want[i]
				break
			end if
		end for
	end if
	if bad == null then
		print "ok   " + label
	else
		outer.failures = outer.failures + 1
		print "FAIL " + label + "  (" + bad + ")"
	end if
end function
""")

# ---- products, across shapes that are not square and not equal ----
lines.append('print "-- numpy oracle: matrix product --"')
for (m, k, n) in [(3, 4, 2), (1, 5, 1), (7, 1, 6), (12, 9, 7), (2, 3, 9), (16, 16, 16)]:
    A, B = rand(m, k), rand(k, n)
    emit("%dx%d * %dx%d" % (m, k, k, n),
         "Matrix.fromList(%s).gemm(Matrix.fromList(%s))" % (lit(A), lit(B)), A @ B)

# ---- all four transpose combinations ----
lines.append("")
lines.append('print "-- numpy oracle: transpose flags --"')
for (m, k, n) in [(4, 3, 5), (9, 7, 2)]:
    A, B = rand(m, k), rand(k, n)
    At, Bt = A.T.copy(), B.T.copy()
    emit("NN %dx%dx%d" % (m, k, n),
         "Matrix.fromList(%s).gemm(Matrix.fromList(%s))" % (lit(A), lit(B)), A @ B)
    emit("TN %dx%dx%d" % (m, k, n),
         "Matrix.fromList(%s).gemm(Matrix.fromList(%s), null, null, true)" % (lit(At), lit(B)),
         At.T @ B)
    emit("NT %dx%dx%d" % (m, k, n),
         "Matrix.fromList(%s).gemm(Matrix.fromList(%s), null, null, false, true)" % (lit(A), lit(Bt)),
         A @ Bt.T)
    emit("TT %dx%dx%d" % (m, k, n),
         "Matrix.fromList(%s).gemm(Matrix.fromList(%s), null, null, true, true)" % (lit(At), lit(Bt)),
         At.T @ Bt.T)

# ---- alpha / beta / addend broadcasting ----
lines.append("")
lines.append('print "-- numpy oracle: alpha, beta and broadcasting --"')
A, B = rand(5, 4), rand(4, 3)
C = rand(5, 3)
row = rand(1, 3)
col = rand(5, 1)
for alpha, beta in [(1.0, 1.0), (2.5, -1.5), (-1.0, 0.5), (0.25, 3.0)]:
    emit("alpha=%g beta=%g, full addend" % (alpha, beta),
         "Matrix.fromList(%s).gemm(Matrix.fromList(%s), Matrix.fromList(%s), null, false, false, %s, %s)"
         % (lit(A), lit(B), lit(C), num(alpha), num(beta)),
         alpha * (A @ B) + beta * C)
emit("row-vector addend broadcasts down the rows",
     "Matrix.fromList(%s).gemm(Matrix.fromList(%s), Matrix.fromList(%s))" % (lit(A), lit(B), lit(row)),
     A @ B + row)
emit("column-vector addend broadcasts across the columns",
     "Matrix.fromList(%s).gemm(Matrix.fromList(%s), Matrix.fromList(%s))" % (lit(A), lit(B), lit(col)),
     A @ B + col)
emit("scalar addend",
     "Matrix.fromList(%s).gemm(Matrix.fromList(%s), 3.75)" % (lit(A), lit(B)), A @ B + 3.75)

# ---- the level-1 path ----
lines.append("")
lines.append('print "-- numpy oracle: level-1 (null B) --"')
X = rand(6, 4)
Y = rand(6, 4)
emit("clone", "Matrix.fromList(%s).gemm(null)" % lit(X), X)
emit("transpose", "Matrix.fromList(%s).gemm(null, null, null, true)" % lit(X), X.T)
emit("negate", "Matrix.fromList(%s).gemm(null, null, null, false, false, -1)" % lit(X), -X)
emit("scale by 3.5", "Matrix.fromList(%s).gemm(null, null, null, false, false, 3.5)" % lit(X), 3.5 * X)
emit("add", "Matrix.fromList(%s).gemm(null, Matrix.fromList(%s))" % (lit(X), lit(Y)), X + Y)
emit("subtract", "Matrix.fromList(%s).gemm(null, Matrix.fromList(%s), null, false, false, 1, -1)"
     % (lit(X), lit(Y)), X - Y)
emit("axpby 2X - 0.5Y", "Matrix.fromList(%s).gemm(null, Matrix.fromList(%s), null, false, false, 2, -0.5)"
     % (lit(X), lit(Y)), 2 * X - 0.5 * Y)
emit("transpose with an addend",
     "Matrix.fromList(%s).gemm(null, Matrix.fromList(%s), null, true)" % (lit(X), lit(X.T.copy())),
     X.T + X.T)

# ---- a dense NN layer, the primary use case ----
lines.append("")
lines.append('print "-- numpy oracle: dense layer --"')
xb, W, b = rand(8, 5), rand(5, 3), rand(1, 3)
emit("x*W + b (batch 8, 5 -> 3)",
     "Matrix.fromList(%s).gemm(Matrix.fromList(%s), Matrix.fromList(%s))" % (lit(xb), lit(W), lit(b)),
     xb @ W + b)
h = np.maximum(xb @ W + b, 0)
W2, b2 = rand(3, 2), rand(1, 2)
emit("second layer over a relu'd hidden state",
     "Matrix.fromList(%s).gemm(Matrix.fromList(%s), Matrix.fromList(%s))" % (lit(h), lit(W2), lit(b2)),
     h @ W2 + b2)

# ---- elementwise ----
lines.append("")
lines.append('print "-- numpy oracle: elementwise --"')
E = rand(5, 4)
F = rand(5, 4, 1, 6)          # kept positive so pow/sqrt stay real
rowv, colv = rand(1, 4), rand(5, 1)
emit("elemMultiplyBy a matrix",
     "Matrix.fromList(%s).elemMultiplyBy(Matrix.fromList(%s))" % (lit(E), lit(F)), E * F)
emit("elemDivideBy a matrix",
     "Matrix.fromList(%s).elemDivideBy(Matrix.fromList(%s))" % (lit(E), lit(F)), E / F)
emit("elemMultiplyBy broadcasts a row vector",
     "Matrix.fromList(%s).elemMultiplyBy(Matrix.fromList(%s))" % (lit(E), lit(rowv)), E * rowv)
emit("elemMultiplyBy broadcasts a column vector",
     "Matrix.fromList(%s).elemMultiplyBy(Matrix.fromList(%s))" % (lit(E), lit(colv)), E * colv)
emit("abs", "Matrix.fromList(%s).abs" % lit(E), np.abs(E))
emit("sqrt", "Matrix.fromList(%s).sqrt" % lit(F), np.sqrt(F))
emit("pow 3", "Matrix.fromList(%s).pow(3)" % lit(E), E ** 3)
emit("pow by a broadcast exponent",
     "Matrix.fromList(%s).pow(Matrix.fromList(%s))" % (lit(F), lit(rowv)), F ** rowv)
emit("clamp both ends", "Matrix.fromList(%s).clamp(-2, 2)" % lit(E), np.clip(E, -2, 2))
emit("clamp from below only (relu)", "Matrix.fromList(%s).clamp(0, null)" % lit(E), np.clip(E, 0, None))
emit("clamp from above only", "Matrix.fromList(%s).clamp(null, 1)" % lit(E), np.clip(E, None, 1))

# ---- reductions ----
#
# NOTE ON round(): we deliberately do NOT compare rounding against numpy.
# numpy rounds half to even; MiniScript's own round() rounds half away from
# zero, and Matrix.round follows the language rather than numpy so that
# m.round and round(m.getElem(...)) cannot disagree.
lines.append("")
lines.append('print "-- numpy oracle: reductions --"')
R = rand(6, 5)
for axis, spell in ((0, "0"), (1, "1")):
    keep = R.sum(axis=axis, keepdims=True)
    emit("sum(axis=%d)" % axis, "Matrix.fromList(%s).sum(%s)" % (lit(R), spell), keep)
    emit("max(axis=%d)" % axis, "Matrix.fromList(%s).max(%s)" % (lit(R), spell),
         R.max(axis=axis, keepdims=True))
    emit("min(axis=%d)" % axis, "Matrix.fromList(%s).min(%s)" % (lit(R), spell),
         R.min(axis=axis, keepdims=True))
    emit("argmax(axis=%d)" % axis, "Matrix.fromList(%s).argmax(%s)" % (lit(R), spell),
         np.expand_dims(R.argmax(axis=axis), axis).astype(float))
    emit("argmin(axis=%d)" % axis, "Matrix.fromList(%s).argmin(%s)" % (lit(R), spell),
         np.expand_dims(R.argmin(axis=axis), axis).astype(float))
    emit("sumOfSquares(axis=%d)" % axis, "Matrix.fromList(%s).sumOfSquares(%s)" % (lit(R), spell),
         (R * R).sum(axis=axis, keepdims=True))

# Whole-matrix reductions come back as scalars, so they need their own check.
lines.append("")
lines.append('print "-- numpy oracle: whole-matrix reductions --"')
lines.append("""
cmpNum = function(got, want, label)
	outer.checks = outer.checks + 1
	if got isa error then
		outer.failures = outer.failures + 1
		print "FAIL " + label + "  (error: " + got + ")"
		return
	end if
	d = got - want
	if d < 0 then d = -d
	if d <= 1e-9 then
		print "ok   " + label
	else
		outer.failures = outer.failures + 1
		print "FAIL " + label + "  (got " + got + ", want " + want + ")"
	end if
end function
""")
for name, want in (("sum", R.sum()), ("sumOfSquares", (R*R).sum()),
                   ("max", R.max()), ("min", R.min()),
                   ("argmax", float(R.argmax())), ("argmin", float(R.argmin()))):
    checks += 1
    lines.append('cmpNum Matrix.fromList(%s).%s, %s, "%s over the whole matrix"'
                 % (lit(R), name, num(want), name))

# Tie-breaking: numpy takes the first maximum.  Build a matrix with deliberate
# ties so the convention is pinned rather than assumed.
T = np.array([[1.0, 5.0, 5.0, 2.0], [7.0, 7.0, 0.0, 7.0]])
checks += 1
lines.append('cmpNum Matrix.fromList(%s).argmax, %s, "argmax tie goes to the first, as numpy does"'
             % (lit(T), num(float(T.argmax()))))
emit("argmax(axis=1) with ties", "Matrix.fromList(%s).argmax(1)" % lit(T),
     np.expand_dims(T.argmax(axis=1), 1).astype(float))

# ---- linear algebra ----
#
# numpy earns its keep most here.  A determinant or an inverse written twice by
# the same author from the same formula will agree with itself even when the
# cofactor signs are wrong; LAPACK will not.  The closed-form 1x1..4x4 paths and
# the LU path above them are both exercised.
lines.append("")
lines.append('print "-- numpy oracle: linear algebra --"')
lines.append("""
cmpNear = function(got, want, label)
	outer.checks = outer.checks + 1
	if got isa error then
		outer.failures = outer.failures + 1
		print "FAIL " + label + "  (error: " + got + ")"
		return
	end if
	scale = want
	if scale < 0 then scale = -scale
	if scale < 1 then scale = 1
	d = got - want
	if d < 0 then d = -d
	if d <= 1e-9 * scale then
		print "ok   " + label
	else
		outer.failures = outer.failures + 1
		print "FAIL " + label + "  (got " + got + ", want " + want + ")"
	end if
end function
""")


def well_conditioned(n):
    """Random, but diagonally dominant so it is far from singular -- otherwise
    the comparison measures conditioning rather than correctness."""
    a = rand(n, n, -3, 3)
    a[np.diag_indices(n)] += n + 3
    return np.round(a, 4)


for n in (1, 2, 3, 4, 5, 7):
    a = well_conditioned(n)
    checks += 1
    lines.append('cmpNear Matrix.fromList(%s).determinant, %s, "%dx%d determinant"'
                 % (lit(a), num(np.linalg.det(a)), n, n))
    emit("%dx%d inverse" % (n, n), "Matrix.fromList(%s).inverse" % lit(a), np.linalg.inv(a))

# A matrix whose leading entry is zero: correct only with partial pivoting.
piv = well_conditioned(5)
piv[0, 0] = 0.0
checks += 1
lines.append('cmpNear Matrix.fromList(%s).determinant, %s, "determinant with a zero leading pivot"'
             % (lit(piv), num(np.linalg.det(piv))))
emit("inverse with a zero leading pivot", "Matrix.fromList(%s).inverse" % lit(piv),
     np.linalg.inv(piv))

# A row swap must flip the sign, exactly as it does for numpy.
sw = well_conditioned(6)
swapped = sw[[1, 0, 2, 3, 4, 5], :]
checks += 1
lines.append('cmpNear Matrix.fromList(%s).swapRows(0, 1).determinant, %s, '
             '"determinant after a row swap"' % (lit(sw), num(np.linalg.det(swapped))))

# solve: one right-hand side, then several at once.
for n in (2, 4, 6):
    a = well_conditioned(n)
    b1 = rand(n, 1)
    emit("%dx%d solve, one right-hand side" % (n, n),
         "Matrix.fromList(%s).solve(%s)" % (lit(a), lit(b1)), np.linalg.solve(a, b1))
    b3 = rand(n, 3)
    emit("%dx%d solve, three right-hand sides" % (n, n),
         "Matrix.fromList(%s).solve(%s)" % (lit(a), lit(b3)), np.linalg.solve(a, b3))

# A flat list of n numbers is a column vector, which is how numpy reads a 1-D b.
a = well_conditioned(4)
bv = rand(4, 1)
emit("solve with a flat-list right-hand side",
     "Matrix.fromList(%s).solve(%s)" % (lit(a), flat(bv)),
     np.linalg.solve(a, bv))

# Least squares through the normal equations, the motivating use.
X = rand(8, 3)
y = rand(8, 1)
emit("least squares via transposedTimes + solve",
     "Matrix.fromList(%s).gemm(Matrix.fromList(%s), null, null, true)"
     ".solve(Matrix.fromList(%s).gemm(Matrix.fromList(%s), null, null, true))"
     % (lit(X), lit(X), lit(X), lit(y)),
     np.linalg.solve(X.T @ X, X.T @ y))

# ---- per-row vector ops ----
#
# The cross product is all sign convention, and a sign convention is exactly the
# kind of thing a self-written reference reproduces wrongly with total
# confidence.  np.cross settles it.
lines.append("")
lines.append('print "-- numpy oracle: rowCross --"')

u = rand(6, 3)
w = rand(6, 3)
emit("rowCross, one operand row per row",
     "Matrix.fromList(%s).rowCross(Matrix.fromList(%s))" % (lit(u), lit(w)),
     np.cross(u, w))
emit("rowCross is anticommutative",
     "Matrix.fromList(%s).rowCross(Matrix.fromList(%s))" % (lit(w), lit(u)),
     np.cross(w, u))

# A single row crossed with every row.  numpy broadcasts a (3,) against an
# (n,3) the same way, which is what makes this comparable.
axis = rand(1, 3)
emit("rowCross broadcasting a single row",
     "Matrix.fromList(%s).rowCross(%s)" % (lit(u), flat(axis)),
     np.cross(u, axis[0]))

# Torque = r x F, the motivating physics case, on a full frame's worth of rows.
r = rand(32, 3)
force = rand(32, 3)
emit("torque = r x F over 32 bodies",
     "Matrix.fromList(%s).rowCross(Matrix.fromList(%s))" % (lit(r), lit(force)),
     np.cross(r, force))

lines.append("")
lines.append('print')
lines.append('print checks + " numpy-oracle checks, " + failures + " failures"')

with open(OUT, "w") as f:
    f.write("\n".join(lines) + "\n")
print("wrote %s  (%d checks)" % (OUT, checks))
