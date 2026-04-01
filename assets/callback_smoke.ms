// callback_smoke.ms
// Smoke checks for native-to-MiniScript callback bridges in RCore:
// - SetLoadFileDataCallback
// - SetSaveFileDataCallback
// - SetLoadFileTextCallback
// - SetSaveFileTextCallback
// - SetTraceLogCallback

if not raylib.IsWindowReady then raylib.InitWindow 

rl = raylib

fail = function(msg)
	print "FAIL: " + msg
	// Force a runtime error so smoke runs fail fast.
	x = null[0]
end function

assert = function(ok, msg)
	if not ok then fail(msg)
end function

state = {}
state.traceCount = 0
state.lastTraceLevel = -1
state.lastTraceText = ""
state.loadDataCount = 0
state.saveDataCount = 0
state.loadTextCount = 0
state.saveTextCount = 0
state.savedDataLen = -1
state.savedText = ""

onTrace = function(logLevel, text)
	state.traceCount += 1
	state.lastTraceLevel = logLevel
	state.lastTraceText = text
end function

onLoadData = function(fileName)
	state.loadDataCount += 1
	if fileName == "cb_data.bin" then return [10, 20, 30, 40]
	return null
end function

onSaveData = function(fileName, data, dataSize)
	state.saveDataCount += 1
	if data == null then return false

	state.savedDataLen = data.len

	if fileName != "cb_out.bin" then return false
	if dataSize != data.len then return false
	return true
end function

onLoadText = function(fileName)
	state.loadTextCount += 1
	if fileName == "cb_text.txt" then return "hello callbacks"
	return null
end function

onSaveText = function(fileName, text)
	state.saveTextCount += 1
	state.savedText = text
	return fileName == "cb_out.txt"
end function

run = function
	print "callback smoke: begin"

	rl.SetLoadFileDataCallback(@onLoadData)
	rl.SetSaveFileDataCallback(@onSaveData)
	rl.SetLoadFileTextCallback(@onLoadText)
	rl.SetSaveFileTextCallback(@onSaveText)
	rl.SetTraceLogCallback(@onTrace)

	data = rl.LoadFileData("cb_data.bin")
	assert data != null, "LoadFileData callback returned null"
	assert data.len == 4, "LoadFileData length"
	assert rl.LoadFileData("missing.bin") == null, "LoadFileData missing path should be null"

	text = rl.LoadFileText("cb_text.txt")
	assert text == "hello callbacks", "LoadFileText callback"
	assert rl.LoadFileText("missing.txt") == null, "LoadFileText missing path should be null"

	savedOk = rl.SaveFileData("cb_out.bin", [9, 8, 7], 3)
	assert savedOk, "SaveFileData callback result"
	assert state.savedDataLen == 3, "SaveFileData len forwarded"

	savedTextOk = rl.SaveFileText("cb_out.txt", "saved from callback")
	assert savedTextOk, "SaveFileText callback result"
	assert state.savedText == "saved from callback", "SaveFileText text forwarded"

	rl.TraceLog(rl.LOG_INFO, "callback smoke trace")
	assert state.traceCount > 0, "Trace callback count"
	assert state.lastTraceLevel == rl.LOG_INFO, "Trace callback level"
	assert state.lastTraceText == "callback smoke trace", "Trace callback text"

	assert state.loadDataCount >= 2, "Load data callback call count"
	assert state.loadTextCount >= 2, "Load text callback call count"
	assert state.saveDataCount == 1, "Save data callback call count"
	assert state.saveTextCount == 1, "Save text callback call count"

	// Unregister all callback bridges.
	rl.SetLoadFileDataCallback(null)
	rl.SetSaveFileDataCallback(null)
	rl.SetLoadFileTextCallback(null)
	rl.SetSaveFileTextCallback(null)
	rl.SetTraceLogCallback(null)

	print "callback smoke: PASS"
end function

if locals == globals then run
