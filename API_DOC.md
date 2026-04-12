
## RMath

|Name | Parameters | Purpose |
|-----|------------|---------|
|Clamp |**value**=0, **min**=0, **max**=1 |Clamp float value |
|Lerp |**start**=0, **end**=1, **amount**=0 |Calculate linear interpolation between two floats |
|Normalize |**value**=0, **start**=0, **end**=1 |Normalize input value within input range |
|Remap |**value**=0, **inputStart**=0, **inputEnd**=1, **outputStart**=0, **outputEnd**=1 |Remap input value within input range to output range |
|Wrap |**value**=0, **min**=0, **max**=1 |Wrap input value from min to max |
|FloatEquals |**x**=0, **y**=0 |Check whether two given floats are almost equal |
|Vector2Zero | |Vector with components value 0.0f |
|Vector2One | |Vector with components value 1.0f |
|Vector2Add |**v1**=[0, 0], **v2**=[0, 0] |Add two vectors (v1 + v2) |
|Vector2AddValue |**v**=[0, 0], **add**=0 |Add vector and float value |
|Vector2Subtract |**v1**=[0, 0], **v2**=[0, 0] |Subtract two vectors (v1 - v2) |
|Vector2SubtractValue |**v**=[0, 0], **sub**=0 |Subtract vector by float value |
|Vector2Length |**v**=[0, 0] |Calculate vector length |
|Vector2LengthSqr |**v**=[0, 0] |Calculate vector square length |
|Vector2DotProduct |**v1**=[0, 0], **v2**=[0, 0] |Calculate two vectors dot product |
|Vector2CrossProduct |**v1**=[0, 0], **v2**=[0, 0] |Calculate two vectors cross product |
|Vector2Distance |**v1**=[0, 0], **v2**=[0, 0] |Calculate distance between two vectors |
|Vector2DistanceSqr |**v1**=[0, 0], **v2**=[0, 0] |Calculate square distance between two vectors |
|Vector2Angle |**v1**=[0, 0], **v2**=[0, 0] |Calculate the signed angle from v1 to v2, relative to the origin (0, 0) NOTE: Coordinate system convention: positive X right, positive Y down positive angles appear clockwise, and negative angles appear counterclockwise |
|Vector2LineAngle |**start**=[0, 0], **end**=[0, 0] |Calculate angle defined by a two vectors line NOTE: Parameters need to be normalized Current implementation should be aligned with glm::angle |
|Vector2Scale |**v**=[0, 0], **scale**=1 |Scale vector (multiply by value) |
|Vector2Multiply |**v1**=[0, 0], **v2**=[0, 0] |Multiply vector by vector |
|Vector2Negate |**v**=[0, 0] |Negate vector |
|Vector2Divide |**v1**=[0, 0], **v2**=[0, 0] |Divide vector by vector |
|Vector2Normalize |**v**=[0, 0] |Normalize provided vector |
|Vector2Transform |**v**=[0, 0], **mat**=MatrixIdentity |Transforms a Vector2 by a given Matrix |
|Vector2Lerp |**v1**=[0, 0], **v2**=[0, 0], **amount**=0 |Calculate linear interpolation between two vectors |
|Vector2Reflect |**v**=[0, 0], **normal**=[0, 1] |Calculate reflected vector to normal |
|Vector2Min |**v1**=[0, 0], **v2**=[0, 0] |Get min value for each pair of components |
|Vector2Max |**v1**=[0, 0], **v2**=[0, 0] |Get max value for each pair of components |
|Vector2Rotate |**v**=[0, 0], **angle**=0 |Rotate vector by angle |
|Vector2MoveTowards |**v**=[0, 0], **target**=[0, 0], **maxDistance**=0 |Move Vector towards target |
|Vector2Invert |**v**=[0, 0] |Invert the given vector |
|Vector2Clamp |**v**=[0, 0], **min**=[-1, -1], **max**=[1, 1] |Clamp the components of the vector between min and max values specified by the given vectors |
|Vector2ClampValue |**v**=[0, 0], **min**=0, **max**=1 |Clamp the magnitude of the vector between two min and max values |
|Vector2Equals |**p**=[0, 0], **q**=[0, 0] |Check whether two given vectors are almost equal |
|Vector2Refract |**v**=[0, 0], **n**=[0, 1], **r**=1 |Compute the direction of a refracted ray v: normalized direction of the incoming ray n: normalized normal vector of the interface of two optical media r: ratio of the refractive index of the medium from where the ray comes to the refractive index of the medium on the other side of the surface |
|Vector3Zero | |Vector with components value 0.0f |
|Vector3One | |Vector with components value 1.0f |
|Vector3Add |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Add two vectors |
|Vector3AddValue |**v**=[0, 0, 0], **add**=0 |Add vector and float value |
|Vector3Subtract |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Subtract two vectors |
|Vector3SubtractValue |**v**=[0, 0, 0], **sub**=0 |Subtract vector by float value |
|Vector3Scale |**v**=[0, 0, 0], **scalar**=1 |Multiply vector by scalar |
|Vector3Multiply |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Multiply vector by vector |
|Vector3CrossProduct |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate two vectors cross product |
|Vector3Perpendicular |**v**=[0, 0, 0] |Calculate one vector perpendicular vector |
|Vector3Length |**v**=[0, 0, 0] |Calculate vector length |
|Vector3LengthSqr |**v**=[0, 0, 0] |Calculate vector square length |
|Vector3DotProduct |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate two vectors dot product |
|Vector3Distance |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate distance between two vectors |
|Vector3DistanceSqr |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate square distance between two vectors |
|Vector3Angle |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Calculate angle between two vectors |
|Vector3Negate |**v**=[0, 0, 0] |Negate provided vector (invert direction) |
|Vector3Divide |**v1**=[0, 0, 0], **v2**=[1, 1, 1] |Divide vector by vector |
|Vector3Normalize |**v**=[0, 0, 0] |Normalize provided vector |
|Vector3Project |**v1**=[0, 0, 0], **v2**=[0, 1, 0] | |
|Vector3Reject |**v1**=[0, 0, 0], **v2**=[0, 1, 0] | |
|Vector3OrthoNormalize |**v1**=[1, 0, 0], **v2**=[0, 1, 0] |Orthonormalize provided vectors Makes vectors normalized and orthogonal to each other Gram-Schmidt function implementation |
|Vector3Transform |**v**=[0, 0, 0], **mat**=MatrixIdentity |Transforms a Vector3 by a given Matrix |
|Vector3RotateByQuaternion |**v**=[0, 0, 0], **q**=QuaternionIdentity |Transform a vector by quaternion rotation |
|Vector3RotateByAxisAngle |**v**=[0, 0, 0], **axis**=[0, 1, 0], **angle**=0 |Rotates a vector around an axis |
|Vector3MoveTowards |**v**=[0, 0, 0], **target**=[0, 0, 0], **maxDistance**=0 |Move Vector towards target |
|Vector3Lerp |**v1**=[0, 0, 0], **v2**=[0, 0, 0], **amount**=0 |Calculate linear interpolation between two vectors |
|Vector3CubicHermite |**v1**=[0, 0, 0], **tangent1**=[0, 0, 0], **v2**=[0, 0, 0], **tangent2**=[0, 0, 0], **amount**=0 |Calculate cubic hermite interpolation between two vectors and their tangents as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic |
|Vector3Reflect |**v**=[0, 0, 0], **normal**=[0, 1, 0] |Calculate reflected vector to normal |
|Vector3Min |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Get min value for each pair of components |
|Vector3Max |**v1**=[0, 0, 0], **v2**=[0, 0, 0] |Get max value for each pair of components |
|Vector3Barycenter |**p**=[0, 0, 0], **a**=[0, 0, 0], **b**=[0, 0, 0], **c**=[0, 0, 0] |Compute barycenter coordinates (u, v, w) for point p with respect to triangle (a, b, c) NOTE: Assumes P is on the plane of the triangle |
|Vector3Unproject |**source**=[0, 0, 0], **projection**=MatrixIdentity, **view**=MatrixIdentity |Projects a Vector3 from screen space into object space NOTE: Self-contained function, no other raymath functions are called |
|Vector3ToFloatV |**v**=[0, 0, 0] |Get Vector3 as float array |
|Vector3Invert |**v**=[0, 0, 0] |Invert the given vector |
|Vector3Clamp |**v**=[0, 0, 0], **min**=[-1, -1, -1], **max**=[1, 1, 1] |Clamp the components of the vector between min and max values specified by the given vectors |
|Vector3ClampValue |**v**=[0, 0, 0], **min**=0, **max**=1 |Clamp the magnitude of the vector between two values |
|Vector3Equals |**p**=[0, 0, 0], **q**=[0, 0, 0] |Check whether two given vectors are almost equal |
|Vector3Refract |**v**=[0, 0, 0], **n**=[0, 1, 0], **r**=1 |Compute the direction of a refracted ray v: normalized direction of the incoming ray n: normalized normal vector of the interface of two optical media r: ratio of the refractive index of the medium from where the ray comes to the refractive index of the medium on the other side of the surface |
|Vector4Zero | |Vector4 math |
|Vector4One | | |
|Vector4Add |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] | |
|Vector4AddValue |**v**=[0, 0, 0, 0], **add**=0 | |
|Vector4Subtract |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] | |
|Vector4SubtractValue |**v**=[0, 0, 0, 0], **sub**=0 | |
|Vector4Length |**v**=[0, 0, 0, 0] | |
|Vector4LengthSqr |**v**=[0, 0, 0, 0] | |
|Vector4DotProduct |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] | |
|Vector4Distance |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Calculate distance between two vectors |
|Vector4DistanceSqr |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Calculate square distance between two vectors |
|Vector4Scale |**v**=[0, 0, 0, 0], **scale**=1 | |
|Vector4Multiply |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Multiply vector by vector |
|Vector4Negate |**v**=[0, 0, 0, 0] |Negate vector |
|Vector4Divide |**v1**=[0, 0, 0, 0], **v2**=[1, 1, 1, 1] |Divide vector by vector |
|Vector4Normalize |**v**=[0, 0, 0, 0] |Normalize provided vector |
|Vector4Min |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Get min value for each pair of components |
|Vector4Max |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0] |Get max value for each pair of components |
|Vector4Lerp |**v1**=[0, 0, 0, 0], **v2**=[0, 0, 0, 0], **amount**=0 |Calculate linear interpolation between two vectors |
|Vector4MoveTowards |**v**=[0, 0, 0, 0], **target**=[0, 0, 0, 0], **maxDistance**=0 |Move Vector towards target |
|Vector4Invert |**v**=[1, 1, 1, 1] |Invert the given vector |
|Vector4Equals |**p**=[0, 0, 0, 0], **q**=[0, 0, 0, 0] |Check whether two given vectors are almost equal |
|MatrixDeterminant |**mat**=MatrixIdentity |Compute matrix determinant |
|MatrixTrace |**mat**=MatrixIdentity |Get the trace of the matrix (sum of the values along the diagonal) |
|MatrixTranspose |**mat**=MatrixIdentity |Transposes provided matrix |
|MatrixInvert |**mat**=MatrixIdentity |Invert provided matrix |
|MatrixIdentity | |Get identity matrix |
|MatrixAdd |**left**=MatrixIdentity, **right**=MatrixIdentity |Add two matrices |
|MatrixSubtract |**left**=MatrixIdentity, **right**=MatrixIdentity |Subtract two matrices (left - right) |
|MatrixMultiply |**left**=MatrixIdentity, **right**=MatrixIdentity |Get two matrix multiplication NOTE: When multiplying matrices... the order matters! |
|MatrixMultiplyValue |**left**=MatrixIdentity, **value**=1 | |
|MatrixTranslate |**x**=0, **y**=0, **z**=0 |Get translation matrix |
|MatrixRotate |**axis**=[0, 1, 0], **angle**=0 |Create rotation matrix from axis and angle NOTE: Angle should be provided in radians |
|MatrixRotateX |**angle**=0 |Get x-rotation matrix NOTE: Angle must be provided in radians |
|MatrixRotateY |**angle**=0 |Get y-rotation matrix NOTE: Angle must be provided in radians |
|MatrixRotateZ |**angle**=0 |Get z-rotation matrix NOTE: Angle must be provided in radians |
|MatrixRotateXYZ |**angle**=[0, 0, 0] |Get xyz-rotation matrix NOTE: Angle must be provided in radians |
|MatrixRotateZYX |**angle**=[0, 0, 0] |Get zyx-rotation matrix NOTE: Angle must be provided in radians |
|MatrixScale |**x**=1, **y**=1, **z**=1 |Get scaling matrix |
|MatrixFrustum |**left**=-1, **right**=1, **bottom**=-1, **top**=1, **nearPlane**=0.01, **farPlane**=1000 |Get perspective projection matrix |
|MatrixPerspective |**fovY**=45.0 * DEG2RAD, **aspect**=1, **nearPlane**=0.01, **farPlane**=1000 |Get perspective projection matrix NOTE: Fovy angle must be provided in radians |
|MatrixOrtho |**left**=-1, **right**=1, **bottom**=-1, **top**=1, **nearPlane**=0.01, **farPlane**=1000 |Get orthographic projection matrix |
|MatrixLookAt |**eye**=[0, 0, 1], **target**=[0, 0, 0], **up**=[0, 1, 0] |Get camera look-at matrix (view matrix) |
|MatrixToFloatV |**mat**=MatrixIdentity |Get float array of matrix data |
|MatrixCompose |**translation**=[0, 0, 0], **rotation**=QuaternionIdentity, **scale**=[1, 1, 1] |Compose a transformation matrix from rotational, translational and scaling components TODO: This function is not following raymath conventions defined in header: NOT self-contained |
|MatrixDecompose |**mat**=MatrixIdentity |Decompose a transformation matrix into its rotational, translational and scaling components and remove shear TODO: This function is not following raymath conventions defined in header: NOT self-contained |
|QuaternionAdd |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Add two quaternions |
|QuaternionAddValue |**q**=QuaternionIdentity, **add**=0 |Add quaternion and float value |
|QuaternionSubtract |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Subtract two quaternions |
|QuaternionSubtractValue |**q**=QuaternionIdentity, **sub**=0 |Subtract quaternion and float value |
|QuaternionIdentity | |Get identity quaternion |
|QuaternionLength |**q**=QuaternionIdentity |Computes the length of a quaternion |
|QuaternionNormalize |**q**=QuaternionIdentity |Normalize provided quaternion |
|QuaternionInvert |**q**=QuaternionIdentity |Invert provided quaternion |
|QuaternionMultiply |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Calculate two quaternion multiplication |
|QuaternionScale |**q**=QuaternionIdentity, **mul**=1 |Scale quaternion by float value |
|QuaternionDivide |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity |Divide two quaternions |
|QuaternionLerp |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity, **amount**=0 |Calculate linear interpolation between two quaternions |
|QuaternionNlerp |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity, **amount**=0 |Calculate slerp-optimized interpolation between two quaternions |
|QuaternionSlerp |**q1**=QuaternionIdentity, **q2**=QuaternionIdentity, **amount**=0 |Calculates spherical linear interpolation between two quaternions |
|QuaternionCubicHermiteSpline |**q1**=QuaternionIdentity, **outTangent1**=QuaternionIdentity, **q2**=QuaternionIdentity, **inTangent2**=QuaternionIdentity, **t**=0 |Calculate quaternion cubic spline interpolation using Cubic Hermite Spline algorithm as described in the GLTF 2.0 specification: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#interpolation-cubic |
|QuaternionFromVector3ToVector3 |**from**=[1, 0, 0], **to**=[0, 1, 0] |Calculate quaternion based on the rotation from one vector to another |
|QuaternionFromMatrix |**mat**=MatrixIdentity |Get a quaternion for a given rotation matrix |
|QuaternionToMatrix |**q**=QuaternionIdentity |Get a matrix for a given quaternion |
|QuaternionFromAxisAngle |**axis**=[0, 1, 0], **angle**=0 |Get rotation quaternion for an angle and axis NOTE: Angle must be provided in radians |
|QuaternionToAxisAngle |**q**=QuaternionIdentity |Get the rotation angle and axis for a given quaternion |
|QuaternionFromEuler |**pitch**=0, **yaw**=0, **roll**=0 |Get the quaternion equivalent to Euler angles NOTE: Rotation order is ZYX |
|QuaternionToEuler |**q**=QuaternionIdentity |Get the Euler angles equivalent to quaternion (roll, pitch, yaw) NOTE: Angles are returned in a Vector3 struct in radians |
|QuaternionTransform |**q**=QuaternionIdentity, **mat**=MatrixIdentity |Transform a quaternion given a transformation matrix |
|QuaternionEquals |**p**=QuaternionIdentity, **q**=QuaternionIdentity |Check whether two given quaternions are almost equal |

## RShapes

|Name | Parameters | Purpose |
|-----|------------|---------|
|DrawPixel |**posX**=0, **posY**=0, **color**=WHITE |Draw a pixel |
|DrawPixelV |**position**=[0, 0], **color**=WHITE |Draw a pixel (Vector version) |
|DrawLine |**startPosX**=0, **startPosY**=0, **endPosX**=0, **endPosY**=0, **color**=WHITE |Draw a line (using gl lines) |
|DrawLineV |**startPos**=[0, 0], **endPos**=[0, 0], **color**=WHITE |Draw a line (using gl lines) |
|DrawLineEx |**startPos**=[0, 0], **endPos**=[0, 0], **thick**=1, **color**=WHITE |Draw a line defining thickness |
|DrawCircle |**centerX**=100, **centerY**=100, **radius**=32, **color**=WHITE |Draw a color-filled circle |
|DrawCircleV |**center**=[100, 100], **radius**=32, **color**=WHITE |Draw a color-filled circle (Vector version) NOTE: On OpenGL 3.3 and ES2 using QUADS to avoid drawing order issues |
|DrawCircleLines |**centerX**=100, **centerY**=100, **radius**=32, **color**=WHITE |Draw circle outline |
|DrawEllipse |**centerX**=100, **centerY**=100, **radiusH**=32, **radiusV**=32, **color**=WHITE |Draw ellipse |
|DrawEllipseLines |**centerX**=100, **centerY**=100, **radiusH**=32, **radiusV**=32, **color**=WHITE |Draw ellipse outline |
|DrawRing |**center**=[100, 100], **innerRadius**=20, **outerRadius**=32, **startAngle**=0, **endAngle**=360, **segments**=36, **color**=WHITE |Draw ring |
|DrawRingLines |**center**=[100, 100], **innerRadius**=20, **outerRadius**=32, **startAngle**=0, **endAngle**=360, **segments**=36, **color**=WHITE |Draw ring outline |
|DrawRectangle |**x**=0, **y**=0, **width**=256, **height**=256, **color**=WHITE |Draw a color-filled rectangle |
|DrawRectangleV |**position**=[0, 0], **size**=[256, 256], **color**=WHITE |Draw a color-filled rectangle (Vector version) NOTE: On OpenGL 3.3 and ES2 using QUADS to avoid drawing order issues |
|DrawRectangleRec |**rec**, **color**=WHITE |Draw a color-filled rectangle |
|DrawRectanglePro |**rec**, **origin**=[0, 0], **rotation**=0, **color**=WHITE |Draw a color-filled rectangle with pro parameters |
|DrawRectangleLines |**x**=0, **y**=0, **width**=256, **height**=256, **color** |Draw rectangle outline WARNING: All Draw*Lines() functions use RL_LINES for drawing, it implies flushing the current batch and changing draw mode to RL_LINES but it solves another issue: https://github.com/raysan5/raylib/issues/3884 |
|DrawRectangleLinesEx |**rec**, **lineThick**=1, **color**=WHITE |Draw rectangle outline with extended parameters |
|DrawRectangleRounded |**rec**, **roundness**=0.5, **segments**=36, **color**=WHITE |Draw rectangle with rounded edges |
|DrawRectangleRoundedLines |**rec**, **roundness**=0.5, **segments**=36, **color**=WHITE |Draw rectangle with rounded edges |
|DrawRectangleGradientV |**posX**=0, **posY**=0, **width**=256, **height**=256, **color1**=WHITE, **color2**=BLACK |Draw a vertical-gradient-filled rectangle |
|DrawRectangleGradientH |**posX**=0, **posY**=0, **width**=256, **height**=256, **color1**=WHITE, **color2**=BLACK |Draw a horizontal-gradient-filled rectangle |
|DrawRectangleGradientEx |**rec**, **col1**, **col2**, **col3**, **col4** |Draw a gradient-filled rectangle |
|DrawTriangle |**v1**, **v2**, **v3**, **color**=WHITE |Draw a triangle NOTE: Vertex must be provided in counter-clockwise order |
|DrawTriangleLines |**v1**, **v2**, **v3**, **color**=WHITE |Draw a triangle using lines NOTE: Vertex must be provided in counter-clockwise order |
|DrawPoly |**center**=[100, 100], **sides**=6, **radius**=32, **rotation**=0, **color**=WHITE |Draw a regular polygon of n sides (Vector version) |
|DrawPolyLines |**center**=[100, 100], **sides**=6, **radius**=32, **rotation**=0, **color**=WHITE |Draw a polygon outline of n sides |
|DrawPolyLinesEx |**center**=[100, 100], **sides**=6, **radius**=32, **rotation**=0, **lineThick**=1, **color**=WHITE | |
|CheckCollisionRecs |**rec1**, **rec2** |Check collision between two rectangles |
|CheckCollisionCircles |**center1**, **radius1**, **center2**, **radius2** |Check collision between two circles |
|CheckCollisionCircleRec |**center**, **radius**, **rec** |Check collision between circle and rectangle NOTE: Reviewed version to take into account corner limit case |
|CheckCollisionPointRec |**point**, **rec** |Check if point is inside rectangle |
|CheckCollisionPointCircle |**point**, **center**, **radius** |Check if point is inside circle |
|CheckCollisionPointTriangle |**point**, **p1**, **p2**, **p3** |Check if point is inside a triangle defined by three points (p1, p2, p3) |
|GetCollisionRec |**rec1**, **rec2** |Get collision rectangle for two rectangles collision |
|CheckCollisionCircleLine |**center**, **radius**, **p1**, **p2** |Check if circle collides with a line created between two points [p1] and [p2] |
|CheckCollisionLines |**startPos1**, **endPos1**, **startPos2**, **endPos2** |Check the collision between two lines defined by two points each, returns collision point by reference REF: https://en.wikipedia.org/wiki/Line–line_intersection#Given_two_points_on_each_line_segment |
|CheckCollisionPointLine |**point**, **p1**, **p2**, **threshold** |Check if point belongs to line created between two points [p1] and [p2] with defined margin in pixels [threshold] |
|CheckCollisionPointPoly |**point**, **points** |Check if point is within a polygon described by array of vertices NOTE: Based on http://jeffreythompson.org/collision-detection/poly-point.php |
|DrawCircleGradient |**centerX**=0, **centerY**=0, **radius**=10.0, **colorInner**=WHITE, **colorOuter**=BLACK |Draw a gradient-filled circle |
|DrawCircleLinesV |**center**, **radius**=10.0, **color**=WHITE |Draw circle outline (Vector version) |
|DrawCircleSector |**center**, **radius**=10.0, **startAngle**=0, **endAngle**=90.0, **segments**=36, **color**=WHITE |Draw a piece of a circle |
|DrawCircleSectorLines |**center**, **radius**=10.0, **startAngle**=0, **endAngle**=90.0, **segments**=36, **color**=WHITE |Draw a piece of a circle outlines |
|DrawEllipseV |**center**, **radiusH**=10.0, **radiusV**=5.0, **color**=WHITE |Draw ellipse (Vector version) |
|DrawEllipseLinesV |**center**, **radiusH**=10.0, **radiusV**=5.0, **color**=WHITE |Draw ellipse outline |
|DrawLineBezier |**startPos**, **endPos**, **thick**=1.0, **color**=WHITE |Draw line using cubic-bezier spline, in-out interpolation, no control points |
|DrawLineDashed |**startPos**, **endPos**, **dashSize**, **spaceSize**, **color**=WHITE |Draw a dashed line |
|DrawLineStrip |**points**, **color**=WHITE |Draw lines sequuence (using gl lines) |
|DrawRectangleRoundedLinesEx |**rec**, **roundness**=0.0, **segments**=0, **lineThick**=1.0, **color**=WHITE |Draw rectangle with rounded edges outline |
|DrawSplineLinear |**points**, **thick**=1.0, **color**=WHITE |Draw spline: linear, minimum 2 points |
|DrawSplineBasis |**points**, **thick**=1.0, **color**=WHITE |Draw spline: B-Spline, minimum 4 points |
|DrawSplineCatmullRom |**points**, **thick**=1.0, **color**=WHITE |Draw spline: Catmull-Rom, minimum 4 points |
|DrawSplineBezierQuadratic |**points**, **thick**=1.0, **color**=WHITE |Draw spline: Quadratic Bezier, minimum 3 points (1 control point): [p1, c2, p3, c4...] |
|DrawSplineBezierCubic |**points**, **thick**=1.0, **color**=WHITE |Draw spline: Cubic Bezier, minimum 4 points (2 control points): [p1, c2, c3, p4, c5, c6...] |
|DrawSplineSegmentLinear |**p1**, **p2**, **thick**=1.0, **color**=WHITE |Draw spline segment: Linear, 2 points |
|DrawSplineSegmentBasis |**p1**, **p2**, **p3**, **p4**, **thick**=1.0, **color**=WHITE |Draw spline segment: B-Spline, 4 points |
|DrawSplineSegmentCatmullRom |**p1**, **p2**, **p3**, **p4**, **thick**=1.0, **color**=WHITE |Draw spline segment: Catmull-Rom, 4 points |
|DrawSplineSegmentBezierQuadratic |**p1**, **p2**, **p3**, **thick**=1.0, **color**=WHITE |Draw spline segment: Quadratic Bezier, 2 points, 1 control point |
|DrawSplineSegmentBezierCubic |**p1**, **p2**, **p3**, **p4**, **thick**=1.0, **color**=WHITE |Draw spline segment: Cubic Bezier, 2 points, 2 control points |
|GetSplinePointLinear |**startPos**, **endPos**, **t** |Get spline point for a given t [0.0f .. 1.0f], Linear |
|GetSplinePointBasis |**p1**, **p2**, **p3**, **p4**, **t** |Get spline point for a given t [0.0f .. 1.0f], B-Spline |
|GetSplinePointCatmullRom |**p1**, **p2**, **p3**, **p4**, **t** |Get spline point for a given t [0.0f .. 1.0f], Catmull-Rom |
|GetSplinePointBezierQuad |**p1**, **c2**, **p3**, **t** |Get spline point for a given t [0.0f .. 1.0f], Quadratic Bezier |
|GetSplinePointBezierCubic |**p1**, **c2**, **c3**, **p4**, **t** |Get spline point for a given t [0.0f .. 1.0f], Cubic Bezier |
|DrawTriangleFan |**points**, **color**=WHITE |Draw a triangle fan defined by points NOTE: First vertex provided is the center, shared by all triangles By default, following vertex should be provided in counter-clockwise order |
|DrawTriangleStrip |**points**, **color**=WHITE |Draw a triangle strip defined by points NOTE: Every new vertex connects with previous two |
|SetShapesTexture |**texture**, **source** |Set texture and rectangle to be used on shapes drawing NOTE: It can be useful when using basic shapes and one single font, defining a font char white rectangle would allow drawing everything in a single draw call |
|GetShapesTexture | |Get texture that is used for shapes drawing |
|GetShapesTextureRectangle | |Get texture source rectangle that is used for shapes drawing |

## RModels

|Name | Parameters | Purpose |
|-----|------------|---------|
|DrawLine3D |**startPos**, **endPos**, **color**=WHITE |Draw a line in 3D world space |
|DrawPoint3D |**position**, **color**=WHITE |Draw a point in 3D space, actually a small line WARNING: OpenGL ES 2.0 does not support point mode drawing |
|DrawCircle3D |**center**, **radius**=1.0, **rotationAxis**=[0, 1, 0], **rotationAngle**=0, **color**=WHITE |Draw a circle in 3D world space |
|DrawTriangle3D |**v1**, **v2**, **v3**, **color**=WHITE |Draw a color-filled triangle (vertex in counter-clockwise order!) |
|DrawTriangleStrip3D |**points**, **color**=WHITE |Draw a triangle strip defined by points |
|DrawCube |**position**, **width**=1.0, **height**=1.0, **length**=1.0, **color**=WHITE |Draw cube NOTE: Cube position is the center position |
|DrawCubeV |**position**, **size**=[1, 1, 1], **color**=WHITE |Draw cube (Vector version) |
|DrawCubeWires |**position**, **width**=1.0, **height**=1.0, **length**=1.0, **color**=WHITE |Draw cube wires |
|DrawCubeWiresV |**position**, **size**=[1, 1, 1], **color**=WHITE |Draw cube wires (vector version) |
|DrawSphere |**centerPos**, **radius**=1.0, **color**=WHITE |Draw sphere |
|DrawSphereEx |**centerPos**, **radius**=1.0, **rings**=16, **slices**=16, **color**=WHITE |Draw sphere with extended parameters |
|DrawSphereWires |**centerPos**, **radius**=1.0, **rings**=16, **slices**=16, **color**=WHITE |Draw sphere wires |
|DrawCylinder |**position**, **radiusTop**=1.0, **radiusBottom**=1.0, **height**=2.0, **slices**=16, **color**=WHITE |Draw a cylinder NOTE: It could be also used for pyramid and cone |
|DrawCylinderEx |**startPos**, **endPos**, **startRadius**=1.0, **endRadius**=1.0, **sides**=16, **color**=WHITE |Draw a cylinder with base at startPos and top at endPos NOTE: It could be also used for pyramid and cone |
|DrawCylinderWires |**position**, **radiusTop**=1.0, **radiusBottom**=1.0, **height**=2.0, **slices**=16, **color**=WHITE |Draw a wired cylinder NOTE: It could be also used for pyramid and cone |
|DrawCylinderWiresEx |**startPos**, **endPos**, **startRadius**=1.0, **endRadius**=1.0, **sides**=16, **color**=WHITE |Draw a wired cylinder with base at startPos and top at endPos NOTE: It could be also used for pyramid and cone |
|DrawCapsule |**startPos**, **endPos**, **radius**=1.0, **slices**=16, **rings**=8, **color**=WHITE |Draw a capsule with the center of its sphere caps at startPos and endPos |
|DrawCapsuleWires |**startPos**, **endPos**, **radius**=1.0, **slices**=16, **rings**=8, **color**=WHITE |Draw capsule wires with the center of its sphere caps at startPos and endPos |
|DrawPlane |**centerPos**, **size**=[1, 1], **color**=WHITE |Draw a plane |
|DrawRay |**ray**, **color**=WHITE |Draw a ray line |
|DrawGrid |**slices**=10, **spacing**=1.0 |Draw a grid centered at (0, 0, 0) |
|DrawBillboard |**camera**, **texture**, **position**, **scale**=1.0, **tint**=WHITE |Draw a billboard |
|DrawBillboardRec |**camera**, **texture**, **source**, **position**, **size**=[1, 1], **tint**=WHITE |Draw a billboard (part of a texture defined by a rectangle) |
|DrawBillboardPro |**camera**, **texture**, **source**, **position**, **up**=[0, 1, 0], **size**=[1, 1], **origin**=[0.5, 0.5], **rotation**=0, **tint**=WHITE |Draw a billboard with additional parameters |
|LoadModel |**fileName** |Load model from files (mesh and material) |
|LoadModelFromMesh |**mesh** |Load model from generated mesh WARNING: A shallow copy of mesh is generated, passed by value, as long as struct contains pointers to data and some values, get a copy of mesh pointing to same data as original version... be careful! |
|IsModelValid |**model** |Check if a model is valid (loaded in GPU, VAO/VBOs) |
|UnloadModel |**model** |Unload model (meshes/materials) from memory (RAM and/or VRAM) NOTE: This function takes care of all model elements, for a detailed control over them, use UnloadMesh() and UnloadMaterial() |
|GetModelBoundingBox |**model** |Compute model bounding box limits (considers all meshes) |
|DrawModel |**model**, **position**=[0, 0, 0], **scale**=1.0, **tint**=WHITE |Draw a model (with texture if set) |
|DrawModelEx |**model**, **position**=[0, 0, 0], **rotationAxis**=[0, 1, 0], **rotationAngle**=0, **scale**=[1, 1, 1], **tint**=WHITE |Draw a model with extended parameters |
|DrawModelWires |**model**, **position**=[0, 0, 0], **scale**=1.0, **tint**=WHITE |Draw a model wires (with texture if set) |
|DrawModelWiresEx |**model**, **position**=[0, 0, 0], **rotationAxis**=[0, 1, 0], **rotationAngle**=0, **scale**=[1, 1, 1], **tint**=WHITE |Draw a model wires (with texture if set) with extended parameters |
|DrawBoundingBox |**box**, **color**=WHITE |Draw a bounding box with wires |
|UploadMesh |**mesh**, **dynamic**=0 |Upload vertex data into a VAO (if supported) and VBO |
|UpdateMeshBuffer |**mesh**, **index**, **data**, **offset**=0 |Update mesh vertex data in GPU for a specific buffer index |
|UnloadMesh |**mesh** |Unload mesh from memory (RAM and VRAM) |
|DrawMesh |**mesh**, **material**, **transform**=MatrixIdentity |Draw a 3d mesh with material and transform |
|DrawMeshInstanced |**mesh**, **material**, **transforms** |Draw multiple mesh instances with material and different transforms |
|GetMeshBoundingBox |**mesh** |Compute mesh bounding box limits NOTE: minVertex and maxVertex should be transformed by model transform matrix |
|GenMeshTangents |**mesh** |Compute mesh tangents |
|ExportMesh |**mesh**, **fileName** |Export mesh data to file |
|ExportMeshAsCode |**mesh**, **fileName** |Export mesh as code file (.h) defining multiple arrays of vertex attributes |
|GenMeshPoly |**sides**=6, **radius**=1.0 |Generate polygonal mesh |
|GenMeshPlane |**width**=1.0, **length**=1.0, **resX**=1, **resZ**=1 |Generate plane mesh (with subdivisions) |
|GenMeshCube |**width**=1.0, **height**=1.0, **length**=1.0 |Generated cuboid mesh |
|GenMeshSphere |**radius**=1.0, **rings**=16, **slices**=16 |Generate sphere mesh (standard sphere) |
|GenMeshHemiSphere |**radius**=1.0, **rings**=16, **slices**=16 |Generate hemisphere mesh (half sphere, no bottom cap) |
|GenMeshCylinder |**radius**=1.0, **height**=2.0, **slices**=16 |Generate cylinder mesh |
|GenMeshCone |**radius**=1.0, **height**=2.0, **slices**=16 |Generate cone/pyramid mesh |
|GenMeshTorus |**radius**=1.0, **size**=0.5, **radSeg**=16, **sides**=16 |Generate torus mesh |
|GenMeshKnot |**radius**=1.0, **size**=0.5, **radSeg**=16, **sides**=16 |Generate trefoil knot mesh |
|GenMeshHeightmap |**heightmap**, **size**=[1, 1, 1] |Generate a mesh from heightmap NOTE: Vertex data is uploaded to GPU |
|GenMeshCubicmap |**cubicmap**, **cubeSize**=[1, 1, 1] |Generate a cubes mesh from pixel data NOTE: Vertex data is uploaded to GPU |
|LoadMaterials |**fileName** |Load materials from model file |
|LoadMaterialDefault | |Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps) |
|IsMaterialValid |**material** |Check if a material is valid (map textures loaded in GPU) |
|UnloadMaterial |**material** |Unload material from memory |
|SetMaterialTexture |**material**, **mapType**, **texture** |Set texture for a material map type (MATERIAL_MAP_DIFFUSE, MATERIAL_MAP_SPECULAR...) NOTE: Previous texture should be manually unloaded |
|GetMaterialShader |**material** | |
|SetMaterialShader |**material**, **shader** | |
|GetMaterialShaderLocation |**material**, **uniformName** | |
|GetMaterialShaderLocationAttrib |**material**, **attribName** | |
|SetMaterialShaderValue |**material**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT | |
|SetMaterialShaderValueV |**material**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT, **count**=0 | |
|SetMaterialShaderValueMatrix |**material**, **locIndex**, **mat** | |
|SetMaterialShaderValueTexture |**material**, **locIndex**, **texture** | |
|SetModelMeshMaterial |**model**, **meshId**, **materialId** |Set the material for a mesh |
|LoadModelAnimations |**fileName** |Load model animations from file |
|UpdateModelAnimation |**model**, **animation**, **frame**=0 |Update model animation data (vertex buffers / bone matrices) for a specific pose NOTE 1: Request frame could be fractional, using a lerp interpolation between two frames NOTE 2: Updated vertex animation data is uploaded to GPU in case of CPU skinning, for GPU skinning, bone matrices are uploaded to shader on DrawModelEx() |
|UpdateModelAnimationEx |**model**, **animationA**, **frameA**=0, **animationB**, **frameB**=0, **blend**=0 |Update model animation data (vertex buffers / bone matrices) for a specific pose, defined by two different animations at specific frames blended together NOTE 1: Request frames could be fractional, using a lerp interpolation between two frames NOTE 2: Updated vertex animation data is uploaded to GPU in case of CPU skinning, for GPU skinning, bone matrices are uploaded to shader on DrawModelEx() |
|UnloadModelAnimations |**animations** |Unload animation array data |
|IsModelAnimationValid |**model**, **animation** |Check model animation skeleton match NOTE: Only number of bones and parent connections are checked |
|CheckCollisionSpheres |**center1**, **radius1**, **center2**, **radius2** |Check collision between two spheres |
|CheckCollisionBoxes |**box1**, **box2** |Check collision between two boxes NOTE: Boxes are defined by two points minimum and maximum |
|CheckCollisionBoxSphere |**box**, **center**, **radius** |Check collision between box and sphere |
|GetRayCollisionSphere |**ray**, **center**, **radius** |Get collision info between ray and sphere |
|GetRayCollisionBox |**ray**, **box** |Get collision info between ray and box |
|GetRayCollisionMesh |**ray**, **mesh**, **transform**=MatrixIdentity |Get collision info between ray and mesh |
|GetRayCollisionTriangle |**ray**, **p1**, **p2**, **p3** |Get collision info between ray and triangle NOTE: The points are expected to be in counter-clockwise winding NOTE: Based on https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm |
|GetRayCollisionQuad |**ray**, **p1**, **p2**, **p3**, **p4** |Get collision info between ray and quad NOTE: The points are expected to be in counter-clockwise winding |

## RTextures

|Name | Parameters | Purpose |
|-----|------------|---------|
|LoadImage |**fileName** |Load image from file into CPU memory (RAM) |
|GenImageGradientLinear |**width**=256, **height**=256, **direction**=0, **start**=BLACK, **end**=WHITE |Generate image: linear gradient The direction value specifies the direction of the gradient (in degrees) with 0 being vertical (from top to bottom), 90 being horizontal (from left to right) The gradient effectively rotates counter-clockwise by the specified amount |
|UnloadImage |**image** |Unload image from CPU memory (RAM) |
|LoadTexture |**fileName** |Load texture from file into GPU memory (VRAM) |
|LoadTextureFromImage |**image** |Load a texture from image data NOTE: image is not unloaded, it must be done manually |
|UnloadTexture |**texture** |Unload texture from GPU memory (VRAM) |
|DrawTexture |**texture**, **posX**=0, **posY**=0, **tint**=WHITE |Draw a texture |
|DrawTextureV |**texture**, **position**=[0, 0], **tint**=WHITE |Draw a texture with position defined as Vector2 |
|DrawTextureEx |**texture**, **position**=[0, 0], **rotation**=0, **scale**=1.0, **tint**=WHITE |Draw a texture with extended parameters |
|DrawTextureRec |**texture**, **source**, **position**=[0, 0], **tint**=WHITE |Draw a part of a texture (defined by a rectangle) |
|DrawTexturePro |**texture**, **source**, **dest**, **origin**=[0, 0], **rotation**=0, **tint**=WHITE |Draw a part of a texture (defined by a rectangle) with 'pro' parameters NOTE: origin is relative to destination rectangle size |
|GenImageColor |**width**=256, **height**=256, **color**=WHITE |Generate image: plain color |
|GenImageGradientRadial |**width**=256, **height**=256, **density**=0.5, **inner**=WHITE, **outer**=BLACK |Generate image: radial gradient |
|GenImageGradientSquare |**width**=256, **height**=256, **density**=0.5, **inner**=WHITE, **outer**=BLACK |Generate image: square gradient |
|GenImageChecked |**width**=256, **height**=256, **checksX**=8, **checksY**=8, **col1**=WHITE, **col2**=BLACK |Generate image: checked |
|GenImageWhiteNoise |**width**=256, **height**=256, **factor**=0.5 |Generate image: white noise NOTE: It requires GetRandomValue(), defined in [rcore] |
|GenImageCellular |**width**=256, **height**=256, **tileSize**=32 |Generate image: cellular algorithm. Bigger tileSize means bigger cells |
|ImageCopy |**image** |Copy an image to a new image |
|ImageCrop |**image**, **crop** |Crop an image to area defined by a rectangle NOTE: Security checks are performed in case rectangle goes out of bounds |
|ImageResize |**image**, **newWidth**, **newHeight** |Resize and image to new size NOTE: Uses stb default scaling filters (both bicubic): STBIR_DEFAULT_FILTER_UPSAMPLE    STBIR_FILTER_CATMULLROM STBIR_DEFAULT_FILTER_DOWNSAMPLE  STBIR_FILTER_MITCHELL   (high-quality Catmull-Rom) |
|ImageResizeNN |**image**, **newWidth**, **newHeight** |Resize and image to new size using Nearest-Neighbor scaling algorithm |
|ImageFlipVertical |**image** |Flip image vertically |
|ImageFlipHorizontal |**image** |Flip image horizontally |
|ImageRotateCW |**image** |Rotate image clockwise 90deg |
|ImageRotateCCW |**image** |Rotate image counter-clockwise 90deg |
|ImageColorTint |**image**, **color**=WHITE |Modify image color: tint |
|ImageColorInvert |**image** |Modify image color: invert |
|ImageColorGrayscale |**image** |Modify image color: grayscale |
|ImageColorContrast |**image**, **contrast** |Modify image color: contrast NOTE: Contrast values between -100 and 100 |
|ImageColorBrightness |**image**, **brightness** |Modify image color: brightness NOTE: Brightness values between -255 and 255 |
|ImageClearBackground |**dst**, **color**=WHITE |Clear image background with given color |
|ImageDrawPixel |**dst**, **x**=0, **y**=0, **color**=WHITE |Draw pixel within an image NOTE: Compressed image formats not supported |
|ImageDrawPixelV |**dst**, **position**=[0, 0], **color**=WHITE |Draw pixel within an image (Vector version) |
|ImageDrawLine |**dst**, **startPosX**=0, **startPosY**=0, **endPosX**=0, **endPosY**=0, **color**=WHITE |Draw line within an image |
|ImageDrawLineV |**dst**, **start**=[0, 0], **end**=[0, 0], **color**=WHITE |Draw line within an image (Vector version) |
|ImageDrawCircle |**dst**, **centerX**=100, **centerY**=100, **radius**=32, **color**=WHITE |Draw circle within an image |
|ImageDrawCircleV |**dst**, **center**=[100, 100], **radius**=32, **color**=WHITE |Draw circle within an image (Vector version) |
|ImageDrawRectangle |**dst**, **posX**=0, **posY**=0, **width**=256, **height**=256, **color**=WHITE |Draw rectangle within an image |
|ImageDrawRectangleRec |**dst**, **rec**, **color**=WHITE |Draw rectangle within an image |
|ImageDrawRectangleLines |**dst**, **rec**, **thick**=1, **color**=WHITE |Draw rectangle lines within an image |
|ImageDraw |**dst**, **src**, **srcRec**, **dstRec**, **tint**=WHITE |Draw an image (source) within an image (destination) NOTE: Color tint is applied to source image |
|ImageDrawText |**dst**, **text**, **posX**=0, **posY**=0, **fontSize**=20, **color**=BLACK |Draw text (default font) within an image (destination) |
|SetTextureFilter |**texture**, **filter** |Set texture scaling filter mode |
|SetTextureWrap |**texture**, **wrap** |Set texture wrapping mode |
|GenTextureMipmaps |**texture** |Generate GPU mipmaps for a texture |
|LoadRenderTexture |**width**=960, **height**=640 |Load texture for rendering (framebuffer) NOTE: Render texture is loaded by default with RGBA color attachment and depth RenderBuffer |
|UnloadRenderTexture |**target** |Unload render texture from GPU memory (VRAM) |
|BeginTextureMode |**target** |Initializes render texture for drawing |
|EndTextureMode | |Ends drawing to render texture |
|ColorAlpha |**color**, **alpha** |Get color with alpha applied, alpha goes from 0.0f to 1.0f |
|ColorAlphaBlend |**dst**, **src**, **tint** |Get src alpha-blended into dst color with tint |
|ColorBrightness |**color**, **factor** |Get color with brightness correction, brightness factor goes from -1.0f to 1.0f |
|ColorContrast |**color**, **contrast** |Get color with contrast correction NOTE: Contrast values between -1.0f and 1.0f |
|ColorFromHSV |**hue**, **saturation**, **value** |Get a Color from HSV values Implementation reference: https://en.wikipedia.org/wiki/HSL_and_HSV#Alternative_HSV_conversion NOTE: Color->HSV->Color conversion will not yield exactly the same color due to rounding errors Hue is provided in degrees: [0..360] Saturation/Value are provided normalized: [0.0f..1.0f] |
|ColorFromNormalized |**normalized** |Get color from normalized values [0..1] |
|ColorIsEqual |**col1**, **col2** |Check if two colors are equal |
|ColorLerp |**color1**, **color2**, **amount** |Get color lerp interpolation between two colors, factor [0.0f..1.0f] |
|ColorNormalize |**color** |Get color normalized as float [0..1] |
|ColorTint |**color**, **tint** |Get color multiplied with another color |
|ColorToHSV |**color** |Get HSV values for a Color NOTE: Hue is returned as degrees [0..360] |
|ColorToInt |**color** |Get hexadecimal value for a Color |
|Fade |**color**, **alpha** |Get color with alpha applied, alpha goes from 0.0f to 1.0f |
|GetColor |**hexValue** |Get a Color struct from hexadecimal value |
|GetPixelColor |**srcPtr**, **format** |Get color from a pixel from certain format |
|GetPixelDataSize |**width**, **height**, **format** |Get pixel data size in bytes for certain format NOTE: Size can be requested for Image or Texture data |
|SetPixelColor |**dstPtr**, **color**, **format** |Set pixel color formatted into destination pointer |
|GenImagePerlinNoise |**width**, **height**, **offsetX**, **offsetY**, **scale** |Generate image: perlin noise |
|GenImageText |**text**, **fontSize**, **color** |Generate image: grayscale image from text data |
|IsImageValid |**image** |Check if an image is ready |
|IsRenderTextureValid |**target** |Check if a render texture is valid (loaded in GPU) |
|IsTextureValid |**texture** |Check if a texture is valid (loaded in GPU) |
|LoadImageAnim |**fileName**, **frames** |Load animated image data  - Image.data buffer includes all frames: [image#0][image#1][image#2][...]  - Number of frames is returned through 'frames' parameter  - All frames are returned in RGBA format  - Frames delay data is discarded |
|LoadImageAnimFromMemory |**fileType**, **fileData**, **frames** |Load animated image data  - Image.data buffer includes all frames: [image#0][image#1][image#2][...]  - Number of frames is returned through 'frames' parameter  - All frames are returned in RGBA format  - Frames delay data is discarded |
|LoadImageColors |**image** |Load color data from image as a Color array (RGBA - 32bit) NOTE: Memory allocated should be freed using UnloadImageColors(); |
|LoadImageFromMemory |**fileType**, **fileData** |Load image from memory buffer, fileType refers to extension: i.e. ".png" WARNING: File extension must be provided in lower-case |
|LoadImageFromScreen | |Load image from screen buffer and (screenshot) |
|LoadImageFromTexture |**texture** |Load image from GPU texture data NOTE: Compressed texture formats not supported |
|ExportImage |**image**, **fileName** |Export image data to file NOTE: File format depends on fileName extension |
|ExportImageToMemory |**image**, **fileType** | |
|ExportImageAsCode |**image**, **fileName** |Export image as code file (.h) defining an array of bytes |
|LoadImagePalette |**image**, **colorCount** |Load colors palette from image as a Color array (RGBA - 32bit) NOTE: Memory allocated should be freed using UnloadImagePalette() |
|LoadImageRaw |**fileName**, **width**, **height**, **format**, **headerSize** |Load an image from RAW file data |
|UnloadImageColors |**colors** |Unload color data loaded with LoadImageColors() |
|UnloadImagePalette |**palette** |Unload colors palette loaded with LoadImagePalette() |
|GetImageAlphaBorder |**image**, **threshold** |Get image alpha border rectangle NOTE: Threshold is defined as a percentage: 0.0f -> 1.0f |
|GetImageColor |**image**, **x**, **y** |Get image pixel color at (x, y) position |
|ImageAlphaClear |**image**, **color**, **threshold** |Clear alpha channel to desired color NOTE: Threshold defines the alpha limit, 0.0f to 1.0f |
|ImageAlphaCrop |**image**, **threshold** |Crop image depending on alpha value NOTE: Threshold is defined as a percentage: 0.0f -> 1.0f |
|ImageAlphaMask |**image**, **alphaMask** |Apply alpha mask to image NOTE 1: Returned image is GRAY_ALPHA (16bit) or RGBA (32bit) NOTE 2: alphaMask should be same size as image |
|ImageAlphaPremultiply |**image** |Premultiply alpha channel |
|ImageColorReplace |**image**, **color**, **replace** |Modify image color: replace color |
|ImageBlurGaussian |**image**, **blurSize** |Apply box blur to image |
|ImageDither |**image**, **rBpp**, **gBpp**, **bBpp**, **aBpp** |Dither image data to 16bpp or lower (Floyd-Steinberg dithering) NOTE: In case selected bpp do not represent a known 16bit format, dithered data is stored in the LSB part of the unsigned short |
|ImageFormat |**image**, **newFormat** |Convert image data to desired format |
|ImageFromChannel |**image**, **channel** |Create an image from a selected channel of another image |
|ImageFromImage |**image**, **rec** |Create an image from another image piece |
|ImageKernelConvolution |**image**, **kernel**, **kernelSize** |Apply custom square convolution kernel to image NOTE: The convolution kernel matrix is expected to be square |
|ImageMipmaps |**image** |Generate all mipmap levels for a provided image NOTE 1: Supports POT and NPOT images NOTE 2: image.data is scaled to include mipmap levels NOTE 3: Mipmaps format is the same as base image |
|ImageResizeCanvas |**image**, **offsetX**, **offsetY**, **newWidth**, **newHeight**, **fill** |Resize canvas and fill with color NOTE: Resize offset is relative to the top-left corner of the original image |
|ImageRotate |**image**, **degrees** |Rotate image in degrees |
|ImageToPOT |**image** |Convert image to POT (power-of-two) NOTE: It could be useful on OpenGL ES 2.0 (RPI, HTML5) |
|ImageDrawCircleLines |**dst**, **centerX**, **centerY**, **radius**, **color** |Draw circle outline within an image |
|ImageDrawCircleLinesV |**dst**, **center**, **radius**, **color** |Draw circle outline within an image (Vector version) |
|ImageDrawLineEx |**dst**, **start**, **end**, **thick**, **color** |Draw a line defining thickness within an image |
|ImageDrawRectangleV |**dst**, **rec**, **color** |Draw rectangle within an image (Vector version) |
|ImageDrawTextEx |**dst**, **font**, **text**, **position**, **fontSize**, **spacing**, **tint** |Draw text (custom sprite font) within an image (destination) |
|ImageDrawTriangle |**dst**, **v1**, **v2**, **v3**, **color** |Draw triangle within an image |
|ImageDrawTriangleEx |**dst**, **v1**, **v2**, **v3**, **c1**, **c2**, **c3** |Draw triangle with interpolated colors within an image |
|ImageDrawTriangleFan |**dst**, **points**, **color** |Draw a triangle fan defined by points within an image (first vertex is the center) |
|ImageDrawTriangleLines |**dst**, **v1**, **v2**, **v3**, **color** |Draw triangle outline within an image |
|ImageDrawTriangleStrip |**dst**, **points**, **color** |Draw a triangle strip defined by points within an image |
|ImageText |**text**, **fontSize**, **color** |Create an image from text (default font) |
|ImageTextEx |**font**, **text**, **fontSize**, **spacing**, **tint** |Create an image from text (custom sprite font) WARNING: Module required: rtext |
|DrawTextureNPatch |**texture**, **nPatchInfo**, **dest**, **origin**=[0, 0], **rotation**=0, **tint**=WHITE |Draws a texture (or part of it) that stretches or shrinks nicely using n-patch info |
|UpdateTexture |**texture**, **pixels** |Update GPU texture with new data NOTE 1: pixels data must match texture.format NOTE 2: pixels data must contain at least as many pixels as texture |
|UpdateTextureRec |**texture**, **rec**, **pixels** |Update GPU texture rectangle with new data NOTE 1: pixels data must match texture.format NOTE 2: pixels data must contain as many pixels as rec contains NOTE 3: rec must fit completely within texture's width and height |
|LoadTextureCubemap |**image**, **layout** |Load cubemap from image, multiple image cubemap layouts supported |

## RText

|Name | Parameters | Purpose |
|-----|------------|---------|
|LoadFont |**fileName** |Load Font from file into GPU memory (VRAM) |
|LoadFontEx |**fileName**, **fontSize**=20, **codepoints**=null, **codepointCount**=0 |Load Font from TTF or BDF font file with generation parameters NOTE: You can pass an array with desired characters, those characters should be available in the font if array is NULL, default char set is selected 32..126 |
|LoadFontFromImage |**image**, **key**=Color{255, 0, 255, 255}, **firstChar**=32 |Load an Image font file (XNA style) |
|IsFontValid |**font** |Check if a font is valid (font data loaded) WARNING: GPU texture not checked |
|UnloadFont |**font** |Unload Font from GPU memory (VRAM) |
|MakeFont |**baseSize**, **glyphs**, **recs**, **texture**, **glyphPadding**=0 | |
|DrawFPS |**posX**=0, **posY**=0 |Draw current FPS NOTE: Uses default font |
|DrawText |**text**, **posX**=0, **posY**=0, **fontSize**=20, **color**=BLACK |Draw text (using default font) NOTE: fontSize work like in any drawing program but if fontSize is lower than font-base-size, then font-base-size is used NOTE: chars spacing is proportional to fontSize |
|DrawTextEx |**font**, **text**, **position**=[0, 0], **fontSize**=20, **spacing**=0, **tint**=BLACK |Draw text using Font NOTE: chars spacing is NOT proportional to fontSize |
|DrawTextPro |**font**, **text**, **position**=[0, 0], **origin**=[0, 0], **rotation**=0, **fontSize**=20, **spacing**=0, **tint**=BLACK |Draw text using Font and pro parameters (rotation) |
|DrawTextCodepoint |**font**, **codepoint**, **position**=[0, 0], **fontSize**=20, **tint**=BLACK |Draw one character (codepoint) |
|MeasureText |**text**, **fontSize**=20 |Measure string width for default font |
|MeasureTextEx |**font**, **text**, **fontSize**=20, **spacing**=0 |Measure string size for Font |
|GetGlyphIndex |**font**, **codepoint** |Get index position for a unicode character on font NOTE: If codepoint is not found in the font it fallbacks to '?' |
|GetFontDefault | |Get the default font, useful to be used with extended parameters |
|SetTextLineSpacing |**spacing** |Set vertical line spacing when drawing with line-breaks |
|GetGlyphAtlasRec |**font**, **codepoint** |Get glyph rectangle in font atlas for a codepoint (unicode character) NOTE: If codepoint is not found in the font it fallbacks to '?' |
|GetGlyphInfo |**font**, **codepoint** |Get glyph font info data for a codepoint (unicode character) NOTE: If codepoint is not found in the font it fallbacks to '?' |
|GetCodepointCount |**text** |Get total number of characters(codepoints) in a UTF-8 encoded text, until '\0' is found NOTE: If an invalid UTF-8 sequence is encountered a '?'(0x3f) codepoint is counted instead |
|GetCodepoint |**text**, **codepointSize**=null |Get next codepoint in a UTF-8 encoded text, scanning until '\0' is found When an invalid UTF-8 byte is encountered, exit as soon as possible and a '?'(0x3f) codepoint is returned Total number of bytes processed are returned as a parameter NOTE: The standard says U+FFFD should be returned in case of errors but that character is not supported by the default font in raylib |
|GetCodepointNext |**text**, **codepointSize**=null |Get next codepoint in a byte sequence and bytes processed |
|GetCodepointPrevious |**text**, **codepointSize**=null |Get previous codepoint in a byte sequence and bytes processed |
|CodepointToUTF8 |**codepoint** | |
|TextIsEqual |**text1**, **text2** |Check if two text string are equal REQUIRES: strcmp() |
|TextLength |**text** | |
|TextCopy |**dst**, **src** |Copy one string to another, returns bytes copied NOTE: Alternative implementation to strcpy(dst, src) from C standard library |
|LoadFontFromMemory |**fileType**, **fileData**, **fontSize**, **codepoints**=null, **codepointCount**=0 |Load font from memory buffer, fileType refers to extension: i.e. ".ttf" |
|LoadFontData |**fileData**, **fontSize**, **codepoints**=null, **codepointCount**=0, **type**=0 |Load font data for further use NOTE: Requires TTF font memory data and can generate SDF data |
|UnloadFontData |**glyphs** |Unload font glyphs info data (RAM) |
|LoadCodepoints |**text** |Load all codepoints from a UTF-8 text string, codepoints count returned by parameter |
|UnloadCodepoints |**codepoints** |Unload codepoints data from memory |
|LoadUTF8 |**codepoints** |Encode text codepoint into UTF-8 text REQUIRES: memcpy() WARNING: Allocated memory must be manually freed |
|UnloadUTF8 |**text** |Unload UTF-8 text encoded from codepoints array |
|DrawTextCodepoints |**font**, **codepoints**, **position**=[0, 0], **fontSize**=20, **spacing**=0, **tint**=BLACK |Draw multiple character (codepoints) |
|GenImageFontAtlas |**glyphs**, **glyphRecs**, **fontSize**, **padding**, **packMethod** |Generate image font atlas using chars info NOTE: Packing method: 0-Default, 1-Skyline |
|TextFormat |**text**, **args**=ValueList( | |
|TextFindIndex |**text**, **search** |Find first text occurrence within a string REQUIRES: strstr() |
|GetTextBetween |**text**, **begin**, **end** |Get text between two strings |
|TextReplace |**text**, **search**, **replacement** |Replace text string REQUIRES: strstr(), strncpy() NOTE: Limited text replace functionality, using static string |
|TextReplaceAlloc |**text**, **search**, **replacement** |Replace text string REQUIRES: strstr(), strncpy() WARNING: Allocated memory must be manually freed |
|TextReplaceBetween |**text**, **begin**, **end**, **replacement** |Replace text between two specific strings REQUIRES: strncpy() NOTE: If (replacement == NULL) removes "begin"[ ]"end" text |
|TextReplaceBetweenAlloc |**text**, **begin**, **end**, **replacement** |Replace text between two specific strings REQUIRES: strncpy() NOTE: If (replacement == NULL) remove "begin"[ ]"end" text WARNING: Returned string must be freed by user |
|TextInsert |**text**, **insert**, **position** |Insert text in a specific position, moves all text forward WARNING: Allocated memory must be manually freed |
|TextInsertAlloc |**text**, **insert**, **position** |Insert text in a specific position, moves all text forward WARNING: Allocated memory must be manually freed |
|TextSplit |**text**, **delimiter** | |
|TextJoin |**textList**, **delimiter**="" |Join text strings with delimiter REQUIRES: memset(), memcpy() |
|TextAppend |**text**, **append** |Append text at specific position and move cursor WARNING: It's up to the user to make sure appended text does not overflow the buffer! |
|TextToUpper |**text** |Get upper case version of provided string WARNING: Limited functionality, only basic characters set TODO: Support UTF-8 diacritics to upper-case, check codepoints |
|TextToLower |**text** |Get lower case version of provided string WARNING: Limited functionality, only basic characters set |
|TextToPascal |**text** |Get Pascal case notation version of provided string WARNING: Limited functionality, only basic characters set |
|TextToSnake |**text** |Get snake case notation version of provided string WARNING: Limited functionality, only basic characters set |
|TextToCamel |**text** |Get Camel case notation version of provided string WARNING: Limited functionality, only basic characters set |
|TextToInteger |**text** |Get integer value from text NOTE: This function replaces atoi() [stdlib.h] |
|TextToFloat |**text** |Get float value from text NOTE: This function replaces atof() [stdlib.h] WARNING: Only '.' character is understood as decimal point |
|LoadTextLines |**text** | |

## RCore

|Name | Parameters | Purpose |
|-----|------------|---------|
|BeginDrawing | |Setup canvas (framebuffer) to start drawing |
|EndDrawing | |End canvas drawing and swap buffers (double buffering) |
|ClearBackground |**color**=BLACK |Set background color (framebuffer clear color) |
|BeginMode3D |**camera** |Initializes 3D mode with custom camera (3D) |
|EndMode3D | |Ends 3D mode and returns to default 2D orthographic mode |
|GetScreenToWorldRay |**position**, **camera** |Get a ray trace from screen position (i.e mouse) |
|GetScreenToWorldRayEx |**position**, **camera**, **width**=0, **height**=0 |Get a ray trace from the screen position (i.e mouse) within a specific section of the screen |
|GetWorldToScreen |**position**, **camera** |Get the screen space position from a 3d world space position |
|GetWorldToScreenEx |**position**, **camera**, **width**=0, **height**=0 |Get size position for a 3d world space position (useful for texture drawing) |
|GetCameraMatrix |**camera** |Get transform matrix for camera |
|UpdateCamera |**camera**, **mode**=CAMERA_CUSTOM |Update camera position for selected mode Camera mode: CAMERA_FREE, CAMERA_FIRST_PERSON, CAMERA_THIRD_PERSON, CAMERA_ORBITAL or CUSTOM |
|UpdateCameraPro |**camera**, **movement**=[0, 0, 0], **rotation**=[0, 0, 0], **zoom**=0 |Update camera movement, movement/rotation values should be provided by user |
|LoadShader |**vsFileName**=String(), **fsFileName**=String() |Load shader from files and bind default locations NOTE: If shader filename is NULL, using default vertex/fragment shaders |
|LoadShaderFromMemory |**vsCode**=String(), **fsCode**=String() |Load shader from code strings and bind default locations |
|IsShaderValid |**shader** |Check if a shader is valid (loaded on GPU) |
|BeginShaderMode |**shader** |Begin custom shader mode |
|EndShaderMode | |End custom shader mode (returns to default shader) |
|GetShaderLocation |**shader**, **uniformName** |Get shader uniform location |
|GetShaderLocationAttrib |**shader**, **attribName** |Get shader attribute location |
|SetShaderValueMatrix |**shader**, **locIndex**, **mat** |Set shader uniform value (matrix 4x4) |
|SetShaderValueTexture |**shader**, **locIndex**, **texture** |Set shader uniform value for texture |
|SetShaderValue |**shader**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT |Set shader uniform value |
|SetShaderValueV |**shader**, **locIndex**, **value**, **uniformType**=SHADER_UNIFORM_FLOAT, **count**=0 |Set shader uniform value vector |
|UnloadShader |**shader** |Unload shader from GPU memory (VRAM) |
|SetTargetFPS |**fps** |Set target FPS (maximum) |
|GetFrameTime | |Get time in seconds for last frame drawn (delta time) |
|GetTime | |Get elapsed time measure in seconds since InitTimer() |
|GetFPS | |Get current FPS NOTE: Calculating an average framerate |
|IsKeyPressed |**key** |Check if a key has been pressed once |
|IsKeyPressedRepeat |**key** |Check if a key has been pressed again |
|IsKeyDown |**key** |Check if a key is being pressed (key held down) |
|IsKeyReleased |**key** |Check if a key has been released once |
|IsKeyUp |**key** |Check if a key is NOT being pressed (key not held down) |
|GetKeyPressed | |Get the last key pressed |
|GetCharPressed | |Get the last char pressed |
|GetKeyName |**key** | |
|SetExitKey |**key** |Set a custom key to exit program NOTE: default exitKey is set to ESCAPE |
|IsGamepadAvailable |**gamepad**=0 |Check if a gamepad is available |
|GetGamepadName |**gamepad**=0 | |
|IsGamepadButtonPressed |**gamepad**=0, **button** |Check if a gamepad button has been pressed once |
|IsGamepadButtonDown |**gamepad**=0, **button** |Check if a gamepad button is being pressed |
|IsGamepadButtonReleased |**gamepad**=0, **button** |Check if a gamepad button has NOT been pressed once |
|IsGamepadButtonUp |**gamepad**=0, **button** |Check if a gamepad button is NOT being pressed |
|GetGamepadButtonPressed | |Get the last gamepad button pressed NOTE: Returns last gamepad button down, down->up change not considered |
|GetGamepadAxisCount |**gamepad**=0 |Get gamepad axis count |
|GetGamepadAxisMovement |**gamepad**=0, **axis** |Get axis movement vector for a gamepad |
|SetGamepadMappings |**mappings** |Set internal gamepad mappings |
|SetGamepadVibration |**gamepad**=0, **leftMotor**=0.0, **rightMotor**=0.0, **duration**=0.0 |Set gamepad vibration |
|IsMouseButtonPressed |**button** |Check if a mouse button has been pressed once |
|IsMouseButtonDown |**button** |Check if a mouse button is being pressed |
|IsMouseButtonReleased |**button** |Check if a mouse button has been released once |
|IsMouseButtonUp |**button** |Check if a mouse button is NOT being pressed |
|GetMouseX | |Get mouse position X |
|GetMouseY | |Get mouse position Y |
|GetMousePosition | |Get mouse position XY |
|GetMouseDelta | |Get mouse delta between frames |
|GetMouseWheelMove | |Get mouse wheel movement Y |
|SetMouseCursor |**cursor** |Set mouse cursor |
|ShowCursor | |Show mouse cursor |
|HideCursor | |Hides mouse cursor |
|IsCursorHidden | |Check if cursor is not visible |
|IsCursorOnScreen | |Check if cursor is on the current screen |
|SetWindowTitle |**caption**="raylib-miniscript" |Set title for window |
|SetWindowIcon |**image** |Set icon for window NOTE 1: Image must be in RGBA format, 8bit per channel NOTE 2: Image is scaled by the OS for all required sizes |
|SetWindowIcons |**images**, **count**=0 |Set icon for window, multiple images NOTE 1: Images must be in RGBA format, 8bit per channel NOTE 2: The multiple images are used depending on provided sizes Standard Windows icon sizes: 256, 128, 96, 64, 48, 32, 24, 16 |
|GetScreenWidth | |Get current screen width |
|GetScreenHeight | |Get current screen height |
|GetRenderWidth | |Get current render width which is equal to screen width*dpi scale |
|GetRenderHeight | |Get current screen height which is equal to screen height*dpi scale |
|InitWindow |**width**=960, **height**=640, **title**="raylib-miniscript" |Initialize window and OpenGL context |
|CloseWindow | |Close window and unload OpenGL context |
|WindowShouldClose | |Check if application should close NOTE: By default, if KEY_ESCAPE pressed or window close icon clicked |
|IsWindowFullscreen | |Check if window is currently fullscreen |
|IsWindowHidden | |Check if window is currently hidden |
|IsWindowMinimized | |Check if window has been minimized |
|IsWindowMaximized | |Check if window has been maximized |
|IsWindowResized | |Check if window has been resizedLastFrame |
|IsWindowState |**flags** |Check if one specific window flag is enabled |
|SetWindowState |**flags** |Set window configuration state using flags |
|ClearWindowState |**flags** |Clear window configuration state flags |
|ToggleFullscreen | |Toggle fullscreen mode |
|ToggleBorderlessWindowed | |Toggle borderless windowed mode |
|MaximizeWindow | |Set window state: maximized, if resizable |
|MinimizeWindow | |Set window state: minimized |
|RestoreWindow | |Restore window from being minimized/maximized |
|SetWindowPosition |**x**, **y** |Set window position on screen (windowed mode) |
|SetWindowMonitor |**monitor** |Set monitor for the current window |
|SetWindowMinSize |**width**, **height** |Set window minimum dimensions (FLAG_WINDOW_RESIZABLE) |
|SetWindowMaxSize |**width**, **height** |Set window maximum dimensions (FLAG_WINDOW_RESIZABLE) |
|SetWindowSize |**width**, **height** |Set window dimensions |
|SetWindowOpacity |**opacity** |Set window opacity, value opacity is between 0.0 and 1.0 |
|SetWindowFocused | |Set window focused |
|GetWindowHandle | |Local storage for the window handle returned by glfwGetX11Window This is needed as X11 handles are integers and may not fit inside a pointer depending on platform Storing the handle locally and returning a pointer in GetWindowHandle allows the code to work regardless of pointer width Get native window handle |
|GetMonitorCount | |Get number of monitors |
|GetCurrentMonitor | |Get current monitor where window is placed |
|GetMonitorPosition |**monitor**=0 |Get selected monitor position |
|GetMonitorWidth |**monitor**=0 |Get selected monitor width (currently used by monitor) |
|GetMonitorHeight |**monitor**=0 |Get selected monitor height (currently used by monitor) |
|GetMonitorPhysicalWidth |**monitor**=0 |Get selected monitor physical width in millimetres |
|GetMonitorPhysicalHeight |**monitor**=0 |Get selected monitor physical height in millimetres |
|GetMonitorRefreshRate |**monitor**=0 |Get selected monitor refresh rate |
|GetMonitorName |**monitor**=0 | |
|GetWindowPosition | |Get window position XY on monitor |
|GetWindowScaleDPI | |Get window scale DPI factor for current monitor |
|IsWindowFocused | |Check if window has the focus |
|IsWindowReady | |Check if window has been initialized successfully |
|GetMouseWheelMoveV | |Get mouse wheel movement X/Y as a vector |
|SetMousePosition |**x**, **y** |Set mouse position XY |
|SetMouseOffset |**offsetX**, **offsetY** |Set mouse offset NOTE: Useful when rendering to different size targets |
|SetMouseScale |**scaleX**, **scaleY** |Set mouse scaling NOTE: Useful when rendering to different size targets |
|EnableCursor | |Enables cursor (unlock cursor) |
|DisableCursor | |Disables cursor (lock cursor) |
|GetTouchX | |Get touch position X for touch point 0 (relative to screen size) |
|GetTouchY | |Get touch position Y for touch point 0 (relative to screen size) |
|GetTouchPosition |**index**=0 |Get touch position XY for a touch point index (relative to screen size) |
|GetTouchPointId |**index**=0 |Get touch point identifier for given index |
|GetTouchPointCount | |Get number of touch points |
|SetGesturesEnabled |**flags** |Enable only desired gestures to be detected |
|IsGestureDetected |**gesture** |Check if a gesture have been detected |
|GetGestureDetected | |Get latest detected gesture |
|GetGestureHoldDuration | |Hold time measured in seconds |
|GetGestureDragVector | |Get drag vector (between initial touch point to current) |
|GetGestureDragAngle | |Get drag angle NOTE: Angle in degrees, horizontal-right is 0, counterclockwise |
|GetGesturePinchVector | |Get distance between two pinch points |
|GetGesturePinchAngle | |Get angle between two pinch points NOTE: Angle in degrees, horizontal-right is 0, counterclockwise |
|BeginMode2D |**camera** |Initialize 2D mode with custom camera (2D) |
|EndMode2D | |Ends 2D mode with custom camera |
|GetCameraMatrix2D |**camera** |Get camera 2d transform matrix |
|GetWorldToScreen2D |**position**, **camera** |Get the screen space position for a 2d camera world space position |
|GetScreenToWorld2D |**position**, **camera** |Get the world space position for a 2d camera screen space position |
|BeginBlendMode |**mode** |Begin blending mode (alpha, additive, multiplied, subtract, custom) NOTE: Blend modes supported are enumerated in BlendMode enum |
|EndBlendMode | |End blending mode (reset to default: alpha blending) |
|rlSetBlendFactors |**glSrcFactor**, **glDstFactor**, **glEquation** |Set blending mode factor and equation |
|rlSetBlendFactorsSeparate |**glSrcRGB**, **glDstRGB**, **glSrcAlpha**, **glDstAlpha**, **glEqRGB**, **glEqAlpha** |Set blending mode factor and equation separately for RGB and alpha |
|rlMatrixMode |**mode** |Choose the current matrix to be transformed |
|rlPushMatrix | |Push the current matrix into RLGL.State.stack |
|rlPopMatrix | |Pop latest inserted matrix from RLGL.State.stack |
|rlLoadIdentity | |Reset current matrix to identity matrix |
|rlTranslatef |**x**=0, **y**=0, **z**=0 |Multiply the current matrix by a translation matrix |
|rlRotatef |**angle**=0, **x**=0, **y**=0, **z**=0 |Multiply the current matrix by a rotation matrix NOTE: The provided angle must be in degrees |
|rlScalef |**x**=1, **y**=1, **z**=1 |Multiply the current matrix by a scaling matrix |
|rlMultMatrixf |**matf** |Multiply the current matrix by another matrix |
|rlFrustum |**left**, **right**, **bottom**, **top**, **znear**, **zfar** |Multiply the current matrix by a perspective matrix generated by parameters |
|rlOrtho |**left**, **right**, **bottom**, **top**, **znear**, **zfar** |Multiply the current matrix by an orthographic matrix generated by parameters |
|rlViewport |**x**, **y**, **width**, **height** |Set the viewport area (transformation from normalized device coordinates to window coordinates) |
|rlSetClipPlanes |**nearPlane**, **farPlane** |Set clip planes distances |
|rlGetCullDistanceNear | |Get cull plane distance near |
|rlGetCullDistanceFar | |Get cull plane distance far |
|rlEnableBackfaceCulling | | |
|rlDisableBackfaceCulling | | |
|rlEnableDepthTest | | |
|rlDisableDepthTest | | |
|rlEnableDepthMask | | |
|rlDisableDepthMask | | |
|rlEnableWireMode | |Enable wire mode |
|rlDisableWireMode | |Disable wire mode |
|rlEnableSmoothLines | |Enable line aliasing |
|rlDisableSmoothLines | |Disable line aliasing |
|rlSetLineWidth |**width**=1 | |
|rlGetLineWidth | |Get the line drawing width |
|rlDrawRenderBatchActive | |Update and draw internal render batch |
|rlGetMatrixModelview | |Get internal modelview matrix |
|rlGetMatrixProjection | |Get internal projection matrix |
|rlSetMatrixProjection |**proj** |Set a custom projection matrix (replaces internal projection matrix) |
|rlSetMatrixModelview |**view** |Set a custom modelview matrix (replaces internal modelview matrix) |
|BeginScissorMode |**x**, **y**, **width**, **height** |Begin scissor mode (define screen area for following drawing) NOTE: Scissor rec refers to bottom-left corner, changing it to upper-left |
|EndScissorMode | |End scissor mode |
|BeginVrStereoMode |**config** |Begin VR drawing configuration |
|EndVrStereoMode | |End VR drawing process (and desktop mirror) |
|LoadVrStereoConfig |**device** |Load VR stereo config for VR simulator device parameters |
|UnloadVrStereoConfig |**config** |Unload VR stereo config properties |
|OpenURL |**url** |Open URL with default system browser (if available) NOTE: This function is only safe to use if you control the URL given A user could craft a malicious string performing another action Only call this function yourself not with user input or make sure to check the string yourself REF: https://github.com/raysan5/raylib/issues/686 |
|SetClipboardText |**text** |Set clipboard text content |
|GetClipboardText | | |
|EnableEventWaiting | |Enable waiting for events on EndDrawing(), no automatic event polling |
|DisableEventWaiting | |Disable waiting for events on EndDrawing(), automatic events polling |
|GetClipboardImage | |Get clipboard image |
|IsFileExtension |**fileName**, **ext** |Check file extension |
|TakeScreenshot |**fileName** |Takes a screenshot of current screen NOTE: Provided fileName should not contain paths, saving to working directory |
|SetConfigFlags |**flags** |Setup window configuration flags (view FLAGS) NOTE: This function is expected to be called before window creation, because it sets up some flags for the window creation process To configure window states after creation, use SetWindowState() |
|EncodeDataBase64 |**data**, **dataSize** |Encode data to Base64 string NOTE: Returned string includes NULL terminator, considered on outputSize |
|WaitTime |**seconds**=1.0 |Wait for some time (stop program execution) NOTE: Sleep() granularity could be around 10 ms, it means, Sleep() could take longer than expected... for that reason a busy wait loop is used REF: http://stackoverflow.com/questions/43057578/c-programming-win32-games-sleep-taking-longer-than-expected REF: http://www.geisswerks.com/ryan/FAQS/timing.html --> All about timing on Win32! |
|SwapScreenBuffer | |Swap back buffer with front buffer (screen drawing) |
|PollInputEvents | |Register all input events |
|LoadFileText |**fileName** |Load text data from file, returns a '\0' terminated string NOTE: text chars array should be freed manually |
|LoadFileData |**fileName** | |
|UnloadFileData |**data** |Unload file data allocated by LoadFileData() |
|SaveFileData |**fileName**, **data**, **dataSize**=0 |Save data to file from buffer |
|ExportDataAsCode |**data**, **dataSize**, **fileName** |Export data to code (.h), returns true on success |
|UnloadFileText |**text** |Unload file text data allocated by LoadFileText() |
|SaveFileText |**fileName**, **text** |Save text data to file (write), string must be '\0' terminated |
|SetLoadFileDataCallback |**callback**=null |Set custom file binary data loader |
|SetSaveFileDataCallback |**callback**=null |Set custom file binary data saver |
|SetLoadFileTextCallback |**callback**=null |Set custom file text data loader |
|SetSaveFileTextCallback |**callback**=null |Set custom file text data saver |
|FileRename |**fileName**, **fileRename** |Rename file (if exists) NOTE: Only rename file name required, not full path |
|FileRemove |**fileName** |Remove file (if exists) |
|FileCopy |**srcPath**, **dstPath** |Copy file from one path to another NOTE: If destination path does not exist, it is created! |
|FileMove |**srcPath**, **dstPath** |Move file from one directory to another NOTE: If dst directories do not exists they are created |
|FileTextReplace |**fileName**, **search**, **replacement** |Replace text in an existing file WARNING: DEPENDENCY: [rtext] module |
|FileTextFindIndex |**fileName**, **search** |Find text index position in existing file WARNING: DEPENDENCY: [rtext] module |
|FileExists |**fileName** |Check if the file exists |
|DirectoryExists |**dirPath** |Check if a directory path exists |
|GetFileLength |**fileName** |Get file length in bytes NOTE: GetFileSize() conflicts with windows.h |
|GetFileModTime |**fileName** |Get file modification time (last write time) |
|GetFileExtension |**fileName** | |
|GetFileName |**filePath** | |
|GetFileNameWithoutExt |**filePath** | |
|GetDirectoryPath |**filePath** | |
|GetPrevDirectoryPath |**dirPath** | |
|GetWorkingDirectory | | |
|GetApplicationDirectory | | |
|MakeDirectory |**dirPath** |Create directories (including full path requested), returns 0 on success |
|ChangeDirectory |**dirPath** |Change working directory, returns true on success |
|IsPathFile |**path** |Check if a given path point to a file |
|IsFileNameValid |**fileName** |Check if fileName is valid for the platform/OS |
|LoadDirectoryFiles |**dirPath** |Load directory filepaths NOTE: Base path is prepended to the scanned filepaths WARNING: Directory is scanned twice, first time to get files count No recursive scanning is done! |
|LoadDirectoryFilesEx |**basePath**, **filter**, **scanSubdirs**=0 |Load directory filepaths with extension filtering and recursive directory scan Use 'DIR*' to include directories on directory scan Use '*.*' to include all file types and directories on directory scan WARNING: Directory is scanned twice, first time to get files count |
|UnloadDirectoryFiles |**files** |Unload directory filepaths WARNING: files.count is not reseted to 0 after unloading |
|IsFileDropped | |Check if a file has been dropped into window |
|LoadDroppedFiles | |Load dropped filepaths |
|UnloadDroppedFiles |**files** |Unload dropped filepaths |
|GetDirectoryFileCount |**dirPath** | |
|GetDirectoryFileCountEx |**basePath**, **filter**, **scanSubdirs**=0 | |
|CompressData |**data**, **dataSize**=0 | |
|DecompressData |**compData**, **compDataSize**=0 | |
|DecodeDataBase64 |**text** | |
|ComputeCRC32 |**data**, **dataSize**=0 | |
|ComputeMD5 |**data**, **dataSize**=0 | |
|ComputeSHA1 |**data**, **dataSize**=0 | |
|ComputeSHA256 |**data**, **dataSize**=0 | |
|LoadAutomationEventList |**fileName**=String() |Load automation events list from file, NULL for empty list, capacity = MAX_AUTOMATION_EVENTS |
|UnloadAutomationEventList |**list** |Unload automation events list from file |
|ExportAutomationEventList |**list**, **fileName** |Export automation events list as text file |
|SetAutomationEventList |**list**=null |Setup automation event list to record to |
|SetAutomationEventBaseFrame |**frame** |Set automation event internal base frame to start recording |
|StartAutomationEventRecording | |Start recording automation events (AutomationEventList must be set) |
|StopAutomationEventRecording | |Stop recording automation events |
|PlayAutomationEvent |**event** |Play a recorded automation event |
|SetRandomSeed |**seed** |Set the seed for the random number generator |
|GetRandomValue |**min**, **max** |Get a random value between min and max included |
|LoadRandomSequence |**count**, **min**, **max** |Load random values sequence, no values repeated, min and max included |
|UnloadRandomSequence |**sequence**=null |Unload random values sequence |
|SetTraceLogLevel |**logLevel** |Logging and tracing |
|SetTraceLogCallback |**callback**=null |Set custom trace log |
|TraceLog |**logLevel**, **text** |Show trace log messages (LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_DEBUG) |

## RAudio

|Name | Parameters | Purpose |
|-----|------------|---------|
|InitAudioDevice | |Initialize audio device |
|CloseAudioDevice | |Close the audio device for all contexts |
|IsAudioDeviceReady | |Check if device has been initialized successfully |
|SetMasterVolume |**volume**=1.0 |Set master volume (listener) |
|GetMasterVolume | |Get master volume (listener) |
|LoadWave |**fileName** |Load wave data from file |
|LoadWaveFromMemory |**fileType**, **fileData**, **dataSize** |Load wave from memory buffer, fileType refers to extension: i.e. ".wav" WARNING: File extension must be provided in lower-case |
|CreateWave |**frameCount**, **sampleRate**, **sampleSize**, **channels**, **samples** | |
|IsWaveValid |**wave** |Checks if wave data is valid (data loaded and parameters) |
|UnloadWave |**wave** |Unload wave data |
|LoadWaveSamples |**wave** |Load samples data from wave as a floats array NOTE 1: Returned sample values are normalized to range [-1..1] NOTE 2: Sample data allocated should be freed with UnloadWaveSamples() |
|UnloadWaveSamples |**samples** |Unload samples data loaded with LoadWaveSamples() |
|WaveCopy |**wave** |Copy a wave to a new wave |
|WaveCrop |**wave**, **initFrame**=0, **finalFrame**=100 |Crop a wave to defined frames range NOTE: Security check in case of out-of-range |
|WaveFormat |**wave**, **sampleRate**=44100, **sampleSize**=16, **channels**=2 |Convert wave data to desired format |
|LoadMusicStream |**fileName** |Load music stream from file |
|LoadMusicStreamFromMemory |**fileType**, **data**, **dataSize** |Load music stream from memory buffer, fileType refers to extension: i.e. ".wav" WARNING: File extension must be provided in lower-case |
|IsMusicValid |**music** |Checks if a music stream is valid (context and buffers initialized) |
|UnloadMusicStream |**music** |Unload music stream |
|PlayMusicStream |**music** |Start music playing (open stream) from beginning |
|IsMusicStreamPlaying |**music** |Check if any music is playing |
|UpdateMusicStream |**music** |Update (re-fill) music buffers if data already processed |
|StopMusicStream |**music** |Stop music playing (close stream) |
|PauseMusicStream |**music** |Pause music playing |
|ResumeMusicStream |**music** |Resume music playing |
|SeekMusicStream |**music**, **position**=0 |Seek music to a certain position (in seconds) |
|SetMusicVolume |**music**, **volume**=1.0 |Set volume for music |
|SetMusicPitch |**music**, **pitch**=1.0 |Set pitch for music |
|SetMusicPan |**music**, **pan**=0.5 |Set pan for a music |
|GetMusicTimeLength |**music** |Get music time length (in seconds) |
|GetMusicTimePlayed |**music** |Get current music time played (in seconds) |
|LoadSound |**fileName** |Load sound from file NOTE: The entire file is loaded to memory to be played (no-streaming) |
|LoadSoundFromWave |**wave** |Load sound from wave data NOTE: Wave data must be unallocated manually |
|LoadSoundAlias |**source** |Clone sound from existing sound data, clone does not own wave data NOTE: Wave data must be unallocated manually and will be shared across all clones |
|IsSoundValid |**sound** |Checks if a sound is valid (data loaded and buffers initialized) |
|UnloadSound |**sound** |Unload sound |
|UnloadSoundAlias |**alias** |Also delete the heap-allocated Sound |
|PlaySound |**sound** |Play a sound |
|StopSound |**sound** |Stop reproducing a sound |
|PauseSound |**sound** |Pause a sound |
|ResumeSound |**sound** |Resume a paused sound |
|IsSoundPlaying |**sound** |Check if a sound is playing |
|UpdateSound |**sound**, **data**, **sampleCount** |Update sound buffer with new data PARAMS: [data], format must match sound.stream.sampleSize, default 32 bit float - stereo PARAMS: [frameCount] must not exceed sound.frameCount |
|SetSoundVolume |**sound**, **volume**=1.0 |Set volume for a sound |
|SetSoundPitch |**sound**, **pitch**=1.0 |Set pitch for a sound |
|SetSoundPan |**sound**, **pan**=0.5 |Set pan for a sound |
|LoadAudioStream |**sampleRate**=44100, **sampleSize**=32, **channels**=1 |Load audio stream (to stream audio pcm data) |
|IsAudioStreamValid |**stream** |Checks if an audio stream is valid (buffers initialized) |
|UnloadAudioStream |**stream** |Unload audio stream and free memory |
|UpdateAudioStream |**stream**, **data** |Update audio stream buffers with data NOTE 1: Only updates one buffer of the stream source: dequeue -> update -> queue NOTE 2: To dequeue a buffer it needs to be processed: IsAudioStreamProcessed() |
|IsAudioStreamProcessed |**stream** |Check if any audio stream buffers requires refill |
|PlayAudioStream |**stream** |Play audio stream |
|PauseAudioStream |**stream** |Play audio stream |
|ResumeAudioStream |**stream** |Resume audio stream playing |
|IsAudioStreamPlaying |**stream** |Check if audio stream is playing |
|StopAudioStream |**stream** |Stop audio stream |
|SetAudioStreamVolume |**stream**, **volume**=1.0 |Set volume for audio stream (1.0 is max level) |
|SetAudioStreamPitch |**stream**, **pitch**=1.0 |Set pitch for audio stream (1.0 is base level) |
|SetAudioStreamPan |**stream**, **pan**=0.5 |Set pan for audio stream |
|SetAudioStreamBufferSizeDefault |**size**=4096 |Default size for new audio streams |

## Global Intrinsics

|Name | Parameters | Purpose |
|-----|------------|---------|
|import |**libname**="" |Import a MiniScript library by name, searching MS_IMPORT_PATH |
|exit |**resultCode** |Exit the program with the given result code |
|env | |Get a map of all environment variables |
|run |**path**="" |Load and run a MiniScript file in the current interpreter context |
|resourceCounts | |Get a map of currently loaded resource counts by type (Image, Texture, Font, etc.) |

## file module

|Name | Parameters | Purpose |
|-----|------------|---------|
|curdir | |Get current working directory |
|setdir |**path** |Change current working directory |
|children |**path** |Get list of file and directory names in the given directory |
|name |**path** |Get the filename (last path component) of a path string |
|exists |**path** |Get whether a file or directory exists at the given path |
|info |**path** |Get a map of info (path, isDirectory, size, date) about the given path |
|makedir |**path** |Create a directory at the given path |
|parent |**path** |Get the parent directory of the given path |
|child |**parentPath**, **childName** |Combine a parent path and child name into a single path |
|move |**oldPath**, **newPath** |Move (rename) a file or directory |
|copy |**oldPath**, **newPath** |Copy a file |
|delete |**path** |Delete a file or empty directory |
|open |**path**, **mode**="r+" |Open a file; returns a FileHandle, or null on failure |
|readLines |**path** |Read all lines from a text file, returning a list of strings |
|writeLines |**path**, **lines** |Write a list of strings (or a single string) to a text file |
|loadRaw |**path** |Load a binary file, returning a RawData object |
|saveRaw |**path**, **data** |Save a RawData object to a binary file |

## FileHandle

|Name | Parameters | Purpose |
|-----|------------|---------|
|close | |Close the file handle |
|isOpen | |Get whether the file handle is still open |
|write |**data** |Write a string to the file |
|writeLine |**data** |Write a string followed by a newline to the file |
|read |**byteCount**=-1 |Read up to byteCount bytes from the file (or all remaining if -1) |
|readLine | |Read the next line from the file |
|position | |Get the current read/write position in the file |
|atEnd | |Get whether the file position is at the end of the file |

## http module

|Name | Parameters | Purpose |
|-----|------------|---------|
|post |**url**="", **data**, **headers** |Send an HTTP POST request; returns the response body as a string |

## RawData

|Name | Parameters | Purpose |
|-----|------------|---------|
|len | |get the size of this RawData object in bytes |
|resize |**bytes**=32 |resize this RawData object to the given number of bytes |
|byte |**offset**=0 |get unsigned byte (0-255) at the given byte offset |
|setByte |**offset**=0, **value**=0 |set unsigned byte at the given byte offset |
|sbyte |**offset**=0 |get signed byte (-128 to 127) at the given byte offset |
|setSbyte |**offset**=0, **value**=0 |set signed byte at the given byte offset |
|ushort |**offset**=0 |get unsigned 16-bit integer (0-65535) at the given byte offset |
|setUshort |**offset**=0, **value**=0 |set unsigned 16-bit integer at the given byte offset |
|short |**offset**=0 |get signed 16-bit integer (-32768 to 32767) at the given byte offset |
|setShort |**offset**=0, **value**=0 |set signed 16-bit integer at the given byte offset |
|uint |**offset**=0 |get unsigned 32-bit integer at the given byte offset |
|setUint |**offset**=0, **value**=0 |set unsigned 32-bit integer at the given byte offset |
|int |**offset**=0 |get signed 32-bit integer at the given byte offset |
|setInt |**offset**=0, **value**=0 |set signed 32-bit integer at the given byte offset |
|float |**offset**=0 |get 32-bit float at the given byte offset |
|setFloat |**offset**=0, **value**=0 |set 32-bit float at the given byte offset |
|double |**offset**=0 |get 64-bit double at the given byte offset |
|setDouble |**offset**=0, **value**=0 |set 64-bit double at the given byte offset |
|utf8 |**offset**=0, **bytes**=-1 |get UTF-8 string from the given byte offset (bytes=-1 reads to end) |
|setUtf8 |**offset**=0, **value**="" |write a UTF-8 string at the given byte offset; returns bytes written |
