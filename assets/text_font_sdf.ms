// text_font_sdf.ms
//
// raylib [text] example - font SDF
// Translated from raylib/examples/text/text_font_sdf.c
//
// Example originally created with raylib 1.3, last time updated with raylib 4.0
//
// Copyright (c) 2015-2025 Ramon Santamaria (@raysan5)

rl = raylib
rl.SetConfigFlags rl.FLAG_WINDOW_HIGHDPI
rl.InitWindow

FONT_DEFAULT = 0
FONT_SDF = 2

main = function
	// Initialization
	//--------------------------------------------------------------------------------------
	screenWidth = 800
	screenHeight = 450

	rl.SetWindowSize screenWidth, screenHeight
	rl.SetWindowTitle "raylib [text] example - font SDF"

	// NOTE: Textures/Fonts MUST be loaded after Window initialization (OpenGL context is required)

	msg = "Signed Distance Fields"

	// Loading file to memory
	fileData = rl.LoadFileData("assets/Merkin.ttf")

	// Default font generation from TTF font
	fontDefault = {}
	fontDefault.baseSize = 16
	fontDefault.glyphCount = 95

	// Loading font data from memory data
	// Parameters > font size: 16, no glyphs array provided (null), glyphs count: 95 (autogenerate chars array)
	fontDefault.glyphs = rl.LoadFontData(fileData, 16, null, 95, FONT_DEFAULT)
	// Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 4 px, pack method: 0 (default)
	fontDefault.recs = []
	for i in range(0, fontDefault.glyphs.len - 1)
		fontDefault.recs.push {"x": 0, "y": 0, "width": 0, "height": 0}
	end for
	atlas = rl.GenImageFontAtlas(fontDefault.glyphs, fontDefault.recs, 16, 4, 0)
	fontDefault.texture = rl.LoadTextureFromImage(atlas)
	rl.UnloadImage atlas
	fontDefault = rl.MakeFont(fontDefault.baseSize, fontDefault.glyphs, fontDefault.recs, fontDefault.texture, 4)

	// SDF font generation from TTF font
	fontSDF = {}
	fontSDF.baseSize = 16
	fontSDF.glyphCount = 95
	// Parameters > font size: 16, no glyphs array provided (null), glyphs count: 0 (defaults to 95)
	fontSDF.glyphs = rl.LoadFontData(fileData, 16, null, 0, FONT_SDF)
	// Parameters > glyphs count: 95, font size: 16, glyphs padding in image: 0 px, pack method: 1 (Skyline algorythm)
	fontSDF.recs = []
	for i in range(0, fontSDF.glyphs.len - 1)
		fontSDF.recs.push {"x": 0, "y": 0, "width": 0, "height": 0}
	end for
	atlas = rl.GenImageFontAtlas(fontSDF.glyphs, fontSDF.recs, 16, 0, 1)
	fontSDF.texture = rl.LoadTextureFromImage(atlas)
	rl.UnloadImage atlas
	fontSDF = rl.MakeFont(fontSDF.baseSize, fontSDF.glyphs, fontSDF.recs, fontSDF.texture)

	rl.UnloadFileData fileData	// Free memory from loaded file

	// Load SDF required shader (we use default vertex shader)
	fragCode = rl.LoadFileText("assets/shaders/sdf.fs")
	shader = rl.LoadShaderFromMemory("", fragCode)
	rl.SetTextureFilter fontSDF.texture, rl.TEXTURE_FILTER_BILINEAR	// Required for SDF font

	fontPosition = [40, screenHeight/2 - 50]
	textSize = [0, 0]
	fontSize = 16.0
	currentFont = 0		// 0 - fontDefault, 1 - fontSDF

	rl.SetTargetFPS 60		// Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

	// Main game loop
	while not rl.WindowShouldClose		// Detect window close button or ESC key
		// Update
		//----------------------------------------------------------------------------------
		fontSize = fontSize + sign(rl.GetMouseWheelMove) * 8

		if fontSize < 8 then fontSize = 8

		if rl.IsKeyDown(rl.KEY_SPACE) then
			currentFont = 1
		else
			currentFont = 0
		end if

		if currentFont == 0 then
			textSize = rl.MeasureTextEx(fontDefault, msg, fontSize, 0)
		else
			textSize = rl.MeasureTextEx(fontSDF, msg, fontSize, 0)
		end if

		fontPosition = [rl.GetScreenWidth/2 - textSize.x/2, rl.GetScreenHeight/2 - textSize.y/2 + 80]
		//----------------------------------------------------------------------------------

		// Draw
		//----------------------------------------------------------------------------------
		rl.BeginDrawing

		rl.ClearBackground rl.RAYWHITE

		if currentFont == 1 then
			// NOTE: SDF fonts require a custom SDF shader to compute fragment color
			rl.BeginShaderMode shader		// Activate SDF font shader
			rl.DrawTextEx fontSDF, msg, fontPosition, fontSize, 0, rl.BLACK
			rl.EndShaderMode				// Activate our default shader for next drawings
	
			rl.DrawTexture fontSDF.texture, 10, 10, rl.BLACK
		else
			rl.DrawTextEx fontDefault, msg, fontPosition, fontSize, 0, rl.BLACK
			rl.DrawTexture fontDefault.texture, 10, 10, rl.BLACK
		end if

		if currentFont == 1 then
			rl.DrawText "SDF!", 320, 20, 80, rl.RED
		else
			rl.DrawText "default font", 315, 40, 30, rl.GRAY
		end if

		rl.DrawText "FONT SIZE: 16.0", rl.GetScreenWidth - 240, 20, 20, rl.DARKGRAY
		rl.DrawText "RENDER SIZE: " + round(fontSize, 2), rl.GetScreenWidth - 240, 50, 20, rl.DARKGRAY
		rl.DrawText "Use MOUSE WHEEL to SCALE TEXT!", rl.GetScreenWidth - 240, 90, 10, rl.MAROON

		rl.DrawText "HOLD SPACE to USE SDF FONT VERSION!", 340, rl.GetScreenHeight - 30, 20, rl.MAROON

		rl.EndDrawing
		//----------------------------------------------------------------------------------
		yield
	end while

	// De-Initialization
	//--------------------------------------------------------------------------------------
	rl.UnloadFont fontDefault		// Default font unloading
	rl.UnloadFont fontSDF			// SDF font unloading

	rl.UnloadShader shader			// Unload SDF shader

	rl.CloseWindow					// Close window and OpenGL context
	//--------------------------------------------------------------------------------------
end function

if locals == globals then main
