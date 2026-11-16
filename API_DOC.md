
## RMath

|Name | Parameters | Purpose |
|-----|------------|---------|
|Clamp |**value**=0, **min**=0, **max**=1 |Clamp value to the range [min, max] |
|Lerp |**start**=0, **end**=1, **amount**=0 |Linearly interpolate between start and end; amount 0 gives start, 1 gives end |
|Normalize |**value**=0, **start**=0, **end**=1 |Map value from the range [start, end] to the range [0, 1] |
|Remap |**value**=0, **inputStart**=0, **inputEnd**=1, **outputStart**=0, **outputEnd**=1 |Remap input value within input range to output range |
|Wrap |**value**=0, **min**=0, **max**=1 |Wrap value into the range [min, max) |
|FloatEquals |**x**=0, **y**=0 |Return true (1) if x and y are almost equal (within a small relative tolerance), else false (0) |
|Vector2Zero | |Get a Vector2 map {x, y} with all components 0 |
|Vector2One | |Get a Vector2 map {x, y} with all components 1 |
|Vector2Add |**v1**=[0, 0], **v2**=[0, 0] |Add two vectors (v1 + v2) |
|Vector2AddValue |**v**=[0, 0], **add**=0 |Add a number to each component of vector v |
|Vector2Subtract |**v1**=[0, 0], **v2**=[0, 0] |Subtract two vectors (v1 - v2) |
|Vector2SubtractValue |**v**=[0, 0], **sub**=0 |Subtract a number from each component of vector v |
|Vector2Length |**v**=[0, 0] |Calculate vector length |
|Vector2LengthSqr |**v**=[0, 0] |Calculate vector squared length |
|Vector2DotProduct |**v1**=[0, 0], **v2**=[0, 0] |Calculate the dot product of two vectors |
|Vector2CrossProduct |**v1**=[0, 0], **v2**=[0, 0] |Calculate the 2D cross product of two vectors (v1.x*v2.y - v1.y*v2.x) |
|Vector2Distance |**v1**=[0, 0], **v2**=[0, 0] |Calculate distance between two vectors |
|Vector2DistanceSqr |**v1**=[0, 0], **v2**=[0, 0] |Calculate squared distance between two vectors |
|Vector2Angle |**v1**=[0, 0], **v2**=[0, 0] |Calculate the signed angle (in radians) from v1 to v2, relative to the origin (0, 0). With positive X right and positive Y down, positive angles appear clockwise and negative angles appear counterclockwise |
|Vector2LineAngle |**start**=[0, 0], **end**=[0, 0] |Calculate the angle (in radians) of the line from start to end, i.e. -atan(end.y - start.y, end.x - start.x) |
|Vector2Scale |**v**=[0, 0], **scale**=1 |Scale vector (multiply by value) |
|Vector2Multiply |**v1**=[0, 0], **v2**=[0, 0] |Multiply two vectors component-wise |
|Vector2Negate |**v**=[0, 0] |Negate vector |
|Vector2Divide |**v1**=[0, 0], **v2**=[0, 0] |Divide v1 by v2 component-wise |
|Vector2Normalize |**v**=[0, 0] |Scale vector to length 1 (a zero vector is returned unchanged) |
|Vector2Transform |**v**=[0, 0], **mat**=MatrixIdentity |Transforms a Vector2 by a given Matrix |
|Vector2Lerp |**v1**=[0, 0], **v2**=[0, 0], **amount**=0 |Calculate linear interpolation between two vectors |
|Vector2Reflect |**v**=[0, 0], **normal**=[0, 1] |Calculate the reflection of v off a surface with the given normal |
|Vector2Min |**v1**=[0, 0], **v2**=[0, 0] |Get min value for each pair of components |
|Vector2Max |**v1**=[0, 0], **v2**=[0, 0] |Get max value for each pair of components |
|Vector2Rotate |**v**=[0, 0], **angle**=0 |Rotate vector by angle (in radians) |
|Vector2MoveTowards |**v**=[0, 0], **target**=[0, 0], **maxDistance**=0 |Move vector v towards target by at most maxDistance |
|Vector2Invert |**v**=[0, 0] |Get the component-wise reciprocal of the vector (1/x, 1/y) |
|Vector2Clamp |**v**=[0, 0], **min**=[-1, -1], **max**=[1, 1] |Clamp the components of the vector between min and max values specified by the given vectors |
|Vector2ClampValue |**v**=[0, 0], **min**=0, **max**=1 |Clamp the magnitude of the vector between two min and max values |
|Vector2Equals |**p**=[0, 0], **q**=[0, 0] |Return true (1) if two vectors are almost equal (within a small relative tolerance), else false (0) |
|Vector2Refract |**v**=[0, 0], **n**=[0, 1], **r**=1 |Compute the direction of a refracted ray. v: normalized direction of the incoming ray; n: normalized normal vector of the interface between the two media; r: ratio of the refractive index of the medium the ray comes from to that of the medium on the other side |
|Vector3Zero | |Get a Vector3 map {x, y, z} with all components 0 |
|Vector3One | |Get a Vector3 map {x, y, z} with all components 1 |
|Vector3Add |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Add two vectors |
|Vector3AddValue |**v**=[0, 0, 0], **add**=0 |Add a number to each component of vector v |
|Vector3Subtract |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Subtract two vectors |
|Vector3SubtractValue |**v**=[0, 0, 0], **sub**=0 |Subtract a number from each component of vector v |
|Vector3Scale |**v**=[0, 0, 0], **scalar**=1 |Multiply vector by scalar |
|Vector3Multiply |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Multiply two vectors component-wise |
|Vector3CrossProduct |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate two vectors cross product |
|Vector3Perpendicular |**v**=[0, 0, 0] |Calculate a vector perpendicular to v |
|Vector3Length |**v**=[0, 0, 0] |Calculate vector length |
|Vector3LengthSqr |**v**=[0, 0, 0] |Calculate vector squared length |
|Vector3DotProduct |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate the dot product of two vectors |
|Vector3Distance |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate distance between two vectors |
|Vector3DistanceSqr |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate squared distance between two vectors |
|Vector3Angle |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate the angle (in radians) between two vectors |
|Vector3Negate |**v**=[0, 0, 0] |Negate provided vector (invert direction) |
|Vector3Divide |**v1**=[0, 0, 0], **v2**=[1, 1, 1] |Divide v1 by v2 component-wise |
|Vector3Normalize |**v**=[0, 0, 0] |Scale vector to length 1 (a zero vector is returned unchanged) |
|Vector3Project |**v1**=[0, 0, 0], **v2**=[0, 1, 0] |Calculate the projection of v1 onto v2 |
|Vector3Reject |**v1**=[0, 0, 0], **v2**=[0, 1, 0] |Calculate the rejection of v1 from v2: the part of v1 perpendicular to v2 |
|Vector3OrthoNormalize |**v1**=[1, 0, 0], **v2**=[0, 1, 0] |Orthonormalize v1 and v2 (Gram-Schmidt); returns a map with keys v1 and v2, which are normalized and orthogonal to each other |
|Vector3Transform |**v**=[0, 0, 0], **mat**=MatrixIdentity |Transforms a Vector3 by a given Matrix |
|Vector3RotateByQuaternion |**v**=[0, 0, 0], **q**=QuaternionIdentity |Transform a vector by quaternion rotation |
|Vector3RotateByAxisAngle |**v**=[0, 0, 0], **axis**=[0, 1, 0], **angle**=0 |Rotate vector v around axis by angle (in radians) |
|Vector3MoveTowards |**v**=[0, 0, 0], **target**=[0, 0, 0], **maxDistance**=0 |Move vector v towards target by at most maxDistance |
|Vector3Lerp |**v1**=[0, 0, 0], **v2**=[0, 0, 0], **amount**=0 |Calculate linear interpolation between two vectors |
|Vector3CubicHermite |**v1**=[0, 0, 0], **tangent1**=[0, 0, 0], **v2**=[0, 0, 0], **tangent2**=[0, 0, 0], **amount**=0 |Calculate cubic hermite interpolation between two vectors and their tangents as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic |
|Vector3Reflect |**v**=[0, 0, 0], **normal**=[0, 1, 0] |Calculate the reflection of v off a surface with the given normal |
|Vector3Min |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Get min value for each pair of components |
|Vector3Max |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Get max value for each pair of components |
|Vector3Barycenter |**p**=[0, 0, 0], **a**=[0, 0, 0], **b**=[0, 0, 0], **c**=[0, 0, 0] |Compute barycentric coordinates (u, v, w), returned as {x, y, z}, for point p with respect to triangle (a, b, c). Assumes p lies in the plane of the triangle |
|Vector3Unproject |**source**=[0, 0, 0], **projection**=MatrixIdentity, **view**=MatrixIdentity |Unproject a point from normalized device coordinates (-1 to 1) into world space, using the given projection and view matrices |
|Vector3ToFloatV |**v**=[0, 0, 0] |Get vector components as a list [x, y, z] |
|Vector3Invert |**v**=[0, 0, 0] |Get the component-wise reciprocal of the vector (1/x, 1/y, 1/z) |
|Vector3Clamp |**v**=[0, 0, 0], **min**=[-1, -1, -1], **max**=[1, 1, 1] |Clamp the components of the vector between min and max values specified by the given vectors |
|Vector3ClampValue |**v**=[0, 0, 0], **min**=0, **max**=1 |Clamp the magnitude of the vector between min and max values |
|Vector3Equals |**p**=[0, 0, 0], **q**=[0, 0, 0] |Return true (1) if two vectors are almost equal (within a small relative tolerance), else false (0) |
|Vector3Refract |**v**=[0, 0, 0], **n**=[0, 1, 0], **r**=1 |Compute the direction of a refracted ray. v: normalized direction of the incoming ray; n: normalized normal vector of the interface between the two media; r: ratio of the refractive index of the medium the ray comes from to that of the medium on the other side |
|Vector4Zero | |Get a Vector4 map {x, y, z, w} with all components 0 |
|Vector4One | |Get a Vector4 map {x, y, z, w} with all components 1 |
|Vector4Add |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Add two vectors |
|Vector4AddValue |**v**=[0, 0, 0, 0], **add**=0 |Add a number to each component of vector v |
|Vector4Subtract |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Subtract two vectors (v1 - v2) |
|Vector4SubtractValue |**v**=[0, 0, 0, 0], **sub**=0 |Subtract a number from each component of vector v |
|Vector4Length |**v**=[0, 0, 0, 0] |Calculate vector length |
|Vector4LengthSqr |**v**=[0, 0, 0, 0] |Calculate vector squared length |
|Vector4DotProduct |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Calculate the dot product of two vectors |
|Vector4Distance |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Calculate distance between two vectors |
|Vector4DistanceSqr |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Calculate squared distance between two vectors |
|Vector4Scale |**v**=[0, 0, 0, 0], **scale**=1 |Scale vector components by value (multiply) |
|Vector4Multiply |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Multiply two vectors component-wise |
|Vector4Negate |**v**=[0, 0, 0, 0] |Negate vector |
|Vector4Divide |**v1**=[0, 0, 0, 0], **v2**=[1, 1, 1, 1] |Divide v1 by v2 component-wise |
|Vector4Normalize |**v**=[0, 0, 0, 0] |Scale vector to length 1 (a zero vector is returned unchanged) |
|Vector4Min |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Get min value for each pair of components |
|Vector4Max |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Get max value for each pair of components |
|Vector4Lerp |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0], **amount**=0 |Calculate linear interpolation between two vectors |
|Vector4MoveTowards |**v**=[0, 0, 0, 0], **target**=[0, 0, 0, 0], **maxDistance**=0 |Move vector v towards target by at most maxDistance |
|Vector4Invert |**v**=[1, 1, 1, 1] |Get the component-wise reciprocal of the vector (1/x, 1/y, 1/z, 1/w) |
|Vector4Equals |**p**=[0, 0, 0, 0], **q**=[0, 0, 0, 0] |Return true (1) if two vectors are almost equal (within a small relative tolerance), else false (0) |
|MatrixDeterminant |**mat**=MatrixIdentity |Compute matrix determinant |
|MatrixTrace |**mat**=MatrixIdentity |Get the trace of the matrix (sum of the values along the diagonal) |
|MatrixTranspose |**mat**=MatrixIdentity |Transpose provided matrix |
|MatrixInvert |**mat**=MatrixIdentity |Invert provided matrix |
|MatrixIdentity | |Get identity matrix |
|MatrixAdd |**left**=MatrixIdentity, **right**=MatrixIdentity |Add two matrices |
|MatrixSubtract |**left**=MatrixIdentity, **right**=MatrixIdentity |Subtract two matrices (left - right) |
|MatrixMultiply |**left**=MatrixIdentity, **right**=MatrixIdentity |Multiply two matrices; note that the order of the arguments matters |
|MatrixMultiplyValue |**left**=MatrixIdentity, **value**=1 |Multiply matrix components by value |
|MatrixTranslate |**x**=0, **y**=0, **z**=0 |Get translation matrix for offset (x, y, z) |
|MatrixRotate |**axis**=[0, 1, 0], **angle**=0 |Create rotation matrix from axis and angle (in radians) |
|MatrixRotateX |**angle**=0 |Get rotation matrix about the X axis; angle is in radians |
|MatrixRotateY |**angle**=0 |Get rotation matrix about the Y axis; angle is in radians |
|MatrixRotateZ |**angle**=0 |Get rotation matrix about the Z axis; angle is in radians |
|MatrixRotateXYZ |**angle**=[0, 0, 0] |Get xyz-rotation matrix; angle is a Vector3 of rotations about each axis, in radians |
|MatrixRotateZYX |**angle**=[0, 0, 0] |Get zyx-rotation matrix; angle is a Vector3 of rotations about each axis, in radians |
|MatrixScale |**x**=1, **y**=1, **z**=1 |Get scaling matrix for scale factors (x, y, z) |
|MatrixFrustum |**left**=-1, **right**=1, **bottom**=-1, **top**=1, **nearPlane**=0.01, **farPlane**=1000 |Get perspective projection matrix |
|MatrixPerspective |**fovY**=45.0 * DEG2RAD, **aspect**=1, **nearPlane**=0.01, **farPlane**=1000 |Get perspective projection matrix; fovY is the vertical field of view in radians |
|MatrixOrtho |**left**=-1, **right**=1, **bottom**=-1, **top**=1, **nearPlane**=0.01, **farPlane**=1000 |Get orthographic projection matrix |
|MatrixLookAt |**eye**=[0, 0, 1], **target**=[0, 0, 0], **up**=[0, 1, 0] |Get camera look-at matrix (view matrix) |
|MatrixToFloatV |**mat**=MatrixIdentity |Get matrix data as a flat list of 16 numbers, in column-major order |
|MatrixCompose |**translation**=[0, 0, 0], **rotation**=QuaternionIdentity, **scale**=[1, 1, 1] |Compose a transformation matrix from rotational, translational and scaling components |
|MatrixDecompose |**mat**=MatrixIdentity |Decompose a transformation matrix into translation, rotation and scale (removing any shear); returns a map with keys translation (Vector3), rotation (Quaternion) and scale (Vector3) |
|QuaternionAdd |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Add two quaternions |
|QuaternionAddValue |**q**=QuaternionIdentity, **add**=0 |Add a number to each component of quaternion q |
|QuaternionSubtract |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Subtract two quaternions |
|QuaternionSubtractValue |**q**=QuaternionIdentity, **sub**=0 |Subtract a number from each component of quaternion q |
|QuaternionIdentity | |Get identity quaternion |
|QuaternionLength |**q**=QuaternionIdentity |Compute the length of a quaternion |
|QuaternionNormalize |**q**=QuaternionIdentity |Normalize provided quaternion |
|QuaternionInvert |**q**=QuaternionIdentity |Invert provided quaternion |
|QuaternionMultiply |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Multiply two quaternions (q1 * q2) |
|QuaternionScale |**q**=QuaternionIdentity, **mul**=1 |Scale quaternion by float value |
|QuaternionDivide |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Divide q1 by q2 component-wise |
|QuaternionLerp |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity, **amount**=0 |Calculate linear interpolation between two quaternions |
|QuaternionNlerp |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity, **amount**=0 |Calculate slerp-optimized interpolation between two quaternions |
|QuaternionSlerp |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity, **amount**=0 |Calculate spherical linear interpolation between two quaternions |
|QuaternionCubicHermiteSpline |**q1**=QuaternionIdentity, **outTangent1**=QuaternionIdentity, **q2**=QuaternionIdentity, **inTangent2**=QuaternionIdentity, **t**=0 |Calculate quaternion cubic spline interpolation using Cubic Hermite Spline algorithm as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic |
|QuaternionFromVector3ToVector3 |**from**=[1, 0, 0], **to**=[0, 1, 0] |Calculate quaternion based on the rotation from one vector to another |
|QuaternionFromMatrix |**mat**=MatrixIdentity |Get a quaternion for a given rotation matrix |
|QuaternionToMatrix |**q**=QuaternionIdentity |Get a matrix for a given quaternion |
|QuaternionFromAxisAngle |**axis**=[0, 1, 0], **angle**=0 |Get rotation quaternion for an axis and angle (in radians) |
|QuaternionToAxisAngle |**q**=QuaternionIdentity |Get the rotation axis and angle for a given quaternion; returns a map with keys axis (Vector3) and angle (in radians) |
|QuaternionFromEuler |**pitch**=0, **yaw**=0, **roll**=0 |Get the quaternion equivalent to Euler angles (in radians): pitch (about X), yaw (about Y) and roll (about Z). Rotation order is ZYX |
|QuaternionToEuler |**q**=QuaternionIdentity |Get the Euler angles equivalent to quaternion, as a Vector3 of rotations (in radians) about the X, Y and Z axes |
|QuaternionTransform |**q**=QuaternionIdentity, **mat**=MatrixIdentity |Transform a quaternion given a transformation matrix |
|QuaternionEquals |**p**=QuaternionIdentity, **q**=QuaternionIdentity |Return true (1) if two quaternions are almost equal (within a small relative tolerance), else false (0) |

## RShapes

|Name | Parameters | Purpose |
|-----|------------|---------|
|DrawPixel |**posX**=0, **posY**=0, **color**=WHITE |Draw a pixel |
|DrawPixelV |**position**=[0, 0], **color**=WHITE |Draw a pixel at a Vector2 position |
|DrawLine |**startPosX**=0, **startPosY**=0, **endPosX**=0, **endPosY**=0, **color**=WHITE |Draw a 1-pixel line between two points |
|DrawLineV |**startPos**=[0, 0], **endPos**=[0, 0], **color**=WHITE |Draw a 1-pixel line between two Vector2 points |
|DrawLineEx |**startPos**=[0, 0], **endPos**=[0, 0], **thick**=1, **color**=WHITE |Draw a line with the given thickness |
|DrawCircle |**centerX**=100, **centerY**=100, **radius**=32, **color**=WHITE |Draw a color-filled circle |
|DrawCircleV |**center**=[100, 100], **radius**=32, **color**=WHITE |Draw a color-filled circle at a Vector2 center |
|DrawCircleLines |**centerX**=100, **centerY**=100, **radius**=32, **color**=WHITE |Draw circle outline |
|DrawEllipse |**centerX**=100, **centerY**=100, **radiusH**=32, **radiusV**=32, **color**=WHITE |Draw ellipse |
|DrawEllipseLines |**centerX**=100, **centerY**=100, **radiusH**=32, **radiusV**=32, **color**=WHITE |Draw ellipse outline |
|DrawRing |**center**=[100, 100], **innerRadius**=20, **outerRadius**=32, **startAngle**=0, **endAngle**=360, **segments**=36, **color**=WHITE |Draw ring |
|DrawRingLines |**center**=[100, 100], **innerRadius**=20, **outerRadius**=32, **startAngle**=0, **endAngle**=360, **segments**=36, **color**=WHITE |Draw ring outline |
|DrawRingLinesEx |**center**=[100, 100], **innerRadius**=20, **outerRadius**=32, **startAngle**=0, **endAngle**=360, **segments**=36, **thick**=1.0, **color**=WHITE |Draw ring outline with line thickness |
|DrawRectangle |**x**=0, **y**=0, **width**=256, **height**=256, **color**=WHITE |Draw a color-filled rectangle |
|DrawRectangleV |**position**=[0, 0], **size**=[256, 256], **color**=WHITE |Draw a color-filled rectangle given Vector2 position and size |
|DrawRectangleRec |**rec**, **color**=WHITE |Draw a color-filled rectangle given a Rectangle |
|DrawRectanglePro |**rec**, **origin**=[0, 0], **rotation**=0, **color**=WHITE |Draw a color-filled rectangle, rotated (in degrees) around the given origin (relative to the rectangle's top-left corner) |
|DrawRectangleLines |**x**=0, **y**=0, **width**=256, **height**=256, **color** |Draw rectangle outline |
|DrawRectangleLinesEx |**rec**, **thick**=1, **color**=WHITE |Draw rectangle outline with line thickness |
|DrawRectangleRounded |**rec**, **roundness**=0.5, **segments**=36, **color**=WHITE |Draw rectangle with rounded edges; roundness ranges from 0 to 1 |
|DrawRectangleRoundedLines |**rec**, **roundness**=0.5, **segments**=36, **color**=WHITE |Draw outline of a rectangle with rounded edges |
|DrawRectangleGradientV |**posX**=0, **posY**=0, **width**=256, **height**=256, **color1**=WHITE, **color2**=BLACK |Draw a vertical-gradient-filled rectangle |
|DrawRectangleGradientH |**posX**=0, **posY**=0, **width**=256, **height**=256, **color1**=WHITE, **color2**=BLACK |Draw a horizontal-gradient-filled rectangle |
|DrawRectangleGradientEx |**rec**, **col1**, **col2**, **col3**, **col4** |Draw a gradient-filled rectangle with custom corner colors, in counter-clockwise order starting from the top-left |
|DrawTriangle |**v1**, **v2**, **v3**, **color**=WHITE |Draw a color-filled triangle; vertices may be given in either winding order |
|DrawTriangleGradient |**v1**, **v2**, **v3**, **c1**, **c2**, **c3** |Draw triangle with interpolated colors; vertices must be in counter-clockwise order |
|DrawTriangleLines |**v1**, **v2**, **v3**, **color**=WHITE |Draw a triangle outline |
|DrawTriangleLinesEx |**v1**, **v2**, **v3**, **thick**=1.0, **color**=WHITE |Draw a triangle outline with line thickness; vertices must be in counter-clockwise order |
|DrawPoly |**center**=[100, 100], **sides**=6, **radius**=32, **rotation**=0, **color**=WHITE |Draw a filled regular polygon with the given number of sides |
|DrawPolyLines |**center**=[100, 100], **sides**=6, **radius**=32, **rotation**=0, **color**=WHITE |Draw the outline of a regular polygon with the given number of sides |
|DrawPolyLinesEx |**center**=[100, 100], **sides**=6, **radius**=32, **rotation**=0, **thick**=1, **color**=WHITE |Draw the outline of a regular polygon with the given number of sides and line thickness |
|CheckCollisionRecs |**rec1**, **rec2** |Check collision between two rectangles |
|CheckCollisionCircles |**center1**, **radius1**, **center2**, **radius2** |Check collision between two circles |
|CheckCollisionCircleRec |**center**, **radius**, **rec** |Check collision between circle and rectangle |
|CheckCollisionPointRec |**point**, **rec** |Check if point is inside rectangle |
|CheckCollisionPointCircle |**point**, **center**, **radius** |Check if point is inside circle |
|CheckCollisionPointTriangle |**point**, **p1**, **p2**, **p3** |Check if point is inside a triangle defined by three points (p1, p2, p3) |
|GetCollisionRec |**rec1**, **rec2** |Get the overlapping rectangle of two rectangles (zero size if they do not overlap) |
|CheckCollisionCircleLine |**center**, **radius**, **p1**, **p2** |Check if circle collides with the line segment between points p1 and p2 |
|CheckCollisionLines |**startPos1**, **endPos1**, **startPos2**, **endPos2** |Check collision between two line segments, each defined by two points; returns the intersection point (Vector2), or null if they do not intersect |
|CheckCollisionPointLine |**point**, **p1**, **p2**, **threshold** |Check if point lies on the line segment between points p1 and p2, within a margin of threshold pixels |
|CheckCollisionPointPoly |**point**, **points** |Check if point is within a polygon described by a list of vertices (returns false if fewer than 3 points) |
|DrawCircleGradient |**centerX**=0, **centerY**=0, **radius**=10.0, **colorInner**=WHITE, **colorOuter**=BLACK |Draw a gradient-filled circle |
|DrawCircleLinesV |**center**, **radius**=10.0, **color**=WHITE |Draw circle outline at a Vector2 center |
|DrawCircleLinesEx |**center**, **radius**=10.0, **thick**=1.0, **color**=WHITE |Draw circle outline with line thickness |
|DrawCircleSector |**center**, **radius**=10.0, **startAngle**=0, **endAngle**=90.0, **segments**=36, **color**=WHITE |Draw a piece of a circle |
|DrawCircleSectorLines |**center**, **radius**=10.0, **startAngle**=0, **endAngle**=90.0, **segments**=36, **color**=WHITE |Draw the outline of a piece of a circle |
|DrawCircleSectorLinesEx |**center**, **radius**=10.0, **startAngle**=0, **endAngle**=90.0, **segments**=36, **thick**=1.0, **color**=WHITE |Draw the outline of a piece of a circle, with line thickness |
|DrawEllipseV |**center**, **radiusH**=10.0, **radiusV**=5.0, **color**=WHITE |Draw ellipse at a Vector2 center |
|DrawEllipseLinesV |**center**, **radiusH**=10.0, **radiusV**=5.0, **color**=WHITE |Draw ellipse outline at a Vector2 center |
|DrawEllipseLinesEx |**center**, **radiusH**=10.0, **radiusV**=5.0, **thick**=1.0, **color**=WHITE |Draw ellipse outline with line thickness |
|DrawLineBezier |**startPos**, **endPos**, **thick**=1.0, **color**=WHITE |Draw a line using cubic-bezier ease-in-out interpolation (no control points needed) |
|DrawLineDashed |**startPos**, **endPos**, **dashSize**, **spaceSize**, **color**=WHITE |Draw a dashed line |
|DrawLineStrip |**points**, **color**=WHITE |Draw a sequence of connected lines through a list of points |
|DrawRectangleRoundedLinesEx |**rec**, **roundness**=0.0, **segments**=0, **thick**=1.0, **color**=WHITE |Draw outline of a rectangle with rounded edges, with line thickness |
|DrawSplineLinear |**points**, **thick**=1.0, **color**=WHITE |Draw spline through a list of points: Linear, minimum 2 points |
|DrawSplineBasis |**points**, **thick**=1.0, **color**=WHITE |Draw spline from a list of points: B-Spline, minimum 4 points |
|DrawSplineCatmullRom |**points**, **thick**=1.0, **color**=WHITE |Draw spline from a list of points: Catmull-Rom, minimum 4 points |
|DrawSplineBezierQuadratic |**points**, **thick**=1.0, **color**=WHITE |Draw spline from a list of points: Quadratic Bezier, minimum 3 points (1 control point): [p1, c2, p3, c4...] |
|DrawSplineBezierCubic |**points**, **thick**=1.0, **color**=WHITE |Draw spline from a list of points: Cubic Bezier, minimum 4 points (2 control points): [p1, c2, c3, p4, c5, c6...] |
|DrawSplineSegmentLinear |**p1**, **p2**, **thick**=1.0, **color**=WHITE |Draw spline segment: Linear, 2 points |
|DrawSplineSegmentBasis |**p1**, **p2**, **p3**, **p4**, **thick**=1.0, **color**=WHITE |Draw spline segment: B-Spline, 4 points |
|DrawSplineSegmentCatmullRom |**p1**, **p2**, **p3**, **p4**, **thick**=1.0, **color**=WHITE |Draw spline segment: Catmull-Rom, 4 points |
|DrawSplineSegmentBezierQuadratic |**p1**, **p2**, **p3**, **thick**=1.0, **color**=WHITE |Draw spline segment: Quadratic Bezier, 2 points (p1, p3) and 1 control point (p2) |
|DrawSplineSegmentBezierCubic |**p1**, **p2**, **p3**, **p4**, **thick**=1.0, **color**=WHITE |Draw spline segment: Cubic Bezier, 2 points (p1, p4) and 2 control points (p2, p3) |
|GetSplinePointLinear |**startPos**, **endPos**, **t** |Get spline point (map with x, y) for a given t from 0 to 1: Linear |
|GetSplinePointBasis |**p1**, **p2**, **p3**, **p4**, **t** |Get spline point (map with x, y) for a given t from 0 to 1: B-Spline |
|GetSplinePointCatmullRom |**p1**, **p2**, **p3**, **p4**, **t** |Get spline point (map with x, y) for a given t from 0 to 1: Catmull-Rom |
|GetSplinePointBezierQuadratic |**p1**, **c2**, **p3**, **t** |Get spline point (map with x, y) for a given t from 0 to 1: Quadratic Bezier |
|GetSplinePointBezierCubic |**p1**, **c2**, **c3**, **p4**, **t** |Get spline point (map with x, y) for a given t from 0 to 1: Cubic Bezier |
|DrawTriangleFan |**points**, **color**=WHITE |Draw a triangle fan defined by a list of points (minimum 3). The first point is the center, shared by all triangles; the following points should be in counter-clockwise order |
|DrawTriangleStrip |**points**, **color**=WHITE |Draw a triangle strip defined by a list of points (minimum 3); each new point forms a triangle with the previous two |
|SetShapesTexture |**texture**, **rec** |Set texture and source rectangle to be used for shapes drawing. Useful for batching: e.g. pointing at a white area of a font texture lets shapes and text draw in a single draw call |
|GetShapesTexture | |Get texture that is used for shapes drawing |
|GetShapesTextureRectangle | |Get texture source rectangle that is used for shapes drawing |

## RModels

|Name | Parameters | Purpose |
|-----|------------|---------|
|DrawLine3D |**startPos**, **endPos**, **color**=WHITE |Draw a line in 3D world space |
|DrawPoint3D |**position**, **color**=WHITE |Draw a point in 3D space (actually a very short line, as OpenGL ES 2.0 does not support point mode drawing) |
|DrawCircle3D |**center**, **radius**=1.0, **rotationAxis**=[0, 1, 0], **rotationAngle**=0, **color**=WHITE |Draw a circle in 3D world space, rotated rotationAngle degrees around rotationAxis |
|DrawTriangle3D |**v1**, **v2**, **v3**, **color**=WHITE |Draw a color-filled triangle (vertices in counter-clockwise order!) |
|DrawTriangleStrip3D |**points**, **color**=WHITE |Draw a triangle strip defined by a list of points (at least 3) |
|DrawCube |**position**, **width**=1.0, **height**=1.0, **length**=1.0, **color**=WHITE |Draw a cube; position is its center |
|DrawCubeV |**position**, **size**=[1, 1, 1], **color**=WHITE |Draw a cube, with its size given as a vector; position is its center |
|DrawCubeWires |**position**, **width**=1.0, **height**=1.0, **length**=1.0, **color**=WHITE |Draw cube wires; position is the cube's center |
|DrawCubeWiresV |**position**, **size**=[1, 1, 1], **color**=WHITE |Draw cube wires, with the size given as a vector; position is the cube's center |
|DrawSphere |**centerPos**, **radius**=1.0, **color**=WHITE |Draw sphere |
|DrawSphereEx |**centerPos**, **radius**=1.0, **rings**=16, **slices**=16, **color**=WHITE |Draw sphere with defined rings and slices |
|DrawSphereWires |**centerPos**, **radius**=1.0, **rings**=16, **slices**=16, **color**=WHITE |Draw sphere wires |
|DrawCylinder |**position**, **radiusTop**=1.0, **radiusBottom**=1.0, **height**=2.0, **sides**=16, **color**=WHITE |Draw a cylinder. It can also be used for pyramids and cones |
|DrawCylinderEx |**startPos**, **endPos**, **startRadius**=1.0, **endRadius**=1.0, **sides**=16, **color**=WHITE |Draw a cylinder with base at startPos and top at endPos. It can also be used for pyramids and cones |
|DrawCylinderWires |**position**, **radiusTop**=1.0, **radiusBottom**=1.0, **height**=2.0, **sides**=16, **color**=WHITE |Draw a wired cylinder. It can also be used for pyramids and cones |
|DrawCylinderWiresEx |**startPos**, **endPos**, **startRadius**=1.0, **endRadius**=1.0, **sides**=16, **color**=WHITE |Draw a wired cylinder with base at startPos and top at endPos. It can also be used for pyramids and cones |
|DrawCapsule |**startPos**, **endPos**, **radius**=1.0, **rings**=8, **slices**=16, **color**=WHITE |Draw a capsule with the center of its sphere caps at startPos and endPos |
|DrawCapsuleWires |**startPos**, **endPos**, **radius**=1.0, **rings**=8, **slices**=16, **color**=WHITE |Draw capsule wires with the center of its sphere caps at startPos and endPos |
|DrawPlane |**centerPos**, **size**=[1, 1], **color**=WHITE |Draw a plane |
|DrawRay |**ray**, **color**=WHITE |Draw a ray line |
|DrawGrid |**slices**=10, **spacing**=1.0 |Draw a grid centered at (0, 0, 0) |
|DrawBillboard |**camera**, **texture**, **position**, **scale**=1.0, **tint**=WHITE |Draw a billboard |
|DrawBillboardRec |**camera**, **texture**, **rec**, **position**, **size**=[1, 1], **tint**=WHITE |Draw a billboard (part of a texture defined by a rectangle) |
|DrawBillboardPro |**camera**, **texture**, **rec**, **position**, **up**=[0, 1, 0], **size**=[1, 1], **origin**=[0.5, 0.5], **rotation**=0, **tint**=WHITE |Draw a billboard texture defined by source rectangle, with scaling and rotation (in degrees) |
|LoadModel |**fileName** |Load a model (meshes and materials) from a file. Returns null on failure |
|LoadModelFromMesh |**mesh** |Load a model from a mesh (e.g. from MakeMesh or GenMeshCube), with a default material. The model takes ownership of the mesh's data: you can go on using the mesh while the model is loaded, but UnloadModel unloads it (and UnloadMesh of it does nothing).  Returns null if the mesh is unloaded or already part of a model |
|IsModelValid |**model** |Check if model is valid (loaded in GPU, VAO/VBOs) |
|UnloadModel |**model** |Unload a model and its meshes from memory (RAM and VRAM). Shaders and textures used by its materials are not unloaded, as they may be shared; unload those separately |
|GetModelBoundingBox |**model** |Compute model bounding box limits (considers all meshes) |
|DrawModel |**model**, **position**=[0, 0, 0], **scale**=1.0, **tint**=WHITE |Draw a model (with texture if set) at a position and uniform scale |
|DrawModelEx |**model**, **position**=[0, 0, 0], **rotationAxis**=[0, 1, 0], **rotationAngle**=0, **scale**=[1, 1, 1], **tint**=WHITE |Draw a model with custom transform (rotationAngle in degrees) |
|DrawModelWires |**model**, **position**=[0, 0, 0], **scale**=1.0, **tint**=WHITE |Draw a model as wires (with texture if set) |
|DrawModelWiresEx |**model**, **position**=[0, 0, 0], **rotationAxis**=[0, 1, 0], **rotationAngle**=0, **scale**=[1, 1, 1], **tint**=WHITE |Draw a model as wires, with custom transform (rotationAngle in degrees) |
|DrawBoundingBox |**box**, **color**=WHITE |Draw a bounding box with wires |
|UploadMesh |**mesh**, **dynamic**=0 |Upload a mesh's vertex data to the GPU, into a VAO (if supported) and VBOs; set dynamic true for buffers you will update often. Meshes from MakeMesh and the GenMesh functions are already uploaded |
|UpdateMeshBuffer |**mesh**, **index**, **data**, **offset**=0 |Update one of a mesh's vertex buffers on the GPU. index is as for GetMeshBuffer (0 vertices, 1 texcoords, 2 normals, 3 colors, ... 6 indices ...); data is a RawData (raw bytes) or a list of numbers or of vectors/colors, packed to the buffer's element type; offset is in bytes. Only the GPU copy changes, not the data GetMeshBuffer returns |
|MakeMesh |**vertices**, **indices**=null, **texcoords**=null, **normals**=null, **colors**=null, **dynamic**=0 |Make a mesh from vertex data: flat lists of numbers, lists of vectors/colors (one per vertex), or RawData. vertices is required (3 numbers per vertex); indices (3 per triangle, 0-65535), texcoords (2 per vertex), normals (3 per vertex) and colors (4 per vertex, 0-255) are optional. The mesh is uploaded to the GPU (with dynamic buffers if dynamic is true, for frequent UpdateMeshBuffer calls), ready to draw; UnloadMesh frees it |
|GetMeshBuffer |**mesh**, **index** |Get a copy of one of a mesh's vertex data arrays, as a flat list of numbers, by the same index UpdateMeshBuffer takes: 0 vertices, 1 texcoords, 2 normals, 3 colors, 4 tangents, 5 texcoords2, 6 indices, 7 boneIndices, 8 boneWeights. Returns null if the mesh has no such data |
|UnloadMesh |**mesh** |Unload mesh from memory (RAM and VRAM).  Does nothing for a mesh a model owns (from GetModelMesh, or given to LoadModelFromMesh): UnloadModel unloads that |
|DrawMesh |**mesh**, **material**, **transform**=MatrixIdentity |Draw a 3D mesh with material and transform |
|DrawMeshInstanced |**mesh**, **material**, **transforms** |Draw multiple mesh instances with material, one per matrix in the transforms list |
|GetMeshBoundingBox |**mesh** |Compute mesh bounding box limits, in the mesh's own coordinates (not transformed by any model transform) |
|GenMeshTangents |**mesh** |Compute mesh tangents |
|ExportMesh |**mesh**, **fileName** |Export mesh data to a file (e.g. .obj). Returns true on success |
|ExportMeshAsCode |**mesh**, **fileName** |Export mesh as a C code file (.h) defining arrays of vertex attributes. Returns true on success |
|GenMeshPoly |**sides**=6, **radius**=1.0 |Generate polygonal mesh |
|GenMeshPlane |**width**=1.0, **length**=1.0, **resX**=1, **resZ**=1 |Generate plane mesh (with subdivisions) |
|GenMeshCube |**width**=1.0, **height**=1.0, **length**=1.0 |Generate cuboid mesh |
|GenMeshSphere |**radius**=1.0, **rings**=16, **slices**=16 |Generate sphere mesh (standard sphere) |
|GenMeshHemiSphere |**radius**=1.0, **rings**=16, **slices**=16 |Generate hemisphere mesh (half sphere, no bottom cap) |
|GenMeshCylinder |**radius**=1.0, **height**=2.0, **slices**=16 |Generate cylinder mesh |
|GenMeshCone |**radius**=1.0, **height**=2.0, **slices**=16 |Generate cone/pyramid mesh |
|GenMeshTorus |**radius**=1.0, **size**=0.5, **radSeg**=16, **sides**=16 |Generate torus mesh. The ring's center line has radius size/2; radius (clamped to 0.1-1) is the tube's radius relative to that |
|GenMeshKnot |**radius**=1.0, **size**=0.5, **radSeg**=16, **sides**=16 |Generate trefoil knot mesh |
|GenMeshHeightmap |**heightmap**, **size**=[1, 1, 1] |Generate a terrain mesh from a heightmap image; size is the mesh's size in world units (y being the maximum height) |
|GenMeshCubicmap |**cubicmap**, **cubeSize**=[1, 1, 1] |Generate a mesh of cubes from a cubicmap image (white pixels become cubes, other pixels floor and ceiling), each cubeSize in size |
|LoadMaterials |**fileName** |Load materials from a model file. Returns a list of materials, or null on failure |
|LoadMaterialDefault | |Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps) |
|IsMaterialValid |**material** |Check if material is valid (map textures loaded in GPU) |
|UnloadMaterial |**material** |Unload material from memory. Also unloads the shader and every texture map set on it, so don't unload those separately |
|SetMaterialTexture |**material**, **mapType**, **texture** |Set texture for a material map type (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...). The material owns the new texture: UnloadMaterial unloads it. Any texture previously set on that map is not unloaded |
|SetMaterialColor |**material**, **mapType**, **color**=WHITE |Set the color of one map (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...) of a material. The diffuse color tints whatever is drawn with the material, multiplied with its texture. |
|GetMaterialShader |**material** |Get the shader a material uses.  The material owns it: UnloadMaterial unloads it |
|SetMaterialShader |**material**, **shader** |Set the shader a material uses.  The material takes ownership of it: UnloadMaterial unloads it |
|GetMaterialShaderLocation |**material**, **uniformName** |Get the location of a uniform in a material's shader, or -1 |
|GetMaterialShaderLocationAttrib |**material**, **attribName** |Get the location of a vertex attribute in a material's shader, or -1 |
|SetMaterialShaderValue |**material**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT |Set a uniform in a material's shader.  value is a number, a list of components, or a RawData; uniformType is a SHADER_UNIFORM_ constant |
|SetMaterialShaderValueV |**material**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT, **count**=0 |Set a uniform array in a material's shader, from a list (of numbers or of vectors) or a RawData.  If count is 0, it is the number of elements in the list, or 1 for a RawData |
|SetMaterialShaderValueMatrix |**material**, **locIndex**, **mat** |Set a matrix uniform in a material's shader |
|SetMaterialShaderValueTexture |**material**, **locIndex**, **texture** |Set a sampler2D uniform in a material's shader to a texture |
|SetModelMeshMaterial |**model**, **meshId**, **materialId** |Set the material for a mesh |
|SetModelMaterialShader |**model**, **materialIndex**, **shader** |Set the shader of one material of a model. Use this to shade a model with a custom shader; BeginShaderMode does not affect DrawModel. The script keeps ownership of the shader and must unload it with UnloadShader. |
|SetModelMaterialTexture |**model**, **materialIndex**, **mapType**, **texture** |Set the texture of one map (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...) of one material of a model. The script keeps ownership of the texture and must unload it with UnloadTexture. |
|SetModelMaterialColor |**model**, **materialIndex**, **mapType**, **color**=WHITE |Set the color of one map (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...) of one material of a model. The diffuse color tints the model, multiplied with the texture and the DrawModel tint. |
|GetModelMesh |**model**, **meshIndex**=0 |Get one of a model's meshes, for DrawMesh, GetRayCollisionMesh, GetMeshBoundingBox, GetMeshBuffer, UpdateMeshBuffer and the like. The model owns the mesh: UnloadMesh of it does nothing, and once the model is unloaded it reads as unloaded too. Functions that change the Mesh struct itself (GenMeshTangents, UploadMesh) do not affect the model; use them before LoadModelFromMesh |
|GetModelTransform |**model** |Get a model's local transform matrix, which DrawModel applies before the position, rotation and scale it is given |
|SetModelTransform |**model**, **transform** |Set a model's local transform matrix, which DrawModel applies before the position, rotation and scale it is given |
|LoadModelAnimations |**fileName** |Load model animations from a file. Returns a list of animations (maps with name, boneCount and keyframeCount), or null on failure; unload with UnloadModelAnimations |
|UpdateModelAnimation |**model**, **animation**, **frame**=0 |Update model animation data (vertex buffers / bone matrices) to the pose at a given frame. frame may be fractional, interpolating between two frames. With CPU skinning the vertex data is uploaded to the GPU now; with GPU skinning, bone matrices go to the shader when the model is drawn |
|UpdateModelAnimationEx |**model**, **animationA**, **frameA**=0, **animationB**, **frameB**=0, **blend**=0 |Update model animation data (vertex buffers / bone matrices) to a pose blending two animations at given frames (blend 0-1, 0 being all animationA). Frames may be fractional, interpolating between two frames. Not available on web |
|UnloadModelAnimations |**animations** |Unload the list of animations LoadModelAnimations returned (passing any one of them unloads them all) |
|IsModelAnimationValid |**model**, **animation** |Check whether an animation's skeleton matches a model's (only the number of bones and parent connections are checked) |
|GetModelAnimationPose |**animation**, **frame**=0 |Get an animation's bone poses at one keyframe: a list of {translation, rotation, scale}, one per bone (in the order of the model's bones list). Returns null if the frame is out of range |
|CheckCollisionSpheres |**center1**, **radius1**, **center2**, **radius2** |Check collision between two spheres; returns true or false |
|CheckCollisionBoxes |**box1**, **box2** |Check collision between two bounding boxes (each defined by its min and max points); returns true or false |
|CheckCollisionBoxSphere |**box**, **center**, **radius** |Check collision between bounding box and sphere; returns true or false |
|GetRayCollisionSphere |**ray**, **center**, **radius** |Get collision info between ray and sphere, as a map with hit, distance, point and normal |
|GetRayCollisionBox |**ray**, **box** |Get collision info between ray and bounding box, as a map with hit, distance, point and normal |
|GetRayCollisionMesh |**ray**, **mesh**, **transform**=MatrixIdentity |Get collision info between ray and mesh (placed by transform), as a map with hit, distance, point and normal |
|GetRayCollisionTriangle |**ray**, **p1**, **p2**, **p3** |Get collision info between ray and triangle (points in counter-clockwise winding), as a map with hit, distance, point and normal |
|GetRayCollisionQuad |**ray**, **p1**, **p2**, **p3**, **p4** |Get collision info between ray and quad (points in counter-clockwise winding), as a map with hit, distance, point and normal |

## RTextures

|Name | Parameters | Purpose |
|-----|------------|---------|
|LoadImage |**fileName** |Load image from file into CPU memory (RAM); returns null on failure |
|GenImageGradientLinear |**width**=256, **height**=256, **direction**=0, **start**=BLACK, **end**=WHITE |Generate image: linear gradient. direction is in degrees: 0 is vertical (from top to bottom), 90 is horizontal (from left to right); the gradient rotates counter-clockwise by the specified amount |
|UnloadImage |**image** |Unload image from CPU memory (RAM) |
|LoadTexture |**fileName** |Load texture from file into GPU memory (VRAM); returns null on failure |
|LoadTextureFromImage |**image** |Load a texture from image data. The image is not affected and remains usable |
|UnloadTexture |**texture** |Unload texture from GPU memory (VRAM) |
|DrawTexture |**texture**, **posX**=0, **posY**=0, **tint**=WHITE |Draw a texture |
|DrawTextureV |**texture**, **position**=[0, 0], **tint**=WHITE |Draw a texture with position defined as a Vector2 |
|DrawTextureEx |**texture**, **position**=[0, 0], **rotation**=0, **scale**=1.0, **tint**=WHITE |Draw a texture with rotation (degrees) and scale. NOTE: Rotation is around the top-left corner |
|DrawTextureRec |**texture**, **rec**, **position**=[0, 0], **tint**=WHITE |Draw a part of a texture (defined by a rectangle) |
|DrawTexturePro |**texture**, **source**, **dest**, **origin**=[0, 0], **rotation**=0, **tint**=WHITE |Draw a part of a texture (source rectangle) into a destination rectangle, with origin and rotation (degrees). NOTE: origin is relative to the destination rectangle |
|GenImageColor |**width**=256, **height**=256, **color**=WHITE |Generate image: plain color |
|GenImageGradientRadial |**width**=256, **height**=256, **density**=0.5, **inner**=WHITE, **outer**=BLACK |Generate image: radial gradient |
|GenImageGradientSquare |**width**=256, **height**=256, **density**=0.5, **inner**=WHITE, **outer**=BLACK |Generate image: square gradient |
|GenImageChecked |**width**=256, **height**=256, **checksX**=8, **checksY**=8, **col1**=WHITE, **col2**=BLACK |Generate image: checkerboard |
|GenImageWhiteNoise |**width**=256, **height**=256, **factor**=0.5 |Generate image: white noise; factor (0-1) is the fraction of white pixels |
|GenImageCellular |**width**=256, **height**=256, **tileSize**=32 |Generate image: cellular algorithm. Bigger tileSize means bigger cells |
|ImageCopy |**image** |Copy an image to a new image |
|ImageCrop |**image**, **crop** |Crop an image (in place) to the area defined by a rectangle. The rectangle is clamped to the image bounds |
|ImageResize |**image**, **newWidth**, **newHeight** |Resize an image (in place) to a new size, using high-quality bicubic filtering |
|ImageResizeNN |**image**, **newWidth**, **newHeight** |Resize an image (in place) to a new size using nearest-neighbor scaling |
|ImageFlipVertical |**image** |Flip image vertically |
|ImageFlipHorizontal |**image** |Flip image horizontally |
|ImageRotateCW |**image** |Rotate image clockwise 90 degrees |
|ImageRotateCCW |**image** |Rotate image counter-clockwise 90 degrees |
|ImageColorTint |**image**, **color**=WHITE |Modify image color: tint |
|ImageColorInvert |**image** |Modify image color: invert |
|ImageColorGrayscale |**image** |Modify image color: grayscale |
|ImageColorContrast |**image**, **contrast** |Modify image color: contrast (-100 to 100) |
|ImageColorBrightness |**image**, **brightness** |Modify image color: brightness (-255 to 255) |
|ImageClearBackground |**dst**, **color**=WHITE |Clear image background with given color |
|ImageDrawPixel |**dst**, **x**=0, **y**=0, **color**=WHITE |Draw pixel within an image. NOTE: Compressed image formats not supported |
|ImageDrawPixelV |**dst**, **position**=[0, 0], **color**=WHITE |Draw pixel within an image (Vector version) |
|ImageDrawLine |**dst**, **startPosX**=0, **startPosY**=0, **endPosX**=0, **endPosY**=0, **color**=WHITE |Draw line within an image |
|ImageDrawLineV |**dst**, **start**=[0, 0], **end**=[0, 0], **color**=WHITE |Draw line within an image (Vector version) |
|ImageDrawCircle |**dst**, **centerX**=100, **centerY**=100, **radius**=32, **color**=WHITE |Draw circle within an image |
|ImageDrawCircleV |**dst**, **center**=[100, 100], **radius**=32, **color**=WHITE |Draw circle within an image (Vector version) |
|ImageDrawRectangle |**dst**, **posX**=0, **posY**=0, **width**=256, **height**=256, **color**=WHITE |Draw rectangle within an image |
|ImageDrawRectangleRec |**dst**, **rec**, **color**=WHITE |Draw rectangle within an image |
|ImageDrawRectangleLines |**dst**, **posX**=0, **posY**=0, **width**=256, **height**=256, **color**=WHITE |Draw rectangle lines within an image |
|ImageDrawRectangleLinesEx |**dst**, **rec**, **thick**=1, **color**=WHITE |Draw rectangle lines within an image with line thickness |
|ImageDrawRectanglePro |**dst**, **rec**, **origin**=[0, 0], **rotation**=0, **color**=WHITE |Draw a color-filled rectangle with origin and rotation (degrees) within an image |
|ImageDrawRectangleGradientEx |**dst**, **rec**, **col1**, **col2**, **col3**, **col4** |Draw rectangle with gradient colors within an image, counter-clockwise color order |
|ImageDrawImage |**dst**, **src**, **posX**=0, **posY**=0, **tint**=WHITE |Draw an image within an image |
|ImageDrawImageEx |**dst**, **src**, **position**=[0, 0], **rotation**=0, **scale**=1.0, **tint**=WHITE |Draw an image with scaling and rotation (degrees) within an image. NOTE: Rotation is around the top-left corner |
|ImageDrawImageRec |**dst**, **src**, **srcRec**, **position**=[0, 0], **tint**=WHITE |Draw a part of an image defined by a rectangle within an image |
|ImageDrawImagePro |**dst**, **src**, **srcRec**, **dstRec**, **origin**=[0, 0], **rotation**=0, **tint**=WHITE |Draw a part of an image defined by a rectangle into destination rectangle, with scaling, within an image. NOTE: origin and rotation are currently ignored by raylib |
|ImageDrawText |**dst**, **text**, **posX**=0, **posY**=0, **fontSize**=20, **color**=BLACK |Draw text (default font) within an image |
|SetTextureFilter |**texture**, **filter** |Set texture scaling filter mode (a TEXTURE_FILTER_* value) |
|SetTextureWrap |**texture**, **wrap** |Set texture wrapping mode (a TEXTURE_WRAP_* value) |
|GenTextureMipmaps |**texture** |Generate GPU mipmaps for a texture |
|LoadRenderTexture |**width**=960, **height**=640 |Load RGBA texture for rendering (framebuffer) |
|LoadRenderTextureEx |**width**=960, **height**=640, **format**=PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 |Load texture for rendering (framebuffer), with specific pixel format (a PIXELFORMAT_* value) |
|UnloadRenderTexture |**target** |Unload render texture from GPU memory (VRAM) |
|BeginTextureMode |**target** |Begin drawing to render texture |
|EndTextureMode | |End drawing to render texture |
|ColorAlpha |**color**, **alpha** |Get color with alpha applied, alpha goes from 0.0 to 1.0 |
|ColorAlphaBlend |**dst**, **src**, **tint** |Get src alpha-blended into dst color with tint |
|ColorBrightness |**color**, **factor** |Get color with brightness correction, brightness factor goes from -1.0 to 1.0 |
|ColorContrast |**color**, **contrast** |Get color with contrast correction, contrast goes from -1.0 to 1.0 |
|ColorFromHSV |**hue**, **saturation**, **value** |Get a color from HSV values. Hue is in degrees [0..360]; saturation and value are [0.0..1.0]. NOTE: Color->HSV->Color conversion may not yield exactly the same color due to rounding errors |
|ColorFromNormalized |**normalized** |Get color from a map of normalized values [0..1]: x=red, y=green, z=blue, w=alpha (w defaults to 1) |
|ColorIsEqual |**col1**, **col2** |Check if two colors are equal |
|ColorLerp |**color1**, **color2**, **amount** |Get color linearly interpolated between two colors, amount [0.0..1.0] |
|ColorNormalize |**color** |Get color normalized as a map of values [0..1]: x=red, y=green, z=blue, w=alpha |
|ColorTint |**color**, **tint** |Get color multiplied with another color |
|ColorToHSV |**color** |Get HSV values for a color, as a map: x=hue in degrees [0..360], y=saturation and z=value in [0..1] |
|ColorToInt |**color** |Get a color as a 32-bit integer (0xRRGGBBAA); may be negative when red >= 128 |
|Fade |**color**, **alpha** |Get color with alpha applied, alpha goes from 0.0 to 1.0 |
|GetColor |**hexValue** |Get a color from a 32-bit integer value (0xRRGGBBAA) |
|GetPixelColor |**srcPtr**, **format** |Get the color of the pixel at the start of a RawData, interpreted in the given pixel format (a PIXELFORMAT_* value) |
|GetPixelDataSize |**width**, **height**, **format** |Get pixel data size in bytes for the given width, height, and pixel format (works for image or texture data) |
|SetPixelColor |**dstPtr**, **color**, **format** |Write a color, in the given pixel format (a PIXELFORMAT_* value), to the start of a RawData |
|GenImagePerlinNoise |**width**, **height**, **offsetX**, **offsetY**, **scale** |Generate image: perlin noise |
|GenImageText |**text**, **fontSize**, **color** |Generate image: text drawn in the default font (same as ImageText) |
|IsImageValid |**image** |Check if an image is valid (has data and a valid size and format) |
|IsRenderTextureValid |**target** |Check if render texture is valid (loaded in GPU) |
|IsTextureValid |**texture** |Check if texture is valid (loaded in GPU) |
|LoadImageAnim |**fileName**, **frames** |Load animated image data (e.g. GIF). Returns a map with 'image' and 'frames' (the frame count). The image's pixel data holds all frames back to back, in RGBA format; frame delays are discarded. The frames argument is ignored |
|LoadImageAnimFromMemory |**fileType**, **fileData**, **frames** |Load animated image data from a RawData; fileType is the extension, e.g. ".gif". Returns a map with 'image' and 'frames' (the frame count). The image's pixel data holds all frames back to back, in RGBA format; frame delays are discarded. The frames argument is ignored |
|LoadImageColors |**image** |Get the pixel colors of an image as a list of colors, row by row from the top-left |
|LoadImageFromMemory |**fileType**, **fileData** |Load image from file data in a RawData; fileType is the extension, e.g. ".png" (must be lower-case). Returns null on failure |
|LoadImageFromScreen | |Load image from screen buffer (screenshot) |
|LoadImageFromTexture |**texture** |Load image from GPU texture data. NOTE: Compressed texture formats not supported |
|ExportImage |**image**, **fileName** |Export image data to file; the file format depends on the fileName extension. Returns true on success |
|ExportImageToMemory |**image**, **fileType** |Export an image to a RawData in the file format named by fileType (e.g. ".png"), or null on failure |
|ExportImageAsCode |**image**, **fileName** |Export image as a C code file (.h) defining an array of bytes. Returns true on success |
|LoadImagePalette |**image**, **colorCount** |Get the distinct colors used in an image, as a list of up to colorCount colors |
|LoadImageRaw |**fileName**, **width**, **height**, **format**, **headerSize** |Load an image from a raw pixel data file, with the given size, pixel format (a PIXELFORMAT_* value), and number of header bytes to skip. Returns null on failure |
|UnloadImageColors |**colors** |Does nothing; provided for compatibility (LoadImageColors returns an ordinary list) |
|UnloadImagePalette |**palette** |Does nothing; provided for compatibility (LoadImagePalette returns an ordinary list) |
|GetImageAlphaBorder |**image**, **threshold** |Get the rectangle bounding the image's non-transparent pixels; threshold is an alpha level from 0.0 to 1.0 |
|GetImageColor |**image**, **x**, **y** |Get image pixel color at (x, y) position |
|ImageAlphaClear |**image**, **color**, **threshold** |Replace pixels with alpha at or below threshold (0.0 to 1.0) with the given color |
|ImageAlphaCrop |**image**, **threshold** |Crop image (in place) to its non-transparent area; threshold is an alpha level from 0.0 to 1.0 |
|ImageAlphaMask |**image**, **alphaMask** |Apply alpha mask to image (in place); the image becomes GRAY_ALPHA or RGBA format. alphaMask should be the same size as image |
|ImageAlphaPremultiply |**image** |Premultiply alpha channel |
|ImageColorReplace |**image**, **color**, **replace** |Modify image color: replace color |
|ImageBlurGaussian |**image**, **blurSize** |Apply Gaussian blur (box blur approximation) to image |
|ImageDither |**image**, **rBpp**, **gBpp**, **bBpp**, **aBpp** |Dither image data to 16bpp or lower (Floyd-Steinberg dithering), given bits per channel for r, g, b, a |
|ImageFormat |**image**, **newFormat** |Convert image data (in place) to desired pixel format (a PIXELFORMAT_* value) |
|ImageFromChannel |**image**, **channel** |Create a grayscale image from a selected channel (0-3 for R, G, B, A) of another image |
|ImageFromImage |**image**, **rec** |Create an image from a rectangular piece of another image |
|ImageKernelConvolution |**image**, **kernel**, **kernelSize**=null |Apply a custom square convolution kernel to image (in place).  kernel is either a list of numbers, row by row (e.g. 9 values for a 3x3 kernel), or a list of rows, like [[1,1,1], [1,0,1], [1,1,1]].  kernelSize is optional; if given, it must equal the total number of values |
|ImageMipmaps |**image** |Generate all mipmap levels for an image (in place); the image's pixel data grows to include them. Works with power-of-two and other sizes |
|ImageResizeCanvas |**image**, **newWidth**, **newHeight**, **offsetX**, **offsetY**, **fill** |Resize canvas (in place) and fill new area with color. NOTE: offset is relative to the top-left corner of the original image |
|ImageRotate |**image**, **degrees** |Rotate image (in place) by the given degrees |
|ImageToPOT |**image**, **fill** |Convert image (in place) to power-of-two size, padding with fill color. Can be useful on OpenGL ES 2.0 (web) |
|ImageDrawCircleLines |**dst**, **centerX**, **centerY**, **radius**, **color** |Draw circle outline within an image |
|ImageDrawCircleLinesV |**dst**, **center**, **radius**, **color** |Draw circle outline within an image (Vector version) |
|ImageDrawCircleGradient |**dst**, **center**, **radius**, **inner**=WHITE, **outer**=BLACK |Draw a gradient-filled circle within an image |
|ImageDrawLineEx |**dst**, **start**, **end**, **thick**, **color** |Draw a line with given thickness within an image |
|ImageDrawLineStrip |**dst**, **points**, **color**=WHITE |Draw a sequence of connected lines within an image, given a list of at least 2 points |
|ImageDrawRectangleV |**dst**, **position**, **size**, **color** |Draw rectangle within an image, given its top-left position and its size (x = width, y = height) as Vector2s |
|ImageDrawTextEx |**dst**, **font**, **text**, **position**, **fontSize**, **spacing**, **tint** |Draw text (custom font) within an image |
|ImageDrawTriangle |**dst**, **v1**, **v2**, **v3**, **color** |Draw triangle within an image |
|ImageDrawTriangleGradient |**dst**, **v1**, **v2**, **v3**, **c1**, **c2**, **c3** |Draw triangle with interpolated colors within an image |
|ImageDrawTriangleFan |**dst**, **points**, **color** |Draw a triangle fan defined by a list of at least 3 points within an image (first point is the center) |
|ImageDrawTriangleLines |**dst**, **v1**, **v2**, **v3**, **color** |Draw triangle outline within an image |
|ImageDrawTriangleStrip |**dst**, **points**, **color** |Draw a triangle strip defined by a list of at least 3 points within an image |
|ImageText |**text**, **fontSize**, **color** |Create an image from text (default font) |
|ImageTextEx |**font**, **text**, **fontSize**, **spacing**, **tint** |Create an image from text (custom font) |
|DrawTextureNPatch |**texture**, **nPatchInfo**, **dest**, **origin**=[0, 0], **rotation**=0, **tint**=WHITE |Draw a texture (or part of it) that stretches or shrinks nicely using n-patch info: a map with source (rectangle), left, top, right, bottom, and layout |
|UpdateTexture |**texture**, **pixels** |Update GPU texture with new pixel data from a RawData. NOTE: pixels must match the texture's format and contain at least as many pixels as the texture |
|UpdateTextureRec |**texture**, **rec**, **pixels** |Update GPU texture rectangle with new pixel data from a RawData. NOTE: pixels must match the texture's format and contain as many pixels as rec; rec must fit completely within the texture |
|LoadTextureCubemap |**image**, **layout** |Load cubemap from image; layout is a CUBEMAP_LAYOUT_* value |

## RLgl

|Name | Parameters | Purpose |
|-----|------------|---------|
|rlSetBlendFactors |**glSrcFactor**, **glDstFactor**, **glEquation** |Set blending mode factor and equation |
|rlSetBlendFactorsSeparate |**glSrcRGB**, **glDstRGB**, **glSrcAlpha**, **glDstAlpha**, **glEqRGB**, **glEqAlpha** |Set blending mode factor and equation separately for RGB and alpha |
|rlMatrixMode |**mode** |Choose the current matrix to be transformed (RL_MODELVIEW or RL_PROJECTION) |
|rlPushMatrix | |Push the current matrix onto the matrix stack |
|rlPopMatrix | |Pop the most recently pushed matrix from the matrix stack |
|rlLoadIdentity | |Reset current matrix to identity matrix |
|rlTranslatef |**x**=0, **y**=0, **z**=0 |Multiply the current matrix by a translation matrix |
|rlRotatef |**angle**=0, **x**=0, **y**=0, **z**=0 |Multiply the current matrix by a rotation of angle degrees about the axis (x, y, z) |
|rlScalef |**x**=1, **y**=1, **z**=1 |Multiply the current matrix by a scaling matrix |
|rlMultMatrixf |**matf** |Multiply the current matrix by another matrix, given as a list of 16 numbers in column-major (OpenGL) order; missing entries are 0 |
|rlFrustum |**left**, **right**, **bottom**, **top**, **znear**, **zfar** |Multiply the current matrix by a perspective matrix generated by parameters |
|rlOrtho |**left**, **right**, **bottom**, **top**, **znear**, **zfar** |Multiply the current matrix by an orthographic matrix generated by parameters |
|rlViewport |**x**, **y**, **width**, **height** |Set the viewport area (transformation from normalized device coordinates to window coordinates) |
|rlSetClipPlanes |**nearPlane**, **farPlane** |Set the near and far clip plane distances |
|rlGetCullDistanceNear | |Get the near clip plane distance |
|rlGetCullDistanceFar | |Get the far clip plane distance |
|rlGetActiveFramebuffer | |Get the id of the framebuffer (fbo) currently bound for drawing, such as a render texture's; 0 for the default framebuffer (the screen) |
|rlEnableBackfaceCulling | |Enable backface culling |
|rlDisableBackfaceCulling | |Disable backface culling |
|rlEnableDepthTest | |Enable depth testing |
|rlDisableDepthTest | |Disable depth testing |
|rlEnableDepthMask | |Enable writing to the depth buffer |
|rlDisableDepthMask | |Disable writing to the depth buffer |
|rlEnableWireMode | |Enable wire mode |
|rlDisableWireMode | |Disable wire mode |
|rlEnableSmoothLines | |Enable line antialiasing |
|rlDisableSmoothLines | |Disable line antialiasing |
|rlSetLineWidth |**width**=1 |Set the width of drawn lines |
|rlGetLineWidth | |Get the line drawing width |
|rlDrawRenderBatchActive | |Draw (flush) the active render batch now |
|rlGetMatrixModelview | |Get internal modelview matrix |
|rlGetMatrixProjection | |Get internal projection matrix |
|rlSetMatrixProjection |**proj** |Set a custom projection matrix (replaces internal projection matrix) |
|rlSetMatrixModelview |**view** |Set a custom modelview matrix (replaces internal modelview matrix) |
|rlGetMatrixTransform | |Get internal accumulated transform matrix |
|rlGetMatrixProjectionStereo |**eye**=0 |Get internal projection matrix for stereo render (selected eye) |
|rlGetMatrixViewOffsetStereo |**eye**=0 |Get internal view offset matrix for stereo render (selected eye) |
|rlSetMatrixProjectionStereo |**right**, **left** |Set eyes projection matrices for stereo rendering |
|rlSetMatrixViewOffsetStereo |**right**, **left** |Set eyes view offsets matrices for stereo rendering |
|rlBegin |**mode**=RL_TRIANGLES |Begin defining vertices, organized as mode (RL_LINES, RL_TRIANGLES or RL_QUADS); finish with rlEnd |
|rlEnd | |Finish defining vertices begun with rlBegin |
|rlVertex2i |**x**=0, **y**=0 |Add one vertex at (x, y), using the current color, texture coordinate and normal |
|rlVertex2f |**x**=0, **y**=0 |Add one vertex at (x, y), using the current color, texture coordinate and normal |
|rlVertex3f |**x**=0, **y**=0, **z**=0 |Add one vertex at (x, y, z), using the current color, texture coordinate and normal |
|rlTexCoord2f |**x**=0, **y**=0 |Set the texture coordinate for the following vertices |
|rlNormal3f |**x**=0, **y**=0, **z**=1 |Set the normal for the following vertices |
|rlColor4ub |**r**=255, **g**=255, **b**=255, **a**=255 |Set the color (components 0-255) for the following vertices |
|rlColor3f |**x**=1, **y**=1, **z**=1 |Set the color (r, g, b as x, y, z, each 0-1; alpha 1) for the following vertices |
|rlColor4f |**x**=1, **y**=1, **z**=1, **w**=1 |Set the color (r, g, b, a as x, y, z, w, each 0-1) for the following vertices |
|rlSetTexture |**id**=0 |Set the texture id used by the following vertices (0 for none) |
|rlCheckRenderBatchLimit |**vCount** |Check whether adding vCount vertices would overflow the active render batch, and if so draw (flush) it; returns true if it was flushed |
|rlEnableColorBlend | |Enable color blending |
|rlDisableColorBlend | |Disable color blending |
|rlColorMask |**r**=1, **g**=1, **b**=1, **a**=1 |Choose which color channels drawing writes to (true to write r, g, b and a) |
|rlSetCullFace |**mode**=RL_CULL_FACE_BACK |Set which faces are culled (RL_CULL_FACE_BACK or RL_CULL_FACE_FRONT) |
|rlEnableScissorTest | |Enable the scissor test |
|rlDisableScissorTest | |Disable the scissor test |
|rlScissor |**x**, **y**, **width**, **height** |Set the scissor rectangle, in framebuffer pixels from the bottom left (used while the scissor test is enabled) |
|rlEnablePointMode | |Enable point mode |
|rlDisablePointMode | |Disable point mode |
|rlEnableStereoRender | |Enable stereo rendering |
|rlDisableStereoRender | |Disable stereo rendering |
|rlIsStereoRenderEnabled | |Check if stereo render is enabled |
|rlClearColor |**r**=0, **g**=0, **b**=0, **a**=255 |Set the color (components 0-255) used to clear the screen |
|rlClearScreenBuffers | |Clear used screen buffers (color and depth) |
|rlCheckErrors | |Check and log OpenGL error codes |
|rlSetBlendMode |**mode**=RL_BLEND_ALPHA |Set the blend mode (e.g. RL_BLEND_ALPHA) |
|rlLoadDrawCube | |Load and draw a cube in normalized device coordinates |
|rlLoadDrawQuad | |Load and draw a quad in normalized device coordinates |
|rlGetVersion | |Get the current OpenGL version, as an RL_OPENGL_ constant (e.g. RL_OPENGL_33) |
|rlSetFramebufferWidth |**width** |Set current framebuffer width |
|rlGetFramebufferWidth | |Get default framebuffer width |
|rlSetFramebufferHeight |**height** |Set current framebuffer height |
|rlGetFramebufferHeight | |Get default framebuffer height |
|rlGetTextureIdDefault | |Get the id of rlgl's default texture (1x1 white) |
|rlGetShaderIdDefault | |Get the id of rlgl's default shader |
|rlGetShaderLocsDefault | |Get a list of the default shader's locations (a copy; changing it has no effect) |
|rlGetPixelFormatName |**format** |Get the name of a PIXELFORMAT_ constant (e.g. "R8G8B8A8") |
|rlActiveTextureSlot |**slot**=0 |Select and activate a texture slot |
|rlEnableTexture |**id** |Enable texture |
|rlDisableTexture | |Disable texture |
|rlEnableTextureCubemap |**id** |Enable texture cubemap |
|rlDisableTextureCubemap | |Disable texture cubemap |
|rlTextureParameters |**id**, **param**, **value** |Set texture parameters (wrap mode/filter mode) |
|rlCubemapParameters |**id**, **param**, **value** |Set cubemap parameters (wrap mode/filter mode) |
|rlEnableShader |**id** |Enable shader program |
|rlDisableShader | |Disable shader program |
|rlEnableFramebuffer |**id** |Enable rendering to texture (fbo) |
|rlDisableFramebuffer | |Disable rendering to texture |
|rlBindFramebuffer |**target**, **framebuffer** |Bind framebuffer object (fbo) |
|rlBlitFramebuffer |**srcX**, **srcY**, **srcWidth**, **srcHeight**, **dstX**, **dstY**, **dstWidth**, **dstHeight**, **bufferMask**=0x4000 |Copy a rectangle of the read framebuffer to the draw framebuffer (see rlBindFramebuffer); bufferMask: 0x4000 color (default), 0x0100 depth, 0x0400 stencil |
|rlActiveDrawBuffers |**count**=1 |Activate count color attachments (up to 8) as draw buffers, for multiple render targets; one is active by default |
|rlLoadRenderBatch |**numBuffers**=RL_DEFAULT_BATCH_BUFFERS, **bufferElements**=2048, **bufferElements**=RL_DEFAULT_BATCH_BUFFER_ELEMENTS |Create a custom render batch with numBuffers buffers of bufferElements quads each; returns a batch map, or null on failure.  Unload with rlUnloadRenderBatch |
|rlUnloadRenderBatch |**batch** |Unload a render batch made by rlLoadRenderBatch (switching back to the default batch if it was active) |
|rlDrawRenderBatch |**batch** |Draw a render batch; this resets it and moves on to its next buffer (if it has several) |
|rlSetRenderBatchActive |**batch** |Set the render batch rlgl draws into (null for the default batch) |
|rlEnableVertexArray |**vaoId** |Enable (bind) a vertex array object (VAO); returns true if VAOs are supported |
|rlDisableVertexArray | |Disable vertex array object (VAO) |
|rlEnableVertexBuffer |**id** |Enable vertex buffer (VBO) |
|rlDisableVertexBuffer | |Disable vertex buffer (VBO) |
|rlEnableVertexBufferElement |**id** |Enable vertex buffer element (VBO element) |
|rlDisableVertexBufferElement | |Disable vertex buffer element (VBO element) |
|rlEnableVertexAttribute |**index** |Enable vertex attribute index |
|rlDisableVertexAttribute |**index** |Disable vertex attribute index |
|rlLoadVertexArray | |Create a vertex array object (VAO), if supported; returns its id, or 0 |
|rlLoadVertexBuffer |**buffer**, **size**=0, **dynamic**=0 |Create a vertex buffer object (VBO) from a RawData and return its id.  buffer may be null to allocate size bytes without filling them; size <= 0 uses the whole RawData.  dynamic hints that the data will be updated often |
|rlLoadVertexBufferElement |**buffer**, **size**=0, **dynamic**=0 |Create an element (index) buffer from a RawData and return its id.  buffer may be null to allocate size bytes without filling them; size <= 0 uses the whole RawData.  dynamic hints that the data will be updated often |
|rlUpdateVertexBuffer |**bufferId**, **data**, **dataSize**=0, **offset**=0 |Update a vertex buffer with data from a RawData, starting offset bytes into the buffer; dataSize (in bytes) <= 0 uses the whole RawData |
|rlUpdateVertexBufferElements |**id**, **data**, **dataSize**=0, **offset**=0 |Update an element (index) buffer with data from a RawData, starting offset bytes into the buffer; dataSize (in bytes) <= 0 uses the whole RawData |
|rlUnloadVertexArray |**vaoId** |Unload vertex array object (VAO) |
|rlUnloadVertexBuffer |**vboId** |Unload vertex buffer (VBO) |
|rlSetVertexAttribute |**index**, **compSize**, **type**=RL_FLOAT, **normalized**=0, **stride**=0, **offset**=0 |Describe vertex attribute index of the bound vertex buffer: compSize components of type (e.g. RL_FLOAT), whether normalized, and stride and offset in bytes |
|rlSetVertexAttributeDivisor |**index**, **divisor** |Set vertex attribute divisor (for instanced drawing) |
|rlSetVertexAttributeDefault |**locIndex**, **value**, **attribType**=RL_SHADER_ATTRIB_FLOAT, **count**=0 |Set the default value of a vertex attribute (used when no buffer feeds it); value is a number, vector map, list of numbers, or RawData of floats; count <= 0 uses the number of components in attribType |
|rlDrawVertexArray |**offset**=0, **count** |Draw count vertices of the bound vertex array as triangles, starting at vertex offset |
|rlDrawVertexArrayElements |**offset**=0, **count**, **buffer**=0 |Draw count indices of the bound element buffer (16-bit indices) as triangles, starting at index offset; buffer must be 0 |
|rlDrawVertexArrayInstanced |**offset**=0, **count**, **instances**=1 |Draw count vertices of the bound vertex array as triangles, starting at vertex offset, instances times |
|rlDrawVertexArrayElementsInstanced |**offset**=0, **count**, **buffer**=0, **instances**=1 |Draw count indices of the bound element buffer (16-bit indices) as triangles, starting at index offset, instances times; buffer must be 0 |
|rlLoadTexture |**data**, **width**, **height**, **format**=RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, **mipmapCount**=1 |Create a width x height texture in the given pixel format from data (a RawData holding each mipmap level in turn, or null for an empty texture); returns its id, or 0 on failure |
|rlLoadTextureDepth |**width**, **height**, **useRenderBuffer**=0 |Create a depth texture (or, if useRenderBuffer, a depth renderbuffer) to attach to a framebuffer; returns its id |
|rlLoadTextureCubemap |**data**, **size**, **format**=RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, **mipmapCount**=1 |Create a cubemap texture with size x size faces; data is a RawData holding, for each mipmap level, all 6 faces (+X, -X, +Y, -Y, +Z, -Z), or null for an empty cubemap.  Returns its id, or 0 on failure |
|rlUpdateTexture |**id**, **offsetX**, **offsetY**, **width**, **height**, **format**, **data** |Update a width x height region of a loaded texture, at offsetX, offsetY, with pixel data from a RawData; format should match the texture's own format (this can't be checked) |
|rlGetGlTextureFormats |**format** |Get the OpenGL formats and data type for a raylib pixel format, as a map with glInternalFormat, glFormat and glType |
|rlUnloadTexture |**id** |Unload texture from GPU memory |
|rlGenTextureMipmaps |**id**, **width**, **height**, **format**=RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 |Generate mipmaps for a texture on the GPU; returns the resulting number of mipmap levels |
|rlReadTexturePixels |**id**, **width**, **height**, **format**=RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 |Read a texture's pixel data as a RawData; null on failure or if width or height < 1 |
|rlReadScreenPixels |**width**, **height** |Read a width x height region from the bottom left of the current framebuffer (normally the whole screen) as a RawData of RGBA bytes, top row first; null if width or height < 1 |
|rlLoadFramebuffer | |Create an empty framebuffer object; returns its id |
|rlFramebufferAttach |**fboId**, **texId**, **attachType**=RL_ATTACHMENT_COLOR_CHANNEL0, **texType**=RL_ATTACHMENT_TEXTURE2D, **mipLevel**=0 |Attach a texture or renderbuffer to a framebuffer object.  attachType: an RL_ATTACHMENT_ constant (COLOR_CHANNEL0-7, DEPTH or STENCIL); texType: RL_ATTACHMENT_TEXTURE2D, RL_ATTACHMENT_RENDERBUFFER, or an RL_ATTACHMENT_CUBEMAP_ side |
|rlFramebufferComplete |**id** |Check whether a framebuffer is complete (ready to draw to) |
|rlUnloadFramebuffer |**id** |Unload a framebuffer from GPU memory; its depth texture or renderbuffer (if any) is also deleted, but color attachments are not |
|rlLoadShader |**code**, **type**=RL_FRAGMENT_SHADER |Compile one shader stage from source (type: RL_VERTEX_SHADER, RL_FRAGMENT_SHADER or RL_COMPUTE_SHADER); returns its id.  On web, a desktop GLSL #version line is adapted for WebGL |
|rlLoadShaderProgram |**vsCode**="", **fsCode**="" |Compile and link a shader program from vertex and fragment shader source; either may be empty (or null) to use rlgl's default shader for that stage.  Returns the program id, or 0 on failure.  On web, a desktop GLSL #version line is adapted for WebGL |
|rlLoadShaderProgramEx |**vsId**, **fsId** |Link a shader program from already-compiled vertex and fragment shader ids; returns its id |
|rlUnloadShader |**id** |Delete a compiled shader stage (from rlLoadShader) |
|rlUnloadShaderProgram |**id** |Unload shader program |
|rlGetLocationUniform |**id**, **uniformName** |Get the location of a uniform in shader program id; -1 if not found |
|rlGetLocationAttrib |**id**, **attribName** |Get the location of a vertex attribute in shader program id; -1 if not found |
|rlSetUniform |**locIndex**, **value**, **uniformType**=RL_SHADER_UNIFORM_FLOAT, **count**=0 |Set a uniform of the shader enabled with rlEnableShader; value may be a number, vector map, list (flat or of vectors), or RawData; count <= 0 infers it from value |
|rlSetUniformMatrix |**locIndex**, **mat** |Set a matrix uniform of the shader enabled with rlEnableShader |
|rlSetUniformMatrices |**locIndex**, **mats** |Set a matrix-array uniform of the shader enabled with rlEnableShader from a list of matrices |
|rlSetUniformSampler |**locIndex**, **textureId** |Set a sampler uniform of the shader enabled with rlEnableShader to a texture id |
|rlSetShader |**id**, **locs** |Set the shader the render batch draws with: program id, and locs as a list of locations (missing ones are -1) or null for the default shader's |

## RText

|Name | Parameters | Purpose |
|-----|------------|---------|
|LoadFont |**fileName** |Load a Font from file into GPU memory (VRAM).  Returns null on failure |
|LoadFontEx |**fileName**, **fontSize**=20, **codepoints**=null, **codepointCount**=0 |Load a font from file at fontSize (in pixels of height), with the given codepoints (a list of ints or a string of characters); null codepoints loads the default character set (32..126).  codepointCount is ignored.  Returns null on failure |
|LoadFontFromImage |**image**, **key**=Color{255, 0, 255, 255}, **firstChar**=32 |Load a font from an Image (XNA style): glyphs separated by key color, starting at firstChar (a codepoint or a one-character string) |
|IsFontValid |**font** |Check if font is valid (font data loaded).  WARNING: GPU texture not checked |
|UnloadFont |**font** |Unload Font from GPU memory (VRAM) |
|MakeFont |**baseSize**, **glyphs**, **recs**, **texture**, **glyphPadding**=0 |Build a Font from its parts, e.g. for an SDF font.  glyphs is a list of maps (value, offsetX, offsetY, advanceX, image), recs a matching list of each glyph's rectangle in texture.  The font takes over texture, so UnloadFont unloads it; glyph images are copied.  Returns null if the lists are empty or differ in length, or the font is not valid |
|DrawFPS |**posX**=0, **posY**=0 |Draw current FPS, using the default font |
|DrawText |**text**, **posX**=0, **posY**=0, **fontSize**=20, **color**=BLACK |Draw text using the default font.  If fontSize is smaller than the font's base size (10), the base size is used.  Character spacing is proportional to fontSize |
|DrawTextEx |**font**, **text**, **position**=[0, 0], **fontSize**=20, **spacing**=0, **tint**=BLACK |Draw text using Font.  Character spacing is NOT proportional to fontSize |
|DrawTextPro |**font**, **text**, **position**=[0, 0], **origin**=[0, 0], **rotation**=0, **fontSize**=20, **spacing**=0, **tint**=BLACK |Draw text using Font, rotated (in degrees) around origin |
|DrawTextCodepoint |**font**, **codepoint**, **position**=[0, 0], **fontSize**=20, **tint**=BLACK |Draw one character, given as an int codepoint |
|MeasureText |**text**, **fontSize**=20 |Measure text width in pixels, for the default font |
|MeasureTextEx |**font**, **text**, **fontSize**=20, **spacing**=0 |Measure text size for Font; returns a map with x (width) and y (height) |
|MeasureTextCodepoints |**font**, **codepoints**, **fontSize**=20, **spacing**=0 |Measure size of codepoints (a list of ints or a string) for Font; returns a map with x (width) and y (height) |
|GetGlyphIndex |**font**, **codepoint** |Get the glyph index of a Unicode codepoint in font.  If codepoint is not found in the font, it falls back to '?' |
|GetFontDefault | |Get the default font |
|SetTextLineSpacing |**spacing** |Set vertical line spacing when drawing with line-breaks |
|GetGlyphAtlasRec |**font**, **codepoint** |Get glyph rectangle in font atlas for a codepoint (Unicode character).  If codepoint is not found in the font, it falls back to '?' |
|GetGlyphInfo |**font**, **codepoint** |Get glyph info for a codepoint (Unicode character), as a map with value, offsetX, offsetY, advanceX and image.  If codepoint is not found in the font, it falls back to '?' |
|GetCodepointCount |**text** |Get the number of characters (codepoints) in text.  An invalid UTF-8 sequence counts as one '?' (0x3f) codepoint |
|CodepointToUTF8 |**codepoint** |Get the character for a Unicode codepoint, as a string |
|TextIsEqual |**text1**, **text2** |Check if two text strings are equal |
|TextLength |**text** |Get the length of a string in bytes (of UTF-8), not characters |
|TextCopy |**dst**, **src** |Returns src unchanged (dst is ignored, since MiniScript strings cannot be modified) |
|LoadFontFromMemory |**fileType**, **fileData**, **fontSize**, **codepoints**=null, **codepointCount**=0 |Load a font from a RawData buffer.  fileType is the file extension, e.g. ".ttf".  codepoints is a list of ints or a string (null for the default set, 32..126); codepointCount is ignored.  Returns null on failure |
|LoadFontData |**fileData**, **fontSize**, **codepoints**=null, **codepointCount**=0, **type**=0 |Load glyph data from TTF font data (a RawData), for use with GenImageFontAtlas or MakeFont.  codepoints is a list of ints or a string (null for 32..126); codepointCount is ignored.  type: 0 = default, 1 = bitmap, 2 = SDF.  Returns a list of glyph maps (value, offsetX, offsetY, advanceX, image); release the images with UnloadFontData |
|UnloadFontData |**glyphs** |Unload the glyph images in a list of glyph maps returned by LoadFontData |
|LoadCodepoints |**text** |Get all codepoints in a text string, as a list of ints |
|UnloadCodepoints |**codepoints** |Does nothing (codepoint lists are freed automatically); kept for raylib compatibility |
|LoadUTF8 |**codepoints** |Encode codepoints (a list of ints) into a UTF-8 string |
|UnloadUTF8 |**text** |Does nothing (strings are freed automatically); kept for raylib compatibility |
|DrawTextCodepoints |**font**, **codepoints**, **position**=[0, 0], **fontSize**=20, **spacing**=0, **tint**=BLACK |Draw multiple characters, given as a list of int codepoints or a string |
|GenImageFontAtlas |**glyphs**, **glyphRecs**, **fontSize**, **padding**, **packMethod** |Generate a font atlas Image from glyphs (a list of glyph maps, as from LoadFontData).  glyphRecs is a list of the same length, which is filled with each glyph's rectangle in the atlas.  packMethod: 0 = default, 1 = Skyline.  Returns null if the lists are empty or differ in length |
|TextFormat |**text**, **args**=[] |Format text, replacing each %s, %d (or %i) and %f with the next value from args, and %% with % |
|TextFindIndex |**text**, **search** |Find first occurrence of search within text; returns its byte index, or -1 if not found |
|GetTextBetween |**text**, **begin**, **end** |Get the text between the first occurrence of begin and the next end after it ("" if not found).  Result is limited to 1023 bytes |
|TextReplace |**text**, **search**, **replacement** |Replace all occurrences of search in text with replacement.  Result is limited to 1023 bytes; use TextReplaceAlloc for longer text |
|TextReplaceAlloc |**text**, **search**, **replacement** |Replace all occurrences of search in text with replacement, with no length limit |
|TextReplaceBetween |**text**, **begin**, **end**, **replacement** |Replace the text between begin and end (keeping begin and end) with replacement; an empty or null replacement just removes it.  Result is limited to 1023 bytes; use TextReplaceBetweenAlloc for longer text |
|TextReplaceBetweenAlloc |**text**, **begin**, **end**, **replacement** |Replace the text between begin and end (keeping begin and end) with replacement, with no length limit; an empty or null replacement just removes it |
|TextInsert |**text**, **insert**, **position** |Insert text at a byte position, moving the rest forward.  Result is limited to 1023 bytes; use TextInsertAlloc for longer text |
|TextSubtext |**text**, **position**=0, **length**=0 |Get part of a string: length bytes from byte position (bytes of UTF-8, not characters) |
|TextRemoveSpaces |**text** |Remove all the spaces from a string.  Result is limited to 1023 bytes |
|TextInsertAlloc |**text**, **insert**, **position** |Insert text at a byte position, moving the rest forward, with no length limit |
|TextSplit |**text**, **delimiter** |Split text at each occurrence of delimiter (its first character only) into a list of at most 128 strings; null if delimiter is empty |
|TextJoin |**textList**, **delimiter**="" |Join a list of strings with delimiter.  Result is limited to 1023 bytes |
|TextAppend |**text**, **append** |Returns text with append added to the end |
|TextToUpper |**text** |Get upper case version of provided string.  Only basic (ASCII) letters are converted; result is limited to 1023 bytes |
|TextToLower |**text** |Get lower case version of provided string.  Only basic (ASCII) letters are converted; result is limited to 1023 bytes |
|TextToPascal |**text** |Get PascalCase version of provided string (from snake_case).  Only basic (ASCII) characters are handled; result is limited to 1023 bytes |
|TextToSnake |**text** |Get snake_case version of provided string.  Only basic (ASCII) characters are handled; result is limited to 1023 bytes |
|TextToCamel |**text** |Get camelCase version of provided string (from snake_case).  Only basic (ASCII) characters are handled; result is limited to 1023 bytes |
|TextToInteger |**text** |Get integer value from text |
|TextToFloat |**text** |Get float value from text.  Only '.' is understood as decimal point |
|LoadTextLines |**text** |Split text into a list of lines, at each line feed |
|UnloadTextLines |**lines** |Does nothing (lists are freed automatically); kept for raylib compatibility |

## RCore

|Name | Parameters | Purpose |
|-----|------------|---------|
|BeginDrawing | |Begin canvas (framebuffer) drawing |
|EndDrawing | |End canvas (framebuffer) drawing and swap buffers (double buffering) |
|ClearBackground |**color**=BLACK |Clear background (framebuffer) to color (default BLACK) |
|BeginMode3D |**camera** |Begin 3D mode with a custom camera (Camera3D map) |
|EndMode3D | |End 3D mode and return to default 2D orthographic mode |
|GetScreenToWorldRay |**position**, **camera** |Get a ray trace from a screen position (e.g. mouse) |
|GetScreenToWorldRayEx |**position**, **camera**, **width**=0, **height**=0 |Get a ray trace from a screen position (e.g. mouse) within a viewport of the given width and height (0 means the screen width/height) |
|GetWorldToScreen |**position**, **camera** |Get screen space position from a 3D world space position |
|GetWorldToScreenEx |**position**, **camera**, **width**=0, **height**=0 |Get screen space position for a 3D world space position, within a viewport of the given width and height (0 means the screen width/height); useful for texture drawing |
|GetCameraMatrix |**camera** |Get transform matrix for camera |
|UpdateCamera |**camera**, **mode**=CAMERA_CUSTOM |Update camera position for the selected mode: CAMERA_FREE, CAMERA_FIRST_PERSON, CAMERA_THIRD_PERSON, CAMERA_ORBITAL or CAMERA_CUSTOM (default). Updates the camera map in place |
|UpdateCameraPro |**camera**, **movement**=[0, 0, 0], **rotation**=[0, 0, 0], **zoom**=0 |Update camera by the given movement and rotation (Vector3s) and zoom, all provided by the caller. Updates the camera map in place |
|GetCameraForward |**camera** |Returns the camera's forward vector (normalized) |
|GetCameraUp |**camera** |Returns the camera's up vector (normalized). Note: the up vector might not be perpendicular to the forward vector |
|GetCameraRight |**camera** |Returns the camera's right vector (normalized) |
|CameraMoveForward |**camera**, **distance**, **moveInWorldPlane**=1 |Moves the camera in its forward direction (in the world plane if moveInWorldPlane is true). Updates the camera map in place |
|CameraMoveUp |**camera**, **distance** |Moves the camera in its up direction. Updates the camera map in place |
|CameraMoveRight |**camera**, **distance**, **moveInWorldPlane**=1 |Moves the camera in its current right direction (in the world plane if moveInWorldPlane is true). Updates the camera map in place |
|CameraMoveToTarget |**camera**, **delta** |Moves the camera position closer to/farther from the camera target. Updates the camera map in place |
|CameraYaw |**camera**, **angle**, **rotateAroundTarget**=0 |Rotates the camera around its up vector. Yaw is "looking left and right". If rotateAroundTarget is false, the camera rotates around its position. Angle is in radians. Updates the camera map in place |
|CameraPitch |**camera**, **angle**, **lockView**=1, **rotateAroundTarget**=0, **rotateUp**=0 |Rotates the camera around its right vector; pitch is "looking up and down". lockView prevents camera overrotation (aka "somersaults"); rotateAroundTarget defines if rotation is around target or around its position; rotateUp rotates the up direction as well (typically only useful in CAMERA_FREE). Angle is in radians. Updates the camera map in place |
|CameraRoll |**camera**, **angle** |Rotates the camera around its forward vector. Roll is "turning your head sideways to the left or right". Angle is in radians. Updates the camera map in place |
|GetCameraViewMatrix |**camera** |Returns the camera view matrix |
|GetCameraProjectionMatrix |**camera**, **aspect**=0 |Returns the camera projection matrix for the given aspect ratio (0 means the screen's aspect ratio) |
|LoadShader |**vsFileName**="", **fsFileName**="" |Load shader from files and bind default locations. If a filename is empty, the default vertex/fragment shader is used. Returns null on failure |
|LoadShaderFromMemory |**vsCode**="", **fsCode**="" |Load shader from code strings and bind default locations. If a code string is empty, the default vertex/fragment shader is used. Returns null on failure |
|IsShaderValid |**shader** |Check if shader is valid (loaded on GPU) |
|BeginShaderMode |**shader** |Begin custom shader mode |
|EndShaderMode | |End custom shader mode (returns to default shader) |
|GetShaderLocation |**shader**, **uniformName** |Get shader uniform location |
|GetShaderLocationAttrib |**shader**, **attribName** |Get shader attribute location |
|GetShaderLoc |**shader**, **locIndex** |Get one entry of a shader's location table (SHADER_LOC_MATRIX_MODEL, SHADER_LOC_VECTOR_VIEW...): the uniform or attribute location raylib uses for it, or -1 if none |
|SetShaderLoc |**shader**, **locIndex**, **location** |Set one entry of a shader's location table (SHADER_LOC_MATRIX_MODEL, SHADER_LOC_VECTOR_VIEW...), usually to a location from GetShaderLocation, so raylib feeds that uniform or attribute when drawing. Every copy of the shader shares the table |
|SetShaderValueMatrix |**shader**, **locIndex**, **mat** |Set shader uniform value (matrix 4x4) |
|SetShaderValueTexture |**shader**, **locIndex**, **texture** |Set shader uniform value for texture |
|SetShaderValue |**shader**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT |Set shader uniform value. value may be a number, a list of numbers, a vector/color map, or RawData; uniformType is a SHADER_UNIFORM_* constant (default SHADER_UNIFORM_FLOAT) |
|SetShaderValueV |**shader**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT, **count**=0 |Set shader uniform array value: value is a list of numbers, a list of vectors/colors, or RawData. count is the number of elements (0 means infer from the list) |
|UnloadShader |**shader** |Unload shader from GPU memory (VRAM) |
|SetTargetFPS |**fps** |Set target FPS (maximum) |
|GetFrameTime | |Get time in seconds for last frame drawn (delta time) |
|GetTime | |Get elapsed time in seconds since InitWindow() |
|GetFPS | |Get current FPS (averaged over recent frames) |
|IsKeyPressed |**key** |Check if key has been pressed once |
|IsKeyPressedRepeat |**key** |Check if key has been pressed again (key repeat while held down) |
|IsKeyDown |**key** |Check if key is being pressed (key held down) |
|IsKeyReleased |**key** |Check if key has been released once |
|IsKeyUp |**key** |Check if key is NOT being pressed (key not held down) |
|GetKeyPressed | |Get key pressed (keycode); call it multiple times for keys queued. Returns 0 when the queue is empty |
|GetCharPressed | |Get char pressed (Unicode codepoint); call it multiple times for chars queued. Returns 0 when the queue is empty |
|GetKeyName |**key** |Get the character a key types on the current keyboard layout (e.g. "q" for KEY_A on an AZERTY keyboard), or "" if none |
|SetExitKey |**key** |Set a custom key to exit program (default is KEY_ESCAPE; KEY_NULL disables it) |
|IsGamepadAvailable |**gamepad**=0 |Check if gamepad is available |
|GetGamepadName |**gamepad**=0 |Get the internal name of a gamepad, or "" if there is none |
|IsGamepadButtonPressed |**gamepad**=0, **button** |Check if gamepad button has been pressed once |
|IsGamepadButtonDown |**gamepad**=0, **button** |Check if gamepad button is being pressed |
|IsGamepadButtonReleased |**gamepad**=0, **button** |Check if gamepad button has been released once |
|IsGamepadButtonUp |**gamepad**=0, **button** |Check if gamepad button is NOT being pressed |
|GetGamepadButtonPressed | |Get the last gamepad button pressed. Note: returns the last button that went down; releases are not considered |
|GetGamepadAxisCount |**gamepad**=0 |Get gamepad axis count |
|GetGamepadAxisMovement |**gamepad**=0, **axis** |Get movement value (-1 to 1) for a gamepad axis |
|SetGamepadMappings |**mappings** |Set internal gamepad mappings from a string in SDL_GameControllerDB format |
|SetGamepadVibration |**gamepad**=0, **leftMotor**=0.0, **rightMotor**=0.0, **duration**=0.0 |Set gamepad vibration: motor strengths (0 to 1) for the given duration in seconds |
|IsMouseButtonPressed |**button** |Check if mouse button has been pressed once |
|IsMouseButtonDown |**button** |Check if mouse button is being pressed |
|IsMouseButtonReleased |**button** |Check if mouse button has been released once |
|IsMouseButtonUp |**button** |Check if mouse button is NOT being pressed |
|GetMouseX | |Get mouse position X |
|GetMouseY | |Get mouse position Y |
|GetMousePosition | |Get mouse position as a map with x and y |
|GetMouseDelta | |Get mouse delta between frames, as a map with x and y |
|GetMouseWheelMove | |Get mouse wheel movement for X or Y, whichever is larger |
|SetMouseCursor |**cursor** |Set mouse cursor shape (a MOUSE_CURSOR_* constant) |
|ShowCursor | |Show mouse cursor |
|HideCursor | |Hide mouse cursor |
|IsCursorHidden | |Check if cursor is not visible |
|IsCursorOnScreen | |Check if cursor is on the current screen |
|SetWindowTitle |**caption**="raylib-miniscript" |Set title for window |
|SetWindowIcon |**image** |Set icon for window. Image must be in RGBA format, 8 bits per channel; it is scaled by the OS for all required sizes |
|SetWindowIcons |**images**, **count**=0 |Set icon for window from a list of images (count limits how many are used; 0 means all). Images must be in RGBA format, 8 bits per channel; the OS picks among them by size. Standard Windows icon sizes: 256, 128, 96, 64, 48, 32, 24, 16. Not supported on web |
|GetScreenWidth | |Get current screen width |
|GetScreenHeight | |Get current screen height |
|GetRenderWidth | |Get current render width, which is equal to screen width * DPI scale |
|GetRenderHeight | |Get current render height, which is equal to screen height * DPI scale |
|InitWindow |**width**=960, **height**=640, **title**="raylib-miniscript" |Initialize window and OpenGL context |
|CloseWindow | |Close window and unload OpenGL context |
|WindowShouldClose | |Check if application should close (by default, KEY_ESCAPE pressed or window close icon clicked). Always false on web |
|IsWindowFullscreen | |Check if window is currently fullscreen |
|IsWindowHidden | |Check if window is currently hidden. Not supported on web |
|IsWindowMinimized | |Check if window has been minimized. Not supported on web |
|IsWindowMaximized | |Check if window has been maximized. Not supported on web |
|IsWindowResized | |Check if window has been resized last frame. Not supported on web |
|IsWindowState |**flags** |Check if one specific window flag (FLAG_*) is enabled. Not supported on web |
|SetWindowState |**flags** |Set window configuration state using flags (FLAG_*). Not supported on web |
|ClearWindowState |**flags** |Clear window configuration state flags (FLAG_*). Not supported on web |
|ToggleFullscreen | |Toggle fullscreen mode |
|ToggleBorderlessWindowed | |Toggle borderless windowed mode. Not supported on web |
|MaximizeWindow | |Set window state: maximized, if resizable. Not supported on web |
|MinimizeWindow | |Set window state: minimized. Not supported on web |
|RestoreWindow | |Restore window from being minimized/maximized. Not supported on web |
|SetWindowPosition |**x**, **y** |Set window position on screen (windowed mode). Not supported on web |
|SetWindowMonitor |**monitor** |Set monitor for the current window. Not supported on web |
|SetWindowMinSize |**width**, **height** |Set window minimum dimensions (for FLAG_WINDOW_RESIZABLE). Not supported on web |
|SetWindowMaxSize |**width**, **height** |Set window maximum dimensions (for FLAG_WINDOW_RESIZABLE). Not supported on web |
|SetWindowSize |**width**, **height** |Set window dimensions. Not supported on web |
|SetWindowOpacity |**opacity** |Set window opacity, from 0.0 to 1.0. Not supported on web |
|SetWindowFocused | |Set window focused. Not supported on web |
|GetWindowHandle | |Get native window handle, as a number (for display/identification only). Not supported on web |
|GetMonitorCount | |Get number of monitors. Not supported on web |
|GetCurrentMonitor | |Get current monitor where window is placed. Not supported on web |
|GetMonitorPosition |**monitor**=0 |Get selected monitor position, as a map with x and y. Not supported on web |
|GetMonitorWidth |**monitor**=0 |Get selected monitor's current video mode width. Not supported on web |
|GetMonitorHeight |**monitor**=0 |Get selected monitor's current video mode height. Not supported on web |
|GetMonitorPhysicalWidth |**monitor**=0 |Get selected monitor physical width in millimetres. Not supported on web |
|GetMonitorPhysicalHeight |**monitor**=0 |Get selected monitor physical height in millimetres. Not supported on web |
|GetMonitorRefreshRate |**monitor**=0 |Get selected monitor refresh rate. Not supported on web |
|GetMonitorName |**monitor**=0 |Get the human-readable name of a monitor ("" on web) |
|GetWindowPosition | |Get window position on monitor, as a map with x and y. Not supported on web |
|GetWindowScaleDPI | |Get window scale DPI factor for current monitor, as a map with x and y. Not supported on web |
|IsWindowFocused | |Check if window has the focus |
|IsWindowReady | |Check if window has been initialized successfully |
|GetMouseWheelMoveV | |Get mouse wheel movement X/Y, as a map with x and y |
|SetMousePosition |**x**, **y** |Set mouse position XY |
|SetMouseOffset |**offsetX**, **offsetY** |Set mouse offset (useful when rendering to different size targets) |
|SetMouseScale |**scaleX**, **scaleY** |Set mouse scaling (useful when rendering to different size targets) |
|EnableCursor | |Enable cursor (unlock cursor) |
|DisableCursor | |Disable cursor (lock cursor) |
|GetTouchX | |Get touch position X for touch point 0 (relative to screen size) |
|GetTouchY | |Get touch position Y for touch point 0 (relative to screen size) |
|GetTouchPosition |**index**=0 |Get touch position for a touch point index (relative to screen size), as a map with x and y |
|GetTouchPointId |**index**=0 |Get touch point identifier for provided index |
|GetTouchPointCount | |Get number of touch points |
|SetGesturesEnabled |**flags** |Enable only desired gestures to be detected |
|IsGestureDetected |**gesture** |Check if a gesture (GESTURE_* constant) has been detected |
|GetGestureDetected | |Get latest detected gesture (a GESTURE_* constant) |
|GetGestureHoldDuration | |Get gesture hold time in seconds |
|GetGestureDragVector | |Get drag vector (from initial touch point to current), as a map with x and y |
|GetGestureDragAngle | |Get drag angle in degrees (horizontal-right is 0, counterclockwise) |
|GetGesturePinchVector | |Get the vector between two pinch points, as a map with x and y |
|GetGesturePinchAngle | |Get angle between two pinch points, in degrees (horizontal-right is 0, counterclockwise) |
|BeginMode2D |**camera** |Begin 2D mode with a custom camera: a map with offsetX, offsetY, targetX, targetY, rotation (default 0) and zoom (default 1) |
|EndMode2D | |End 2D mode with custom camera |
|GetCameraMatrix2D |**camera** |Get 2D camera transform matrix (camera map as for BeginMode2D) |
|GetWorldToScreen2D |**position**, **camera** |Get screen space position for a 2D camera world space position, as a map with x and y |
|GetScreenToWorld2D |**position**, **camera** |Get world space position for a 2D camera screen space position, as a map with x and y |
|BeginBlendMode |**mode** |Begin blending mode (a BLEND_* constant: alpha, additive, multiplied, subtract, custom...) |
|EndBlendMode | |End blending mode (reset to default: alpha blending) |
|BeginScissorMode |**x**, **y**, **width**, **height** |Begin scissor mode (define screen area for following drawing); x and y are the upper-left corner |
|EndScissorMode | |End scissor mode |
|BeginVrStereoMode |**config** |Begin VR drawing configuration. Not supported on web |
|EndVrStereoMode | |End VR drawing process (and desktop mirror). Not supported on web |
|LoadVrStereoConfig |**device** |Load VR stereo config for VR simulator device parameters (a VrDeviceInfo map). Not supported on web |
|UnloadVrStereoConfig |**config** |Unload VR stereo config properties. Not supported on web |
|OpenURL |**url** |Open a URL in the default browser. Refused (does nothing) once sandboxed |
|SetClipboardText |**text** |Set clipboard text content |
|GetClipboardText | |Get the clipboard's text content |
|EnableEventWaiting | |Enable waiting for events on EndDrawing(), no automatic event polling. Not supported on web |
|DisableEventWaiting | |Disable waiting for events on EndDrawing(), automatic events polling. Not supported on web |
|GetClipboardImage | |Get clipboard image (desktop only) |
|IsFileExtension |**fileName**, **ext** |Check file extension, including the dot (e.g. ".png"); ext may list several, like ".png;.jpg" |
|TakeScreenshot |**fileName** |Take a screenshot of current screen and save it to a file; the file format depends on the fileName extension (e.g. .png) |
|SetConfigFlags |**flags** |Set up window configuration flags (FLAG_*). Call this before InitWindow, since it sets up flags for the window creation process; to change window states after creation, use SetWindowState() |
|EncodeDataBase64 |**data**, **dataSize** |Encode data (a string or RawData) to a Base64 string. If dataSize > 0, only that many bytes are encoded |
|WaitTime |**seconds**=1.0 |Wait for some time in seconds (default 1), halting program execution |
|SwapScreenBuffer | |Swap back buffer with front buffer (screen drawing). Not supported on web |
|PollInputEvents | |Register all input events. Not supported on web |
|LoadFileText |**fileName** |Load text data from file as a string, or null if it can't be read |
|LoadFileData |**fileName** |Load a file's contents as a RawData, or null if it can't be read |
|UnloadFileData |**data** |Release the buffer of a RawData from LoadFileData(), leaving it empty. Not needed: the RawData is freed automatically. Not supported on web |
|SaveFileData |**fileName**, **data**, **dataSize**=0 |Save data (RawData, string, or list of byte values) to file; if dataSize > 0, only that many bytes are written. Returns true on success. Not supported on web |
|ExportDataAsCode |**data**, **dataSize**, **fileName** |Export data (RawData, string, or list of byte values) to code (.h); if dataSize > 0, only that many bytes are exported. Returns true on success. Not supported on web |
|UnloadFileText |**text** |Does nothing; LoadFileText() returns an ordinary string, which needs no unloading |
|SaveFileText |**fileName**, **text** |Save text data to file (write). Returns true on success. Not supported on web |
|SetLoadFileDataCallback |**callback**=null |Set custom file binary data loader: a function(fileName) that returns the file's data (RawData, string, or list of byte values), or null to clear. Refused once sandboxed; not supported on web |
|SetSaveFileDataCallback |**callback**=null |Set custom file binary data saver: a function(fileName, data, dataSize), where data is a RawData, that returns true on success; or null to clear. Refused once sandboxed; not supported on web |
|SetLoadFileTextCallback |**callback**=null |Set custom file text data loader: a function(fileName) that returns the text as a string, or null to clear. Refused once sandboxed; not supported on web |
|SetSaveFileTextCallback |**callback**=null |Set custom file text data saver: a function(fileName, text) that returns true on success; or null to clear. Refused once sandboxed; not supported on web |
|FileRename |**fileName**, **fileRename** |Rename file (if it exists); returns 0 on success. Not supported on web |
|FileRemove |**fileName** |Remove file (if it exists); returns 0 on success. Not supported on web |
|FileCopy |**srcPath**, **dstPath** |Copy file from one path to another; returns 0 on success. If the destination path does not exist, it is created. Not supported on web |
|FileMove |**srcPath**, **dstPath** |Move file from one directory to another; returns 0 on success. If destination directories do not exist, they are created. Not supported on web |
|FileTextReplace |**fileName**, **search**, **replacement** |Replace text in an existing file; returns 0 on success. Not supported on web |
|FileTextFindIndex |**fileName**, **search** |Find text index position in an existing file, or -1 if not found. Not supported on web |
|FileExists |**fileName** |Check if the file exists |
|DirectoryExists |**dirPath** |Check if directory path exists |
|GetFileLength |**fileName** |Get file length in bytes. Not supported on web |
|GetFileModTime |**fileName** |Get file modification time (last write time), in seconds since the Unix epoch. Not supported on web |
|GetFileExtension |**fileName** |Get the extension of a file name, including the dot (e.g. ".png"), or "" if it has none |
|GetFileName |**filePath** |Get the file name at the end of a path |
|GetFileNameWithoutExt |**filePath** |Get the file name at the end of a path, without its last extension |
|GetDirectoryPath |**filePath** |Get the directory part of a file path |
|GetPrevDirectoryPath |**dirPath** |Get the parent of a directory path |
|GetWorkingDirectory | |Get the current working directory ("" once sandboxed, and on web) |
|GetApplicationDirectory | |Get the directory of the running application ("" once sandboxed, and on web) |
|MakeDirectory |**dirPath** |Create directories (including full path requested); returns 0 on success. Not supported on web |
|ChangeDirectory |**dirPath** |Change working directory; returns 0 on success (nonzero on failure). Refused once sandboxed; not supported on web |
|IsPathFile |**path** |Check if provided path points to a file. Not supported on web |
|IsPathDirectory |**path** |Check if provided path points to an existing directory. Not supported on web |
|IsPathAbsolute |**path** |Check if provided path is an absolute path |
|IsFileHidden |**filePath** |Check if file path (file or directory) is hidden by OS. Not supported on web |
|IsFileNameValid |**fileName** |Check if file name is valid for the platform/OS |
|LoadDirectoryFiles |**dirPath** |Get a list of the paths of files and directories in a directory (not recursive), with the directory path prepended. Returns an empty list on web |
|LoadDirectoryFilesEx |**basePath**, **filter**, **scanSubdirs**=0 |Get a list of file paths in a directory, filtered by extension (e.g. ".png;.jpg"), optionally scanning subdirectories. Use "*.*" to include all files and directories, "FILES*" for only files, or "DIRS*" for only directories. Returns an empty list once sandboxed, and on web |
|UnloadDirectoryFiles |**files** |Does nothing; LoadDirectoryFiles/LoadDirectoryFilesEx return ordinary lists, which need no unloading |
|IsFileDropped | |Check if file has been dropped into window. Not supported on web |
|LoadDroppedFiles | |Get a list of the paths of files dropped into the window. Returns an empty list once sandboxed, and on web |
|UnloadDroppedFiles |**files** |Does nothing; LoadDroppedFiles returns an ordinary list, which needs no unloading |
|GetDirectoryFileCount |**dirPath** |Get the number of files and directories in a directory (0 on web) |
|GetDirectoryFileCountEx |**basePath**, **filter**, **scanSubdirs**=0 |Count the files in a directory with the given extensions (e.g. ".png;.jpg"; include "DIR" to count directories too), optionally searching subdirectories. Returns 0 once sandboxed, and on web |
|CompressData |**data**, **dataSize**=0 |Compress data with DEFLATE: a RawData, a string, or a list of byte values; dataSize, if given, uses only that many bytes. Returns a RawData, or null |
|DecompressData |**compData**, **compDataSize**=0 |Decompress DEFLATE data (as from CompressData): a RawData, a string, or a list of byte values; compDataSize, if given, uses only that many bytes. Returns a RawData, or null |
|DecodeDataBase64 |**text** |Decode a Base64 string into a RawData, or null |
|ComputeCRC32 |**data**, **dataSize**=0 |Compute the CRC32 checksum of data: a RawData, a string, or a list of byte values; dataSize, if given, uses only that many bytes |
|ComputeMD5 |**data**, **dataSize**=0 |Compute the MD5 hash of data (as for ComputeCRC32), as a list of four 32-bit integers |
|ComputeSHA1 |**data**, **dataSize**=0 |Compute the SHA-1 hash of data (as for ComputeCRC32), as a list of five 32-bit integers |
|ComputeSHA256 |**data**, **dataSize**=0 |Compute the SHA-256 hash of data (as for ComputeCRC32), as a list of eight 32-bit integers |
|LoadAutomationEventList |**fileName**="" |Load automation events list from file, or pass no file name ("") for a new empty list (capacity MAX_AUTOMATION_EVENTS). Not supported on web |
|UnloadAutomationEventList |**list** |Unload automation events list, leaving it empty. Not supported on web |
|ExportAutomationEventList |**list**, **fileName** |Export automation events list as text file; returns true on success. Not supported on web |
|SetAutomationEventList |**list**=null |Set up automation event list to record to (null to clear). Not supported on web |
|SetAutomationEventBaseFrame |**frame** |Set automation event internal base frame to start recording. Not supported on web |
|StartAutomationEventRecording | |Start recording automation events (an automation event list must be set with SetAutomationEventList). Not supported on web |
|StopAutomationEventRecording | |Stop recording automation events. Not supported on web |
|PlayAutomationEvent |**event** |Play a recorded automation event. Not supported on web |
|SetRandomSeed |**seed** |Set the seed for the random number generator |
|GetRandomValue |**min**, **max** |Get a random value between min and max included |
|LoadRandomSequence |**count**, **min**, **max** |Get a list of count random integers between min and max (inclusive), with no values repeated |
|UnloadRandomSequence |**sequence**=null |Does nothing; LoadRandomSequence returns an ordinary list, which needs no unloading |
|SetTraceLogLevel |**logLevel** |Set the lowest level of raylib log message to print: LOG_ALL, LOG_TRACE, LOG_DEBUG, LOG_INFO (the default), LOG_WARNING, LOG_ERROR, LOG_FATAL or LOG_NONE |
|SetTraceLogCallback |**callback**=null |Set custom trace log handler: a function(logLevel, text), or null to restore default logging. Not supported on web |
|TraceLog |**logLevel**, **text** |Log a message at the given level (LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG...) |

## RAudio

|Name | Parameters | Purpose |
|-----|------------|---------|
|InitAudioDevice | |Initialize audio device |
|CloseAudioDevice | |Close the audio device for all contexts |
|IsAudioDeviceReady | |Check if the audio device has been initialized successfully |
|SetMasterVolume |**volume**=1.0 |Set master volume (listener), 0.0 to 1.0 |
|GetMasterVolume | |Get master volume (listener) |
|LoadWave |**fileName** |Load wave data from file.  Returns null on failure |
|LoadWaveFromMemory |**fileType**, **fileData**, **dataSize** |Load wave from a RawData buffer.  fileType is the file extension in lower case, e.g. ".wav".  dataSize limits how many bytes are used (0 or more than the buffer's length uses all of it).  Returns null on failure |
|CreateWave |**frameCount**, **sampleRate**, **sampleSize**, **channels**, **samples** |Create a Wave from samples: frameCount frames at sampleRate Hz, with sampleSize 8, 16 or 32 (float) bits and the given number of channels.  samples is a RawData of exactly that size, or a list of frameCount * channels values.  Returns null if they don't agree |
|IsWaveValid |**wave** |Check if wave data is valid (data loaded and parameters) |
|UnloadWave |**wave** |Unload wave data |
|LoadWaveSamples |**wave** |Get the samples of wave as a RawData of 32-bit floats (frameCount * channels of them, channels interleaved), normalized to the range -1 to 1.  Returns null if wave is invalid |
|UnloadWaveSamples |**samples** |Release the RawData returned by LoadWaveSamples early, leaving it empty (optional; it is freed automatically otherwise) |
|ExportWave |**wave**, **fileName** |Export wave data to file; returns true on success |
|WaveCopy |**wave** |Copy a wave to a new wave |
|WaveCrop |**wave**, **initFrame**=0, **finalFrame**=100 |Crop wave in place to the frames from initFrame up to (not including) finalFrame.  Does nothing if the range is invalid |
|WaveFormat |**wave**, **sampleRate**=44100, **sampleSize**=16, **channels**=2 |Convert wave in place to the given sampleRate (Hz), sampleSize (8, 16 or 32 bits) and number of channels |
|LoadMusicStream |**fileName** |Load music stream from file.  Returns null on failure |
|LoadMusicStreamFromMemory |**fileType**, **data**, **dataSize** |Not yet implemented: always returns null |
|IsMusicValid |**music** |Check if music stream is valid (context and buffers initialized) |
|UnloadMusicStream |**music** |Unload music stream |
|PlayMusicStream |**music** |Start music playing (open stream) from beginning |
|IsMusicStreamPlaying |**music** |Check if music is playing |
|UpdateMusicStream |**music** |Update (re-fill) music buffers if data already processed; call this every frame while music is playing |
|StopMusicStream |**music** |Stop music playing (close stream) |
|PauseMusicStream |**music** |Pause music playing |
|ResumeMusicStream |**music** |Resume music playing |
|SeekMusicStream |**music**, **position**=0 |Seek music to a certain position (in seconds) |
|SetMusicVolume |**music**, **volume**=1.0 |Set volume for music (1.0 is max level) |
|SetMusicPitch |**music**, **pitch**=1.0 |Set pitch for music (1.0 is base level) |
|SetMusicPan |**music**, **pan**=0.0 |Set pan for music (-1.0 left, 0.0 center, 1.0 right) |
|GetMusicTimeLength |**music** |Get music time length (in seconds) |
|GetMusicTimePlayed |**music** |Get current music time played (in seconds) |
|LoadSound |**fileName** |Load sound from file.  The entire file is loaded into memory (no streaming).  Returns null on failure |
|LoadSoundFromWave |**wave** |Load sound from wave data.  The data is copied, so wave is unaffected and can be unloaded independently |
|LoadSoundAlias |**source** |Load a sound alias: a clone of source that shares its sample data, so it can play at the same time.  The alias keeps source alive; unload it with UnloadSoundAlias |
|IsSoundValid |**sound** |Check if sound is valid (data loaded and buffers initialized) |
|UnloadSound |**sound** |Unload sound and its sample data |
|UnloadSoundAlias |**alias** |Unload a sound alias (does not free the shared sample data) |
|PlaySound |**sound** |Play a sound |
|StopSound |**sound** |Stop playing a sound |
|PauseSound |**sound** |Pause a sound |
|ResumeSound |**sound** |Resume a paused sound |
|IsSoundPlaying |**sound** |Check if sound is playing |
|UpdateSound |**sound**, **data**, **frameCount** |Update sound buffer with new data from a RawData.  The data format must match the sound's stream sampleSize and channels (by default 32-bit float, stereo); frameCount must be greater than 0 and not exceed the sound's frameCount |
|SetSoundVolume |**sound**, **volume**=1.0 |Set volume for a sound (1.0 is max level) |
|SetSoundPitch |**sound**, **pitch**=1.0 |Set pitch for a sound (1.0 is base level) |
|SetSoundPan |**sound**, **pan**=0.0 |Set pan for a sound (-1.0 left, 0.0 center, 1.0 right) |
|LoadAudioStream |**sampleRate**=44100, **sampleSize**=32, **channels**=1 |Load an audio stream, to play PCM data you supply with UpdateAudioStream.  sampleRate is in Hz; sampleSize is 8, 16 or 32 (float) bits |
|IsAudioStreamValid |**stream** |Check if an audio stream is valid (buffers initialized) |
|UnloadAudioStream |**stream** |Unload audio stream |
|UpdateAudioStream |**stream**, **data** |Update audio stream buffers with data, a list of sample values (ints for 8- or 16-bit streams, floats otherwise), with the channels interleaved for a stereo stream (left, right, left, ...).  Only one buffer of the stream is updated, and only once it has been processed: check IsAudioStreamProcessed first |
|IsAudioStreamProcessed |**stream** |Check if any audio stream buffer requires refill |
|PlayAudioStream |**stream** |Play audio stream |
|PauseAudioStream |**stream** |Pause audio stream |
|ResumeAudioStream |**stream** |Resume audio stream playing |
|IsAudioStreamPlaying |**stream** |Check if audio stream is playing |
|StopAudioStream |**stream** |Stop audio stream |
|SetAudioStreamVolume |**stream**, **volume**=1.0 |Set volume for audio stream (1.0 is max level) |
|SetAudioStreamPitch |**stream**, **pitch**=1.0 |Set pitch for audio stream (1.0 is base level) |
|SetAudioStreamPan |**stream**, **pan**=0.0 |Set pan for audio stream (-1.0 left, 0.0 center, 1.0 right) |
|SetAudioStreamBufferSizeDefault |**size**=4096 |Set the default buffer size (in frames) for new audio streams |

## Global Intrinsics

|Name | Parameters | Purpose |
|-----|------------|---------|
|import |**libname**="" |Import a MiniScript library by name, searching MS_IMPORT_PATH; stores the module under that name and returns it |
|exit |**resultCode** |Exit the program with the given result code (default 0) |
|env | |Get a map of all environment variables |
|shellArgs | |Get a list of the script path and the command-line arguments following it |
|run |**path**="" |Load and run a MiniScript file, replacing the current program but keeping its globals |
|autoUnload |**resource** |Flag a raylib resource to be unloaded automatically when no longer referenced; returns the resource |
|resourceCounts | |Get a map of currently loaded resource counts by type (Image, Texture, Font, etc.), plus a total |
|hostMemory | |Bytes of memory currently allocated by the host program (including raylib, outside the MiniScript GC), or null if this platform cannot report it.  Meaningful only as a difference between readings. |
|Interp | |Get the Interp class, for running code in a child interpreter |

## file module

|Name | Parameters | Purpose |
|-----|------------|---------|
|curdir | |Get current working directory |
|setdir |**path** |Change the current working directory.  Returns null on success, or an error. |
|curdir= |**value** |Change current working directory (same as setdir) |
|children |**path** |Get a list of file and directory names in the given directory (default: the current directory), or an error if it cannot be read |
|name |**path** |Get the filename (last path component) of a path string |
|exists |**path** |Get whether a file or directory exists at the given path |
|info |**path** |Get a map of info (path, isDirectory, size, date) about the given path (default: the current directory), or null if it does not exist |
|makedir |**path** |Create a directory at the given path.  Returns null on success, or an error. |
|parent |**path** |Get the parent directory of the given path |
|child |**parentPath**, **childName** |Combine a parent path and child name into a single path |
|move |**oldPath**, **newPath** |Move (rename) a file or directory, replacing any existing file at newPath.  Returns null on success, or an error. |
|copy |**oldPath**, **newPath** |Copy a file, replacing any existing file at newPath.  Returns null on success, or an error. |
|delete |**path** |Delete a file or empty directory.  Returns null on success, or an error. |
|open |**path**, **mode**="r+" |Open a file; returns a FileHandle, or an error on failure.  The default mode "r+" opens for reading and writing, creating the file if needed. |
|readLines |**path** |Read all lines from a text file, returning a list of strings (or an error if the file cannot be read) |
|writeLines |**path**, **lines** |Write a list of strings (or one string) to a text file, a line each.  Returns null on success, or an error. |
|loadRaw |**path** |Load a binary file, returning a RawData object (or null on failure) |
|saveRaw |**path**, **data** |Save a RawData object to a binary file.  Returns null on success, or an error. |
|enterSandbox | |Enter sandbox mode (one-way; there is no way back) |
|mountAppData |**mountName**="usr", **folderName** |Mount a folder of this application's own data as /usr or /usr2; returns true on success |
|unmount |**mountName**="usr" |Unmount /usr or /usr2; returns true on success |
|droppedFiles | |Files the user has dropped on the window: [{name, isDirectory}] |
|dropPosition | |Where in the window the last drop landed: {x, y} |
|mountDropped |**index**=0, **mountName**="usr" |Mount one of the dropped files (by index into droppedFiles) as /usr or /usr2; returns true on success |
|clearDroppedFiles | |Forget the dropped files (the next drop clears them anyway) |

## FileHandle

|Name | Parameters | Purpose |
|-----|------------|---------|
|close | |Close the file handle, writing any changes to the disk.  Closing a handle that is already closed does nothing. |
|isOpen | |Get whether the file handle is still open |
|write |**data** |Write a string to the file; returns the number of bytes written (0 on error), or an error if the file is not open |
|writeLine |**data** |Write a string followed by a newline to the file; returns the number of bytes written (0 on error), or an error if the file is not open |
|read |**codePointCount**=-1 |Read up to codePointCount characters from the file (or all remaining if -1); returns null on failure |
|readLine | |Read the next line from the file, or null at the end of the file (an error if the file is not open) |
|position | |Get the current read/write position in the file (an error if the file is not open) |
|seek |**pos**=0 |Move the read/write position within the file (an error if the file is not open) |
|position= |**value**=0 |Move the read/write position within the file (same as seek; an error if the file is not open) |
|atEnd | |Get whether the file position is at the end of the file (an error if the file is not open) |

## http module

|Name | Parameters | Purpose |
|-----|------------|---------|
|post |**url**="", **data**, **headers** |Send an HTTP POST request; returns the response body as a string (or an error message string on failure).  data may be a string, or a map to send as form data; headers is an optional map of header names to values. |

## RawData

|Name | Parameters | Purpose |
|-----|------------|---------|
|len | |get the size of this RawData object in bytes |
|resize |**bytes**=32 |resize this RawData object to the given number of bytes (default 32); new bytes are zero |
|len= |**value** |set the size of this RawData object in bytes (same as resize) |
|byte |**offset**=0 |get unsigned byte (0-255) at the given byte offset |
|setByte |**offset**=0, **value**=0 |set unsigned byte at the given byte offset |
|sbyte |**offset**=0 |get signed byte (-128 to 127) at the given byte offset |
|setSbyte |**offset**=0, **value**=0 |set signed byte at the given byte offset |
|ushort |**offset**=0 |get unsigned 16-bit integer (0-65535) at the given byte offset |
|setUshort |**offset**=0, **value**=0 |set unsigned 16-bit integer at the given byte offset |
|short |**offset**=0 |get signed 16-bit integer (-32768 to 32767) at the given byte offset |
|setShort |**offset**=0, **value**=0 |set signed 16-bit integer at the given byte offset |
|uint |**offset**=0 |get unsigned 32-bit integer (0-4294967295) at the given byte offset |
|setUint |**offset**=0, **value**=0 |set unsigned 32-bit integer at the given byte offset |
|int |**offset**=0 |get signed 32-bit integer at the given byte offset |
|setInt |**offset**=0, **value**=0 |set signed 32-bit integer at the given byte offset |
|float |**offset**=0 |get 32-bit float at the given byte offset |
|setFloat |**offset**=0, **value**=0 |set 32-bit float at the given byte offset |
|double |**offset**=0 |get 64-bit double at the given byte offset |
|setDouble |**offset**=0, **value**=0 |set 64-bit double at the given byte offset |
|utf8 |**offset**=0, **bytes**=-1 |get UTF-8 string from the given byte offset (bytes=-1 reads to end); stops early at a zero byte |
|setUtf8 |**offset**=0, **value**="" |write a UTF-8 string at the given byte offset, truncated to fit; returns bytes written |

## Matrix

|Name | Parameters | Purpose |
|-----|------------|---------|
|ofSize |**rows**, **columns**, **initialValue**=0 |Create a new matrix of the given size, filled with a number or by calling a function |
|identity |**size** |Create a new square identity matrix |
|fromList |**sourceList** |Create a new matrix from a 1D or 2D list of numbers |
|fromRawData |**rd**, **dtype**="auto", **startPos**=0, **rows**=null, **columns**=null |Create a new matrix from data in a RawData buffer |
|toFlatList | |Get all elements as one flat list, row by row |
|getElem |**row**, **column** |Get the element at the given row and column |
|setElem |**row**, **column**, **value**=0 |Set the element at the given row and column |
|getRow |**row** |Get one row as a list of numbers |
|setRow |**row**, **values** |Set one row from a list of numbers, or set every element in it to one number |
|getColumn |**column** |Get one column as a list of numbers |
|setColumn |**column**, **values** |Set one column from a list of numbers, or set every element in it to one number |
|getSub |**row**=0, **col**=0, **rows**, **columns**, **out** |Copy a rectangular block out into a new matrix (or into out) |
|setSub |**row**=0, **col**=0, **m2** |Copy another matrix into this one at the given row and column |
|reshape |**rows**, **columns** |Change the shape without moving any elements (this is not a transpose) |
|resize |**rows**, **columns** |Change the shape, keeping the top-left block and zero-filling any new elements |
|reserve |**rows** |Pre-allocate room for the given number of rows, without changing the matrix |
|trim | |Release any memory reserved beyond the current size |
|removeRow |**index** |Remove a row, moving the rows below it up |
|removeRowFast |**index** |Remove a row by moving the last row into its place (changes the row order) |
|equals |**m2**, **tolerance**=1e-9 |Get whether this matrix matches another matrix or list, within a tolerance |
|gemm |**B**, **addend**, **out**, **transA**=0, **transB**=0, **alpha**=1, **beta**=1 |General matrix multiply-add: alpha * self * B + beta * addend, with optional transposes |
|elemMultiplyBy |**x** |Multiply each element in place by a number, or elementwise by a matrix or list |
|elemDivideBy |**x** |Divide each element in place by a number, or elementwise by a matrix or list |
|pow |**k** |Raise each element to a power, in place |
|abs | |Replace each element with its absolute value |
|sqrt | |Replace each element with its square root |
|round |**places**=0 |Round each element to the given number of decimal places |
|clamp |**lo**, **hi** |Limit each element to the range lo to hi (either may be null) |
|fill |**value**=0 |Set every element to the given value |
|randomize |**mean**=0, **sd**=1 |Fill with normally distributed random numbers of the given mean and standard deviation |
|apply |**func** |Replace each element with the result of func(value) |
|apply1 |**func**, **arg1** |Replace each element with the result of func(value, arg1) |
|sum |**axis** |Get the sum of all elements, or of each column (axis 0) or row (axis 1) |
|sumOfSquares |**axis** |Get the sum of the squared elements, overall or per column (axis 0) or row (axis 1) |
|max |**axis** |Get the largest element, overall or per column (axis 0) or row (axis 1) |
|min |**axis** |Get the smallest element, overall or per column (axis 0) or row (axis 1) |
|argmax |**axis** |Get the index of the largest element, overall or per column (axis 0) or row (axis 1) |
|argmin |**axis** |Get the index of the smallest element, overall or per column (axis 0) or row (axis 1) |
|determinant | |Get the determinant of a square matrix |
|inverse | |Get the inverse of a square matrix as a new matrix |
|solve |**b** |Solve self * X = b for X, returning X as a new matrix |
|swapRows |**row1**, **row2** |Swap two rows in place |
|swapColumns |**column1**, **column2** |Swap two columns in place |
|rowCross |**m2** |Get the cross product of each row with the matching row of m2 (both 3 columns wide) |
|convolve |**kernel**, **out** |Apply a kernel (correlation) at every position where it fits, into a same-sized matrix |
|sigmoid | |Apply the sigmoid function to each element, in place |
|tanh | |Apply the tanh function to each element, in place |
|softmax |**axis**=1 |Apply softmax in place, across each row by default |
|greaterThan |**x** |Replace each element with 1 if it is greater than x, or 0 otherwise |
|softmaxCrossEntropy |**targets**, **outProbs** |Get the per-sample softmax cross-entropy loss of these logits against targets |
|toRawData |**rd**, **dtype**="float64", **startPos**=0, **includeHeader**=1 |Write the matrix into a RawData buffer, returning the position after what was written |
|readRawData |**rd**, **dtype**="auto", **startPos**=0 |Read data from a RawData buffer into this matrix, returning the position after what was read |
|format |**fieldWidth**=10, **precision**, **columnSep**="", **rowSep** |Get the matrix as a human-readable table string |
|size | |Get the shape of the matrix as [rows, columns] |
|size= |**value** |Set the shape, keeping the top-left block and zero-filling any new elements |
|rows= |**value** |Set the number of rows, keeping the top-left block and zero-filling any new elements |
|columns= |**value** |Set the number of columns, keeping the top-left block and zero-filling any new elements |
|capacity | |Get how many elements the matrix can hold without reallocating |

## physicsCore

|Name | Parameters | Purpose |
|-----|------------|---------|
|updateGeometry |**shapes**, **verts**, **pos**, **bodyMask** |Transform shapes into world coordinates and compute their bounds, for the bodies whose row in bodyMask is nonzero (or all bodies, if bodyMask is null) |
|findPairs |**shapes**, **invMass**, **margin**=0, **pairs** |Find the pairs of shapes whose bounding boxes come within margin and that may collide; fills the pairs matrix and returns the number of pairs |
|solveContacts |**contacts**, **pos**, **vel**, **invMass**, **dt**, **iterations**=10, **biasFactor**=0.2, **slop**=0.5, **restThreshold**=30 |Solve contact impulses for one step, updating vel and the contacts' ColPn/ColPt in place |
|collidePairs |**pairs**, **shapes**, **verts**, **margin**=0, **prevContacts**, **contacts** |Collide shape pairs into the contacts matrix (one row per point), warm-started from prevContacts (or null); returns how many pairs are touching |

## Interp

|Name | Parameters | Purpose |
|-----|------------|---------|
|create | |create a new child interpreter, with empty globals |
|dispose | |release this interpreter and its VM (an error if it is currently running) |
|setGlobal |**name**="", **value** |set one global in the child |
|getGlobal |**name**="" |read one global back out of the child |
|setGlobals |**map** |bulk-seed the child's globals from a map (typically the caller's `globals`); returns the number of names copied |
|load |**source**="", **sourceName**="" |load and compile a program into the child, with empty globals; sourceName is used in error messages.  Check `error` for a compile error, and runUntilDone to run it |
|loadKeepingGlobals |**source**="" |as load, but the child keeps the globals it already has |
|replLine |**line**="", **seconds**=0.03 |feed one REPL line to the child, and run it for up to `seconds`; if it is not finished by then, keep calling runUntilDone |
|needMoreInput | |true when the last replLine left a block open |
|runUntilDone |**seconds**=0.03 |run the child for up to `seconds`; returns true if it finished |
|done | |true when the child has nothing left to run |
|running | |true while the child still has code to run (the inverse of done) |
|stop | |interrupt the child (like pressing ctrl-C) |
|exitRequested | |true if the child called `exit` |
|exitCode | |the result code the child passed to `exit`, or 0 |
|error | |the child's last error as a string, or null |
|stackTrace | |stack trace of the child's last error, as a list of strings captured where the error was raised (null for a compile error, or if there is no error) |
|lastResult | |value of the last bare expression a replLine evaluated, for REPL echo |
|takeOutput | |return and clear whatever the child has printed since the last call (null if nothing) |

## Plugins

Each plugin documents its own API, in an API.md next to its source.  The examples
(plugins/examples) are built only when asked for; see plugins/README.md.

- [quadtree](plugins/examples/quadtree/API.md)
