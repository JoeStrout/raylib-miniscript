//
//  MoreIntrinsics.cpp
//  raylib-miniscript
//
//  Additional intrinsics (import, exit, env) for the MiniScript environment.
//

#include "MoreIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptIntrinsics.h"
#include "MiniscriptParser.h"
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

#if defined(_WIN32)
#define PATH_SEP ';'
#define PATH_SEP_STR ";"
#else
#define PATH_SEP ':'
#define PATH_SEP_STR ":"
#endif

using namespace MiniScript;

static bool exitASAP = false;
static int exitResult = 0;

//--------------------------------------------------------------------------------
// Environment variable support
//--------------------------------------------------------------------------------

static bool assignEnvVar(ValueDict& dict, Value key, Value value);  // forward declaration

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
		envMap.SetAssignOverride(assignEnvVar);
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

static bool assignEnvVar(ValueDict& dict, Value key, Value value) {
	setEnvVar(key.ToString().c_str(), value.ToString().c_str());
	return true;	// setEnvVar already updated the map
}

static ValueDict getEnvMap() {
	return envMapRef();
}

// Expand any occurrences of $VAR, $(VAR) or ${VAR} on all platforms,
// and also of %VAR% under Windows only, using variables from getEnvMap().
static String ExpandVariables(String path) {
	long p0, p1;
	long len = path.LengthB();
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

static IntrinsicResult intrinsic_env(Context *context, IntrinsicResult partialResult) {
	return IntrinsicResult(getEnvMap());
}

// Get the import search directory at the given index from MS_IMPORT_PATH.
// Returns empty string if index is out of range.
static String GetImportDir(int index) {
	String importPath = ExpandVariables(String("$MS_IMPORT_PATH"));
	const char* start = importPath.c_str();
	int current = 0;
	while (true) {
		const char* sep = strchr(start, PATH_SEP);
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

//--------------------------------------------------------------------------------
// Import intrinsic
//--------------------------------------------------------------------------------

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

static IntrinsicResult intrinsic_import(Context *context, IntrinsicResult partialResult) {
	// State 3: Import function has finished, store result in parent context
	if (!partialResult.Done() && partialResult.Result().type == ValueType::String) {
		Value importedValues = context->GetTemp(0);
		String libname = partialResult.Result().ToString();
		Context *callerContext = context->parent;
		if (callerContext) {
			callerContext->SetVar(libname, importedValues);
		}
		return IntrinsicResult::Null;
	}

	// State 2: File has been fetched, parse and create import
	if (!partialResult.Done() && partialResult.Result().type == ValueType::Number) {
		long fetchId = (long)partialResult.Result().DoubleValue();
		auto it = activeImportFetches.find(fetchId);
		if (it == activeImportFetches.end()) {
			RuntimeException("import: internal error (fetch not found)").raise();
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
				RuntimeException("import: memory allocation failed").raise();
			}
			memcpy(moduleData, fetch->data, fetch->numBytes);
			moduleData[fetch->numBytes] = '\0';
			String moduleSource(moduleData);
			free(moduleData);

			emscripten_fetch_close(fetch);
			activeImportFetches.erase(it);

			Parser parser;
			parser.errorContext = libname + ".ms";
			parser.Parse(moduleSource);
			FunctionStorage *import = parser.CreateImport();
			context->vm->ManuallyPushCall(import, Value::Temp(0));

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
				RuntimeException("import: library not found: " + libname).raise();
			}
		}
	}

	// State 1: Start the import - fetch the file
	String libname = context->GetVar("libname").ToString();
	if (libname.empty()) {
		RuntimeException("import: libname required").raise();
	}
	if (libname.IndexOfB('/') >= 0) {
		RuntimeException("import: argument must be library name, not path").raise();
	}

	String dir = GetImportDir(0);
	if (dir.empty()) {
		RuntimeException("import: no import paths configured").raise();
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

static IntrinsicResult intrinsic_import(Context *context, IntrinsicResult partialResult) {
	// State 2: Import function has finished, store result in parent context
	if (!partialResult.Done() && partialResult.Result().type == ValueType::String) {
		Value importedValues = context->GetTemp(0);
		String libname = partialResult.Result().ToString();
		Context *callerContext = context->parent;
		if (callerContext) {
			callerContext->SetVar(libname, importedValues);
		}
		return IntrinsicResult::Null;
	}

	// State 1: Load and parse the file synchronously
	String libname = context->GetVar("libname").ToString();
	if (libname.empty()) {
		RuntimeException("import: libname required").raise();
	}
	if (libname.IndexOfB('/') >= 0) {
		RuntimeException("import: argument must be library name, not path").raise();
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
		RuntimeException("import: library not found: " + libname).raise();
	}

	Parser parser;
	parser.errorContext = libname + ".ms";
	parser.Parse(moduleSource);
	FunctionStorage *import = parser.CreateImport();
	context->vm->ManuallyPushCall(import, Value::Temp(0));

	return IntrinsicResult(libname, false);
}

#endif

//--------------------------------------------------------------------------------
// Exit intrinsic
//--------------------------------------------------------------------------------

static IntrinsicResult intrinsic_exit(Context *context, IntrinsicResult partialResult) {
	exitASAP = true;
	Value resultCode = context->GetVar("resultCode");
	if (!resultCode.IsNull()) exitResult = (int)resultCode.IntValue();
	context->vm->Stop();
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

void RunScriptSource(Interpreter* interpreter, String source) {
	// Save the current global variables
	ValueDict savedGlobals = interpreter->vm->GetGlobalContext()->variables;

	// Save the cached type maps (which may have user-added methods)
	Value savedMapType = interpreter->vm->mapType;
	Value savedListType = interpreter->vm->listType;
	Value savedStringType = interpreter->vm->stringType;
	Value savedNumberType = interpreter->vm->numberType;
	Value savedFunctionType = interpreter->vm->functionType;

	// Reset and recompile with the new source
	interpreter->Reset(source);
	interpreter->Compile();

	// Restore the saved globals and type maps into the new VM
	if (interpreter->vm) {
		interpreter->vm->GetGlobalContext()->variables = savedGlobals;
		interpreter->vm->mapType = savedMapType;
		interpreter->vm->listType = savedListType;
		interpreter->vm->stringType = savedStringType;
		interpreter->vm->numberType = savedNumberType;
		interpreter->vm->functionType = savedFunctionType;
	}
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

static IntrinsicResult intrinsic_run(Context *context, IntrinsicResult partialResult) {
	// State 2: File has been fetched, run it
	if (!partialResult.Done() && partialResult.Result().type == ValueType::Number) {
		long fetchId = (long)partialResult.Result().DoubleValue();
		auto it = activeRunFetches.find(fetchId);
		if (it == activeRunFetches.end()) {
			RuntimeException("run: internal error (fetch not found)").raise();
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
				RuntimeException("run: memory allocation failed").raise();
			}
			memcpy(fileData, fetch->data, fetch->numBytes);
			fileData[fetch->numBytes] = '\0';
			String source(fileData);
			free(fileData);

			emscripten_fetch_close(fetch);
			activeRunFetches.erase(it);

			UpdateScriptDir(path.c_str());
			RunScriptSource(context->vm->interpreter, source);
			return IntrinsicResult::Null;
		} else {
			emscripten_fetch_close(fetch);
			activeRunFetches.erase(it);
			RuntimeException("run: failed to load file: " + path).raise();
		}
	}

	// State 1: Start the fetch
	String path = context->GetVar("path").ToString();
	if (path.empty()) {
		RuntimeException("run: path required").raise();
	}
	if (!path.EndsWith(".ms")) path += ".ms"

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

static IntrinsicResult intrinsic_run(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
	if (path.empty()) {
		RuntimeException("run: path required").raise();
	}

	char* text = LoadFileText(path.c_str());
	if (text == nullptr) {
		RuntimeException("run: failed to load file: " + path).raise();
	}
	String source(text);
	UnloadFileText(text);

	UpdateScriptDir(path.c_str());
	RunScriptSource(context->vm->interpreter, source);
	return IntrinsicResult::Null;
}

#endif

//--------------------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------------------

void AddMoreIntrinsics() {
	Intrinsic *importFunc = Intrinsic::Create("import");
	importFunc->AddParam("libname", "");
	importFunc->code = &intrinsic_import;

	Intrinsic *exitFunc = Intrinsic::Create("exit");
	exitFunc->AddParam("resultCode");
	exitFunc->code = &intrinsic_exit;

	Intrinsic *envFunc = Intrinsic::Create("env");
	envFunc->code = &intrinsic_env;

	Intrinsic *runFunc = Intrinsic::Create("run");
	runFunc->AddParam("path", "");
	runFunc->code = &intrinsic_run;

#ifdef PLATFORM_WEB
	// On web, set default path variables (on desktop, these are set in main.cpp)
	setEnvVar("MS_EXE_DIR", ".");
	setEnvVar("MS_SCRIPT_DIR", "assets");
#endif

	Intrinsic *rcFunc = Intrinsic::Create("resourceCounts");
	rcFunc->code = [](Context *context, IntrinsicResult partialResult) -> IntrinsicResult {
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
		return IntrinsicResult(Value(map));
	};

	// Set the default import search path (variables are expanded at import time)
	setEnvVar("MS_IMPORT_PATH",
		"$MS_SCRIPT_DIR" PATH_SEP_STR
		"$MS_SCRIPT_DIR/lib" PATH_SEP_STR
		"$MS_EXE_DIR/assets/lib");
}

bool ExitRequested() {
	return exitASAP;
}

int ExitResultCode() {
	return exitResult;
}
