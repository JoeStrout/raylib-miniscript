//
//  MatrixInternal.h
//  raylib-miniscript
//
//  The seam between MatrixCore.cpp and MatrixClass.cpp.  Not part of the host's
//  public surface -- Matrix.h is that.  Only those two files include this.
//
//  The split runs along one rule: **MatrixClass.cpp is everything that reads an
//  intrinsic's arguments or builds its result; MatrixCore.cpp is everything
//  else.**  Concretely, `context.GetVar` appears only in MatrixClass.cpp.  So
//  the core still speaks `Value` where an operand is genuinely polymorphic (a
//  scalar, a Matrix, or a list -- see Gemm, Solve, RowCross,
//  SoftmaxCrossEntropy) and still reports failures as MiniScript error values.
//  What it does not do is know anything about parameter names, defaults, or the
//  intrinsic table.
//
//  Declared here are exactly the back-end entry points the class layer calls.
//  Everything else in MatrixCore.cpp stays `static`.  They live in a nested
//  namespace because names like Gemm, Solve and kHandle are too generic to put
//  into MiniScript's own namespace, where other host modules already use them
//  (RawData.cpp and InterpModule.cpp each have their own file-static kHandle).
//

#ifndef MATRIX_INTERNAL_H
#define MATRIX_INTERNAL_H

#include "Matrix.h"
#include "RawData.h"
#include "miniscript.h"

namespace MiniScript {
namespace MatrixInternal {

//--------------------------------------------------------------------------------
// Storage and instance plumbing  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

// The instance map's keys.  Lazily built, interned, and therefore immortal.
const Value& kHandle();
const Value& kRows();
const Value& kColumns();

// Raw element allocation, retried once after a collection.  Null on failure.
double* AllocElems(long elems);

// Storage for a rows x columns matrix, all zeros / the identity.  Null if the
// size is out of range or the allocation failed; the caller says which op.
MatrixData* NewMatrixData(int rows, int columns);
MatrixData* NewIdentity(int size);

// Make at least `needed` elements addressable, growing (never shrinking) as
// needed.  False if out of range or out of memory.
bool EnsureCapacity(MatrixData* m, long needed);

// Rewrite the instance map's rows/columns entries after a shape change.  Every
// intrinsic that resizes must call this or `m.rows` reports the old shape.
void SyncShape(Value self, MatrixData* m);

//--------------------------------------------------------------------------------
// Reading a list as a matrix  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

// The shape a list argument denotes, by the same rules as Matrix.fromList.
// Validates rectangularity and numericness, so ListElem needs no re-checking.
bool ListShape(ValueList outer, bool* outNested, int* outRows, int* outCols,
               const char* who, Value* outErr);
double ListElem(ValueList outer, bool nested, int r, int c);

//--------------------------------------------------------------------------------
// gemm  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

// How a second operand maps onto the m x n result.
enum AddendMode {
	kAddendNone,        // absent, or beta == 0
	kAddendScalar,      // a number, or a 1x1 matrix
	kAddendFull,        // exactly m x n
	kAddendRow,         // 1 x n, broadcast down every row
	kAddendCol          // m x 1, broadcast across every column
};

// A second operand that must match, or broadcast to, an m x n matrix: a number,
// a Matrix, or a list (read exactly as Matrix.fromList would read it).  Reuses
// the AddendMode classification gemm's addend uses, so the broadcasting rules
// cannot drift between gemm and the elementwise ops.
struct Broadcast {
	AddendMode mode = kAddendNone;
	double scalar = 0.0;
	const double* data = nullptr;
	int stride = 0;
	double At(int i, int j) const {
		switch (mode) {
			case kAddendScalar: return scalar;
			case kAddendFull:   return data[(long)i * stride + j];
			case kAddendRow:    return data[j];
			case kAddendCol:    return data[i];
			default:            return 0.0;
		}
	}
};

// Resolve such an operand.  A list is materialized into the core's shared
// scratch buffer, so a caller must resolve at most ONE broadcast operand per
// intrinsic -- a second one may realloc the buffer and invalidate the first.
bool ResolveBroadcast(Value v, int m, int n, const char* who,
                      Broadcast* out, Value* outErr);

// out := alpha * op(A) * op(B) + beta * addend.  B null is the level-1 path;
// out null allocates a fresh result.  Returns the result, or null with *outErr.
MatrixData* Gemm(MatrixData* A, MatrixData* B, Value vAdd, MatrixData* out,
                 bool transA, bool transB, double alpha, double beta,
                 Value* outErr);

//--------------------------------------------------------------------------------
// Reductions  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

enum ReduceOp {
	kReduceSum, kReduceSumSq, kReduceMax, kReduceMin, kReduceArgMax, kReduceArgMin
};

// True for the ops that need at least one element (max/min/argmax/argmin).
bool ReduceNeedsElement(int op);

// Reduce `count` elements starting at `base`, `stride` apart.
double ReduceRun(const double* base, long count, long stride, int op);

//--------------------------------------------------------------------------------
// Linear algebra  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

bool Determinant(const MatrixData* m, double* out, Value* outErr);
MatrixData* Inverse(const MatrixData* m, Value* outErr);
MatrixData* Solve(const MatrixData* A, Value vB, Value* outErr);
MatrixData* RowCross(const MatrixData* A, Value vB, Value* outErr);

//--------------------------------------------------------------------------------
// Neural network primitives  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

double StableSigmoid(double x);
void SoftmaxInPlace(MatrixData* m, int axis);
MatrixData* SoftmaxCrossEntropy(const MatrixData* logits, Value vTargets,
                                Value vOutProbs, Value* outErr);

//--------------------------------------------------------------------------------
// Serialization  (MatrixCore.cpp)
//--------------------------------------------------------------------------------

// An element format.  `code` is what goes in the file header, so these numbers
// are part of the format; see MatrixCore.cpp for the table.
struct DType {
	const char* name;
	int code;
	int size;
	bool isFloat;
	double lo, hi;      // representable range, for the integer formats
};

// Look up a dtype by name.  With `allowAuto`, "auto" yields a null dtype and
// true -- "read the header for it", which is only meaningful on read.
bool ParseDType(Value v, bool allowAuto, const char* who,
                const DType** out, Value* outErr);

// Write / read a matrix's elements to or from a RawData, with or without our
// 16-byte header.  Both return the ending position, or -1 with *outErr set.
int MatrixToRawData(const MatrixData* m, Value rdValue, const DType* dt,
                    int startPos, bool includeHeader, Value* outErr);
int MatrixFromRawData(MatrixData* m, BinaryData* rd, const DType* dt,
                      int startPos, int rows, int columns,
                      const char* who, Value* outErr);

} // namespace MatrixInternal
} // namespace MiniScript

#endif // MATRIX_INTERNAL_H
