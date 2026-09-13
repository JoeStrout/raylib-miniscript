// physics.ms
// A 2D rigid-body physics engine, in MiniScript.
//
// Bodies are made of one or more shapes -- circles and convex polygons (boxes
// are just four-sided polygons) -- so a body with several shapes is a compound
// collider.  Collisions are resolved with sequential impulses, in the manner of
// Erin Catto's Box2D-Lite.
//
// Usage:
//	import "physics"
//	world = physics.World.make(0, 980)
//	ground = world.addBody(400, 580, physics.STATIC)
//	ground.addBox 800, 40
//	box = world.addBody(400, 100)
//	box.addBox 50, 50
//	// then each frame:
//	world.step 1/60
//	world.draw
//
// Units are yours to choose, but the tuning defaults (slop, restitution
// threshold, gravity) assume pixels, with y pointing down the screen.
//
// Data layout: per-body state lives in Matrix rows, one row per body, so that
// integration is a handful of bulk operations no matter how many bodies there
// are.  The per-shape, per-pair and per-contact work cannot be done that way.
// Shapes are rows of a matrix too (and polygon vertices rows of another), and
// the work on them -- geometry, broadphase, narrowphase, solver -- is done by
// physicsCore: native code if the host has it, or the same API in MiniScript
// (physicsFallback.ms) if not.

import "matrixUtil"

// physicsCore does the per-contact work: the narrowphase and the solver.  It
// is native code if this host has the physicsCore intrinsic; if not, the
// same API in MiniScript, from physicsFallback.ms.
if not intrinsics.hasIndex("physicsCore") then
	import "physicsFallback"
	physicsCore = physicsFallback
end if

// Shape kinds
CIRCLE = 0
POLYGON = 1

// Body types
STATIC = 0      // never moves (unless you move it)
KINEMATIC = 1   // moves at its set velocity; unaffected by forces or collisions
DYNAMIC = 2     // moved by forces and collisions

//----------------------------------------------------------------------
// Geometry helpers
//----------------------------------------------------------------------

// Convex hull of a list of [x, y] points, by Andrew's monotone chain.
// Returns the hull as a list of [x, y] in positive-area order (counter-
// clockwise with y up; clockwise on a y-down screen), with collinear points
// removed.  This lets addPolygon accept points in any order.
convexHull = function(points)
	pts = points[:]
	// insertion sort by x, then y (hulls are small)
	for i in range(1, pts.len - 1, 1)
		p = pts[i]
		j = i - 1
		while j >= 0 and (pts[j][0] > p[0] or (pts[j][0] == p[0] and pts[j][1] > p[1]))
			pts[j + 1] = pts[j]
			j -= 1
		end while
		pts[j + 1] = p
	end for
	cross = function(o, a, b)
		return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
	end function
	lower = []
	for p in pts
		while lower.len >= 2 and cross(lower[-2], lower[-1], p) <= 0
			lower.pop
		end while
		lower.push p
	end for
	upper = []
	for i in range(pts.len - 1, 0, -1)
		p = pts[i]
		while upper.len >= 2 and cross(upper[-2], upper[-1], p) <= 0
			upper.pop
		end while
		upper.push p
	end for
	lower.pop
	upper.pop
	return lower + upper
end function

//----------------------------------------------------------------------
// Shapes
//
// A shape map keeps its geometry as given, relative to the body's origin:
// ox/oy and radius for a circle, px/py (vertices) and nx/ny (edge normals)
// for a polygon.  Once it is added to a body, what the engine works with is
// its row in the world's shape matrix, and (for a polygon) its rows in the
// vertex matrix -- see World.  That is where its world-space geometry is;
// read it with worldCenter, worldVertices and bounds.
//
// Set friction, restitution, category and mask before adding the shape to a
// body, or use the set methods after; assigning them after has no effect.
//----------------------------------------------------------------------

Shape = {}
Shape.kind = null
Shape.body = null
Shape.id = 0
Shape.row = -1          // row in the world's shape matrix
Shape.density = 1
Shape.friction = 0.4
Shape.restitution = 0
Shape.category = 1      // collision filtering: two shapes collide only if each
Shape.mask = 65535      // one's category has a bit in common with the other's mask
Shape.color = null      // for debug drawing; null for the default
Shape.data = null       // for your own use

// Write one column of this shape's row in the shape matrix, if it has one.
Shape.setColumn = function(column, value)
	if self.body == null or self.body.world == null then return
	self.body.world.shapeData.setElem self.row, column, value
end function

Shape.setFriction = function(friction)
	self.friction = friction
	self.setColumn physicsCore.ShapeFriction, friction
end function

Shape.setRestitution = function(restitution)
	self.restitution = restitution
	self.setColumn physicsCore.ShapeRestitution, restitution
end function

Shape.setFilter = function(category=1, mask=65535)
	self.category = category; self.mask = mask
	self.setColumn physicsCore.ShapeCategory, category
	self.setColumn physicsCore.ShapeMask, mask
end function

// World bounding box, as [minX, minY, maxX, maxY].
Shape.bounds = function
	w = self.body.world
	w.refreshGeometry
	C = physicsCore
	r = w.shapeData.getRow(self.row)
	return [r[C.ShapeMinX], r[C.ShapeMinY], r[C.ShapeMaxX], r[C.ShapeMaxY]]
end function

Circle = new Shape
Circle.kind = CIRCLE
Circle.radius = 1
Circle.ox = 0; Circle.oy = 0    // center, relative to body origin

Circle.make = function(radius, ox=0, oy=0, density=1)
	c = new Circle
	c.radius = radius
	c.ox = ox; c.oy = oy
	c.density = density
	return c
end function

// Center in world coordinates, as [x, y].
Circle.worldCenter = function
	w = self.body.world
	w.refreshGeometry
	r = w.shapeData.getRow(self.row)
	return [r[physicsCore.ShapeCx], r[physicsCore.ShapeCy]]
end function

// Returns [mass, centroidX, centroidY, inertia about the body origin].
Circle.massInfo = function
	m = self.density * pi * self.radius^2
	return [m, self.ox, self.oy, m * (0.5 * self.radius^2 + self.ox^2 + self.oy^2)]
end function

// Store the center relative to the body's center of mass (lcx, lcy).
Circle.setCenterOfMass = function(lcx, lcy)
	self.setColumn physicsCore.ShapeLx, self.ox - lcx
	self.setColumn physicsCore.ShapeLy, self.oy - lcy
end function

Polygon = new Shape
Polygon.kind = POLYGON
Polygon.count = 0
Polygon.vertStart = 0   // first row in the world's vertex matrix

// Make a polygon from a list of [x, y] points relative to the body origin.
// The points may be in any order; their convex hull is used.
Polygon.make = function(points, density=1)
	hull = convexHull(points)
	if hull.len < 3 then return err("Polygon needs at least 3 non-collinear points")
	p = new Polygon
	p.density = density
	n = hull.len
	p.count = n
	p.indexes = range(0, n - 1)
	p.px = []; p.py = []; p.nx = []; p.ny = []
	for i in p.indexes
		p.px.push hull[i][0]
		p.py.push hull[i][1]
	end for
	for i in p.indexes
		j = (i + 1) % n
		ex = p.px[j] - p.px[i]
		ey = p.py[j] - p.py[i]
		len = sqrt(ex*ex + ey*ey)
		p.nx.push ey / len
		p.ny.push -ex / len
	end for
	return p
end function

// Vertices in world coordinates, as a list of [x, y].
Polygon.worldVertices = function
	w = self.body.world
	w.refreshGeometry
	C = physicsCore
	result = []
	for i in range(self.vertStart, self.vertStart + self.count - 1)
		v = w.vertData.getRow(i)
		result.push [v[C.VertWx], v[C.VertWy]]
	end for
	return result
end function

// A width x height box centered on (ox, oy), rotated by angle.
Polygon.makeBox = function(width, height, ox=0, oy=0, angle=0, density=1)
	hw = width / 2; hh = height / 2
	c = cos(angle); s = sin(angle)
	pts = []
	for corner in [[-hw, -hh], [hw, -hh], [hw, hh], [-hw, hh]]
		pts.push [ox + c*corner[0] - s*corner[1], oy + s*corner[0] + c*corner[1]]
	end for
	return Polygon.make(pts, density)
end function

// Returns [mass, centroidX, centroidY, inertia about the body origin].
// (The triangle-fan method from Box2D.)
Polygon.massInfo = function
	px = self.px; py = self.py
	sx = px[0]; sy = py[0]
	area = 0; cx = 0; cy = 0; inertia = 0
	for i in range(1, self.count - 2)
		e1x = px[i] - sx; e1y = py[i] - sy
		e2x = px[i+1] - sx; e2y = py[i+1] - sy
		d = e1x*e2y - e1y*e2x
		triArea = 0.5 * d
		area += triArea
		cx += triArea * (e1x + e2x) / 3
		cy += triArea * (e1y + e2y) / 3
		intx2 = e1x*e1x + e2x*e1x + e2x*e2x
		inty2 = e1y*e1y + e2y*e1y + e2y*e2y
		inertia += (0.25 / 3 * d) * (intx2 + inty2)
	end for
	m = self.density * area
	cx /= area; cy /= area
	wcx = sx + cx; wcy = sy + cy
	return [m, wcx, wcy, self.density * inertia + m * (wcx^2 + wcy^2 - cx^2 - cy^2)]
end function

// Store the vertices relative to the body's center of mass (lcx, lcy).
Polygon.setCenterOfMass = function(lcx, lcy)
	if self.body == null or self.body.world == null then return
	vd = self.body.world.vertData
	for i in self.indexes
		vd.setElem self.vertStart + i, physicsCore.VertLx, self.px[i] - lcx
		vd.setElem self.vertStart + i, physicsCore.VertLy, self.py[i] - lcy
	end for
end function

//----------------------------------------------------------------------
// Body
//
// A body's position, angle, velocity and so on live in its world's state
// matrices (see World), in row self.index.  Read them through the methods
// below -- b.x, b.y, b.angle, b.vx, b.vy, b.angularVelocity -- and change them
// with the set methods.  Don't assign b.x etc. directly; that would just hide
// the method.
//
// The body's position is its origin, the point its shapes were placed
// relative to.  Internally the engine tracks the center of mass instead;
// lcx/lcy is the center of mass relative to the origin, in the body's frame.
//----------------------------------------------------------------------

Body = {}
Body.world = null
Body.index = -1
Body.id = 0
Body.type = DYNAMIC
Body.shapes = null
Body.mass = 0
Body.inertia = 0
Body.lcx = 0; Body.lcy = 0
Body.fixedRotation = false
Body.gravityScale = 1
Body.data = null        // for your own use

Body.addShape = function(shape)
	w = self.world
	C = physicsCore
	shape.body = self
	shape.id = w.nextId
	w.nextId += 1
	w.shapeById[shape.id] = shape
	self.shapes.push shape
	shape.row = w.shapes.len
	w.shapes.push shape
	// its row in the shape matrix...
	row = [0] * C.ShapeColumns
	row[C.ShapeKind] = shape.kind
	row[C.ShapeBody] = self.index
	row[C.ShapeId] = shape.id
	row[C.ShapeFriction] = shape.friction
	row[C.ShapeRestitution] = shape.restitution
	row[C.ShapeCategory] = shape.category
	row[C.ShapeMask] = shape.mask
	if shape.kind == CIRCLE then
		row[C.ShapeRadius] = shape.radius
	else
		// ...and a polygon's in the vertex matrix
		shape.vertStart = w.vertData.rows
		row[C.ShapeVertStart] = shape.vertStart
		row[C.ShapeVertCount] = shape.count
		w.vertData.resize shape.vertStart + shape.count, C.VertColumns
		for i in shape.indexes
			v = [0] * C.VertColumns
			v[C.VertNx] = shape.nx[i]; v[C.VertNy] = shape.ny[i]
			w.vertData.setRow shape.vertStart + i, v
		end for
	end if
	w.shapeData.resize shape.row + 1, C.ShapeColumns
	w.shapeData.setRow shape.row, row
	self.updateMass     // (also fills in the positions relative to the center of mass)
	return shape
end function

Body.addCircle = function(radius, ox=0, oy=0, density=1)
	return self.addShape(Circle.make(radius, ox, oy, density))
end function

Body.addBox = function(width, height, ox=0, oy=0, angle=0, density=1)
	return self.addShape(Polygon.makeBox(width, height, ox, oy, angle, density))
end function

Body.addPolygon = function(points, density=1)
	p = Polygon.make(points, density)
	if p isa error then return p
	return self.addShape(p)
end function

// Recompute mass, center of mass and rotational inertia from the shapes.
// The body's origin stays where it is.
Body.updateMass = function
	ox = self.x; oy = self.y
	m = 0; mx = 0; my = 0; inertia = 0
	for s in self.shapes
		info = s.massInfo
		m += info[0]
		mx += info[0] * info[1]
		my += info[0] * info[2]
		inertia += info[3]
	end for
	if m > 0 then
		self.lcx = mx / m; self.lcy = my / m
		inertia -= m * (self.lcx^2 + self.lcy^2)
	else
		self.lcx = 0; self.lcy = 0
	end if
	self.mass = m
	self.inertia = inertia
	for s in self.shapes
		s.setCenterOfMass self.lcx, self.lcy
	end for
	self.setPosition ox, oy
	self.updateInvMass
end function

Body.updateInvMass = function
	w = self.world; i = self.index
	im = 0; ii = 0
	if self.type == DYNAMIC then
		if self.mass > 0 then im = 1 / self.mass else im = 1
		if self.inertia > 0 and not self.fixedRotation then ii = 1 / self.inertia
	end if
	w.invMass.setRow i, [im, im, ii]
	w.gravMask.setElem i, 0, self.gravityScale * (self.type == DYNAMIC)
end function

Body.setType = function(type)
	self.type = type
	if type == STATIC then self.world.vel.setRow self.index, 0
	self.updateInvMass
	self.world.movingMask.setElem self.index, 0, type != STATIC
	self.world.markDirty self
end function

Body.setFixedRotation = function(fixed=true)
	self.fixedRotation = fixed
	if fixed then self.world.vel.setElem self.index, 2, 0
	self.updateInvMass
end function

Body.setGravityScale = function(scale)
	self.gravityScale = scale
	self.updateInvMass
end function

Body.setDamping = function(linear=0, angular=0)
	self.world.damping.setRow self.index, [linear, linear, angular]
	if linear != 0 or angular != 0 then self.world.hasDamping = true
end function

// Position of the body origin, as [x, y].
Body.position = function
	p = self.world.pos.getRow(self.index)
	c = cos(p[2]); s = sin(p[2])
	return [p[0] - (c*self.lcx - s*self.lcy), p[1] - (s*self.lcx + c*self.lcy)]
end function

Body.x = function
	return self.position[0]
end function

Body.y = function
	return self.position[1]
end function

Body.angle = function
	return self.world.pos.getElem(self.index, 2)
end function

// Center of mass in world coordinates, as [x, y].
Body.centerOfMass = function
	p = self.world.pos.getRow(self.index)
	return [p[0], p[1]]
end function

Body.velocity = function
	v = self.world.vel.getRow(self.index)
	return [v[0], v[1]]
end function

Body.vx = function
	return self.world.vel.getElem(self.index, 0)
end function

Body.vy = function
	return self.world.vel.getElem(self.index, 1)
end function

Body.angularVelocity = function
	return self.world.vel.getElem(self.index, 2)
end function

// Move the body so its origin is at (x, y).
Body.setPosition = function(x, y)
	a = self.angle
	c = cos(a); s = sin(a)
	self.world.pos.setRow self.index, [x + c*self.lcx - s*self.lcy, y + s*self.lcx + c*self.lcy, a]
	self.world.markDirty self
end function

// Rotate the body about its origin.
Body.setAngle = function(angle)
	o = self.position
	c = cos(angle); s = sin(angle)
	self.world.pos.setRow self.index, [o[0] + c*self.lcx - s*self.lcy, o[1] + s*self.lcx + c*self.lcy, angle]
	self.world.markDirty self
end function

Body.setVelocity = function(vx, vy)
	self.world.vel.setElem self.index, 0, vx
	self.world.vel.setElem self.index, 1, vy
end function

Body.setAngularVelocity = function(w)
	self.world.vel.setElem self.index, 2, w
end function

// Velocity of a world point (px, py) attached to the body, as [vx, vy].
Body.velocityAt = function(px, py)
	p = self.world.pos.getRow(self.index)
	v = self.world.vel.getRow(self.index)
	return [v[0] - v[2] * (py - p[1]), v[1] + v[2] * (px - p[0])]
end function

// Apply a force (for the next step) at world point (px, py), or at the
// center of mass if no point is given.
Body.applyForce = function(fx, fy, px=null, py=null)
	f = self.world.force; i = self.index
	f.setElem i, 0, f.getElem(i, 0) + fx
	f.setElem i, 1, f.getElem(i, 1) + fy
	if px != null then
		p = self.world.pos.getRow(i)
		f.setElem i, 2, f.getElem(i, 2) + (px - p[0]) * fy - (py - p[1]) * fx
	end if
end function

Body.applyTorque = function(torque)
	f = self.world.force; i = self.index
	f.setElem i, 2, f.getElem(i, 2) + torque
end function

// Apply an impulse (an immediate change in momentum) at world point (px, py),
// or at the center of mass if no point is given.
Body.applyImpulse = function(ix, iy, px=null, py=null)
	if self.type != DYNAMIC then return
	v = self.world.vel.getRow(self.index)
	im = self.world.invMass.getRow(self.index)
	v[0] += im[0] * ix
	v[1] += im[1] * iy
	if px != null then
		p = self.world.pos.getRow(self.index)
		v[2] += im[2] * ((px - p[0]) * iy - (py - p[1]) * ix)
	end if
	self.world.vel.setRow self.index, v
end function

Body.applyAngularImpulse = function(impulse)
	if self.type != DYNAMIC then return
	i = self.index
	self.world.vel.setElem i, 2, self.world.vel.getElem(i, 2) +
	  self.world.invMass.getElem(i, 2) * impulse
end function

//----------------------------------------------------------------------
// World
//
// State matrices, one row per body (row i belongs to bodies[i]):
//	pos      [x, y, angle] of the center of mass
//	vel      [vx, vy, angularVelocity]
//	invMass  [1/mass, 1/mass, 1/inertia] (all 0 for static and kinematic)
//	force    [fx, fy, torque], accumulated for the next step
//	damping  [linear, linear, angular] damping rates
//	gravMask [gravityScale], or 0 for non-dynamic bodies (one column)
//	movingMask  1 for non-static bodies (one column)
//	dirtyMask   1 for bodies whose shapes need their world geometry
//	            refreshed before the next step (one column)
//
// Shapes are rows of shapeData (row i belongs to shapes[i]), and polygon
// vertices rows of vertData, in the columns named by physicsCore.ShapeKind
// etc. and physicsCore.VertLx etc.
//
// Contacts from the last step are the rows of contactMatrix, one per contact
// point, in the columns named by physicsCore.ColBodyA etc.  World.manifolds
// gives them as manifold maps instead.
//----------------------------------------------------------------------

World = {}
World.gravity = [0, 980]
World.iterations = 10           // velocity solver iterations per step
World.biasFactor = 0.2          // fraction of overlap corrected per step
World.slop = 0.5                // overlap allowed without correction
World.contactMargin = 2         // shapes this close count as (speculatively) touching
World.restitutionThreshold = 30 // slower impacts than this don't bounce
World.warmStarting = true

World.make = function(gravityX=0, gravityY=980)
	w = new World
	w.gravity = [gravityX, gravityY]
	w.bodies = []
	w.shapes = []
	w.nextId = 1
	w.pos = Matrix.ofSize(0, 3)
	w.vel = Matrix.ofSize(0, 3)
	w.invMass = Matrix.ofSize(0, 3)
	w.force = Matrix.ofSize(0, 3)
	w.damping = Matrix.ofSize(0, 3)
	w.gravMask = Matrix.ofSize(0, 1)
	w.movingMask = Matrix.ofSize(0, 1)
	w.dirtyMask = Matrix.ofSize(0, 1)
	w.anyDirty = false
	w.hasDamping = false
	w.shapeById = {}
	w.shapeData = Matrix.ofSize(0, physicsCore.ShapeColumns)
	w.vertData = Matrix.ofSize(0, physicsCore.VertColumns)
	w.pairMatrix = Matrix.ofSize(0, 2)      // broadphase pairs of shape rows
	// this step's contacts and last step's, swapped each step
	w.contactMatrix = Matrix.ofSize(0, physicsCore.ContactColumns)
	w.prevContactMatrix = Matrix.ofSize(0, physicsCore.ContactColumns)
	w.manifoldCount = 0         // touching shape pairs, from the last step
	w.contactPointCount = 0     // contact points, from the last step
	w.pairCount = 0     // broadphase pairs, from the last step
	w.stepCount = 0
	w.resetProfile
	return w
end function

World.stateMatrices = function
	return [self.pos, self.vel, self.invMass, self.force, self.damping, self.gravMask,
	  self.movingMask, self.dirtyMask]
end function

// Add a body with its origin at (x, y).  Give it shapes with addCircle,
// addBox, addPolygon or addShape.
World.addBody = function(x=0, y=0, type=null, angle=0)
	if type == null then type = DYNAMIC
	b = new Body
	b.world = self
	b.id = self.nextId
	self.nextId += 1
	b.type = type
	b.shapes = []
	b.index = self.bodies.len
	self.bodies.push b
	for m in self.stateMatrices
		m.resize b.index + 1, m.columns
	end for
	self.pos.setRow b.index, [x, y, angle]
	self.movingMask.setElem b.index, 0, type != STATIC
	b.updateInvMass
	return b
end function

// Note that body b has moved (or changed) other than by stepping, so its
// shapes' world geometry must be refreshed.
World.markDirty = function(b)
	self.dirtyMask.setElem b.index, 0, 1
	self.anyDirty = true
end function

// Refresh the world geometry of any bodies marked dirty.  The world does
// this itself when it needs to; you don't need to call it.
World.refreshGeometry = function
	if not self.anyDirty then return
	physicsCore.updateGeometry self.shapeData, self.vertData, self.pos, self.dirtyMask
	self.dirtyMask.fill 0
	self.anyDirty = false
end function

// Convenience: a body with a single box, circle, or polygon.
World.addBox = function(x, y, width, height, type=null, angle=0)
	b = self.addBody(x, y, type, angle)
	b.addBox width, height
	return b
end function

World.addCircle = function(x, y, radius, type=null)
	b = self.addBody(x, y, type)
	b.addCircle radius
	return b
end function

World.addPolygon = function(x, y, points, type=null, angle=0)
	b = self.addBody(x, y, type, angle)
	b.addPolygon points
	return b
end function

World.removeBody = function(b)
	if not refEquals(b.world, self) then return
	for s in b.shapes
		self.removeShapeRows s
		self.shapeById.remove s.id
	end for
	// Remove the body's rows, moving the last body into its place.
	i = b.index
	last = self.bodies.len - 1
	for m in self.stateMatrices
		m.removeRowFast i
	end for
	if i < last then
		moved = self.bodies[last]
		self.bodies[i] = moved
		moved.index = i
		for s in moved.shapes
			self.shapeData.setElem s.row, physicsCore.ShapeBody, i
		end for
	end if
	self.bodies.pop
	b.world = null
	b.index = -1
end function

// Remove shape s's rows from shapeData and vertData (and s from shapes).
World.removeShapeRows = function(s)
	C = physicsCore
	if s.kind == POLYGON then
		// Close up the gap in the vertex matrix, and move the later
		// polygons' vertex ranges down to match.
		start = s.vertStart
		for k in range(1, s.count)
			self.vertData.removeRow start
		end for
		for other in self.shapes
			if other.kind == POLYGON and other.vertStart > start then
				other.vertStart -= s.count
				self.shapeData.setElem other.row, C.ShapeVertStart, other.vertStart
			end if
		end for
	end if
	// Move the last shape into this one's row.
	r = s.row
	last = self.shapes.len - 1
	self.shapeData.removeRowFast r
	if r < last then
		moved = self.shapes[last]
		self.shapes[r] = moved
		moved.row = r
	end if
	self.shapes.pop
	s.row = -1
end function

World.resetProfile = function
	self.profile = {"geometry": 0, "broadphase": 0, "narrowphase": 0, "solver": 0, "integrate": 0, "steps": 0}
end function

// Advance the simulation by dt seconds.
World.step = function(dt)
	if dt <= 0 or not self.bodies then return
	prof = self.profile
	t0 = time

	// Bring the geometry of any bodies moved by hand up to date.
	pos = self.pos
	self.refreshGeometry
	t1 = time

	// Find what's touching.
	self.pairCount = physicsCore.findPairs(self.shapeData, self.invMass,
	  self.contactMargin, self.pairMatrix)
	tb = time
	cm = self.prevContactMatrix
	self.prevContactMatrix = self.contactMatrix
	self.contactMatrix = cm
	prev = null
	if self.warmStarting then prev = self.prevContactMatrix
	self.manifoldCount = physicsCore.collidePairs(self.pairMatrix, self.shapeData,
	  self.vertData, self.contactMargin, prev, cm)
	self.contactPointCount = cm.rows
	tn = time

	// Integrate forces and gravity into velocities (bulk).
	vel = self.vel
	self.force.elemMultiplyBy self.invMass
	vel.addScaled self.force, dt
	self.force.fill 0
	g = self.gravity
	self.gravMask.gemm Matrix.fromList([[g[0], g[1], 0]]), vel, vel, false, false, dt
	if self.hasDamping then
		// v *= 1 / (1 + dt * damping)
		f = self.damping.clone
		f.elemMultiplyBy dt
		f.add 1
		f.pow -1
		vel.elemMultiplyBy f
	end if
	ti = time

	// Solve the contacts.
	physicsCore.solveContacts self.contactMatrix, pos, vel, self.invMass, dt,
	  self.iterations, self.biasFactor, self.slop, self.restitutionThreshold
	ts = time

	// Integrate velocities into positions (bulk).
	pos.addScaled vel, dt
	t2 = time

	// Update the geometry of everything that moved.
	physicsCore.updateGeometry self.shapeData, self.vertData, pos, self.movingMask
	t3 = time

	prof.geometry += (t1 - t0) + (t3 - t2)
	prof.broadphase += tb - t1
	prof.narrowphase += tn - tb
	prof.integrate += (ti - tn) + (t2 - ts)
	prof.solver += ts - ti
	prof.steps += 1
	self.stepCount += 1
end function

// Print average milliseconds per step for each phase since the last
// resetProfile.
World.printProfile = function
	n = self.profile.steps
	if not n then return
	total = 0
	for kv in self.profile
		if kv.key == "steps" then continue
		print ("  " + kv.key + "            ")[:14] + round(kv.value * 1000 / n, 3) + " ms"
		total += kv.value
	end for
	print "  total         " + round(total * 1000 / n, 3) + " ms/step, over " + n + " steps"
end function

// The shapes containing world point (x, y), topmost (last added) first.
World.shapesAt = function(x, y)
	self.refreshGeometry
	C = physicsCore
	sd = self.shapeData; vd = self.vertData
	minX = sd.getColumn(C.ShapeMinX); minY = sd.getColumn(C.ShapeMinY)
	maxX = sd.getColumn(C.ShapeMaxX); maxY = sd.getColumn(C.ShapeMaxY)
	result = []
	for r in self.shapes.indexes
		if x < minX[r] or x > maxX[r] or y < minY[r] or y > maxY[r] then continue
		sh = self.shapes[r]
		row = sd.getRow(r)
		if sh.kind == CIRCLE then
			if (x - row[C.ShapeCx])^2 + (y - row[C.ShapeCy])^2 <= sh.radius^2 then result.push sh
		else
			inside = true
			for k in range(sh.vertStart, sh.vertStart + sh.count - 1)
				v = vd.getRow(k)
				if v[C.VertWnx] * (x - v[C.VertWx]) + v[C.VertWny] * (y - v[C.VertWy]) > 0 then
					inside = false
					break
				end if
			end for
			if inside then result.push sh
		end if
	end for
	result.sort "id", false
	return result
end function

// The topmost body with a shape containing world point (x, y), or null.
World.bodyAt = function(x, y)
	shapes = self.shapesAt(x, y)
	if not shapes then return null
	return shapes[0].body
end function

// The contacts from the last step, as manifold maps: one per touching pair
// of shapes, each {"a", "b", "ia", "ib", "nx", "ny", "friction",
// "restitution", "points"}, where a and b are the shapes, ia and ib their
// bodies' indexes, (nx, ny) the normal from a to b, and points a list of one
// or two {"x", "y", "sep", "id", "Pn", "Pt"} (see physicsCore.collidePairs).
// Made from the contact matrix on each call, so keep the result rather than
// asking again.
World.manifolds = function
	cm = self.contactMatrix
	C = physicsCore
	sa = cm.getColumn(C.ColShapeA); sb = cm.getColumn(C.ColShapeB)
	ia = cm.getColumn(C.ColBodyA); ib = cm.getColumn(C.ColBodyB)
	nx = cm.getColumn(C.ColNx); ny = cm.getColumn(C.ColNy)
	x = cm.getColumn(C.ColX); y = cm.getColumn(C.ColY); sep = cm.getColumn(C.ColSep)
	fr = cm.getColumn(C.ColFriction); re = cm.getColumn(C.ColRestitution)
	pn = cm.getColumn(C.ColPn); pt = cm.getColumn(C.ColPt)
	pair = cm.getColumn(C.ColPair); feature = cm.getColumn(C.ColFeature)
	result = []
	i = 0
	while i < cm.rows
		count = 1 + (pair[i] == 1)
		// (Either shape may have been removed since the step.)
		byId = self.shapeById
		if byId.hasIndex(sa[i]) and byId.hasIndex(sb[i]) then
			m = {"a": byId[sa[i]], "b": byId[sb[i]], "ia": ia[i], "ib": ib[i], "nx": nx[i], "ny": ny[i],
			  "friction": fr[i], "restitution": re[i], "points": []}
			for j in range(i, i + count - 1)
				m.points.push {"x": x[j], "y": y[j], "sep": sep[j], "id": feature[j],
				  "Pn": pn[j], "Pt": pt[j]}
			end for
			result.push m
		end if
		i += count
	end while
	return result
end function

// The contact manifolds (see collide) involving body b, from the last step,
// each flipped if need be so that b is shape a and the normal points away
// from b.
World.contactsOf = function(b)
	result = []
	for m in self.manifolds
		if m.a.body.id == b.id then
			result.push m
		else if m.b.body.id == b.id then
			result.push {"a": m.b, "b": m.a, "nx": -m.nx, "ny": -m.ny, "points": m.points}
		end if
	end for
	return result
end function

//----------------------------------------------------------------------
// Debug drawing (requires raylib)
//----------------------------------------------------------------------

World.staticColor = [120, 130, 140, 255]
World.kinematicColor = [110, 150, 220, 255]
World.dynamicColor = [230, 180, 90, 255]

World.draw = function
	rl = raylib
	self.refreshGeometry
	C = physicsCore
	sd = self.shapeData; vd = self.vertData
	if not sd.rows then return
	cx = sd.getColumn(C.ShapeCx); cy = sd.getColumn(C.ShapeCy)
	angle = self.pos.getColumn(2)
	if vd.rows then
		wx = vd.getColumn(C.VertWx); wy = vd.getColumn(C.VertWy)
	end if
	for r in self.shapes.indexes
		sh = self.shapes[r]
		color = sh.color
		if color == null then
			t = sh.body.type
			if t == DYNAMIC then
				color = self.dynamicColor
			else if t == KINEMATIC then
				color = self.kinematicColor
			else
				color = self.staticColor
			end if
		end if
		fill = [color[0], color[1], color[2], 70]
		if sh.kind == CIRCLE then
			center = [cx[r], cy[r]]
			rl.DrawCircleV center, sh.radius, fill
			rl.DrawCircleLinesV center, sh.radius, color
			a = angle[sh.body.index]
			rl.DrawLineV center, [cx[r] + cos(a) * sh.radius, cy[r] + sin(a) * sh.radius], color
		else
			s0 = sh.vertStart; n = sh.count
			// Our vertex order is clockwise on a y-down screen; raylib wants
			// counter-clockwise, so the fan goes backwards.
			pts = [[wx[s0], wy[s0]]]
			for k in range(s0 + n - 1, s0 + 1)
				pts.push [wx[k], wy[k]]
			end for
			rl.DrawTriangleFan pts, fill
			for k in range(0, n - 1)
				a = s0 + k; b = s0 + (k + 1) % n
				rl.DrawLineV [wx[a], wy[a]], [wx[b], wy[b]], color
			end for
		end if
	end for
end function

World.contactColor = [255, 70, 70, 255]

// Draw each contact point from the last step, with its normal (pointing from
// shape a to shape b) scaled by normalLength.
World.drawContacts = function(normalLength=12)
	rl = raylib
	for m in self.manifolds
		for c in m.points
			p = [c.x, c.y]
			rl.DrawCircleV p, 3, self.contactColor
			rl.DrawLineV p, [c.x + m.nx * normalLength, c.y + m.ny * normalLength], self.contactColor
		end for
	end for
end function
