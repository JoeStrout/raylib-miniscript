//
//  MatrixClass.cpp
//  raylib-miniscript
//
//  The MiniScript face of the Matrix class: the intrinsic table itself, and the
//  helpers that unpack an intrinsic's arguments into the plain values the back
//  end takes.  `context.GetVar` appears in this file and nowhere else -- that is
//  the rule the Matrix/MatrixCore split runs along.  The numeric work lives in
//  MatrixCore.cpp; MatrixInternal.h declares what this file may call.
//
//  See notes/MATRIX_DESIGN.md for the API these intrinsics implement.
//

#include "MatrixInternal.h"
#include "PRNG.g.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <cstdio>

namespace MiniScript {

using namespace MatrixInternal;

#define INTRINSIC_LAMBDA [](Context context, IntrinsicResult partialResult) -> IntrinsicResult

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

// Reduction axis: null (whole matrix) is reported as -1; 0 reduces down the
// columns to a 1 x n result, 1 reduces across the rows to an m x 1 result.
// Same axis meaning as numpy, differing only in that we return a matrix rather
// than a 1-D array.
static bool AxisArg(Value v, int* out, Value* outErr) {
	if (v.IsNull()) { *out = -1; return true; }
	if (v.Type() != ValueType::Number) {
		*outErr = ErrorTypes::TypeError("number", v);
		return false;
	}
	int a = v.IntValue();
	if (a != 0 && a != 1) {
		*outErr = ErrorTypes::RuntimeError("Matrix: axis must be 0, 1, or null");
		return false;
	}
	*out = a;
	return true;
}

// The body of every reduction intrinsic.
static IntrinsicResult DoReduce(Context context, int op, const char* who) {
	Value err;
	MatrixData* m = SelfMatrix(context, &err);
	if (m == nullptr) return IntrinsicResult(err);
	int axis;
	if (!AxisArg(context.GetVar("axis"), &axis, &err)) return IntrinsicResult(err);

	if (axis < 0) {
		// Whole matrix.  The live region is contiguous, so this is one run --
		// which also makes an argmax a FLAT index, exactly as numpy's argmax
		// over a flattened array reports it.
		long count = m->LiveElems();
		if (count == 0) {
			if (ReduceNeedsElement(op)) return IntrinsicResult(ErrorTypes::RuntimeError(
				String("Matrix.") + who + ": cannot reduce an empty matrix"));
			return IntrinsicResult::Zero;
		}
		return IntrinsicResult(Value(ReduceRun(m->data, count, 1, op)));
	}

	int runLen = (axis == 0) ? m->rows : m->columns;
	if (runLen == 0 && ReduceNeedsElement(op)) {
		return IntrinsicResult(ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": cannot reduce a zero-length axis"));
	}
	int outRows = (axis == 0) ? 1 : m->rows;
	int outCols = (axis == 0) ? m->columns : 1;
	MatrixData* out = NewMatrixData(outRows, outCols);
	if (out == nullptr) return IntrinsicResult(ErrorTypes::RuntimeError(
		String("Matrix.") + who + ": out of memory"));

	if (axis == 0) {
		// Down each column: start at the top of the column, step by the stride.
		for (int j = 0; j < m->columns; j++) {
			out->data[j] = (runLen == 0) ? 0.0
				: ReduceRun(m->data + j, runLen, m->columns, op);
		}
	} else {
		// Across each row: contiguous.
		for (int i = 0; i < m->rows; i++) {
			out->data[i] = (runLen == 0) ? 0.0
				: ReduceRun(m->data + (long)i * m->columns, runLen, 1, op);
		}
	}
	return IntrinsicResult(MatrixToValue(out));
}

// One matrix element as display text.
//
// With a precision, fixed-point with that many decimals; without, MiniScript's
// own number formatting, so a matrix shows its numbers exactly the way `print`
// would show them individually.  NaN and the infinities are spelled the way
// MiniScript spells them, so the two cannot disagree.
static std::string FormatElem(double v, bool hasPrecision, int precision) {
	if (std::isnan(v)) return "NaN";
	if (std::isinf(v)) return v < 0 ? "-Inf" : "Inf";
	if (!hasPrecision) return std::string(Value(v).ToString().c_str());
	char buf[512];
	snprintf(buf, sizeof(buf), "%.*f", precision, v);
	return std::string(buf);
}

// Common front end for all three entry points: fetch the RawData argument and
// validate startPos against it.  `rd` may still have no buffer at all (length
// 0), which is fine for a write that is about to grow it.
static bool RawDataArg(Context context, const char* who,
                       Value* outValue, BinaryData** outData, int* outStart,
                       Value* outErr) {
	Value v = context.GetVar("rd");
	BinaryData* data = ValueToRawData(v);
	if (data == nullptr && v.Type() != ValueType::Map) {
		*outErr = ErrorTypes::RuntimeError(String("Matrix.") + who + ": RawData required");
		return false;
	}
	Value vs = context.GetVar("startPos");
	if (vs.Type() != ValueType::Number) {
		*outErr = ErrorTypes::TypeError("number", vs);
		return false;
	}
	int start = vs.IntValue();
	if (start < 0) {
		*outErr = ErrorTypes::RuntimeError(
			String("Matrix.") + who + ": startPos must be >= 0");
		return false;
	}
	*outValue = v;
	*outData = data;
	*outStart = start;
	return true;
}

//--------------------------------------------------------------------------------
// Callback fill
//--------------------------------------------------------------------------------

// Fill m->data[0 .. elems-1] by calling a MiniScript function once per element,
// in row-major order.  The function is invoked with no arguments; each call's
// result must be a number.
//
// The re-entrant call is VM::RunFunction, which pushes the callee's frame above
// this intrinsic's own registers and drives the VM until it returns.  That means
// the callback can do anything a normal function can -- including allocating,
// and so triggering a collection.  The MatrixData we are filling is not a GC
// object yet (its handle is made later, by MatrixToValue), so a collection
// cannot take it; but it is also owned by nobody but us, so every early exit
// below has to delete it.  That is the caller's job, and the caller does it.
//
// Returns true on success.  On failure, *outErr holds what the caller should
// return in place of the matrix:
//   - the callback's own error value, if it returned one;
//   - a TypeError, if it returned anything else that isn't a number;
//   - null, if the callback RAISED a runtime error rather than returning one.
//     RunFunction has already surfaced that on the outer run and stopped the
//     VM, so there is nothing left for us to report -- we just stop filling.
static bool FillFromCallback(Context context, MatrixData* m, long elems,
							 Value fn, Value* outErr) {
	*outErr = Value::Null;
	ValueList noArgs;
	for (long i = 0; i < elems; i++) {
		Value v = context.vm.RunFunction(fn, noArgs);
		if (!context.vm.IsRunning) return false;
		if (v.IsError()) { *outErr = v; return false; }
		if (v.Type() != ValueType::Number) {
			*outErr = ErrorTypes::TypeError("number", v);
			return false;
		}
		m->data[i] = v.DoubleValue();
	}
	return true;
}

// Drive `fn` over every element of a LIVE matrix, in row-major order, replacing
// each element with what the call returns.  `arg1` is passed as a second
// argument when `hasArg1` -- the apply1 form.
//
// Unlike FillFromCallback, the matrix here is already a GC object reachable
// from `self`, so the callback can reach it too, and can resize it -- which
// reallocs m->data out from under us.  Hence the reentrancy restriction the
// design notes call for: the shape is rechecked after every call, and a
// callback that changed it stops the loop with an error rather than writing
// through a stale pointer or past a new, shorter end.  m->data is re-read every
// iteration for the same reason, since reserve/trim can move the buffer without
// changing the shape.
//
// Returns true on success; *outErr on failure follows FillFromCallback's rules
// (the callback's own error value, a TypeError, or null when the callback
// raised and the VM has already stopped).
static bool ApplyCallback(Context context, MatrixData* m, Value fn,
						  bool hasArg1, Value arg1, const char* who, Value* outErr) {
	*outErr = Value::Null;
	int rows = m->rows, columns = m->columns;
	long n = m->LiveElems();
	for (long i = 0; i < n; i++) {
		ValueList args;
		args.Add(Value(m->data[i]));
		if (hasArg1) args.Add(arg1);
		Value v = context.vm.RunFunction(fn, args);
		if (!context.vm.IsRunning) return false;
		if (v.IsError()) { *outErr = v; return false; }
		if (v.Type() != ValueType::Number) {
			*outErr = ErrorTypes::TypeError("number", v);
			return false;
		}
		if (m->rows != rows || m->columns != columns) {
			*outErr = ErrorTypes::RuntimeError(String("Matrix.") + who +
				": the matrix was resized by the callback");
			return false;
		}
		m->data[i] = v.DoubleValue();
	}
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
	// initialValue may be a number, or a function of no arguments called once
	// per element in row-major order (see FillFromCallback).
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

		// Note that null is NOT accepted: the parameter already defaults to 0,
		// so an explicit null would be a second spelling of the same thing.
		Value initial = context.GetVar("initialValue");
		bool isFunc = (initial.Type() == ValueType::Function);
		if (!isFunc && initial.Type() != ValueType::Number) {
			return IntrinsicResult(ErrorTypes::TypeError("number", initial));
		}

		MatrixData* m = NewMatrixData(rows, columns);
		if (m == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.ofSize: out of memory"));
		}
		if (isFunc) {
			Value err;
			if (!FillFromCallback(context, m, elems, initial, &err)) {
				delete m;
				return IntrinsicResult(err);
			}
		} else {
			// NewMatrixData zero-fills, so only a nonzero value needs work.
			double init = initial.DoubleValue();
			if (init != 0.0) {
				for (long i = 0; i < elems; i++) m->data[i] = init;
			}
		}
		return IntrinsicResult(MatrixToValue(m));
	});
	matrixClass.SetValue(String("ofSize"), f.GetFunc());

	// Matrix.identity(size) -> new size x size Matrix, ones on the main diagonal
	f = Intrinsic::Create("");
	f.AddParam("size");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		int size = 0;
		if (!IntArg(context, "size", &size, &err)) return IntrinsicResult(err);
		if (size < 0) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.identity: size must be >= 0"));
		}
		if ((long)size * (long)size > kMaxMatrixElems) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.identity: requested size exceeds the maximum matrix size"));
		}
		MatrixData* m = NewIdentity(size);
		if (m == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.identity: out of memory"));
		}
		return IntrinsicResult(MatrixToValue(m));
	});
	matrixClass.SetValue(String("identity"), f.GetFunc());

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

	// Matrix.fromRawData(rd, dtype="auto", startPos=0, rows=null, columns=null)
	//   -> new Matrix
	//
	// With the default dtype the data carries a header and supplies its own
	// dtype and shape, so rows/columns must be left out.  With an explicit
	// dtype the data is headerless and the shape is yours to give: name both,
	// or name one and let the other follow from how much data is left.
	f = Intrinsic::Create("");
	f.AddParam("rd");
	f.AddParam("dtype", "auto");
	f.AddParam("startPos", 0);
	f.AddParam("rows", Value::Null);
	f.AddParam("columns", Value::Null);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		Value rdValue; BinaryData* rd; int startPos;
		if (!RawDataArg(context, "fromRawData", &rdValue, &rd, &startPos, &err)) {
			return IntrinsicResult(err);
		}
		const DType* dt;
		if (!ParseDType(context.GetVar("dtype"), true, "fromRawData", &dt, &err)) {
			return IntrinsicResult(err);
		}
		Value vr = context.GetVar("rows"), vc = context.GetVar("columns");
		int rows = 0, columns = 0;
		if (dt == nullptr) {
			// The header is the authority on shape; accepting rows/columns too
			// would mean deciding what to do when they disagree with it.
			if (!vr.IsNull() || !vc.IsNull()) {
				return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.fromRawData: rows/columns cannot be given with dtype \"auto\""
					" (the header supplies the shape)"));
			}
		} else {
			if (!vr.IsNull() && vr.Type() != ValueType::Number) {
				return IntrinsicResult(ErrorTypes::TypeError("number", vr));
			}
			if (!vc.IsNull() && vc.Type() != ValueType::Number) {
				return IntrinsicResult(ErrorTypes::TypeError("number", vc));
			}
			if (vr.IsNull() && vc.IsNull()) {
				return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.fromRawData: headerless data needs rows and/or columns"));
			}
			rows = vr.IsNull() ? -1 : vr.IntValue();
			columns = vc.IsNull() ? -1 : vc.IntValue();
			if (rows < -1 || columns < -1) {
				return IntrinsicResult(ErrorTypes::RuntimeError(
					"Matrix.fromRawData: rows and columns must be >= 0"));
			}
			if (rows < 0 || columns < 0) {
				// Infer the missing extent from what is left in the buffer.
				// Division must be exact: a leftover tail means the data is not
				// the shape the caller thinks it is, which is worth an error
				// rather than a silently truncated matrix.
				int length = (rd == nullptr) ? 0 : rd->length;
				long avail = ((long)length - startPos) / dt->size;
				int known = (rows < 0) ? columns : rows;
				if (known == 0) {
					rows = columns = 0;
				} else if (avail % known != 0) {
					return IntrinsicResult(ErrorTypes::RuntimeError(
						"Matrix.fromRawData: the remaining data is not a whole"
						" number of rows/columns at that size"));
				} else if (rows < 0) {
					rows = (int)(avail / known);
				} else {
					columns = (int)(avail / known);
				}
			}
		}

		MatrixData* m = NewMatrixData(0, 0);
		if (m == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError("Matrix.fromRawData: out of memory"));
		}
		int end = MatrixFromRawData(m, rd, dt, startPos, rows, columns,
		                            "fromRawData", &err);
		if (end < 0) { delete m; return IntrinsicResult(err); }
		return IntrinsicResult(MatrixToValue(m));
	});
	matrixClass.SetValue(String("fromRawData"), f.GetFunc());

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

	// A.gemm(B, addend=null, out=null, transA=false, transB=false, alpha=1, beta=1)
	//
	//     alpha * op(A) * op(B) + beta * addend
	//
	// B null  -> alpha*op(A) + beta*addend (the level-1 path)
	// addend  -> null, a number, a 1xn / mx1 vector (broadcast), or a full mxn
	// out     -> null allocates a fresh result; otherwise written in place and
	//            resized as needed, and returned
	//
	// Aliasing is allowed everywhere: an input that shares storage with `out` is
	// snapshotted into scratch before `out` is touched.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("B");
	f.AddParam("addend");
	f.AddParam("out");
	f.AddParam("transA", Value::zero);
	f.AddParam("transB", Value::zero);
	f.AddParam("alpha", Value::one);
	f.AddParam("beta", Value::one);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* A = SelfMatrix(context, &err);
		if (A == nullptr) return IntrinsicResult(err);

		Value vAlpha = context.GetVar("alpha");
		Value vBeta = context.GetVar("beta");
		if (vAlpha.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vAlpha));
		if (vBeta.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vBeta));

		Value vB = context.GetVar("B");
		MatrixData* B = nullptr;
		if (!vB.IsNull()) {
			B = ValueToMatrix(vB);
			if (B == nullptr) return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.gemm: B must be a Matrix or null"));
		}

		Value vOut = context.GetVar("out");
		MatrixData* out = nullptr;
		if (!vOut.IsNull()) {
			out = ValueToMatrix(vOut);
			if (out == nullptr) return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.gemm: out must be a Matrix or null"));
		}

		MatrixData* result = Gemm(A, B, context.GetVar("addend"), out,
			context.GetVar("transA").BoolValue(), context.GetVar("transB").BoolValue(),
			vAlpha.DoubleValue(), vBeta.DoubleValue(), &err);
		if (result == nullptr) return IntrinsicResult(err);

		if (out == nullptr) return IntrinsicResult(MatrixToValue(result));
		SyncShape(vOut, out);
		return IntrinsicResult(vOut);
	});
	matrixClass.SetValue(String("gemm"), f.GetFunc());

	// ---- Elementwise, in place ----
	//
	// Only the mutating forms are intrinsic; the copying counterparts
	// (elemTimes, elemOver, powed, absed, sqrted, rounded, clamped) are
	// clone-then-mutate wrappers in the script layer.
	//
	// Each takes a number, a Matrix, or a list, with row/column broadcasting --
	// the same rules gemm's addend follows, from the same code.

	// m.elemMultiplyBy(x) -> self   (Hadamard product when x is a Matrix)
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("x");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Broadcast b;
		if (!ResolveBroadcast(context.GetVar("x"), m->rows, m->columns, "elemMultiplyBy", &b, &err)) {
			return IntrinsicResult(err);
		}
		for (int i = 0; i < m->rows; i++) {
			double* row = m->data + (long)i * m->columns;
			for (int j = 0; j < m->columns; j++) row[j] *= b.At(i, j);
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("elemMultiplyBy"), f.GetFunc());

	// m.elemDivideBy(x) -> self
	//
	// Division by zero yields inf or NaN rather than an error, following IEEE
	// and numpy: a matrix op should not abort a whole batch over one element.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("x");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Broadcast b;
		if (!ResolveBroadcast(context.GetVar("x"), m->rows, m->columns, "elemDivideBy", &b, &err)) {
			return IntrinsicResult(err);
		}
		for (int i = 0; i < m->rows; i++) {
			double* row = m->data + (long)i * m->columns;
			for (int j = 0; j < m->columns; j++) row[j] /= b.At(i, j);
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("elemDivideBy"), f.GetFunc());

	// m.pow(k) -> self   (k may broadcast, as numpy's ** does)
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("k");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Broadcast b;
		if (!ResolveBroadcast(context.GetVar("k"), m->rows, m->columns, "pow", &b, &err)) {
			return IntrinsicResult(err);
		}
		for (int i = 0; i < m->rows; i++) {
			double* row = m->data + (long)i * m->columns;
			for (int j = 0; j < m->columns; j++) row[j] = std::pow(row[j], b.At(i, j));
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("pow"), f.GetFunc());

	// m.abs -> self
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) if (m->data[i] < 0) m->data[i] = -m->data[i];
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("abs"), f.GetFunc());

	// m.sqrt -> self   (negative input gives NaN, as in numpy)
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) m->data[i] = std::sqrt(m->data[i]);
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("sqrt"), f.GetFunc());

	// m.round(places=0) -> self
	//
	// Rounds half AWAY FROM ZERO, matching MiniScript's own round() -- not
	// numpy, which rounds half to even.  Agreeing with the language matters
	// more here: m.round and round(m.getElem(...)) must not disagree.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("places", Value::zero);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int places = 0;
		if (!IntArg(context, "places", &places, &err)) return IntrinsicResult(err);
		if (places > 15) places = 15;
		double factor = std::pow(10.0, (double)places);
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) m->data[i] = std::round(m->data[i] * factor) / factor;
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("round"), f.GetFunc());

	// m.clamp(lo, hi) -> self
	//
	// Either bound may be null for an open end.  Bounds are plain numbers
	// rather than broadcastable operands: two broadcast operands would need two
	// scratch segments, and a per-element clamp range is a rare thing to want.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("lo");
	f.AddParam("hi");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value vlo = context.GetVar("lo");
		Value vhi = context.GetVar("hi");
		bool hasLo = !vlo.IsNull(), hasHi = !vhi.IsNull();
		if (hasLo && vlo.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vlo));
		if (hasHi && vhi.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vhi));
		double lo = hasLo ? vlo.DoubleValue() : 0.0;
		double hi = hasHi ? vhi.DoubleValue() : 0.0;
		if (hasLo && hasHi && lo > hi) return IntrinsicResult(ErrorTypes::RuntimeError(
			"Matrix.clamp: lo must not exceed hi"));
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) {
			double v = m->data[i];
			if (hasLo && v < lo) v = lo;
			if (hasHi && v > hi) v = hi;
			m->data[i] = v;
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("clamp"), f.GetFunc());

	// ---- Bulk transforms ----

	// m.fill(value=0) -> self
	//
	// Not routed through gemm: the alpha*A term always READS A, and a fill must
	// not -- spare capacity brought into the live region may hold anything, and
	// 0 * NaN is NaN.  So this is the one op that needs its own write-only pass.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("value", Value::zero);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value v = context.GetVar("value");
		if (v.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", v));
		double x = v.DoubleValue();
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) m->data[i] = x;
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("fill"), f.GetFunc());

	// m.randomize(mean=0, sd=1) -> self
	//
	// Normal (Gaussian) deviates, drawn from the interpreter's own PRNG -- the
	// same stream rnd() uses -- so rnd(seed) makes weight initialization
	// reproducible across runs and platforms.
	//
	// Box-Muller, filling two elements per transform: the sine and cosine
	// outputs are both usable deviates, and an odd element count simply drops
	// the second.  sd of 0 is legal and fills with the mean.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("mean", Value::zero);
	f.AddParam("sd", Value::one);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value vmean = context.GetVar("mean");
		Value vsd = context.GetVar("sd");
		if (vmean.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vmean));
		if (vsd.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vsd));
		double mean = vmean.DoubleValue(), sd = vsd.DoubleValue();
		if (sd < 0) return IntrinsicResult(ErrorTypes::RuntimeError(
			"Matrix.randomize: sd must be >= 0"));
		const double kTwoPi = 6.28318530717958647692;
		long n = m->LiveElems();
		for (long i = 0; i < n; i += 2) {
			// PRNG::Next() is [0,1); 1-u moves the log's argument to (0,1], so
			// the -inf case is unreachable rather than merely unlikely.
			double r = sd * std::sqrt(-2.0 * std::log(1.0 - PRNG::Next()));
			double theta = kTwoPi * PRNG::Next();
			m->data[i] = mean + r * std::cos(theta);
			if (i + 1 < n) m->data[i + 1] = mean + r * std::sin(theta);
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("randomize"), f.GetFunc());

	// m.apply(func) -> self          func(value) -> value
	// m.apply1(func, arg1) -> self   func(value, arg1) -> value
	//
	// In place, row-major, like list.apply / list.apply1.  Index-dependent
	// initialization is deliberately not offered: the callback sees the value
	// and nothing else (build a list and use fromList if you need the index).
	// See ApplyCallback for what a callback may and may not do to the matrix.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("func");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value fn = context.GetVar("func");
		if (fn.Type() != ValueType::Function) return IntrinsicResult(ErrorTypes::TypeError("function", fn));
		if (!ApplyCallback(context, m, fn, false, Value::Null, "apply", &err)) {
			return IntrinsicResult(err);
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("apply"), f.GetFunc());

	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("func");
	f.AddParam("arg1");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value fn = context.GetVar("func");
		if (fn.Type() != ValueType::Function) return IntrinsicResult(ErrorTypes::TypeError("function", fn));
		if (!ApplyCallback(context, m, fn, true, context.GetVar("arg1"), "apply1", &err)) {
			return IntrinsicResult(err);
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("apply1"), f.GetFunc());

	// m.sum(axis=null) / sumOfSquares / max / min / argmax / argmin
	//
	// axis null -> a scalar; axis 0 -> 1 x n (down the columns);
	// axis 1 -> m x 1 (across the rows).  Same axis meaning as numpy.
	{
		struct Reg { const char* name; int op; };
		static const Reg regs[] = {
			{ "sum",           kReduceSum    },
			{ "sumOfSquares",  kReduceSumSq  },
			{ "max",           kReduceMax    },
			{ "min",           kReduceMin    },
			{ "argmax",        kReduceArgMax },
			{ "argmin",        kReduceArgMin },
		};
		// One lambda per op, since an intrinsic's code must be captureless.
		NativeCallbackDelegate codes[] = {
			INTRINSIC_LAMBDA { return DoReduce(context, kReduceSum, "sum"); },
			INTRINSIC_LAMBDA { return DoReduce(context, kReduceSumSq, "sumOfSquares"); },
			INTRINSIC_LAMBDA { return DoReduce(context, kReduceMax, "max"); },
			INTRINSIC_LAMBDA { return DoReduce(context, kReduceMin, "min"); },
			INTRINSIC_LAMBDA { return DoReduce(context, kReduceArgMax, "argmax"); },
			INTRINSIC_LAMBDA { return DoReduce(context, kReduceArgMin, "argmin"); },
		};
		for (int i = 0; i < 6; i++) {
			f = Intrinsic::Create("");
			f.AddParam("self");
			f.AddParam("axis");
			f.set_Code(codes[i]);
			matrixClass.SetValue(String(regs[i].name), f.GetFunc());
		}
	}

	// ---- Linear algebra ----

	// m.determinant -> number.  Square only; singular gives 0.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		double det = 0.0;
		if (!Determinant(m, &det, &err)) return IntrinsicResult(err);
		return IntrinsicResult(Value(det));
	});
	matrixClass.SetValue(String("determinant"), f.GetFunc());

	// m.inverse -> new Matrix, or an error value if singular.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		MatrixData* inv = Inverse(m, &err);
		if (inv == nullptr) return IntrinsicResult(err);
		return IntrinsicResult(MatrixToValue(inv));
	});
	matrixClass.SetValue(String("inverse"), f.GetFunc());

	// m.solve(b) -> new Matrix X with m*X = b.  LU with partial pivoting.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("b");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		MatrixData* x = Solve(m, context.GetVar("b"), &err);
		if (x == nullptr) return IntrinsicResult(err);
		return IntrinsicResult(MatrixToValue(x));
	});
	matrixClass.SetValue(String("solve"), f.GetFunc());

	// m.swapRows(row1, row2) -> self.  Negative indices count from the end.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("row1");
	f.AddParam("row2");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int r1 = 0, r2 = 0;
		if (!ResolveIndex(context.GetVar("row1"), m->rows, "row", &r1, &err)) return IntrinsicResult(err);
		if (!ResolveIndex(context.GetVar("row2"), m->rows, "row", &r2, &err)) return IntrinsicResult(err);
		if (r1 != r2) {
			double* a = m->data + (long)r1 * m->columns;
			double* b = m->data + (long)r2 * m->columns;
			for (int j = 0; j < m->columns; j++) {
				double t = a[j]; a[j] = b[j]; b[j] = t;
			}
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("swapRows"), f.GetFunc());

	// m.swapColumns(column1, column2) -> self.  Negative indices count from the end.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("column1");
	f.AddParam("column2");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int c1 = 0, c2 = 0;
		if (!ResolveIndex(context.GetVar("column1"), m->columns, "column", &c1, &err)) return IntrinsicResult(err);
		if (!ResolveIndex(context.GetVar("column2"), m->columns, "column", &c2, &err)) return IntrinsicResult(err);
		if (c1 != c2) {
			for (int i = 0; i < m->rows; i++) {
				double* row = m->data + (long)i * m->columns;
				double t = row[c1]; row[c1] = row[c2]; row[c2] = t;
			}
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("swapColumns"), f.GetFunc());

	// ---- Per-row vector ops ----

	// m.rowCross(m2) -> new n x 3 Matrix.  Both need 3 columns; a single-row
	// operand is crossed with every row.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("m2");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		MatrixData* result = RowCross(m, context.GetVar("m2"), &err);
		if (result == nullptr) return IntrinsicResult(err);
		return IntrinsicResult(MatrixToValue(result));
	});
	matrixClass.SetValue(String("rowCross"), f.GetFunc());

	// ---- Neural network primitives ----
	//
	// Derivatives are expressed in terms of a layer's OUTPUT, not its input
	// (sigmoid' = y*(1-y), tanh' = 1-y^2), so a layer caches only what it
	// returned.  Those derivatives, and softmaxCrossEntropyGrad, are one-line
	// script wrappers; only the forward passes need C++.

	// m.sigmoid -> self
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) m->data[i] = StableSigmoid(m->data[i]);
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("sigmoid"), f.GetFunc());

	// m.tanh -> self
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		long n = m->LiveElems();
		for (long i = 0; i < n; i++) m->data[i] = std::tanh(m->data[i]);
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("tanh"), f.GetFunc());

	// m.softmax(axis=1) -> self.  1 = across rows (the usual: samples in rows),
	// 0 = down columns, null = over the whole matrix.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("axis", Value::one);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		int axis = 1;
		if (!AxisArg(context.GetVar("axis"), &axis, &err)) return IntrinsicResult(err);
		SoftmaxInPlace(m, axis);
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("softmax"), f.GetFunc());

	// m.greaterThan(x) -> self, each element replaced by 1 or 0.
	//
	// Broadcasts like the elementwise ops.  This is the indicator that makes
	// relu's derivative a wrapper: y.greaterThan(0).  NaN compares false, so a
	// NaN element becomes 0.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("x");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Broadcast b;
		if (!ResolveBroadcast(context.GetVar("x"), m->rows, m->columns, "greaterThan", &b, &err)) {
			return IntrinsicResult(err);
		}
		for (int i = 0; i < m->rows; i++) {
			double* row = m->data + (long)i * m->columns;
			for (int j = 0; j < m->columns; j++) row[j] = (row[j] > b.At(i, j)) ? 1.0 : 0.0;
		}
		return IntrinsicResult(context.GetVar("self"));
	});
	matrixClass.SetValue(String("greaterThan"), f.GetFunc());

	// m.softmaxCrossEntropy(targets, outProbs=null) -> new n x 1 Matrix
	//
	// self holds LOGITS, not probabilities.  Returns the unreduced per-sample
	// loss; `.mean` is the caller's to write, and visibly so.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("targets");
	f.AddParam("outProbs");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value vOutProbs = context.GetVar("outProbs");
		MatrixData* losses = SoftmaxCrossEntropy(m, context.GetVar("targets"), vOutProbs, &err);
		if (losses == nullptr) return IntrinsicResult(err);
		if (!vOutProbs.IsNull()) SyncShape(vOutProbs, ValueToMatrix(vOutProbs));
		return IntrinsicResult(MatrixToValue(losses));
	});
	matrixClass.SetValue(String("softmaxCrossEntropy"), f.GetFunc());

	// m.toRawData(rd, dtype="float64", startPos=0, includeHeader=true)
	//   -> the position just past what was written
	//
	// The RawData grows to fit if it needs to, so writing a network's weights
	// into one blob is a loop over `pos = m.toRawData(rd, "float32", pos)`.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("rd");
	f.AddParam("dtype", "float64");
	f.AddParam("startPos", 0);
	f.AddParam("includeHeader", Value::one);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value rdValue; BinaryData* rd; int startPos;
		if (!RawDataArg(context, "toRawData", &rdValue, &rd, &startPos, &err)) {
			return IntrinsicResult(err);
		}
		const DType* dt;
		if (!ParseDType(context.GetVar("dtype"), false, "toRawData", &dt, &err)) {
			return IntrinsicResult(err);
		}
		bool header = context.GetVar("includeHeader").BoolValue();
		int end = MatrixToRawData(m, rdValue, dt, startPos, header, &err);
		if (end < 0) return IntrinsicResult(err);
		return IntrinsicResult(Value(end));
	});
	matrixClass.SetValue(String("toRawData"), f.GetFunc());

	// m.readRawData(rd, dtype="auto", startPos=0)
	//   -> the position just past what was read
	//
	// Reads into the receiver, reshaping it.  With the default dtype the data
	// must start with a header, which supplies both dtype and shape; an
	// explicit dtype means headerless data, whose shape is the receiver's
	// current one -- so `Matrix.ofSize(28, 28).readRawData(rd, "uint8")` is how
	// you pull an image out of a blob somebody else wrote.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("rd");
	f.AddParam("dtype", "auto");
	f.AddParam("startPos", 0);
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);
		Value rdValue; BinaryData* rd; int startPos;
		if (!RawDataArg(context, "readRawData", &rdValue, &rd, &startPos, &err)) {
			return IntrinsicResult(err);
		}
		const DType* dt;
		if (!ParseDType(context.GetVar("dtype"), true, "readRawData", &dt, &err)) {
			return IntrinsicResult(err);
		}
		int end = MatrixFromRawData(m, rd, dt, startPos, m->rows, m->columns,
		                            "readRawData", &err);
		if (end < 0) return IntrinsicResult(err);
		SyncShape(context.GetVar("self"), m);
		return IntrinsicResult(Value(end));
	});
	matrixClass.SetValue(String("readRawData"), f.GetFunc());

	// m.format(fieldWidth=10, precision=null, columnSep="", rowSep=null) -> string
	//
	// A human-readable table.  Elements are right-aligned in fieldWidth
	// characters, columns joined by columnSep, rows by rowSep (newline by
	// default).  Returns the string rather than printing it, so `print` is a
	// one-line script wrapper and the result can also be written to a file.
	//
	// On tiny values: the old matrixUtil.print rewrote any element whose text
	// contained "E-" to "0", to stop floating-point noise like 1e-17 from
	// dominating a table.  That also silently zeroed legitimately small values
	// such as 1.5e-5, which is why the rule is gone.  Passing a `precision`
	// handles the real case properly -- fixed-point notation renders 1e-17 as
	// "0.000" because that is genuinely its value to three places -- while
	// leaving the default honest about magnitude.
	f = Intrinsic::Create("");
	f.AddParam("self");
	f.AddParam("fieldWidth", Value(10));
	f.AddParam("precision");
	f.AddParam("columnSep", Value::emptyString);
	f.AddParam("rowSep");
	f.set_Code(INTRINSIC_LAMBDA {
		Value err;
		MatrixData* m = SelfMatrix(context, &err);
		if (m == nullptr) return IntrinsicResult(err);

		int fieldWidth = 0;
		if (!IntArg(context, "fieldWidth", &fieldWidth, &err)) return IntrinsicResult(err);
		if (fieldWidth < 0) return IntrinsicResult(ErrorTypes::RuntimeError(
			"Matrix.format: fieldWidth must be >= 0"));

		Value vPrec = context.GetVar("precision");
		bool hasPrec = !vPrec.IsNull();
		int precision = 0;
		if (hasPrec) {
			if (vPrec.Type() != ValueType::Number) return IntrinsicResult(ErrorTypes::TypeError("number", vPrec));
			precision = vPrec.IntValue();
			if (precision < 0 || precision > 15) return IntrinsicResult(ErrorTypes::RuntimeError(
				"Matrix.format: precision must be between 0 and 15"));
		}

		std::string colSep = context.GetVar("columnSep").ToString().c_str();
		Value vRowSep = context.GetVar("rowSep");
		std::string rowSep = vRowSep.IsNull() ? "\n" : std::string(vRowSep.ToString().c_str());

		std::string out;
		for (int i = 0; i < m->rows; i++) {
			if (i > 0) out += rowSep;
			const double* row = m->data + (long)i * m->columns;
			for (int j = 0; j < m->columns; j++) {
				if (j > 0) out += colSep;
				std::string s = FormatElem(row[j], hasPrec, precision);
				if (fieldWidth > 0) {
					// Truncate to fieldWidth-1 rather than fieldWidth, so that a
					// too-wide number still leaves one space before the next
					// column and the table cannot run together.  Only ever
					// truncate a value that has a fractional part, so the
					// integer magnitude is never falsified.
					if ((int)s.size() >= fieldWidth && s.find('.') != std::string::npos) {
						s = s.substr(0, fieldWidth - 1);
					}
					if ((int)s.size() < fieldWidth) s.insert(0, fieldWidth - s.size(), ' ');
				}
				out += s;
			}
		}
		return IntrinsicResult(String(out.c_str()));
	});
	matrixClass.SetValue(String("format"), f.GetFunc());

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

	// Register the class under a short name.  Value.CodeForm consults it when
	// stringifying an __isa entry, which is the difference between str(m)
	// reporting `{"__isa": Matrix, ...}` and dumping every method in the class.
	Intrinsic::AddShortName(StaticMap(matrixClass), String("Matrix"));

	return matrixClass;
}

} // namespace MiniScript
