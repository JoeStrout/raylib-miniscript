# Intrinsic `Matrix` Class — Design Notes

Target: raylib-miniscript. Status: agreed design, not yet implemented.
Replaces the pure-MiniScript `sys/lib/matrixUtil.ms`.

## Goals

Fast bulk linear algebra for:
- neural network **inference** (training possible, but as a later MiniScript layer)
- simple physics over many entities (position / velocity / inertia / rotation)
- general linear algebra in games

The point is **bulk operations that obviate scripted loops**. Scalar element access
should be adequate, not fast.

## Non-goals (v1)

- No operator overloading, no index overloading. Intrinsic classes can't do this,
  and it's not obvious what `*` should mean anyway. Named methods only.
- **No views / aliasing.** Every op returns fresh storage or writes into a supplied
  matrix. This keeps GC trivial and avoids lifetime and reshape-vs-view headaches.  (Passing the same matrix as input and output is fine; this is handled internally.)
- No N-dimensional tensors. See "Why 2D only" below.
- Single-threaded. Revisit with benchmarks, not on principle.
- No automatic differentiation in C++.
- **No lazy expression graphs.** See "Rejected: lazy expression graphs" below.

---

## Implementation model

`Matrix` is an ordinary MiniScript **class** (a map with predefined methods), with the
numeric payload behind an opaque **handle** value stored in the instance map.

### Storage

```c
struct MatrixData {
    double* data;       // capacityElems doubles
    long    capacityElems;
    int     rows, columns;
};
```

- **Row-major.** Row `r` begins at `data + r*columns`.
- Live region is `data[0 .. rows*columns-1]`, always contiguous. Spare capacity
  sits *after* the live region.
- **Capacity is measured in elements, not rows** (so that `reshape` to a different
  column count leaves the bookkeeping meaningful). Row capacity is derived:
  `capacityElems / columns`.
- **Stride is always exactly `columns`.** No separate column capacity. Growing the row
  count is amortized O(1); changing the column count is a realloc-and-copy, documented
  as expensive. Justification: for all three use cases, rows are what change
  (entities spawn/die, batch sizes vary) while columns are structural.
- Element type is `double` throughout. A `dtype` option could be added later without
  disturbing the handle model; the *storage* type for serialization is separate
  (see RawData).

### Growth / shrink policy

- On overflow: `newCap = max(2*oldCap, needed)`.
- **Never shrink capacity automatically.** A pool oscillating between 900 and 1000
  rows allocates zero times after warmup. `trim` is the explicit release.
- Total size limit: a sensible cap (TBD, order of a few hundred MB); exceeding it,
  or a failed allocation, returns an error value.

### Handle lifetime

- Refcounted storage, freed on collection. No manual `dispose` required.
- **`Matrix` itself must hold a null handle.** Otherwise `new Matrix` produces a map
  containing only `__isa`, and `m.handle` resolves *up the chain* to `Matrix`'s handle,
  silently sharing one buffer across every instance. Every constructor attaches a
  fresh handle; every intrinsic errors on a null handle.
- Consequence: `m = new Matrix` followed by manual field assignment is no longer a
  valid user pattern. `ofSize` and friends are the only allocators.
- Only `.clone` copies the payload. Any map-level copy shares the handle.

### `rows` / `columns`

Plain map entries, written by the C++ side on every resize. Keeps `m.rows` syntax and
map-lookup speed. Documented as read-only; assigning them desyncs from the handle.

### `==` gotcha (document prominently)

`m1 == m2` compares *maps* — `__isa` plus the handle — so two matrices with identical
contents compare **unequal**. Use `.equals`. The handle value has identity equality only.

### The `gemm` seam

All matrix products — every named script wrapper, and the `m.gemm` intrinsic itself —
funnel through one *GEneral Matrix Multiply* routine, BLAS-style:

```c
gemm(double alpha, const Matrix& A, bool transA,
                   const Matrix& B, bool transB,
     double beta,  Matrix& C);           // C := alpha*op(A)*op(B) + beta*C
```

`transA`/`transB` are plain bools. (BLAS itself uses `'N'`/`'T'`/`'C'` characters, a
Fortran-API holdover; we have no complex types, so there's no third case to encode.)

`op(A)` is `A` if `transA` is false, or `A.transpose` if `transA` is true.

The MiniScript-visible `m.gemm` (see "Matrix product") separates `addend` from `out`,
which this BLAS-shaped kernel does not. The intrinsic reconciles them: broadcast-copy
`beta*addend` into `out`, then call the kernel with `beta=1`. That copy is O(mn) against
an O(mnk) multiply — noise — and is skipped entirely when `out` *is* `addend`, preserving
the optimal path. **This is what keeps the backend swappable:** our richer signature still
reduces to a stock BLAS call.

- Exposing `alpha`/`beta` rather than hiding them is what keeps the C++ method count at
  one: transposition, scaling, and accumulation are parameters, not variants.
- **`beta == 0` must not read `C`**, just overwrite it (fresh storage may hold garbage;
  `0 * NaN` is `NaN`). Same contract as BLAS.
- **The transpose flags do not transpose anything** — they change the inner-loop indexing.
  All three variants have contiguous row-major formulations: `A·B` broadcasts `A[i][k]`
  over a contiguous `j` loop; `A·Bᵀ` is a dot product of two contiguous rows; `Aᵀ·B` is a
  rank-1 update over contiguous rows of both. So `transposedTimes`/`timesTransposed`
  cost no allocation and no copy — which is the whole reason those wrappers exist, and a
  large part of why lazy expression graphs bought too little to justify themselves
  (see "Rejected: lazy expression graphs").

**Level-1 ops enter through `gemm` but must dispatch away from it immediately.** Because
`add`, `plus`, `subtract`, `negate`, `addScaled`, `transpose`, `clone`, and `copyFrom` are
all defined as `gemm` calls with a null `B` operand (see "Convenience wrapper methods"),
`gemm` is the entry point for a great deal of bandwidth-bound work that has nothing to do
with matrix multiplication.

So `gemm` must **early-dispatch on a null `B`** to a simple axpby loop, and on
`transA` with null `B` to a transpose-copy — never letting either walk into the blocked
matmul path, which would make every matrix addition pay matmul setup and lose to a naive
loop. Internally this is three clearly separate kernels — **axpby**, **transpose-copy**,
and **matmul** — sharing only argument validation and de-aliasing. That keeps the
intrinsic-count reduction from becoming a performance regression, and keeps the C++
readable instead of one seven-mode function.

**Implementation: custom C++, no BLAS dependency.** The workload is dominated by small
matrices and bandwidth-bound level-1 ops, where BLAS's advantage is near zero; and
consistent results across platforms matter for a program-sharing platform. Use `i-k-j`
loop order (contiguous inner loop, auto-vectorizes at `-O2`), `__restrict` on the
kernel pointers (legitimate because the intrinsic de-aliases into a scratch buffer before the kernel is entered, and because we have no user-visible views), and cache blocking only above
~128. The seam exists so an Accelerate/OpenBLAS backend can be swapped in behind an
`#ifdef` later without touching the API.

The **scratch buffer** is maintained internally as static thread-local storage.  It grows as needed, and never shrinks automatically, so a steady-state loop that aliases allocates zero times after warmup.  There is no reentrancy hazard because `gemm` never calls back into MiniScript.

---

## Conventions

### Naming

| Form | Meaning | Example |
|---|---|---|
| imperative verb | mutates in place, returns `self` or nothing | `m.add x` |
| `-ed` / noun form | returns a new matrix, receiver untouched | `m.plus(x)` |
| `...Into(args, out)` | writes result into caller-supplied `out` | `a.timesInto(b, out)` |

Existing inconsistencies (`transpose` and `round`, both imperative-but-not-matching)
are **fixed**, not grandfathered — the current library has almost no users. So: `transpose` mutates, `transposed` returns new; `round` mutates, `rounded`returns new.

`...Into` forms exist for the hot-loop ops (`times`, `plus`, `elemTimes`, `apply`) so
that per-frame / per-batch code allocates nothing.

### Unspecified dimensions

Use `null`, not `-1`. e.g. `m.reshape 4, null` infers columns.

### Errors

MiniScript 2.0 errors are values, and existing intrinsics already propagate them. So:

- Dimension mismatch, bad index, null handle, allocation failure → **return an error value**.
- Errors propagate through chained calls: `a.times(b).plus(c)` on a mismatch yields the
  original error rather than a second, more confusing one.
- Note for users: an error value is not `null` and not `0`. Check it.

### Randomness

`randomize` and any `@rnd`-style fill use **the interpreter's RNG state**, so `rnd(seed)`
gives reproducible weight initialization. (Passing `@rnd` as a fill function gets this
for free.)

---

## API

### Construction

```
Matrix.ofSize(rows, columns, initialValue=0)
```
`initialValue` may be a number, or a **function taking no arguments**, invoked once per
element, in row-major order — e.g. `Matrix.ofSize(3, 3, @rnd)`. (Matches `list.init` /
`list.init2d`.) A function will always be slower than a numeric value; users understand
this.

The callback runs re-entrantly (`VM::RunFunction` pushes its frame above the intrinsic's
own registers), so it may do anything an ordinary function can: allocate, call other
intrinsics, even call `ofSize` with another callback. Each result must be a number:

- an **error value** returned by the callback is returned to the caller in place of the
  matrix — that exact error, not a generic wrapper;
- any other non-number is a `TypeError` (this includes the `null` from a function with no
  explicit return, so a mistaken callback fails loudly rather than filling with zeros);
- a runtime error **raised** inside the callback stops the program, as it would anywhere
  else; the half-built matrix is discarded.

```
Matrix.identity(size)              // square; ones on the main diagonal
Matrix.fromList(sourceList)        // 1D list -> 1 x n row vector; 2D list -> n x m
Matrix.fromRawData(rd, dtype="auto", startPos=0, rows=null, columns=null)
```

`fromList([])` yields a clean 0x0 matrix (the old code left `elem = null`).

### Shape

```
m.rows, m.columns                  // map entries, read-only; number of rows and columns
m.capacity                         // read-only report on current element capacity
```

`m.size` (a wrapper) is treated as canonical in the docs, with `rows`/`columns` as 2D
conveniences, so a hypothetical N-D future wouldn't break the shape API.  We'll take care to handle zero-row and zero-column matrices properly.

### Resize/Reshape

```
m.reshape(rows, columns)           // in place, O(1); null infers one dimension
m.resize(rows, columns)            // preserve overlapping submatrix, zero-fill growth
m.reserve(rows)                    // pre-allocate, no visible change
m.removeRow(index)                 // order-preserving, O((rows-index)*columns)
m.removeRowFast(index)             // swap-with-last, O(columns) — REORDERS
m.trim                             // release excess capacity
```

`reshape` is O(1) because the live region is contiguous (total size is required to be unchanged). It is **not** transpose — say so in the docs, users conflate them.

Two separate removal methods rather than one with a flag: the naming *is* the warning
that `removeRowFast` invalidates any external index→entity mapping.

### Element access

```
m.getElem(row, col)
m.setElem(row, col, value)
```

**Negative indices count from the end**, as they do for MiniScript lists and
strings — and as they do in numpy, which is the closer reference for a matrix
type. `m.getElem(-1, -1)` is the bottom-right element. This applies to every
index the API takes, not just `getElem`/`setElem`. An index that is still out of
range after that adjustment returns an error value; in particular *every* index
into a zero-row or zero-column matrix is out of range.

Convenience path, not the loop path. For loops, use row/flat access:

```
m.getRow(r) / m.setRow(r, list)         // plain MiniScript list of numbers
m.getColumn(c) / m.setColumn(c, list)
m.toFlatList                            // single flat list; index as r*columns + c
```

Block access — a rectangular sub-region, returned or written as a **Matrix**:

```
m.getSub(row=0, col=0, rows=null, columns=null)   // -> new Matrix; null = through the end
m.setSub(row, col, m2)                            // write m2 as a block, in place
```

Intrinsic rather than a wrapper despite the reduction goal: the script version would
allocate a MiniScript list per row to accomplish what is fundamentally a strided memcpy,
which is real garbage pressure for a 512-row mini-batch. The kernel is ~15 lines.

Motivating cases: mini-batch extraction (`m.getSub(10, 0, 32)`), pulling the 3x3 rotation
out of a 4x4 transform (`m.getSub(0, 0, 3, 3)`), and the inverse — assembling a 4x4 from a
rotation block plus a translation column.

Note the deliberate asymmetry with `getRow`, which returns a plain list: a 2-D result has
no natural flat-list form, so `getSub` returns a Matrix. Out-of-range indices return an
error value; `setSub` errors if `m2` does not fit at the given offset rather than
silently clipping.

`m.row(r)`, `m.column(c)`, `m.columnAsRow(c)` still return **Matrix** copies, as today,
and are now wrappers over `getSub`.

### Arithmetic

Every `plus`/`elemTimes`-style op accepts a scalar, a Matrix, or a 2D list, and supports
**row/column broadcasting** (a 1 x m or n x 1 operand against an n x m matrix) — this is
what makes bias vectors and per-entity gravity one call.

Only the **in-place** forms are intrinsic. The copying counterparts (`elemTimes`,
`elemOver`, `powed`, `absed`, `sqrted`, `rounded`, `clamped`) are clone-then-mutate
wrappers, and the whole add/scale family (`add`, `plus`, `subtract`, `minus`, `negate`,
`negated`) comes from `gemm`. See **Convenience wrapper methods**.

```
m.elemMultiplyBy(x)                // elementwise; Hadamard when x is a Matrix
m.elemDivideBy(x)
m.pow(k)
m.abs, m.sqrt
m.round(places=0)
m.clamp(lo, hi)                    // pass null for clamp-from-below or clamp-from-above
```

### General Matrix Multiply

**Exactly one product intrinsic in C++:**

```
A.gemm(B, addend=null, out=null, transA=false, transB=false, alpha=1, beta=1)
```

Computes `alpha*op(A)*op(B) + beta*addend`, where `op(A)` is just `A` if `transA` is `false`, or `A.transpose` if `transA` is true (and similar for `B`).  `B` may be `null` in which case this computes `alpha*op(A) + beta*addend`.  Similarly, `addend` may be `null` (or `beta`==0) and the `beta*addend` term is skipped.
With `out=null` it allocates and returns a new matrix; otherwise it writes into `out` (resizing it as necessary) and returns it. 
In the event of an error (e.g. dimension mismatch), it returns an error value.

**Deliberate deviation from BLAS:** real BLAS makes `C` serve as both addend and
destination. We separate them, because writing our own kernel means we don't have to
inherit that. Three payoffs:

- `addend` may be **null, a scalar, a 1 x n or n x 1 vector (broadcast), or a full
  m x n matrix** — so `x.gemm(W, b)` is a complete dense NN layer in one kernel pass,
  bias broadcast included. This is the most common operation in the primary use case.
- A non-mutating `A·B + C` needs no defensive clone.
- In-place accumulate is just `out == addend`, stated positively rather than as a trap.

Parameter order puts `addend` and `out` early because they're the common ad-hoc
overrides; transposition is usually reached through a named wrapper.

**Aliasing is allowed everywhere:**

| Combination | Allowed? |
|---|---|
| `out` aliases `addend` | **Yes** — the accumulate case, elementwise, and the fast path |
| `out` aliases `A` or `B` | **Yes** — aliased input is copied to internal buffer before calculation |
| `addend` aliases `A` or `B` | Yes — read-only |

`A := A·B + ...` would be impossible without a temporary regardless of shapes, so we maintain an internal scratch buffer, resized as necessary, used for this purpose.

There is no combinatorial explosion of C++ methods, because transposition, scaling, and
accumulation are *parameters*, not variants. The only real problem is that MiniScript has
positional defaults but no keyword arguments, so `m.gemm(b, null, null, true)` reads
poorly.  But this can be fixed in script, rather than in C++, via wrapper methods (see **Convenience wrapper methods**).


### Bulk transforms

```
m.fill(value)
m.randomize(mean=0, sd=1)          // normal distribution
m.apply(@f)                        // f(value) -> value, in place; like list.apply
m.apply1(@f, arg1)                 // f(value, arg1); like list.apply1
```

Index-dependent initialization is deliberately **not** supported by a callback. Write a
loop, or build a list and use `fromList`.  Where a MiniScript function is passed as an argument (e.g. `apply`), we will impose sensible reentrancy restrictions.

### Reductions

Axis parameter: `null` = whole matrix (scalar), `0` = down columns, `1` = across rows.  Return value is a smaller matrix (1xN for axis 0, Mx1 for axis 1) or a simple scalar (axis==null).

```
m.sum(axis=null)
m.max(axis=null), m.min(axis=null)
m.argmax(axis=null), m.argmin(axis=null)
m.sumOfSquares(axis=null)
```

### Linear algebra

```
m.determinant
m.inverse                          // error value if singular
m.solve(b)                         // LU with partial pivot; -> n x k Matrix
m.swapRows(row1, row2), m.swapColumns(column1, column2)
m.equals(m2, tolerance=1e-9)       // Matrix or 2D list
```

Closed-form fast paths for 1x1 / 2x2 / 3x3 / 4x4 determinant and inverse — not merely
an optimization, since a 4x4 transform inverted every frame is the common case, and
cofactor expansion at that size costs less than a pivot search. Above 4x4 both go
through one LU factorization with partial pivoting; `inverse` is that factorization plus
a single multi-column solve against the identity, not n separate solves.

A **singular** matrix has determinant 0 — an answer, not an error — but `inverse` and
`solve` return an error value, matching numpy's `LinAlgError`: an un-invertible transform
is a bug upstream, not a value worth propagating as NaN.

`solve`'s `b` is a Matrix or a list. A *flat* list of n numbers is read as a **column**
vector rather than the 1 x n that `fromList` would make of it: this is the one place where
a bare list of numbers unambiguously means "the right-hand side of n equations", and numpy
reads a 1-D `b` the same way. A nested list or a Matrix must already be n x k. All k
right-hand sides are solved from one factorization.

`transpose` / `transposed` are not here: they fall out of `gemm` with a null `B` operand.

### Per-row vector ops (physics)

```
m.rowCross(m2)                     // n x 3, requires 3 columns
```

`rowLengths`, `normalizeRows`/`normalizedRows`, and `rowDot` are wrappers; only the
3-component cross product needs its own kernel.

Both operands need exactly 3 columns. `m2` may have one row per row of `m`, or exactly
one row, which is then crossed with every row — the fixed-axis case (`vel.rowCross(up)`),
and the same row broadcasting the arithmetic ops do. A flat list of 3 numbers is that
single row.

Deliberate divergence from numpy: `np.cross` broadcasts in *both* directions, so a 1x3
crossed with an nx3 yields n rows. Here the result always has **self's** row count.
`a.rowCross(b)` reads as "for each of my rows", and a method that silently returned more
rows than the receiver has would be a trap. A 1x3 receiver against an nx3 operand is an
error, not an upcast.

### Neural network primitives

Chosen so a MiniScript layer library can be built on top without further C++ work.

```
m.sigmoid
m.tanh
m.softmax(axis=1)
m.greaterThan(x)
m.softmaxCrossEntropy(targets, outProbs=null)   // fused; -> n x 1 per-sample losses
```

**Derivatives are expressed in terms of the layer's OUTPUT, not its input:**
`sigmoid' = y*(1-y)`, `tanh' = 1-y²`, `relu' = (y>0)`. So a layer caches only its
output, never the pre-activation. Meaningfully simpler layer code. The derivatives
themselves (`sigmoidDeriv`, `tanhDeriv`) and `softmaxCrossEntropyGrad` are script
wrappers — the last is literally `yHat.minus(targets)`.

`relu` and related functions (`relued`, `reluDeriv`) are missing at the C++ layer because they are trivially defined in terms of `clamp` and `greaterThan`, but would probably be added at the MiniScript layer.

#### `softmaxCrossEntropy` in detail

Fused because it is numerically necessary anyway (log-sum-exp stability) and because its
gradient collapses to `yHat - y`, removing the single most error-prone derivative from
user code.

**Input is logits, not probabilities.** It applies the softmax itself. State this loudly:
`m.softmax` exists as a separate method, and the obvious-looking `z.softmax` followed by
`softmaxCrossEntropy` is both wrong (double softmax) and numerically worse than the thing
the fusion exists to avoid.

**Row-wise only**, samples in rows — consistent with batch x features everywhere else
(`getSub` for mini-batches, `sumRows` for bias gradients). Deliberately *no* `axis`
parameter, unlike `softmax`: cross-entropy is inherently a per-sample loss, and an axis
knob would only invite confusion about what a sample is.

**`targets` accepts two forms, dispatched on shape:**

| Shape | Interpretation |
|---|---|
| n x k (same as logits) | target distribution — one-hot, but also label smoothing / soft targets |
| n x 1 | class indices — avoids materializing an n x k one-hot matrix per batch |

Dispatch: if `targets.columns == self.columns` it is a distribution; else if
`targets.columns == 1` it is class indices; else error. (The two overlap only at k=1,
where softmax is degenerate anyway; distribution wins.) Out-of-range class indices return
an error value, as do fractional ones.

A **flat list** is always read as a column of class indices — the same reading `solve`
gives its right-hand side — never as a one-row distribution. Deliberately unconditional:
dispatching a flat list on whether n happens to equal k would make `[2,0,1]` mean one
thing for a 3-class batch of 3 and something else for a 4-class batch of 3. A distribution
must be written as a nested list or a Matrix.

`outProbs` may safely be the logits matrix itself — each row is fully read before anything
is written back to it — but not the targets matrix, which is refused, since reshaping
`outProbs` would move storage the targets are still being read from.

**Returns the unreduced n x 1 per-sample loss vector, not a scalar.** This matters because
of the gradient pairing: `softmaxCrossEntropyGrad` is `yHat - targets`, which is the
gradient of the **summed** loss. Had this returned the *mean* — the conventional default —
then a user pairing the documented loss with the documented gradient would train with
gradients n times too large: silent, batch-size-dependent, and exactly the class of bug
`gradCheck` exists to catch but that most people never run.

Returning unreduced removes the hidden convention. `sum` and `mean` are already
first-class reductions, so the choice is visible in the user's own code:

```
losses = z.softmaxCrossEntropy(targets)   // n x 1
reported = losses.mean
dZ = z.softmaxCrossEntropyGrad(targets)   // gradient of the SUM
```

Training on the mean means scaling the gradient by `1/n`.

**`outProbs`**, if supplied, receives `softmax(m)` — backprop needs `yHat` as well as the
loss, and this avoids computing the softmax twice. Matches the out-parameter convention
used by `gemm`.

**Numerics:** natural log (nats, not bits), and log-sum-exp with max subtraction, so
`log(0)` is unreachable by construction.

### Serialization (RawData)

```
m.toRawData(rd, dtype="float64", startPos=0, includeHeader=true)   // -> ending position
m.readRawData(rd, dtype="auto", startPos=0)                        // -> ending position
```

(`Matrix.fromRawData` is listed under **Construction**.)

- `dtype`: `float64`, `float32`, plus the integer formats (`int8`/`uint8`, `int16`/`uint16`,
  `int32`/`uint32`, `int64`) for completeness — `uint8` and `int16` matter for image and
  quantized-weight data.
- **`dtype="auto"` on read means "the data starts with a header."** Specifying an explicit
  dtype instead means "no header, read raw from `startPos`." This is the key affordance:
  it lets you load data produced by something other than us.
- Headerless read gets its shape from the receiver (`readRawData`) or from explicit
  `rows`/`columns` (`fromRawData`).  `fromRawData` will infer whichever of the two you
  leave null from how much data remains after `startPos`, so `Matrix.fromRawData(rd,
  "float32", 0, null, 3)` loads "however many 3-vectors are in there".  The division must
  come out even; a leftover tail means the data is not the shape you think it is, and
  that is worth an error rather than a quietly truncated matrix.  With `dtype="auto"`,
  passing `rows`/`columns` at all is an error — the header is the authority on shape, and
  accepting both would mean deciding what to do when they disagree.
- Both directions **return the ending position**, so loading a whole network's weights
  out of one blob is a clean sequential loop.
- `toRawData` **grows the RawData** to fit what it is writing (allocating one for a bare
  `new RawData`).  The alternative — making the caller size the buffer first — means
  restating this file's own layout rules in script.  A buffer we merely borrow rather
  than own is never reallocated; that is an error instead.
- Header: 16 bytes — magic `MSMX`, uint16 version, uint16 dtype code, int32 rows, int32
  columns.  Room to extend to N-D or new dtypes without breaking old files, and 16 keeps
  `float64` payload 8-byte aligned behind it.  Dtype codes are part of the format and are
  never reused for a different type.
- **Little-endian**, stated explicitly; these files will move between platforms.  More
  precisely, byte order follows the RawData object's own `littleEndian` flag, which
  defaults to little-endian.  The flag has to be honored so that headerless *foreign* data
  of the other byte order can be read at all, and it would be strange for it to govern
  `rd.int` but not matrix data in the same buffer.

**Integer conversion:** writing to an integer format rounds to nearest with ties away
from zero (matching MiniScript's `round`) and **saturates** at the ends of the range; NaN
stores as 0.  This is a deliberate departure from numpy's `astype`, which truncates
toward zero and leaves out-of-range conversion undefined — in practice wrapping, so 300
becomes 44 in a `uint8`.  For what these formats are actually for, a clamped bright pixel
is a small error and a wrapped one is garbage.  Reading back is exact for every format
except `int64` beyond 2^53, where the double itself is the limit.

### Formatting

```
m.format(fieldWidth=10, precision=null, columnSep="", rowSep=null)   // -> string
```

`m.str` (a script wrapper) gets a **truncating** default — dimensions plus a few
elements — so a 1000x1000 matrix doesn't dump 10⁶ numbers into the console.

It is `m.str` rather than the global `str(m)` because MiniScript has no hook for a
map to override its own string form: `Value.CodeForm` stringifies a map by walking
its entries, with no per-map override to consult. What the global `str(m)` *does*
get is a readable `__isa`: the C++ side registers the class with
`Intrinsic::AddShortName`, which `CodeForm` consults for `__isa` entries, so
`str(m)` yields `{"__isa": Matrix, "_handle": <value>, "rows": 2, "columns": 3}`
instead of dumping every method in the class. (Every intrinsic class in the host
does this now, not just Matrix.) Should MiniScript ever gain a per-map `_str` hook,
pointing it at `m.str` is a one-line change.

**The old `if s.indexOf("E-") != null then s = "0"` hack (`matrixUtil.ms:281`) is
gone**, not made optional: it also silently zeroed legitimately small values like
`1.5e-5`. `format` handles the real case properly instead — with a `precision`,
fixed-point notation renders `1e-17` as `0.000` because that genuinely is its value
to three places, while the default stays honest about magnitude.

---

## Convenience wrapper methods

A large number of methods a matrix-math user would expect can be defined in terms of the existing C++ API.  This will be done as MiniScript methods added to the Matrix class in the matrixUtil module, e.g.:

```
Matrix.times = function(m2)
	return self.gemm(m2)
end function
```
GEMM wrapper methods:

| Wrapper | GEMM call |
|---|---|
| `A.times(B)` | `A.gemm(B)` *if B matrix*, or `A.gemm(null, null, null, false, false, B)` *if B scalar* |
| `A.multiplyBy(B)` | `A.gemm(B, null, A)` or `A.gemm(null, null, A, false, false, B)` |
| `A.timesInto(B, out)` | `A.gemm(B, null, out)` |
| `A.transposedTimes(B)` | `A.gemm(B, null, null, true)` |
| `A.timesTransposed(B)` | `A.gemm(B, null, null, false, true)` |
| `A.timesPlus(B,c)` | `A.gemm(B, c)` |
| `A.timesPlusInto(B,c,out)` | `A.gemm(B, c, out)` |
| `A.plusScaled(C,beta)` | `A.gemm(null, C, null, false, false, 1, beta)` |
| `A.addScaled(C,beta)` | `A.gemm(null, C, A, false, false, 1, beta)` |
| `C.addProduct(A,B)` | `A.gemm(B, C, C)` |
| | *— transpose & copy: no second operand, so `op(A)` is the whole product term —* |
| `A.transposed` | `A.gemm(null, null, null, true)` |
| `A.transpose` | `A.gemm(null, null, A, true)` |
| `A.clone` | `A.gemm(null)` |
| `A.copyFrom(B)` | `B.gemm(null, null, A)` *(note swapped receiver)* |
| | *— axpby family: `alpha*A + beta*addend` —* |
| `A.plus(x)` | `A.gemm(null, x)` |
| `A.add(x)` | `A.gemm(null, x, A)` |
| `A.minus(x)` | `A.gemm(null, x, null, false, false, 1, -1)` |
| `A.subtract(x)` | `A.gemm(null, x, A, false, false, 1, -1)` |
| `A.negated` | `A.gemm(null, null, null, false, false, -1)` |
| `A.negate` | `A.gemm(null, null, A, false, false, -1)` |

`addScaled(m2, k)` is the level-1 accumulate, `addProduct(a, b)` the level-3 one; both
mutate the receiver, per the naming convention. New named combinations can be added
freely without touching C++; or users may call `gemm` directly.

`transpose`/`transposed` falling out of `gemm` is the non-obvious one: with no second
operand the product term is just `op(A)`, so the transpose flag alone does the whole job
and no separate transpose kernel is needed. It relies on `out` being resized to fit.

`copyFrom` swaps the receiver (`B.gemm(null, null, A)`) rather than using `alpha = 0`,
which avoids needing a "don't read `A`" contract on `alpha` to match the one on `beta`.
`fill` would need that contract, so it stays an intrinsic.

**Not subsumable:** `elemMultiplyBy` / `elemDivideBy` stay intrinsic for the Hadamard
case. Their *scalar* paths could route to `gemm`'s `alpha`, but that's a script-level
branch and saves no C++.

### Copying forms as clone-then-mutate

Every `-ed` copying form is just `clone` plus the in-place version, so only the mutator
needs to be intrinsic:

`minus`, `elemTimes`, `elemOver`, `powed`, `absed`, `sqrted`, `rounded`, `clamped`,
`applied`, `applied1`, `sigmoided`, `tanhed`, `softmaxed`, `normalizedRows`, `reshaped`,
`flattened`

Cost is two passes instead of one on the copying variants. That's the right trade: both
forms exist precisely because hot-loop code uses the in-place one, so the convenience
form is the one that can afford to be slower.

### Other wrappers

| Wrapper | Definition |
|---|---|
| `m.size`, `m.sameSize`, `m.rowRange`, `m.colRange` | from the `rows` / `columns` map entries |
| `m.row(r)` | `m.getSub(r, 0, 1)` |
| `m.column(c)` | `m.getSub(0, c, null, 1)` |
| `m.columnAsRow(c)` | `m.getSub(0, c, null, 1).transposed` |
| `Matrix.vcat(matrices)` | `ofSize` + `setSub` per block |
| `Matrix.hcat(matrices)` | same, roles swapped |
| `m.appendRows(m2)` | `resize` + `setSub` at the old row count |
| `m.toList` | loop of `getRow` |
| `m.mean(axis)` | `sum(axis)`, scaled |
| `m.sumRows` / `m.sumColumns` | `sum(0)` / `sum(1)` |
| `m.rowLengths` | `sumOfSquares(1).sqrt` |
| `m.normalizeRows` | `elemDivideBy(rowLengths)` — n x 1 broadcast |
| `m.flatten` | `reshape(1, null)` |
| `m.addRow` / `m.addRows` | `resize` + `setRow` |
| `Matrix.fromFlatList` | `fromList` + `reshape` |
| `m.trace` | O(n) `getElem` loop |
| `m.rowDot(m2)` | `elemTimes(m2).sum(1)` |
| `m.sigmoidDeriv` | `y.elemTimes(y.negated.plus(1))` |
| `m.tanhDeriv` | `y.powed(2).negated.plus(1)` |
| `m.softmaxCrossEntropyGrad(t)` | `yHat.minus(t)` |
| `m.relu` / `m.relued` | `clamp(0, null)` / `clamped(0, null)` |
| `m.print` | `format` |
| `m.str` | `format`, truncated to a few rows/columns plus the dimensions |

`softmaxCrossEntropyGrad` never needed to be C++ — it is documented as exactly
`yHat - targets`.

**Concatenation** takes a *list*, not a pair: pairwise chaining (`a.vcat(b).vcat(c)`)
would recopy the accumulated prefix at every step, whereas one pass computes the total
size, allocates once, and block-copies each input into place — exactly what a C++
implementation would do.

`appendRows` is the one worth distinguishing from `vcat`: it grows in place, so against
reserved row capacity it allocates nothing at steady state. That is the case the storage
design was built for — a physics pool absorbing a batch of spawned entities. `vcat`
always allocates a fresh matrix.

`hcat` hits the expensive column-resize path, but that is unavoidable: row-major storage
means columns are never contiguous, so any horizontal concatenation is inherently a full
copy. No loss versus a hand-written kernel. Dimension mismatches (differing column counts
for `vcat`) return an error value, checked before allocating rather than partway through
the copy loop.

Note that the classic augmented-matrix trick — `hcat`-ing a column of ones to fold bias
into the weight matrix — is *not* needed here, because `gemm`'s separate broadcast
`addend` handles bias directly.

Net effect of all three mechanisms: roughly **90 named methods down to ~50 intrinsics**,
with the script layer supplying the rest at no implementation cost.

---

## Compatibility

`Matrix` is intrinsic, hence always in globals; `import "matrixUtil"` no longer needed.
Keep `sys/lib/matrixUtil.ms` as a thin script layer that is harmless to import, holds
the unit tests, and hosts pure-script conveniences not worth writing in C++. Later the
`nn` library lives beside it.

Breaking changes, all intentional:

- **`m.elem` is gone.** Use `getElem`/`setElem`, `getRow`/`setRow`, `toList`/`toFlatList`.
- `m = new Matrix` + manual field assignment no longer works; use `ofSize` etc.
- `transpose` and `round` now mutate; use `transposed` / `rounded` for the old behavior.
- `times` on incompatible sizes returns an error value instead of `print`-then-`exit`
  (`matrixUtil.ms:206`).
- `rowRange`/`colRange` no longer cached on the instance.

---

## Rejected idea: lazy expression graphs

The idea: methods like `transpose`, `times`, `plus`, and scaling would not compute
immediately, but return a Matrix holding a pointer to its input(s) and a record of the
pending operation. Chains would materialize only when something needed real elements,
resolving into a single `gemm` call — avoiding every intermediate allocation, and
removing the apparent need for a proliferation of fused methods.

**Rejected primarily because laziness fights the mutation-heavy API we chose on purpose.**
A lazy node holds references to its inputs, and our API is full of in-place mutators
(`add`, `fill`, `apply`, `addScaled`, `copyFrom`, `resize`, `removeRowFast`) that exist
specifically to avoid GC churn in game loops:

```
c = a.times(b)          // lazy: holds refs to a and b, computes nothing
a.fill 0                // in-place mutation of an input
print c.getElem(0, 0)   // materializes NOW — against the zeroed a
```

Silent wrong answer, no error, arbitrarily far from the call site — and not an exotic
case, since the physics loop mutates the same matrices every frame. Every fix is bad:
document-and-pray is unacceptable for a beginner platform; version-stamping inputs gives
every mutator a hidden, unpredictable matmul; copy-on-write makes `fill` sometimes
allocate a whole matrix, destroying the reason the mutators exist. This is Eigen's
aliasing problem, and a large part of why numpy stayed eager.

Note the pattern: **laziness pays off in immutable array systems** (JAX, Eigen const
expressions); **we deliberately chose mutable-in-place** for game-loop performance.
Taking both gets the worst of each.

Secondary costs: every intrinsic needs a materialize-if-needed prologue; errors surface
far from their cause; lazy chains pin their inputs alive; printing a matrix in a
debugger triggers computation; performance acquires cliffs at unpredictable points.

**What we do instead:** expose the full `gemm` as the single product intrinsic and name
its common specializations with script wrappers (see "Matrix product"). That captures
essentially all of the win — `AᵀB`, `A·Bᵀ`, and `αA·B + βC` all reach the kernel with zero
intermediate allocations — while staying eager, predictable, and alias-safe. The only
thing genuinely lost is fusion *across* operation types, and the important instance of
that (`A·B + C`) is already reachable via `beta`.

---

## Why 2D only

The three use cases are natively 2D: batch x features, entities x components,
points x 3-or-4. N-D drags in general broadcasting, batched matmul, and axis permutation
— collectively the most confusing part of numpy's surface, against MiniScript's
readability ethos.

The cases wanting more dimensions have decent 2D spellings:

- **Images** `(H, W, C)` → `(H*W) x C` is arguably *more* natural: a list of pixels, each
  a row of channels. Column ops become per-channel ops.
- **Conv nets** `(N, C, H, W)` → im2col + 2D matmul, which is what CPU conv
  implementations do internally anyway. If conv ever matters, add `im2col` as a
  primitive rather than adding a dimension to the type.

Because `reshape` is O(1), users can freely reinterpret a buffer as a collapsed N-D
array and do the leading-index arithmetic themselves. Document that idiom.

---

## Use cases, sketched

### NN inference

```
// y = softmax(relu(x*W1 + b1) * W2 + b2)
h = x.gemm(W1, b1)    // x·W1 + b1, bias broadcast — one kernel pass
h.relu   // equivalent to h.clamp(0, null)
y = h.gemm(W2, b2)
y.softmax
```

Weights load from one RawData blob via sequential `readRawData` calls. No scripted loop
anywhere in the forward pass.

### Physics (n entities)

```
pos = Matrix.ofSize(n, 2)
vel = Matrix.ofSize(n, 2)
acc = Matrix.ofSize(n, 2)

// per frame:
acc.fill 0
acc.add gravity           // gravity is 1 x 2, broadcast down all rows
vel.addScaled acc, dt
pos.addScaled vel, dt

// spawn / despawn:
i = pos.addRow([x, y]); vel.addRow [vx, vy]; acc.addRows 1
pos.removeRowFast i; vel.removeRowFast i; acc.removeRowFast i
```

`reserve` up front means steady-state allocation is zero. `removeRowFast` is O(columns).
Parallel matrices must be resized in lockstep — a thin MiniScript `EntityPool` wrapper
should own that invariant.

### Game linear algebra

```
worldPts = localPts.times(modelMatrix)      // n x 4 times 4 x 4
dirs.normalizeRows
sims = dirsA.rowDot(dirsB)
```

---

## Higher layers, in MiniScript, on top of this

### Neural network library (`nn.ms`)

A **layer library**, not autodifferentiation. `Dense`, `ReLU`, `Sigmoid`, `Softmax`, `MSELoss`,
each with `forward` and `backward`; `Sequential` runs the list forward, then backward.
~150 lines of MiniScript, zero additional C++. Each derivative is written once by the
library author in closed form, so users never hand-derive anything. (Keras worked this
way for years before eager mode.)

The dense backward pass is four calls, which is why the primitives above are shaped as
they are:

```
dZ = dA.elemTimes(actDeriv)
dW = X.transposedTimes(dZ)
db = dZ.sumRows
dX = dZ.timesTransposed(W)
```

**The loss layers own the reduction choice, and it lives in exactly one place.** The
intrinsics deliberately return unreduced per-sample losses (see `softmaxCrossEntropy`),
so `nn.ms` must decide what a reported loss means and keep its gradient consistent. The
convention to adopt: **losses report the mean over the batch, and the corresponding
`backward` scales the gradient by `1/n`** — so `softmaxCrossEntropyGrad`, which is the
gradient of the *sum*, gets divided by the batch size inside the loss layer and nowhere
else. Users then never see the scaling, and it cannot silently disagree with the loss
they are watching. Document it on the loss layers, not just in the code.

Ship a **`gradCheck`** helper (finite differences vs. the analytic gradient) alongside it.
That's the difference between "manual derivatives are practical" and "manual derivatives
are a nightmare" — and it is precisely what catches a reduction/gradient mismatch if the
convention above ever slips.

**Real tape-based autodiff would also need no C++ changes.** A tape records one entry per
*matrix op* — a few hundred per forward pass, not per element — so interpreter overhead is
noise next to the matmuls. It could be a pure-MiniScript `Tensor` wrapper written later,
by us or by a user. Worth not foreclosing; not worth building now.

### Physics

An `EntityPool` class owning a set of parallel matrices, keeping their row counts in
lockstep, and handling the index remapping that `removeRowFast` implies. Integrators
(Euler, Verlet) are a handful of `addScaled` calls.

### Others

Statistics / curve fitting (`solve` + `transposedTimes` gives least squares directly),
image kernels via `reshape` + column ops, pathfinding cost grids.
