//
//  Matrix.h
//  raylib-miniscript
//
//  Intrinsic Matrix class: fast bulk linear algebra over a dense, row-major
//  block of doubles.  See notes/MATRIX_DESIGN.md for the full design.
//
//  The numeric payload lives behind a GCHandle, so a matrix is freed when the
//  last MiniScript map referring to it becomes unreachable -- no dispose call.
//

#ifndef MATRIX_H
#define MATRIX_H

#include "miniscript.h"

namespace MiniScript {

// The storage behind a Matrix instance.
//
// Row-major: row r begins at data[r * columns].  The live region is
// data[0 .. rows*columns-1] and is always contiguous; spare capacity sits after
// it, which is what makes adding rows amortized O(1) and reshape O(1).
//
// Capacity is measured in *elements*, not rows, so that a reshape to a
// different column count leaves the number meaningful.  Stride is always
// exactly `columns` -- there is no separate column capacity, so changing the
// column count is a realloc-and-copy and is documented as expensive.
struct MatrixData {
	double* data = nullptr;
	long capacityElems = 0;
	int rows = 0;
	int columns = 0;

	long LiveElems() const { return (long)rows * (long)columns; }

	~MatrixData() { free(data); }
};

// Upper bound on a single matrix, in elements.  32M doubles is 256MB; a request
// beyond this returns an error value rather than attempting the allocation, so
// a typo in a computed size fails as a script error instead of an OOM abort.
extern const long kMaxMatrixElems;

// The Matrix class map (an intrinsic MiniScript class).
ValueDict& MatrixClass();

// Wrap storage in a fresh Matrix instance map.  Takes ownership: the returned
// value's handle finalizer deletes `data`.
Value MatrixToValue(MatrixData* data);

// The MatrixData behind a Matrix value, or null if it is not one (including a
// bare `new Matrix`, whose handle is null).
MatrixData* ValueToMatrix(Value value);

} // namespace MiniScript

#endif // MATRIX_H
