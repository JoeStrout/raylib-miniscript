// profview.ms -- view a profile made with `raylib-miniscript --profile`.
//
// Usage:
//   raylib-miniscript tools/profview.ms myprofile.speedscope.json
//
// Shows a flame graph of where the time went (each bar is a function, its
// width its share of the time, the functions it called stacked beneath it)
// and a table of functions by self and total time.  Click a bar to zoom in;
// right-click, Escape or Backspace to zoom out.  Click a table row to light up
// that function wherever it appears.  Tab switches between the profile's two
// views, By function and By line.
//
// Options (after the file name):
//   --view <n>            start in view n (0 = By function, 1 = By line)
//   --select <name>       highlight the first function whose name contains name
//   --zoom <name>         zoom to the heaviest bar whose name contains name
//   --screenshot <file>   draw one frame, save it as <file>, and quit
//   --mouse <x>,<y>       (for testing) keep the mouse at x, y
//   --selftest            check the profile's totals add up, print, and quit

rl = raylib

//==============================================================================
// JSON reader
//
// Reads one line at a time: JSON never breaks a token across lines, and short
// lines keep string indexing cheap (strings index by code point, so indexing
// far into a long non-ASCII string is slow).
//==============================================================================

Json = {}
Json.lines = null
Json.lineNum = -1
Json.s = ""      // current line
Json.i = 0       // position in it
Json.n = 0       // its length
Json.error = null   // the first problem found, as a message

// Parse JSON text.  Returns [value, null], or [null, error message].
Json.parse = function(text)
	r = new Json
	r.lines = text.split(char(10))
	r.lineNum = -1
	r.s = ""
	r.i = 0
	r.n = 0
	r.error = null
	result = r.value
	if r.error == null then
		r.skipWhitespace
		if r.lineNum < r.lines.len then r.fail "unexpected text after the end"
	end if
	if r.error != null then return [null, r.error]
	return [result, null]
end function

// Note the first error and jump to the end of the text, so that every caller
// unwinds quickly.  Returns null, for `return self.fail(...)`.
Json.fail = function(msg)
	if self.error == null then self.error = "JSON error on line " + (self.lineNum + 1) + ": " + msg
	self.lineNum = self.lines.len
	self.s = ""
	self.i = 0
	self.n = 0
	return null
end function

// Move to the next line; false at the end of the text.
Json.nextLine = function
	self.lineNum += 1
	if self.lineNum >= self.lines.len then
		self.s = ""
		self.i = 0
		self.n = 0
		return false
	end if
	self.s = self.lines[self.lineNum]
	self.i = 0
	self.n = self.s.len
	return true
end function

Json.skipWhitespace = function
	while true
		if self.i >= self.n then
			if not self.nextLine then return
			continue
		end if
		c = self.s[self.i]
		if c != " " and c != char(9) and c != char(13) then return
		self.i += 1
	end while
end function

// The next significant character, without consuming it ("" at the end).
Json.peek = function
	self.skipWhitespace
	if self.i >= self.n then return ""
	return self.s[self.i]
end function

Json.value = function
	c = self.peek
	if c == "{" then return self.object
	if c == "[" then return self.array
	if c == """" then return self.string
	if c == "t" then return self.word("true", true)
	if c == "f" then return self.word("false", false)
	if c == "n" then return self.word("null", null)
	if c == "" then return self.fail("unexpected end of text")
	return self.number
end function

Json.word = function(w, result)
	if self.s[self.i : self.i + w.len] != w then return self.fail("expected " + w)
	self.i += w.len
	return result
end function

Json.number = function
	s = self.s
	start = self.i
	i = start
	n = self.n
	while i < n and "-+.eE0123456789".indexOf(s[i]) != null
		i += 1
	end while
	if i == start then return self.fail("unexpected " + s[i])
	self.i = i
	return val(s[start:i])
end function

Json.string = function
	self.i += 1   // opening quote
	s = self.s
	n = self.n
	parts = []
	start = self.i
	i = start
	while true
		if i >= n then return self.fail("unterminated string")
		c = s[i]
		if c == """" then break
		if c == "\" then
			parts.push s[start:i]
			e = s[i + 1]
			if e == "n" then
				parts.push char(10)
			else if e == "t" then
				parts.push char(9)
			else if e == "r" then
				parts.push char(13)
			else if e == "b" then
				parts.push char(8)
			else if e == "f" then
				parts.push char(12)
			else if e == "u" then
				parts.push char(hexValue(s[i + 2 : i + 6]))
				i += 4
			else
				parts.push e   // \" \\ \/
			end if
			i += 2
			start = i
			continue
		end if
		i += 1
	end while
	parts.push s[start:i]
	self.i = i + 1
	return parts.join("")
end function

Json.array = function
	self.i += 1
	result = []
	if self.peek == "]" then
		self.i += 1
		return result
	end if
	while true
		result.push self.value
		if self.error != null then return null
		c = self.peek
		self.i += 1
		if c == "]" then return result
		if c != "," then return self.fail("expected , or ]")
	end while
end function

Json.object = function
	self.i += 1
	result = {}
	if self.peek == "}" then
		self.i += 1
		return result
	end if
	while true
		if self.peek != """" then return self.fail("expected a key")
		key = self.string
		if self.peek != ":" then return self.fail("expected :")
		self.i += 1
		result[key] = self.value
		if self.error != null then return null
		c = self.peek
		self.i += 1
		if c == "}" then return result
		if c != "," then return self.fail("expected , or }")
	end while
end function

hexValue = function(h)
	result = 0
	for c in h.lower
		result = result * 16 + "0123456789abcdef".indexOf(c)
	end for
	return result
end function

//==============================================================================
// Profile model
//==============================================================================

// Load a speedscope file.  Returns a map with `title`, `frames` and `profiles`,
// or a string describing what went wrong.
loadProfile = function(path)
	text = rl.LoadFileText(path)
	if text == null or text == "" then return "Could not read " + path
	parsed = Json.parse(text)
	if parsed[1] != null then return path + ": " + parsed[1]
	data = parsed[0]
	if not (data isa map) or not data.hasIndex("shared") or not data.hasIndex("profiles") then
		return path + " is not a speedscope profile"
	end if
	result = {}
	result.title = path
	if data.hasIndex("name") then result.title = data.name
	result.frames = []
	for f in data.shared.frames
		result.frames.push makeFrame(f)
	end for
	result.profiles = []
	for p in data.profiles
		if p.type != "sampled" then continue
		result.profiles.push {"name": p.name, "samples": p.samples, "weights": p.weights}
	end for
	if not result.profiles then return path + " has no sampled profiles"
	return result
end function

// A frame from the file, plus what the viewer needs: its kind, the function
// it belongs to (a By line frame "foo line 12" belongs to "foo"), and a color.
makeFrame = function(f)
	fr = {}
	fr.name = f.name
	fr.file = ""
	if f.hasIndex("file") then fr.file = f.file
	fr.line = 0
	if f.hasIndex("line") then fr.line = f.line
	fr.kind = "ms"
	if fr.name[0] == "[" then
		fr.kind = "call"
	else if fr.name == "(garbage collection)" then
		fr.kind = "gc"
	else if fr.name.indexOf("(host") == 0 then
		fr.kind = "host"
	end if
	fr.func = fr.name
	p = fr.name.indexOf(" line ")
	if p == null then p = fr.name.indexOf(" (entry)")
	if p != null and fr.kind == "ms" then fr.func = fr.name[:p]
	fr.color = frameColor(fr)
	fr.textWidth = null   // measured on first draw
	return fr
end function

// Call tree and per-frame totals for one profile.  Weights are microseconds.
View = {}

View.make = function(prof, frames)
	v = new View
	v.prof = prof
	v.frames = frames
	v.root = newNode(-1, null)
	v.selfTime = [0] * frames.len
	v.totalTime = [0] * frames.len
	samples = prof.samples
	weights = prof.weights
	for k in range(0, samples.len - 1, 1)
		w = weights[k]
		node = v.root
		node.total += w
		seen = {}
		for fi in samples[k]
			kids = node.children
			if kids.hasIndex(fi) then
				node = kids[fi]
			else
				child = newNode(fi, node)
				kids[fi] = child
				node = child
			end if
			node.total += w
			if not seen.hasIndex(fi) then
				v.totalTime[fi] += w
				seen[fi] = true
			end if
		end for
		node.own += w
		if samples[k] then v.selfTime[samples[k][-1]] += w
	end for
	v.total = v.root.total
	setHeight v.root
	return v
end function

// Give each node the number of rows below it (0 for a leaf), for scrolling.
setHeight = function(node)
	h = 0
	for kid in node.children.values
		kh = setHeight(kid) + 1
		if kh > h then h = kh
	end for
	node.height = h
	return h
end function

newNode = function(frameIndex, parent)
	node = {}
	node.frame = frameIndex
	node.parent = parent
	node.total = 0
	node.own = 0
	node.children = {}   // frame index -> node
	node.sorted = null   // children, heaviest first (made when first needed)
	node.height = 0      // rows below this node
	return node
end function

sortedChildren = function(node)
	if node.sorted == null then
		node.sorted = node.children.values
		node.sorted.sort "total", false
	end if
	return node.sorted
end function

// The heaviest node whose frame name contains `part`, or null.
View.findNode = function(part)
	best = null
	todo = [self.root]
	while todo
		node = todo.pop
		if node.frame >= 0 and self.frames[node.frame].name.indexOf(part) != null then
			if best == null or node.total > best.total then best = node
		end if
		for kid in node.children.values
			todo.push kid
		end for
	end while
	return best
end function

// Time per kind of frame (by self time): MiniScript code, timed calls, GC, host.
View.kindTotals = function
	result = {"ms": 0, "call": 0, "gc": 0, "host": 0}
	for i in self.selfTime.indexes
		result[self.frames[i].kind] += self.selfTime[i]
	end for
	return result
end function

// Check that the totals are consistent; returns a list of problems.
View.check = function
	problems = []
	sum = 0
	for w in self.prof.weights
		sum += w
	end for
	if abs(sum - self.total) > 0.01 then problems.push "root " + self.total + " != weights " + sum
	selfSum = 0
	for t in self.selfTime
		selfSum += t
	end for
	if abs(selfSum - self.total) > 0.01 then problems.push "self times " + selfSum + " != total " + self.total
	todo = [self.root]
	count = 0
	while todo
		node = todo.pop
		count += 1
		kidSum = node.own
		for kid in node.children.values
			kidSum += kid.total
			todo.push kid
		end for
		if abs(kidSum - node.total) > 0.01 then problems.push "node " + node.frame + " children+self " + kidSum + " != " + node.total
	end while
	self.nodeCount = count
	return problems
end function

//==============================================================================
// Colors
//==============================================================================

BACKGROUND = [24, 26, 32, 255]

// Warm colors for MiniScript functions, picked by name so a function keeps its
// color in both views; cool ones for timed calls, green for GC, gray for host.
WARM = [[232, 152, 84], [227, 178, 92], [214, 128, 88], [236, 196, 118],
	[219, 160, 110], [240, 140, 100], [205, 170, 96], [230, 120, 110],
	[224, 186, 140], [212, 146, 120]]

frameColor = function(fr)
	if fr.kind == "call" then return [104, 150, 214, 255]
	if fr.kind == "gc" then return [110, 180, 120, 255]
	if fr.kind == "host" then return [150, 150, 150, 255]
	h = 0
	for c in fr.func
		h = (h * 31 + c.code) % 1000003
	end for
	c = WARM[h % WARM.len]
	return [c[0], c[1], c[2], 255]
end function

//==============================================================================
// Drawing helpers
//==============================================================================

FONT = 10          // raylib's default font is crisp at multiples of 10
ROW = 18           // flame graph row height
TROW = 16          // table row height
HEADER = 60
STATUS = 22
TEXT = [230, 232, 238, 255]
DIM_TEXT = [150, 156, 168, 255]
BAR_TEXT = [26, 26, 30, 255]
PANEL = [34, 37, 46, 255]
RULE = [58, 62, 76, 255]
ROW_ALT = [29, 31, 39, 255]
ROW_HOVER = [48, 53, 66, 255]
ROW_SELECTED = [70, 78, 104, 255]
WHITE = [255, 255, 255, 255]

drawText = function(s, x, y, color=null, size=10)
	if color == null then color = TEXT
	rl.DrawText s, floor(x), floor(y), size, color
end function

drawTextRight = function(s, right, y, color=null)
	drawText s, right - rl.MeasureText(s, FONT), y, color
end function

// A frame's name shortened with ".." to fit `width` pixels, or "" if nothing fits.
fitLabel = function(fr, width)
	if fr.textWidth == null then fr.textWidth = rl.MeasureText(fr.name, FONT)
	return fitText(fr.name, width, fr.textWidth)
end function

// Text shortened with ".." to fit `width` pixels, or "" if nothing fits.
// fullWidth is the text's own width, if already known.
fitText = function(text, width, fullWidth=null)
	if fullWidth == null then fullWidth = rl.MeasureText(text, FONT)
	if fullWidth <= width then return text
	if width < 20 then return ""
	n = floor(text.len * width / fullWidth)
	while n > 0
		label = text[:n] + ".."
		if rl.MeasureText(label, FONT) <= width then return label
		n -= 1
	end while
	return ""
end function

// A color blended most of the way toward the background.
dim = function(c)
	return [floor(c[0] * 0.3 + BACKGROUND[0] * 0.7), floor(c[1] * 0.3 + BACKGROUND[1] * 0.7),
		floor(c[2] * 0.3 + BACKGROUND[2] * 0.7), 255]
end function

// Microseconds as milliseconds with one decimal, and a share as a percentage.
fmtMs = function(us)
	return fixed(us / 1000, 1)
end function

fmtPct = function(part, whole)
	if whole <= 0 then return "0.0%"
	return fixed(100 * part / whole, 1) + "%"
end function

fixed = function(x, places)
	scale = 10 ^ places
	r = round(x * scale)
	whole = floor(r / scale)
	frac = str(r - whole * scale)
	while frac.len < places
		frac = "0" + frac
	end while
	return str(whole) + "." + frac
end function

// Where a frame is defined: "file.ms line 12", "line 12", "file.ms", or "".
location = function(fr)
	if fr.line <= 0 then return fr.file
	if fr.file then return fr.file + " line " + fr.line
	return "line " + fr.line
end function

// Whether a list holds this very map.  (Not indexOf: == compares maps by
// content, which for linked tree nodes is slow at best.)
inList = function(list, item)
	for x in list
		if refEquals(x, item) then return true
	end for
	return false
end function

contains = function(x, y, w, h, px, py)
	return px >= x and px < x + w and py >= y and py < y + h
end function

//==============================================================================
// The viewer
//==============================================================================

UI = {}
UI.profile = null
UI.view = null
UI.viewIndex = 0
UI.path = []           // zoom path, root first; the last is the bar in focus
UI.hover = null        // flame graph node under the mouse
UI.hoverRow = null     // table row under the mouse
UI.selected = -1       // frame index lit up in the flame graph, or -1
UI.flameScroll = 0
UI.tableScroll = 0
UI.sortBy = "own"      // or "total"
UI.rows = null         // table rows, sorted (made when first needed)
UI.buttons = []        // clickable areas this frame: [x, y, w, h, action]

UI.setView = function(index)
	self.viewIndex = index
	self.view = View.make(self.profile.profiles[index], self.profile.frames)
	self.path = [self.view.root]
	self.selected = -1
	self.flameScroll = 0
	self.tableScroll = 0
	self.rows = null
end function

UI.tableRows = function
	if self.rows != null then return self.rows
	rows = []
	v = self.view
	for i in v.totalTime.indexes
		if v.totalTime[i] > 0 then rows.push {"frame": i, "own": v.selfTime[i], "total": v.totalTime[i]}
	end for
	rows.sort self.sortBy, false
	self.rows = rows
	return rows
end function

UI.zoomTo = function(node)
	path = []
	while node != null
		path.insert 0, node
		node = node.parent
	end while
	self.path = path
	self.flameScroll = 0
end function

UI.zoomOut = function
	if self.path.len > 1 then
		self.path.pop
		self.flameScroll = 0
	end if
end function

UI.select = function(frameIndex)
	if self.selected == frameIndex then self.selected = -1 else self.selected = frameIndex
end function

// One frame: draw everything, then act on the mouse and keyboard.
UI.frame = function
	W = rl.GetScreenWidth
	H = rl.GetScreenHeight
	mx = rl.GetMouseX
	my = rl.GetMouseY
	tableH = floor((H - HEADER - STATUS) * 0.4)
	flameY = HEADER
	flameH = H - HEADER - STATUS - tableH
	tableY = flameY + flameH

	// Scrolling goes to whichever panel the mouse is over.
	wheel = rl.GetMouseWheelMove
	if wheel != 0 then
		if contains(0, flameY, W, flameH, mx, my) then
			self.flameScroll -= wheel * ROW * 3
			limit = (self.path.len + self.path[-1].height) * ROW - flameH
			if self.flameScroll > limit then self.flameScroll = limit
			if self.flameScroll < 0 then self.flameScroll = 0
		else if contains(0, tableY, W, tableH, mx, my) then
			self.tableScroll -= round(wheel * 3)
			limit = self.tableRows.len - floor((tableH - TROW - 4) / TROW)
			if self.tableScroll > limit then self.tableScroll = limit
			if self.tableScroll < 0 then self.tableScroll = 0
		end if
	end if

	rl.BeginDrawing
	rl.ClearBackground BACKGROUND
	self.buttons = []
	self.drawFlame 0, flameY, W, flameH, mx, my
	self.drawTable 0, tableY, W, tableH, mx, my
	self.drawHeader W
	self.drawStatus W, H
	self.drawTooltip W, H, mx, my
	rl.EndDrawing

	// Mouse: header buttons, then the flame graph, then the table.
	if rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_LEFT) then
		handled = false
		for b in self.buttons
			if contains(b[0], b[1], b[2], b[3], mx, my) then
				self.act b[4]
				handled = true
				break
			end if
		end for
		if not handled and self.hover != null then
			self.zoomTo self.hover
		else if not handled and self.hoverRow != null then
			self.select self.hoverRow.frame
		end if
	end if
	if rl.IsMouseButtonPressed(rl.MOUSE_BUTTON_RIGHT) then self.zoomOut
	if rl.IsKeyPressed(rl.KEY_ESCAPE) or rl.IsKeyPressed(rl.KEY_BACKSPACE) then self.zoomOut
	if rl.IsKeyPressed(rl.KEY_HOME) then self.zoomTo self.view.root
	if rl.IsKeyPressed(rl.KEY_TAB) then self.setView (self.viewIndex + 1) % self.profile.profiles.len
end function

UI.act = function(action)
	if action isa number then
		if action != self.viewIndex then self.setView action
	else if action == "own" or action == "total" then
		self.sortBy = action
		self.rows = null
		self.tableScroll = 0
	end if
end function

UI.drawHeader = function(W)
	rl.DrawRectangle 0, 0, W, HEADER, PANEL
	rl.DrawLine 0, HEADER - 1, W, HEADER - 1, RULE
	drawText self.profile.title, 12, 10, TEXT, 20
	x = 12 + rl.MeasureText(self.profile.title, 20) + 24
	for i in self.profile.profiles.indexes
		label = self.profile.profiles[i].name
		w = rl.MeasureText(label, FONT) + 20
		if i == self.viewIndex then
			rl.DrawRectangle x, 10, w, 20, ROW_SELECTED
		else
			rl.DrawRectangleLines x, 10, w, 20, RULE
		end if
		drawText label, x + 10, 15
		self.buttons.push [x, 10, w, 20, i]
		x += w + 6
	end for
	v = self.view
	k = v.kindTotals
	parts = ["Total " + fixed(v.total / 1000000, 2) + " s"]
	parts.push "MiniScript code " + fixed(k.ms / 1000000, 2) + " s (" + fmtPct(k.ms, v.total) + ")"
	if k.call > 0 then parts.push "timed raylib calls " + fixed(k.call / 1000000, 2) + " s (" + fmtPct(k.call, v.total) + ")"
	parts.push "garbage collection " + fixed(k.gc / 1000000, 2) + " s (" + fmtPct(k.gc, v.total) + ")"
	if k.host > 0 then parts.push "host " + fixed(k.host / 1000000, 2) + " s (" + fmtPct(k.host, v.total) + ")"
	drawText parts.join("     "), 12, 40, DIM_TEXT
end function

UI.drawStatus = function(W, H)
	rl.DrawRectangle 0, H - STATUS, W, STATUS, PANEL
	help = "Click a bar to zoom in   Right-click, Esc or Backspace: zoom out   Home: zoom all the way out   " +
		"Click a row to light it up   Tab: switch view   Wheel: scroll"
	drawText help, 12, H - STATUS + 6, DIM_TEXT
end function

//------------------------------------------------------------------------------
// Flame graph
//------------------------------------------------------------------------------

UI.drawFlame = function(x0, y0, w, h, mx, my)
	self.areaTop = y0
	self.areaBottom = y0 + h
	self.mx = mx
	self.my = my
	self.mouseIn = contains(x0, y0, w, h, mx, my)
	self.hover = null
	rl.BeginScissorMode x0, y0, w, h
	top = y0 - self.flameScroll
	// The zoom path, root first, each across the full width.
	for d in self.path.indexes
		self.drawBar self.path[d], x0, w, top + d * ROW
	end for
	self.drawChildren self.path[-1], x0, w, self.path.len, top
	rl.EndScissorMode
end function

UI.drawChildren = function(node, x, w, depth, top)
	if node.total <= 0 then return
	y = top + depth * ROW
	if y >= self.areaBottom then return
	for kid in sortedChildren(node)
		kw = w * kid.total / node.total
		if kw < 0.5 then break   // heaviest first, so the rest are narrower still
		self.drawBar kid, x, kw, y
		self.drawChildren kid, x, kw, depth + 1, top
		x += kw
	end for
end function

UI.drawBar = function(node, x, w, y)
	if y + ROW <= self.areaTop or y >= self.areaBottom then return
	bx = floor(x)
	bw = floor(x + w) - bx - 1
	if bw < 1 then bw = 1
	if node.frame < 0 then
		color = [96, 100, 116, 255]
		label = "All (" + fixed(node.total / 1000000, 2) + " s)"
	else
		fr = self.view.frames[node.frame]
		color = fr.color
		label = fitLabel(fr, bw - 6)
	end if
	textColor = BAR_TEXT
	if self.selected >= 0 and node.frame != self.selected then
		color = dim(color)
		textColor = DIM_TEXT
	end if
	rl.DrawRectangle bx, y, bw, ROW - 1, color
	if self.mouseIn and contains(x, y, w, ROW, self.mx, self.my) then
		self.hover = node
		rl.DrawRectangleLines bx, y, bw, ROW - 1, WHITE
	end if
	if label then drawText label, bx + 3, y + 4, textColor
end function

UI.drawTooltip = function(W, H, mx, my)
	node = self.hover
	if node == null then return
	v = self.view
	lines = []
	if node.frame < 0 then
		lines.push "All"
	else
		fr = v.frames[node.frame]
		lines.push fr.name
		loc = location(fr)
		if loc then lines.push loc
	end if
	lines.push "Total: " + fmtMs(node.total) + " ms (" + fmtPct(node.total, v.total) + ")"
	if node.frame >= 0 then lines.push "Self:  " + fmtMs(node.own) + " ms (" + fmtPct(node.own, v.total) + ")"
	// Below the zoomed bar, also give its share of that bar.
	if self.path.len > 1 and not inList(self.path, node) then
		lines.push "Of the zoomed bar: " + fmtPct(node.total, self.path[-1].total)
	end if
	w = 0
	for s in lines
		sw = rl.MeasureText(s, FONT)
		if sw > w then w = sw
	end for
	w += 16
	h = lines.len * 14 + 10
	x = mx + 16
	y = my + 16
	if x + w > W then x = mx - w - 8
	if y + h > H then y = my - h - 8
	rl.DrawRectangle x, y, w, h, [16, 17, 22, 240]
	rl.DrawRectangleLines x, y, w, h, RULE
	for i in lines.indexes
		color = TEXT
		if i > 0 then color = DIM_TEXT
		drawText lines[i], x + 8, y + 6 + i * 14, color
	end for
end function

//------------------------------------------------------------------------------
// Function table
//------------------------------------------------------------------------------

UI.drawTable = function(x0, y0, w, h, mx, my)
	rl.DrawRectangle x0, y0, w, h, BACKGROUND
	rl.DrawLine x0, y0, x0 + w, y0, RULE
	cols = {"own": x0 + 90, "selfPct": x0 + 150, "total": x0 + 240, "totalPct": x0 + 300,
		"name": x0 + 324, "loc": x0 + floor(w * 0.62)}
	hy = y0 + 6
	rl.DrawRectangle x0, y0 + 1, w, TROW + 4, PANEL
	// Clickable column headings choose the sort.
	headings = [["own", "Self ms", cols.own, cols.selfPct], ["total", "Total ms", cols.total, cols.totalPct]]
	for hd in headings
		label = hd[1]
		if self.sortBy == hd[0] then label = label + " v"
		color = DIM_TEXT
		if self.sortBy == hd[0] then color = TEXT
		drawTextRight label, hd[2], hy, color
		drawTextRight "%", hd[3], hy, DIM_TEXT
		self.buttons.push [hd[2] - 70, y0, hd[3] - hd[2] + 74, TROW + 4, hd[0]]
	end for
	drawText "Function", cols.name, hy, DIM_TEXT
	drawText "Defined at", cols.loc, hy, DIM_TEXT

	rows = self.tableRows
	v = self.view
	self.hoverRow = null
	y = y0 + TROW + 6
	rl.BeginScissorMode x0, y, w, h - (y - y0)
	i = self.tableScroll
	while i < rows.len and y < y0 + h
		r = rows[i]
		fr = v.frames[r.frame]
		bg = BACKGROUND
		if i % 2 then bg = ROW_ALT
		if contains(x0, y, w, TROW, mx, my) then
			bg = ROW_HOVER
			self.hoverRow = r
		end if
		if r.frame == self.selected then bg = ROW_SELECTED
		rl.DrawRectangle x0, y, w, TROW, bg
		ty = y + 3
		drawTextRight fmtMs(r.own), cols.own, ty
		drawTextRight fmtPct(r.own, v.total), cols.selfPct, ty, DIM_TEXT
		drawTextRight fmtMs(r.total), cols.total, ty
		drawTextRight fmtPct(r.total, v.total), cols.totalPct, ty, DIM_TEXT
		rl.DrawRectangle cols.name, y + 4, 8, 8, fr.color
		drawText fitLabel(fr, cols.loc - cols.name - 30), cols.name + 14, ty
		drawText location(fr), cols.loc, ty, DIM_TEXT
		y += TROW
		i += 1
	end while
	rl.EndScissorMode
end function

//==============================================================================
// Main
//==============================================================================

args = shellArgs[1:]
options = {}
path = null
k = 0
while k < args.len
	a = args[k]
	if a == "--selftest" then
		options.selftest = true
	else if a[:2] == "--" and k + 1 < args.len then
		options[a[2:]] = args[k + 1]
		k += 1
	else if path == null then
		path = a
	end if
	k += 1
end while

if options.hasIndex("selftest") then
	t0 = time
	profile = loadProfile(path)
	if profile isa string then
		print profile
		exit 1
	end if
	print "loaded " + profile.frames.len + " frames, " + profile.profiles.len + " profiles in " + round(time - t0, 3) + " s"
	failures = 0
	for prof in profile.profiles
		t0 = time
		v = View.make(prof, profile.frames)
		problems = v.check
		print prof.name + ": " + prof.samples.len + " samples, " + v.nodeCount + " nodes, total " + round(v.total / 1000, 1) + " ms, built in " + round(time - t0, 3) + " s"
		print "  by kind (us): " + v.kindTotals
		for p in problems[:5]
			print "  PROBLEM: " + p
		end for
		failures += problems.len
	end for
	if failures then
		print "FAILED"
		exit 1
	end if
	print "OK"
	exit 0
end if

//------------------------------------------------------------------------------
// The window
//------------------------------------------------------------------------------

rl.SetTraceLogLevel rl.LOG_WARNING
rl.SetConfigFlags rl.FLAG_WINDOW_RESIZABLE
rl.InitWindow 1280, 800, "MiniScript profile"
rl.SetWindowMinSize 800, 480
rl.SetExitKey 0      // Escape zooms out; close the window to quit
rl.SetTargetFPS 60

// Show something while a large profile loads.
showMessage = function(lines)
	rl.BeginDrawing
	rl.ClearBackground BACKGROUND
	for i in lines.indexes
		drawText lines[i], 24, 24 + i * 26, TEXT, 20
	end for
	rl.EndDrawing
end function

problem = null
if path == null then
	problem = ["Usage: raylib-miniscript tools/profview.ms <profile>.speedscope.json",
		"", "Make a profile with:  raylib-miniscript --profile <name> <game script>"]
else
	showMessage ["Loading " + path + "..."]
	loaded = loadProfile(path)
	if loaded isa string then
		problem = [loaded]
	else
		UI.profile = loaded
		rl.SetWindowTitle loaded.title + " - MiniScript profile"
		start = 0
		if options.hasIndex("view") then start = val(options.view)
		if start < 0 or start >= loaded.profiles.len then start = 0
		UI.setView start
		if options.hasIndex("select") then
			for i in loaded.frames.indexes
				if loaded.frames[i].name.indexOf(options.select) != null and UI.view.totalTime[i] > 0 then
					UI.selected = i
					break
				end if
			end for
		end if
		if options.hasIndex("zoom") then
			node = UI.view.findNode(options.zoom)
			if node != null then UI.zoomTo node
		end if
	end if
end if

if problem != null then print problem.join(char(10))
mousePos = null
if options.hasIndex("mouse") then mousePos = options.mouse.split(",")
frames = 0
while not rl.WindowShouldClose
	if mousePos != null then rl.SetMousePosition val(mousePos[0]), val(mousePos[1])
	if problem != null then showMessage problem else UI.frame
	frames += 1
	if options.hasIndex("screenshot") and frames >= 3 then
		rl.TakeScreenshot options.screenshot
		break
	end if
	yield
end while
rl.CloseWindow
