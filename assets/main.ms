// MSRLWeb examples -- main program.
//
// This draws a simple menu of buttons, each leading to another example.

print "Running main.ms"

rl = raylib  // (just to shorten the subsequent code)
rl.SetTraceLogLevel(rl.LOG_WARNING)

titleFont = raylib.LoadFontEx("assets/Merkin.ttf", 48)
print titleFont


mouseCaughtBy = null // Button or other UI element currently handling mouse press

Button = {}
Button.left = 0
Button.top = 0
Button.width = 200
Button.height = 40
Button.caption = "Button"
Button.texture = null
Button.right = function; return self.left + self.width; end function
Button.bottom = function; return self.top + self.height; end function
Button.centerX = function; return self.left + self.width/2; end function
Button.centerY = function; return self.top + self.height/2; end function
Button.hovered = false
Button.pressed = false
Button.action = null

Button.contains = function(x, y)
	return self.left <= x <= self.right and self.top <= y <= self.bottom
end function

Button.containsMouse = function
	return self.contains(rl.GetMouseX, rl.GetMouseY)
end function

Button.updateTexture = function
	if self.texture then rl.UnloadTexture(self.texture)
	
	borderColor = rl.SKYBLUE
	fillColor = rl.BLUE
	textColor = rl.RAYWHITE
	if self.pressed then
		borderColor = rl.ColorLerp(borderColor, rl.BLACK, 0.1)
		fillColor = rl.ColorLerp(fillColor, rl.BLACK, 0.1)
		textColor = rl.YELLOW
	else if self.hovered then
		borderColor = rl.ColorLerp(borderColor, rl.WHITE, 0.25)
		fillColor = rl.ColorLerp(fillColor, rl.WHITE, 0.25)
		textColor = rl.YELLOW
	end if
	fontSize = 20
	
	img = rl.GenImageColor(self.width, self.height)
	rl.ImageDrawRectangleRec img, [0, 0, self.width, self.height], borderColor
	rl.ImageDrawRectangleRec img, [1, 1, self.width-2, self.height-2], fillColor

	textWidth = rl.MeasureText(self.caption, fontSize)
	textX = (self.width - textWidth) / 2
	textY = (self.height - fontSize) / 2
	rl.ImageDrawText img, self.caption, textX, textY, fontSize, textColor

	self.texture = rl.LoadTextureFromImage(img)
	rl.UnloadImage img	
end function

Button.handleMouseDown = function
	self.pressed = true
	self.updateTexture
end function

Button.handleMouseDrag = function
	nowPressed = self.containsMouse
	if nowPressed != self.hovered then
		self.pressed = nowPressed
		self.hovered = nowPressed
		self.updateTexture
	end if
end function

Button.handleMouseUp = function
	if not self.pressed then return
	print self.caption + " clicked!"
	self.pressed = false
	self.updateTexture
	self.action
end function

Button.update = function
	if mouseCaughtBy then return	
	nowHover = self.containsMouse
	if nowHover != self.hovered then
		self.hovered = nowHover
		self.updateTexture
	end if
end function

Button.render = function
	rl.DrawTexture self.texture, self.left, self.top + 2 * self.pressed
end function

Button.Make = function(caption, centerX, centerY, width, height)
	btn = new Button
	if width != null then btn.width = width
	if height != null then btn.height = height
	btn.left = centerX - btn.width/2
	btn.top = centerY - btn.height/2
	btn.caption = caption
	btn.updateTexture
	return btn
end function

updateMouse = function
	if mouseCaughtBy then
		if rl.IsMouseButtonDown then
			mouseCaughtBy.handleMouseDrag
		else
			mouseCaughtBy.handleMouseUp
			globals.mouseCaughtBy = null
		end if
	else if rl.IsMouseButtonDown then
		for btn in buttons
			if btn.containsMouse then
				globals.mouseCaughtBy = btn
				btn.handleMouseDown
				break
			end if
		end for
	end if
end function			

// Create a button for each example
buttons = []
x = 480; y = 150
dy = 60

btn = Button.Make("Basic", x, y)
btn.action = function
	import "basic"
	basic.run
end function
buttons.push btn
y += dy

btn = Button.Make("Asteroids", x, y)
btn.action = function
	import "asteroids"
	asteroids.run
end function
buttons.push btn
y += dy

btn = Button.Make("Breakout", x, y)
btn.action = function
	import "breakout"
	breakout.run
end function
buttons.push btn
y += dy

btn = Button.Make("Sounds", x, y)
btn.action = function
	import "sounds"
	sounds.run
end function
buttons.push btn
y += dy

btn = Button.Make("3D Models", x, y)
btn.action = function
	import "rmodels_smoke"
	rmodels_smoke.run
end function
buttons.push btn
y += dy

btn = Button.Make("Shader Demo", x, y)
btn.action = function
	import "shaderDemo"
	shaderDemo.run
end function
buttons.push btn
y += dy

printCentered = function(s, y, fontSize=20, color)
	if color == null then color = rl.WHITE
	// DrawText uses a spacing equal to fontSize/10, so measure as:

	//size = rl.MeasureTextEx(rl.GetFontDefault, s, fontSize, fontSize/10)
	//rl.DrawText s, 480 - size.x/2, y, fontSize, color
	
	size = rl.MeasureTextEx(titleFont, s, fontSize, 0)
	raylib.DrawTextEx titleFont, s, [480 - size.x/2, y], fontSize, 0, color

end function

// Load fire background shader
fireShader = null
fireQuadTex = null
fragCode = rl.LoadFileText("assets/shaders/fireShader.fs")
if fragCode != "" then
	fireShader = rl.LoadShaderFromMemory("", fragCode)
	if fireShader != null and rl.IsShaderValid(fireShader) then
		globals.fireLocTime = rl.GetShaderLocation(fireShader, "uTime")
		screenW = rl.GetScreenWidth
		screenH = rl.GetScreenHeight
		fireImg = rl.GenImageColor(screenW, screenH, rl.WHITE)
		fireQuadTex = rl.LoadTextureFromImage(fireImg)
		rl.UnloadImage fireImg
		print "Fire shader loaded, locTime=" + fireLocTime
	else
		print "Fire shader failed to compile"
		fireShader = null
	end if
else
	print "Could not load fireShader.fs"
end if

// Main loop
while true
	rl.BeginDrawing
	rl.ClearBackground rl.BLACK

	// update stuff
	updateMouse
	for btn in buttons
		btn.update
	end for

	// render fire background
	if fireShader then
		t = rl.GetTime
		if fireLocTime >= 0 then
			rl.SetShaderValue fireShader, fireLocTime, t, rl.SHADER_UNIFORM_FLOAT
		end if
		rl.BeginShaderMode fireShader
		rl.DrawTexture fireQuadTex, 0, 0, rl.WHITE
		rl.EndShaderMode
	end if

	// render stuff
	printCentered "raylib-miniscript demo", 20, 48, [100,100,255,255]
	printCentered "version " + version.host, 60, 24, [50,50,200,255]
	for btn in buttons
		btn.render
	end for
	rl.DrawFPS

	rl.EndDrawing
	yield
end while
