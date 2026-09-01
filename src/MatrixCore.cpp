//
//  MatrixCore.cpp
//  raylib-miniscript
//
//  The Matrix back end: storage and handles, the gemm kernels, reductions,
//  linear algebra, the NN primitives, and RawData serialization.  Nothing here
//  knows about the intrinsic table -- no parameter names, no defaults, no
//  `context.GetVar`, which appears only in MatrixClass.cpp.  What does still
//  cross over is a `Value` operand where the API deliberately accepts a scalar,
//  a Matrix, or a list (Gemm's addend, Solve's b, RowCross's operand,
//  SoftmaxCrossEntropy's targets), and a `Value* outErr` for failures, since
//  MiniScript 2 errors are values.  See MatrixInternal.h for the seam, and
//  notes/MATRIX_DESIGN.md for the design.
//

#include "MatrixInternal.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>

namespace MiniScript {

const long kMaxMatrixElems = 32L * 1024L * 1024L;   // 256MB of doubles

namespace MatrixInternal {

// A GC-backed string Value cannot be built at static-init time (before
// GCManager exists), so the map keys are made lazily on first use.  Interned
// (< 128 bytes) and therefore immortal, so they are safe to hold.
const Value& kHandle()  { static Value v("_handle"); return v; }
const Value& kRows()    { static Value v("rows");    return v; }
const Value& kColumns() { static Value v("columns"); return v; }

//--------------------------------------------------------------------------------
// Storage
//--------------------------------------------------------------------------------

// Allocate `elems` doubles, retrying once after a collection.  A failure here
// may only mean that unreachable matrices are still waiting to be swept, since
// a buffer is not freed until its handle is collected.  One retry only: if a
// full mark-sweep did not free enough, nothing is reclaimable.
double* AllocElems(long elems) {
	if (elems <= 0) return nullptr;
	double* p = (double*)calloc((size_t)elems, sizeof(double));
	if (p != nullptr) return p;
	GCManager::CollectGarbage();
	return (double*)calloc((size_t)elems, sizeof(double));
}

// Build storage for a rows x columns matrix, all zeros.  Returns null if the
// size is out of range or the allocation failed; the caller turns that into an
// error value (it has the context to say which).
MatrixData* NewMatrixData(int rows, int columns) {
	MatrixData* m = new MatrixData();
	m->rows = rows;
	m->columns = columns;
	long elems = (long)rows * (long)columns;
	m->capacityElems = elems;
	if (elems > 0) {
		m->data = AllocElems(elems);
		if (m->data == nullptr) { delete m; return nullptr; }
	}
	return m;
}

// Build a size x size identity.  Returns null on the same terms as
// NewMatrixData, which does the zero-filling; only the diagonal is left.
MatrixData* NewIdentity(int size) {
	MatrixData* m = NewMatrixData(size, size);
	if (m == nullptr) return nullptr;
	for (int i = 0; i < size; i++) m->data[(long)i * size + i] = 1.0;
	return m;
}

// Make sure at least `needed` elements are addressable, growing if not.
//
// Growth policy: newCap = max(2*oldCap, needed), so repeatedly appending rows
// is amortized O(1).  Capacity is NEVER shrunk here -- a pool oscillating
// between 900 and 1000 rows should allocate zero times after warmup; `trim`
// (not yet implemented) is the explicit release.
//
// Returns false if the request is out of range or the allocation failed.  New
// capacity beyond the old is zeroed, so a caller that then extends the live
// region sees zeros rather than whatever was there before.
bool EnsureCapacity(MatrixData* m, long needed) {
	if (needed <= m->capacityElems) return true;
	if (needed > kMaxMatrixElems) return false;
	long newCap = m->capacityElems * 2;
	if (newCap < needed) newCap = needed;
	if (newCap > kMaxMatrixElems) newCap = kMaxMatrixElems;

	double* p = (double*)realloc(m->data, (size_t)newCap * sizeof(double));
	if (p == nullptr) {
		GCManager::CollectGarbage();
		p = (double*)realloc(m->data, (size_t)newCap * sizeof(double));
		if (p == nullptr) return false;
	}
	memset(p + m->capacityElems, 0, (size_t)(newCap - m->capacityElems) * sizeof(double));
	m->data = p;
	m->capacityElems = newCap;
	return true;
}

static Value MakeHandle(MatrixData* data) {
	return Value::NewHandle(data, [](void* p) { delete (MatrixData*)p; });
}

// Rewrite the instance map's rows/columns entries after a shape change.  They
// are plain map entries for lookup speed (see MatrixToValue), which means every
// resize has to write them back or `m.rows` would report the old shape.
void SyncShape(Value self, MatrixData* m) {
	if (self.Type() != ValueType::Map) return;
	ValueDict map = self.GetDict();
	map.SetValue(kRows(), Value(m->rows));
	map.SetValue(kColumns(), Value(m->columns));
}

// Work out the shape a list argument denotes, by the same rules as
// Matrix.fromList: a 1D list of numbers is 1 x n, a list of lists is n x m, and
// [] is 0 x 0.  Validates that the input is rectangular and entirely numeric,
// so callers can read elements afterward without re-checking.
//
// Shared so that everywhere a Matrix is accepted, a list means exactly what
// fromList would have made of it -- one rule to learn, and no chance of the two
// interpretations drifting apart.
bool ListShape(ValueList outer, bool* outNested, int* outRows, int* outCols,
                      const char* who, Value* outErr) {
	int count = outer.Count();
	if (count == 0) { *outNested = false; *outRows = 0; *outCols = 0; return true; }

	bool nested = (outer[0].Type() == ValueType::List);
	int rows, columns;
	if (nested) {
		rows = count;
		columns = outer[0].GetList().Count();
		for (int r = 0; r < rows; r++) {
			if (outer[r].Type() != ValueType::List) {
				*outErr = ErrorTypes::RuntimeError(
					String("Matrix.") + who + ": mixed list and non-list rows");
				return false;
			}
			if (outer[r].GetList().Count() != columns) {
				*outErr = ErrorTypes::RuntimeError(
					String("Matrix.") + who + ": rows have differing lengths");
				return false;
			}
		}
	} else {
		rows = 1;
		columns = count;
	}
	for (int r = 0; r < rows; r++) {
		ValueList row = nested ? outer[r].GetList() : outer;
		for (int c = 0; c < columns; c++) {
			if (row[c].Type() != ValueType::Number) {
				*outErr = ErrorTypes::TypeError("number", row[c]);
				return false;
			}
		}
	}
	*outNested = nested;
	*outRows = rows;
	*outCols = columns;
	return true;
}

// Element (r, c) of a list already validated by ListShape.
double ListElem(ValueList outer, bool nested, int r, int c) {
	return (nested ? outer[r].GetList()[c] : outer[c]).DoubleValue();
}

//--------------------------------------------------------------------------------
// gemm
//--------------------------------------------------------------------------------
//
// One routine behind every product and every add/scale in the API:
//
//     out := alpha * op(A) * op(B) + beta * addend
//
// with op(X) being X or its transpose.  A null B collapses this to
// alpha*op(A) + beta*addend, which is how plus/minus/negate/clone/transpose are
// all expressed.  See notes/MATRIX_DESIGN.md.

// Scratch for de-aliasing.  Static and thread-local, grown on demand and never
// shrunk, so a steady-state loop that aliases allocates zero times after
// warmup.  There is no reentrancy hazard: gemm never calls back into MiniScript.
//
// Call this ONCE per gemm with the total needed, then carve segments out of the
// result -- a second call may realloc and invalidate the first pointer.
static double* Scratch(long elems) {
	static thread_local double* buf = nullptr;
	static thread_local long cap = 0;
	if (elems <= cap) return buf;
	double* p = (double*)realloc(buf, (size_t)elems * sizeof(double));
	if (p == nullptr) {
		GCManager::CollectGarbage();
		p = (double*)realloc(buf, (size_t)elems * sizeof(double));
		if (p == nullptr) return nullptr;
	}
	buf = p;
	cap = elems;
	return buf;
}

// A read-only snapshot of an operand: dimensions plus contiguous data with
// stride == columns.  Taken BEFORE `out` is reshaped, because an operand that
// aliases `out` is the very same MatrixData -- reshaping out would change this
// operand's dimensions out from under us.
struct Operand {
	const double* data = nullptr;
	int rows = 0;
	int columns = 0;
	long Elems() const { return (long)rows * (long)columns; }
};

// Write beta*addend into the m x n result, or zeros when there is nothing to
// add.  The kernels then accumulate onto this, which is what lets one code path
// serve both the "fresh result" and "accumulate into existing" cases.
//
// beta == 0 must not read the addend at all -- fresh storage may hold anything,
// and 0*NaN is NaN.  Same contract as BLAS.
static void Prefill(double* out, int m, int n, const Operand& add,
                    AddendMode mode, double beta, double scalar) {
	long total = (long)m * n;
	if (mode == kAddendNone) {
		memset(out, 0, (size_t)total * sizeof(double));
		return;
	}
	switch (mode) {
		case kAddendScalar: {
			double v = beta * scalar;
			for (long i = 0; i < total; i++) out[i] = v;
			break;
		}
		case kAddendFull:
			for (long i = 0; i < total; i++) out[i] = beta * add.data[i];
			break;
		case kAddendRow:
			for (int i = 0; i < m; i++) {
				double* crow = out + (long)i * n;
				for (int j = 0; j < n; j++) crow[j] = beta * add.data[j];
			}
			break;
		case kAddendCol:
			for (int i = 0; i < m; i++) {
				double v = beta * add.data[i];
				double* crow = out + (long)i * n;
				for (int j = 0; j < n; j++) crow[j] = v;
			}
			break;
		default:
			break;
	}
}

// Level-1: out += alpha * op(A).  Reached whenever B is null.
//
// This is the reason gemm early-dispatches instead of funnelling everything
// through the matmul: add, plus, subtract, negate, addScaled, clone, copyFrom,
// transpose and transposed are ALL defined as gemm with a null B, and they are
// bandwidth-bound work that must not pay matmul setup.
static void KernelAxpby(double* out, int m, int n, const Operand& A, bool transA, double alpha) {
	if (!transA) {
		long total = (long)m * n;
		for (long i = 0; i < total; i++) out[i] += alpha * A.data[i];
	} else {
		// Transpose-copy: the flag does not transpose anything, it changes the
		// indexing.  out is n_A-rows by m_A-cols reversed, so walk A by column.
		for (int i = 0; i < m; i++) {
			double* crow = out + (long)i * n;
			for (int j = 0; j < n; j++) crow[j] += alpha * A.data[(long)j * A.columns + i];
		}
	}
}

// Level-3: out += alpha * op(A) * op(B), with (m x k) times (k x n).
//
// Three of the four transpose combinations have a formulation whose inner loop
// walks contiguous memory, which is why transposedTimes and timesTransposed
// cost no allocation and no copy:
//   NN  i-k-j : broadcast A[i][k] over a contiguous run of B's row
//   NT  i-j-k : a dot product of two contiguous rows
//   TN  k-i-j : a rank-1 update over contiguous rows of both
// TT has no such form and falls back to strided indexing; it is also the one
// combination nothing in the API actually asks for.
static void KernelMatmul(double* out, int m, int n, int k,
                         const Operand& A, bool transA,
                         const Operand& B, bool transB, double alpha) {
	if (!transA && !transB) {
		for (int i = 0; i < m; i++) {
			double* crow = out + (long)i * n;
			const double* arow = A.data + (long)i * A.columns;
			for (int kk = 0; kk < k; kk++) {
				double aik = alpha * arow[kk];
				if (aik == 0.0) continue;
				const double* brow = B.data + (long)kk * B.columns;
				for (int j = 0; j < n; j++) crow[j] += aik * brow[j];
			}
		}
	} else if (!transA && transB) {
		for (int i = 0; i < m; i++) {
			double* crow = out + (long)i * n;
			const double* arow = A.data + (long)i * A.columns;
			for (int j = 0; j < n; j++) {
				const double* brow = B.data + (long)j * B.columns;
				double sum = 0.0;
				for (int kk = 0; kk < k; kk++) sum += arow[kk] * brow[kk];
				crow[j] += alpha * sum;
			}
		}
	} else if (transA && !transB) {
		for (int kk = 0; kk < k; kk++) {
			const double* arow = A.data + (long)kk * A.columns;
			const double* brow = B.data + (long)kk * B.columns;
			for (int i = 0; i < m; i++) {
				double a = alpha * arow[i];
				if (a == 0.0) continue;
				double* crow = out + (long)i * n;
				for (int j = 0; j < n; j++) crow[j] += a * brow[j];
			}
		}
	} else {
		for (int i = 0; i < m; i++) {
			double* crow = out + (long)i * n;
			for (int j = 0; j < n; j++) {
				double sum = 0.0;
				for (int kk = 0; kk < k; kk++) {
					sum += A.data[(long)kk * A.columns + i] * B.data[(long)j * B.columns + kk];
				}
				crow[j] += alpha * sum;
			}
		}
	}
}

// The whole of gemm, once the caller has unpacked its arguments:
//
//     out := alpha * op(A) * op(B) + beta * addend
//
// B may be null (the level-1 path).  `addend` is still a Value here because it
// alone may be a list; every other operand arrives resolved.  `out` may be null,
// which allocates a fresh result; otherwise it is reshaped and written in place.
//
// Returns the result (the given `out`, when one was given), or nullptr with
// *outErr set.
MatrixData* Gemm(MatrixData* A, MatrixData* B, Value vAdd, MatrixData* out,
                        bool transA, bool transB, double alpha, double beta,
                        Value* outErr) {
	// ---- the result shape ----
	int m = transA ? A->columns : A->rows;
	int kA = transA ? A->rows : A->columns;
	int n, k = 0;
	if (B != nullptr) {
		int kB = transB ? B->columns : B->rows;
		n = transB ? B->rows : B->columns;
		if (kA != kB) {
			*outErr = ErrorTypes::RuntimeError("Matrix.gemm: inner dimensions do not match");
			return nullptr;
		}
		k = kA;
	} else {
		n = kA;
	}

	// ---- addend, and how it broadcasts onto m x n ----
	AddendMode mode = kAddendNone;
	double addScalar = 0.0;
	MatrixData* addM = nullptr;
	ValueList addList;
	bool addNested = false;
	int addRows = 0, addCols = 0;
	if (!vAdd.IsNull() && beta != 0.0) {
		if (vAdd.Type() == ValueType::Number) {
			mode = kAddendScalar;
			addScalar = vAdd.DoubleValue();
		} else {
			if (vAdd.Type() == ValueType::List) {
				addList = vAdd.GetList();
				if (!ListShape(addList, &addNested, &addRows, &addCols, "gemm", outErr)) return nullptr;
			} else {
				addM = ValueToMatrix(vAdd);
				if (addM == nullptr) {
					*outErr = ErrorTypes::RuntimeError(
						"Matrix.gemm: addend must be null, a number, a Matrix, or a list");
					return nullptr;
				}
				addRows = addM->rows;
				addCols = addM->columns;
			}
			if (addRows == 1 && addCols == 1) {
				// A 1x1 operand is a scalar wearing a matrix's clothes.  Read
				// the value out here; the scalar path never looks at opAdd.data.
				mode = kAddendScalar;
				addScalar = addM != nullptr ? addM->data[0]
				                            : ListElem(addList, addNested, 0, 0);
			}
			else if (addRows == m && addCols == n)          mode = kAddendFull;
			else if (addRows == 1 && addCols == n)          mode = kAddendRow;
			else if (addRows == m && addCols == 1)          mode = kAddendCol;
			else {
				*outErr = ErrorTypes::RuntimeError(
					"Matrix.gemm: addend does not match or broadcast to the result shape");
				return nullptr;
			}
		}
	}

	// ---- snapshot anything that aliases `out` ----
	//
	// An operand aliases `out` only by being the same MatrixData (storage is
	// never shared otherwise), and then reshaping `out` would move its data
	// and rewrite its dimensions.  So copy first, and record the dimensions
	// as they are NOW.  A list addend is copied here too, which lets the
	// prefill treat every addend uniformly as contiguous doubles.
	Operand opA, opB, opAdd;
	opA.rows = A->rows; opA.columns = A->columns;
	if (B != nullptr) { opB.rows = B->rows; opB.columns = B->columns; }
	opAdd.rows = addRows; opAdd.columns = addCols;

	bool addIsList = !addList.Empty();
	bool copyA = (out != nullptr && A == out);
	bool copyB = (B != nullptr && out != nullptr && B == out);
	bool copyAdd = (mode != kAddendNone && mode != kAddendScalar)
	               && (addIsList || (addM != nullptr && out != nullptr && addM == out));

	long needA = copyA ? opA.Elems() : 0;
	long needB = copyB ? opB.Elems() : 0;
	long needAdd = copyAdd ? opAdd.Elems() : 0;
	long total = needA + needB + needAdd;
	double* scratch = nullptr;
	if (total > 0) {
		scratch = Scratch(total);
		if (scratch == nullptr) {
			*outErr = ErrorTypes::RuntimeError("Matrix.gemm: out of memory");
			return nullptr;
		}
	}
	long at = 0;
	if (copyA) { memcpy(scratch + at, A->data, (size_t)needA * sizeof(double)); opA.data = scratch + at; at += needA; }
	else opA.data = A->data;
	if (B != nullptr) {
		if (copyB) { memcpy(scratch + at, B->data, (size_t)needB * sizeof(double)); opB.data = scratch + at; at += needB; }
		else opB.data = B->data;
	}
	if (copyAdd) {
		double* dst = scratch + at;
		if (addIsList) {
			for (int r = 0; r < addRows; r++) {
				for (int c = 0; c < addCols; c++) {
					dst[(long)r * addCols + c] = ListElem(addList, addNested, r, c);
				}
			}
		} else {
			memcpy(dst, addM->data, (size_t)needAdd * sizeof(double));
		}
		opAdd.data = dst;
		at += needAdd;
	} else if (addM != nullptr) {
		opAdd.data = addM->data;
	}

	// ---- shape the destination ----
	if (out == nullptr) {
		if ((long)m * n > kMaxMatrixElems) {
			*outErr = ErrorTypes::RuntimeError("Matrix.gemm: result exceeds the maximum matrix size");
			return nullptr;
		}
		out = NewMatrixData(m, n);
		if (out == nullptr) {
			*outErr = ErrorTypes::RuntimeError("Matrix.gemm: out of memory");
			return nullptr;
		}
	} else {
		if (!EnsureCapacity(out, (long)m * n)) {
			*outErr = ErrorTypes::RuntimeError("Matrix.gemm: out of memory");
			return nullptr;
		}
		out->rows = m;
		out->columns = n;
	}

	// ---- compute ----
	if ((long)m * n > 0) {
		Prefill(out->data, m, n, opAdd, mode, beta, addScalar);
		if (B == nullptr) {
			KernelAxpby(out->data, m, n, opA, transA, alpha);
		} else {
			KernelMatmul(out->data, m, n, k, opA, transA, opB, transB, alpha);
		}
	}
	return out;
}

// Resolve such an operand.  A list is materialized into the shared scratch
// buffer, so callers must resolve at most ONE broadcast operand per call -- a
// second Scratch() call may realloc and invalidate the first pointer.
bool ResolveBroadcast(Value v, int m, int n, const char* who,
                      Broadcast* out, Value* outErr) {
	if (v.Type() == ValueType::Number) {
		out->mode = kAddendScalar;
		out->scalar = v.DoubleValue();
		return true;
	}
	int rows = 0, cols = 0;
	const double* data = nullptr;
	if (v.Type() == ValueType::List) {
		ValueList list = v.GetList();
		bool nested = false;
		if (!ListShape(list, &nested, &rows, &cols, who, outErr)) return false;
		long need = (long)rows * cols;
		if (need > 0) {
			double* buf = Scratch(need);
			if (buf == nullptr) {
				*outErr = ErrorTypes::RuntimeError(String("Matrix.") + who + ": out of memory");
				return false;
			}
			for (int r = 0; r < rows; r++) {
				for (int c = 0; c < cols; c++) {
					buf[(long)r * cols + c] = ListElem(list, nested, r, c);
				}
			}
			data = buf;
		}
	} else {
		MatrixData* md = ValueToMatrix(v);
		if (md == nullptr) {
			*outErr = ErrorTypes::RuntimeError(
				String("Matrix.") + who + ": expected a number, a Matrix, or a list");
			return false;
		}
		rows = md->rows; cols = md->columns; data = md->data;
	}
	out->data = data;
	out->stride = cols;
	if (rows == 1 && cols == 1) {
		out->mode = kAddendScalar;
		out->scalar = (data != nullptr) ? data[0] : 0.0;
	}
	else if (rows == m && cols == n) out->mode = kAddendFull;
	else if (rows == 1 && cols == n) out->mode = kAddendRow;
	else if (rows == m && cols == 1) out->mode = kAddendCol;
	else {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": operand does not match or broadcast to the matrix shape");
		return false;
	}
	return true;
}

// ---- Reductions ----

bool ReduceNeedsElement(int op) {
	return op == kReduceMax || op == kReduceMin || op == kReduceArgMax || op == kReduceArgMin;
}

// Reduce `count` elements starting at `base`, `stride` apart.  For the arg
// forms the return value is an index into that run.
//
// Ties go to the FIRST occurrence (the comparison is strict), and NaN
// propagates: any NaN makes max/min NaN and makes argmax/argmin report that
// NaN's position.  Both match numpy, whose nan-skipping behavior lives in
// separate nanmax/nanargmax functions.
double ReduceRun(const double* base, long count, long stride, int op) {
	switch (op) {
		case kReduceSum: {
			double s = 0.0;
			for (long i = 0; i < count; i++) s += base[i * stride];
			return s;
		}
		case kReduceSumSq: {
			double s = 0.0;
			for (long i = 0; i < count; i++) { double v = base[i * stride]; s += v * v; }
			return s;
		}
		case kReduceMax:
		case kReduceMin: {
			double best = base[0];
			if (best != best) return best;
			for (long i = 1; i < count; i++) {
				double v = base[i * stride];
				if (v != v) return v;
				if (op == kReduceMax ? (v > best) : (v < best)) best = v;
			}
			return best;
		}
		default: {
			double best = base[0];
			long bi = 0;
			if (best != best) return 0.0;
			for (long i = 1; i < count; i++) {
				double v = base[i * stride];
				if (v != v) return (double)i;
				if (op == kReduceArgMax ? (v > best) : (v < best)) { best = v; bi = i; }
			}
			return (double)bi;
		}
	}
}

//--------------------------------------------------------------------------------
// Linear algebra
//--------------------------------------------------------------------------------

// Pivot bookkeeping, kept separate from Scratch so one routine can hold both at
// once.  Same grow-only, never-shrink, thread-local discipline.
static int* ScratchInts(int count) {
	static thread_local int* buf = nullptr;
	static thread_local int cap = 0;
	if (count <= cap) return buf;
	int* p = (int*)realloc(buf, (size_t)count * sizeof(int));
	if (p == nullptr) {
		GCManager::CollectGarbage();
		p = (int*)realloc(buf, (size_t)count * sizeof(int));
		if (p == nullptr) return nullptr;
	}
	buf = p;
	cap = count;
	return buf;
}

// ---- Closed forms for 1x1 .. 4x4 ----
//
// Not merely an optimization.  These sizes are what game code actually asks
// for -- a 4x4 transform inverted every frame -- and cofactor expansion beats
// pivoted elimination on both counts there: fewer operations, and no pivot
// search to make the cost depend on the values.  Above 4x4 the operation count
// of cofactor expansion explodes and LU takes over.

// The six 2x2 minors of the top two rows and the six of the bottom two rows.
// Shared by Det4 and Inverse4: the determinant is a combination of them, and so
// is every entry of the adjugate, so computing the inverse costs barely more
// than computing the determinant alone.
struct Minors4 {
	double s[6];
	double c[6];
	double det;
};

static Minors4 Compute4(const double* a) {
	Minors4 r;
	r.s[0] = a[0]*a[5]  - a[4]*a[1];
	r.s[1] = a[0]*a[6]  - a[4]*a[2];
	r.s[2] = a[0]*a[7]  - a[4]*a[3];
	r.s[3] = a[1]*a[6]  - a[5]*a[2];
	r.s[4] = a[1]*a[7]  - a[5]*a[3];
	r.s[5] = a[2]*a[7]  - a[6]*a[3];
	r.c[0] = a[8]*a[13] - a[12]*a[9];
	r.c[1] = a[8]*a[14] - a[12]*a[10];
	r.c[2] = a[8]*a[15] - a[12]*a[11];
	r.c[3] = a[9]*a[14] - a[13]*a[10];
	r.c[4] = a[9]*a[15] - a[13]*a[11];
	r.c[5] = a[10]*a[15] - a[14]*a[11];
	r.det = r.s[0]*r.c[5] - r.s[1]*r.c[4] + r.s[2]*r.c[3]
	      + r.s[3]*r.c[2] - r.s[4]*r.c[1] + r.s[5]*r.c[0];
	return r;
}

static double DetSmall(const double* a, int n) {
	switch (n) {
		case 0: return 1.0;                       // the empty product, as in numpy
		case 1: return a[0];
		case 2: return a[0]*a[3] - a[1]*a[2];
		case 3: return a[0] * (a[4]*a[8] - a[5]*a[7])
		             - a[1] * (a[3]*a[8] - a[5]*a[6])
		             + a[2] * (a[3]*a[7] - a[4]*a[6]);
		default: return Compute4(a).det;
	}
}

// Write the inverse of the n x n `a` (n <= 4) into `out`.  Returns false when
// the determinant is exactly zero; a near-zero determinant is the caller's
// problem, as it is with any adjugate formula.
static bool InverseSmall(const double* a, int n, double* out) {
	if (n == 0) return true;
	if (n == 1) {
		if (a[0] == 0.0) return false;
		out[0] = 1.0 / a[0];
		return true;
	}
	if (n == 2) {
		double det = a[0]*a[3] - a[1]*a[2];
		if (det == 0.0) return false;
		double f = 1.0 / det;
		out[0] =  a[3]*f; out[1] = -a[1]*f;
		out[2] = -a[2]*f; out[3] =  a[0]*f;
		return true;
	}
	if (n == 3) {
		double m0 = a[4]*a[8] - a[5]*a[7];
		double m1 = a[3]*a[8] - a[5]*a[6];
		double m2 = a[3]*a[7] - a[4]*a[6];
		double det = a[0]*m0 - a[1]*m1 + a[2]*m2;
		if (det == 0.0) return false;
		double f = 1.0 / det;
		out[0] =  m0 * f;
		out[1] = -(a[1]*a[8] - a[2]*a[7]) * f;
		out[2] =  (a[1]*a[5] - a[2]*a[4]) * f;
		out[3] = -m1 * f;
		out[4] =  (a[0]*a[8] - a[2]*a[6]) * f;
		out[5] = -(a[0]*a[5] - a[2]*a[3]) * f;
		out[6] =  m2 * f;
		out[7] = -(a[0]*a[7] - a[1]*a[6]) * f;
		out[8] =  (a[0]*a[4] - a[1]*a[3]) * f;
		return true;
	}
	Minors4 k = Compute4(a);
	if (k.det == 0.0) return false;
	double f = 1.0 / k.det;
	const double* s = k.s;
	const double* c = k.c;
	out[0]  = ( a[5]*c[5] - a[6]*c[4] + a[7]*c[3]) * f;
	out[1]  = (-a[1]*c[5] + a[2]*c[4] - a[3]*c[3]) * f;
	out[2]  = ( a[13]*s[5] - a[14]*s[4] + a[15]*s[3]) * f;
	out[3]  = (-a[9]*s[5] + a[10]*s[4] - a[11]*s[3]) * f;
	out[4]  = (-a[4]*c[5] + a[6]*c[2] - a[7]*c[1]) * f;
	out[5]  = ( a[0]*c[5] - a[2]*c[2] + a[3]*c[1]) * f;
	out[6]  = (-a[12]*s[5] + a[14]*s[2] - a[15]*s[1]) * f;
	out[7]  = ( a[8]*s[5] - a[10]*s[2] + a[11]*s[1]) * f;
	out[8]  = ( a[4]*c[4] - a[5]*c[2] + a[7]*c[0]) * f;
	out[9]  = (-a[0]*c[4] + a[1]*c[2] - a[3]*c[0]) * f;
	out[10] = ( a[12]*s[4] - a[13]*s[2] + a[15]*s[0]) * f;
	out[11] = (-a[8]*s[4] + a[9]*s[2] - a[11]*s[0]) * f;
	out[12] = (-a[4]*c[3] + a[5]*c[1] - a[6]*c[0]) * f;
	out[13] = ( a[0]*c[3] - a[1]*c[1] + a[2]*c[0]) * f;
	out[14] = (-a[12]*s[3] + a[13]*s[1] - a[14]*s[0]) * f;
	out[15] = ( a[8]*s[3] - a[9]*s[1] + a[10]*s[0]) * f;
	return true;
}

// ---- LU with partial pivoting, for everything above 4x4 ----

// Factor the n x n `a` in place: unit-lower L below the diagonal, U on and
// above it.  perm[i] is the original row now sitting at row i, and *outSign is
// the parity of that permutation (+1 or -1), which the determinant needs.
//
// Returns false when some column was entirely zero at and below the diagonal --
// i.e. the matrix is singular.  The factorization still completes, with a zero
// on the diagonal, so a determinant can be read off it either way.
static bool LUDecompose(double* a, int n, int* perm, double* outSign) {
	double sign = 1.0;
	bool ok = true;
	for (int i = 0; i < n; i++) perm[i] = i;
	for (int col = 0; col < n; col++) {
		int best = col;
		double bestAbs = std::fabs(a[(long)col * n + col]);
		for (int r = col + 1; r < n; r++) {
			double v = std::fabs(a[(long)r * n + col]);
			if (v > bestAbs) { bestAbs = v; best = r; }
		}
		if (!(bestAbs > 0.0)) {
			// All zero (or NaN) in this column: nothing to eliminate with.
			ok = false;
			continue;
		}
		if (best != col) {
			double* rowA = a + (long)col * n;
			double* rowB = a + (long)best * n;
			for (int j = 0; j < n; j++) {
				double t = rowA[j]; rowA[j] = rowB[j]; rowB[j] = t;
			}
			int t = perm[col]; perm[col] = perm[best]; perm[best] = t;
			sign = -sign;
		}
		const double* prow = a + (long)col * n;
		double pivot = prow[col];
		for (int r = col + 1; r < n; r++) {
			double* rrow = a + (long)r * n;
			double factor = rrow[col] / pivot;
			rrow[col] = factor;
			if (factor == 0.0) continue;
			for (int j = col + 1; j < n; j++) rrow[j] -= factor * prow[j];
		}
	}
	*outSign = sign;
	return ok;
}

// Solve A*X = B given A's factorization, for all k columns of B at once.  B
// arrives in `b` (n x k) and leaves holding X.  `work` is n*k scratch used to
// apply the row permutation.
//
// Doing every right-hand side in one pass is what makes `inverse` a single
// factorization plus one solve against the identity, rather than n solves.
static void LUSolve(const double* lu, int n, const int* perm, double* b, int k, double* work) {
	for (int i = 0; i < n; i++) {
		memcpy(work + (long)i * k, b + (long)perm[i] * k, (size_t)k * sizeof(double));
	}
	memcpy(b, work, (size_t)n * k * sizeof(double));

	for (int i = 1; i < n; i++) {                 // forward: L has a unit diagonal
		double* brow = b + (long)i * k;
		const double* lrow = lu + (long)i * n;
		for (int j = 0; j < i; j++) {
			double factor = lrow[j];
			if (factor == 0.0) continue;
			const double* bj = b + (long)j * k;
			for (int c = 0; c < k; c++) brow[c] -= factor * bj[c];
		}
	}
	for (int i = n - 1; i >= 0; i--) {            // back: divide through by U's diagonal
		double* brow = b + (long)i * k;
		const double* urow = lu + (long)i * n;
		for (int j = i + 1; j < n; j++) {
			double factor = urow[j];
			if (factor == 0.0) continue;
			const double* bj = b + (long)j * k;
			for (int c = 0; c < k; c++) brow[c] -= factor * bj[c];
		}
		double d = urow[i];
		for (int c = 0; c < k; c++) brow[c] /= d;
	}
}

// ---- The three operations the intrinsics wrap ----

// m.determinant.  A singular matrix has determinant 0; that is an answer, not
// an error, and numpy agrees.
bool Determinant(const MatrixData* m, double* out, Value* outErr) {
	int n = m->rows;
	if (m->columns != n) {
		*outErr = ErrorTypes::RuntimeError("Matrix.determinant: matrix must be square");
		return false;
	}
	if (n <= 4) {
		*out = DetSmall(m->data, n);
		return true;
	}
	double* lu = Scratch((long)n * n);
	int* perm = ScratchInts(n);
	if (lu == nullptr || perm == nullptr) {
		*outErr = ErrorTypes::RuntimeError("Matrix.determinant: out of memory");
		return false;
	}
	memcpy(lu, m->data, (size_t)n * n * sizeof(double));
	double sign = 1.0;
	LUDecompose(lu, n, perm, &sign);
	double det = sign;
	for (int i = 0; i < n; i++) det *= lu[(long)i * n + i];
	*out = det;
	return true;
}

// m.inverse -> a new Matrix, or nullptr with *outErr set.
//
// Singular is an error value here rather than a quiet NaN-filled result, which
// is what numpy does (LinAlgError) and what a caller almost always wants: an
// un-invertible transform is a bug upstream, not a value to propagate.
MatrixData* Inverse(const MatrixData* m, Value* outErr) {
	int n = m->rows;
	if (m->columns != n) {
		*outErr = ErrorTypes::RuntimeError("Matrix.inverse: matrix must be square");
		return nullptr;
	}
	MatrixData* out = NewMatrixData(n, n);
	if (out == nullptr) {
		*outErr = ErrorTypes::RuntimeError("Matrix.inverse: out of memory");
		return nullptr;
	}
	bool ok;
	if (n <= 4) {
		ok = InverseSmall(m->data, n, out->data);
	} else {
		// One factorization, then a single multi-column solve against I.
		long need = (long)n * n * 2;
		double* lu = Scratch(need);
		int* perm = ScratchInts(n);
		if (lu == nullptr || perm == nullptr) {
			delete out;
			*outErr = ErrorTypes::RuntimeError("Matrix.inverse: out of memory");
			return nullptr;
		}
		double* work = lu + (long)n * n;
		memcpy(lu, m->data, (size_t)n * n * sizeof(double));
		double sign = 1.0;
		ok = LUDecompose(lu, n, perm, &sign);
		if (ok) {
			for (int i = 0; i < n; i++) out->data[(long)i * n + i] = 1.0;
			LUSolve(lu, n, perm, out->data, n, work);
		}
	}
	if (!ok) {
		delete out;
		*outErr = ErrorTypes::RuntimeError("Matrix.inverse: matrix is singular");
		return nullptr;
	}
	return out;
}

// m.solve(b) -> a new n x k Matrix X with A*X = b.
//
// `b` may be a Matrix or a list.  A *flat* list of n numbers is read as a
// column vector, not as the 1 x n that fromList would make of it: `solve` is
// the one place where a bare list of numbers unambiguously means "the
// right-hand side of n equations", and numpy reads a 1-D b the same way.  A
// nested list, or a Matrix, must already be n x k.
MatrixData* Solve(const MatrixData* A, Value vB, Value* outErr) {
	int n = A->rows;
	if (A->columns != n) {
		*outErr = ErrorTypes::RuntimeError("Matrix.solve: matrix must be square");
		return nullptr;
	}

	MatrixData* bM = nullptr;
	ValueList bList;
	bool bNested = false;
	int bRows = 0, bCols = 0;
	if (vB.Type() == ValueType::List) {
		bList = vB.GetList();
		if (!ListShape(bList, &bNested, &bRows, &bCols, "solve", outErr)) return nullptr;
		if (!bNested) { bRows = bCols; bCols = 1; }
	} else {
		bM = ValueToMatrix(vB);
		if (bM == nullptr) {
			*outErr = ErrorTypes::RuntimeError("Matrix.solve: b must be a Matrix or a list");
			return nullptr;
		}
		bRows = bM->rows;
		bCols = bM->columns;
	}
	if (bRows != n) {
		*outErr = ErrorTypes::RuntimeError("Matrix.solve: b must have one row per equation");
		return nullptr;
	}

	int k = bCols;
	MatrixData* out = NewMatrixData(n, k);
	if (out == nullptr) {
		*outErr = ErrorTypes::RuntimeError("Matrix.solve: out of memory");
		return nullptr;
	}
	if (bM != nullptr) {
		memcpy(out->data, bM->data, (size_t)n * k * sizeof(double));
	} else if (bNested) {
		for (int r = 0; r < n; r++) {
			for (int c = 0; c < k; c++) out->data[(long)r * k + c] = ListElem(bList, true, r, c);
		}
	} else {
		for (int r = 0; r < n; r++) out->data[r] = ListElem(bList, false, 0, r);
	}

	bool ok = true;
	if (n > 0) {
		double* lu = Scratch((long)n * n + (long)n * k);
		int* perm = ScratchInts(n);
		if (lu == nullptr || perm == nullptr) {
			delete out;
			*outErr = ErrorTypes::RuntimeError("Matrix.solve: out of memory");
			return nullptr;
		}
		double* work = lu + (long)n * n;
		memcpy(lu, A->data, (size_t)n * n * sizeof(double));
		double sign = 1.0;
		ok = LUDecompose(lu, n, perm, &sign);
		if (ok && k > 0) LUSolve(lu, n, perm, out->data, k, work);
	}
	if (!ok) {
		delete out;
		*outErr = ErrorTypes::RuntimeError("Matrix.solve: matrix is singular");
		return nullptr;
	}
	return out;
}

//--------------------------------------------------------------------------------
// Per-row vector ops
//--------------------------------------------------------------------------------

// m.rowCross(m2) -> a new n x 3 Matrix holding the per-row 3-D cross product.
//
// Both operands need exactly 3 columns.  `m2` may have one row per row of self,
// or exactly one row, which is then crossed with every row -- the common game
// case, every velocity crossed against one fixed axis.  That is the same row
// broadcasting the arithmetic ops already do.
//
// Divergence from numpy, deliberate: np.cross broadcasts in both directions, so
// a 1x3 crossed with an nx3 yields n rows.  Here the result always has SELF's
// row count.  `a.rowCross(b)` reads as "for each of my rows", and a method that
// silently returned more rows than the receiver has would be a trap.
//
// The result is freshly allocated, so it can never alias an operand -- which
// matters, because each output row is a function of all three input components.
MatrixData* RowCross(const MatrixData* A, Value vB, Value* outErr) {
	if (A->columns != 3) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix.rowCross: matrix must have exactly 3 columns");
		return nullptr;
	}

	const MatrixData* bM = nullptr;
	ValueList bList;
	bool bNested = false;
	int bRows = 0, bCols = 0;
	if (vB.Type() == ValueType::List) {
		bList = vB.GetList();
		if (!ListShape(bList, &bNested, &bRows, &bCols, "rowCross", outErr)) return nullptr;
	} else {
		bM = ValueToMatrix(vB);
		if (bM == nullptr) {
			*outErr = ErrorTypes::RuntimeError(
				"Matrix.rowCross: expected a Matrix or a list");
			return nullptr;
		}
		bRows = bM->rows;
		bCols = bM->columns;
	}
	if (bCols != 3) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix.rowCross: operand must have exactly 3 columns");
		return nullptr;
	}
	if (bRows != A->rows && bRows != 1) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix.rowCross: operand must have one row per row, or exactly one row");
		return nullptr;
	}

	// Flatten a list operand once, so the kernel below has a single form.
	const double* bData = (bM != nullptr) ? bM->data : nullptr;
	if (bM == nullptr && bRows > 0) {
		double* buf = Scratch((long)bRows * 3);
		if (buf == nullptr) {
			*outErr = ErrorTypes::RuntimeError("Matrix.rowCross: out of memory");
			return nullptr;
		}
		for (int r = 0; r < bRows; r++) {
			for (int c = 0; c < 3; c++) buf[(long)r * 3 + c] = ListElem(bList, bNested, r, c);
		}
		bData = buf;
	}

	MatrixData* out = NewMatrixData(A->rows, 3);
	if (out == nullptr) {
		*outErr = ErrorTypes::RuntimeError("Matrix.rowCross: out of memory");
		return nullptr;
	}
	long bStep = (bRows == 1) ? 0 : 3;
	for (int i = 0; i < A->rows; i++) {
		const double* a = A->data + (long)i * 3;
		const double* b = bData + (long)i * bStep;
		double* o = out->data + (long)i * 3;
		o[0] = a[1]*b[2] - a[2]*b[1];
		o[1] = a[2]*b[0] - a[0]*b[2];
		o[2] = a[0]*b[1] - a[1]*b[0];
	}
	return out;
}

//--------------------------------------------------------------------------------
// Neural network primitives
//--------------------------------------------------------------------------------

// The textbook 1/(1+exp(-x)) overflows exp() for x around -746 and returns inf
// rather than 0.  Branching on the sign keeps the exponent negative either way,
// which is exact at both tails.  Costs one predictable branch per element.
double StableSigmoid(double x) {
	if (x >= 0.0) return 1.0 / (1.0 + std::exp(-x));
	double e = std::exp(x);
	return e / (1.0 + e);
}

// Softmax in place along `axis` (0 = down columns, 1 = across rows, -1 = over
// the whole matrix).  Always max-subtracted, so exp() cannot overflow.
//
// A run that is entirely -inf yields NaN, as it does in numpy: there is no
// distribution over impossible outcomes to report.
static void SoftmaxRun(double* base, long count, long stride) {
	if (count <= 0) return;
	double mx = base[0];
	for (long i = 1; i < count; i++) {
		double v = base[i * stride];
		if (v > mx) mx = v;
	}
	double sum = 0.0;
	for (long i = 0; i < count; i++) {
		double e = std::exp(base[i * stride] - mx);
		base[i * stride] = e;
		sum += e;
	}
	for (long i = 0; i < count; i++) base[i * stride] /= sum;
}

void SoftmaxInPlace(MatrixData* m, int axis) {
	int rows = m->rows, cols = m->columns;
	if (axis == 1) {
		for (int i = 0; i < rows; i++) SoftmaxRun(m->data + (long)i * cols, cols, 1);
	} else if (axis == 0) {
		for (int j = 0; j < cols; j++) SoftmaxRun(m->data + j, rows, cols);
	} else {
		SoftmaxRun(m->data, m->LiveElems(), 1);
	}
}

// m.softmaxCrossEntropy(targets, outProbs=null) -> a new n x 1 Matrix of
// per-sample losses.  See notes/MATRIX_DESIGN.md; the essentials:
//
//   - INPUT IS LOGITS, not probabilities.  This applies the softmax itself.
//     `z.softmax.softmaxCrossEntropy(t)` is a double softmax: wrong, and
//     numerically worse than the thing this fusion exists to avoid.
//   - Row-wise only, samples in rows.  No axis knob: cross-entropy is a
//     per-sample loss, and an axis would only muddy what a sample is.
//   - UNREDUCED.  Returning the mean would silently mispair with the
//     documented gradient (yHat - targets), which is the gradient of the SUM.
//   - Natural log, and log-sum-exp with max subtraction, so log(0) is
//     unreachable by construction.
//
// `targets` is n x k (a distribution -- one-hot, or smoothed/soft) or n x 1
// (class indices), dispatched on column count; they overlap only at k=1, where
// the distribution reading wins.
//
// Each row is fully read before anything is written back to it, so `outProbs`
// may safely be the logits matrix itself.
MatrixData* SoftmaxCrossEntropy(const MatrixData* logits, Value vTargets,
                                       Value vOutProbs, Value* outErr) {
	int n = logits->rows, k = logits->columns;
	if (k == 0 && n > 0) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix.softmaxCrossEntropy: logits must have at least one column");
		return nullptr;
	}

	// ---- targets ----
	const MatrixData* tM = nullptr;
	ValueList tList;
	bool tNested = false;
	int tRows = 0, tCols = 0;
	if (vTargets.Type() == ValueType::List) {
		tList = vTargets.GetList();
		if (!ListShape(tList, &tNested, &tRows, &tCols, "softmaxCrossEntropy", outErr)) return nullptr;
		// A flat list is ALWAYS a column of class indices -- the same reading
		// `solve` gives its right-hand side.  Not conditional on the shape
		// working out, because at n == k a flat list would otherwise be a
		// silently different thing than at n != k.  A distribution must be
		// written as a nested list or a Matrix.
		if (!tNested) { tRows = tCols; tCols = 1; }
	} else {
		tM = ValueToMatrix(vTargets);
		if (tM == nullptr) {
			*outErr = ErrorTypes::RuntimeError(
				"Matrix.softmaxCrossEntropy: targets must be a Matrix or a list");
			return nullptr;
		}
		tRows = tM->rows;
		tCols = tM->columns;
	}
	if (tRows != n) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix.softmaxCrossEntropy: targets must have one row per sample");
		return nullptr;
	}
	bool asIndices = (tCols != k);
	if (asIndices && tCols != 1) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix.softmaxCrossEntropy: targets must be n x k (a distribution) or n x 1 (class indices)");
		return nullptr;
	}

	// ---- outProbs ----
	MatrixData* probs = nullptr;
	if (!vOutProbs.IsNull()) {
		probs = ValueToMatrix(vOutProbs);
		if (probs == nullptr) {
			*outErr = ErrorTypes::RuntimeError(
				"Matrix.softmaxCrossEntropy: outProbs must be a Matrix or null");
			return nullptr;
		}
		// Reshaping outProbs would move storage the targets still point into.
		if (tM != nullptr && tM == probs) {
			*outErr = ErrorTypes::RuntimeError(
				"Matrix.softmaxCrossEntropy: outProbs must not be the targets matrix");
			return nullptr;
		}
		if (!EnsureCapacity(probs, (long)n * k)) {
			*outErr = ErrorTypes::RuntimeError("Matrix.softmaxCrossEntropy: out of memory");
			return nullptr;
		}
		probs->rows = n;
		probs->columns = k;
	}

	MatrixData* out = NewMatrixData(n, 1);
	if (out == nullptr) {
		*outErr = ErrorTypes::RuntimeError("Matrix.softmaxCrossEntropy: out of memory");
		return nullptr;
	}

	// One accessor for element (i, j) of the targets, however they arrived.
	// The flat-list case is the trap worth centralizing: it was reinterpreted
	// above as n x 1, so sample i lives at [i], which ListElem addresses as
	// (row 0, column i) -- not (row i, column 0).
	bool tFlat = (tM == nullptr && !tNested);
	auto targetAt = [&](int i, int j) -> double {
		if (tM != nullptr) return tM->data[(long)i * tCols + j];
		if (tFlat) return ListElem(tList, false, 0, i);
		return ListElem(tList, true, i, j);
	};

	for (int i = 0; i < n; i++) {
		const double* z = logits->data + (long)i * k;
		double mx = z[0];
		for (int j = 1; j < k; j++) {
			if (z[j] > mx) mx = z[j];
		}
		double sum = 0.0;
		for (int j = 0; j < k; j++) sum += std::exp(z[j] - mx);
		double logZ = mx + std::log(sum);

		double loss;
		if (asIndices) {
			double raw = targetAt(i, 0);
			int c = (int)raw;
			if ((double)c != raw || c < 0 || c >= k) {
				delete out;
				*outErr = ErrorTypes::RuntimeError(
					"Matrix.softmaxCrossEntropy: class index out of range");
				return nullptr;
			}
			loss = logZ - z[c];
		} else {
			// -sum(t*log(p)) = sum(t)*logZ - sum(t*z).  Carrying sum(t) rather
			// than assuming it is 1 costs nothing and stays correct for targets
			// that are not normalized.
			double sumT = 0.0, dot = 0.0;
			for (int j = 0; j < k; j++) {
				double t = targetAt(i, j);
				sumT += t;
				dot += t * z[j];
			}
			loss = sumT * logZ - dot;
		}
		out->data[i] = loss;

		// Written last: until now `z` may be the very row we are about to fill.
		if (probs != nullptr) {
			double* p = probs->data + (long)i * k;
			for (int j = 0; j < k; j++) p[j] = std::exp(z[j] - logZ);
		}
	}
	return out;
}

//--------------------------------------------------------------------------------
// Serialization (RawData)
//--------------------------------------------------------------------------------

// The element formats a matrix can be stored in.  float64 is lossless and the
// default; the rest exist so that a matrix can meet data produced by something
// else -- uint8 for images, int8/int16 for quantized weights, float32 for the
// common on-disk halving of size.
//
// The `code` values are part of the file format (see DType in MatrixInternal.h)
// and must never be reused for a different type.  Add new types at the end.
static const DType kDTypes[] = {
	{"float64", 1, 8, true,  0.0, 0.0},
	{"float32", 2, 4, true,  0.0, 0.0},
	{"int8",    3, 1, false, -128.0, 127.0},
	{"uint8",   4, 1, false, 0.0, 255.0},
	{"int16",   5, 2, false, -32768.0, 32767.0},
	{"uint16",  6, 2, false, 0.0, 65535.0},
	{"int32",   7, 4, false, -2147483648.0, 2147483647.0},
	{"uint32",  8, 4, false, 0.0, 4294967295.0},
	// The int64 bounds are the nearest doubles to the true limits.  -2^63 is
	// exact; +2^63-1 is not, so the high bound is 2^63 exclusive -- see
	// StoreElem, which clamps against it with >=.
	{"int64",   9, 8, false, -9223372036854775808.0, 9223372036854775808.0},
};

static const int kDTypeCount = (int)(sizeof(kDTypes) / sizeof(kDTypes[0]));

// Header: 16 bytes, which keeps float64 data 8-byte aligned behind it.
//
//   0..3   magic "MSMX"
//   4..5   uint16 version
//   6..7   uint16 dtype code
//   8..11  int32  rows
//   12..15 int32  columns
//
// Rows and columns are stored even though the dtype alone would let a reader
// infer a length: shape is the thing a caller most often gets wrong, and a
// header that carries it turns a silent misread into a clean error.
static const int kHeaderSize = 16;

static const int kHeaderVersion = 1;

static const unsigned char kMagic[4] = { 'M', 'S', 'M', 'X' };

// Multi-byte integers follow the RawData object's own `littleEndian` flag
// rather than being fixed little-endian.  The default is little-endian, and
// that is the canonical format for our files; the flag is there so that
// headerless foreign data of the other byte order can still be read, and it
// would be strange for it to govern rd.int but not rd-backed matrix data.
static void PutUInt(unsigned char* p, uint64_t bits, int width, bool le) {
	for (int i = 0; i < width; i++) {
		int shift = 8 * (le ? i : width - 1 - i);
		p[i] = (unsigned char)((bits >> shift) & 0xFF);
	}
}

static uint64_t GetUInt(const unsigned char* p, int width, bool le) {
	uint64_t bits = 0;
	for (int i = 0; i < width; i++) {
		int shift = 8 * (le ? i : width - 1 - i);
		bits |= (uint64_t)p[i] << shift;
	}
	return bits;
}

// Store one element in the given format.
//
// The integer formats round to nearest (ties away from zero, matching
// MiniScript's `round`) and *saturate* at the ends of the range; NaN stores 0.
// This departs from numpy's astype, which truncates toward zero and leaves
// out-of-range conversions undefined -- in practice wrapping, so 300 lands as
// 44 in a uint8.  For the two things these formats are actually for, images and
// quantized weights, a clamped bright pixel is a small error and a wrapped one
// is garbage, so saturation is the behavior worth having here.
static void StoreElem(unsigned char* p, const DType* dt, double v, bool le) {
	if (dt->isFloat) {
		if (dt->size == 8) {
			uint64_t bits;
			memcpy(&bits, &v, 8);
			PutUInt(p, bits, 8, le);
		} else {
			float f = (float)v;
			uint32_t bits;
			memcpy(&bits, &f, 4);
			PutUInt(p, bits, 4, le);
		}
		return;
	}
	double r = std::round(v);
	int64_t i;
	if (v != v) {
		i = 0;                                  // NaN has no integer meaning
	} else if (r <= dt->lo) {
		i = (dt->lo == -9223372036854775808.0) ? INT64_MIN : (int64_t)dt->lo;
	} else if (r >= dt->hi) {
		i = (dt->hi == 9223372036854775808.0) ? INT64_MAX : (int64_t)dt->hi;
	} else {
		i = (int64_t)r;
	}
	PutUInt(p, (uint64_t)i, dt->size, le);
}

// Read one element back.  Signed formats are sign-extended from their width;
// int64 values beyond 2^53 lose precision on the way into a double, which is
// inherent in the element type and not something we can flag per value.
static double LoadElem(const unsigned char* p, const DType* dt, bool le) {
	uint64_t bits = GetUInt(p, dt->size, le);
	if (dt->isFloat) {
		if (dt->size == 8) {
			double d;
			memcpy(&d, &bits, 8);
			return d;
		}
		uint32_t b32 = (uint32_t)bits;
		float f;
		memcpy(&f, &b32, 4);
		return (double)f;
	}
	if (dt->lo < 0.0) {
		// Sign-extend: shift the sign bit of this width up to bit 63 and back.
		int shift = 64 - 8 * dt->size;
		return (double)((int64_t)(bits << shift) >> shift);
	}
	return (double)bits;
}

// Look up a dtype by name.  With `allowAuto`, the name "auto" yields a null
// dtype and true -- "read the header for it", which is only meaningful on read.
bool ParseDType(Value v, bool allowAuto, const char* who,
                       const DType** out, Value* outErr) {
	if (v.Type() != ValueType::String) {
		*outErr = ErrorTypes::TypeError("string", v);
		return false;
	}
	String name = v.ToString();
	if (allowAuto && name == "auto") { *out = nullptr; return true; }
	for (int i = 0; i < kDTypeCount; i++) {
		if (name == kDTypes[i].name) { *out = &kDTypes[i]; return true; }
	}
	*outErr = ErrorTypes::RuntimeError(
		String("Matrix.") + who + ": unknown dtype \"" + name + "\"");
	return false;
}

static const DType* DTypeByCode(int code) {
	for (int i = 0; i < kDTypeCount; i++) {
		if (kDTypes[i].code == code) return &kDTypes[i];
	}
	return nullptr;
}

// Read and validate a header at `p`, which the caller has already bounds-checked.
static bool ReadHeader(const unsigned char* p, bool le, const char* who,
                       const DType** outDt, int* outRows, int* outCols, Value* outErr) {
	if (memcmp(p, kMagic, 4) != 0) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": no matrix header at that position"
			" (pass an explicit dtype to read headerless data)");
		return false;
	}
	int version = (int)GetUInt(p + 4, 2, le);
	if (version != kHeaderVersion) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": unsupported matrix format version "
			+ String::Format(version));
		return false;
	}
	const DType* dt = DTypeByCode((int)GetUInt(p + 6, 2, le));
	if (dt == nullptr) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": unknown dtype code in header");
		return false;
	}
	int rows = (int)(int32_t)GetUInt(p + 8, 4, le);
	int cols = (int)(int32_t)GetUInt(p + 12, 4, le);
	if (rows < 0 || cols < 0 || (long)rows * (long)cols > kMaxMatrixElems) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": bad shape in matrix header");
		return false;
	}
	*outDt = dt;
	*outRows = rows;
	*outCols = cols;
	return true;
}

// Reshape without preserving contents.  Only for the read path, which then
// overwrites every element; the resize intrinsic's row-by-row relayout would
// be wasted work here.
static bool SetShapeDiscarding(MatrixData* m, int rows, int columns) {
	long needed = (long)rows * (long)columns;
	if (needed > kMaxMatrixElems) return false;
	if (!EnsureCapacity(m, needed)) return false;
	m->rows = rows;
	m->columns = columns;
	return true;
}

// Write `m` at `p` -- header (if asked) followed by rows*columns elements.
static void WriteElems(const MatrixData* m, unsigned char* p, const DType* dt,
                       bool header, bool le) {
	if (header) {
		memcpy(p, kMagic, 4);
		PutUInt(p + 4, (uint64_t)kHeaderVersion, 2, le);
		PutUInt(p + 6, (uint64_t)dt->code, 2, le);
		PutUInt(p + 8, (uint64_t)(uint32_t)m->rows, 4, le);
		PutUInt(p + 12, (uint64_t)(uint32_t)m->columns, 4, le);
		p += kHeaderSize;
	}
	long n = m->LiveElems();
	for (long i = 0; i < n; i++) {
		StoreElem(p, dt, m->data[i], le);
		p += dt->size;
	}
}

// The reverse: fill an already-shaped `m` from `p`.
static void ReadElems(MatrixData* m, const unsigned char* p, const DType* dt, bool le) {
	long n = m->LiveElems();
	for (long i = 0; i < n; i++) {
		m->data[i] = LoadElem(p, dt, le);
		p += dt->size;
	}
}

// The whole of toRawData, once the caller has unpacked its arguments.  Grows
// the RawData if what we are writing runs past its end -- the alternative,
// making the caller compute the byte count itself, means duplicating this
// file's own layout rules in script.
//
// Returns the ending position, or -1 with *outErr set.
int MatrixToRawData(const MatrixData* m, Value rdValue, const DType* dt,
                           int startPos, bool includeHeader, Value* outErr) {
	long bytes = (includeHeader ? kHeaderSize : 0) + m->LiveElems() * dt->size;
	long end = (long)startPos + bytes;
	if (end > 0x7FFFFFFFL) {
		*outErr = ErrorTypes::RuntimeError("Matrix.toRawData: result would exceed 2GB");
		return -1;
	}
	String growErr;
	BinaryData* rd = RawDataEnsureSize(rdValue, (int)end, &growErr);
	if (rd == nullptr) {
		*outErr = ErrorTypes::RuntimeError(String("Matrix.toRawData: ") + growErr);
		return -1;
	}
	// Nothing to write at all (an empty matrix, headerless) leaves rd->bytes
	// possibly null, and null + 0 is not a pointer we are entitled to form.
	if (bytes > 0) {
		WriteElems(m, rd->bytes + startPos, dt, includeHeader, rd->littleEndian);
	}
	return (int)end;
}

// The shared back end of readRawData and fromRawData.
//
// `dt` null means "the data starts with a header", which supplies both the
// dtype and the shape.  Otherwise the shape comes from `rows`/`columns`, which
// the callers fill in from the receiver or from their own arguments.
//
// Returns the ending position, or -1 with *outErr set.  `m` is only modified
// on success.
int MatrixFromRawData(MatrixData* m, BinaryData* rd, const DType* dt,
                             int startPos, int rows, int columns,
                             const char* who, Value* outErr) {
	// rd is null for a RawData that has no buffer yet: an empty read from one
	// is legal (it asks for nothing), so treat it as a zero-length buffer
	// rather than rejecting it, and never form a pointer into it.
	const unsigned char* base = (rd == nullptr) ? nullptr : rd->bytes;
	int length = (rd == nullptr) ? 0 : rd->length;
	bool le = (rd == nullptr) ? true : rd->littleEndian;
	int pos = startPos;

	if (dt == nullptr) {
		if (length - pos < kHeaderSize) {
			*outErr = ErrorTypes::RuntimeError(
				String("Matrix.") + who + ": not enough data for a matrix header");
			return -1;
		}
		if (!ReadHeader(base + pos, le, who, &dt, &rows, &columns, outErr)) return -1;
		pos += kHeaderSize;
	}

	long bytes = (long)rows * (long)columns * dt->size;
	if ((long)length - pos < bytes) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": not enough data for a "
			+ String::Format(rows) + " x " + String::Format(columns) + " matrix");
		return -1;
	}
	if (!SetShapeDiscarding(m, rows, columns)) {
		*outErr = ErrorTypes::RuntimeError(String("Matrix.") + who + ": out of memory");
		return -1;
	}
	if (bytes > 0) ReadElems(m, base + pos, dt, le);
	return (int)(pos + bytes);
}

} // namespace MatrixInternal

using namespace MatrixInternal;

//--------------------------------------------------------------------------------
// Public entry points (declared in Matrix.h)
//--------------------------------------------------------------------------------

Value MatrixToValue(MatrixData* data) {
	if (data == nullptr) return Value::Null;
	ValueDict instance;
	instance.SetValue(Value::magicIsA, MatrixClass());
	instance.SetValue(kHandle(), MakeHandle(data));
	// rows/columns are plain map entries so that `m.rows` is an ordinary map
	// lookup rather than an intrinsic call.  The C++ side rewrites them on
	// every resize; they are documented read-only, and assigning to them
	// desyncs the map from the handle without changing the storage.
	instance.SetValue(kRows(), Value(data->rows));
	instance.SetValue(kColumns(), Value(data->columns));
	return DynamicMap(instance);
}

MatrixData* ValueToMatrix(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	Value handleVal = value.GetDict().Lookup(kHandle(), Value::Null);
	if (!handleVal.IsHandle()) return nullptr;
	return (MatrixData*)handleVal.HandlePtr();
}

} // namespace MiniScript
