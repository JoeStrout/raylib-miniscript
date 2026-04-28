// raylib-miniscript - MiniScript + Raylib
// A MiniScript-driven application with Raylib graphics

#include "raylib.h"
#include "SimpleString.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptIntrinsics.h"
#include "RaylibIntrinsics.h"
#include "FileModule.h"
#include "MoreIntrinsics.h"
#include "HttpModule.h"
#include "loadfile.h"
#include <stdio.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#else
#include <cstdlib>
#include <cstring>
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
// Initialize MiniScript
//--------------------------------------------------------------------------------

void InitMiniScript() {
	MiniScript::hostVersion = 0.3;
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

	// Add import and exit intrinsics
	AddMoreIntrinsics();

	// Add HTTP module
	AddHttpIntrinsics();

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
	SetTargetFPS(60);
	InitAudioDevice();

#ifdef PLATFORM_WEB
	InstallLoadFileHooks();
#endif

	// Set up path environment variables (desktop only)
#ifndef PLATFORM_WEB
	const char* scriptPath = (argc > 1) ? argv[1] : "assets/main.ms";

	// MS_EXE_DIR: directory containing the executable
	const char* appDir = GetApplicationDirectory();
	String exeDir(appDir);
	// Strip trailing path separator if present
	if (exeDir.LengthB() > 1 && (exeDir[exeDir.LengthB()-1] == '/' || exeDir[exeDir.LengthB()-1] == '\\')) {
		exeDir = exeDir.SubstringB(0, exeDir.LengthB()-1);
	}
#if defined(_WIN32)
	_putenv_s("MS_EXE_DIR", exeDir.c_str());
#else
	setenv("MS_EXE_DIR", exeDir.c_str(), 1);
#endif

	// MS_SCRIPT_DIR: directory containing the script being run
	UpdateScriptDir(scriptPath);
#endif

	// Initialize MiniScript
	InitMiniScript();

	// Load the main script
#ifdef PLATFORM_WEB
	fetchScript("assets/main.ms");
#else
	loadScriptFromFile(scriptPath);
#endif

	// Main loop
#ifdef PLATFORM_WEB
	emscripten_set_main_loop(MainLoop, 0, 1);
#else
	while (true) {
		MainLoop();
		if (ExitRequested() || (IsWindowReady() && WindowShouldClose())) break;
	}
#endif

	// Cleanup
	CleanupMiniScript();
	if (IsAudioDeviceReady()) CloseAudioDevice();
	if (IsWindowReady()) CloseWindow();

	return ExitResultCode();
}
