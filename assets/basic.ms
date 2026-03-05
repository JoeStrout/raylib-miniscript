// main.ms
// Simple test script for MSRLWeb

raylib.SetWindowTitle "MSRLWeb Demo"

wumpusImg = raylib.LoadImage("assets/Wumpus.png")
raylib.SetWindowIcon wumpusImg

print "basic.ms running!  Version info:"
print version

wumpusTex = raylib.LoadTextureFromImage(wumpusImg)
raylib.UnloadImage wumpusImg
woobleSnd = raylib.LoadSound("assets/wooble.wav")

rgb = function(r=0, g=0, b=0, a=255)
	return {"r":r, "g":g, "b":b, "a":a}
end function

drawBackground = function
	// Sunset sky gradient
	raylib.DrawRectangleGradientV 0, 0, 960, 640, rgb(200, 200, 255), rgb(150, 0, 0)

	// Sun
	raylib.DrawCircle 800, 300, 60, rgb(255, 200, 0, 255)
	raylib.DrawCircle 800, 300, 50, raylib.YELLOW

	// Mountains (note: we have one clockwise and one anti-clockwise
	// here, just to prove that in MSRL, it works both ways!)
	raylib.DrawTriangle [50, 640], [500, 640], [300, 400], rgb(60, 40, 80, 255)
	raylib.DrawTriangle [400, 640], [600, 350], [900, 640], rgb(80, 50, 100, 255)
end function

run = function
	x = 100; y = 100
	dx = 10; dy = 10
	
	print "Move with arrow keys, or click with the mouse."
	while true
		raylib.BeginDrawing
		raylib.ClearBackground
		
		drawBackground
		raylib.DrawTexture wumpusTex, x, y, raylib.WHITE
	
		if raylib.IsKeyDown(raylib.KEY_LEFT) then x -= dx
		if raylib.IsKeyDown(raylib.KEY_RIGHT) then x += dx
		if raylib.IsKeyDown(raylib.KEY_UP) then y -= dy
		if raylib.IsKeyDown(raylib.KEY_DOWN) then y += dy
	
		if raylib.IsMouseButtonPressed(raylib.MOUSE_BUTTON_LEFT) then
			pos = raylib.GetMousePosition
			print "Clicked at " + pos.x + ", " + pos.y
			x = pos.x
			y = pos.y
			raylib.SetSoundPitch woobleSnd, 1 - (pos.y - 320) / 640
			raylib.SetSoundPan woobleSnd, 1 - pos.x / 960  // NOTE: 0=right, 1=left
			raylib.PlaySound woobleSnd
		end if
		
		raylib.DrawFPS
		raylib.EndDrawing
		yield
	end while
end function

print "in basic.ms with " + locals.len + " locals and " + globals.len + " globals"
if locals == globals then run
