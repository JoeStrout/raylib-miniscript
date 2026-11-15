#!/usr/bin/env miniscript

// This script generates API_DOC.md by crawling the source files.  Run it from
// the repo root:  ./build/raylib-miniscript scripts/gen_doc.ms
//
// Each entry's "Purpose" text is the binding's description: the note in its
// WithNote(...) call (SetNote(...) for a global), which is also what `info`
// shows.  Change the description there -- never edit API_DOC.md by
// hand, the next run would undo it.

import "stringUtil"

//----------------------------------------------------------------------
// Reading the bindings
//----------------------------------------------------------------------

// MiniScript 2 renamed several of these; keep the old spellings too so this
// table still works if an older binding turns up.
magicValues = {
	"Value::null": "null",
	"Value::Null": "null",
	"Value::zero": "0",
	"Value::one": "1",
	"Value::emptyString": """""",
	"String()": """""",
	"Value::make_list(0)": "[]",
	"QuaternionToValue(QuaternionIdentity())": "QuaternionIdentity",
	"MatrixToValue(MatrixIdentity())": "MatrixIdentity" }

valToText = function(cppVal)
	if magicValues.hasIndex(cppVal) then return magicValues[cppVal]
	m = cppVal.match("Value({val})")
	if m then
		if (m.val.startsWith("Vector2{") or m.val.startsWith("Vector3{") or 
		  m.val.startsWith("Vector4{")) and m.val[-1] == "}" then
			return "[" + m.val[8:-1] + "]"
		end if
		return m.val
	end if
	return cppVal
end function

// A parameter as API_DOC.md shows it, if line is an AddParam call; else null.
// Intrinsic was a pointer under MiniScript 1 and is a value under MiniScript
// 2, so accept "i->AddParam" as well as "i.AddParam", or an older plugin
// silently documents no parameters.
parseParam = function(line)
	m = line.match("{var}.AddParam(""{param}"", {defval});")
	if not m then m = line.match("{var}->AddParam(""{param}"", {defval});")
	if m then return "**" + m.param + "**=" + valToText(m.defval)
	m = line.match("{var}.AddParam(""{param}"");")
	if not m then m = line.match("{var}->AddParam(""{param}"");")
	if m then return "**" + m.param + "**"
	return null
end function

// Parse adjacent C++ string literals ("ab" "cd") starting at s[i], skipping
// space between them.  Returns [text, index after the last one], or null if
// s[i] does not start a literal.
parseLiterals = function(s, i)
	result = []
	got = false
	while true
		while i < s.len and (s[i] == " " or s[i] == char(9))
			i += 1
		end while
		if i >= s.len or s[i] != """" then break
		got = true
		i += 1
		while i < s.len and s[i] != """"
			if s[i] == "\" and i + 1 < s.len then
				i += 1
				c = s[i]
				if c == "n" then c = char(10)
				if c == "t" then c = char(9)
				result.push c
			else
				result.push s[i]
			end if
			i += 1
		end while
		i += 1		// past the closing quote
	end while
	if not got then return null
	return [result.join(""), i]
end function

// Parse `<moduleVar>.SetValue("name", WithNote(<var>, "note..."));`, a whole
// statement on one line.  Returns {api, note}, or null.
parseWithNote = function(stmt, moduleVar)
	head = moduleVar + ".SetValue("""
	if not stmt.startsWith(head) then return null
	q = stmt.indexOf("""", head.len - 1)
	if q == null then return null
	api = stmt[head.len:q]
	rest = stmt[q+1:]
	if not rest.startsWith(", WithNote(") then return null
	comma = rest.indexOf(",", 10)
	if comma == null then return null
	lit = parseLiterals(rest, comma + 1)
	if lit == null or rest[lit[1]:].trim != "));" then return null
	return {"api": api, "note": lit[0]}
end function

// Parse `SetNote(<var>, "note...");`, a whole statement on one line.
// Returns {var, note}, or null.
parseSetNote = function(stmt)
	if not stmt.startsWith("SetNote(") then return null
	comma = stmt.indexOf(",")
	if comma == null then return null
	lit = parseLiterals(stmt, comma + 1)
	if lit == null or stmt[lit[1]:].trim != ");" then return null
	return {"var": stmt[8:comma].trim, "note": lit[0]}
end function

// The statement starting at src[i], joined onto one line, and the index after
// it: a WithNote or SetNote call puts its note on lines of its own.
joinStatement = function(src, i)
	line = src[i].trim
	i += 1
	while not line.endsWith(";") and i < src.len
		line = line + " " + src[i].trim
		i += 1
	end while
	return [line, i]
end function

// Scan a .cpp binding file for the intrinsics it registers in moduleVar
// (e.g. "raylibModule", "fileModule").  Returns a list of {api, params, note}
// maps, in source order; note is "" for one registered without WithNote.
scanBindingFile = function(path, moduleVar="raylibModule")
	entries = []
	src = file.readLines(path)
	params = []
	i = 0
	while i < src.len
		line = src[i].trim
		if line.contains("= Intrinsic::Create(") then
			params = []
			i += 1
			continue
		end if
		p = parseParam(line)
		if p != null then
			if p.indexOf("**self**") != 0 then params.push p
			i += 1
			continue
		end if
		if not line.startsWith(moduleVar + ".SetValue(") then
			i += 1
			continue
		end if
		stmt = joinStatement(src, i)
		line = stmt[0]
		i = stmt[1]
		m = parseWithNote(line, moduleVar)
		if m then
			entries.push {"api": m.api, "params": params, "note": m.note}
		else
			m = line.match(moduleVar + ".SetValue(""{api}"", {var}.GetFunc());")
			if not m then m = line.match(moduleVar + ".SetValue(String(""{api}""), {var}.GetFunc());")
			if not m then m = line.match(moduleVar + ".SetValue(""{api}"", {var}->GetFunc());")
			if not m then m = line.match(moduleVar + ".SetValue(String(""{api}""), {var}->GetFunc());")
			if m then entries.push {"api": m.api, "params": params, "note": ""}
		end if
		params = []
	end while
	return entries
end function

// Scan a .cpp file for intrinsics created by name (globals, like `exit`):
// `Intrinsic xxx = Intrinsic::Create("name")`, each with a SetNote(xxx, ...)
// after its set_Code.  Returns a list of {api, params, note} maps.
scanGlobalIntrinsicsFile = function(path)
	entries = []
	src = file.readLines(path)
	cur = null
	i = 0
	while i < src.len
		line = src[i].trim
		if line.startsWith("SetNote(") then
			stmt = joinStatement(src, i)
			i = stmt[1]
			sn = parseSetNote(stmt[0])
			if sn and cur and sn.var == cur.var then cur.note = sn.note
			continue
		end if
		i += 1
		if cur then
			p = parseParam(line)
			if p != null then
				if p.indexOf("**self**") != 0 then cur.params.push p
				continue
			end if
		end if
		m = line.match("Intrinsic::Create(""{name}"");")
		if m and m.name != "" then
			eq = line.indexOf(" = ")
			var = ""
			if eq != null then var = line[:eq].split(" ")[-1]
			cur = {"api": m.name, "var": var, "params": [], "note": ""}
			entries.push cur
		end if
	end while
	return entries
end function

//----------------------------------------------------------------------
// Writing the doc
//----------------------------------------------------------------------

// Process the given .cpp file, adding a table to the output for all APIs found.
// moduleVar: the C++ variable name used in SetValue calls (e.g. "raylibModule", "fileModule")
// heading: the section heading to use in the output (e.g. "RCore", "file module")
// If heading is "", it is derived from the filename.
processBindingFile = function(path, moduleVar="raylibModule", heading="")
	print "Processing: " + path + " (" + moduleVar + ")"
	if heading == "" then heading = file.name(path) - ".cpp"
	entries = scanBindingFile(path, moduleVar)
	for e in entries
		if e.note == "" then print "   NO NOTE FOUND FOR: " + e.api
	end for
	if entries then
		outLines.push ""
		outLines.push "## " + heading
		outLines.push ""
		outLines.push "|Name | Parameters | Purpose |"
		outLines.push "|-----|------------|---------|"
	end if
	for e in entries
		outLines.push "|" + e.api + " |" + e.params.join(", ") + " |" + e.note + " |"
	end for
	print "   " + entries.len + " APIs"
	return entries.len
end function

// Process files with global named intrinsics (e.g. MoreIntrinsics.cpp),
// into one table: Intrinsic::Create("name") with a non-empty name.
processGlobalIntrinsicsFiles = function(paths, heading="Global Intrinsics")
	entries = []
	for path in paths
		print "Processing: " + path + " (global intrinsics)"
		entries += scanGlobalIntrinsicsFile(path)
	end for
	if entries then
		outLines.push ""
		outLines.push "## " + heading
		outLines.push ""
		outLines.push "|Name | Parameters | Purpose |"
		outLines.push "|-----|------------|---------|"
	end if
	for e in entries
		if e.note == "" then print "   NO NOTE FOUND FOR: " + e.api
		outLines.push "|" + e.api + " |" + e.params.join(", ") + " |" + e.note + " |"
	end for
	print "   " + entries.len + " APIs"
	return entries.len
end function

// Markers fencing the generated part of a plugin's API.md.  Anything the plugin
// author writes outside them (setup notes, caveats, examples) survives a rerun.
pluginBeginMarker = "<!-- BEGIN GENERATED API (scripts/gen_doc.ms; edits between the markers are lost) -->"
pluginEndMarker = "<!-- END GENERATED API -->"

// Write lines (the generated table) into the plugin's API.md between the
// markers.  A new file also gets a title.  An existing file keeps everything
// outside the markers; one with no markers is left alone, since we can't tell
// which part is ours.
writePluginDoc = function(path, name, lines)
	block = [pluginBeginMarker] + lines + [pluginEndMarker]
	if not file.info(path) then
		file.writeLines path, ["# " + name + " plugin", ""] + block
		print "   wrote " + path
		return
	end if
	old = file.readLines(path)
	first = null
	last = null
	for i in old.indexes
		if first == null and old[i].startsWith("<!-- BEGIN GENERATED API") then first = i
		if old[i].startsWith("<!-- END GENERATED API") then last = i
	end for
	if first == null or last == null or last < first then
		print "   " + path + " has no BEGIN/END GENERATED API markers; left unchanged"
		return
	end if
	file.writeLines path, old[:first] + block + old[last+1:]
	print "   updated " + path
end function

// Process the plugins under plugins/ (compile-time plugins; see plugins/README.md).
// Each plugin folder's .cpp files are scanned for the function that fills the
// plugin's module map -- found by its "ValueDict& <var>)" parameter -- and the
// module's name by its PluginAddModule<&Fill>("name") call.  A plugin's API goes
// into its own <folder>/API.md, not into API_DOC.md, which just gets a list of
// links (added to pluginLinks) -- but only if inTree: a plugin from an external
// directory (MS_EXTRA_PLUGIN_DIRS) may be private, so its path stays out of
// the public doc.
processPlugins = function(pluginsDir, inTree=true)
	info = file.info(pluginsDir)
	if not info or not info.isDirectory then return 0
	total = 0
	for folder in file.children(pluginsDir).sort
		dir = file.child(pluginsDir, folder)
		dirInfo = file.info(dir)
		if not dirInfo or not dirInfo.isDirectory then continue
		if dir == "plugins/examples" then continue   // done separately, below
		moduleName = folder
		fillFiles = {}   // path -> name of the map variable its fill function takes
		for child in file.children(dir).sort
			if child[-4:] != ".cpp" then continue
			path = file.child(dir, child)
			for line in file.readLines(path)
				line = line.trim
				m = line.match("PluginAddModule<&{fn}>(""{name}"");")
				if m then moduleName = m.name
				m = line.match("{pre}ValueDict& {var})")
				if m and not m.pre.contains("=") then fillFiles[path] = m.var
			end for
		end for
		if not fillFiles then
			print "Plugin " + folder + ": no function filling a ValueDict& found, skipping"
			continue
		end if
		// processBindingFile appends to outLines; collect this plugin's output separately.
		mainLines = outLines
		outer.outLines = []
		count = 0
		for kv in fillFiles
			count += processBindingFile(kv.key, kv.value, moduleName)
		end for
		pluginLines = outLines
		outer.outLines = mainLines
		total += count
		if count == 0 then continue
		writePluginDoc file.child(dir, "API.md"), moduleName, pluginLines
		if inTree then pluginLinks.push "- [" + moduleName + "](" + dir + "/API.md)"
	end for
	return total
end function

// Begin by verifying that we can access the files we need.
bindingSrc = "src"
bindingSrcInfo = file.info(bindingSrc)
if not bindingSrcInfo or not bindingSrcInfo.isDirectory then
	print "Unable to read " + bindingSrc
	exit
end if

// Prepare the output buffer (outLines).
outLines = []


// Now, process each file of the form R*.cpp in our src directory
totalAPIs = 0
for child in file.children(bindingSrc)
	if child[0] != "R" or child[-4:] != ".cpp" then continue
//	if child != "RMath.cpp" then continue
	totalAPIs += processBindingFile(file.child(bindingSrc, child))
end for

// Process non-raylib intrinsic files
totalAPIs += processGlobalIntrinsicsFiles([file.child(bindingSrc, "MoreIntrinsics.cpp"),
  file.child(bindingSrc, "InterpModule.cpp")], "Global Intrinsics")
totalAPIs += processBindingFile(file.child(bindingSrc, "FileModule.cpp"), "fileModule", "file module")
totalAPIs += processBindingFile(file.child(bindingSrc, "FileModule.cpp"), "fileHandleClass", "FileHandle")
totalAPIs += processBindingFile(file.child(bindingSrc, "HttpModule.cpp"), "httpMap", "http module")
totalAPIs += processBindingFile(file.child(bindingSrc, "RawData.cpp"), "rawDataClass", "RawData")
totalAPIs += processBindingFile(file.child(bindingSrc, "MatrixClass.cpp"), "matrixClass", "Matrix")
totalAPIs += processBindingFile(file.child(bindingSrc, "PhysicsCore.cpp"), "physicsModule", "physicsCore")
totalAPIs += processBindingFile(file.child(bindingSrc, "InterpModule.cpp"), "interpClass", "Interp")

// And the plugins (compiled in from plugins/*; each adds its own module),
// with the examples (built only on request)
pluginLinks = []   // links to each in-tree plugin's API.md, added by processPlugins
totalAPIs += processPlugins("plugins")
totalAPIs += processPlugins("plugins/examples")
if pluginLinks then
	outLines.push ""
	outLines.push "## Plugins"
	outLines.push ""
	outLines.push "Each plugin documents its own API, in an API.md next to its source.  The examples"
	outLines.push "(plugins/examples) are built only when asked for; see plugins/README.md."
	outLines.push ""
	for link in pluginLinks
		outLines.push link
	end for
end if

// Plugins from outside the repo: MS_EXTRA_PLUGIN_DIRS, ";"-separated, as for CMake.
// They get their own API.md but no link in this file.
if env.hasIndex("MS_EXTRA_PLUGIN_DIRS") then
	for extraDir in env.MS_EXTRA_PLUGIN_DIRS.split(";")
		if extraDir and extraDir != "plugins/examples" then totalAPIs += processPlugins(extraDir, false)
	end for
end if

print totalAPIs + " APIs processed in total"

// Finally, write the output.
outPath = "API_DOC.md"
file.writeLines outPath, outLines
print "Wrote " + outLines.len + " lines to " + outPath