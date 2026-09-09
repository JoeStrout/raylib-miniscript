// C4.ms
//
// Connect-4 rules, matching `connect4/env.py` in ML-work-2026/rl_connect4.
//
// The board is a flat 42-element list of 0 (empty), 1, or 2, indexed
// `row*COLS + col` with **row 0 at the bottom**.  That orientation is not
// cosmetic: it is the one the network was trained on, and a board built upside
// down would still run, and would play like nonsense.

ROWS = 6
COLS = 7
CELLS = 42

newBoard = function
	board = []
	for i in range(1, CELLS)
		board.push 0
	end for
	return board
end function

// Lowest empty row in a column, or -1 if the column is full.
dropRow = function(board, col)
	for r in range(0, ROWS - 1, 1)
		if board[r * COLS + col] == 0 then return r
	end for
	return -1
end function

legalCols = function(board)
	out = []
	top = (ROWS - 1) * COLS
	for c in range(0, COLS - 1, 1)
		if board[top + c] == 0 then out.push c
	end for
	return out
end function

// Does the piece just played at (r, c) by player p complete a line of four?
isWin = function(board, r, c, p)
	for d in [[0, 1], [1, 0], [1, 1], [1, -1]]
		count = 1
		for sign in [1, -1]
			rr = r + sign * d[0]
			cc = c + sign * d[1]
			while rr >= 0 and rr < ROWS and cc >= 0 and cc < COLS and
			   board[rr * COLS + cc] == p
				count = count + 1
				rr = rr + sign * d[0]
				cc = cc + sign * d[1]
			end while
		end for
		if count >= 4 then return true
	end for
	return false
end function

// The network's two input planes, from `player`'s point of view: own pieces
// first, then the opponent's.  Mirrors env.observation().
planes = function(board, player)
	own = []
	opp = []
	for v in board
		if v == player then own.push 1 else own.push 0
		if v != 0 and v != player then opp.push 1 else opp.push 0
	end for
	return [own, opp]
end function

// Text rendering, bottom row last -- the same shape env.render() produces, so
// a position can be eyeballed against the Python side.
render = function(board)
	glyphs = [".", "X", "O"]
	lines = []
	for r in range(ROWS - 1, 0, -1)
		row = []
		for c in range(0, COLS - 1, 1)
			row.push glyphs[board[r * COLS + c]]
		end for
		lines.push row.join(" ")
	end for
	lines.push "0 1 2 3 4 5 6"
	return lines.join(char(10))
end function
