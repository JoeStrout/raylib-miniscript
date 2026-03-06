// shader_smoke.ms
// Runtime smoke test for Shader intrinsics.
//
// Exercises:
// 1) LoadShaderFromMemory / IsShaderValid / UnloadShader
// 2) BeginShaderMode / EndShaderMode
// 3) GetShaderLocation / GetShaderLocationAttrib
// 4) SetShaderValue (float), SetShaderValueV (vec3)
// 5) SetShaderValueMatrix, SetShaderValueTexture

rl = raylib

rgb = function(r=1, g=1, b=1)
	return [r, g, b]
end function

makeMatrix = function(scale=1)
	return {
		"rows": 4,
		"columns": 4,
		"elem": [
			[scale, 0, 0, 0],
			[0, scale, 0, 0],
			[0, 0, scale, 0],
			[0, 0, 0, 1],
		],
	}
end function

run = function
	rl.SetWindowTitle "Shader Smoke"
	rl.SetTargetFPS 60

	tex = rl.LoadTexture("assets/Wumpus.png")
	if tex == null then
		print "LoadTexture failed"
		return
	end if

	fragCode = rl.LoadFileText("assets/shader_smoke.fs")
	if fragCode == "" then
		print "LoadFileText failed for assets/shader_smoke.fs"
		rl.UnloadTexture(tex)
		return
	end if

	shader = rl.LoadShaderFromMemory("", fragCode)
	if shader == null then
		print "LoadShaderFromMemory failed"
		rl.UnloadTexture(tex)
		return
	end if

	if not rl.IsShaderValid(shader) then
		print "IsShaderValid returned false"
		rl.UnloadShader(shader)
		rl.UnloadTexture(tex)
		return
	end if

	locTime = rl.GetShaderLocation(shader, "uTime")
	locRgb = rl.GetShaderLocation(shader, "uRgb")
	locTintMat = rl.GetShaderLocation(shader, "uTintMat")
	locExtraTex = rl.GetShaderLocation(shader, "uExtraTex")
	locAttribUV = rl.GetShaderLocationAttrib(shader, "vertexTexCoord")

	print "[shader] loaded id=" + shader.id
	print "[shader] locTime=" + locTime + " locRgb=" + locRgb + " locTintMat=" + locTintMat
	print "[shader] locExtraTex=" + locExtraTex + " attribUV=" + locAttribUV

	if locExtraTex >= 0 then
		rl.SetShaderValueTexture(shader, locExtraTex, tex)
	end if

	print "Shader smoke started. ESC to exit."
	lastSecond = -1

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break

		t = rl.GetTime
		pulseScale = 0.75 + 0.25 * sin(t * 0.5)
		rgbVal = rgb(0.6 + 0.4 * sin(t), 0.7 + 0.3 * sin(t * 1.31), 0.8 + 0.2 * sin(t * 0.77))
		tintMat = makeMatrix(pulseScale)

		if locTime >= 0 then rl.SetShaderValue(shader, locTime, t, rl.SHADER_UNIFORM_FLOAT)
		if locRgb >= 0 then rl.SetShaderValueV(shader, locRgb, rgbVal, rl.SHADER_UNIFORM_VEC3, 1)
		if locTintMat >= 0 then rl.SetShaderValueMatrix(shader, locTintMat, tintMat)

		secNow = floor(t)
		if secNow != lastSecond then
			lastSecond = secNow
			print "[shader] t=" + t + " scale=" + pulseScale
		end if

		rl.BeginDrawing
		rl.ClearBackground(rl.BLACK)

		rl.BeginShaderMode(shader)
		drawX = floor(rl.GetScreenWidth() * 0.5 - tex.width * 0.5)
		drawY = floor(rl.GetScreenHeight() * 0.5 - tex.height * 0.5)
		rl.DrawTexture(tex, drawX, drawY, rl.WHITE)
		rl.EndShaderMode

		rl.DrawText("Shader smoke test", 20, 20, 24, rl.YELLOW)
		rl.DrawText("ESC: exit", 20, 50, 18, rl.RAYWHITE)
		rl.DrawText("SPACE: rebind uExtraTex sampler", 20, 72, 16, rl.RAYWHITE)
		rl.DrawText("uTime loc: " + locTime + "  uRgb loc: " + locRgb, 20, 96, 16, rl.RAYWHITE)
		rl.DrawText("uTintMat loc: " + locTintMat + "  uExtraTex loc: " + locExtraTex, 20, 118, 16, rl.RAYWHITE)
		rl.DrawFPS

		rl.EndDrawing

		if rl.IsKeyPressed(rl.KEY_SPACE) and locExtraTex >= 0 then
			rl.SetShaderValueTexture(shader, locExtraTex, tex)
		end if

		yield
	end while

	rl.UnloadShader(shader)
	rl.UnloadTexture(tex)
	print "Shader smoke finished"
end function

if locals == globals then run