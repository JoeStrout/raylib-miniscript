// physicsDemo.ms
// Interactive demo of physics.ms.
//
// Controls: drag a body to move or throw it; click empty space to drop a
// random shape at the mouse; 1-5 to drop a specific kind (1 circle, 2 box,
// 3 polygon, 4 compound, 5 capsule); T build a tower of boxes at the mouse;
// G toggle gravity; C toggle contact display; SPACE pause, N step (while
// paused), R reset, ESC exit.

rl = raylib
if not rl.IsWindowReady then rl.InitWindow
import "physics"
P = physics

randColor = function
	return [100 + floor(rnd * 155), 100 + floor(rnd * 155), 100 + floor(rnd * 155), 255]
end function

// A random convex polygon of 3 to 7 points, about r in radius.
randPoints = function(r)
	n = 3 + floor(rnd * 5)
	pts = []
	for i in range(0, n - 1)
		a = 2 * pi * (i + rnd * 0.6) / n
		pts.push [cos(a) * r * (0.7 + rnd * 0.3), sin(a) * r * (0.7 + rnd * 0.3)]
	end for
	return pts
end function

dropShape = function(world, kind, x, y)
	b = world.addBody(x, y, P.DYNAMIC, rnd * 2 * pi)
	if kind == 1 then
		b.addCircle 12 + rnd * 20
		b.shapes[0].restitution = 0.5
	else if kind == 2 then
		b.addBox 20 + rnd * 40, 20 + rnd * 40
	else if kind == 3 then
		b.addPolygon randPoints(18 + rnd * 20)
	else if kind == 4 then
		// an L of two boxes, with a ball on the end
		b.addBox 60, 16, 0, 0
		b.addBox 16, 40, -22, 20
		b.addCircle 12, 34, 0
	else
		// a capsule: a box with a circle on each end
		b.addBox 50, 24
		b.addCircle 12, -25, 0
		b.addCircle 12, 25, 0
	end if
	c = randColor
	for s in b.shapes
		s.color = c
	end for
	return b
end function

// A tower of n boxes, standing on whatever is below (x, y).
buildTower = function(world, x, y, n=8)
	c = randColor
	for i in range(0, n - 1)
		b = world.addBox(x, y - 20 - i * 40, 40, 40)
		b.shapes[0].color = c
	end for
end function

makeWorld = function
	w = P.World.make(0, 980)
	sw = rl.GetScreenWidth; sh = rl.GetScreenHeight
	w.addBox sw / 2, sh - 20, sw - 40, 30, P.STATIC
	w.addBox 25, sh / 2, 30, sh - 100, P.STATIC
	w.addBox sw - 25, sh / 2, 30, sh - 100, P.STATIC
	// a tilted ramp and a static compound "peg"
	w.addBox sw * 0.3, sh * 0.55, 300, 20, P.STATIC, 0.3
	peg = w.addBody(sw * 0.7, sh * 0.6, P.STATIC)
	peg.addCircle 30
	peg.addBox 120, 12, 0, 0, 0.8
	// a spinning kinematic paddle
	paddle = w.addBox(sw * 0.55, sh * 0.35, 160, 14, P.KINEMATIC)
	paddle.angularVelocity = 1.5
	for kind in range(1, 5)
		dropShape w, kind, 150 + kind * 120, 80
	end for
	return w
end function

run = function
	rl.SetWindowTitle "Physics"
	rl.SetTargetFPS 60
	world = makeWorld
	paused = false
	gravityOn = true
	showContacts = true
	stepMs = 0
	grabbed = null      // body being dragged
	grabLocal = null    // grabbed point, in the body's frame relative to its origin
	print "Physics demo.  Drag bodies, click or 1-5 to drop shapes, T tower, G gravity, C contacts,"
	print "SPACE pause, N step, R reset, ESC exit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break
		if rl.IsKeyPressed(rl.KEY_SPACE) then paused = not paused
		if rl.IsKeyPressed(rl.KEY_R) then
			world = makeWorld
			grabbed = null
		end if
		if rl.IsKeyPressed(rl.KEY_G) then gravityOn = not gravityOn
		if rl.IsKeyPressed(rl.KEY_C) then showContacts = not showContacts
		world.gravity = [0, 980 * gravityOn]
		mx = rl.GetMouseX; my = rl.GetMouseY

		// Drag: grab a non-static body, or drop a shape if there's none here.
		if rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT) then
			b = world.bodyAt(mx, my)
			if b != null and b.type != P.STATIC then
				grabbed = b
				o = b.position; a = b.angle
				dx = mx - o[0]; dy = my - o[1]
				grabLocal = [cos(a) * dx + sin(a) * dy, -sin(a) * dx + cos(a) * dy]
			else
				dropShape world, 1 + floor(rnd * 5), mx, my
			end if
		end if
		if not rl.IsMouseButtonDown(rl.MOUSE_BUTTON_LEFT) or (grabbed != null and grabbed.world == null) then grabbed = null
		if grabbed != null then
			// Steer the grabbed point toward the mouse, by setting velocity.
			o = grabbed.position; a = grabbed.angle
			gx = o[0] + cos(a) * grabLocal[0] - sin(a) * grabLocal[1]
			gy = o[1] + sin(a) * grabLocal[0] + cos(a) * grabLocal[1]
			grabbed.velocity = [15 * (mx - gx), 15 * (my - gy)]
			grabbed.angularVelocity = grabbed.angularVelocity * 0.9
		end if
		for kind in range(1, 5)
			if rl.IsKeyPressed(rl.KEY_ZERO + kind) then dropShape world, kind, mx, my
		end for
		if rl.IsKeyPressed(rl.KEY_T) then buildTower world, mx, my

		// Forget bodies that have fallen far off screen.
		for i in range(world.bodies.len - 1, 0, -1)
			b = world.bodies[i]
			if b.type == P.DYNAMIC and b.centerOfMass[1] > 2000 then world.removeBody b
		end for

		if not paused or rl.IsKeyPressed(rl.KEY_N) then
			t0 = time
			world.step 1/60
			stepMs = (time - t0) * 1000
		end if

		rl.BeginDrawing
		rl.ClearBackground [20, 24, 30, 255]
		world.draw
		if showContacts then world.drawContacts
		rl.DrawText world.bodies.len + " bodies   " + world.contactPointCount + " contacts   " +
		  round(stepMs, 2) + " ms/step" + "   (no gravity)" * (not gravityOn) +
		  "   (paused)" * paused, 50, 10, 20, rl.RAYWHITE
		rl.DrawText "drag: move/throw   click or 1-5: drop shape   T tower   G gravity   C contacts   SPACE pause   N step   R reset",
		  50, 34, 16, rl.GRAY
		rl.DrawFPS rl.GetScreenWidth - 100, 10
		rl.EndDrawing
		yield
	end while
	print "Physics demo finished."
end function

if locals == globals then run
