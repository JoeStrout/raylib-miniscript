//
//  Matrix.cpp
//  raylib-miniscript
//
//  Matrix class implementation -- first slice: storage, handle plumbing, and
//  scalar element access.  Bulk operations (gemm and friends) come later; see
//  notes/MATRIX_DESIGN.md.
//

#include "Matrix.h"
#include "miniscript.h"
#include <cstdlib>
#include <cstring>

namespace MiniScript {

const long kMaxMatrixElems = 32L * 1024L * 1024L;   // 256MB of doubles

#define INTRINSIC_LAMBDA [](Context context, IntrinsicResult partialResult) -> IntrinsicResult

// A GC-backed string Value cannot be built at static-init time (before
// GCManager exists), so the map keys are made lazily on first use.  Interned
// (< 128 bytes) and therefore immortal, so they are safe to hold.
static const Value& kHandle()  { static Value v("_handle"); return v; }
static const Value& kRows()    { static Value v("rows");    return v; }
static const Value& kColumns() { static Value v("columns"); return v; }

//--------------------------------------------------------------------------------
// Storage
//--------------------------------------------------------------------------------

// Allocate `elems` doubles, retrying once after a collection.  A failure here
// may only mean that unreachable matrices are still waiting to be swept, since
// a buffer is not freed until its handle is collected.  One retry only: if a
// full mark-sweep did not free enough, nothing is reclaimable.
static double* AllocElems(long elems) {
	if (elems <= 0) return nullptr;
	double* p = (double*)calloc((size_t)elems, sizeof(double));
	if (p != nullptr) return p;
	GCManager::CollectGarbage();
	return (double*)calloc((size_t)elems, sizeof(double));
}

// Build storage for a rows x columns matrix, all zeros.  Returns null if the
// size is out of range or the allocation failed; the caller turns that into an
// error value (it has the context to say which).
static MatrixData* NewMatrixData(int rows, int columns) {
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
static bool EnsureCapacity(MatrixData* m, long needed) {
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

Value MatrixToValue(MatrixData* data) {
	if (data == nullptr) return Value::Null;
	ValueDict instance;
	instance.SetValue(Value::magicIsA, StaticMap(MatrixClass()));
	instance.SetValue(kHandle(), MakeHandle(data));
	// rows/columns are plain map entries so that `m.rows` is an ordinary map
	// lookup rather than an intrinsic call.  The C++ side rewrites them on
	// every resize; they are documented read-only, and assigning to them
	// desyncs the map from the handle without changing the storage.
	instance.SetValue(kRows(), Value(data->rows));
	instance.SetValue(kColumns(), Value(data->columns));
	return DynamicMap(instance);
}

// Rewrite the instance map's rows/columns entries after a shape change.  They
// are plain map entries for lookup speed (see MatrixToValue), which means every
// resize has to write them back or `m.rows` would report the old shape.
static void SyncShape(Value self, MatrixData* m) {
	if (self.Type() != ValueType::Map) return;
	ValueDict map = self.GetDict();
	map.SetValue(kRows(), Value(m->rows));
	map.SetValue(kColumns(), Value(m->columns));
}

MatrixData* ValueToMatrix(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	Value handleVal = value.GetDict().Lookup(kHandle(), Value::Null);
	if (!handleVal.IsHandle()) return nullptr;
	return (MatrixData*)handleVal.HandlePtr();
}

//--------------------------------------------------------------------------------
// Argument helpers
//--------------------------------------------------------------------------------

// Every intrinsic below starts here.  A null handle is the `new Matrix` case:
// the class itself carries a null handle on purpose (see MatrixClass), so an
// instance that never went through a constructor has nothing to work on.
static MatrixData* SelfMatrix(Context context, Value* outErr) {
	MatrixData* m = ValueToMatrix(context.GetVar("self"));
	if (m == nullptr) {
		*outErr = ErrorTypes::RuntimeError(
			"Matrix required (use Matrix.ofSize; `new Matrix` alone has no storage)");
		return nullptr;
	}
	*outErr = Value::Null;
	return m;
}

// Read a required whole-number argument.  Returns false and sets *outErr if the
// argument is not a number.
static bool IntArg(Context context, const char* name, int* out, Value* outErr) {
	Value v = context.GetVar(name);
	if (v.Type() != ValueType::Number) {
		*outErr = ErrorTypes::TypeError("number", v);
		return false;
	}
	*out = v.IntValue();
	return true;
}

// Resolve one index against an extent and bounds-check it.
//
// Negative indices count from the end, as they do for MiniScript lists and
// strings, and as they do in numpy: -1 is the last row/column.  Note that this
// makes *every* index into a zero-extent dimension out of range, which is what
// we want -- there is no element to reach.
//
// `what` names the axis for the error message ("row" / "column").
static bool ResolveIndex(Value v, int extent, const char* what, int* out, Value* outErr) {
	if (v.Type() != ValueType::Number) {
		*outErr = ErrorTypes::TypeError("number", v);
		return false;
	}
	int i = v.IntValue();
	if (i < 0) i += extent;
	if (i < 0 || i >= extent) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + what + " index out of range");
		return false;
	}
	*out = i;
	return true;
}

// Resolve a block *offset* rather than an element index.
//
// Deliberately looser than ResolveIndex: an offset equal to the extent is
// legal and denotes an empty block, so `getSub` on a 0x0 yields a 0x0 rather
// than an error, and a zero-length block at the end of a matrix is expressible.
// Negative values still count from the end.
static bool ResolveOffset(Value v, int extent, const char* what, int* out, Value* outErr) {
	if (v.IsNull()) { *out = 0; return true; }
	if (v.Type() != ValueType::Number) {
		*outErr = ErrorTypes::TypeError("number", v);
		return false;
	}
	int i = v.IntValue();
	if (i < 0) i += extent;
	if (i < 0 || i > extent) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + what + " offset out of range");
		return false;
	}
	*out = i;
	return true;
}

// Resolve a block extent: null means "through the end" from `offset`.
// An explicit count that would run past the end is an error, not a clip --
// silently returning fewer rows than asked for hides an indexing bug.
static bool ResolveExtent(Value v, int offset, int limit, const char* what,
                          int* out, Value* outErr) {
	if (v.IsNull()) { *out = limit - offset; return true; }
	if (v.Type() != ValueType::Number) {
		*outErr = ErrorTypes::TypeError("number", v);
		return false;
	}
	int n = v.IntValue();
	if (n < 0 || offset + n > limit) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.getSub: ") + what + " count does not fit at that offset");
		return false;
	}
	*out = n;
	return true;
}

// Work out the shape a list argument denotes, by the same rules as
// Matrix.fromList: a 1D list of numbers is 1 x n, a list of lists is n x m, and
// [] is 0 x 0.  Validates that the input is rectangular and entirely numeric,
// so callers can read elements afterward without re-checking.
//
// Shared so that everywhere a Matrix is accepted, a list means exactly what
// fromList would have made of it -- one rule to learn, and no chance of the two
// interpretations drifting apart.
static bool ListShape(ValueList outer, bool* outNested, int* outRows, int* outCols,
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
static double ListElem(ValueList outer, bool nested, int r, int c) {
	return (nested ? outer[r].GetList()[c] : outer[c]).DoubleValue();
}

// Read a list argument that must have exactly `want` numeric elements.
//
// An exact length is required rather than filling a prefix: a short list is
// almost always a bug, and numpy likewise refuses a shape-incompatible
// assignment rather than padding it.
// A run of values for setRow/setColumn: either a list of exactly `want`
// numbers, or a single number broadcast across all of them (as numpy does for
// `a[0] = 5`).  A wrong-length list is still an error -- broadcasting a scalar
// is a deliberate shorthand, whereas a short list is almost always a bug.
struct ValueRun {
	ValueList list;
	double scalar = 0.0;
	bool isScalar = false;
	double At(int i) const { return isScalar ? scalar : list[i].DoubleValue(); }
};

static bool ValueRunArg(Value v, int want, const char* who, ValueRun* out, Value* outErr) {
	if (v.Type() == ValueType::Number) {
		out->isScalar = true;
		out->scalar = v.DoubleValue();
		return true;
	}
	if (v.Type() != ValueType::List) {
		*outErr = ErrorTypes::TypeError("list or number", v);
		return false;
	}
	ValueList list = v.GetList();
	if (list.Count() != want) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": expected a list of " + String::Format(want)
			+ " numbers, or a single number");
		return false;
	}
	for (int i = 0; i < want; i++) {
		if (list[i].Type() != ValueType::Number) {
			*outErr = ErrorTypes::TypeError("number", list[i]);
			return false;
		}
	}
	out->isScalar = false;
	out->list = list;
	return true;
}

// The common case: a (row, column) pair of arguments, resolved together.
// Nearly every element-level intrinsic starts with this.
static bool ResolveRowCol(Context context, MatrixData* m,
                          int* outRow, int* outCol, Value* outErr) {
	if (!ResolveIndex(context.GetVar("row"), m->rows, "row", outRow, outErr)) return false;
	if (!ResolveIndex(context.GetVar("column"), m->columns, "column", outCol, outErr)) return false;
	return true;
}

//--------------------------------------------------------------------------------
// The Matrix class
//--------------------------------------------------------------------------------

ValueDict& MatrixClass() {
	static ValueDict matrixClass;
	if (matrixClass.Count() > 0) return matrixClass;

	// Matrix itself must hold a NULL handle.  Without this entry, `new Matrix`
	// produces a map containing only __isa, and a lookup of `_handle` resolves
	// up the __isa chain to the class -- so every instance would silently share
	// one buffer.  With it, an unconstructed instance has a null handle and
	// every intrinsic reports that clearly.
	matrixClass.SetValue(kHandle(), Value::Null);
	matrixClass.SetValue(kRows(), Value::zero);
	matrixClass.SetValue(kColumns(), Value::zero);

	Intrinsic f;

	// Matrix.ofSize(rows, columns, initialValue=0) -> new Matrix
	f = Intrinsic::Create("");
	f.AddParam("rows");
	f.AddParam("columns");
	f.AddParam("initialValue", Value::zero);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		int rows = 0, columns = 0;
		if (!IntArg(context, "rows", &rows, &err)) return IntrinsicResult(err);
		if (!IntArg(context, "columns", &columns, &err)) return IntrinsicResult(err);
		if (rows < 0 || columns < 0) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.ofSize: rows and columns must be >= 0"));
		}
		// Overflow-safe: check the product in long before it becomes a size.
		long elems = (long)rows * (long)columns;
		if (elems > kMaxMatrixElems) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.ofSize: requested size exceeds the maximum matrix size"));
		}

		Value initial = context.GetVar("initialValue");
		if (initial.Type() == ValueType::Function) {
			// Per-element callback needs the intrinsic to re-enter the VM,
			// which this slice does not do yet.  Say so rather than quietly
			// filling with zeros.
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.ofSize: a function initialValue is not supported yet"));
		}
		// Note that null is NOT accepted: the parameter already defaults to 0,
		// so an explicit null would be a second spelling of the same thing.
		if (initial.Type() != ValueType::Number) {
			return IntrinsicResult(ErrorTypes::TypeError("number", initial));
		}

		MatrixData* m = NewMatrixData(rows, columns);
		if (m == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.ofSize: out of memory"));
		}
		// NewMatrixData zero-fills, so only a nonzero initial value needs work.
		double init = initial.DoubleValue();
		if (init != 0.0) {
			for (long i = 0; i < elems; i++) m->data[i] = init;
		}
		return IntrinsicResult(MatrixToValue(m));
	});
	matrixClass.SetValue(String("ofSize"), f.GetFunc());

	// Matrix.fromList(sourceList)
	//   1D list of numbers -> 1 x n row vector
	//   2D list (list of lists) -> n x m, one row per inner list
	//   [] -> a clean 0x0
	f = Intrinsic::Create("");
	f.AddParam("sourceList");
	f.set_Code(INTRINSIC_LAMBDA {
		Value src = context.GetVar("sourceList");
		if (src.Type() != ValueType::List) {
			return IntrinsicResult(ErrorTypes::TypeError("list", src));
		}
		ValueList outer = src.GetList();
		bool nested; int rows, columns;
		Value err;
		if (!ListShape(outer, &nested, &rows, &columns, "fromList", &err)) {
			return IntrinsicResult(err);
		}
		if ((long)rows * (long)columns > kMaxMatrixElems) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.fromList: requested size exceeds the maximum matrix size"));
		}
		MatrixData* m = NewMatrixData(rows, columns);
		if (m == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.fromList: out of memory"));
		}
		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < columns; c++) {
				m->data[(long)r * columns + c] = ListElem(outer, nested, r, c);
			}
		}
		return IntrinsicResult(MatrixToValue(m));
	});
	matrixClass.SetValue(String("fromList"), f.GetFunc());

	// m.toFlatList -> a plain list of every element, row by row.
	// Index it as r*columns + c.  There is no 2D form here on purpose: a flat
	// list is the one that costs a single allocation.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		ValueList out;
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) out.Add(Value(m->data[i]));
		return IntrinsicResult(DynamicList(out));
	});
	matrixClass.SetValue(String("toFlatList"), f.GetFunc());

	// m.getElem(row, column) -> number
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row");
	f.AddParam("column");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int row = 0, col = 0;
		if (!ResolveRowCol(context, m, &row, &col, &err)) return IntrinsicResult(err);
		return IntrinsicResult(Value(m->data[(long)row * m->columns + col]));
	});
	matrixClass.SetValue(String("getElem"), f.GetFunc());

	// m.setElem(row, column, value) -> self
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row");
	f.AddParam("column");
	f.AddParam("value", Value::zero);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int row = 0, col = 0;
		if (!ResolveRowCol(context, m, &row, &col, &err)) return IntrinsicResult(err);
		Value v = context.GetVar("value");
		if (v.Type() != ValueType::Number) {
			return IntrinsicResult(ErrorTypes::TypeError("number", v));
		}
		m->data[(long)row * m->columns + col] = v.DoubleValue();
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("setElem"), f.GetFunc());

	// m.getRow(row) -> plain list of numbers
	//
	// Note the deliberate asymmetry with getSub, which returns a Matrix: a 1-D
	// result has a natural flat-list form and a 2-D one does not.  A row is
	// contiguous, so this is a straight walk.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int row;
		if (!ResolveIndex(context.GetVar("row"), m->rows, "getRow", &row, &err)) return IntrinsicResult(err);
		ValueList out;
		const double* p = m->data + (long)row * m->columns;
		for (int c = 0; c < m->columns; c++) out.Add(Value(p[c]));
		return IntrinsicResult(DynamicList(out));
	});
	matrixClass.SetValue(String("getRow"), f.GetFunc());

	// m.setRow(row, values) -> self
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row");
	f.AddParam("values");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int row;
		if (!ResolveIndex(context.GetVar("row"), m->rows, "setRow", &row, &err)) return IntrinsicResult(err);
		ValueRun values;
		if (!ValueRunArg(context.GetVar("values"), m->columns, "setRow", &values, &err)) return IntrinsicResult(err);
		// Validated in full before any write, so a bad element cannot leave the
		// row half-updated.
		double* p = m->data + (long)row * m->columns;
		for (int c = 0; c < m->columns; c++) p[c] = values.At(c);
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("setRow"), f.GetFunc());

	// m.getColumn(column) -> plain list of numbers
	//
	// Strided: row-major storage means a column is the scattered access, which
	// is exactly why columns are the structural dimension in this design and
	// rows are the one that varies.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("column");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int col;
		if (!ResolveIndex(context.GetVar("column"), m->columns, "getColumn", &col, &err)) return IntrinsicResult(err);
		ValueList out;
		for (int r = 0; r < m->rows; r++) out.Add(Value(m->data[(long)r * m->columns + col]));
		return IntrinsicResult(DynamicList(out));
	});
	matrixClass.SetValue(String("getColumn"), f.GetFunc());

	// m.setColumn(column, values) -> self
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("column");
	f.AddParam("values");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int col;
		if (!ResolveIndex(context.GetVar("column"), m->columns, "setColumn", &col, &err)) return IntrinsicResult(err);
		ValueRun values;
		if (!ValueRunArg(context.GetVar("values"), m->rows, "setColumn", &values, &err)) return IntrinsicResult(err);
		for (int r = 0; r < m->rows; r++) m->data[(long)r * m->columns + col] = values.At(r);
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("setColumn"), f.GetFunc());

	// m.getSub(row=0, col=0, rows=null, columns=null) -> new Matrix
	//
	// A rectangular block, copied out.  Intrinsic rather than a script wrapper
	// despite the general preference for wrappers: the script version would
	// allocate a MiniScript list per row to accomplish what is a strided
	// memcpy, which is real garbage pressure for a 512-row mini-batch.
	//
	// A null count means "through the end"; an explicit count that overruns is
	// an error rather than a clip.  Returns a Matrix, not a list of lists,
	// because a 2-D result has no natural flat-list form.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row", Value::zero);
	f.AddParam("col", Value::zero);
	f.AddParam("rows");
	f.AddParam("columns");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int row, col, nrows, ncols;
		if (!ResolveOffset(context.GetVar("row"), m->rows, "getSub row", &row, &err)) return IntrinsicResult(err);
		if (!ResolveOffset(context.GetVar("col"), m->columns, "getSub col", &col, &err)) return IntrinsicResult(err);
		if (!ResolveExtent(context.GetVar("rows"), row, m->rows, "row", &nrows, &err)) return IntrinsicResult(err);
		if (!ResolveExtent(context.GetVar("columns"), col, m->columns, "column", &ncols, &err)) return IntrinsicResult(err);

		MatrixData* out = NewMatrixData(nrows, ncols);
		if (out == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.getSub: out of memory"));
		}
		for (int r = 0; r < nrows; r++) {
			memcpy(out->data + (long)r * ncols,
			       m->data + (long)(row + r) * m->columns + col,
			       (size_t)ncols * sizeof(double));
		}
		return IntrinsicResult(MatrixToValue(out));
	});
	matrixClass.SetValue(String("getSub"), f.GetFunc());

	// m.setSub(row, col, m2) -> self
	//
	// Write m2 as a block at (row, col).  Errors if m2 does not fit rather
	// than clipping: a block that silently lands half-written is far worse to
	// debug than a refused call.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row", Value::zero);
	f.AddParam("col", Value::zero);
	f.AddParam("m2");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value src = context.GetVar("m2");
		MatrixData* m2 = ValueToMatrix(src);
		if (m2 == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.setSub: m2 must be a Matrix"));
		}
		int row, col;
		if (!ResolveOffset(context.GetVar("row"), m->rows, "setSub row", &row, &err)) return IntrinsicResult(err);
		if (!ResolveOffset(context.GetVar("col"), m->columns, "setSub col", &col, &err)) return IntrinsicResult(err);
		if (row + m2->rows > m->rows || col + m2->columns > m->columns) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.setSub: m2 does not fit at that offset"));
		}

		// Aliasing needs no special handling here, and it is worth writing down
		// why, because the general case would.
		//
		// m2 aliases us only by being the very same MatrixData -- storage is
		// never shared any other way, since this API has no views and every op
		// returns fresh storage.  But then m2's dimensions ARE our dimensions,
		// so the fit check above already forced row == 0 and col == 0: the only
		// aliased call that survives it is the whole matrix onto itself.  That
		// is a no-op, so return before copying rather than memcpy'ing each row
		// onto itself (which, with src == dst, is formally undefined anyway).
		//
		// Were the destination ever a *shifted* block of the same buffer -- as
		// it will be for removeRow, which slides rows up -- the fix would not
		// be a staging buffer either: with a shared stride the move is a
		// uniform translation, so iterating away from the direction of travel
		// (bottom-to-top when moving down, top-to-bottom when moving up) is
		// always safe. That is what memmove does in one dimension.
		if (m2 == m) return IntrinsicResult(context.GetVar("self"));

		for (int r = 0; r < m2->rows; r++) {
			memcpy(m->data + (long)(row + r) * m->columns + col,
			       m2->data + (long)r * m2->columns,
			       (size_t)m2->columns * sizeof(double));
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("setSub"), f.GetFunc());

	// m.reshape(rows, columns) -> self
	//
	// Reinterpret the same elements with a different shape, in place and O(1).
	// This is possible only because the live region is contiguous with stride
	// exactly `columns`, so nothing moves -- only the two integers change.
	//
	// It is NOT a transpose.  Users conflate the two constantly: reshaping a
	// 2x3 to 3x2 reads the elements in row-major order into the new shape; it
	// does not swap rows with columns.
	//
	// Exactly one of rows/columns may be null, and is then inferred from the
	// other.  The total element count must not change.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("rows");
	f.AddParam("columns");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value vr = context.GetVar("rows");
		Value vc = context.GetVar("columns");
		long live = m->LiveElems();

		bool rNull = vr.IsNull(), cNull = vc.IsNull();
		if (rNull && cNull) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.reshape: at most one dimension may be null"));
		}
		if (!rNull && vr.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vr));
		if (!cNull && vc.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vc));

		int rows = rNull ? 0 : vr.IntValue();
		int columns = cNull ? 0 : vc.IntValue();
		if ((!rNull && rows < 0) || (!cNull && columns < 0)) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.reshape: rows and columns must be >= 0"));
		}

		// Infer the missing dimension.  A zero given extent only works when
		// there is nothing to lay out; otherwise the inference has no answer.
		if (rNull) {
			if (columns == 0) {
				if (live != 0) return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.reshape: cannot infer rows for a zero-column shape"));
				rows = 0;
			} else {
				if (live % columns != 0) return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.reshape: element count is not divisible by the given columns"));
				rows = (int)(live / columns);
			}
		} else if (cNull) {
			if (rows == 0) {
				if (live != 0) return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.reshape: cannot infer columns for a zero-row shape"));
				columns = 0;
			} else {
				if (live % rows != 0) return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.reshape: element count is not divisible by the given rows"));
				columns = (int)(live / rows);
			}
		}

		if ((long)rows * (long)columns != live) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.reshape: new shape must hold exactly the same number of elements"));
		}
		m->rows = rows;
		m->columns = columns;
		Value self = context.GetVar("self");
		SyncShape(self, m);
		return IntrinsicResult(self);
	});
	matrixClass.SetValue(String("reshape"), f.GetFunc());

	// m.resize(rows, columns) -> self
	//
	// Change the shape, keeping the overlapping top-left submatrix and
	// zero-filling anything new.  Null means "leave this dimension alone"
	// (unlike reshape, where null means "infer me").
	//
	// Changing the row count only is the cheap path: the stride is unchanged,
	// so growing is an amortized realloc and shrinking touches nothing.
	// Changing the column count moves every row, so it is a full copy -- the
	// documented expensive case, and the reason the storage design treats
	// columns as structural and rows as the thing that varies.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("rows");
	f.AddParam("columns");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value vr = context.GetVar("rows");
		Value vc = context.GetVar("columns");
		if (!vr.IsNull() && vr.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vr));
		if (!vc.IsNull() && vc.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vc));
		int newRows = vr.IsNull() ? m->rows : vr.IntValue();
		int newCols = vc.IsNull() ? m->columns : vc.IntValue();
		if (newRows < 0 || newCols < 0) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.resize: rows and columns must be >= 0"));
		}
		long needed = (long)newRows * (long)newCols;
		if (needed > kMaxMatrixElems) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.resize: requested size exceeds the maximum matrix size"));
		}

		if (newCols == m->columns) {
			// Stride unchanged: rows keep their positions.
			if (!EnsureCapacity(m, needed)) {
				return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.resize: out of memory"));
			}
			// Zero any newly-live region.  EnsureCapacity only zeroes capacity
			// it just added, and a shrink-then-grow would otherwise resurrect
			// the old contents of rows that had gone out of the live region.
			if (newRows > m->rows && m->data != nullptr) {
				long from = (long)m->rows * m->columns;
				memset(m->data + from, 0, (size_t)(needed - from) * sizeof(double));
			}
			m->rows = newRows;
		} else {
			// Stride changes, so every row moves: lay out a fresh buffer.
			// Capacity never shrinks, so the new buffer is at least as large
			// as the old one.
			long cap = needed > m->capacityElems ? needed : m->capacityElems;
			if (cap > kMaxMatrixElems) cap = kMaxMatrixElems;
			double* nd = nullptr;
			if (cap > 0) {
				nd = AllocElems(cap);
				if (nd == nullptr) {
					return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.resize: out of memory"));
				}
			}
			int copyRows = newRows < m->rows ? newRows : m->rows;
			int copyCols = newCols < m->columns ? newCols : m->columns;
			for (int r = 0; r < copyRows; r++) {
				memcpy(nd + (long)r * newCols,
				       m->data + (long)r * m->columns,
				       (size_t)copyCols * sizeof(double));
			}
			free(m->data);
			m->data = nd;
			m->capacityElems = cap;
			m->rows = newRows;
			m->columns = newCols;
		}
		Value self = context.GetVar("self");
		SyncShape(self, m);
		return IntrinsicResult(self);
	});
	matrixClass.SetValue(String("resize"), f.GetFunc());

	// m.reserve(rows) -> self
	//
	// Pre-allocate room for `rows` rows at the current column count.  No
	// visible change: the shape and every element stay as they were, only
	// `capacity` moves.  This is what makes a steady-state pool that grows and
	// shrinks by a few rows per frame allocate zero times after warmup.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("rows");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int rows = 0;
		if (!IntArg(context, "rows", &rows, &err)) return IntrinsicResult(err);
		if (rows < 0) {
			return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.reserve: rows must be >= 0"));
		}
		long needed = (long)rows * (long)m->columns;
		if (needed > kMaxMatrixElems) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.reserve: requested size exceeds the maximum matrix size"));
		}
		if (!EnsureCapacity(m, needed)) {
			return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.reserve: out of memory"));
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("reserve"), f.GetFunc());

	// m.trim -> self
	//
	// Give back capacity beyond the live region.  Capacity is never released
	// automatically -- a pool oscillating between 900 and 1000 rows should
	// allocate zero times after warmup -- so this is the explicit way to say
	// that a matrix has finished growing.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		long live = m->LiveElems();
		if (live == m->capacityElems) return IntrinsicResult(context.GetVar("self"));
		if (live == 0) {
			free(m->data);
			m->data = nullptr;
			m->capacityElems = 0;
			return IntrinsicResult(context.GetVar("self"));
		}
		double* p = (double*)realloc(m->data, (size_t)live * sizeof(double));
		// A shrinking realloc that fails is not an error worth reporting: the
		// old buffer is still valid and still holds the same matrix, we simply
		// did not manage to hand any memory back.
		if (p != nullptr) {
			m->data = p;
			m->capacityElems = live;
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("trim"), f.GetFunc());

	// m.removeRow(index) -> self
	//
	// Order-preserving, so it costs O((rows-index)*columns): every row below
	// the removed one slides up. The rows share a stride, so the move is a
	// uniform translation and memmove handles the overlap directly -- no
	// temporary needed.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("index");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int idx;
		if (!ResolveIndex(context.GetVar("index"), m->rows, "removeRow", &idx, &err)) return IntrinsicResult(err);
		long tail = (long)(m->rows - idx - 1) * m->columns;
		if (tail > 0) {
			memmove(m->data + (long)idx * m->columns,
			        m->data + (long)(idx + 1) * m->columns,
			        (size_t)tail * sizeof(double));
		}
		m->rows--;
		// The vacated last row is now spare capacity.  It is deliberately left
		// as-is: resize zeroes any region it makes live again, so stale values
		// beyond the live region can never be observed.
		Value self = context.GetVar("self");
		SyncShape(self, m);
		return IntrinsicResult(self);
	});
	matrixClass.SetValue(String("removeRow"), f.GetFunc());

	// m.removeRowFast(index) -> self
	//
	// O(columns): the last row is moved into the hole instead of everything
	// sliding up.  This REORDERS the matrix, which is why it is a separate
	// method rather than a flag on removeRow -- the name is the warning, and
	// any external index-to-entity mapping is invalidated for both the removed
	// index and the old last row.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("index");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int idx;
		if (!ResolveIndex(context.GetVar("index"), m->rows, "removeRowFast", &idx, &err)) return IntrinsicResult(err);
		int last = m->rows - 1;
		if (idx != last && m->columns > 0) {
			memcpy(m->data + (long)idx * m->columns,
			       m->data + (long)last * m->columns,
			       (size_t)m->columns * sizeof(double));
		}
		m->rows--;
		Value self = context.GetVar("self");
		SyncShape(self, m);
		return IntrinsicResult(self);
	});
	matrixClass.SetValue(String("removeRowFast"), f.GetFunc());

	// m.equals(m2, tolerance=1e-9) -> true/false
	//
	// This exists because `m1 == m2` compares the *maps* -- __isa plus the
	// handle -- so two matrices with identical contents always compare unequal.
	// The handle has identity equality only.
	//
	// Comparison is by absolute difference, matching the single `tolerance`
	// parameter in the design notes (numpy's allclose instead combines a
	// relative and an absolute tolerance).  NaN never compares equal to
	// anything, including itself, which falls out of the comparison.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("m2");
	f.AddParam("tolerance", Value(1e-9));
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value tolV = context.GetVar("tolerance");
		if (tolV.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", tolV));
		double tol = tolV.DoubleValue();

		Value other = context.GetVar("m2");
		MatrixData* m2 = ValueToMatrix(other);
		if (m2 != nullptr) {
			if (m2 == m) return IntrinsicResult::One;
			if (m2->rows != m->rows || m2->columns != m->columns) {
				return IntrinsicResult::Zero;
			}
			long n = m->LiveElems();
			for (long i = 0; i < n; i++) {
				double d = m->data[i] - m2->data[i];
				if (d < 0) d = -d;
				if (!(d <= tol)) return IntrinsicResult::Zero;
			}
			return IntrinsicResult::One;
		}
		if (other.Type() == ValueType::List) {
			// A list means what fromList would make of it, but compared in
			// place: building the matrix just to throw it away would allocate
			// for nothing.
			ValueList outer = other.GetList();
			bool nested; int rows, columns;
			if (!ListShape(outer, &nested, &rows, &columns, "equals", &err)) {
				return IntrinsicResult(err);
			}
			if (rows != m->rows || columns != m->columns) {
				return IntrinsicResult::Zero;
			}
			for (int r = 0; r < rows; r++) {
				for (int c = 0; c < columns; c++) {
					double d = m->data[(long)r * columns + c] - ListElem(outer, nested, r, c);
					if (d < 0) d = -d;
					if (!(d <= tol)) return IntrinsicResult::Zero;
				}
			}
			return IntrinsicResult::One;
		}
		return IntrinsicResult(ErrorTypes::RuntimeError(
			"Matrix.equals: expected a Matrix or a list"));
	});
	matrixClass.SetValue(String("equals"), f.GetFunc());

	// m.size -> [rows, columns]
	//
	// The canonical shape accessor: a list of extents, so a hypothetical N-D
	// future would not break the API.  `rows` and `columns` are the 2D
	// conveniences, and they are plain map entries on the instance (written by
	// MatrixToValue) rather than intrinsics, so `m.rows` costs a map lookup
	// rather than a call.  A fresh list per call, rather than a stored entry,
	// so that construction does not allocate one for a matrix nobody asks.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		ValueList dims;
		dims.Add(Value(m->rows));
		dims.Add(Value(m->columns));
		return IntrinsicResult(DynamicList(dims));
	});
	matrixClass.SetValue(String("size"), f.GetFunc());

	// m.capacity -> current element capacity (read-only report)
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		return IntrinsicResult(Value((double)m->capacityElems));
	});
	matrixClass.SetValue(String("capacity"), f.GetFunc());

	return matrixClass;
}

} // namespace MiniScript
