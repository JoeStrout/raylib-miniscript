#!/usr/bin/env miniscript

// gen_wiki.ms - Generates one wiki page per API in the raylib-miniscript wiki.
//
// Pages are placed in folders by module under the wiki directory,
// and named like "raylib.Vector3Angle.md", "file.curdir.md", etc.
//
// Run from the project root directory.

import "stringUtil"

wikiDir = "../raylib-miniscript.wiki"

// ---------------------------------------------------------------------------
// Return-type detection: scan C++ source blocks for IntrinsicResult patterns.
// ---------------------------------------------------------------------------

// Classify a single line containing "return IntrinsicResult...".
// Returns a type string ("null","number","string","list","map") or null if
// the line contains no IntrinsicResult call.
classifyReturnLine = function(line)
	line = line.trim
	if line.contains("IntrinsicResult::Null") then return "null"
	if line.contains("IntrinsicResult::Bool(") then return "number"
	if not line.contains("IntrinsicResult(") then return null
	parts = line.split("IntrinsicResult(")
	if parts.len < 2 then return null
	arg = parts[1]
	if arg.startsWith("Vector2ToValue") then return "list"
	if arg.startsWith("Vector3ToValue") then return "list"
	if arg.startsWith("Vector4ToValue") then return "list"
	if arg.contains("ToValue") then return "map"
	if arg.contains("Class()") then return "map"
	if arg.startsWith("String(") then return "string"
	if arg.startsWith("Value(""") then return "string"
	if arg.startsWith("Value::emptyString") then return "string"
	if arg.startsWith("Value::null") then return "null"
	if arg.startsWith("Value::zero") then return "number"
	if arg.startsWith("Value::one") then return "number"
	if arg.startsWith("Value::Truth(") then return "number"
	if arg.startsWith("(int)") then return "number"
	if arg.startsWith("(double)") then return "number"
	if arg.startsWith("(float)") then return "number"
	if arg.startsWith("-") then return "number"
	if arg.len > 0 and arg[0] >= "0" and arg[0] <= "9" then return "number"
	// Unknown – caller will apply secondary heuristics
	return null
end function

// Infer the return type from a block of C++ lines (the code between
// Intrinsic creation and its SetValue/Create registration).
//
// Strategy:
//   1. Skip conditional guard returns (lines starting with "if (").
//   2. Last classified non-null main return wins.
//   3. If only unclassified main returns exist (hasAmbiguous), scan the block
//      for variable type declarations as a secondary hint.
//   4. Falls back to "null" (void) if nothing is determinable.
inferReturnType = function(block)
	mainRetType = "null"
	hasAmbiguous = false
	for line in block
		lt = line.trim
		if not lt.contains("return IntrinsicResult") then continue
		// Skip conditional guard returns: "if (...) return ..."
		if lt.startsWith("if (") or lt.startsWith("if(") then continue
		t = classifyReturnLine(line)
		if t == null then
			hasAmbiguous = true
		else if t != "null" then
			mainRetType = t
		end if
	end for
	// If no main return type found but there were unclassified returns,
	// look for local variable type declarations as a hint.
	if mainRetType == "null" and hasAmbiguous then
		for line in block
			lt = line.trim
			if lt.startsWith("ValueList ") then return "list"
			if lt.startsWith("ValueDict result") then return "map"
			if lt.startsWith("String result") or lt.startsWith("String str") then return "string"
		end for
		return "number"   // fallback: returns something but can't classify
	end if
	return mainRetType
end function

// apiReturnType: populated by scanning C++ source files below.
apiReturnType = {}

// Pre-scan a file for named "static IntrinsicResult funcName(Context ...)" functions.
// Returns a map of funcName -> returnType.
// These are used when registrations use function pointers (i->code = &funcName;).
prescanFunctions = function(lines)
	funcTypes = {}
	currentFunc = ""
	currentBlock = []
	for line in lines
		trimmed = line.trim
		// Detect function definition:  static IntrinsicResult funcName(Context ...
		// Handle both "Context *context" and "Context* context" spacing styles.
		m = trimmed.match("static IntrinsicResult {name}(Context{rest}")
		if m and m.name != "" then
			if currentFunc != "" then
				funcTypes[currentFunc] = inferReturnType(currentBlock)
			end if
			currentFunc = m.name
			currentBlock = [line]
			continue
		end if
		if currentFunc != "" then
			currentBlock.push line
			// A lone "}" at the start of the line ends the function body
			if line == "}" then
				funcTypes[currentFunc] = inferReturnType(currentBlock)
				currentFunc = ""
				currentBlock = []
			end if
		end if
	end for
	if currentFunc != "" then
		funcTypes[currentFunc] = inferReturnType(currentBlock)
	end if
	return funcTypes
end function

// Scan a module-variable binding file.
// Looks for blocks of code ending with:  moduleVar.SetValue("name", ...)
// Also handles function-pointer registrations (i->code = &funcName;).
scanBindingFile = function(path, moduleVar)
	lines = file.readLines(path)
	if not lines then return
	// Pre-scan named functions in case this file uses function pointers
	funcTypes = prescanFunctions(lines)
	block = []
	codeRef = ""   // name of function referenced via ->code = &funcName
	for line in lines
		trimmed = line.trim
		// Track function-pointer assignments: varName->code = &funcName;
		// or without & (e.g. i->code = intrinsic_foo;)
		m = trimmed.match("{var}->code = &{funcName};")
		if not m then m = trimmed.match("{var}->code = {funcName};")
		if m then codeRef = m.funcName
		// Each new Intrinsic::Create starts a fresh block (prevents the first
		// API from accumulating the entire top of the file as its block).
		if trimmed.contains("Intrinsic::Create(") then
			block = []
		end if
		// Check for SetValue registration (two common forms)
		m = trimmed.match(moduleVar + ".SetValue(""{name}"", {rest}")
		if not m then m = trimmed.match(moduleVar + ".SetValue(String(""{name}""), {rest}")
		if m then
			retType = inferReturnType(block)
			// If block had no inline returns, try the referenced named function
			if retType == "null" and codeRef != "" and funcTypes.hasIndex(codeRef) then
				retType = funcTypes[codeRef]
			end if
			apiReturnType[m.name] = retType
			block = []
			codeRef = ""
		else
			block.push line
		end if
	end for
end function

// Scan a global-intrinsics file.
// Looks for blocks bounded by:  Intrinsic::Create("name");
scanGlobalFile = function(path)
	lines = file.readLines(path)
	if not lines then return
	block = []
	currentName = ""
	for line in lines
		trimmed = line.trim
		m = trimmed.match("{prefix}Intrinsic::Create(""{name}"");")
		if m and m.name != "" then
			if currentName != "" then
				apiReturnType[currentName] = inferReturnType(block)
			end if
			currentName = m.name
			block = []
		else
			block.push line
		end if
	end for
	if currentName != "" then
		apiReturnType[currentName] = inferReturnType(block)
	end if
end function

// ---------------------------------------------------------------------------
// Section configuration
// Each entry:  heading -> [folder, prefix, cppFile, moduleVar, isGlobal]
// isGlobal: 1 = use global-intrinsics scanner; 0 = use binding-file scanner
// ---------------------------------------------------------------------------
sectionConfig = {}
sectionConfig["RMath"]             = ["RMath",      "raylib",     "src/RMath.cpp",          "raylibModule",    0]
sectionConfig["RShapes"]           = ["RShapes",    "raylib",     "src/RShapes.cpp",         "raylibModule",    0]
sectionConfig["RModels"]           = ["RModels",    "raylib",     "src/RModels.cpp",         "raylibModule",    0]
sectionConfig["RTextures"]         = ["RTextures",  "raylib",     "src/RTextures.cpp",       "raylibModule",    0]
sectionConfig["RText"]             = ["RText",      "raylib",     "src/RText.cpp",           "raylibModule",    0]
sectionConfig["RCore"]             = ["RCore",      "raylib",     "src/RCore.cpp",           "raylibModule",    0]
sectionConfig["RAudio"]            = ["RAudio",     "raylib",     "src/RAudio.cpp",          "raylibModule",    0]
sectionConfig["Global Intrinsics"] = ["global",     "global",     "src/MoreIntrinsics.cpp",  "",                1]
sectionConfig["file module"]       = ["file",       "file",       "src/FileModule.cpp",      "fileModule",      0]
sectionConfig["FileHandle"]        = ["FileHandle", "FileHandle", "src/FileModule.cpp",      "fileHandleClass", 0]
sectionConfig["http module"]       = ["http",       "http",       "src/HttpModule.cpp",      "httpMap",         0]
sectionConfig["RawData"]           = ["RawData",    "RawData",    "src/RawData.cpp",         "rawDataClass",    0]

// Scan all source files.
print "Scanning C++ source files for return types..."
for heading in sectionConfig.indexes
	cfg = sectionConfig[heading]
	cppFile = cfg[2]
	moduleVar = cfg[3]
	isGlobal = cfg[4]
	if isGlobal then
		scanGlobalFile(cppFile)
	else
		scanBindingFile(cppFile, moduleVar)
	end if
end for
print "  Found return types for " + apiReturnType.len + " APIs."

// ---------------------------------------------------------------------------
// Parameter parsing from API_DOC.md table format
// e.g.  "**v1**=[0, 0], **v2**"  ->  [["v1","[0, 0]"], ["v2",""]]
// ---------------------------------------------------------------------------
parseParams = function(paramStr)
	result = []
	if paramStr.trim == "" then return result
	parts = paramStr.split("**")
	// parts[0] is text before first **, then alternating name / after
	i = 1
	while i < parts.len - 1
		name = parts[i]
		after = parts[i + 1]
		defaultVal = ""
		if after.startsWith("=") then
			defaultVal = after[1:]
			// Strip trailing separator (", " that leads into next param)
			if defaultVal.endsWith(", ") then defaultVal = defaultVal[:-2]
			if defaultVal.endsWith(",") then defaultVal = defaultVal[:-1]
		end if
		result.push [name, defaultVal.trim]
		i += 2
	end while
	return result
end function

// ---------------------------------------------------------------------------
// Overview file generation (one per folder)
// Writes {folder}/{folder}.md with the full table, names linked to their pages.
// ---------------------------------------------------------------------------
writeOverview = function(folder, heading, rows)
	if rows.len == 0 then return
	lines = []
	lines.push "## " + heading
	lines.push ""
	lines.push "|Name | Parameters | Purpose |"
	lines.push "|-----|------------|---------|"
	for row in rows
		lines.push row
	end for
	lines.push ""
	overviewPath = file.child(file.child(wikiDir, folder), folder + ".md")
	file.writeLines(overviewPath, lines)
end function

// ---------------------------------------------------------------------------
// Wiki page generation
// ---------------------------------------------------------------------------
generatePage = function(apiName, params, purpose, retType)
	lines = []

	// Opening description (no heading)
	if purpose == "" then purpose = apiName
	lines.push purpose
	lines.push ""

	// Parameters
	lines.push "## Parameters"
	lines.push ""
	if params.len == 0 then
		lines.push "None."
	else
		lines.push "|Parameter | Default Value | Note |"
		lines.push "|-----|------------|---------|"
		for p in params
			lines.push "|" + p[0] + " | " + p[1] + " | |"
		end for
	end if
	lines.push ""

	// Return value
	lines.push "## Return value"
	lines.push ""
	if retType == "null" then
		lines.push "None."
	else
		lines.push "|Condition |Return Value|"
		lines.push "|------|------|"
		lines.push "|(always) |" + retType + "|"
	end if
	lines.push ""

	// Notes (hand-written later)
	lines.push "## Notes"
	lines.push ""

	return lines
end function

// ---------------------------------------------------------------------------
// Main: read API_DOC.md, create folders, write pages.
// ---------------------------------------------------------------------------

// Verify wiki dir exists
if not file.info(wikiDir) then
	print "Wiki directory not found: " + wikiDir
	exit
end if

docLines = file.readLines("API_DOC.md")
if not docLines then
	print "Could not read API_DOC.md"
	exit
end if

currentHeading = ""
currentFolder = ""
currentPrefix = ""
overviewRows = []    // rows accumulated for the current section's overview file
pageCount = 0
overviewCount = 0

for line in docLines
	// Section heading?
	if line.startsWith("## ") then
		// Flush the previous section's overview before switching.
		if currentHeading != "" then
			writeOverview(currentFolder, currentHeading, overviewRows)
			overviewCount += 1
		end if
		overviewRows = []
		currentHeading = line[3:].trim
		if sectionConfig.hasIndex(currentHeading) then
			cfg = sectionConfig[currentHeading]
			currentFolder = cfg[0]
			currentPrefix = cfg[1]
			// Ensure the wiki sub-folder exists
			folderPath = file.child(wikiDir, currentFolder)
			if not file.info(folderPath) then
				file.makedir(folderPath)
				print "  Created folder: " + folderPath
			end if
		else
			currentHeading = ""
		end if
		continue
	end if

	if currentHeading == "" then continue
	if not line.startsWith("|") then continue
	// Skip table header and separator rows
	if line.startsWith("|Name") or line.startsWith("|-----") or line.startsWith("|---") then continue

	parts = line.split("|")
	if parts.len < 4 then continue

	apiName = parts[1].trim
	paramStr = parts[2].trim
	purpose  = parts[3].trim

	if apiName == "" then continue

	// Determine return type
	retType = "null"
	if apiReturnType.hasIndex(apiName) then retType = apiReturnType[apiName]

	// Parse parameters
	params = parseParams(paramStr)

	// Generate and write individual page
	pageContent = generatePage(apiName, params, purpose, retType)
	pageName = currentPrefix + "." + apiName + ".md"
	pagePath = file.child(file.child(wikiDir, currentFolder), pageName)
	file.writeLines(pagePath, pageContent)
	pageCount += 1

	// Accumulate a linked row for this section's overview file.
	// Name cell becomes a wiki link: [Name](prefix.Name)
	linkName = "[" + apiName + "](" + currentPrefix + "." + apiName + ")"
	overviewRows.push "|" + linkName + " |" + paramStr + " |" + purpose + " |"
end for

// Flush the final section's overview.
if currentHeading != "" then
	writeOverview(currentFolder, currentHeading, overviewRows)
	overviewCount += 1
end if

print ""
print "Generated " + pageCount + " wiki pages and " + overviewCount + " overview files."
