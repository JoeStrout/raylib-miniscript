// sandDemo.ms
// Falling sand simulation using ping-pong render textures and a fragment shader.
// All state encoded in one texture (alpha: 0=empty, 128=obstacle, 255=sand).
// Controls: SPACE to cycle obstacles, R to reset, ESC to exit.

rl = raylib
if not rl.IsWindowReady then rl.InitWindow 

// Draw obstacle layout into both sim RTs.
// Uses alpha=128 to mark obstacle pixels.
drawObstacles = function(rtA, rtB, simW, simH, layoutName)
	wallColor = {"r": 140, "g": 120, "b": 100, "a": 128}

	for rt in [rtA, rtB]
		rl.BeginTextureMode rt
		rl.rlSetBlendFactors 1, 0, 32774  // GL_ONE, GL_ZERO, GL_FUNC_ADD
		rl.BeginBlendMode 6               // BLEND_CUSTOM
		rl.ClearBackground rl.BLANK

		if layoutName == "Hourglass" then
			rl.DrawRectangle 0, 0, 4, simH, wallColor
			rl.DrawRectangle simW - 4, 0, 4, simH, wallColor
			rl.DrawRectangle 0, simH - 4, simW, 4, wallColor
			for i in range(0, 80)
				x1 = 4 + i
				x2 = simW - 5 - i
				y = 80 + i
				rl.DrawRectangle x1, y, 3, 1, wallColor
				rl.DrawRectangle x2, y, 3, 1, wallColor
			end for
			for i in range(0, 60)
				x1 = 84 - i
				x2 = simW - 85 + i
				y = 170 + i
				if x1 >= 4 then rl.DrawRectangle x1, y, 3, 1, wallColor
				if x2 <= simW - 5 then rl.DrawRectangle x2, y, 3, 1, wallColor
			end for
		else if layoutName == "Pegs" then
			rl.DrawRectangle 0, 0, 4, simH, wallColor
			rl.DrawRectangle simW - 4, 0, 4, simH, wallColor
			rl.DrawRectangle 0, simH - 4, simW, 4, wallColor
			for row in range(0, 5)
				y = 50 + row * 38
				offset = 0
				if row % 2 == 1 then offset = 20
				for col in range(0, 6)
					cx = 24 + offset + col * 38
					if cx < simW - 10 then
						rl.DrawCircle cx, y, 8, wallColor
					end if
				end for
			end for
		else if layoutName == "Shelves" then
			rl.DrawRectangle 0, 0, 4, simH, wallColor
			rl.DrawRectangle simW - 4, 0, 4, simH, wallColor
			rl.DrawRectangle 0, simH - 4, simW, 4, wallColor
			rl.DrawRectangle 4, 60, 180, 4, wallColor
			rl.DrawRectangle 72, 110, 180, 4, wallColor
			rl.DrawRectangle 4, 160, 180, 4, wallColor
			rl.DrawRectangle 72, 210, 180, 4, wallColor
		end if

		rl.EndBlendMode
		rl.EndTextureMode
	end for
end function

run = function
	rl.SetWindowTitle "Sand Simulation"
	rl.SetTargetFPS 60
	screenW = rl.GetScreenWidth
	screenH = rl.GetScreenHeight

	simW = 256
	simH = 256

	// Load shaders
	simCode = rl.LoadFileText("assets/shaders/sandSim.fs")
	if simCode == "" then
		print "Failed to load sandSim.fs"
		return
	end if
	dispCode = rl.LoadFileText("assets/shaders/sandDisplay.fs")
	if dispCode == "" then
		print "Failed to load sandDisplay.fs"
		return
	end if

	simShader = rl.LoadShaderFromMemory("", simCode)
	if not rl.IsShaderValid(simShader) then
		print "Failed to compile sim shader"
		return
	end if
	dispShader = rl.LoadShaderFromMemory("", dispCode)
	if not rl.IsShaderValid(dispShader) then
		print "Failed to compile display shader"
		return
	end if

	simLocTexelSize = rl.GetShaderLocation(simShader, "uTexelSize")
	simLocTime = rl.GetShaderLocation(simShader, "uTime")
	simLocPass = rl.GetShaderLocation(simShader, "uPass")
	dispLocBgColor = rl.GetShaderLocation(dispShader, "uBgColor")

	print "Sim locs: texel=" + simLocTexelSize + " time=" + simLocTime + " pass=" + simLocPass
	print "Disp locs: bg=" + dispLocBgColor

	// Ping-pong render textures
	rtA = rl.LoadRenderTexture(simW, simH)
	rtB = rl.LoadRenderTexture(simW, simH)
	rl.SetTextureFilter rtA.texture, rl.TEXTURE_FILTER_POINT
	rl.SetTextureFilter rtB.texture, rl.TEXTURE_FILTER_POINT

	obsNames = ["Hourglass", "Pegs", "Shelves"]
	obstacleIdx = 0
	drawObstacles rtA, rtB, simW, simH, obsNames[obstacleIdx]

	if simLocTexelSize >= 0 then
		rl.SetShaderValueV simShader, simLocTexelSize, [1/simW, 1/simH], rl.SHADER_UNIFORM_VEC2, 1
	end if

	spawnWidth = 160
	spawnX = floor((simW - spawnWidth) / 2)
	spawnY = 10

	current = 0

	resetSand = function
		drawObstacles rtA, rtB, simW, simH, obsNames[obstacleIdx]
	end function

	print "Sand simulation. SPACE=cycle obstacles, R=reset, ESC=exit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break

		if rl.IsKeyPressed(rl.KEY_SPACE) then
			obstacleIdx = (obstacleIdx + 1) % obsNames.len
			resetSand
		end if
		if rl.IsKeyPressed(rl.KEY_R) then resetSand

		t = rl.GetTime

		if current == 0 then
			srcRT = rtA
			dstRT = rtB
		else
			srcRT = rtB
			dstRT = rtA
		end if

		// Spawn sand (alpha=255)
		rl.BeginTextureMode srcRT
		rl.rlSetBlendFactors 1, 0, 32774  // GL_ONE, GL_ZERO, GL_FUNC_ADD
		rl.BeginBlendMode 6               // BLEND_CUSTOM
		for i in range(0, spawnWidth - 1)
			if rnd < 0.3 then
				px = spawnX + i
				rv = floor((0.85 + rnd * 0.15) * 255)
				gv = floor((0.65 + rnd * 0.2) * 255)
				bv = floor((0.25 + rnd * 0.2) * 255)
				c = {"r": rv, "g": gv, "b": bv, "a": 255}
				rl.DrawPixel px, spawnY, c
			end if
		end for
		rl.EndBlendMode
		rl.EndTextureMode

		// Sim passes
		for pass in range(0, 1)
			if simLocTime >= 0 then
				rl.SetShaderValue simShader, simLocTime, t + pass * 0.001, rl.SHADER_UNIFORM_FLOAT
			end if
			if simLocPass >= 0 then
				rl.SetShaderValue simShader, simLocPass, pass, rl.SHADER_UNIFORM_INT
			end if

			rl.BeginTextureMode dstRT
			rl.rlSetBlendFactors 1, 0, 32774  // GL_ONE, GL_ZERO, GL_FUNC_ADD
			rl.BeginBlendMode 6               // BLEND_CUSTOM
			rl.BeginShaderMode simShader
			rl.DrawTexturePro srcRT.texture, [0, 0, simW, -simH], [0, 0, simW, simH], [0,0], 0, rl.WHITE
			rl.EndShaderMode
			rl.EndBlendMode
			rl.EndTextureMode

			temp = srcRT
			srcRT = dstRT
			dstRT = temp
			current = 1 - current
		end for

		// Display
		rl.BeginDrawing
		rl.ClearBackground rl.BLACK

		quadSize = screenH - 100
		quadX = floor((screenW - quadSize) / 2)
		quadY = 80

		if dispLocBgColor >= 0 then
			rl.SetShaderValueV dispShader, dispLocBgColor, [0.05, 0.05, 0.1], rl.SHADER_UNIFORM_VEC3, 1
		end if

		rl.BeginShaderMode dispShader
		rl.DrawTexturePro srcRT.texture, [0, 0, simW, -simH], [quadX, quadY, quadSize, quadSize], [0,0], 0, rl.WHITE
		rl.EndShaderMode

		rl.DrawText "Sand Simulation", 20, 16, 24, rl.WHITE
		rl.DrawText "SPACE: obstacles  R: reset  ESC: exit", 20, 46, 18, rl.RAYWHITE
		rl.DrawText "Layout: " + obsNames[obstacleIdx], 20, screenH - 30, 18, rl.YELLOW
		rl.DrawFPS

		rl.EndDrawing
		yield
	end while

	rl.UnloadRenderTexture rtA
	rl.UnloadRenderTexture rtB
	rl.UnloadShader simShader
	rl.UnloadShader dispShader
	print "Sand demo finished."
end function

if locals == globals then run
