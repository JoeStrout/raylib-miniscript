//
//  MoreIntrinsics.cpp
//  raylib-miniscript
//
//  Additional intrinsics (import, exit) for the MiniScript environment.
//

#include "MoreIntrinsics.h"
#include "raylib.h"
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

using namespace MiniScript;

static bool exitASAP = false;
static int exitResult = 0;

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

			const char* searchPaths[] = { "assets/", "assets/lib/" };
			if (nextPathIndex < 2) {
				String path = String(searchPaths[nextPathIndex]) + libname + ".ms";

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

	String path = String("assets/") + libname + ".ms";

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

	// Search paths for desktop
	const char* searchPaths[] = { "assets/", "assets/lib/" };
	String moduleSource;
	bool found = false;

	for (int i = 0; i < 2; i++) {
		String path = String(searchPaths[i]) + libname + ".ms";
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
// Env intrinsic (desktop only)
//--------------------------------------------------------------------------------

#ifndef PLATFORM_WEB

static void setEnvVar(const char* key, const char* value) {
	#if defined(_WIN32)
		_putenv_s(key, value);
	#else
		setenv(key, value, 1);
	#endif
}

static bool assignEnvVar(ValueDict& dict, Value key, Value value) {
	setEnvVar(key.ToString().c_str(), value.ToString().c_str());
	return false;	// allow standard assignment to also apply.
}

static ValueDict getEnvMap() {
	static ValueDict envMap;
	if (envMap.Count() == 0) {
		// The stdlib-supplied `environ` is a null-terminated array of char* (C strings).
		// Each such C string is of the form NAME=VALUE.  So we need to split on the
		// first '=' to separate this into keys and values for our env map.
		for (char **current = environ; *current; current++) {
			char* eqPos = strchr(*current, '=');
			if (!eqPos) continue;	// (should never happen, but just in case)
			String varName(*current, eqPos - *current);
			String valueStr(eqPos+1);
			envMap.SetValue(varName, valueStr);
		}
		envMap.SetAssignOverride(assignEnvVar);
	}
	return envMap;
}

static IntrinsicResult intrinsic_env(Context *context, IntrinsicResult partialResult) {
	return IntrinsicResult(getEnvMap());
}

#endif // !PLATFORM_WEB

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
// Public API
//--------------------------------------------------------------------------------

void AddMoreIntrinsics() {
	Intrinsic *importFunc = Intrinsic::Create("import");
	importFunc->AddParam("libname", "");
	importFunc->code = &intrinsic_import;

	Intrinsic *exitFunc = Intrinsic::Create("exit");
	exitFunc->AddParam("resultCode");
	exitFunc->code = &intrinsic_exit;

#ifndef PLATFORM_WEB
	Intrinsic *envFunc = Intrinsic::Create("env");
	envFunc->code = &intrinsic_env;
#endif
}

bool ExitRequested() {
	return exitASAP;
}

int ExitResultCode() {
	return exitResult;
}
