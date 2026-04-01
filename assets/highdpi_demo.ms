// Direct translation of core_highdpi_demo.c (in the Raylib examples)

rl = raylib

DrawTextCenter = function(text, x, y, fontSize, color)
    size = rl.MeasureTextEx(rl.GetFontDefault, text, fontSize, 3)
    pos = [x - size.x/2, y - size.y/2]
    rl.DrawTextEx rl.GetFontDefault, text, pos, fontSize, 3, color
end function

TextFormat = function(s, d1, d2)
	s = s.replace("%f", "%d")
	s = s.replace("%d", str(d1), 1)
	if d2 != null then s = s.replace("%d", str(d2), 1)
	return s
end function

main = function
    // Initialization
    //--------------------------------------------------------------------------------------
    screenWidth = 800
    screenHeight = 450

	print "setup 1"
	rl.SetConfigFlags rl.FLAG_WINDOW_HIGHDPI + rl.FLAG_WINDOW_RESIZABLE
    rl.InitWindow screenWidth, screenHeight, "raylib [core] example - highdpi demo"
  	rl.SetWindowMinSize 450, 450
  	print "setup 2"

    logicalGridDescY = 120
    logicalGridLabelY = logicalGridDescY + 30
    logicalGridTop = logicalGridLabelY + 30
    logicalGridBottom = logicalGridTop + 80
    pixelGridTop = logicalGridBottom - 20
    pixelGridBottom = pixelGridTop + 80
    pixelGridLabelY = pixelGridBottom + 30
    pixelGridDescY = pixelGridLabelY + 30
    cellSize = 50
    cellSizePx = cellSize

    rl.SetTargetFPS 60
    //--------------------------------------------------------------------------------------

    // Main game loop
    while not rl.WindowShouldClose    // Detect window close button or ESC key

        // Update
        //----------------------------------------------------------------------------------
        monitorCount = rl.GetMonitorCount

        if monitorCount > 1 and rl.IsKeyPressed(rl.KEY_N) then
            rl.SetWindowMonitor (rl.GetCurrentMonitor + 1) % monitorCount
        end if

        currentMonitor = rl.GetCurrentMonitor
        dpiScale = rl.GetWindowScaleDPI
        cellSizePx = cellSize/dpiScale.x;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        rl.BeginDrawing

           rl.ClearBackground rl.RAYWHITE

           windowCenter = rl.GetScreenWidth/2
           DrawTextCenter TextFormat("Dpi Scale: %f", dpiScale.x), windowCenter, 30, 40, rl.DARKGRAY
           DrawTextCenter TextFormat("Monitor: %d/%d ([N] next monitor)", currentMonitor+1, monitorCount), windowCenter, 70, 20, rl.LIGHTGRAY
           DrawTextCenter TextFormat("Window is %d ""logical points"" wide", rl.GetScreenWidth), windowCenter, logicalGridDescY, 20, rl.ORANGE

           odd = true;
           for i in range(cellSize, rl.GetScreenWidth, cellSize)
           	odd = not odd
               if (odd) then rl.DrawRectangle i, logicalGridTop, cellSize, logicalGridBottom-logicalGridTop, rl.ORANGE

               DrawTextCenter TextFormat("%d", i), i, logicalGridLabelY, 10, rl.LIGHTGRAY
               rl.DrawLine i, logicalGridLabelY + 10, i, logicalGridBottom, rl.GRAY
           end for

           odd = true
           minTextSpace = 30;
           lastTextX = -minTextSpace;
           for i in range(cellSize, rl.GetRenderWidth, cellSize)
           	odd = not odd
               x = floor(i/dpiScale.x)
               if (odd) then rl.DrawRectangle x, pixelGridTop, floor(cellSizePx), pixelGridBottom - pixelGridTop, [0, 121, 241, 100]

               rl.DrawLine x, pixelGridTop, floor(i/dpiScale.x), pixelGridLabelY - 10, rl.GRAY

               if x - lastTextX >= minTextSpace then
                   DrawTextCenter TextFormat("%d", i), x, pixelGridLabelY, 10, rl.LIGHTGRAY
                   lastTextX = x
               end if
           end for

		   DrawTextCenter TextFormat("Window is %d ""physical pixels"" wide", rl.GetRenderWidth), windowCenter, pixelGridDescY, 20, rl.BLUE

		   text = "Can you see this?"
		   size = rl.MeasureTextEx(rl.GetFontDefault, text, 20, 3)
		   pos = [rl.GetScreenWidth - size.x - 5, rl.GetScreenHeight - size.y - 5]
		   rl.DrawTextEx rl.GetFontDefault, text, pos, 20, 3, rl.LIGHTGRAY

        rl.EndDrawing
        //----------------------------------------------------------------------------------
	end while

    // De-Initialization
    //--------------------------------------------------------------------------------------
    rl.CloseWindow        // Close window and OpenGL context
    exit				// exit the program
    //--------------------------------------------------------------------------------------
end function

if locals == globals then main
