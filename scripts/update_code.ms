#!/usr/bin/env miniscript

// update_code.ms -- migrate intrinsics from GetVar("name") to GetArg(index).
//
// Background: MS1-style intrinsics read their arguments with
//     context.GetVar(String("name"))
// which does a linear string-compare search of the parameter names on every
// call.  MS2 offers the direct positional form
//     context.GetArg(index)
// where `index` is the 0-based position of that name in the intrinsic's
// AddParam() list.  This tool rewrites the former into the latter.
//
// SCOPE -- it only handles the "inline lambda" style (Pattern A), e.g.
//     i = Intrinsic::Create("...");
//     i.AddParam("posX", Value::zero);
//     i.AddParam("posY", Value::zero);
//     i.set_Code(INTRINSIC_LAMBDA {
//         int posX = context.GetVar(String("posX")).IntValue();
//         int posY = context.GetVar(String("posY")).IntValue();
//         ...
//     });
// where the Create/AddParam calls and the GetVar calls that use them sit in one
// sequential block.  It deliberately does NOT touch intrinsics whose body is a
// separately-defined named function -- there the GetVar calls appear above the
// registration, with no param context in reach, so we leave them for hand
// conversion.  A line of the form  set_Code(&someNamedFunction)  marks such an
// intrinsic and switches conversion off until the next Create.
//
// Only files that actually change are backed up (to <name>.bak, created once and
// never overwritten) and rewritten; unchanged files are left as-is.
//
// Usage:
//     miniscript update_code.ms <dir-or-file> [<dir-or-file> ...] [--revert]
//
//   A directory argument processes every .cpp file directly inside it.
//   A file argument processes just that file.
//   --revert (or -r) restores each target file from its .bak backup instead of
//   converting; the .bak is left in place so you can revert again.

dq = char(34)   // a double-quote character

printUsage = function
	print "Usage: miniscript update_code.ms <dir-or-file> [...] [--revert]"
	print "  <dir-or-file>  a directory (all .cpp files in it) or a single file"
	print "  --revert, -r   restore each target file from its .bak backup"
end function

endsWith = function(s, suffix)
	if s.len < suffix.len then return false
	return s[s.len - suffix.len :] == suffix
end function

// Find the first double-quoted string at or after `startIdx` in `s`.
// Returns a map {text, close} where `close` is the index of the closing quote,
// or null if there is no complete quoted string.  (Avoid the key name "end" --
// it is a reserved word and confuses the lexer as a dot-identifier.)
findQuoted = function(s, startIdx)
	a = s.indexOf(dq, startIdx - 1)
	if a == null then return null
	b = s.indexOf(dq, a)
	if b == null then return null
	return {"text": s[a+1:b], "close": b}
end function

// Given that s[openIdx] is "(", return the index of the matching ")",
// ignoring parentheses that appear inside double-quoted strings.  null if none.
matchParen = function(s, openIdx)
	depth = 0
	inStr = false
	i = openIdx
	while i < s.len
		c = s[i]
		if inStr then
			if c == dq then inStr = false
		else if c == dq then
			inStr = true
		else if c == "(" then
			depth = depth + 1
		else if c == ")" then
			depth = depth - 1
			if depth == 0 then return i
		end if
		i = i + 1
	end while
	return null
end function

// Rewrite every convertible GetVar(...) call on one line, using the ordered
// `params` list (name -> its index).  Returns {line, count}.
convertGetVars = function(line, params)
	marker = "GetVar("
	result = ""
	conversions = 0
	i = 0
	while true
		p = line.indexOf(marker, i - 1)
		if p == null then
			result = result + line[i:]
			break
		end if
		result = result + line[i:p]          // text before the call
		openIdx = p + marker.len - 1          // index of the "(" in "GetVar("
		endIdx = matchParen(line, openIdx)
		if endIdx == null then
			result = result + line[p:]        // malformed; leave the rest alone
			break
		end if
		q = findQuoted(line, openIdx)
		name = null
		if q != null and q.close < endIdx then name = q.text
		idx = null
		if name != null then idx = params.indexOf(name)
		if idx != null then
			result = result + "GetArg(" + idx + ")"
			conversions = conversions + 1
		else
			result = result + line[p:endIdx+1]   // not a known param; leave as-is
		end if
		i = endIdx + 1
	end while
	return {"line": result, "count": conversions}
end function

// Convert one file in place (backing it up first, only if something changes).
processFile = function(path)
	lines = file.readLines(path)
	if lines == null then
		print "  ERROR: could not read " + path
		return
	end if

	params = []       // AddParam names for the intrinsic currently being defined
	active = false    // are we inside a convertible Create...lambda block?
	total = 0
	newLines = []
	for line in lines
		if line.indexOf("Intrinsic::Create(") != null then
			params = []
			active = true
		end if
		if active then
			ap = line.indexOf(".AddParam(")
			if ap != null then
				q = findQuoted(line, ap)
				if q != null then params.push q.text
			end if
		end if
		outLine = line
		if active then
			r = convertGetVars(line, params)
			outLine = r.line
			total = total + r.count
		end if
		if line.indexOf("set_Code(&") != null then active = false
		newLines.push outLine
	end for

	if total == 0 then
		print "  " + path + ": unchanged"
		return
	end if

	bakPath = path + ".bak"
	if not file.exists(bakPath) then
		file.copy path, bakPath
		if not file.exists(bakPath) then
			print "  ERROR: failed to create backup " + bakPath + "; skipping " + path
			return
		end if
	end if

	file.writeLines path, newLines
	print "  " + path + ": " + total + " conversion(s)"
end function

// Restore one file from its .bak backup.
revertFile = function(path)
	bakPath = path + ".bak"
	if not file.exists(bakPath) then
		print "  " + path + ": no backup, skipped"
		return
	end if
	file.copy bakPath, path
	print "  " + path + ": reverted from " + bakPath
end function

// Expand the given targets into a flat list of .cpp files to process.
collectFiles = function(targets)
	files = []
	for arg in targets
		info = file.info(arg)
		if info == null then
			print "skipping (not found): " + arg
			continue
		end if
		if info.isDirectory then
			found = 0
			for name in file.children(arg)
				if endsWith(name, ".cpp") then
					files.push file.child(arg, name)
					found = found + 1
				end if
			end for
			if found == 0 then print "no .cpp files in: " + arg
		else
			files.push arg
		end if
	end for
	return files
end function

main = function
	args = shellArgs[1:]      // drop the script name
	revert = false
	targets = []
	for a in args
		if a == "--revert" or a == "-r" then
			revert = true
		else if a == "--help" or a == "-h" then
			printUsage
			return
		else
			targets.push a
		end if
	end for

	if targets.len == 0 then
		printUsage
		return
	end if

	files = collectFiles(targets)
	if files.len == 0 then
		print "Nothing to do."
		return
	end if

	if revert then
		print "Reverting " + files.len + " file(s):"
		for f in files
			revertFile f
		end for
	else
		print "Converting GetVar -> GetArg in " + files.len + " file(s):"
		for f in files
			processFile f
		end for
	end if
end function

main
