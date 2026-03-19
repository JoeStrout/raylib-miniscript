// raylib-miniscript - MiniScript + Raylib
// A MiniScript-driven application with Raylib graphics

#include "raylib.h"
#include "SimpleString.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptIntrinsics.h"
#include "MiniscriptParser.h"
#include "RaylibIntrinsics.h"
#include "FileModule.h"
#include "loadfile.h"
#include <stdio.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#endif

using namespace MiniScript;

//--------------------------------------------------------------------------------
// Global state
//--------------------------------------------------------------------------------

enum ScriptState {
	LOADING,
	RUNNING,
	ERRORED,
	COMPLETE
};

static Interpreter* interpreter = nullptr;
static ScriptState scriptState = LOADING;
static String scriptSource;
static String loadError;
static String runtimeError;
static ValueList stackTrace;
static bool exitASAP = false;
static int exitResult = 0;

//--------------------------------------------------------------------------------
// Output callbacks for MiniScript
//--------------------------------------------------------------------------------

static void Print(String s, bool lineBreak = true) {
	printf("%s%s", s.c_str(), lineBreak ? "\n" : "");
}

static void PrintErr(String s, bool lineBreak = true) {
	runtimeError = s;
	scriptState = ERRORED;
	stackTrace = Intrinsics::StackList(interpreter->vm);
	ResetRaylibCallbackBridge();
	printf("%s%s", s.c_str(), lineBreak ? "\n" : "");
}

//--------------------------------------------------------------------------------
// Script loading
//--------------------------------------------------------------------------------

#ifdef PLATFORM_WEB

void onScriptFetched(emscripten_fetch_t *fetch) {
	if (fetch->status == 200) {
		printf("Downloaded %llu bytes from URL %s\n", fetch->numBytes, fetch->url);

		char* scriptData = (char*)malloc(fetch->numBytes + 1);
		if (scriptData) {
			memcpy(scriptData, fetch->data, fetch->numBytes);
			scriptData[fetch->numBytes] = '\0';
			scriptSource = String(scriptData);
			free(scriptData);
			printf("Successfully loaded script from %s\n", fetch->url);
		} else {
			loadError = "Memory allocation failed";
			scriptState = ERRORED;
			printf("Failed to allocate memory for script\n");
		}
	} else {
		loadError = String("HTTP error: ") + String::Format(fetch->status);
		scriptState = ERRORED;
		printf("Failed to download %s: HTTP %d\n", fetch->url, fetch->status);
	}

	emscripten_fetch_close(fetch);
}

void fetchScript(const char *url) {
	printf("Fetching script from %s...\n", url);

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "GET");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess = onScriptFetched;
	attr.onerror = onScriptFetched;

	emscripten_fetch(&attr, url);
}

#else // PLATFORM_DESKTOP

void loadScriptFromFile(const char *path) {
	printf("Loading script from %s...\n", path);

	char* text = LoadFileText(path);
	if (text != nullptr) {
		scriptSource = String(text);
		UnloadFileText(text);
		printf("Successfully loaded script from %s\n", path);
	} else {
		loadError = String("Failed to load file: ") + path;
		scriptState = ERRORED;
		printf("Failed to load %s\n", path);
	}
}

#endif

//--------------------------------------------------------------------------------
// Import intrinsic
//--------------------------------------------------------------------------------

#include <map>

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
// exit intrinsic
//--------------------------------------------------------------------------------

static IntrinsicResult intrinsic_exit(Context *context, IntrinsicResult partialResult) {
	exitASAP = true;
	Value resultCode = context->GetVar("resultCode");
	if (!resultCode.IsNull()) exitResult = (int)resultCode.IntValue();
	context->vm->Stop();
	return IntrinsicResult::Null;
}

//--------------------------------------------------------------------------------
// Initialize MiniScript
//--------------------------------------------------------------------------------

void InitMiniScript() {
	MiniScript::hostVersion = 0.2;
	MiniScript::hostName = "raylib-miniscript";
	MiniScript::hostInfo = "https://github.com/JoeStrout/raylib-miniscript";
	ResetRaylibCallbackBridge();

	interpreter = new Interpreter();
	interpreter->standardOutput = &Print;
	interpreter->errorOutput = &PrintErr;
	interpreter->implicitOutput = &Print;

	// Add Raylib intrinsics
	AddRaylibIntrinsics();

#ifndef PLATFORM_WEB
	// Add file module (desktop only)
	AddFileModuleIntrinsics();
#endif

	// Add import intrinsic
	Intrinsic *importFunc = Intrinsic::Create("import");
	importFunc->AddParam("libname", "");
	importFunc->code = &intrinsic_import;

	// Add exit intrinsic
	Intrinsic *exitFunc = Intrinsic::Create("exit");
	exitFunc->AddParam("resultCode");
	exitFunc->code = &intrinsic_exit;

	printf("MiniScript interpreter initialized with Raylib intrinsics\n");
}

//--------------------------------------------------------------------------------
// Run the loaded script
//--------------------------------------------------------------------------------

void RunScript() {
	if (scriptSource.empty()) {
		PrintErr("No script to run");
		return;
	}

	printf("Compiling script...\n");
	interpreter->Reset(scriptSource);
	interpreter->Compile();

	printf("Starting script execution...\n");
	scriptState = RUNNING;
}

//--------------------------------------------------------------------------------
// Main loop
//--------------------------------------------------------------------------------

void MainLoop() {
	// Start the script when it's loaded but not yet started
	if (scriptState == LOADING && !scriptSource.empty()) {
		RunScript();
	}

	if (scriptState == RUNNING) {
		if (!interpreter->Done()) {
			try {
				interpreter->RunUntilDone(0.1, true);
			} catch (MiniscriptException& mse) {
				PrintErr("Runtime Exception: " + mse.message);
				interpreter->vm->Stop();
				ResetRaylibCallbackBridge();
				scriptState = ERRORED;
			}
		} else {
			scriptState = COMPLETE;
			printf("Script finished\n");
		}
	} else {
		// Show loading, error, or completion screen
		BeginDrawing();
		ClearBackground(RAYWHITE);

		if (scriptState == LOADING) {
			DrawText("raylib-miniscript", 10, 10, 30, DARKBLUE);
			DrawText("Loading assets/main.ms...", 10, 50, 20, GRAY);

			int dots = ((int)(GetTime() * 2)) % 4;
			const char* dotStr[] = {"", ".", "..", "..."};
			DrawText(dotStr[dots], 250, 50, 20, GRAY);
		} else if (scriptState == ERRORED) {
			DrawText("raylib-miniscript", 10, 10, 30, DARKBLUE);
			if (!loadError.empty()) {
				DrawText("Error loading script:", 10, 50, 20, RED);
				DrawText(loadError.c_str(), 10, 80, 16, RED);
				DrawText("Make sure assets/main.ms exists", 10, 110, 10, GRAY);
			} else if (!runtimeError.empty()) {
				DrawText("The game has halted due to an error:", 10, 50, 20, RED);
				DrawText(runtimeError.c_str(), 10, 80, 20, RED);
				int y = 110;
				for (int i = 0; i < stackTrace.Count(); i++) {
					String entry = stackTrace[i].ToString();
					DrawText(entry.c_str(), 30, y, 20, GRAY);
					y += 20;
				}
			}
		} else if (scriptState == COMPLETE) {
			DrawText("Script Completed", 10, 10, 20, DARKGREEN);
			DrawText("Check console for output", 10, 50, 10, GRAY);
		}

		EndDrawing();
	}
}

//--------------------------------------------------------------------------------
// Cleanup
//--------------------------------------------------------------------------------

void CleanupMiniScript() {
	ResetRaylibCallbackBridge();
	if (interpreter) {
		delete interpreter;
		interpreter = nullptr;
	}
}

//--------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	const int screenWidth = 960;
	const int screenHeight = 640;

	InitWindow(screenWidth, screenHeight, "raylib-miniscript");
	SetTargetFPS(60);
	InitAudioDevice();

#ifdef PLATFORM_WEB
	InstallLoadFileHooks();
#endif

	// Initialize MiniScript
	InitMiniScript();

	// Load the main script
#ifdef PLATFORM_WEB
	fetchScript("assets/main.ms");
#else
	const char* scriptPath = (argc > 1) ? argv[1] : "assets/main.ms";
	loadScriptFromFile(scriptPath);
#endif

	// Main loop
#ifdef PLATFORM_WEB
	emscripten_set_main_loop(MainLoop, 0, 1);
#else
	while (!WindowShouldClose() && !exitASAP) {
		MainLoop();
	}
#endif

	// Cleanup
	CleanupMiniScript();
	CloseAudioDevice();
	CloseWindow();

	return exitResult;
}
