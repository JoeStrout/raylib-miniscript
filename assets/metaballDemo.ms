// metaballDemo.ms
// Displays animated metaball blobs using a fragment shader.

rl = raylib

run = function
	rl.SetWindowTitle "Metaball Shader Demo"
	rl.SetTargetFPS 60
	screenW = rl.GetScreenWidth
	screenH = rl.GetScreenHeight

	fragCode = rl.LoadFileText("assets/shaders/metaballs.fs")
	if fragCode == "" then
		print "Failed to load assets/shaders/metaballs.fs"
		return
	end if

	shader = rl.LoadShaderFromMemory("", fragCode)
	if shader == null or not rl.IsShaderValid(shader) then
		print "Failed to compile shader"
		return
	end if

	locTime = rl.GetShaderLocation(shader, "uTime")
	locRes = rl.GetShaderLocation(shader, "uResolution")
	locColA = rl.GetShaderLocation(shader, "uColorA")
	locColB = rl.GetShaderLocation(shader, "uColorB")

	// Create a white square texture for the shader quad
	quadSize = screenH - 80
	quadImg = rl.GenImageColor(quadSize, quadSize, rl.WHITE)
	quadTex = rl.LoadTextureFromImage(quadImg)
	rl.UnloadImage quadImg

	// Set resolution once
	if locRes >= 0 then
		rl.SetShaderValueV shader, locRes, [screenW, screenH], rl.SHADER_UNIFORM_VEC2, 1
	end if

	paletteIdx = 0
	palettes = []
	// Each entry: [colorA_r, colorA_g, colorA_b, colorB_r, colorB_g, colorB_b]
	palettes.push [[0.1, 0.6, 1.0], [0.0, 0.2, 0.5]]    // Blue
	palettes.push [[1.0, 0.3, 0.1], [1.0, 0.8, 0.0]]    // Lava
	palettes.push [[0.2, 1.0, 0.3], [0.0, 0.4, 0.1]]    // Slime
	palettes.push [[0.9, 0.2, 0.9], [0.3, 0.0, 0.6]]    // Neon
	palNames = ["Blue", "Lava", "Slime", "Neon"]

	print "Metaball shader demo. SPACE to cycle palettes. ESC to exit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break

		if rl.IsKeyPressed(rl.KEY_SPACE) then
			paletteIdx = (paletteIdx + 1) % palettes.len
		end if

		t = rl.GetTime

		if locTime >= 0 then
			rl.SetShaderValue shader, locTime, t, rl.SHADER_UNIFORM_FLOAT
		end if
		if locColA >= 0 then
			rl.SetShaderValueV shader, locColA, palettes[paletteIdx][0], rl.SHADER_UNIFORM_VEC3, 1
		end if
		if locColB >= 0 then
			rl.SetShaderValueV shader, locColB, palettes[paletteIdx][1], rl.SHADER_UNIFORM_VEC3, 1
		end if

		rl.BeginDrawing
		rl.ClearBackground rl.BLACK

		// Draw a centered square through the shader
		quadX = floor((screenW - quadSize) / 2)
		quadY = floor((screenH - quadSize) / 2)
		rl.BeginShaderMode shader
		rl.DrawTexture quadTex, quadX, quadY, rl.WHITE
		rl.EndShaderMode

		rl.DrawText "Metaball Shader Demo", 20, 16, 24, rl.WHITE
		rl.DrawText "SPACE: cycle palette  |  ESC: exit", 20, 46, 18, rl.RAYWHITE
		rl.DrawText "Palette: " + palNames[paletteIdx], 20, 72, 18, rl.YELLOW
		rl.DrawFPS

		rl.EndDrawing
		yield
	end while

	rl.UnloadTexture quadTex
	rl.UnloadShader shader
	print "Metaball demo finished."
end function

if locals == globals then run
