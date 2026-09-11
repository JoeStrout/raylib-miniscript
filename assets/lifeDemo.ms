// lifeDemo.ms
// Conway's Game of Life, computed with Matrix.convolve.
//
// The whole board advances in five matrix operations per generation -- one
// convolution to count neighbors, then four elementwise passes for the rule --
// with no per-cell script code and no allocation.  Two same-sized buffers are
// convolved back and forth; each has a one-cell border that refreshBorder fills
// before every step (wrapped for a torus, or zeros for dead edges).
//
// Controls: SPACE pause, N step (while paused), UP/DOWN generations per frame,
// R randomize, C clear, G glider gun, E wrap/dead edges, Z cell size,
// mouse left/right to draw/erase.  ESC to exit.

rl = raylib
if not rl.IsWindowReady then rl.InitWindow
import "matrixUtil"

// Every cell is 0 (dead) or 255 (alive).  255 rather than 1 so the buffer can
// go straight into a grayscale texture, with no scaling pass for display.
//
// With cells worth 255, this kernel gives each cell 4*255*n + 2*255*s, where n
// counts live neighbors and s is the cell itself: that is 510*X, for X = 2n + s.
// A cell is alive next generation exactly when X is 5, 6 or 7 -- 5 is a live
// cell with 2 neighbors, 6 and 7 are any cell with 3 -- which is the whole of
// Conway's rule.  Every value involved is an integer, so it is all exact.
lifeKernel = Matrix.fromList([[4,4,4],
                              [4,2,4],
                              [4,4,4]])

// The rule, in place: 510*X in, 0 or 255 out.
//   765 - |510*X - 3060|  =  765 - 510*|X - 6|
// is 765 when X = 6, 255 when X is 5 or 7, and -255 or less otherwise, so
// clamping it to 0..255 leaves exactly 255 for the cells that live.
applyRule = function(m)
	m.subtract 3060                             // 510*(X - 6)
	m.abs                                       // 510*|X - 6|
	m.gemm null, 765, m, false, false, -1       // 765 - 510*|X - 6|, in one pass
	m.clamp 0, 255
end function

// The Gosper glider gun, which fires a glider every 30 generations.
gliderGun = [
	"........................O...........",
	"......................O.O...........",
	"............OO......OO............OO",
	"...........O...O....OO............OO",
	"OO........O.....O...OO..............",
	"OO........O...O.OO....O.O...........",
	"..........O.....O.......O...........",
	"...........O...O....................",
	"............OO......................"]

Life = {}

Life.make = function(rows, cols)
	life = new Life
	life.rows = rows
	life.cols = cols
	life.generation = 0
	// The two buffers, each with a one-cell border for the 3x3 kernel.
	life.src = Matrix.ofSize(rows + 2, cols + 2)
	life.dst = Matrix.ofSize(rows + 2, cols + 2)
	// The texture is the whole buffer, border included; drawing shows only the
	// interior.  toRawData writes one byte per cell, which is exactly the
	// grayscale pixel format.
	life.pixels = new RawData
	img = rl.GenImageColor(cols + 2, rows + 2, rl.BLACK)
	rl.ImageFormat img, rl.PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
	life.texture = rl.LoadTextureFromImage(img)
	rl.UnloadImage img
	rl.SetTextureFilter life.texture, rl.TEXTURE_FILTER_POINT
	return life
end function

Life.unload = function
	rl.UnloadTexture self.texture
end function

// Advance one generation.  edgeMode is refreshBorder's: "wrap" or 0.
Life.step = function(edgeMode)
	self.src.refreshBorder lifeKernel, edgeMode
	self.src.convolve lifeKernel, self.dst
	applyRule self.dst
	temp = self.src; self.src = self.dst; self.dst = temp
	self.generation += 1
end function

Life.clear = function
	self.src.fill 0
	self.generation = 0
end function

// About a quarter of the cells alive: normal deviates above 0.6745 are 25%.
Life.randomize = function
	cells = Matrix.ofSize(self.rows, self.cols).randomize.greaterThan(0.6745)
	self.src.setSub 1, 1, cells.elemMultiplyBy(255)
	self.generation = 0
end function

Life.setCell = function(row, col, value)
	if row < 0 or row >= self.rows or col < 0 or col >= self.cols then return
	self.src.setElem row + 1, col + 1, value
end function

Life.place = function(pattern, top, left)
	for r in pattern.indexes
		for c in range(0, pattern[r].len - 1)
			if pattern[r][c] == "O" then self.setCell top + r, left + c, 255
		end for
	end for
end function

Life.upload = function
	self.src.toRawData self.pixels, "uint8", 0, false
	rl.UpdateTexture self.texture, self.pixels
end function

Life.draw = function(left, top, cellSize, tint)
	rl.DrawTexturePro self.texture, [1, 1, self.cols, self.rows],
	  [left, top, self.cols * cellSize, self.rows * cellSize], [0, 0], 0, tint
end function

run = function
	rl.SetWindowTitle "Life"
	rl.SetTargetFPS 60
	screenW = rl.GetScreenWidth
	screenH = rl.GetScreenHeight
	barH = 56
	cellColor = {"r": 120, "g": 230, "b": 140, "a": 255}

	cellSizes = [1, 2, 3, 4, 6, 8]
	sizeIdx = 2
	edgeModes = ["wrap", 0]
	edgeNames = ["wrapped edges", "dead edges"]
	edgeIdx = 0
	paused = false
	stepsPerFrame = 1
	stepMs = 0

	cellSize = cellSizes[sizeIdx]
	life = Life.make(floor((screenH - barH) / cellSize), floor(screenW / cellSize))
	life.randomize

	print "Life.  SPACE pause, N step, UP/DOWN speed, R random, C clear, G gun, E edges, Z size, ESC exit."

	while true
		if rl.IsKeyPressed(rl.KEY_ESCAPE) then break

		if rl.IsKeyPressed(rl.KEY_SPACE) then paused = not paused
		if rl.IsKeyPressed(rl.KEY_R) then life.randomize
		if rl.IsKeyPressed(rl.KEY_C) then life.clear
		if rl.IsKeyPressed(rl.KEY_G) then
			life.clear
			life.place gliderGun, 2, 2
		end if
		if rl.IsKeyPressed(rl.KEY_E) then edgeIdx = 1 - edgeIdx
		if rl.IsKeyPressed(rl.KEY_UP) and stepsPerFrame < 64 then stepsPerFrame *= 2
		if rl.IsKeyPressed(rl.KEY_DOWN) and stepsPerFrame > 1 then stepsPerFrame /= 2
		if rl.IsKeyPressed(rl.KEY_Z) then
			sizeIdx = (sizeIdx + 1) % cellSizes.len
			cellSize = cellSizes[sizeIdx]
			life.unload
			life = Life.make(floor((screenH - barH) / cellSize), floor(screenW / cellSize))
			life.randomize
		end if

		// Draw or erase with the mouse (a few cells wide when cells are tiny).
		mx = rl.GetMouseX; my = rl.GetMouseY - barH
		drawing = rl.IsMouseButtonDown(rl.MOUSE_BUTTON_LEFT)
		if drawing or rl.IsMouseButtonDown(rl.MOUSE_BUTTON_RIGHT) then
			value = 255 * drawing
			col = floor(mx / cellSize); row = floor(my / cellSize)
			brush = floor(3 / cellSize)
			for dr in range(-brush, brush)
				for dc in range(-brush, brush)
					life.setCell row + dr, col + dc, value
				end for
			end for
		end if

		steps = 0
		if not paused then
			steps = stepsPerFrame
		else if rl.IsKeyPressed(rl.KEY_N) or rl.IsKeyPressedRepeat(rl.KEY_N) then
			steps = 1
		end if
		if steps > 0 then
			t0 = time
			for i in range(1, steps)
				life.step edgeModes[edgeIdx]
			end for
			stepMs = (time - t0) * 1000 / steps
		end if
		life.upload

		rl.BeginDrawing
		rl.ClearBackground {"r": 16, "g": 20, "b": 24, "a": 255}
		life.draw 0, barH, cellSize, cellColor
		rl.DrawText "Life: " + life.cols + " x " + life.rows + ", " + edgeNames[edgeIdx] +
		  "   generation " + life.generation + "   " + stepsPerFrame + "/frame   " +
		  round(stepMs, 2) + " ms/generation" + " (paused)" * paused, 10, 8, 20, rl.RAYWHITE
		rl.DrawText "SPACE pause  N step  UP/DOWN speed  R random  C clear  G gun  E edges  Z size  mouse draw/erase",
		  10, 32, 16, rl.GRAY
		rl.DrawFPS screenW - 90, 8
		rl.EndDrawing
		yield
	end while

	life.unload
	print "Life demo finished."
end function

if locals == globals then run
