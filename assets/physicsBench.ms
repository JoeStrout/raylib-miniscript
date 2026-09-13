// physicsBench.ms
// Benchmark for physics.ms: runs a few fixed scenes and reports the average
// time per step in each phase of World.step, so that versions of the engine
// (e.g. pure MiniScript vs. with intrinsics) can be compared.
//
// Every scene is deterministic (a fixed random seed), and ends by printing a
// checksum of all body positions.  Each scene is run twice if the host has
// the physicsCore intrinsics: once in pure MiniScript, and once using them.
// The native code does the same arithmetic in the same order, so the two
// checksums should match -- exactly on x86, or very nearly where the compiler
// fuses multiply-adds (ARM).  A large difference means a bug.
//
// Needs no window.  Run: raylib-miniscript assets/physicsBench.ms

import "physics"
P = physics

// The two implementations of physicsCore to compare: the native module, if
// this host has one, and the MiniScript one.  runScene picks one by setting
// physics.physicsCore, which physics.ms finds before any global.
import "physicsFallback"
nativeCore = null
if intrinsics.hasIndex("physicsCore") then nativeCore = intrinsics.physicsCore

// Each scene: build it, run warmup steps (not timed), then timed steps.
dt = 1/60

// Scene 1: a 10-row pyramid of 55 boxes, settled.  Mostly solver: many
// persistent two-point contacts.
buildPyramid = function
	w = P.World.make(0, 980)
	w.addBox 400, 600, 800, 40, P.STATIC
	rows = 10
	for row in range(0, rows - 1)
		for i in range(0, rows - 1 - row)
			w.addBox 400 + (i - (rows - 1 - row) / 2) * 42, 559 - row * 40, 40, 40
		end for
	end for
	return w
end function

// Scene 2: 120 mixed shapes (circles, boxes, polygons, compounds) dropped
// into a bin, piling up.  Exercises every narrowphase path.
buildPile = function
	w = P.World.make(0, 980)
	w.addBox 400, 600, 800, 40, P.STATIC
	w.addBox 10, 300, 20, 600, P.STATIC
	w.addBox 790, 300, 20, 600, P.STATIC
	for i in range(0, 119)
		x = 60 + (i % 12) * 57 + rnd * 10
		y = 520 - floor(i / 12) * 60
		b = w.addBody(x, y, P.DYNAMIC, rnd * 2 * pi)
		kind = i % 5
		if kind == 0 then
			b.addCircle 10 + rnd * 12
		else if kind == 1 then
			b.addBox 20 + rnd * 20, 20 + rnd * 20
		else if kind == 2 then
			pts = []
			n = 3 + floor(rnd * 5)
			for k in range(0, n - 1)
				a = 2 * pi * (k + rnd * 0.6) / n
				r = 14 + rnd * 8
				pts.push [cos(a) * r, sin(a) * r]
			end for
			b.addPolygon pts
		else if kind == 3 then
			b.addBox 36, 10
			b.addBox 10, 24, -13, 12
		else
			b.addBox 26, 14
			b.addCircle 7, -13, 0
			b.addCircle 7, 13, 0
		end if
	end for
	return w
end function

// Scene 3: 400 small bodies drifting in zero gravity inside a walled box,
// with only occasional collisions.  Mostly geometry and broadphase.
buildSparse = function
	w = P.World.make(0, 0)
	w.addBox 400, 610, 820, 20, P.STATIC
	w.addBox 400, -10, 820, 20, P.STATIC
	w.addBox -10, 300, 20, 620, P.STATIC
	w.addBox 810, 300, 20, 620, P.STATIC
	for i in range(0, 399)
		x = 20 + (i % 25) * 31
		y = 20 + floor(i / 25) * 36
		if i % 2 then
			b = w.addCircle(x, y, 6)
		else
			b = w.addBox(x, y, 12, 12, P.DYNAMIC, rnd * pi)
		end if
		b.shapes[0].setRestitution 1
		b.setVelocity 120 * (rnd - 0.5), 120 * (rnd - 0.5)
		b.setAngularVelocity 4 * (rnd - 0.5)
	end for
	return w
end function

// Sum of all body positions and angles, as a quick fingerprint of the result.
checksum = function(w)
	sum = 0
	for b in w.bodies
		p = b.position
		sum += p[0] + 2 * p[1] + 100 * b.angle
	end for
	return round(sum, 3)
end function

runScene = function(name, builder, warmup, steps, native)
	if native then P.physicsCore = nativeCore else P.physicsCore = physicsFallback
	rnd -12345
	w = builder
	for i in range(1, warmup, 1)
		w.step dt
	end for
	w.resetProfile
	t0 = time
	for i in range(1, steps)
		w.step dt
	end for
	elapsed = time - t0
	label = " (MiniScript)"
	if native then label = " (physicsCore)"
	print name + label + ": " + w.bodies.len + " bodies, " + w.shapes.len + " shapes, " +
	  w.pairCount + " pairs, " + w.manifoldCount + " manifolds, " +
	  w.contactPointCount + " points (at end)"
	w.printProfile
	print "  wall clock    " + round(elapsed * 1000 / steps, 3) + " ms/step"
	sum = checksum(w)
	print "  checksum      " + sum
	print
	return {"ms": elapsed * 1000 / steps, "checksum": sum, "solver": w.profile.solver * 1000 / steps}
end function

run = function
	print "physics.ms benchmark"
	print
	modes = [false]
	if nativeCore != null then modes.push true
	scenes = [["pyramid", @buildPyramid, 60], ["pile", @buildPile, 120], ["sparse", @buildSparse, 30]]
	results = {}
	for sc in scenes
		for native in modes
			results[sc[0] + native] = runScene(sc[0], @sc[1], sc[2], 240, native)
		end for
	end for

	print "Summary (ms/step)"
	for sc in scenes
		r0 = results[sc[0] + false]
		line = ("  " + sc[0] + "          ")[:11] + "MiniScript " + round(r0.ms, 2)
		if nativeCore != null then
			r1 = results[sc[0] + true]
			line += "   physicsCore " + round(r1.ms, 2) + "   speedup " + round(r0.ms / r1.ms, 2) + "x" +
			  "   (solver " + round(r0.solver, 2) + " -> " + round(r1.solver, 2) + ")"
			if r1.checksum == r0.checksum then
				line += "   checksums match"
			else
				line += "   checksums differ by " + round(abs(r1.checksum - r0.checksum), 3)
			end if
		end if
		print line
	end for
end function

if locals == globals then run
