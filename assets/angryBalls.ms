// angryBalls.ms
// An "Angry Birds" style demo of physics.ms: pull the ball back in the
// slingshot, and let go to fling it at the structure on the right.  The sound
// effects are synthesized when it starts.
//
// Controls: drag the ball back and release to launch; R reset; ESC exit.

rl = raylib
if not rl.IsWindowReady then rl.InitWindow
import "physics"
P = physics

gravity = 980
ballRadius = 16
maxPull = 110       // how far back the ball can be pulled
launchScale = 12    // launch speed, per pixel of pull
reloadDelay = 1.5   // seconds after a launch until the next ball is ready
creakSpacing = 5    // pixels of stretch per creak of the band

groundTop = rl.GetScreenHeight - 40
sling = [160, groundTop - 140]      // where the ball sits in the slingshot

skyColor = [150, 205, 240, 255]
groundColor = [90, 150, 70, 255]
woodColor = [200, 150, 90, 255]
ballColor = [220, 60, 50, 255]
slingColor = [120, 75, 40, 255]
bandColor = [70, 25, 15, 255]
textColor = [30, 40, 60, 255]

//----------------------------------------------------------------------
// Sounds, made on the fly
//----------------------------------------------------------------------

sampleRate = 22050

// Make a Sound from a list of samples, scaled to fill most of the range
// from -1 to 1.
makeSound = function(samples)
	peak = 0
	for s in samples
		if abs(s) > peak then peak = abs(s)
	end for
	if peak > 0 then
		k = 0.9 / peak
		for i in samples.indexes
			samples[i] *= k
		end for
	end if
	wave = rl.CreateWave(samples.len, sampleRate, 32, 1, samples)
	sound = rl.LoadSoundFromWave(wave)
	rl.UnloadWave wave
	return sound
end function

// knock: a hollow wooden "tok" -- a few tones that die away quickly (the
// higher, the quicker), started off with a click of noise.
makeKnock = function(duration=0.2)
	n = floor(sampleRate * duration)
	samples = [0] * n
	freqs = [190, 470, 830, 1400]
	amps = [1, 0.6, 0.4, 0.25]
	fades = []
	for rate in [30, 55, 80, 120]
		fades.push 1 - rate / sampleRate
	end for
	modes = freqs.indexes
	for i in range(0, n - 1)
		t = i / sampleRate
		s = 0
		for m in modes
			s += amps[m] * sin(2 * pi * freqs[m] * t)
			amps[m] *= fades[m]
		end for
		if i < 100 then s += (rnd * 2 - 1) * (1 - i / 100)
		samples[i] = s
	end for
	return makeSound(samples)
end function

// woosh: rushing air -- noise, smoothed to take the hiss off, that swells up
// quickly and dies away more slowly, brightest when it's loudest.
makeWoosh = function(duration=0.5)
	n = floor(sampleRate * duration)
	samples = [0] * n
	lo1 = 0; lo2 = 0
	for i in range(0, n - 1)
		env = sin(pi * (i / n)^0.6)^2
		k = 0.04 + 0.2 * env    // (less smoothing is brighter)
		lo1 += k * (rnd * 2 - 1 - lo1)
		lo2 += k * (lo1 - lo2)
		samples[i] = lo2 * env
	end for
	return makeSound(samples)
end function

// creak: one grain of rubber creaking -- a tiny squeak, rising as it goes.
// Played in quick succession as the band stretches, higher as it gets
// tighter, the grains make the creak of a stretching rubber band.
makeCreak = function(duration=0.035)
	n = floor(sampleRate * duration)
	samples = [0] * n
	fade = 1 - 110 / sampleRate
	amp = 1
	phase = 0
	for i in range(0, n - 1)
		phase += 2 * pi * (380 + 120 * i / n) / sampleRate
		s = sin(phase) + 0.5 * sin(2 * phase) + 0.3 * sin(3 * phase) + 0.15 * (rnd * 2 - 1)
		attack = i / 22
		if attack > 1 then attack = 1
		samples[i] = s * amp * attack
		amp *= fade
	end for
	return makeSound(samples)
end function

// A pool of aliases of a sound, played round robin, so that several can
// sound at once.
makePool = function(sound, count=8)
	pool = {"sound": sound, "aliases": [], "next": 0}
	for i in range(1, count)
		pool.aliases.push rl.LoadSoundAlias(sound)
	end for
	return pool
end function

playFromPool = function(pool, volume, pitch)
	s = pool.aliases[pool.next]
	pool.next = (pool.next + 1) % pool.aliases.len
	rl.SetSoundVolume s, volume
	rl.SetSoundPitch s, pitch
	rl.PlaySound s
end function

unloadPool = function(pool)
	for s in pool.aliases
		rl.UnloadSoundAlias s
	end for
	rl.UnloadSound pool.sound
end function

woosh = makeWoosh
knocks = makePool(makeKnock)
creaks = makePool(makeCreak)

playKnock = function(volume, pitch)
	playFromPool knocks, volume, pitch
end function

// Play a creak grain, for a band at the given tension (0 to 1).
playCreak = function(tension)
	playFromPool creaks, 0.25 + 0.5 * tension, (0.8 + 0.9 * tension) * (0.97 + 0.06 * rnd)
end function

unloadSounds = function
	unloadPool knocks
	unloadPool creaks
	rl.UnloadSound woosh
end function

//----------------------------------------------------------------------
// Building the world
//----------------------------------------------------------------------

// Add a width x height block whose bottom edge is at y; return its top.
block = function(world, x, y, width, height)
	b = world.addBox(x, y - height / 2, width, height)
	b.shapes[0].friction = 0.6
	b.shapes[0].color = woodColor
	return y - height
end function

// A two-story structure of posts and planks, standing on y.
buildStructure = function(world, x, y)
	for dx in [-100, 0, 100]
		top = block(world, x + dx, y, 20, 80)
	end for
	y = block(world, x, top, 240, 16)
	for dx in [-70, 70]
		top = block(world, x + dx, y, 20, 70)
	end for
	y = block(world, x, top, 180, 16)
	y = block(world, x, y, 40, 40)
	block world, x, y, 30, 30
end function

makeWorld = function
	w = P.World.make(0, gravity)
	sw = rl.GetScreenWidth
	ground = w.addBox(sw / 2, groundTop + 20, sw, 40, P.STATIC)
	ground.shapes[0].friction = 0.8
	ground.shapes[0].color = groundColor
	x = sw * 0.72
	buildStructure w, x, groundTop
	y = groundTop
	for i in range(1, 3)
		y = block(w, x + 190, y, 36, 36)
	end for
	return w
end function

// A new ball, sitting in the slingshot.  It's kinematic (so it stays put)
// until it's launched.
loadBall = function(world)
	b = world.addBody(sling[0], sling[1], P.KINEMATIC)
	s = b.addCircle(ballRadius, 0, 0, 4)
	s.restitution = 0.3
	s.friction = 0.5
	s.color = ballColor
	s.data = "ball"
	b.damping = [0, 1]   // (so it eventually stops rolling)
	return b
end function

//----------------------------------------------------------------------
// Drawing
//----------------------------------------------------------------------

darker = function(c)
	return [floor(c[0] * 0.6), floor(c[1] * 0.6), floor(c[2] * 0.6), 255]
end function

// Point (lx, ly) in the frame of a body at p with angle a.
toWorld = function(p, a, lx, ly)
	return [p[0] + cos(a) * lx - sin(a) * ly, p[1] + sin(a) * lx + cos(a) * ly]
end function

// Draw an angry face on a ball, turning with it.
drawFace = function(p, a)
	for side in [-1, 1]
		eye = toWorld(p, a, 5 * side, -3)
		rl.DrawCircleV eye, 4, rl.WHITE
		rl.DrawCircleV toWorld(p, a, 6 * side, -2), 2, rl.BLACK
		rl.DrawLineEx toWorld(p, a, 11 * side, -11), toWorld(p, a, 2 * side, -7), 3, rl.BLACK
	end for
end function

// Draw the world's shapes, solid.
drawWorld = function(world)
	for s in world.shapes
		c = s.color
		dark = darker(c)
		if s.kind == P.CIRCLE then
			p = s.worldCenter
			rl.DrawCircleV p, s.radius, c
			rl.DrawCircleLinesV p, s.radius, dark
			if s.data == "ball" then drawFace p, s.body.angle
		else
			v = s.worldVertices
			// Our vertices go clockwise on screen; raylib wants them
			// counter-clockwise, so the fan goes backwards.
			pts = [v[0]]
			for i in range(v.len - 1, 1)
				pts.push v[i]
			end for
			rl.DrawTriangleFan pts, c
			for i in v.indexes
				rl.DrawLineV v[i - 1], v[i], dark
			end for
		end if
	end for
end function

tipL = [sling[0] - 18, sling[1]]
tipR = [sling[0] + 18, sling[1]]
fork = [sling[0], sling[1] + 40]

// The parts of the slingshot behind the ball.
drawSlingBack = function(ball)
	rl.DrawLineEx [sling[0], groundTop], fork, 12, slingColor
	rl.DrawLineEx fork, tipR, 9, slingColor
	if ball == null then
		rl.DrawLineEx tipL, tipR, 4, bandColor
	else
		rl.DrawLineEx tipR, ball.position, 4, bandColor
	end if
end function

// The parts of the slingshot in front of the ball.
drawSlingFront = function(ball)
	rl.DrawLineEx fork, tipL, 9, slingColor
	if ball != null then rl.DrawLineEx tipL, ball.position, 4, bandColor
end function

// Dots along the path a ball will take if launched from p at velocity v
// (stepping as the physics does, but ignoring collisions).
drawTrajectory = function(p, v)
	dt = 1/60
	x = p[0]; y = p[1]
	vx = v[0]; vy = v[1]
	for i in range(1, 60)
		vy += gravity * dt
		x += vx * dt; y += vy * dt
		if i % 4 == 0 then rl.DrawCircleV [x, y], 3, [255, 255, 255, 255 - i * 3]
	end for
end function

//----------------------------------------------------------------------
// Hit sounds
//
// Contacts are found every step, and resting ones never go away, so rather
// than sound off for each contact, we look at how much collisions changed
// each body's velocity during the step -- the impulse on it, divided by its
// mass.  Resting and gently sliding bodies change hardly at all; a body that
// hits (or is hit by) something changes a lot, all at once.
//----------------------------------------------------------------------

hitThreshold = 80       // speed change (pixels/s) in one step that counts as a hit
hitFullVolume = 700     // speed change that sounds at full volume
maxHitsPerStep = 3      // only the hardest few hits in a step make a sound
hitCooldown = 7         // steps before the same body can sound again

// The hits in the step just taken, as [strength, body], hardest first.
// vx0 and vy0 are the bodies' velocities before the step (lists by body
// index, as from world.vel.getColumn); dt is the step's length.
findHits = function(world, vx0, vy0, dt)
	vx = world.vel.getColumn(0); vy = world.vel.getColumn(1)
	gx = world.gravity[0] * dt; gy = world.gravity[1] * dt
	hits = []
	for i in world.bodies.indexes
		b = world.bodies[i]
		if b.type != P.DYNAMIC then continue
		dv = sqrt((vx[i] - vx0[i] - gx)^2 + (vy[i] - vy0[i] - gy)^2)
		if dv >= hitThreshold then hits.push [dv, b]
	end for
	hits.sort 0, false
	return hits
end function

// Play a knock for each of the hardest hits: louder for harder hits, and
// lower for bigger bodies.
playHits = function(hits)
	count = 0
	for h in hits
		b = h[1]
		step = b.world.stepCount
		if b.hasIndex("hitStep") and step - b.hitStep < hitCooldown then continue
		b.hitStep = step
		t = (h[0] - hitThreshold) / (hitFullVolume - hitThreshold)
		if t > 1 then t = 1
		pitch = (1600 / b.mass)^0.25 * (0.93 + 0.14 * rnd)
		if b.shapes[0].data == "ball" then pitch *= 0.7    // (more of a thud)
		playKnock 0.15 + 0.85 * t, pitch
		count += 1
		if count >= maxHitsPerStep then break
	end for
end function

//----------------------------------------------------------------------
// Main loop
//----------------------------------------------------------------------

run = function
	rl.SetWindowTitle "Angry Balls"
	rl.SetTargetFPS 60
	world = makeWorld
	ball = loadBall(world)  // the ball in the slingshot, or null
	aiming = false          // true while the ball is being pulled back
	pullDist = 0            // how far back it's pulled
	stretch = 0             // stretch not yet creaked for (see creakSpacing)
	reloadTime = 0
	shots = 0
	print "Angry Balls!  Drag the ball back and let go.  R to reset, ESC to exit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break
		if rl.IsKeyPressed(rl.KEY_R) then
			world = makeWorld
			ball = loadBall(world)
			aiming = false
			shots = 0
		end if
		mx = rl.GetMouseX; my = rl.GetMouseY

		if ball != null then
			// Grab the ball, pull it back (up to maxPull), and let go.
			if rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT) then
				p = ball.position
				aiming = (mx - p[0])^2 + (my - p[1])^2 <= (ballRadius + 12)^2
				pullDist = 0; stretch = 0
			end if
			if aiming then
				dx = mx - sling[0]; dy = my - sling[1]
				d = sqrt(dx * dx + dy * dy)
				if d > maxPull then
					dx *= maxPull / d; dy *= maxPull / d
					d = maxPull
				end if
				ball.position = [sling[0] + dx, sling[1] + dy]
				// Creak as the band stretches (but not as it relaxes).
				if d > pullDist then stretch += d - pullDist
				pullDist = d
				grains = floor(stretch / creakSpacing)
				stretch -= grains * creakSpacing
				if grains > 3 then grains = 3
				for i in range(1, grains, 1)
					playCreak d / maxPull
				end for
				if not rl.IsMouseButtonDown(rl.MOUSE_BUTTON_LEFT) then
					aiming = false
					if d > 10 then
						ball.type = P.DYNAMIC
						ball.velocity = [-dx * launchScale, -dy * launchScale]
						// woosh: faster launches are louder and higher
						rl.SetSoundVolume woosh, 0.2 + 0.8 * d / maxPull
						rl.SetSoundPitch woosh, 0.6 + 0.8 * d / maxPull
						rl.PlaySound woosh
						ball = null
						shots += 1
						reloadTime = time + reloadDelay
					else
						ball.position = [sling[0], sling[1]]
					end if
				end if
			end if
		else if time > reloadTime then
			ball = loadBall(world)
		end if

		// Forget bodies that have fallen off the edge of the world.
		for i in range(world.bodies.len - 1, 0, -1)
			b = world.bodies[i]
			if b.type == P.DYNAMIC and b.centerOfMass[1] > rl.GetScreenHeight + 200 then world.removeBody b
		end for

		// (Note the velocities before the step, to see what it hit hard.)
		vx0 = world.vel.getColumn(0); vy0 = world.vel.getColumn(1)
		world.step 1/60
		playHits findHits(world, vx0, vy0, 1/60)

		rl.BeginDrawing
		rl.ClearBackground skyColor
		drawSlingBack ball
		drawWorld world
		drawSlingFront ball
		if aiming then
			p = ball.position
			drawTrajectory p, [(sling[0] - p[0]) * launchScale, (sling[1] - p[1]) * launchScale]
		end if
		rl.DrawText "ANGRY BALLS", 20, 14, 30, textColor
		rl.DrawText "Shots: " + shots, 20, 50, 20, textColor
		rl.DrawText "drag the ball back and let go   R reset   ESC exit", 20, 76, 16, darker(skyColor)
		rl.EndDrawing
		yield
	end while
	unloadSounds
	print "Angry Balls finished."
end function

if locals == globals then run
