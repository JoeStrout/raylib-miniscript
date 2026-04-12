#!/usr/bin/env miniscript

// This script generates API_DOC.md by crawling the source files.

import "stringUtil"


magicValues = {
	"Value::null": "null",
	"Value::zero": "0",
	"Value::one": "1",
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
	for line in file.readLines(path)
		line = line.trim

		// Accumulate inline doc comments
		if line.startsWith("// ") then
			pendingNote = line[3:]
			continue
		end if

		// Blank lines reset the pending note; comments set it; all other lines
		// leave it intact so it survives across Create/AddParam/code=/lambda lines.
		if line == "" then
			pendingNote = ""
			continue
		end if

		if line.contains("= Intrinsic::Create(") then
			params = []
			continue
		end if

		m = line.match("{var}->AddParam(""{param}"", {defval});")
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**=" + valToText(m.defval)
			end if
			continue
		end if

		m = line.match("{var}->AddParam(""{param}"");")
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**"
			end if
			continue
		end if

		// Match: moduleVar.SetValue("api", x->GetFunc()) or moduleVar.SetValue(String("api"), x->GetFunc())
		m = line.match(moduleVar + ".SetValue(""{api}"", {var}->GetFunc());")
		if not m then m = line.match(moduleVar + ".SetValue(String(""{api}""), {var}->GetFunc());")
		if m then
			if apiCount == 0 then
				outLines.push ""
				outLines.push "## " + heading
				outLines.push ""
				outLines.push "|Name | Parameters | Purpose |"
				outLines.push "|-----|------------|---------|"
			end if
			apiCount += 1

			if functionNotes.hasIndex(m.api) then
				note = functionNotes[m.api]
			else if pendingNote != "" then
				note = pendingNote
			else
				print "   NO NOTE FOUND FOR: " + m.api
				note = ""
			end if
			outLines.push "|" + m.api + " |" + params.join(", ") + " |" + note + " |"
			params = []
			pendingNote = ""
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
		if m then
			if m.param != "self" then
				params.push "**" + m.param + "**=" + valToText(m.defval)
			end if
			continue
		end if

		m = line.match("i->AddParam(""{param}"");")
		if not m then m = line.match("{var}->AddParam(""{param}"");")
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

print totalAPIs + " APIs processed in total"

// Finally, write the output.
outPath = "API_DOC.md"
file.writeLines outPath, outLines
print "Wrote " + outLines.len + " lines to " + outPath