#!/usr/bin/env miniscript

// This script generates API_DOC.md by crawling the source files.  Run it from
// the repo root:  ./build/raylib-miniscript scripts/gen_doc.ms
//
// Where each entry's "Purpose" text comes from, in order of preference:
//   1. "// API: text" line(s) directly above the Intrinsic::Create of a binding
//      (used verbatim; several in a row are joined with spaces).  Put the doc
//      text here when the generated one isn't right -- never edit API_DOC.md
//      by hand, the next run would undo it.
//   2. an entry in noteOverrides, below
//   3. the note on the function of the same name in the Raylib sources
//   4. the (last line of the) "// ..." comment above the Intrinsic::Create

import "stringUtil"


// MiniScript 2 renamed several of these; keep the old spellings too so this
// table still works if an older binding turns up.
magicValues = {
	"Value::null": "null",
	"Value::Null": "null",
	"Value::zero": "0",
	"Value::one": "1",
	"Value::emptyString": """""",
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

isValidType = function(type)
	if not type then return false
	if not ("A" <= type[0] <= "Z") and not ("a" <= type[0] <= "z") then return false
	if type.contains(" ") then return false
	return true
end function

functionNotes = {}  // key: API name; value: notes from Raylib source

// Notes to use instead of what the Raylib source says, where its comment is
// stale, missing, or doesn't describe our wrapper's behavior.
noteOverrides = {
	"ChangeDirectory": "Change working directory, returns 0 on success (nonzero on failure)",
	"IsPathDirectory": "Check if provided path points to an existing directory",
	"LoadRenderTextureEx": "Load texture for rendering (framebuffer), with specific pixel format",
	"DrawCircleLinesEx": "Draw circle outline with line thickness",
	"ImageDrawImagePro": "Draw a part of an image defined by a rectangle into destination rectangle, with scaling, within an image NOTE: origin and rotation are currently ignored by raylib" }

// Process the given .c file (Raylib source), finding function notes and 
// stuffing them into the global functionNotes map.
processRaylibFile = function(path)
	print "Scanning: " + path
	found = 0
	notes = []
	for line in file.readLines(path)
		if line.startsWith("// ") then
			notes.push line[3:]
			continue
		end if
		if line == "" or line.startsWith("//---") then
			notes = []
		end if
		if line.startsWith("RMAPI ") then line = line[6:]
		m = ("^" + line + "$").match("^{type} {name}({params})$")
		if m then
			if not isValidType(m.type) or m.name.contains(" ") then
				notes = []
				continue
			end if
			if not notes then continue
			if m.name[0] == "*" then m.name = m.name[1:]
			functionNotes[m.name] = notes.join(" ")
			//print " - " + m.name + ": " + functionNotes[m.name]
			notes = []
			found += 1
		end if			
	end for
	print "   " + found + " APIs"
end function

// Process the given .cpp file, adding a table to the output for all APIs found.
// moduleVar: the C++ variable name used in SetValue calls (e.g. "raylibModule", "fileModule")
// heading: the section heading to use in the output (e.g. "RCore", "file module")
// If heading is "", it is derived from the filename.
processBindingFile = function(path, moduleVar="raylibModule", heading="")
	print "Processing: " + path + " (" + moduleVar + ")"
	apiCount = 0
	if heading == "" then heading = file.name(path) - ".cpp"

	params = []
	pendingNote = ""  // inline comment accumulated just before a Create line
	createNote = ""   // pendingNote as it stood at the Create line
	apiNote = ""      // text of "// API: " lines just before a Create line
	createApiNote = ""// apiNote as it stood at the Create line
	for line in file.readLines(path)
		line = line.trim

		// "// API: text" lines are the documentation for the intrinsic that
		// follows, verbatim: they replace the Raylib note and any other comment.
		// Use them where the generated text isn't enough (ownership rules,
		// restrictions on arguments).  Several in a row are joined with spaces.
		if line.startsWith("// API: ") then
			if apiNote != "" then apiNote = apiNote + " "
			apiNote = apiNote + line[8:]
			continue
		end if

		// Accumulate inline doc comments
		if line.startsWith("// ") then
			pendingNote = line[3:]
			continue
		end if

		// Blank lines reset the pending note; comments set it; all other lines
		// leave it intact so it survives across Create/AddParam/code=/lambda lines.
		if line == "" then
			pendingNote = ""
			apiNote = ""
			continue
		end if

		// Snapshot the note here: an inline lambda body can hold comments of
		// its own, which would otherwise replace it before we reach SetValue.
		if line.contains("= Intrinsic::Create(") then
			params = []
			createNote = pendingNote
			createApiNote = apiNote
			apiNote = ""
			continue
		end if

		// Intrinsic was a pointer under MiniScript 1 and is a value under
		// MiniScript 2, so the bindings say "i.AddParam" where they used to say
		// "i->AddParam".  Accept both, or this scanner silently finds nothing.
		m = line.match("{var}->AddParam(""{param}"", {defval});")
		if not m then m = line.match("{var}.AddParam(""{param}"", {defval});")
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**=" + valToText(m.defval)
			end if
			continue
		end if

		m = line.match("{var}->AddParam(""{param}"");")
		if not m then m = line.match("{var}.AddParam(""{param}"");")
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**"
			end if
			continue
		end if

		// Match: moduleVar.SetValue("api", x.GetFunc()) or moduleVar.SetValue(String("api"), x.GetFunc())
		m = line.match(moduleVar + ".SetValue(""{api}"", {var}->GetFunc());")
		if not m then m = line.match(moduleVar + ".SetValue(String(""{api}""), {var}->GetFunc());")
		if not m then m = line.match(moduleVar + ".SetValue(""{api}"", {var}.GetFunc());")
		if not m then m = line.match(moduleVar + ".SetValue(String(""{api}""), {var}.GetFunc());")
		if m then
			if apiCount == 0 then
				outLines.push ""
				outLines.push "## " + heading
				outLines.push ""
				outLines.push "|Name | Parameters | Purpose |"
				outLines.push "|-----|------------|---------|"
			end if
			apiCount += 1

			if createApiNote != "" then
				note = createApiNote
			else if noteOverrides.hasIndex(m.api) then
				note = noteOverrides[m.api]
			else if functionNotes.hasIndex(m.api) then
				note = functionNotes[m.api]
			else if createNote != "" then
				note = createNote
			else if pendingNote != "" then
				note = pendingNote
			else
				print "   NO NOTE FOUND FOR: " + m.api
				note = ""
			end if
			outLines.push "|" + m.api + " |" + params.join(", ") + " |" + note + " |"
			params = []
			pendingNote = ""
			createNote = ""
			createApiNote = ""
		end if
	end for
	print "   " + apiCount + " APIs"
	return apiCount
end function

// Process a file with global named intrinsics (e.g. MoreIntrinsics.cpp).
// Detects Intrinsic::Create("name") with non-empty name, using the preceding
// comment line as documentation.
processGlobalIntrinsicsFile = function(path, heading="Global Intrinsics")
	print "Processing: " + path + " (global intrinsics)"
	apiCount = 0

	params = []
	pendingNote = ""
	currentName = ""
	for line in file.readLines(path)
		line = line.trim

		if line.startsWith("// ") then
			pendingNote = line[3:]
			continue
		end if

		if line == "" then
			pendingNote = ""
			continue
		end if

		m = line.match("i->AddParam(""{param}"", {defval});")
		if not m then m = line.match("{var}->AddParam(""{param}"", {defval});")
		if not m then m = line.match("{var}.AddParam(""{param}"", {defval});")
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**=" + valToText(m.defval)
			end if
			continue
		end if

		m = line.match("i->AddParam(""{param}"");")
		if not m then m = line.match("{var}->AddParam(""{param}"");")
		if not m then m = line.match("{var}.AddParam(""{param}"");")
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**"
			end if
			continue
		end if

		// Named create: Intrinsic *xxxFunc = Intrinsic::Create("name");
		// or: Intrinsic::Create("name")  with any leading variable assignment
		m = line.match("Intrinsic::Create(""{name}"");")
		if m and m.name != "" then
			// Emit previous entry if any
			if currentName != "" then
				if apiCount == 0 then
					outLines.push ""
					outLines.push "## " + heading
					outLines.push ""
					outLines.push "|Name | Parameters | Purpose |"
					outLines.push "|-----|------------|---------|"
				end if
				apiCount += 1
				outLines.push "|" + currentName + " |" + params.join(", ") + " |" + currentNote + " |"
			end if
			currentName = m.name
			currentNote = pendingNote
			params = []
			pendingNote = ""
			continue
		end if
	end for

	// Emit final entry
	if currentName != "" then
		if apiCount == 0 then
			outLines.push ""
			outLines.push "## " + heading
			outLines.push ""
			outLines.push "|Name | Parameters | Purpose |"
			outLines.push "|-----|------------|---------|"
		end if
		apiCount += 1
		outLines.push "|" + currentName + " |" + params.join(", ") + " |" + currentNote + " |"
	end if

	print "   " + apiCount + " APIs"
	return apiCount
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
raylibSrc = file.child("raylib", "src")
raylibSrcInfo = file.info(raylibSrc)
if not raylibSrcInfo or not raylibSrcInfo.isDirectory then
	print "Unable to read " + raylibSrc
	exit
end if

bindingSrc = "src"
bindingSrcInfo = file.info(bindingSrc)
if not bindingSrcInfo or not bindingSrcInfo.isDirectory then
	print "Unable to read " + bindingSrc
	exit
end if

// Prepare the output buffer (outLines).
outLines = []

// Scan the Raylib files for function notes.
platformFolder = file.child(raylibSrc, "platforms")
for child in file.children(platformFolder)
	if child[0] != "r" or child[-2:] != ".c" then continue
	processRaylibFile file.child(platformFolder, child)
end for
for child in file.children(raylibSrc)
	if child[0] != "r" or (child[-2:] != ".c" and child[-2:] != ".h") then continue
	processRaylibFile file.child(raylibSrc, child)
end for


// Now, process each file of the form R*.cpp in our src directory
totalAPIs = 0
for child in file.children(bindingSrc)
	if child[0] != "R" or child[-4:] != ".cpp" then continue
//	if child != "RMath.cpp" then continue
	totalAPIs += processBindingFile(file.child(bindingSrc, child))
end for

// Process non-raylib intrinsic files
totalAPIs += processGlobalIntrinsicsFile(file.child(bindingSrc, "MoreIntrinsics.cpp"), "Global Intrinsics")
totalAPIs += processBindingFile(file.child(bindingSrc, "FileModule.cpp"), "fileModule", "file module")
totalAPIs += processBindingFile(file.child(bindingSrc, "FileModule.cpp"), "fileHandleClass", "FileHandle")
totalAPIs += processBindingFile(file.child(bindingSrc, "HttpModule.cpp"), "httpMap", "http module")
totalAPIs += processBindingFile(file.child(bindingSrc, "RawData.cpp"), "rawDataClass", "RawData")
totalAPIs += processBindingFile(file.child(bindingSrc, "MatrixClass.cpp"), "matrixClass", "Matrix")
totalAPIs += processBindingFile(file.child(bindingSrc, "PhysicsCore.cpp"), "physicsModule", "physicsCore")

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