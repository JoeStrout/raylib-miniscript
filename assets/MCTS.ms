// MCTS.ms
//
// PUCT Monte Carlo tree search over the AlphaZero Connect-4 network: a direct
// port of `connect4/mcts.py` from ML-work-2026/rl_connect4.
//
// The sign convention is the one that trips everybody up, so state it plainly:
// **a node's value is the expected outcome for the player about to move at that
// node.**  Values therefore flip sign at every parent/child boundary -- when
// selecting (the parent negates the child's Q) and when backing up (the value
// is negated at each level on the way to the root).
//
// Root Dirichlet noise is deliberately absent.  It exists to force exploration
// during self-play training; for actual play it only makes the move worse.
//
// The search is **incremental**.  A network evaluation costs about 8 ms, so
// running a whole search at once would freeze a 60 fps window for roughly a
// second.  `step` instead runs as many simulations as fit in a time budget and
// returns, letting the caller draw a frame in between.  Call `begin`, then
// `step` until it returns true, then `bestMove`.

import "C4"

E = 2.718281828459045

net = null           // an AZNet, set by the caller
cPuct = 1.5
sims = 100           // simulations per move
root = null
board = null
player = 0
done = 0             // simulations completed for this move
evals = 0            // network evaluations for this move

newNode = function(prior)
	return {"prior": prior, "children": {}, "actions": [],
	        "N": 0, "W": 0, "expanded": false,
	        "terminal": false, "terminalValue": 0}
end function

// One network evaluation, with the policy masked to legal columns and then
// softmaxed.  Masking *before* the softmax is what stops a full column from
// taking probability away from the moves actually available.
evalPos = function(board, player)
	pl = C4.planes(board, player)
	out = self.net.evaluate(pl[0], pl[1])
	self.evals = self.evals + 1

	legal = C4.legalCols(board)
	best = -1e18
	for a in legal
		if out.logits[a] > best then best = out.logits[a]
	end for
	priors = {}
	total = 0
	for a in legal
		w = E ^ (out.logits[a] - best)
		priors[a] = w
		total = total + w
	end for
	for a in legal
		priors[a] = priors[a] / total
	end for
	return {"priors": priors, "value": out.value, "legal": legal}
end function

// Attach a child for every legal move.  `actions` keeps them in ascending
// order so that selection ties break identically from run to run; map
// iteration order is not something to rely on.
expand = function(node, ev)
	for a in ev.legal
		node.children[a] = self.newNode(ev.priors[a])
		node.actions.push a
	end for
	node.expanded = true
end function

// PUCT: argmax over children of  Q + cPuct * P * sqrt(sum N) / (1 + N).
// Q is stored from the child's point of view, so the parent negates it.
selectChild = function(node)
	totalN = 0
	for a in node.actions
		totalN = totalN + node.children[a].N
	end for
	sq = sqrt(totalN + 1)
	best = -1e18
	bestA = node.actions[0]
	for a in node.actions
		child = node.children[a]
		q = 0
		if child.N > 0 then q = -child.W / child.N
		score = q + self.cPuct * child.prior * sq / (1 + child.N)
		if score > best then
			best = score
			bestA = a
		end if
	end for
	return bestA
end function

// Start a search for `player` to move at `board`.  The root is evaluated
// immediately, so even a 1-simulation search has a real policy behind it.
begin = function(board, player)
	self.board = board[:]
	self.player = player
	self.root = self.newNode(1)
	self.done = 0
	self.evals = 0
	self.expand self.root, self.evalPos(self.board, player)
end function

finished = function
	return self.done >= self.sims
end function

// One simulation: descend by PUCT to a leaf, evaluate it, back the value up
// the path with a sign flip at every level.
simulate = function
	board = self.board[:]
	player = self.player
	node = self.root
	path = [node]

	while node.expanded and not node.terminal
		a = self.selectChild(node)
		node = node.children[a]
		path.push node
		r = C4.dropRow(board, a)
		board[r * C4.COLS + a] = player
		if C4.isWin(board, r, a, player) then
			node.terminal = true
			// The player who just moved won, and it is this node's turn to
			// move -- so from this node the game is already lost.
			node.terminalValue = -1
		else if C4.legalCols(board).len == 0 then
			node.terminal = true
			node.terminalValue = 0
		end if
		player = 3 - player
	end while

	if node.terminal then
		v = node.terminalValue
	else
		ev = self.evalPos(board, player)
		self.expand node, ev
		v = ev.value
	end if

	for i in range(path.len - 1, 0, -1)
		anc = path[i]
		anc.N = anc.N + 1
		anc.W = anc.W + v
		v = -v
	end for

	self.done = self.done + 1
end function

// Run simulations until the search is complete or `budget` seconds are spent.
// Returns true when the search is done.  A budget of 0 runs exactly one
// simulation per call; null runs the search to completion in one go.
step = function(budget = 0.012)
	t0 = time
	while not self.finished
		self.simulate
		if budget != null and time - t0 >= budget then break
	end while
	return self.finished
end function

// Convenience for headless callers: run the whole search and return the move.
search = function(board, player)
	self.begin board, player
	self.step null
	return self.bestMove
end function

// The most-visited move -- AlphaZero's greedy choice.  Visit count, not value,
// is the output of the search: a child can hold a flattering average from a
// handful of visits, while the visit count is what the tree actually committed
// its evaluations to.
bestMove = function
	best = -1
	bestA = self.root.actions[0]
	for a in self.root.actions
		if self.root.children[a].N > best then
			best = self.root.children[a].N
			bestA = a
		end if
	end for
	return bestA
end function

// Root value estimate, from the searching player's point of view.
rootValue = function
	if self.root == null or self.root.N == 0 then return 0
	return self.root.W / self.root.N
end function

// Visit counts across all COLS columns (0 for illegal or unvisited).
visitCounts = function
	counts = []
	for c in range(0, C4.COLS - 1, 1)
		if self.root != null and self.root.children.hasIndex(c) then
			counts.push self.root.children[c].N
		else
			counts.push 0
		end if
	end for
	return counts
end function
