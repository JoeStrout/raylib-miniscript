//
//  PhysicsCore.cpp
//  raylib-miniscript
//
//  The physicsCore module: the per-contact work of assets/physics.ms, a 2D
//  rigid-body engine written in MiniScript, done natively.
//  assets/physicsFallback.ms is the same API in MiniScript, which physics.ms
//  uses when this module isn't present.  Keep the two in step: each function
//  here does the same arithmetic in the same order as its namesake there, so
//  a scene gives the same result either way (assets/physicsBench.ms checks
//  this) -- exactly, unless the compiler fuses multiply-adds (as it may on
//  ARM), which rounds differently.
//
//  Data is passed as Matrix values:
//
//  - Body state, one row per body: pos [x, y, angle] of the center of mass,
//    vel [vx, vy, angularVelocity], invMass [1/mass, 1/mass, 1/inertia].
//
//  - Contacts, one row per contact point, in the columns named by the Col*
//    constants below (physicsCore.ColPn and so on).  A manifold of two points
//    is two consecutive rows, with ColPair 1 on the first and 2 on the second;
//    a lone point has ColPair 0.
//
//  - Shapes, one row per shape, in the columns named by the Shape* constants
//    (physicsCore.ShapeKind and so on); and polygon vertices, one row per
//    vertex, in the Vert* columns.  A polygon's vertices are rows
//    ShapeVertStart through ShapeVertStart + ShapeVertCount - 1, in order
//    around the polygon; normal k belongs to the edge from vertex k to k+1.
//

#include "PhysicsCore.h"
#include "Matrix.h"
#include "miniscript.h"
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace MiniScript {

#define INTRINSIC_LAMBDA [](Context context, IntrinsicResult partialResult) -> IntrinsicResult

//--------------------------------------------------------------------------------
// Contact matrix layout
//--------------------------------------------------------------------------------

enum ContactColumn {
	kColBodyA = 0,      // row index (in pos/vel/invMass) of shape a's body
	kColBodyB,          // same, for shape b
	kColNx,             // unit normal, pointing from a to b
	kColNy,
	kColX,              // contact point, in world coordinates
	kColY,
	kColSep,            // separation along the normal (negative = overlap)
	kColFriction,       // mixed friction coefficient
	kColRestitution,    // mixed restitution
	kColPn,             // accumulated normal impulse (in: warm start; out: result)
	kColPt,             // accumulated tangent impulse (likewise)
	kColPair,           // 0 = lone point; 1 = first of two; 2 = second of two
	kColShapeA,         // id of shape a (Shape.id in physics.ms)
	kColShapeB,         // id of shape b
	kColFeature,        // which features touch, the same from step to step
	kContactColumns
};

//--------------------------------------------------------------------------------
// Shape and vertex matrix layouts
//--------------------------------------------------------------------------------

enum { kCircle = 0, kPolygon = 1 };

enum ShapeColumn {
	kShapeKind = 0,     // 0 = circle, 1 = convex polygon
	kShapeBody,         // row index (in pos/vel/invMass) of the shape's body
	kShapeId,           // unique id (Shape.id in physics.ms)
	kShapeFriction,
	kShapeRestitution,
	kShapeRadius,       // circles: radius
	kShapeLx,           // circles: center, relative to the body's center of mass
	kShapeLy,           //   (in the body's frame)
	kShapeVertStart,    // polygons: first vertex row
	kShapeVertCount,    // polygons: number of vertices
	kShapeCategory,     // collision filtering: two shapes collide only if each
	kShapeMask,         //   one's category has a bit in common with the other's mask
	kShapeCx,           // circles: center, in world coordinates (from updateGeometry)
	kShapeCy,
	kShapeMinX,         // world bounding box (from updateGeometry)
	kShapeMinY,
	kShapeMaxX,
	kShapeMaxY,
	kShapeColumns
};

enum VertColumn {
	kVertLx = 0,        // position, relative to the body's center of mass
	kVertLy,
	kVertNx,            // edge normal, in the body's frame
	kVertNy,
	kVertWx,            // position, in world coordinates (from updateGeometry)
	kVertWy,
	kVertWnx,           // edge normal, in world coordinates (from updateGeometry)
	kVertWny,
	kVertColumns
};

// Check that shapes and verts have the right columns, and that every
// polygon's vertex range is inside verts.
static bool CheckShapes(const MatrixData* shapes, const MatrixData* verts, const char* who, Value* outErr) {
	char buf[160];
	if (shapes->columns != kShapeColumns || verts->columns != kVertColumns) {
		snprintf(buf, sizeof(buf), "%s: shapes must have %d columns and verts %d",
			who, (int)kShapeColumns, (int)kVertColumns);
		*outErr = ErrorTypes::RuntimeError(buf);
		return false;
	}
	for (int r = 0; r < shapes->rows; r++) {
		const double* sh = shapes->data + (long)r * kShapeColumns;
		if (sh[kShapeKind] == kCircle) continue;
		double start = sh[kShapeVertStart], count = sh[kShapeVertCount];
		if (!(count >= 3 && start >= 0 && start + count <= verts->rows)) {
			snprintf(buf, sizeof(buf), "%s: shape row %d has a bad vertex range", who, r);
			*outErr = ErrorTypes::RuntimeError(buf);
			return false;
		}
	}
	return true;
}

// Check that every shape's body index is a row of a matrix with bodyCount rows.
static bool CheckShapeBodies(const MatrixData* shapes, int bodyCount, const char* who, Value* outErr) {
	for (int r = 0; r < shapes->rows; r++) {
		double b = shapes->data[(long)r * kShapeColumns + kShapeBody];
		if (!(b >= 0 && b < bodyCount)) {
			char buf[140];
			snprintf(buf, sizeof(buf), "%s: shape row %d has a body index out of range", who, r);
			*outErr = ErrorTypes::RuntimeError(buf);
			return false;
		}
	}
	return true;
}

//--------------------------------------------------------------------------------
// Geometry and broadphase
//--------------------------------------------------------------------------------

// Transform the shapes of the bodies whose row in bodyMask is nonzero (every
// body, if bodyMask is null) into world coordinates, from their bodies'
// centers of mass and angles in pos, and compute their bounding boxes.
static bool UpdateGeometry(MatrixData* shapes, MatrixData* verts, const MatrixData* pos,
		const MatrixData* bodyMask, Value* outErr) {
	const char* who = "physicsCore.updateGeometry";
	if (!CheckShapes(shapes, verts, who, outErr)) return false;
	if (pos->columns < 3) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.updateGeometry: pos must have 3 columns");
		return false;
	}
	if (bodyMask != nullptr && bodyMask->rows != pos->rows) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.updateGeometry: bodyMask must have a row per body");
		return false;
	}
	if (!CheckShapeBodies(shapes, pos->rows, who, outErr)) return false;

	const int ps = pos->columns;
	for (int r = 0; r < shapes->rows; r++) {
		double* sh = shapes->data + (long)r * kShapeColumns;
		int body = (int)sh[kShapeBody];
		if (bodyMask != nullptr && bodyMask->data[(long)body * bodyMask->columns] == 0) continue;
		const double* p = pos->data + (long)body * ps;
		double x = p[0], y = p[1];
		double c = cos(p[2]), s = sin(p[2]);
		if (sh[kShapeKind] == kCircle) {
			double lx = sh[kShapeLx], ly = sh[kShapeLy];
			double cx = x + c*lx - s*ly;
			double cy = y + s*lx + c*ly;
			sh[kShapeCx] = cx; sh[kShapeCy] = cy;
			double rad = sh[kShapeRadius];
			sh[kShapeMinX] = cx - rad; sh[kShapeMaxX] = cx + rad;
			sh[kShapeMinY] = cy - rad; sh[kShapeMaxY] = cy + rad;
		} else {
			double* v = verts->data + (long)sh[kShapeVertStart] * kVertColumns;
			int count = (int)sh[kShapeVertCount];
			double minX = 1e300, maxX = -1e300, minY = 1e300, maxY = -1e300;
			for (int k = 0; k < count; k++, v += kVertColumns) {
				double vx = x + c*v[kVertLx] - s*v[kVertLy];
				double vy = y + s*v[kVertLx] + c*v[kVertLy];
				v[kVertWx] = vx; v[kVertWy] = vy;
				if (vx < minX) minX = vx;
				if (vx > maxX) maxX = vx;
				if (vy < minY) minY = vy;
				if (vy > maxY) maxY = vy;
				v[kVertWnx] = c*v[kVertNx] - s*v[kVertNy];
				v[kVertWny] = s*v[kVertNx] + c*v[kVertNy];
			}
			sh[kShapeMinX] = minX; sh[kShapeMaxX] = maxX;
			sh[kShapeMinY] = minY; sh[kShapeMaxY] = maxY;
		}
	}
	*outErr = Value::Null;
	return true;
}

// The broadphase: find every pair of shapes whose bounding boxes overlap (or
// come within margin of it), on different bodies, at least one of them
// dynamic (nonzero inverse mass in invMass), and passing each other's
// category/mask filter.  Writes them into pairs as rows [shape row a, shape
// row b], with a's id less than b's, sorted by (id a, id b).  Returns the
// count, or -1 with *outErr.
static int FindPairs(const MatrixData* shapes, const MatrixData* invMass, double margin,
		Value pairsVal, MatrixData* pairs, Value* outErr) {
	const char* who = "physicsCore.findPairs";
	if (shapes->columns != kShapeColumns || pairs->columns != 2 || invMass->columns < 1) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.findPairs: shapes must be a shape matrix, and pairs have 2 columns");
		return -1;
	}
	if (!CheckShapeBodies(shapes, invMass->rows, who, outErr)) return -1;

	const int n = shapes->rows;
	const double* S = shapes->data;
	#define SH(r, col) S[(long)(r) * kShapeColumns + (col)]

	// Sweep along x: sort by left edge (then id, so the order is fully
	// determined), and for each shape look only at those starting before it
	// ends.
	static std::vector<int> order;
	order.resize(n);
	for (int i = 0; i < n; i++) order[i] = i;
	std::sort(order.begin(), order.end(), [S](int a, int b) {
		double ax = SH(a, kShapeMinX), bx = SH(b, kShapeMinX);
		if (ax != bx) return ax < bx;
		return SH(a, kShapeId) < SH(b, kShapeId);
	});

	struct Found { double key; int a, b; };
	static std::vector<Found> found;
	found.clear();
	const int ms = invMass->columns;
	for (int i = 0; i < n; i++) {
		int a = order[i];
		double aMaxX = SH(a, kShapeMaxX) + margin;
		double aMinY = SH(a, kShapeMinY) - margin, aMaxY = SH(a, kShapeMaxY) + margin;
		int bodyA = (int)SH(a, kShapeBody);
		bool aDynamic = invMass->data[(long)bodyA * ms] > 0;
		int64_t catA = (int64_t)SH(a, kShapeCategory), maskA = (int64_t)SH(a, kShapeMask);
		for (int j = i + 1; j < n; j++) {
			int b = order[j];
			if (SH(b, kShapeMinX) > aMaxX) break;
			if (SH(b, kShapeMinY) > aMaxY || SH(b, kShapeMaxY) < aMinY) continue;
			int bodyB = (int)SH(b, kShapeBody);
			if (bodyB == bodyA) continue;
			if (!aDynamic && !(invMass->data[(long)bodyB * ms] > 0)) continue;
			if (!(catA & (int64_t)SH(b, kShapeMask)) || !((int64_t)SH(b, kShapeCategory) & maskA)) continue;
			double idA = SH(a, kShapeId), idB = SH(b, kShapeId);
			if (idA < idB) found.push_back({ idA * 1048576 + idB, a, b });
			else found.push_back({ idB * 1048576 + idA, b, a });
		}
	}
	std::sort(found.begin(), found.end(), [](const Found& x, const Found& y) { return x.key < y.key; });
	#undef SH

	int count = (int)found.size();
	if (!MatrixSetRows(pairsVal, count)) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.findPairs: out of memory");
		return -1;
	}
	for (int i = 0; i < count; i++) {
		pairs->data[(long)i * 2] = found[i].a;
		pairs->data[(long)i * 2 + 1] = found[i].b;
	}
	*outErr = Value::Null;
	return count;
}

//--------------------------------------------------------------------------------
// Contact solver
//--------------------------------------------------------------------------------

// What the solver works out for each contact point before iterating.
struct PointScratch {
	double r1x, r1y, r2x, r2y;  // lever arms from each body's center of mass
	double rnA, rnB;            // cross(r1, n), cross(r2, n)
	double massN, massT;        // effective mass along the normal and tangent
	double bias;                // target separating speed
	// For the first point of a two-point manifold: the block solver's 2x2
	// matrix K and its inverse, if K is well conditioned.
	bool block;
	double k11, k12, k22, n11, n12, n22;
};

// Reused between calls, so a running simulation allocates nothing here.
static std::vector<PointScratch> scratch;

// Check that a state matrix has at least minCols columns and the given rows.
static bool CheckState(const MatrixData* m, const char* name, int rows, int minCols, Value* outErr) {
	if (m == nullptr) {
		*outErr = ErrorTypes::TypeError("Matrix", Value::Null);
		return false;
	}
	if (m->rows != rows || m->columns < minCols) {
		char buf[160];
		snprintf(buf, sizeof(buf),
			"physicsCore.solveContacts: %s must be %d x %d or wider (got %d x %d)",
			name, rows, minCols, m->rows, m->columns);
		*outErr = ErrorTypes::RuntimeError(buf);
		return false;
	}
	return true;
}

// The contact solver: solveContacts in assets/physicsFallback.ms, which
// explains each step.  Updates vel, and contacts' ColPn and
// ColPt, in place.  Returns false with *outErr on bad input, having changed
// nothing.
static bool SolveContacts(MatrixData* contacts, const MatrixData* pos, MatrixData* vel,
		const MatrixData* invMass, double dt, int iterations, double biasFactor,
		double slop, double restThreshold, Value* outErr) {
	if (contacts->columns < kContactColumns) {
		char buf[120];
		snprintf(buf, sizeof(buf),
			"physicsCore.solveContacts: contacts needs %d columns (got %d)",
			(int)kContactColumns, contacts->columns);
		*outErr = ErrorTypes::RuntimeError(buf);
		return false;
	}
	int bodyCount = pos->rows;
	if (!CheckState(vel, "vel", bodyCount, 3, outErr)) return false;
	if (!CheckState(invMass, "invMass", bodyCount, 3, outErr)) return false;
	if (!CheckState(pos, "pos", bodyCount, 2, outErr)) return false;
	if (!(dt > 0)) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.solveContacts: dt must be positive");
		return false;
	}

	const int n = contacts->rows;
	const int cs = contacts->columns;
	double* C = contacts->data;

	// Validate body indexes and pairing before touching anything.
	for (int i = 0; i < n; i++) {
		const double* row = C + (long)i * cs;
		double a = row[kColBodyA], b = row[kColBodyB];
		if (!(a >= 0 && a < bodyCount && b >= 0 && b < bodyCount)) {
			char buf[120];
			snprintf(buf, sizeof(buf),
				"physicsCore.solveContacts: contact %d has a body index out of range", i);
			*outErr = ErrorTypes::RuntimeError(buf);
			return false;
		}
		int pair = (int)row[kColPair];
		bool ok = (pair == 0) || (pair == 1 && i + 1 < n && (int)C[(long)(i + 1) * cs + kColPair] == 2)
			|| (pair == 2 && i > 0 && (int)C[(long)(i - 1) * cs + kColPair] == 1);
		if (!ok) {
			char buf[140];
			snprintf(buf, sizeof(buf),
				"physicsCore.solveContacts: contact %d has a bad ColPair (%d); two-point manifolds are rows marked 1 then 2", i, pair);
			*outErr = ErrorTypes::RuntimeError(buf);
			return false;
		}
	}

	const double invDt = 1 / dt;
	const int ps = pos->columns, vs = vel->columns, ms = invMass->columns;
	const double* P = pos->data;
	const double* M = invMass->data;
	double* V = vel->data;
	if ((int)scratch.size() < n) scratch.resize(n);

	// Accessors in the script's terms: px[a], vx[a], w[a], im[a], ii[a].
	#define PX(k) P[(long)(k) * ps]
	#define PY(k) P[(long)(k) * ps + 1]
	#define VX(k) V[(long)(k) * vs]
	#define VY(k) V[(long)(k) * vs + 1]
	#define W(k)  V[(long)(k) * vs + 2]
	#define IM(k) M[(long)(k) * ms]
	#define II(k) M[(long)(k) * ms + 2]

	// ---- setup ----
	for (int i = 0; i < n; ) {
		const double* row = C + (long)i * cs;
		int count = (row[kColPair] == 1) ? 2 : 1;
		int a = (int)row[kColBodyA], b = (int)row[kColBodyB];
		double nx = row[kColNx], ny = row[kColNy];
		double ima = IM(a), imb = IM(b), iia = II(a), iib = II(b);
		double mSum = ima + imb;
		double restitution = row[kColRestitution];
		for (int j = i; j < i + count; j++) {
			const double* c = C + (long)j * cs;
			PointScratch& s = scratch[j];
			double r1x = c[kColX] - PX(a), r1y = c[kColY] - PY(a);
			double r2x = c[kColX] - PX(b), r2y = c[kColY] - PY(b);
			s.r1x = r1x; s.r1y = r1y; s.r2x = r2x; s.r2y = r2y;
			double rnA = r1x*ny - r1y*nx, rnB = r2x*ny - r2y*nx;
			s.rnA = rnA; s.rnB = rnB;
			s.massN = 1 / (mSum + iia*rnA*rnA + iib*rnB*rnB);
			double rtA = r1x*nx + r1y*ny, rtB = r2x*nx + r2y*ny;
			s.massT = 1 / (mSum + iia*rtA*rtA + iib*rtB*rtB);
			double sep = c[kColSep];
			double bias;
			if (sep > 0) bias = -sep * invDt;
			else if (sep < -slop) bias = -biasFactor * invDt * (sep + slop);
			else bias = 0;
			if (restitution > 0) {
				double vn = (VX(b) - W(b)*r2y - VX(a) + W(a)*r1y) * nx +
				            (VY(b) + W(b)*r2x - VY(a) - W(a)*r1x) * ny;
				if (vn < -restThreshold && -restitution * vn > bias) bias = -restitution * vn;
			}
			s.bias = bias;
			s.block = false;
		}
		if (count == 2) {
			PointScratch& p1 = scratch[i];
			PointScratch& p2 = scratch[i + 1];
			double k11 = mSum + iia*p1.rnA*p1.rnA + iib*p1.rnB*p1.rnB;
			double k22 = mSum + iia*p2.rnA*p2.rnA + iib*p2.rnB*p2.rnB;
			double k12 = mSum + iia*p1.rnA*p2.rnA + iib*p1.rnB*p2.rnB;
			double det = k11*k22 - k12*k12;
			if (k11*k11 < 1000 * det) {
				p1.block = true;
				p1.k11 = k11; p1.k12 = k12; p1.k22 = k22;
				p1.n11 = k22 / det; p1.n12 = -k12 / det; p1.n22 = k11 / det;
			}
		}
		i += count;
	}

	// Warm start.
	for (int i = 0; i < n; i++) {
		const double* c = C + (long)i * cs;
		double Pn = c[kColPn], Pt = c[kColPt];
		if (Pn == 0 && Pt == 0) continue;
		int a = (int)c[kColBodyA], b = (int)c[kColBodyB];
		double nx = c[kColNx], ny = c[kColNy];
		const PointScratch& s = scratch[i];
		double Px = Pn*nx + Pt*ny, Py = Pn*ny - Pt*nx;
		double ima = IM(a), imb = IM(b);
		VX(a) -= ima*Px; VY(a) -= ima*Py;
		W(a) -= II(a) * (s.r1x*Py - s.r1y*Px);
		VX(b) += imb*Px; VY(b) += imb*Py;
		W(b) += II(b) * (s.r2x*Py - s.r2y*Px);
	}

	// ---- iterations ----
	for (int iter = 0; iter < iterations; iter++) {
		for (int i = 0; i < n; ) {
			double* row = C + (long)i * cs;
			int count = (row[kColPair] == 1) ? 2 : 1;
			int a = (int)row[kColBodyA], b = (int)row[kColBodyB];
			double nx = row[kColNx], ny = row[kColNy];
			double ima = IM(a), imb = IM(b), iia = II(a), iib = II(b);

			// friction, along the tangent (ny, -nx)
			double friction = row[kColFriction];
			for (int j = i; j < i + count; j++) {
				double* c = C + (long)j * cs;
				const PointScratch& s = scratch[j];
				double r1x = s.r1x, r1y = s.r1y, r2x = s.r2x, r2y = s.r2y;
				double dvx = VX(b) - W(b)*r2y - VX(a) + W(a)*r1y;
				double dvy = VY(b) + W(b)*r2x - VY(a) - W(a)*r1x;
				double dPt = -s.massT * (dvx*ny - dvy*nx);
				double maxPt = friction * c[kColPn];
				double Pt0 = c[kColPt];
				double Pt = Pt0 + dPt;
				if (Pt < -maxPt) Pt = -maxPt;
				else if (Pt > maxPt) Pt = maxPt;
				c[kColPt] = Pt;
				dPt = Pt - Pt0;
				double Px = dPt*ny, Py = -dPt*nx;
				VX(a) -= ima*Px; VY(a) -= ima*Py;
				W(a) -= iia * (r1x*Py - r1y*Px);
				VX(b) += imb*Px; VY(b) += imb*Py;
				W(b) += iib * (r2x*Py - r2y*Px);
			}

			const PointScratch& s1 = scratch[i];
			if (!s1.block) {
				// normal impulses, one point at a time
				for (int j = i; j < i + count; j++) {
					double* c = C + (long)j * cs;
					const PointScratch& s = scratch[j];
					double dvx = VX(b) - W(b)*s.r2y - VX(a) + W(a)*s.r1y;
					double dvy = VY(b) + W(b)*s.r2x - VY(a) - W(a)*s.r1x;
					double dPn = s.massN * (s.bias - dvx*nx - dvy*ny);
					double Pn0 = c[kColPn];
					double Pn = Pn0 + dPn;
					if (Pn < 0) Pn = 0;
					c[kColPn] = Pn;
					dPn = Pn - Pn0;
					VX(a) -= ima*dPn*nx; VY(a) -= ima*dPn*ny;
					W(a) -= iia * dPn * s.rnA;
					VX(b) += imb*dPn*nx; VY(b) += imb*dPn*ny;
					W(b) += iib * dPn * s.rnB;
				}
				i += count;
				continue;
			}

			// Both normal impulses at once (the block solver).
			double* c1 = row;
			double* c2 = row + cs;
			const PointScratch& s2 = scratch[i + 1];
			double a1 = c1[kColPn], a2 = c2[kColPn];
			double vn1 = (VX(b) - W(b)*s1.r2y - VX(a) + W(a)*s1.r1y) * nx +
			             (VY(b) + W(b)*s1.r2x - VY(a) - W(a)*s1.r1x) * ny;
			double vn2 = (VX(b) - W(b)*s2.r2y - VX(a) + W(a)*s2.r1y) * nx +
			             (VY(b) + W(b)*s2.r2x - VY(a) - W(a)*s2.r1x) * ny;
			double k11 = s1.k11, k12 = s1.k12, k22 = s1.k22;
			double b1 = vn1 - s1.bias - (k11*a1 + k12*a2);
			double b2 = vn2 - s2.bias - (k12*a1 + k22*a2);
			bool solved = true;
			double x1 = -(s1.n11*b1 + s1.n12*b2);
			double x2 = -(s1.n12*b1 + s1.n22*b2);
			if (x1 < 0 || x2 < 0) {
				x1 = -b1 / k11; x2 = 0;
				if (x1 < 0 || k12*x1 + b2 < 0) {
					x1 = 0; x2 = -b2 / k22;
					if (x2 < 0 || k12*x2 + b1 < 0) {
						x1 = 0; x2 = 0;
						if (b1 < 0 || b2 < 0) solved = false;   // no solution; leave as is
					}
				}
			}
			if (solved) {
				double d1 = x1 - a1, d2 = x2 - a2;
				c1[kColPn] = x1; c2[kColPn] = x2;
				double Px = (d1 + d2) * nx, Py = (d1 + d2) * ny;
				VX(a) -= ima*Px; VY(a) -= ima*Py;
				W(a) -= iia * (d1*s1.rnA + d2*s2.rnA);
				VX(b) += imb*Px; VY(b) += imb*Py;
				W(b) += iib * (d1*s1.rnB + d2*s2.rnB);
			}
			i += 2;
		}
	}

	#undef PX
	#undef PY
	#undef VX
	#undef VY
	#undef W
	#undef IM
	#undef II

	*outErr = Value::Null;
	return true;
}

//--------------------------------------------------------------------------------
// Narrowphase
//
// physicsFallback.ms's collide and the functions it calls, over plain
// structs.  Each shape's world geometry is copied out of the shape and vertex
// matrices once per call, into a ShapeGeom; polygon vertex and normal arrays
// go into one shared pool.
//--------------------------------------------------------------------------------

struct ShapeGeom {
	int kind;
	int body;               // body index (row in the state matrices)
	double id;
	double friction, restitution;
	double cx, cy, radius;  // circles
	int count;              // polygons: vertex count...
	long off;               // ...and where wx, wy, wnx, wny start in the pool
};

struct GeomPool {
	std::vector<ShapeGeom> shapes;
	std::vector<double> data;
	// A polygon's arrays, each `count` long, back to back.
	const double* WX(const ShapeGeom& g) const { return &data[g.off]; }
	const double* WY(const ShapeGeom& g) const { return &data[g.off + g.count]; }
	const double* WNX(const ShapeGeom& g) const { return &data[g.off + 2 * g.count]; }
	const double* WNY(const ShapeGeom& g) const { return &data[g.off + 3 * g.count]; }
};

// Copy shape row r's geometry into the pool.  Returns its index there.
static int ReadShapeRow(const MatrixData* shapes, const MatrixData* verts, int r, GeomPool& pool) {
	const double* sh = shapes->data + (long)r * kShapeColumns;
	ShapeGeom g = {};
	g.kind = (int)sh[kShapeKind];
	g.body = (int)sh[kShapeBody];
	g.id = sh[kShapeId];
	g.friction = sh[kShapeFriction];
	g.restitution = sh[kShapeRestitution];
	if (g.kind == kCircle) {
		g.cx = sh[kShapeCx];
		g.cy = sh[kShapeCy];
		g.radius = sh[kShapeRadius];
	} else {
		g.count = (int)sh[kShapeVertCount];
		g.off = (long)pool.data.size();
		const double* v0 = verts->data + (long)sh[kShapeVertStart] * kVertColumns;
		const int cols[4] = { kVertWx, kVertWy, kVertWnx, kVertWny };
		for (int col : cols) {
			const double* v = v0;
			for (int k = 0; k < g.count; k++, v += kVertColumns) pool.data.push_back(v[col]);
		}
	}
	pool.shapes.push_back(g);
	return (int)pool.shapes.size() - 1;
}

struct ContactPoint { double x, y, sep, id; };
struct Manifold {
	double nx, ny;
	int count;
	ContactPoint points[2];
};

static bool CollideCircles(const ShapeGeom& a, const ShapeGeom& b, double margin, Manifold* m) {
	double dx = b.cx - a.cx, dy = b.cy - a.cy;
	double r = a.radius + b.radius;
	double d2 = dx*dx + dy*dy;
	if (d2 > (r + margin)*(r + margin)) return false;
	double d = sqrt(d2);
	double nx, ny;
	if (d > 1e-9) { nx = dx / d; ny = dy / d; }
	else { nx = 0; ny = 1; }
	double k = (a.radius - b.radius) * 0.5;
	m->nx = nx; m->ny = ny; m->count = 1;
	m->points[0] = { (a.cx + b.cx) * 0.5 + nx * k, (a.cy + b.cy) * 0.5 + ny * k, d - r, 0 };
	return true;
}

// Polygon p against circle c; the normal points from p to c, or from c to p
// if flip is true.
static bool CollidePolygonCircle(const GeomPool& pool, const ShapeGeom& p, const ShapeGeom& c,
		bool flip, double margin, Manifold* m) {
	double cx = c.cx, cy = c.cy, r = c.radius;
	double rm = r + margin;
	const double* wx = pool.WX(p); const double* wy = pool.WY(p);
	const double* wnx = pool.WNX(p); const double* wny = pool.WNY(p);
	double best = -1e30; int bi = 0;
	for (int i = 0; i < p.count; i++) {
		double s = wnx[i] * (cx - wx[i]) + wny[i] * (cy - wy[i]);
		if (s > rm) return false;
		if (s > best) { best = s; bi = i; }
	}
	int bi2 = bi + 1;
	if (bi2 == p.count) bi2 = 0;
	double v1x = wx[bi], v1y = wy[bi], v2x = wx[bi2], v2y = wy[bi2];
	int id = bi;
	double nx, ny, px, py, sep;
	if (best > 1e-9) {
		double u1 = (cx - v1x) * (v2x - v1x) + (cy - v1y) * (v2y - v1y);
		double u2 = (cx - v2x) * (v1x - v2x) + (cy - v2y) * (v1y - v2y);
		if (u1 <= 0 || u2 <= 0) {
			if (u1 > 0) { v1x = v2x; v1y = v2y; id = bi2; }
			double dx = cx - v1x, dy = cy - v1y;
			double d2 = dx*dx + dy*dy;
			if (d2 > rm*rm) return false;
			double d = sqrt(d2);
			nx = dx / d; ny = dy / d;
			px = (v1x + cx - nx * r) * 0.5;
			py = (v1y + cy - ny * r) * 0.5;
			if (flip) { nx = -nx; ny = -ny; }
			m->nx = nx; m->ny = ny; m->count = 1;
			m->points[0] = { px, py, d - r, (double)(100 + id) };
			return true;
		}
	}
	nx = wnx[bi]; ny = wny[bi];
	px = cx - nx * (best + r) * 0.5;
	py = cy - ny * (best + r) * 0.5;
	sep = best - r;
	if (flip) { nx = -nx; ny = -ny; }
	m->nx = nx; m->ny = ny; m->count = 1;
	m->points[0] = { px, py, sep, (double)id };
	return true;
}

// The edge of p1 along whose normal p2 is most separated; stops early once
// that is over margin.
static void FindMaxSeparation(const GeomPool& pool, const ShapeGeom& p1, const ShapeGeom& p2,
		double margin, double* outSep, int* outEdge) {
	const double* wx1 = pool.WX(p1); const double* wy1 = pool.WY(p1);
	const double* wnx = pool.WNX(p1); const double* wny = pool.WNY(p1);
	const double* wx2 = pool.WX(p2); const double* wy2 = pool.WY(p2);
	double best = -1e30; int bestEdge = 0;
	for (int i = 0; i < p1.count; i++) {
		double nx = wnx[i], ny = wny[i];
		double ox = wx1[i], oy = wy1[i];
		double si = 1e30;
		for (int j = 0; j < p2.count; j++) {
			double d = nx * (wx2[j] - ox) + ny * (wy2[j] - oy);
			if (d < si) si = d;
		}
		if (si > best) {
			best = si; bestEdge = i;
			if (best > margin) break;
		}
	}
	*outSep = best; *outEdge = bestEdge;
}

// A segment being clipped: up to two points, each with a feature id.
struct Seg { int n; double x[2], y[2], id[2]; };

// Keep the part of seg on the inner side of {p : nx*px + ny*py = offset}; a
// point made by the cut gets clipId.  False if fewer than two points remain.
static bool ClipSegment(const Seg& in, double nx, double ny, double offset, double clipId, Seg* out) {
	double x0 = in.x[0], y0 = in.y[0], x1 = in.x[1], y1 = in.y[1];
	double d0 = nx * x0 + ny * y0 - offset;
	double d1 = nx * x1 + ny * y1 - offset;
	Seg s; s.n = 0;
	if (d0 <= 0) { s.x[s.n] = x0; s.y[s.n] = y0; s.id[s.n] = in.id[0]; s.n++; }
	if (d1 <= 0) { s.x[s.n] = x1; s.y[s.n] = y1; s.id[s.n] = in.id[1]; s.n++; }
	if (d0 * d1 < 0 && s.n < 2) {
		double t = d0 / (d0 - d1);
		s.x[s.n] = x0 + t * (x1 - x0); s.y[s.n] = y0 + t * (y1 - y0); s.id[s.n] = clipId; s.n++;
	}
	if (s.n < 2) return false;
	*out = s;
	return true;
}

static bool CollidePolygons(const GeomPool& pool, const ShapeGeom& a, const ShapeGeom& b,
		double margin, Manifold* m) {
	double sepA, sepB; int edgeA, edgeB;
	FindMaxSeparation(pool, a, b, margin, &sepA, &edgeA);
	if (sepA > margin) return false;
	FindMaxSeparation(pool, b, a, margin, &sepB, &edgeB);
	if (sepB > margin) return false;
	const ShapeGeom* ref; const ShapeGeom* inc; int e1; bool flip;
	if (sepB > sepA + 0.05 * margin) { ref = &b; inc = &a; e1 = edgeB; flip = true; }
	else { ref = &a; inc = &b; e1 = edgeA; flip = false; }
	int e2 = e1 + 1;
	if (e2 == ref->count) e2 = 0;
	const double* rwx = pool.WX(*ref); const double* rwy = pool.WY(*ref);
	double v1x = rwx[e1], v1y = rwy[e1];
	double v2x = rwx[e2], v2y = rwy[e2];
	double nx = pool.WNX(*ref)[e1], ny = pool.WNY(*ref)[e1];
	double tx = -ny, ty = nx;

	const double* iwnx = pool.WNX(*inc); const double* iwny = pool.WNY(*inc);
	double minDot = 1e30; int i1 = 0;
	for (int i = 0; i < inc->count; i++) {
		double d = nx * iwnx[i] + ny * iwny[i];
		if (d < minDot) { minDot = d; i1 = i; }
	}
	int i2 = i1 + 1;
	if (i2 == inc->count) i2 = 0;
	const double* iwx = pool.WX(*inc); const double* iwy = pool.WY(*inc);
	Seg seg; seg.n = 2;
	seg.x[0] = iwx[i1]; seg.y[0] = iwy[i1]; seg.id[0] = i1;
	seg.x[1] = iwx[i2]; seg.y[1] = iwy[i2]; seg.id[1] = i2;

	if (!ClipSegment(seg, -tx, -ty, -(tx * v1x + ty * v1y), 500 + i1, &seg)) return false;
	if (!ClipSegment(seg, tx, ty, tx * v2x + ty * v2y, 600 + i1, &seg)) return false;

	double front = nx * v1x + ny * v1y;
	double idBase = (flip ? 1 : 0) * 1000000 + e1 * 1000;
	m->count = 0;
	for (int k = 0; k < 2; k++) {
		double px = seg.x[k], py = seg.y[k];
		double sep = nx * px + ny * py - front;
		if (sep <= margin) {
			m->points[m->count++] = { px - 0.5 * sep * nx, py - 0.5 * sep * ny, sep, idBase + seg.id[k] };
		}
	}
	if (m->count == 0) return false;
	if (flip) { nx = -nx; ny = -ny; }
	m->nx = nx; m->ny = ny;
	return true;
}

static bool Collide(const GeomPool& pool, const ShapeGeom& a, const ShapeGeom& b, double margin, Manifold* m) {
	if (a.kind == kCircle) {
		if (b.kind == kCircle) return CollideCircles(a, b, margin, m);
		return CollidePolygonCircle(pool, b, a, true, margin, m);
	}
	if (b.kind == kCircle) return CollidePolygonCircle(pool, a, b, false, margin, m);
	return CollidePolygons(pool, a, b, margin, m);
}

// Warm-start lookup key: a contact point is the same one as last step's if
// it is between the same two shapes and the same features.
struct ContactKey {
	double a, b, feature;
	bool operator==(const ContactKey& o) const { return a == o.a && b == o.b && feature == o.feature; }
};
struct ContactKeyHash {
	size_t operator()(const ContactKey& k) const {
		uint64_t h = 1469598103934665603ULL;
		const double parts[3] = { k.a, k.b, k.feature };
		for (double d : parts) {
			uint64_t bits; memcpy(&bits, &d, sizeof bits);
			h = (h ^ bits) * 1099511628211ULL;
		}
		return (size_t)(h ^ (h >> 29));
	}
};

// Collide each pair of shapes, and write every contact point into contacts,
// one row per point, replacing what was there.  Points that match one in
// prevContacts (if not null) start with its impulses; the rest start at 0.
// Returns the number of pairs that touch, or -1 with *outErr.
static int CollidePairs(const MatrixData* pairs, const MatrixData* shapes, const MatrixData* verts,
		double margin, const MatrixData* prev, Value contactsVal, MatrixData* contacts, Value* outErr) {
	const char* who = "physicsCore.collidePairs";
	if (!CheckShapes(shapes, verts, who, outErr)) return -1;
	if (pairs->columns != 2) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.collidePairs: pairs must have 2 columns");
		return -1;
	}
	if (contacts->columns != kContactColumns || (prev != nullptr && prev->columns != kContactColumns)) {
		char buf[120];
		snprintf(buf, sizeof(buf),
			"physicsCore.collidePairs: contact matrices must have %d columns", (int)kContactColumns);
		*outErr = ErrorTypes::RuntimeError(buf);
		return -1;
	}
	if (prev == contacts) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.collidePairs: prevContacts and contacts must be different matrices");
		return -1;
	}

	// Read each shape's geometry once.  Scratch is reused between calls.
	static GeomPool pool;
	static std::vector<int> poolIndex;      // shape row -> index in pool, or -1
	static std::vector<int> pairShapes;
	static std::vector<double> rows;
	static std::unordered_map<ContactKey, std::pair<double, double>, ContactKeyHash> warm;
	pool.shapes.clear(); pool.data.clear(); rows.clear(); warm.clear();
	poolIndex.assign(shapes->rows, -1);

	int pairCount = pairs->rows;
	pairShapes.resize((size_t)pairCount * 2);
	for (int i = 0; i < pairCount; i++) {
		for (int j = 0; j < 2; j++) {
			double r = pairs->data[(long)i * 2 + j];
			if (!(r >= 0 && r < shapes->rows)) {
				char buf[120];
				snprintf(buf, sizeof(buf), "physicsCore.collidePairs: pair %d has a shape row out of range", i);
				*outErr = ErrorTypes::RuntimeError(buf);
				return -1;
			}
			int row = (int)r;
			if (poolIndex[row] < 0) poolIndex[row] = ReadShapeRow(shapes, verts, row, pool);
			pairShapes[(size_t)i * 2 + j] = poolIndex[row];
		}
	}

	if (prev != nullptr) {
		for (int r = 0; r < prev->rows; r++) {
			const double* row = prev->data + (long)r * kContactColumns;
			warm.emplace(ContactKey{ row[kColShapeA], row[kColShapeB], row[kColFeature] },
				std::make_pair(row[kColPn], row[kColPt]));
		}
	}

	int touching = 0;
	double row[kContactColumns];
	for (int i = 0; i < pairCount; i++) {
		const ShapeGeom& a = pool.shapes[pairShapes[(size_t)i * 2]];
		const ShapeGeom& b = pool.shapes[pairShapes[(size_t)i * 2 + 1]];
		Manifold m;
		if (!Collide(pool, a, b, margin, &m)) continue;
		touching++;
		double friction = sqrt(a.friction * b.friction);
		double restitution = a.restitution;
		if (b.restitution > restitution) restitution = b.restitution;
		for (int k = 0; k < m.count; k++) {
			const ContactPoint& c = m.points[k];
			row[kColBodyA] = a.body;
			row[kColBodyB] = b.body;
			row[kColNx] = m.nx;
			row[kColNy] = m.ny;
			row[kColX] = c.x;
			row[kColY] = c.y;
			row[kColSep] = c.sep;
			row[kColFriction] = friction;
			row[kColRestitution] = restitution;
			row[kColPn] = 0;
			row[kColPt] = 0;
			row[kColPair] = (m.count == 2) ? k + 1 : 0;
			row[kColShapeA] = a.id;
			row[kColShapeB] = b.id;
			row[kColFeature] = c.id;
			if (prev != nullptr) {
				auto w = warm.find(ContactKey{ a.id, b.id, c.id });
				if (w != warm.end()) {
					row[kColPn] = w->second.first;
					row[kColPt] = w->second.second;
				}
			}
			rows.insert(rows.end(), row, row + kContactColumns);
		}
	}

	int n = (int)(rows.size() / kContactColumns);
	if (!MatrixSetRows(contactsVal, n)) {
		*outErr = ErrorTypes::RuntimeError("physicsCore.collidePairs: out of memory");
		return -1;
	}
	if (n > 0) memcpy(contacts->data, rows.data(), rows.size() * sizeof(double));
	*outErr = Value::Null;
	return touching;
}

//--------------------------------------------------------------------------------
// Intrinsics
//--------------------------------------------------------------------------------

static void AddPhysicsCoreMethods(ValueDict& physicsModule) {
	Intrinsic f;

	// Column numbers of the contact matrix
	physicsModule.SetValue(String("ColBodyA"), Value((double)kColBodyA));
	physicsModule.SetValue(String("ColBodyB"), Value((double)kColBodyB));
	physicsModule.SetValue(String("ColNx"), Value((double)kColNx));
	physicsModule.SetValue(String("ColNy"), Value((double)kColNy));
	physicsModule.SetValue(String("ColX"), Value((double)kColX));
	physicsModule.SetValue(String("ColY"), Value((double)kColY));
	physicsModule.SetValue(String("ColSep"), Value((double)kColSep));
	physicsModule.SetValue(String("ColFriction"), Value((double)kColFriction));
	physicsModule.SetValue(String("ColRestitution"), Value((double)kColRestitution));
	physicsModule.SetValue(String("ColPn"), Value((double)kColPn));
	physicsModule.SetValue(String("ColPt"), Value((double)kColPt));
	physicsModule.SetValue(String("ColPair"), Value((double)kColPair));
	physicsModule.SetValue(String("ColShapeA"), Value((double)kColShapeA));
	physicsModule.SetValue(String("ColShapeB"), Value((double)kColShapeB));
	physicsModule.SetValue(String("ColFeature"), Value((double)kColFeature));
	physicsModule.SetValue(String("ContactColumns"), Value((double)kContactColumns));

	// Column numbers of the shape matrix
	physicsModule.SetValue(String("ShapeKind"), Value((double)kShapeKind));
	physicsModule.SetValue(String("ShapeBody"), Value((double)kShapeBody));
	physicsModule.SetValue(String("ShapeId"), Value((double)kShapeId));
	physicsModule.SetValue(String("ShapeFriction"), Value((double)kShapeFriction));
	physicsModule.SetValue(String("ShapeRestitution"), Value((double)kShapeRestitution));
	physicsModule.SetValue(String("ShapeRadius"), Value((double)kShapeRadius));
	physicsModule.SetValue(String("ShapeLx"), Value((double)kShapeLx));
	physicsModule.SetValue(String("ShapeLy"), Value((double)kShapeLy));
	physicsModule.SetValue(String("ShapeVertStart"), Value((double)kShapeVertStart));
	physicsModule.SetValue(String("ShapeVertCount"), Value((double)kShapeVertCount));
	physicsModule.SetValue(String("ShapeCategory"), Value((double)kShapeCategory));
	physicsModule.SetValue(String("ShapeMask"), Value((double)kShapeMask));
	physicsModule.SetValue(String("ShapeCx"), Value((double)kShapeCx));
	physicsModule.SetValue(String("ShapeCy"), Value((double)kShapeCy));
	physicsModule.SetValue(String("ShapeMinX"), Value((double)kShapeMinX));
	physicsModule.SetValue(String("ShapeMinY"), Value((double)kShapeMinY));
	physicsModule.SetValue(String("ShapeMaxX"), Value((double)kShapeMaxX));
	physicsModule.SetValue(String("ShapeMaxY"), Value((double)kShapeMaxY));
	physicsModule.SetValue(String("ShapeColumns"), Value((double)kShapeColumns));

	// Column numbers of the vertex matrix
	physicsModule.SetValue(String("VertLx"), Value((double)kVertLx));
	physicsModule.SetValue(String("VertLy"), Value((double)kVertLy));
	physicsModule.SetValue(String("VertNx"), Value((double)kVertNx));
	physicsModule.SetValue(String("VertNy"), Value((double)kVertNy));
	physicsModule.SetValue(String("VertWx"), Value((double)kVertWx));
	physicsModule.SetValue(String("VertWy"), Value((double)kVertWy));
	physicsModule.SetValue(String("VertWnx"), Value((double)kVertWnx));
	physicsModule.SetValue(String("VertWny"), Value((double)kVertWny));
	physicsModule.SetValue(String("VertColumns"), Value((double)kVertColumns));

	// physicsCore.updateGeometry shapes, verts, pos, bodyMask
	//
	// Transform shapes into world coordinates (ShapeCx/Cy for circles, and the
	// VertW* columns for polygons) and compute their bounding boxes, for the
	// bodies whose row of bodyMask (one column, a row per body) is nonzero --
	// or for every body, if bodyMask is null.
	//
	// Transform shapes into world coordinates and compute their bounds, for the bodies in bodyMask
	f = Intrinsic::Create("");
	f.AddParam("shapes");
	f.AddParam("verts");
	f.AddParam("pos");
	f.AddParam("bodyMask");
	f.set_Code(INTRINSIC_LAMBDA {
		MatrixData* shapes = ValueToMatrix(context.GetVar("shapes"));
		MatrixData* verts = ValueToMatrix(context.GetVar("verts"));
		MatrixData* pos = ValueToMatrix(context.GetVar("pos"));
		Value vMask = context.GetVar("bodyMask");
		MatrixData* mask = vMask.IsNull() ? nullptr : ValueToMatrix(vMask);
		if (shapes == nullptr || verts == nullptr || pos == nullptr || (!vMask.IsNull() && mask == nullptr)) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"physicsCore.updateGeometry: shapes, verts and pos must be Matrix values (and bodyMask a Matrix or null)"));
		}
		Value err;
		if (!UpdateGeometry(shapes, verts, pos, mask, &err)) return IntrinsicResult(err);
		return IntrinsicResult::Null;
	});
	physicsModule.SetValue(String("updateGeometry"), f.GetFunc());

	// physicsCore.findPairs shapes, invMass, margin, pairs -> count
	//
	// The broadphase: fills pairs (2 columns) with every pair of shape rows
	// whose bounding boxes come within margin of each other, that are on
	// different bodies, at least one dynamic (by invMass), and pass each
	// other's category/mask filter.  Each row is [a, b] with a's id less than
	// b's, and rows are sorted by those ids.
	//
	// Find the pairs of shapes whose bounding boxes overlap and that may collide
	f = Intrinsic::Create("");
	f.AddParam("shapes");
	f.AddParam("invMass");
	f.AddParam("margin", Value(0));
	f.AddParam("pairs");
	f.set_Code(INTRINSIC_LAMBDA {
		MatrixData* shapes = ValueToMatrix(context.GetVar("shapes"));
		MatrixData* invMass = ValueToMatrix(context.GetVar("invMass"));
		Value vPairs = context.GetVar("pairs");
		MatrixData* pairs = ValueToMatrix(vPairs);
		if (shapes == nullptr || invMass == nullptr || pairs == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"physicsCore.findPairs: shapes, invMass and pairs must be Matrix values"));
		}
		Value err;
		int count = FindPairs(shapes, invMass, context.GetVar("margin").DoubleValue(), vPairs, pairs, &err);
		if (count < 0) return IntrinsicResult(err);
		return IntrinsicResult(Value(count));
	});
	physicsModule.SetValue(String("findPairs"), f.GetFunc());

	// physicsCore.solveContacts contacts, pos, vel, invMass, dt, iterations,
	//   biasFactor, slop, restThreshold
	//
	// The whole contact solver for one step: works out each contact's lever
	// arms, effective masses and target speed, applies the impulses already
	// in ColPn/ColPt (warm starting), then runs `iterations` passes of
	// sequential impulses.  Updates vel and contacts' ColPn/ColPt in place.
	//
	// Solve contact impulses for one step, updating vel and the contacts' ColPn/ColPt in place
	f = Intrinsic::Create("");
	f.AddParam("contacts");
	f.AddParam("pos");
	f.AddParam("vel");
	f.AddParam("invMass");
	f.AddParam("dt");
	f.AddParam("iterations", Value(10));
	f.AddParam("biasFactor", Value(0.2));
	f.AddParam("slop", Value(0.5));
	f.AddParam("restThreshold", Value(30));
	f.set_Code(INTRINSIC_LAMBDA {
		MatrixData* contacts = ValueToMatrix(context.GetVar("contacts"));
		MatrixData* pos = ValueToMatrix(context.GetVar("pos"));
		MatrixData* vel = ValueToMatrix(context.GetVar("vel"));
		MatrixData* invMass = ValueToMatrix(context.GetVar("invMass"));
		if (contacts == nullptr || pos == nullptr || vel == nullptr || invMass == nullptr) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"physicsCore.solveContacts: contacts, pos, vel and invMass must be Matrix values"));
		}
		Value err;
		if (!SolveContacts(contacts, pos, vel, invMass,
				context.GetVar("dt").DoubleValue(),
				context.GetVar("iterations").IntValue(),
				context.GetVar("biasFactor").DoubleValue(),
				context.GetVar("slop").DoubleValue(),
				context.GetVar("restThreshold").DoubleValue(), &err)) {
			return IntrinsicResult(err);
		}
		return IntrinsicResult::Null;
	});
	physicsModule.SetValue(String("solveContacts"), f.GetFunc());

	// physicsCore.collidePairs pairs, shapes, verts, margin, prevContacts, contacts -> touching
	//
	// The narrowphase for one step: collides each pair of shape rows in pairs
	// (from findPairs; their world geometry must be current), and fills
	// contacts with one row per contact point.  Points that match one in
	// prevContacts (same shapes, same features) start with its ColPn/ColPt,
	// for warm starting; pass null for prevContacts to start them all at 0.
	// Returns how many pairs are touching (or within margin).
	//
	// Collide shape pairs into a contact matrix (one row per point), warm-started from prevContacts
	f = Intrinsic::Create("");
	f.AddParam("pairs");
	f.AddParam("shapes");
	f.AddParam("verts");
	f.AddParam("margin", Value(0));
	f.AddParam("prevContacts");
	f.AddParam("contacts");
	f.set_Code(INTRINSIC_LAMBDA {
		MatrixData* pairs = ValueToMatrix(context.GetVar("pairs"));
		MatrixData* shapes = ValueToMatrix(context.GetVar("shapes"));
		MatrixData* verts = ValueToMatrix(context.GetVar("verts"));
		Value vPrev = context.GetVar("prevContacts");
		Value vContacts = context.GetVar("contacts");
		MatrixData* contacts = ValueToMatrix(vContacts);
		MatrixData* prev = vPrev.IsNull() ? nullptr : ValueToMatrix(vPrev);
		if (pairs == nullptr || shapes == nullptr || verts == nullptr || contacts == nullptr
				|| (!vPrev.IsNull() && prev == nullptr)) {
			return IntrinsicResult(ErrorTypes::RuntimeError(
				"physicsCore.collidePairs: pairs, shapes, verts and contacts must be Matrix values (and prevContacts a Matrix or null)"));
		}
		Value err;
		int touching = CollidePairs(pairs, shapes, verts, context.GetVar("margin").DoubleValue(),
			prev, vContacts, contacts, &err);
		if (touching < 0) return IntrinsicResult(err);
		return IntrinsicResult(Value(touching));
	});
	physicsModule.SetValue(String("collidePairs"), f.GetFunc());
}

} // namespace MiniScript

using namespace MiniScript;

void AddPhysicsCoreIntrinsics() {
	// Built on first use, then wrapped and GC-rooted in the same breath (see
	// the raylib module in RaylibIntrinsics.cpp for why).
	Intrinsic f = Intrinsic::Create("physicsCore");
	f.set_Code(INTRINSIC_LAMBDA {
		static ValueDict physicsModule;
		static Value physicsModuleValue;
		if (physicsModuleValue.IsNull()) {
			AddPhysicsCoreMethods(physicsModule);
			physicsModuleValue = GCManager::NewMapFromDict(physicsModule);
			GCManager::AddRoot(physicsModuleValue);
		}
		return IntrinsicResult(physicsModuleValue);
	});
}
