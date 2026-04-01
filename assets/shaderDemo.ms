// shaderDemo.ms
// Displays an animated shader effect on a big square.

rl = raylib
if not rl.IsWindowReady then rl.InitWindow 

run = function
	rl.SetWindowTitle "Plasma Shader Demo"
	rl.SetTargetFPS 60
	screenW = rl.GetScreenWidth
	screenH = rl.GetScreenHeight

	fragCode = rl.LoadFileText("assets/shaders/plasmaShader.fs")
	if fragCode == "" then
		print "Failed to load assets/shaders/plasmaShader.fs"
		return
	end if

	shader = rl.LoadShaderFromMemory("", fragCode)
	if shader == null or not rl.IsShaderValid(shader) then
		print "Failed to compile shader"
		return
	end if

	locTime = rl.GetShaderLocation(shader, "uTime")
	locRes = rl.GetShaderLocation(shader, "uResolution")
	locPalette = rl.GetShaderLocation(shader, "uPalette")

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
	palettes.push [0.0, 0.0, 0.0]      // classic rainbow
	palettes.push [1.0, 2.0, 3.0]      // ocean
	palettes.push [0.5, 1.5, 4.0]      // fire
	palettes.push [2.0, 0.5, 3.5]      // neon
	palNames = ["Rainbow", "Ocean", "Fire", "Neon"]

	print "locTime=" + locTime + " locRes=" + locRes + " locPalette=" + locPalette
	print "quadTex id=" + quadTex.id + " size=" + quadTex.width + "x" + quadTex.height
	print "Plasma shader demo. SPACE to cycle palettes. ESC to exit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break

		if rl.IsKeyPressed(rl.KEY_SPACE) then
			paletteIdx = (paletteIdx + 1) % palettes.len
		end if

		t = rl.GetTime

		if locTime >= 0 then
			rl.SetShaderValue shader, locTime, t, rl.SHADER_UNIFORM_FLOAT
		end if
		if locPalette >= 0 then
			rl.SetShaderValueV shader, locPalette, palettes[paletteIdx], rl.SHADER_UNIFORM_VEC3, 1
		end if

		rl.BeginDrawing
		rl.ClearBackground rl.DARKBLUE

		// Draw a centered square through the shader
		quadX = floor((screenW - quadSize) / 2)
		quadY = floor((screenH - quadSize) / 2)
		rl.BeginShaderMode shader
		rl.DrawTexture quadTex, quadX, quadY, rl.WHITE
		rl.EndShaderMode

		rl.DrawText "Plasma Shader Demo", 20, 16, 24, rl.WHITE
		rl.DrawText "SPACE: cycle palette  |  ESC: exit", 20, 46, 18, rl.RAYWHITE
		rl.DrawText "Palette: " + palNames[paletteIdx], 20, 72, 18, rl.YELLOW
		rl.DrawFPS

		rl.EndDrawing
		yield
	end while

	rl.UnloadTexture quadTex
	rl.UnloadShader shader
	print "Plasma demo finished."
end function

if locals == globals then run
