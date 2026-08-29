// fireworks.ms
//
// A fireworks particle system in which every particle attribute lives in a
// Matrix, and the whole physics step -- Newton, gravity, drag, and the alpha
// fade -- is three Matrix calls, whatever the population.  The script touches
// individual particles in exactly one place, the draw loop, because raylib
// wants one call per sprite; retiring the dead ones rides along in that same
// pass.  Drawing costs far more than the physics -- the HUD shows the split.
//
// Three parallel N-row matrices make up the pool:
//
//      pos   N x 2    x, y             (pixels, y down)
//      vel   N x 2    vx, vy           (pixels/second)
//      col   N x 4    r, g, b, a       (0-255; alpha doubles as the life)
//
// A burst appends rows to all three; a particle dies when its alpha reaches
// zero and is removed with removeRowFast (O(1), reorders the pool -- fine,
// since nothing outside holds an index).  Because the pool is grown and
// shrunk in place against reserved capacity, the steady state allocates
// nothing.
//
// Controls:
//      click / SPACE   launch a burst (at the mouse, or somewhere random)
//      A               toggle automatic fireworks
//      S               toggle the stress test: keep adding bursts as long as
//                      the frame still fits in the 60 fps budget
//      T               toggle the trail effect
//      ESC             quit

import "matrixUtil"

rl = raylib
if not rl.IsWindowReady then rl.InitWindow

//----------------------------------------------------------------------
// Tuning
//----------------------------------------------------------------------

gravity = 160        // pixels/second^2, downward
dragPerSecond = 0.4  // fraction of speed remaining after one second
fadeRate = 70        // alpha lost per second
burstSize = 400      // particles per burst
burstSpeed = 260     // pixels/second at the leading edge
glowSize = 16        // particle sprite size, pixels

// Each palette is a core and an edge color; every particle in a burst is a
// random blend of the two.
palettes = [
	[[255, 240, 180], [255,  70,  30]],   // ember
	[[200, 240, 255], [ 40, 110, 255]],   // sapphire
	[[255, 220, 255], [220,  40, 200]],   // orchid
	[[220, 255, 220], [ 40, 220, 110]],   // verdant
	[[255, 255, 255], [255, 190,  40]] ]  // gold

// A uniform deviate in [0,1).  This is `rnd` and nothing more; it exists
// because Matrix's callbacks (`ofSize`'s function form, `apply`, `apply1`)
// cannot yet call an *intrinsic* function reference -- `@rnd` comes back null
// -- so the callback has to be a script function.
// ToDo: fix this in MiniScript2.
uniform = function
	return rnd
end function

//----------------------------------------------------------------------
// The pool
//----------------------------------------------------------------------

Pool = {}
Pool.pos = null
Pool.vel = null
Pool.col = null
Pool.scratch = null   // draw positions, recomputed in place every frame
Pool.gravityRow = null
Pool.fadeRow = null
Pool.offsetRow = null

Pool.make = function(reserveRows = 20000)
	p = new Pool
	p.pos = Matrix.ofSize(0, 2)
	p.vel = Matrix.ofSize(0, 2)
	p.col = Matrix.ofSize(0, 4)
	p.scratch = Matrix.ofSize(0, 2)
	for m in [p.pos, p.vel, p.scratch]; m.reserve reserveRows; end for
	p.col.reserve reserveRows
	// Constant broadcast rows, built once: gemm and friends accept a 1 x n
	// addend and apply it down every row, so these never need to be resized
	// to match the population.
	p.gravityRow = Matrix.fromList([[0, gravity]])
	p.fadeRow = Matrix.fromList([[0, 0, 0, -fadeRate]])
	p.offsetRow = Matrix.fromList([[-glowSize/2, -glowSize/2]])
	return p
end function

Pool.count = function
	return self.pos.rows
end function

// Add `count` particles flying out from (x, y).
Pool.burst = function(x, y, count, palette, speed)
	core = palette[0]
	edge = palette[1]

	// Positions: every new particle starts at the burst point.
	np = Matrix.ofSize(count, 2)
	np.add [[x, y]]

	// Velocities: a standard normal pair per row is a uniformly distributed
	// direction once the row is normalized to unit length.  Scaling by
	// sqrt(rnd) rather than rnd spreads the particles evenly over the disc
	// instead of clumping them at the rim.
	nv = Matrix.ofSize(count, 2)
	nv.randomize 0, 1
	nv.normalizeRows
	sp = Matrix.ofSize(count, 1, @uniform)
	sp.sqrt
	sp.multiplyBy speed
	nv.elemMultiplyBy sp

	// Colors: blend from `core` to `edge` by a random t per particle, and
	// let t take a bite out of the starting alpha too, so the particles do
	// not all wink out on the same frame.  A per-row scalar times a constant
	// row is an outer product -- which is to say, one gemm.
	nc = Matrix.ofSize(count, 4)
	nc.add [[core[0], core[1], core[2], 255]]
	delta = Matrix.fromList([[edge[0] - core[0], edge[1] - core[1],
	   edge[2] - core[2], -110]])
	t = Matrix.ofSize(count, 1, @uniform)
	t.gemm delta, nc, nc

	self.pos.appendRows np
	self.vel.appendRows nv
	self.col.appendRows nc
end function

// Newton, gravity, drag, and the alpha fade -- three calls, whatever N is.
Pool.update = function(dt)
	if self.pos.rows == 0 then return

	// x += v*dt
	self.pos.addScaled self.vel, dt

	// v = v*drag^dt + gravity*dt.  One gemm: alpha scales the receiver,
	// beta scales the broadcast addend.
	self.vel.gemm null, self.gravityRow, self.vel, false, false,
	   dragPerSecond ^ dt, dt

	// alpha -= fadeRate*dt  (the r, g, b entries of fadeRow are zero)
	self.col.addScaled self.fadeRow, dt
end function

// Draw the pool -- and, since this is already a walk over every particle,
// retire the dead ones on the way, which is why the loop runs backwards.
// `removeRowFast` fills the hole at i with the current last row, and every
// index above i has already been drawn, so the flat-list snapshots taken
// before the loop stay valid for everything still ahead of us.  Additive
// blending is commutative, so back-to-front costs nothing.
//
// Drawing is, measured, about ten times the whole physics step: raylib wants
// one call per sprite and that call has to come from script.  The draw
// positions are still computed as a matrix -- the sprite's half-size
// subtracted from every row in one pass -- so the loop itself does nothing but
// unpack numbers, call raylib, and occasionally drop a row.
Pool.renderAndReap = function(glowTex)
	n = self.pos.rows
	if n == 0 then return
	self.pos.gemm null, self.offsetRow, self.scratch
	p = self.scratch.toFlatList
	c = self.col.toFlatList
	i = n - 1
	j = i * 2
	k = i * 4
	while i >= 0
		a = c[k+3]
		if a > 0 then
			rl.DrawTexture glowTex, p[j], p[j+1], [c[k], c[k+1], c[k+2], a]
		else
			// Faded out: never drawn (a negative alpha would wrap around to
			// bright), just removed.
			self.pos.removeRowFast i
			self.vel.removeRowFast i
			self.col.removeRowFast i
		end if
		i -= 1; j -= 2; k -= 4
	end while
end function

//----------------------------------------------------------------------
// Demo
//----------------------------------------------------------------------

// A soft round sprite: white in the middle, fading to fully transparent, so
// that additive blending turns overlapping particles into a glow.
makeGlowTexture = function(size)
	img = rl.GenImageGradientRadial(size, size, 0.1, [255, 255, 255, 255],
	   [255, 255, 255, 0])
	tex = rl.LoadTextureFromImage(img)
	rl.UnloadImage img
	return tex
end function

// stress: start with the stress test running.
// maxSeconds: quit automatically after this long (0 = run until ESC).
run = function(stress = false, maxSeconds = 0)
	rl.SetWindowTitle "Matrix Fireworks"
	rl.SetTargetFPS 60
	screenW = rl.GetScreenWidth
	screenH = rl.GetScreenHeight

	glowTex = makeGlowTexture(glowSize)
	pool = Pool.make

	auto = true
	trails = true
	nextLaunch = 0.3
	simMs = 0           // milliseconds in the physics step, smoothed
	drawMs = 0          // milliseconds in draw-and-reap, smoothed
	workMs = 0          // the two together: the frame's real cost
	peak = 0            // most particles ever carried inside the frame budget
	startTime = rl.GetTime

	print "Matrix fireworks: click to launch, A auto, S stress test, " +
	   "T trails, ESC to quit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break
		if maxSeconds > 0 and rl.GetTime - startTime > maxSeconds then break
		if rl.IsKeyPressed(rl.KEY_A) then auto = not auto
		if rl.IsKeyPressed(rl.KEY_T) then trails = not trails
		if rl.IsKeyPressed(rl.KEY_S) then
			stress = not stress
			if stress then peak = 0
		end if

		dt = rl.GetFrameTime
		if dt > 0.1 then dt = 0.1     // a hitch must not teleport everything

		// Launches.
		if rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT) then
			pool.burst rl.GetMouseX, rl.GetMouseY, burstSize,
			   palettes[floor(rnd * palettes.len)], burstSpeed
		end if
		if rl.IsKeyPressed(rl.KEY_SPACE) then
			auto = true
			nextLaunch = 0
		end if
		if auto then
			nextLaunch -= dt
			if nextLaunch <= 0 then
				nextLaunch = 0.4 + rnd * 0.8
				pool.burst 80 + rnd * (screenW - 160),
				   60 + rnd * (screenH * 0.45), burstSize,
				   palettes[floor(rnd * palettes.len)], burstSpeed
			end if
		end if
		// The stress test adds a burst on any frame with room to spare in the
		// 16.7 ms budget, and remembers the largest population that fit.
		if stress then
			if workMs < 13 then
				pool.burst rnd * screenW, rnd * screenH * 0.7, burstSize,
				   palettes[floor(rnd * palettes.len)], burstSpeed
			end if
			if workMs <= 16.7 and pool.count > peak then peak = pool.count
		end if

		t0 = rl.GetTime
		pool.update dt
		t1 = rl.GetTime

		rl.BeginDrawing
		if trails then
			// Instead of clearing, veil the previous frame: the particles
			// leave streaks, and it costs one rectangle.
			rl.DrawRectangle 0, 0, screenW, screenH, [0, 0, 8, 44]
		else
			rl.ClearBackground [0, 0, 8, 255]
		end if
		rl.BeginBlendMode rl.BLEND_ADDITIVE
		t2 = rl.GetTime
		pool.renderAndReap glowTex
		rl.EndBlendMode
		// Everything the frame is being timed for is now submitted.  The two
		// halves are tracked separately because they scale for different
		// reasons: the simulation is a handful of C++ passes over N, while the
		// draw is N script iterations and N raylib calls.
		simMs = simMs * 0.9 + (t1 - t0) * 100
		drawMs = drawMs * 0.9 + (rl.GetTime - t2) * 100
		workMs = simMs + drawMs

		rl.DrawText "particles: " + pool.count, 10, 10, 20, [200, 220, 255, 255]
		rl.DrawText "sim: " + round(simMs, 2) + " ms   draw: " +
		   round(drawMs, 2) + " ms", 10, 34, 20, [200, 220, 255, 255]
		rl.DrawText "fps: " + rl.GetFPS, 10, 58, 20, [200, 220, 255, 255]
		if stress then
			rl.DrawText "STRESS -- peak within budget: " + peak, 10, 82, 20,
			   [255, 200, 120, 255]
		end if
		rl.EndDrawing
		yield
	end while

	rl.UnloadTexture glowTex
	print "Fireworks finished. " + pool.count + " particles still in flight."
	print "Last frame: sim " + round(simMs, 2) + " ms, draw " +
	   round(drawMs, 2) + " ms."
	if peak > 0 then print "Peak population inside the 60 fps budget: " + peak
end function

if locals == globals then run
