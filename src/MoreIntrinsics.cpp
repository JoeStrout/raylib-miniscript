//
//  MoreIntrinsics.cpp
//  raylib-miniscript
//
//  Additional intrinsics (import, exit, env) for the MiniScript environment.
//

#include "MoreIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "miniscript.h"
#include <map>
#include <cstring>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#else
#include <cstdlib>
extern "C" {
	// list of environment variables provided by C standard library:
	extern char **environ;
}
#endif

// Separator between entries in MS_IMPORT_PATH.  We write ':' on all platforms,
// matching MiniScript 1.x and command-line MiniScript 2; ';' is also accepted
// when parsing (see FindPathSep).
#define PATH_SEP_STR ":"

using namespace MiniScript;

//--------------------------------------------------------------------------------
// Environment variable support
//--------------------------------------------------------------------------------

// Get a reference to the shared environment map.
// On desktop, initialized from the OS environment on first call.
// On web, starts empty (populated via setEnvVar).
static ValueDict& envMapRef() {
	static ValueDict envMap;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
#ifndef PLATFORM_WEB
		// Read all current environment variables from the OS
		for (char **current = environ; *current; current++) {
			char* eqPos = strchr(*current, '=');
			if (!eqPos) continue;
			String varName(*current, eqPos - *current);
			String valueStr(eqPos+1);
			envMap.SetValue(varName, valueStr);
		}
#endif
	}
	return envMap;
}

static void setEnvVar(const char* key, const char* value) {
#ifndef PLATFORM_WEB
	#if defined(_WIN32)
		_putenv_s(key, value);
	#else
		setenv(key, value, 1);
	#endif
#endif
	// Always update the in-memory map (essential for web; keeps desktop in sync)
	envMapRef().SetValue(String(key), String(value));
}

static ValueDict getEnvMap() {
	return envMapRef();
}

// Expand any occurrences of $VAR, $(VAR) or ${VAR} on all platforms,
// and also of %VAR% under Windows only, using variables from getEnvMap().
static String ExpandVariables(String path) {
	int p0, p1;
	int len = path.LengthB();
	ValueDict envMap = getEnvMap();
	while (true) {
		p0 = path.IndexOfB("${");
		if (p0 >= 0) {
			for (p1=p0+1; p1<len && path[p1] != '}'; p1++) {}
			if (p1 < len) {
				String varName = path.SubstringB(p0 + 2, p1 - p0 - 2);
				path = path.Substring(0, p0) + envMap.Lookup(varName, Value::emptyString).ToString() + path.SubstringB(p1 + 1);
				len = path.LengthB();
				continue;
			}
		}
		p0 = path.IndexOfB("$(");
		if (p0 >= 0) {
			for (p1=p0+1; p1<len && path[p1] != ')'; p1++) {}
			if (p1 < len) {
				String varName = path.SubstringB(p0 + 2, p1 - p0 - 2);
				path = path.Substring(0, p0) + envMap.Lookup(varName, Value::emptyString).ToString() + path.SubstringB(p1 + 1);
				len = path.LengthB();
				continue;
			}
		}
#if defined(_WIN32)
		p0 = path.IndexOfB("%");
		if (p0 >= 0) {
			for (p1=p0+1; p1<len && path[p1] != '%'; p1++) {}
			if (p1 < len) {
				String varName = path.SubstringB(p0 + 1, p1 - p0 - 1);
				path = path.Substring(0, p0) + envMap.Lookup(varName, Value::emptyString).ToString() + path.SubstringB(p1 + 1);
				len = path.LengthB();
				continue;
			}
		}
#endif
		p0 = path.IndexOfB("$");
		if (p0 >= 0) {
			// variable continues until non-alphanumeric char
			p1 = p0+1;
			while (p1 < len) {
				char c = path[p1];
				if (c < '0' || (c > '9' && c < 'A') || (c > 'Z' && c < '_') || c == '`' || c > 'z') break;
				p1++;
			}
			String varName = path.SubstringB(p0 + 1, p1 - p0 - 1);
			path = path.Substring(0, p0) + envMap.Lookup(varName, Value::emptyString).ToString() + path.SubstringB(p1);
			len = path.LengthB();
			continue;
		}
		break;
	}
	return path;
}

static IntrinsicResult intrinsic_env(Context context, IntrinsicResult partialResult) {
	// The env map has stable identity (scripts may hold onto it), so root it
	// once via StaticMap.  Note: unlike MS1, assigning to a member of this map
	// from script does NOT propagate to the OS environment (MS2 dropped the
	// map assign-override hook); host code changes the OS env via setEnvVar().
	return IntrinsicResult(StaticMap(envMapRef()));
}

static bool IsAlpha(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// Find the separator ending the MS_IMPORT_PATH entry that starts at `entry`,
// or null if that entry runs to the end of the string.  Both ';' and ':'
// separate entries on all platforms, except that a ':' forming a Windows drive
// letter (the second character of an entry, as in "C:\lib") is part of the
// path rather than a separator.
static const char* FindPathSep(const char* entry) {
	for (const char* p = entry; *p; p++) {
		if (*p == ';') return p;
		if (*p != ':') continue;
		bool driveLetter = (p == entry + 1) && IsAlpha(entry[0])
			&& (p[1] == '/' || p[1] == '\\');
		if (!driveLetter) return p;
	}
	return nullptr;
}

// Import directories, frozen at the moment the sandbox latches.
//
// `env` is a writable map, so without this a sandboxed program could set
// env.MS_IMPORT_PATH = "/etc" and then `import "passwd"` to read any .ms file
// on the disk.  Narrow, but an escape.  Freezing also covers MS_EXE_DIR and
// MS_SCRIPT_DIR, which the path is written in terms of.
//
// The directories themselves stay real host paths: they are chosen by the host
// during boot, not by script, and a library name may not contain a separator,
// so nothing reachable through them lies outside them.
static std::vector<String>& frozenImportDirs() {
	static std::vector<String> dirs;
	return dirs;
}
static bool importPathFrozen = false;

static String GetImportDirFromEnv(int index) {
	String importPath = ExpandVariables(String("$MS_IMPORT_PATH"));
	const char* start = importPath.c_str();
	int current = 0;
	while (true) {
		const char* sep = FindPathSep(start);
		long dirLen = sep ? (long)(sep - start) : (long)strlen(start);
		if (dirLen > 0) {
			if (current == index) return String(start, dirLen);
			current++;
		}
		if (!sep) break;
		start = sep + 1;
	}
	return String();
}

// Get the import search directory at the given index.  Returns empty string if
// the index is out of range.
static String GetImportDir(int index) {
	if (importPathFrozen) {
		std::vector<String>& dirs = frozenImportDirs();
		if (index < 0 || index >= (int)dirs.size()) return String();
		return dirs[index];
	}
	return GetImportDirFromEnv(index);
}

void FreezeImportPath() {
	if (importPathFrozen) return;
	std::vector<String>& dirs = frozenImportDirs();
	for (int i = 0; ; i++) {
		String dir = GetImportDirFromEnv(i);
		if (dir.empty()) break;
		dirs.push_back(dir);
	}
	importPathFrozen = true;
}

//--------------------------------------------------------------------------------
// Import intrinsic
//--------------------------------------------------------------------------------

// Wrap a compiler error from an imported module so it names the module (the
// parser's own message carries only a line number), keeping the original as the
// inner error and attaching the stack trace of the import call site.  Returned
// as import's result value: discarded by a bare `import "foo"` statement, ERRCHK
// then halts the program, while `x = import("foo")` lets the caller inspect it.
static Value ImportCompileError(String libname, Value compileErr) {
	String msg = String("in ") + libname + ".ms: " + compileErr.Message().ToString();
	return ErrorTypes::CompilerError(msg, compileErr);
}

#ifdef PLATFORM_WEB

// Track import fetches
struct ImportFetchData {
	emscripten_fetch_t* fetch;
	bool completed;
	int status;
	String libname;
	int searchPathIndex;
	ImportFetchData() : fetch(nullptr), completed(false), status(0), searchPathIndex(0) {}
};

static std::map<long, ImportFetchData> activeImportFetches;
static long nextImportFetchId = 1;

static void import_fetch_completed(emscripten_fetch_t *fetch) {
	for (auto& pair : activeImportFetches) {
		if (pair.second.fetch == fetch) {
			pair.second.completed = true;
			pair.second.status = fetch->status;
			printf("import_fetch_completed: Fetch ID %ld completed with status %d\n", pair.first, fetch->status);
			break;
		}
	}
}

static IntrinsicResult intrinsic_import(Context context, IntrinsicResult partialResult) {
	// State 3: the imported module finished running; store its locals map
	// (the manual-call result) under the library name in the caller's scope.
	if (!partialResult.Done() && partialResult.result.Type() == ValueType::String) {
		Value importedValues = context.vm.ManualCallResult;
		String libname = partialResult.result.ToString();
		context.vm.SetVar(libname, importedValues);
		return IntrinsicResult::Null;
	}

	// State 2: File has been fetched, parse and create import
	if (!partialResult.Done() && partialResult.result.Type() == ValueType::Number) {
		long fetchId = (long)partialResult.result.DoubleValue();
		auto it = activeImportFetches.find(fetchId);
		if (it == activeImportFetches.end()) {
			context.vm.RaiseRuntimeError("import: internal error (fetch not found)");
			return IntrinsicResult::Null;
		}

		ImportFetchData& data = it->second;

		if (!data.completed) {
			return partialResult;
		}

		emscripten_fetch_t* fetch = data.fetch;
		String libname = data.libname;

		if (data.status == 200) {
			char* moduleData = (char*)malloc(fetch->numBytes + 1);
			if (!moduleData) {
				emscripten_fetch_close(fetch);
				activeImportFetches.erase(it);
				context.vm.RaiseRuntimeError("import: memory allocation failed");
				return IntrinsicResult::Null;
			}
			memcpy(moduleData, fetch->data, fetch->numBytes);
			moduleData[fetch->numBytes] = '\0';
			String moduleSource(moduleData);
			free(moduleData);

			emscripten_fetch_close(fetch);
			activeImportFetches.erase(it);

			Value compileErr;
			FuncDef moduleMain = Interpreter::CompileToFunc(moduleSource, libname + ".ms", &compileErr);
			if (IsNull(moduleMain)) {
				if (!compileErr.IsNull()) return IntrinsicResult(ImportCompileError(libname, compileErr));
				return IntrinsicResult::Null;
			}
			context.vm.ManuallyPushCall(context.baseIndex, moduleMain);

			return IntrinsicResult(libname, false);
		} else {
			emscripten_fetch_close(fetch);
			int nextPathIndex = data.searchPathIndex + 1;
			activeImportFetches.erase(it);

			String dir = GetImportDir(nextPathIndex);
			if (!dir.empty()) {
				String path = dir + "/" + libname + ".ms";

				long newFetchId = nextImportFetchId++;
				ImportFetchData& newData = activeImportFetches[newFetchId];
				newData.libname = libname;
				newData.searchPathIndex = nextPathIndex;

				emscripten_fetch_attr_t attr;
				emscripten_fetch_attr_init(&attr);
				strcpy(attr.requestMethod, "GET");
				attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
				attr.onsuccess = import_fetch_completed;
				attr.onerror = import_fetch_completed;

				newData.fetch = emscripten_fetch(&attr, path.c_str());

				return IntrinsicResult(Value((double)newFetchId), false);
			} else {
				context.vm.RaiseRuntimeError("import: library not found: " + libname);
				return IntrinsicResult::Null;
			}
		}
	}

	// State 1: Start the import - fetch the file
	String libname = context.GetVar("libname").ToString();
	if (libname.empty()) {
		context.vm.RaiseRuntimeError("import: libname required");
		return IntrinsicResult::Null;
	}
	// A library name is a bare name, never a path.  Backslash and ".." matter as
	// much as "/": with the search directories frozen, this check is the only
	// thing keeping an import inside them.
	if (libname.IndexOfB('/') >= 0 || libname.IndexOfB('\\') >= 0
	    || libname.IndexOfB(String("..")) >= 0) {
		context.vm.RaiseRuntimeError("import: argument must be library name, not path");
		return IntrinsicResult::Null;
	}

	String dir = GetImportDir(0);
	if (dir.empty()) {
		context.vm.RaiseRuntimeError("import: no import paths configured");
		return IntrinsicResult::Null;
	}
	String path = dir + "/" + libname + ".ms";

	long fetchId = nextImportFetchId++;
	ImportFetchData& data = activeImportFetches[fetchId];
	data.libname = libname;
	data.searchPathIndex = 0;

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "GET");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess = import_fetch_completed;
	attr.onerror = import_fetch_completed;

	data.fetch = emscripten_fetch(&attr, path.c_str());

	return IntrinsicResult(Value((double)fetchId), false);
}

#else // PLATFORM_DESKTOP

static IntrinsicResult intrinsic_import(Context context, IntrinsicResult partialResult) {
	// State 2: the imported module finished running; store its locals map
	// (the manual-call result) under the library name in the caller's scope.
	if (!partialResult.Done() && partialResult.result.Type() == ValueType::String) {
		Value importedValues = context.vm.ManualCallResult;
		String libname = partialResult.result.ToString();
		context.vm.SetVar(libname, importedValues);
		return IntrinsicResult::Null;
	}

	// State 1: Load and parse the file synchronously
	String libname = context.GetVar("libname").ToString();
	if (libname.empty()) {
		context.vm.RaiseRuntimeError("import: libname required");
		return IntrinsicResult::Null;
	}
	// A library name is a bare name, never a path.  Backslash and ".." matter as
	// much as "/": with the search directories frozen, this check is the only
	// thing keeping an import inside them.
	if (libname.IndexOfB('/') >= 0 || libname.IndexOfB('\\') >= 0
	    || libname.IndexOfB(String("..")) >= 0) {
		context.vm.RaiseRuntimeError("import: argument must be library name, not path");
		return IntrinsicResult::Null;
	}

	// Search each directory in MS_IMPORT_PATH
	String moduleSource;
	bool found = false;
	for (int i = 0; ; i++) {
		String dir = GetImportDir(i);
		if (dir.empty()) break;
		String path = dir + "/" + libname + ".ms";
		char* text = LoadFileText(path.c_str());
		if (text != nullptr) {
			moduleSource = String(text);
			UnloadFileText(text);
			found = true;
			break;
		}
	}

	if (!found) {
		context.vm.RaiseRuntimeError("import: library not found: " + libname);
		return IntrinsicResult::Null;
	}

	// Compile the module and push it as a manual call; we'll be re-invoked
	// (State 2) when it finishes.  moduleMain is the module's @main; nested
	// functions are reachable from its constant pool.
	Value compileErr;
	FuncDef moduleMain = Interpreter::CompileToFunc(moduleSource, libname + ".ms", &compileErr);
	if (IsNull(moduleMain)) {
		if (!compileErr.IsNull()) return IntrinsicResult(ImportCompileError(libname, compileErr));
		return IntrinsicResult::Null;
	}
	context.vm.ManuallyPushCall(context.baseIndex, moduleMain);

	return IntrinsicResult(libname, false);
}

#endif

//--------------------------------------------------------------------------------
// Exit intrinsic
//--------------------------------------------------------------------------------

static IntrinsicResult intrinsic_exit(Context context, IntrinsicResult partialResult) {
	// The request is recorded on the VM that made it, so `exit` needs to know
	// nothing about who is running: our main loop polls the root interpreter
	// and shuts the process down, while a child interpreter's `exit` simply
	// ends that child (see Interp.exitRequested in InterpModule.cpp).
	Value resultCode = context.GetVar("resultCode");
	int code = resultCode.IsNull() ? 0 : (int)resultCode.IntValue();
	context.vm.RequestExit(code);
	return IntrinsicResult::Null;
}

//--------------------------------------------------------------------------------
// Script loading helpers (shared by main.cpp and the run intrinsic)
//--------------------------------------------------------------------------------

void UpdateScriptDir(const char* path) {
	const char* lastSlash = strrchr(path, '/');
#ifdef _WIN32
	const char* lastBackslash = strrchr(path, '\\');
	if (lastBackslash && (!lastSlash || lastBackslash > lastSlash)) lastSlash = lastBackslash;
#endif
	if (lastSlash) {
		String scriptDir(path, (long)(lastSlash - path));
		setEnvVar("MS_SCRIPT_DIR", scriptDir.c_str());
	} else {
		setEnvVar("MS_SCRIPT_DIR", ".");
	}
}

void RunScriptSource(Interpreter interpreter, String source) {
	// Recompile, carrying the old program's globals over into the new one (as MS1
	// did by snapshotting the globals ValueDict).  This also stops the outgoing
	// VM, which matters because we're called from the `run` intrinsic, i.e. from
	// inside that VM's own Run loop.  The cached type maps MS1 also had to save
	// and restore now live in CoreIntrinsics and survive a reset on their own, so
	// they need no handling here.
	interpreter.ResetPreservingGlobals(source);
}

//--------------------------------------------------------------------------------
// Run intrinsic
//--------------------------------------------------------------------------------

#ifdef PLATFORM_WEB

struct RunFetchData {
	emscripten_fetch_t* fetch;
	bool completed;
	int status;
	String path;
	RunFetchData() : fetch(nullptr), completed(false), status(0) {}
};

static std::map<long, RunFetchData> activeRunFetches;
static long nextRunFetchId = 1;

static void run_fetch_completed(emscripten_fetch_t *fetch) {
	for (auto& pair : activeRunFetches) {
		if (pair.second.fetch == fetch) {
			pair.second.completed = true;
			pair.second.status = fetch->status;
			break;
		}
	}
}

static IntrinsicResult intrinsic_run(Context context, IntrinsicResult partialResult) {
	// State 2: File has been fetched, run it
	if (!partialResult.Done() && partialResult.result.Type() == ValueType::Number) {
		long fetchId = (long)partialResult.result.DoubleValue();
		auto it = activeRunFetches.find(fetchId);
		if (it == activeRunFetches.end()) {
			context.vm.RaiseRuntimeError("run: internal error (fetch not found)");
			return IntrinsicResult::Null;
		}

		RunFetchData& data = it->second;

		if (!data.completed) {
			return partialResult;
		}

		emscripten_fetch_t* fetch = data.fetch;
		String path = data.path;

		if (data.status == 200) {
			char* fileData = (char*)malloc(fetch->numBytes + 1);
			if (!fileData) {
				emscripten_fetch_close(fetch);
				activeRunFetches.erase(it);
				context.vm.RaiseRuntimeError("run: memory allocation failed");
				return IntrinsicResult::Null;
			}
			memcpy(fileData, fetch->data, fetch->numBytes);
			fileData[fetch->numBytes] = '\0';
			String source(fileData);
			free(fileData);

			emscripten_fetch_close(fetch);
			activeRunFetches.erase(it);

			UpdateScriptDir(path.c_str());
			RunScriptSource(context.vm.GetInterpreter(), source);
			return IntrinsicResult::Null;
		} else {
			emscripten_fetch_close(fetch);
			activeRunFetches.erase(it);
			context.vm.RaiseRuntimeError("run: failed to load file: " + path);
			return IntrinsicResult::Null;
		}
	}

	// State 1: Start the fetch
	String path = context.GetVar("path").ToString();
	if (path.empty()) {
		context.vm.RaiseRuntimeError("run: path required");
		return IntrinsicResult::Null;
	}
	if (!path.EndsWith(".ms")) path += ".ms";

	long fetchId = nextRunFetchId++;
	RunFetchData& data = activeRunFetches[fetchId];
	data.path = path;

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "GET");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess = run_fetch_completed;
	attr.onerror = run_fetch_completed;

	data.fetch = emscripten_fetch(&attr, path.c_str());

	return IntrinsicResult(Value((double)fetchId), false);
}

#else // PLATFORM_DESKTOP

static IntrinsicResult intrinsic_run(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	if (path.empty()) {
		context.vm.RaiseRuntimeError("run: path required");
		return IntrinsicResult::Null;
	}

	char* text = LoadFileText(path.c_str());
	if (text == nullptr) {
		context.vm.RaiseRuntimeError("run: failed to load file: " + path);
		return IntrinsicResult::Null;
	}
	String source(text);
	UnloadFileText(text);

	UpdateScriptDir(path.c_str());
	RunScriptSource(context.vm.GetInterpreter(), source);
	return IntrinsicResult::Null;
}

#endif

//--------------------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------------------

void AddMoreIntrinsics() {
	// Import a MiniScript library by name, searching MS_IMPORT_PATH
	Intrinsic importFunc = Intrinsic::Create("import");
	importFunc.AddParam("libname", "");
	importFunc.set_Code(&intrinsic_import);

	// Exit the program with the given result code
	Intrinsic exitFunc = Intrinsic::Create("exit");
	exitFunc.AddParam("resultCode");
	exitFunc.set_Code(&intrinsic_exit);

	// Get a map of all environment variables
	Intrinsic envFunc = Intrinsic::Create("env");
	envFunc.set_Code(&intrinsic_env);

	// Load and run a MiniScript file in the current interpreter context
	Intrinsic runFunc = Intrinsic::Create("run");
	runFunc.AddParam("path", "");
	runFunc.set_Code(&intrinsic_run);

#ifdef PLATFORM_WEB
	// On web, set default path variables (on desktop, these are set in main.cpp)
	setEnvVar("MS_EXE_DIR", ".");
	setEnvVar("MS_SCRIPT_DIR", "assets");
#endif

	// Get a map of currently loaded resource counts by type (Image, Texture, Font, etc.)
	Intrinsic rcFunc = Intrinsic::Create("resourceCounts");
	rcFunc.set_Code([](Context context, IntrinsicResult partialResult) -> IntrinsicResult {
		ValueDict map;
		int total = 0;
		auto add = [&](const char* name, int count) {
			map.SetValue(String(name), Value(count));
			total += count;
		};
		add("Image", rcImage);
		add("Texture", rcTexture);
		add("Font", rcFont);
		add("Wave", rcWave);
		add("Music", rcMusic);
		add("Sound", rcSound);
		add("AudioStream", rcAudioStream);
		add("RenderTexture", rcRenderTexture);
		add("Shader", rcShader);
		add("Mesh", rcMesh);
		add("Material", rcMaterial);
		add("Model", rcModel);
		add("ModelAnimation", rcModelAnimation);
		map.SetValue(String("total"), Value(total));
		return IntrinsicResult(DynamicMap(map));
	});

	// Set the default import search path (variables are expanded at import time),
	// unless the user already set MS_IMPORT_PATH in the environment.
	if (envMapRef().Lookup(String("MS_IMPORT_PATH"), Value::emptyString).ToString().empty()) {
		setEnvVar("MS_IMPORT_PATH",
			"$MS_SCRIPT_DIR" PATH_SEP_STR
			"$MS_SCRIPT_DIR/lib" PATH_SEP_STR
			"$MS_EXE_DIR/assets/lib");
	}
}

