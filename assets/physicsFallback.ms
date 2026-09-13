// physicsFallback.ms
// The physicsCore API (see src/PhysicsCore.cpp), in MiniScript: what
// physics.ms uses when the host has no physicsCore intrinsic.  It imports
// this and sets physicsCore = physicsFallback, so the two are interchangeable.
//
// Keep the two in step.  The C++ does the same arithmetic in the same order,
// so a scene gives the same result with either (physicsBench.ms checks).
//
// Everything works on Matrix values, but reads them into plain lists first
// (getColumn), which are much faster than a Matrix for one element at a time.

// Shape kinds
CIRCLE = 0
POLYGON = 1

//----------------------------------------------------------------------
// Contact matrix layout: one row per contact point.  A manifold of two
// points is two consecutive rows, with ColPair 1 on the first and 2 on the
// second; a lone point has ColPair 0.
//----------------------------------------------------------------------

ColBodyA = 0        // row index (in pos/vel/invMass) of shape a's body
ColBodyB = 1        // same, for shape b
ColNx = 2           // unit normal, pointing from a to b
ColNy = 3
ColX = 4            // contact point, in world coordinates
ColY = 5
ColSep = 6          // separation along the normal (negative = overlap)
ColFriction = 7     // mixed friction coefficient
ColRestitution = 8  // mixed restitution
ColPn = 9           // accumulated normal impulse (in: warm start; out: result)
ColPt = 10          // accumulated tangent impulse (likewise)
ColPair = 11        // 0 = lone point; 1 = first of two; 2 = second of two
ColShapeA = 12      // id of shape a
ColShapeB = 13      // id of shape b
ColFeature = 14     // which features touch, the same from step to step
ContactColumns = 15

//----------------------------------------------------------------------
// Shape matrix layout: one row per shape.
//----------------------------------------------------------------------

ShapeKind = 0           // 0 = circle, 1 = convex polygon
ShapeBody = 1           // row index (in pos/vel/invMass) of the shape's body
ShapeId = 2             // unique id (Shape.id in physics.ms)
ShapeFriction = 3
ShapeRestitution = 4
ShapeRadius = 5         // circles: radius
ShapeLx = 6             // circles: center, relative to the body's center of mass
ShapeLy = 7             //   (in the body's frame)
ShapeVertStart = 8      // polygons: first vertex row
ShapeVertCount = 9      // polygons: number of vertices
ShapeCategory = 10      // collision filtering: two shapes collide only if each
ShapeMask = 11          //   one's category has a bit in common with the other's mask
ShapeCx = 12            // circles: center, in world coordinates (from updateGeometry)
ShapeCy = 13
ShapeMinX = 14          // world bounding box (from updateGeometry)
ShapeMinY = 15
ShapeMaxX = 16
ShapeMaxY = 17
ShapeColumns = 18

//----------------------------------------------------------------------
// Vertex matrix layout: one row per polygon vertex.  A polygon's vertices
// are rows ShapeVertStart through ShapeVertStart + ShapeVertCount - 1, in
// order around it; normal k belongs to the edge from vertex k to k+1.
//----------------------------------------------------------------------

VertLx = 0              // position, relative to the body's center of mass
VertLy = 1
VertNx = 2              // edge normal, in the body's frame
VertNy = 3
VertWx = 4              // position, in world coordinates (from updateGeometry)
VertWy = 5
VertWnx = 6             // edge normal, in world coordinates (from updateGeometry)
VertWny = 7
VertColumns = 8

//----------------------------------------------------------------------
// Geometry and broadphase
//----------------------------------------------------------------------

// updateGeometry shapes, verts, pos, bodyMask
//
// Transform shapes into world coordinates (ShapeCx/Cy for circles, and the
// VertW* columns for polygons) and compute their bounding boxes, for the
// bodies whose row of bodyMask (one column, a row per body) is nonzero -- or
// for every body, if bodyMask is null.
updateGeometry = function(shapes, verts, pos, bodyMask)
	n = shapes.rows
	if n == 0 then return
	kind = shapes.getColumn(ShapeKind); body = shapes.getColumn(ShapeBody)
	rad = shapes.getColumn(ShapeRadius)
	slx = shapes.getColumn(ShapeLx); sly = shapes.getColumn(ShapeLy)
	vs = shapes.getColumn(ShapeVertStart); vc = shapes.getColumn(ShapeVertCount)
	scx = shapes.getColumn(ShapeCx); scy = shapes.getColumn(ShapeCy)
	minXs = shapes.getColumn(ShapeMinX); minYs = shapes.getColumn(ShapeMinY)
	maxXs = shapes.getColumn(ShapeMaxX); maxYs = shapes.getColumn(ShapeMaxY)
	px = pos.getColumn(0); py = pos.getColumn(1); pa = pos.getColumn(2)
	mask = null
	if bodyMask != null then mask = bodyMask.getColumn(0)
	vlx = null
	if verts.rows > 0 then
		vlx = verts.getColumn(VertLx); vly = verts.getColumn(VertLy)
		vnx = verts.getColumn(VertNx); vny = verts.getColumn(VertNy)
		wx = verts.getColumn(VertWx); wy = verts.getColumn(VertWy)
		wnx = verts.getColumn(VertWnx); wny = verts.getColumn(VertWny)
	end if
	for r in range(0, n - 1)
		i = body[r]
		if mask != null and mask[i] == 0 then continue
		x = px[i]; y = py[i]
		c = cos(pa[i]); s = sin(pa[i])
		if kind[r] == CIRCLE then
			lx = slx[r]; ly = sly[r]
			cx = x + c*lx - s*ly
			cy = y + s*lx + c*ly
			scx[r] = cx; scy[r] = cy
			radius = rad[r]
			minXs[r] = cx - radius; maxXs[r] = cx + radius
			minYs[r] = cy - radius; maxYs[r] = cy + radius
		else
			minX = 1e300; maxX = -1e300; minY = 1e300; maxY = -1e300
			for k in range(vs[r], vs[r] + vc[r] - 1)
				vx = x + c*vlx[k] - s*vly[k]
				vy = y + s*vlx[k] + c*vly[k]
				wx[k] = vx; wy[k] = vy
				if vx < minX then minX = vx
				if vx > maxX then maxX = vx
				if vy < minY then minY = vy
				if vy > maxY then maxY = vy
				wnx[k] = c*vnx[k] - s*vny[k]
				wny[k] = s*vnx[k] + c*vny[k]
			end for
			minXs[r] = minX; maxXs[r] = maxX
			minYs[r] = minY; maxYs[r] = maxY
		end if
	end for
	shapes.setColumn ShapeCx, scx; shapes.setColumn ShapeCy, scy
	shapes.setColumn ShapeMinX, minXs; shapes.setColumn ShapeMinY, minYs
	shapes.setColumn ShapeMaxX, maxXs; shapes.setColumn ShapeMaxY, maxYs
	if vlx != null then
		verts.setColumn VertWx, wx; verts.setColumn VertWy, wy
		verts.setColumn VertWnx, wnx; verts.setColumn VertWny, wny
	end if
end function

// findPairs shapes, invMass, margin, pairs -> count
//
// The broadphase: fill pairs (2 columns) with every pair of shape rows whose
// bounding boxes come within margin of each other, that are on different
// bodies, at least one dynamic (nonzero inverse mass), and pass each other's
// category/mask filter.  Each row is [a, b] with a's id less than b's, and
// rows are sorted by those ids (so the order doesn't depend on how shapes
// happen to be arranged).
findPairs = function(shapes, invMass, margin, pairs)
	n = shapes.rows
	minXs = shapes.getColumn(ShapeMinX); minYs = shapes.getColumn(ShapeMinY)
	maxXs = shapes.getColumn(ShapeMaxX); maxYs = shapes.getColumn(ShapeMaxY)
	ids = shapes.getColumn(ShapeId); body = shapes.getColumn(ShapeBody)
	cat = shapes.getColumn(ShapeCategory); msk = shapes.getColumn(ShapeMask)
	im = invMass.getColumn(0)
	// Sweep along x: sort by left edge, and for each shape look only at those
	// starting before it ends.
	order = []
	for r in range(0, n - 1, 1)
		order.push [minXs[r], r]
	end for
	order.sort 0
	found = []
	for i in range(0, n - 1, 1)
		a = order[i][1]
		aMaxX = maxXs[a] + margin; aMinY = minYs[a] - margin; aMaxY = maxYs[a] + margin
		bodyA = body[a]
		aDynamic = im[bodyA] > 0
		j = i + 1
		while j < n
			b = order[j][1]
			if minXs[b] > aMaxX then break
			j += 1
			if minYs[b] > aMaxY or maxYs[b] < aMinY then continue
			bodyB = body[b]
			if bodyB == bodyA or not (aDynamic or im[bodyB] > 0) then continue
			if not bitAnd(cat[a], msk[b]) or not bitAnd(cat[b], msk[a]) then continue
			if ids[a] < ids[b] then
				found.push [ids[a] * 1048576 + ids[b], a, b]
			else
				found.push [ids[b] * 1048576 + ids[a], b, a]
			end if
		end while
	end for
	found.sort 0
	as = []; bs = []
	for f in found
		as.push f[1]; bs.push f[2]
	end for
	pairs.resize found.len, 2
	if found then
		pairs.setColumn 0, as; pairs.setColumn 1, bs
	end if
	return found.len
end function

//----------------------------------------------------------------------
// Narrowphase
//
// The collision functions take shape rows, plus G: the shape and vertex
// columns as lists ("kind", "cx", "cy", "rad", "vs" (vertex start), "vc"
// (vertex count), "wx", "wy", "wnx", "wny").
//----------------------------------------------------------------------

// Collide shape rows a and b.  Returns null if they are more than margin
// apart, or a manifold: {"nx", "ny", "points"}, where (nx, ny) is the unit
// normal pointing from a to b, and points is a list of one or two contact
// points, each {"x", "y", "sep", "id"}: the world position (midway between
// the surfaces), the separation along the normal (negative when
// overlapping), and a feature id that is the same from step to step for the
// same pair of features.
//
// Points with a positive separation (up to margin) are "speculative": the
// solver lets the shapes close the gap, but no further.  Reporting them
// keeps resting contacts from flickering in and out as rounding puts them a
// hair apart or a hair overlapping, which would make stacks wobble.
collide = function(a, b, margin, G)
	if G.kind[a] == CIRCLE then
		if G.kind[b] == CIRCLE then return collideCircles(a, b, margin, G)
		return collidePolygonCircle(b, a, true, margin, G)
	end if
	if G.kind[b] == CIRCLE then return collidePolygonCircle(a, b, false, margin, G)
	return collidePolygons(a, b, margin, G)
end function

collideCircles = function(a, b, margin, G)
	acx = G.cx[a]; acy = G.cy[a]; bcx = G.cx[b]; bcy = G.cy[b]
	ar = G.rad[a]; br = G.rad[b]
	dx = bcx - acx; dy = bcy - acy
	r = ar + br
	rm = r + margin
	d2 = dx*dx + dy*dy
	if d2 > rm*rm then return null
	d = sqrt(d2)
	if d > 1e-9 then
		nx = dx / d; ny = dy / d
	else
		nx = 0; ny = 1
	end if
	// midway between the surface point on a and the one on b
	k = (ar - br) * 0.5
	return {"nx": nx, "ny": ny, "points": [
	  {"x": (acx + bcx) * 0.5 + nx * k, "y": (acy + bcy) * 0.5 + ny * k, "sep": d - r, "id": 0}]}
end function

// Polygon p against circle c.  The normal points from p to c, or from c to p
// if flip is true (for when the circle is shape a).
collidePolygonCircle = function(p, c, flip, margin, G)
	cx = G.cx[c]; cy = G.cy[c]; r = G.rad[c]
	rm = r + margin
	wx = G.wx; wy = G.wy; wnx = G.wnx; wny = G.wny
	s0 = G.vs[p]; count = G.vc[p]
	// the face the center is furthest outside of (or least inside)
	best = -1e30; bi = 0
	for i in range(0, count - 1)
		s = wnx[s0+i] * (cx - wx[s0+i]) + wny[s0+i] * (cy - wy[s0+i])
		if s > rm then return null
		if s > best then
			best = s; bi = i
		end if
	end for
	bi2 = bi + 1
	if bi2 == count then bi2 = 0
	v1x = wx[s0+bi]; v1y = wy[s0+bi]; v2x = wx[s0+bi2]; v2y = wy[s0+bi2]
	id = bi
	if best > 1e-9 then
		// Center is outside the polygon: is it nearest a vertex or the face?
		u1 = (cx - v1x) * (v2x - v1x) + (cy - v1y) * (v2y - v1y)
		u2 = (cx - v2x) * (v1x - v2x) + (cy - v2y) * (v1y - v2y)
		if u1 <= 0 or u2 <= 0 then
			if u1 > 0 then
				v1x = v2x; v1y = v2y; id = bi2
			end if
			dx = cx - v1x; dy = cy - v1y
			d2 = dx*dx + dy*dy
			if d2 > rm*rm then return null
			d = sqrt(d2)
			nx = dx / d; ny = dy / d
			px = (v1x + cx - nx * r) * 0.5
			py = (v1y + cy - ny * r) * 0.5
			if flip then
				nx = -nx; ny = -ny
			end if
			return {"nx": nx, "ny": ny, "points": [{"x": px, "y": py, "sep": d - r, "id": 100 + id}]}
		end if
	end if
	// Nearest the face (or inside the polygon): use the face normal.
	nx = wnx[s0+bi]; ny = wny[s0+bi]
	px = cx - nx * (best + r) * 0.5
	py = cy - ny * (best + r) * 0.5
	if flip then
		nx = -nx; ny = -ny
	end if
	return {"nx": nx, "ny": ny, "points": [{"x": px, "y": py, "sep": best - r, "id": id}]}
end function

// The edge of p1 along whose normal p2 is most separated from p1 (least
// overlapping).  Returns [separation, edge index]; stops early at the first
// separation over margin, which proves the polygons are too far apart.
findMaxSeparation = function(p1, p2, margin, G)
	wx = G.wx; wy = G.wy; wnx = G.wnx; wny = G.wny
	s1 = G.vs[p1]; s2 = G.vs[p2]
	idx2 = range(s2, s2 + G.vc[p2] - 1)
	best = -1e30; bestEdge = 0
	for i in range(0, G.vc[p1] - 1)
		nx = wnx[s1+i]; ny = wny[s1+i]
		ox = wx[s1+i]; oy = wy[s1+i]
		si = 1e30
		for j in idx2
			d = nx * (wx[j] - ox) + ny * (wy[j] - oy)
			if d < si then si = d
		end for
		if si > best then
			best = si; bestEdge = i
			if best > margin then break
		end if
	end for
	return [best, bestEdge]
end function

// Keep the part of segment seg = [x0, y0, id0, x1, y1, id1] on the inner side
// of the line {p : nx*px + ny*py = offset}.  A point made by the cut gets id
// clipId.  Returns the clipped segment, or null if less than two points are
// left.
clipSegment = function(seg, nx, ny, offset, clipId)
	x0 = seg[0]; y0 = seg[1]; x1 = seg[3]; y1 = seg[4]
	d0 = nx * x0 + ny * y0 - offset
	d1 = nx * x1 + ny * y1 - offset
	out = []
	if d0 <= 0 then out = [x0, y0, seg[2]]
	if d1 <= 0 then out = out + [x1, y1, seg[5]]
	if d0 * d1 < 0 then
		t = d0 / (d0 - d1)
		out = out + [x0 + t * (x1 - x0), y0 + t * (y1 - y0), clipId]
	end if
	if out.len < 6 then return null
	return out
end function

// Polygon against polygon, by the separating axis test, then clipping the
// incident edge against the reference face (as in Box2D).
collidePolygons = function(a, b, margin, G)
	sepA = findMaxSeparation(a, b, margin, G)
	if sepA[0] > margin then return null
	sepB = findMaxSeparation(b, a, margin, G)
	if sepB[0] > margin then return null
	// The reference face is the one of least overlap, with a slight
	// preference for a, so it doesn't flip back and forth between steps.
	if sepB[0] > sepA[0] + 0.05 * margin then
		ref = b; inc = a; e1 = sepB[1]; flip = true
	else
		ref = a; inc = b; e1 = sepA[1]; flip = false
	end if
	wx = G.wx; wy = G.wy; wnx = G.wnx; wny = G.wny
	rs = G.vs[ref]; ics = G.vs[inc]
	e2 = e1 + 1
	if e2 == G.vc[ref] then e2 = 0
	v1x = wx[rs+e1]; v1y = wy[rs+e1]
	v2x = wx[rs+e2]; v2y = wy[rs+e2]
	nx = wnx[rs+e1]; ny = wny[rs+e1]
	tx = -ny; ty = nx     // along the reference edge, from v1 to v2

	// The incident edge is the edge of inc most opposed to the reference normal.
	minDot = 1e30; i1 = 0
	incCount = G.vc[inc]
	for i in range(0, incCount - 1)
		d = nx * wnx[ics+i] + ny * wny[ics+i]
		if d < minDot then
			minDot = d; i1 = i
		end if
	end for
	i2 = i1 + 1
	if i2 == incCount then i2 = 0
	seg = [wx[ics+i1], wy[ics+i1], i1, wx[ics+i2], wy[ics+i2], i2]

	// Clip it to the sides of the reference face.
	seg = clipSegment(seg, -tx, -ty, -(tx * v1x + ty * v1y), 500 + i1)
	if seg == null then return null
	seg = clipSegment(seg, tx, ty, tx * v2x + ty * v2y, 600 + i1)
	if seg == null then return null

	// Keep the points that are behind the reference face (or within margin).
	front = nx * v1x + ny * v1y
	idBase = flip * 1000000 + e1 * 1000
	points = []
	for k in [0, 3]
		px = seg[k]; py = seg[k + 1]
		sep = nx * px + ny * py - front
		if sep <= margin then
			points.push {"x": px - 0.5 * sep * nx, "y": py - 0.5 * sep * ny,
			  "sep": sep, "id": idBase + seg[k + 2]}
		end if
	end for
	if not points then return null
	if flip then
		nx = -nx; ny = -ny
	end if
	return {"nx": nx, "ny": ny, "points": points}
end function

// collidePairs pairs, shapes, verts, margin, prevContacts, contacts -> touching
//
// Collide each pair of shape rows in pairs (from findPairs; their world
// geometry must be current), and fill contacts with one row per contact
// point.  Points that match one in prevContacts (same shapes, same features)
// start with its ColPn/ColPt, for warm starting; pass null for prevContacts
// to start them all at 0.  Returns how many pairs touch.
collidePairs = function(pairs, shapes, verts, margin, prevContacts, contacts)
	// Last step's impulses: warm[pair key][feature] = [Pn, Pt]
	warm = {}
	if prevContacts != null and prevContacts.rows > 0 then
		sa = prevContacts.getColumn(ColShapeA); sb = prevContacts.getColumn(ColShapeB)
		ft = prevContacts.getColumn(ColFeature)
		pn = prevContacts.getColumn(ColPn); pt = prevContacts.getColumn(ColPt)
		for i in range(0, prevContacts.rows - 1)
			key = sa[i] * 1048576 + sb[i]
			if not warm.hasIndex(key) then warm[key] = {}
			features = warm[key]
			if not features.hasIndex(ft[i]) then features[ft[i]] = [pn[i], pt[i]]
		end for
	end if

	G = {}
	G.kind = shapes.getColumn(ShapeKind)
	G.cx = shapes.getColumn(ShapeCx); G.cy = shapes.getColumn(ShapeCy)
	G.rad = shapes.getColumn(ShapeRadius)
	G.vs = shapes.getColumn(ShapeVertStart); G.vc = shapes.getColumn(ShapeVertCount)
	if verts.rows > 0 then
		G.wx = verts.getColumn(VertWx); G.wy = verts.getColumn(VertWy)
		G.wnx = verts.getColumn(VertWnx); G.wny = verts.getColumn(VertWny)
	end if
	body = shapes.getColumn(ShapeBody); ids = shapes.getColumn(ShapeId)
	fr = shapes.getColumn(ShapeFriction); re = shapes.getColumn(ShapeRestitution)

	// Build the rows a column at a time.
	cols = []
	for c in range(0, ContactColumns - 1)
		cols.push []
	end for
	touching = 0
	pa = []; pb = []
	if pairs.rows > 0 then
		pa = pairs.getColumn(0); pb = pairs.getColumn(1)
	end if
	for p in range(0, pa.len - 1, 1)
		a = pa[p]; b = pb[p]
		m = collide(a, b, margin, G)
		if m == null then continue
		touching += 1
		friction = sqrt(fr[a] * fr[b])
		restitution = re[a]
		if re[b] > restitution then restitution = re[b]
		key = ids[a] * 1048576 + ids[b]
		old = null
		if warm.hasIndex(key) then old = warm[key]
		count = m.points.len
		k = 0
		for c in m.points
			k += 1
			Pn = 0; Pt = 0
			if old != null and old.hasIndex(c.id) then
				Pn = old[c.id][0]; Pt = old[c.id][1]
			end if
			cols[ColBodyA].push body[a]
			cols[ColBodyB].push body[b]
			cols[ColNx].push m.nx
			cols[ColNy].push m.ny
			cols[ColX].push c.x
			cols[ColY].push c.y
			cols[ColSep].push c.sep
			cols[ColFriction].push friction
			cols[ColRestitution].push restitution
			cols[ColPn].push Pn
			cols[ColPt].push Pt
			cols[ColPair].push k * (count == 2)
			cols[ColShapeA].push ids[a]
			cols[ColShapeB].push ids[b]
			cols[ColFeature].push c.id
		end for
	end for
	contacts.resize cols[0].len, ContactColumns
	if cols[0] then
		for c in range(0, ContactColumns - 1)
			contacts.setColumn c, cols[c]
		end for
	end if
	return touching
end function

//----------------------------------------------------------------------
// Contact solver
//----------------------------------------------------------------------

// solveContacts contacts, pos, vel, invMass, dt, iterations, biasFactor, slop,
//   restThreshold
//
// The whole contact solver for one step.  First, for each contact point,
// work out its lever arms (r1 from body a's center of mass, r2 from b's),
// its effective masses along the normal and the tangent, and its velocity
// bias: the separating speed the solver aims for, which closes gaps, pushes
// overlaps apart, and makes things bounce.  For two-point manifolds, also
// work out the 2x2 matrix that lets the solver handle both points at once.
// Then apply the impulses already in ColPn/ColPt (warm starting), and run
// `iterations` passes of sequential impulses.
//
// Each pass visits every manifold and applies, at each point, the friction
// impulse that would stop the shapes sliding, then the normal impulses that
// would stop them approaching (and meet the bias).  Running totals are
// clamped so that contacts only push, and friction never exceeds friction *
// normal impulse.  Friction goes first because not overlapping matters more
// than not sliding.  The two normal impulses of a two-point manifold are
// solved together (the "block solver" from Box2D), since solving them one
// after the other leaves a lopsided error each step that makes stacks lean.
//
// Updates vel, and contacts' ColPn and ColPt, in place.
solveContacts = function(contacts, pos, vel, invMass, dt, iterations=10, biasFactor=0.2, slop=0.5, restThreshold=30)
	n = contacts.rows
	if n == 0 then return
	// Everything as plain lists, which are much faster than a Matrix for one
	// element at a time.
	ca = contacts.getColumn(ColBodyA); cb = contacts.getColumn(ColBodyB)
	cnx = contacts.getColumn(ColNx); cny = contacts.getColumn(ColNy)
	cx = contacts.getColumn(ColX); cy = contacts.getColumn(ColY)
	csep = contacts.getColumn(ColSep)
	cfr = contacts.getColumn(ColFriction); cre = contacts.getColumn(ColRestitution)
	pn = contacts.getColumn(ColPn); pt = contacts.getColumn(ColPt)
	cpair = contacts.getColumn(ColPair)
	px = pos.getColumn(0); py = pos.getColumn(1)
	vx = vel.getColumn(0); vy = vel.getColumn(1); w = vel.getColumn(2)
	im = invMass.getColumn(0); ii = invMass.getColumn(2)
	invDt = 1 / dt

	// per point
	r1xs = [0] * n; r1ys = [0] * n; r2xs = [0] * n; r2ys = [0] * n
	rnAs = [0] * n; rnBs = [0] * n; massNs = [0] * n; massTs = [0] * n; biases = [0] * n
	// per two-point manifold, on its first row
	block = [0] * n
	k11s = [0] * n; k12s = [0] * n; k22s = [0] * n
	n11s = [0] * n; n12s = [0] * n; n22s = [0] * n

	// ---- setup ----
	i = 0
	while i < n
		count = 1 + (cpair[i] == 1)
		a = ca[i]; b = cb[i]
		nx = cnx[i]; ny = cny[i]
		ima = im[a]; imb = im[b]; iia = ii[a]; iib = ii[b]
		mSum = ima + imb
		restitution = cre[i]
		for j in range(i, i + count - 1)
			r1x = cx[j] - px[a]; r1y = cy[j] - py[a]
			r2x = cx[j] - px[b]; r2y = cy[j] - py[b]
			r1xs[j] = r1x; r1ys[j] = r1y; r2xs[j] = r2x; r2ys[j] = r2y
			rnA = r1x*ny - r1y*nx; rnB = r2x*ny - r2y*nx    // cross(r, n)
			rnAs[j] = rnA; rnBs[j] = rnB
			massNs[j] = 1 / (mSum + iia*rnA*rnA + iib*rnB*rnB)
			rtA = r1x*nx + r1y*ny; rtB = r2x*nx + r2y*ny    // -cross(r, t)
			massTs[j] = 1 / (mSum + iia*rtA*rtA + iib*rtB*rtB)
			// For a gap (a speculative contact), allow closing it in this step
			// but no more; for overlap beyond the slop, push apart a fraction...
			sep = csep[j]
			if sep > 0 then
				bias = -sep * invDt
			else if sep < -slop then
				bias = -biasFactor * invDt * (sep + slop)
			else
				bias = 0
			end if
			// ...or bounce, if that asks for more.
			if restitution > 0 then
				vn = (vx[b] - w[b]*r2y - vx[a] + w[a]*r1y) * nx +
				     (vy[b] + w[b]*r2x - vy[a] - w[a]*r1x) * ny
				if vn < -restThreshold and -restitution * vn > bias then bias = -restitution * vn
			end if
			biases[j] = bias
		end for
		if count == 2 then
			// K relates the two points' normal impulses to their normal
			// velocities.  Use it only if it is well conditioned (the points
			// aren't nearly on top of each other).
			k11 = mSum + iia*rnAs[i]*rnAs[i] + iib*rnBs[i]*rnBs[i]
			k22 = mSum + iia*rnAs[i+1]*rnAs[i+1] + iib*rnBs[i+1]*rnBs[i+1]
			k12 = mSum + iia*rnAs[i]*rnAs[i+1] + iib*rnBs[i]*rnBs[i+1]
			det = k11*k22 - k12*k12
			if k11*k11 < 1000 * det then
				block[i] = 1
				k11s[i] = k11; k12s[i] = k12; k22s[i] = k22
				n11s[i] = k22 / det; n12s[i] = -k12 / det; n22s[i] = k11 / det
			end if
		end if
		i += count
	end while

	// ---- warm start ----
	for i in range(0, n - 1)
		Pn = pn[i]; Pt = pt[i]
		if Pn == 0 and Pt == 0 then continue
		a = ca[i]; b = cb[i]
		nx = cnx[i]; ny = cny[i]
		Px = Pn*nx + Pt*ny; Py = Pn*ny - Pt*nx
		ima = im[a]; imb = im[b]
		vx[a] -= ima*Px; vy[a] -= ima*Py
		w[a] -= ii[a] * (r1xs[i]*Py - r1ys[i]*Px)
		vx[b] += imb*Px; vy[b] += imb*Py
		w[b] += ii[b] * (r2xs[i]*Py - r2ys[i]*Px)
	end for

	// ---- iterations ----
	for iter in range(1, iterations, 1)
		i = 0
		while i < n
			count = 1 + (cpair[i] == 1)
			a = ca[i]; b = cb[i]
			nx = cnx[i]; ny = cny[i]
			ima = im[a]; imb = im[b]; iia = ii[a]; iib = ii[b]

			// friction, along the tangent (ny, -nx)
			friction = cfr[i]
			for j in range(i, i + count - 1)
				r1x = r1xs[j]; r1y = r1ys[j]; r2x = r2xs[j]; r2y = r2ys[j]
				dvx = vx[b] - w[b]*r2y - vx[a] + w[a]*r1y
				dvy = vy[b] + w[b]*r2x - vy[a] - w[a]*r1x
				dPt = -massTs[j] * (dvx*ny - dvy*nx)
				maxPt = friction * pn[j]
				Pt0 = pt[j]
				Pt = Pt0 + dPt
				if Pt < -maxPt then
					Pt = -maxPt
				else if Pt > maxPt then
					Pt = maxPt
				end if
				pt[j] = Pt
				dPt = Pt - Pt0
				Px = dPt*ny; Py = -dPt*nx
				vx[a] -= ima*Px; vy[a] -= ima*Py
				w[a] -= iia * (r1x*Py - r1y*Px)
				vx[b] += imb*Px; vy[b] += imb*Py
				w[b] += iib * (r2x*Py - r2y*Px)
			end for

			if not block[i] then
				// normal impulses, one point at a time
				for j in range(i, i + count - 1)
					dvx = vx[b] - w[b]*r2ys[j] - vx[a] + w[a]*r1ys[j]
					dvy = vy[b] + w[b]*r2xs[j] - vy[a] - w[a]*r1xs[j]
					dPn = massNs[j] * (biases[j] - dvx*nx - dvy*ny)
					Pn0 = pn[j]
					Pn = Pn0 + dPn
					if Pn < 0 then Pn = 0
					pn[j] = Pn
					dPn = Pn - Pn0
					vx[a] -= ima*dPn*nx; vy[a] -= ima*dPn*ny
					w[a] -= iia * dPn * rnAs[j]
					vx[b] += imb*dPn*nx; vy[b] += imb*dPn*ny
					w[b] += iib * dPn * rnBs[j]
				end for
				i += count
				continue
			end if

			// Both normal impulses at once.  Find new totals x1, x2 >= 0 such
			// that each point either ends up separating at its bias speed, or
			// is separating faster than that with no impulse; there are four
			// cases, depending on which points are pushing.
			j = i + 1
			a1 = pn[i]; a2 = pn[j]
			vn1 = (vx[b] - w[b]*r2ys[i] - vx[a] + w[a]*r1ys[i]) * nx +
			      (vy[b] + w[b]*r2xs[i] - vy[a] - w[a]*r1xs[i]) * ny
			vn2 = (vx[b] - w[b]*r2ys[j] - vx[a] + w[a]*r1ys[j]) * nx +
			      (vy[b] + w[b]*r2xs[j] - vy[a] - w[a]*r1xs[j]) * ny
			k11 = k11s[i]; k12 = k12s[i]; k22 = k22s[i]
			b1 = vn1 - biases[i] - (k11*a1 + k12*a2)
			b2 = vn2 - biases[j] - (k12*a1 + k22*a2)
			solved = true
			// both pushing
			x1 = -(n11s[i]*b1 + n12s[i]*b2)
			x2 = -(n12s[i]*b1 + n22s[i]*b2)
			if x1 < 0 or x2 < 0 then
				// only point 1 pushing
				x1 = -b1 / k11; x2 = 0
				if x1 < 0 or k12*x1 + b2 < 0 then
					// only point 2 pushing
					x1 = 0; x2 = -b2 / k22
					if x2 < 0 or k12*x2 + b1 < 0 then
						// neither pushing
						x1 = 0; x2 = 0
						if b1 < 0 or b2 < 0 then solved = false    // no solution; leave as is
					end if
				end if
			end if
			if solved then
				d1 = x1 - a1; d2 = x2 - a2
				pn[i] = x1; pn[j] = x2
				Px = (d1 + d2) * nx; Py = (d1 + d2) * ny
				vx[a] -= ima*Px; vy[a] -= ima*Py
				w[a] -= iia * (d1*rnAs[i] + d2*rnAs[j])
				vx[b] += imb*Px; vy[b] += imb*Py
				w[b] += iib * (d1*rnBs[i] + d2*rnBs[j])
			end if
			i += 2
		end while
	end for

	vel.setColumn 0, vx; vel.setColumn 1, vy; vel.setColumn 2, w
	contacts.setColumn ColPn, pn; contacts.setColumn ColPt, pt
end function
