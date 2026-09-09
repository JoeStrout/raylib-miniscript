// connect4.ms
//
// Play Connect-4 against the AlphaZero network trained in
// ML-work-2026/rl_connect4, running entirely in MiniScript on the intrinsic
// Matrix class. Requires these helper files:
//
//      AZNet.ms    the 1.19M-parameter residual conv net (inference)
//      C4.ms       game rules
//      MCTS.ms     PUCT tree search
//      az_c4.msmx  the trained weights, written by export_msmx.py
//
// A single network evaluation costs about 8 ms, so a 100-simulation search
// takes roughly a second.  Rather than freeze the window for it, the search is
// stepped incrementally -- `MCTS.step` runs as many simulations as fit in a
// frame's budget and returns -- so the board stays live and you can watch the
// visit counts fill in underneath it while the net thinks.
//
// Controls:
//      click a column   drop a piece
//      [ / ]            halve / double the simulations per move
//      R                new game
//      S                swap who moves first
//      ESC              quit

import "C4"
import "MCTS"
import "AZNet"

rl = raylib

WIDTH = 640
HEIGHT = 748

if not rl.IsWindowReady then rl.InitWindow WIDTH, HEIGHT, "Connect 4 vs AlphaZero"
rl.SetWindowTitle "Connect 4 vs AlphaZero"
rl.SetTargetFPS 60


//----------------------------------------------------------------------
// Layout and palette.
//----------------------------------------------------------------------

CELL = 78
BOARD_X = 47
BOARD_Y = 150
BOARD_W = C4.COLS * CELL
BOARD_H = C4.ROWS * CELL
DISC_R = CELL * 0.39

BG_COLOR = [24, 26, 32, 255]
BOARD_COLOR = [34, 74, 178, 255]
HOLE_COLOR = [16, 18, 24, 255]
HUMAN_COLOR = [228, 72, 72, 255]
AI_COLOR = [246, 202, 72, 255]
TEXT_COLOR = [232, 234, 240, 255]
DIM_COLOR = [128, 134, 148, 255]
TRACK_COLOR = [52, 56, 66, 255]      // progress-bar and visit-bar background
HINT_COLOR = [88, 94, 108, 255]
BAR_COLOR = [92, 202, 142, 255]

HUMAN = 1
AI = 2

discColor = function(v)
	if v == HUMAN then return HUMAN_COLOR
	return AI_COLOR
end function

// Screen centre of cell (r, c).  Row 0 is the bottom of the board, so it draws
// at the bottom of the rectangle.
cellCenter = function(r, c)
	return [BOARD_X + c * CELL + CELL / 2,
	        BOARD_Y + (C4.ROWS - 1 - r) * CELL + CELL / 2]
end function

// Column under the mouse, or -1.  The band above the board counts too, since
// that is where the piece about to drop is drawn.
mouseCol = function
	mx = rl.GetMouseX
	my = rl.GetMouseY
	if my < BOARD_Y - CELL or my > BOARD_Y + BOARD_H then return -1
	if mx < BOARD_X or mx >= BOARD_X + BOARD_W then return -1
	return floor((mx - BOARD_X) / CELL)
end function

centerText = function(s, y, size, color)
	rl.DrawText s, (WIDTH - rl.MeasureText(s, size)) / 2, y, size, color
end function


//----------------------------------------------------------------------
// Game state.
//----------------------------------------------------------------------

board = null
toMove = 0
humanFirst = true
state = ""             // "human", "thinking", or "over"
winner = 0
winLine = null         // cells of the winning four, for the highlight
thinkStart = 0
thinkTime = 0

// The four cells through (r, c) that complete a line for p, or null.
findWinLine = function(board, r, c, p)
	for d in [[0, 1], [1, 0], [1, 1], [1, -1]]
		cells = [[r, c]]
		for sign in [1, -1]
			rr = r + sign * d[0]
			cc = c + sign * d[1]
			while rr >= 0 and rr < C4.ROWS and cc >= 0 and cc < C4.COLS and
			   board[rr * C4.COLS + cc] == p
				cells.push [rr, cc]
				rr = rr + sign * d[0]
				cc = cc + sign * d[1]
			end while
		end for
		if cells.len >= 4 then return cells
	end for
	return null
end function

startThinking = function
	globals.state = "thinking"
	MCTS.begin board, AI
	globals.thinkStart = time
end function

newGame = function
	globals.board = C4.newBoard
	globals.winner = 0
	globals.winLine = null
	globals.thinkTime = 0
	MCTS.root = null
	if humanFirst then
		globals.toMove = HUMAN
		globals.state = "human"
	else
		globals.toMove = AI
		startThinking
	end if
end function

// Drop a piece into `col` and advance the game state.
playMove = function(col, player)
	r = C4.dropRow(board, col)
	if r < 0 then return false
	board[r * C4.COLS + col] = player
	if C4.isWin(board, r, col, player) then
		globals.winner = player
		globals.winLine = findWinLine(board, r, col, player)
		globals.state = "over"
	else if C4.legalCols(board).len == 0 then
		globals.winner = 0
		globals.state = "over"
	else
		globals.toMove = 3 - player
		if toMove == AI then
			startThinking
		else
			globals.state = "human"
		end if
	end if
	return true
end function


//----------------------------------------------------------------------
// Drawing.
//----------------------------------------------------------------------

drawBoard = function(hover)
	// The piece that would drop if you clicked, floating above its column.
	if hover >= 0 and C4.dropRow(board, hover) >= 0 then
		c = discColor(HUMAN)
		rl.DrawCircle BOARD_X + hover * CELL + CELL / 2, BOARD_Y - CELL / 2,
		   DISC_R, [c[0], c[1], c[2], 110]
	end if

	rl.DrawRectangleRounded [BOARD_X, BOARD_Y, BOARD_W, BOARD_H], 0.03, 12,
	   BOARD_COLOR

	for r in range(0, C4.ROWS - 1, 1)
		for c in range(0, C4.COLS - 1, 1)
			p = cellCenter(r, c)
			v = board[r * C4.COLS + c]
			if v == 0 then
				rl.DrawCircle p[0], p[1], DISC_R, HOLE_COLOR
			else
				rl.DrawCircle p[0], p[1], DISC_R, discColor(v)
			end if
		end for
	end for

	// Ring the winning four.
	if winLine != null then
		for cell in winLine
			p = cellCenter(cell[0], cell[1])
			rl.DrawRing p, DISC_R - 5, DISC_R, 0, 360, 32, TEXT_COLOR
		end for
	end if
end function

// Per-column visit counts under the board.  This is the search's real output:
// the shape of it says far more about what the net is considering than the
// single number it finally plays.
drawVisits = function
	if MCTS.root == null then return
	counts = MCTS.visitCounts
	total = 0
	best = 0
	for n in counts
		total = total + n
		if n > best then best = n
	end for
	if total == 0 then return

	y = BOARD_Y + BOARD_H + 12
	h = 30
	for c in range(0, C4.COLS - 1, 1)
		x = BOARD_X + c * CELL + CELL / 2 - 15
		rl.DrawRectangle x, y, 30, h, TRACK_COLOR
		bar = round(counts[c] / best * h)
		if bar > 0 then rl.DrawRectangle x, y + h - bar, 30, bar, BAR_COLOR
	end for
end function

drawStatus = function
	centerText "Connect 4 vs AlphaZero", 20, 28, TEXT_COLOR

	if state == "human" then
		centerText "Your move", 58, 22, HUMAN_COLOR
	else if state == "thinking" then
		centerText "Thinking... " + MCTS.done + " / " + MCTS.sims, 58, 22,
		   AI_COLOR
		rl.DrawRectangle BOARD_X, 92, BOARD_W, 6, TRACK_COLOR
		w = round(BOARD_W * MCTS.done / MCTS.sims)
		if w > 0 then rl.DrawRectangle BOARD_X, 92, w, 6, AI_COLOR
	else
		if winner == HUMAN then
			centerText "You win!", 58, 22, HUMAN_COLOR
		else if winner == AI then
			centerText "AlphaZero wins", 58, 22, AI_COLOR
		else
			centerText "A draw", 58, 22, TEXT_COLOR
		end if
		centerText "press R for a new game", 90, 16, DIM_COLOR
	end if

	y = BOARD_Y + BOARD_H + 54
	line = MCTS.sims + " sims/move"
	if thinkTime > 0 then
		line = line + "     " + round(thinkTime, 2) + "s last move" +
		   "     " + MCTS.evals + " evaluations"
	end if
	rl.DrawText line, BOARD_X, y, 16, DIM_COLOR

	if MCTS.root != null and MCTS.root.N > 0 then
		// Positive means the net expects to win, from its own point of view.
		rl.DrawText "net evaluation: " + round(MCTS.rootValue, 3),
		   BOARD_X, y + 22, 16, DIM_COLOR
	end if

	rl.DrawText "[ ] strength    R new game    S swap sides    ESC quit",
	   BOARD_X, y + 44, 15, HINT_COLOR
end function


//----------------------------------------------------------------------
// Main.
//----------------------------------------------------------------------

print "Loading network..."
t0 = time
net = AZNet.load
if net isa error then
	print "Failed to load the network: " + net
	print "Run export_msmx.py in ML-work-2026/rl_connect4 to write " +
	   "assets/az_c4.msmx."
	exit
end if
MCTS.net = net
print "Loaded " + net.w.len + " weight matrices in " + round(time - t0, 2) + "s"
print "Click a column to play.  [ and ] change search strength, " +
   "R restarts, S swaps sides, ESC quits."

newGame

while true
	if rl.IsKeyPressed(rl.KEY_ESCAPE) then break
	if rl.IsKeyPressed(rl.KEY_R) then newGame
	if rl.IsKeyPressed(rl.KEY_S) then
		humanFirst = not humanFirst
		newGame
	end if
	if rl.IsKeyPressed(rl.KEY_LEFT_BRACKET) and MCTS.sims > 1 then
		MCTS.sims = floor(MCTS.sims / 2)
	end if
	if rl.IsKeyPressed(rl.KEY_RIGHT_BRACKET) and MCTS.sims < 800 then
		MCTS.sims = MCTS.sims * 2
	end if

	hover = -1
	if state == "human" then
		hover = mouseCol
		if hover >= 0 and rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT) then
			if playMove(hover, HUMAN) then hover = -1
		end if
	end if

	// One frame's worth of simulations, then back to drawing.
	if state == "thinking" then
		if MCTS.step then
			globals.thinkTime = time - thinkStart
			playMove MCTS.bestMove, AI
		end if
	end if

	rl.BeginDrawing
	rl.ClearBackground BG_COLOR
	drawStatus
	drawBoard hover
	drawVisits
	rl.EndDrawing
	yield
end while

rl.CloseWindow
