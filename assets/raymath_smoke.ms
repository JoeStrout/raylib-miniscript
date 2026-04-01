// raymath_smoke.ms
// Quick smoke checks for initial raymath intrinsic tranche

if not raylib.IsWindowReady then raylib.InitWindow 

approx = function(a, b, tol = 0.0001)
	return abs(a - b) <= tol
end function

fail = function(msg)
	print "FAIL: " + msg
	// Trigger a runtime error so CI/manual runs fail fast.
	x = null[0]
end function

assert = function(ok, msg)
	if not ok then fail(msg)
end function

print "raymath smoke: begin"

// Utility functions
assert approx(raylib.Clamp(2.5, 0, 2), 2), "Clamp"
assert approx(raylib.Lerp(0, 10, 0.25), 2.5), "Lerp"
assert approx(raylib.Normalize(5, 0, 10), 0.5), "Normalize"
assert approx(raylib.Remap(
	5,
	0,
	10,
	0,
	100), 50), "Remap"
assert approx(raylib.Wrap(11, 0, 10), 1), "Wrap"
assert raylib.FloatEquals(1, 1), "FloatEquals"

// Vector2 family
v2 = raylib.Vector2Add([
	1,
	2,
], [
	3,
	4,
])
assert approx(v2.x, 4) and approx(v2.y, 6), "Vector2Add"
assert approx(raylib.Vector2Length([
	3,
	4,
]), 5), "Vector2Length"
n2 = raylib.Vector2Normalize([
	3,
	4,
])
assert approx(raylib.Vector2Length(n2), 1), "Vector2Normalize"
c2 = raylib.Vector2Clamp([
	3,
	-2,
], [
	-1,
	-1,
], [
	1,
	1,
])
assert approx(c2.x, 1) and approx(c2.y, -1), "Vector2Clamp"

// Vector3 family
v3 = raylib.Vector3CrossProduct([
	1,
	0,
	0,
], [
	0,
	1,
	0,
])
assert approx(v3.x, 0) and approx(v3.y, 0) and approx(v3.z, 1), "Vector3CrossProduct"
assert approx(raylib.Vector3Length([
	0,
	0,
	2,
]), 2), "Vector3Length"
r3 = raylib.Vector3RotateByQuaternion([
	1,
	2,
	3,
], {
	"x": 0,
	"y": 0,
	"z": 0,
	"w": 1,
})
assert approx(r3.x, 1) and approx(r3.y, 2) and approx(r3.z, 3), "Vector3RotateByQuaternion identity"
ortho = raylib.Vector3OrthoNormalize([
	1,
	0,
	0,
], [
	1,
	1,
	0,
])
assert ortho.hasIndex("v1") and ortho.hasIndex("v2"), "Vector3OrthoNormalize shape"
f3 = raylib.Vector3ToFloatV([
	9,
	8,
	7,
])
assert f3.len == 3 and approx(f3[0], 9) and approx(f3[1], 8) and approx(f3[2], 7), "Vector3ToFloatV"

// Vector4 family
v4 = raylib.Vector4Add([
	1,
	2,
	3,
	4,
], [
	5,
	6,
	7,
	8,
])
assert approx(v4.x, 6) and approx(v4.y, 8) and approx(v4.z, 10) and approx(v4.w, 12), "Vector4Add"
assert approx(raylib.Vector4Length([
	0,
	0,
	0,
	2,
]), 2), "Vector4Length"
n4 = raylib.Vector4Normalize([
	0,
	0,
	0,
	2,
])
assert approx(n4.w, 1), "Vector4Normalize"
mv4 = raylib.Vector4MoveTowards([
	0,
	0,
	0,
	0,
], [
	10,
	0,
	0,
	0,
], 2)
assert approx(mv4.x, 2), "Vector4MoveTowards"
iv4 = raylib.Vector4Invert([
	1,
	2,
	4,
	8,
])
assert approx(iv4.x, 1) and approx(iv4.y, 0.5) and approx(iv4.z, 0.25) and approx(iv4.w, 0.125), "Vector4Invert"
assert raylib.Vector4Equals([
	1,
	2,
	3,
	4,
], [
	1,
	2,
	3,
	4,
]), "Vector4Equals"

// Matrix family
mi = raylib.MatrixIdentity
assert approx(raylib.MatrixDeterminant(mi), 1), "MatrixDeterminant"
assert approx(raylib.MatrixTrace(mi), 4), "MatrixTrace"

mt = raylib.MatrixTranslate(1, 2, 3)
ms = raylib.MatrixScale(2, 3, 4)
mm = raylib.MatrixMultiply(mt, ms)
assert mm.hasIndex("rows") and mm.hasIndex("columns") and mm.hasIndex("elem"), "MatrixMultiply shape"
assert mm.rows == 4 and mm.columns == 4 and mm.elem.len == 4 and mm.elem[0].len == 4, "MatrixMultiply dimensions"

manualIdentity = {
	"rows": 4,
	"columns": 4,
	"elem": [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]],
}
assert approx(raylib.MatrixDeterminant(manualIdentity), 1), "MatrixDeterminant nested matrix"

mf = raylib.MatrixToFloatV(mi)
assert mf.len == 16 and approx(mf[0], 1) and approx(mf[5], 1) and approx(mf[10], 1) and approx(mf[15], 1), "MatrixToFloatV"

mc = raylib.MatrixCompose([
	1,
	2,
	3,
], {
	"x": 0,
	"y": 0,
	"z": 0,
	"w": 1,
}, [
	2,
	3,
	4,
])
md = raylib.MatrixDecompose(mc)
assert approx(md.translation.x, 1) and approx(md.translation.y, 2) and approx(md.translation.z, 3), "MatrixDecompose translation"
assert approx(md.scale.x, 2) and approx(md.scale.y, 3) and approx(md.scale.z, 4), "MatrixDecompose scale"

// Quaternion family
qi = raylib.QuaternionIdentity
assert approx(qi.x, 0) and approx(qi.y, 0) and approx(qi.z, 0) and approx(qi.w, 1), "QuaternionIdentity"
assert approx(raylib.QuaternionLength(qi), 1), "QuaternionLength"

qsum = raylib.QuaternionAdd(qi, qi)
assert approx(qsum.w, 2), "QuaternionAdd"
qn = raylib.QuaternionNormalize({
	"x": 0,
	"y": 0,
	"z": 0,
	"w": 2,
})
assert approx(qn.w, 1), "QuaternionNormalize"

qa = raylib.QuaternionFromAxisAngle([
	0,
	1,
	0,
], 0.5)
aa = raylib.QuaternionToAxisAngle(qa)
assert aa.hasIndex("axis") and aa.hasIndex("angle"), "QuaternionToAxisAngle shape"

qm = raylib.QuaternionToMatrix(qi)
assert approx(qm.elem[0][0], 1) and approx(qm.elem[1][1], 1) and approx(qm.elem[2][2], 1), "QuaternionToMatrix"
qfm = raylib.QuaternionFromMatrix(raylib.MatrixIdentity)
assert approx(qfm.w, 1), "QuaternionFromMatrix"

qe = raylib.QuaternionFromEuler(0.1, 0.2, 0.3)
e = raylib.QuaternionToEuler(qe)
assert e.hasIndex("x") and e.hasIndex("y") and e.hasIndex("z"), "QuaternionToEuler shape"
assert raylib.QuaternionEquals(qi, raylib.QuaternionIdentity), "QuaternionEquals"

print "raymath smoke: PASS"
