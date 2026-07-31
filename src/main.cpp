// raylib-miniscript - MiniScript + Raylib
// A MiniScript-driven application with Raylib graphics

// Include MiniScript before raylib: raylib.h #defines PI as a macro, which
// otherwise clobbers MiniScript's Math::PI constant when its headers are parsed.
#include "miniscript.h"
#include "raylib.h"
#include "RaylibIntrinsics.h"
#include "FileModule.h"
#include "MoreIntrinsics.h"
#include "HttpModule.h"
#include "FileSystem.h"
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

static Interpreter interpreter;   // value type; default-constructed with null storage until InitMiniScript
static ScriptState scriptState = LOADING;
static String scriptSource;
static String loadError;
static String runtimeError;
static Value stackTrace;           // stack trace list Value, captured on error

//--------------------------------------------------------------------------------
// Output callbacks for MiniScript
//--------------------------------------------------------------------------------

static void Print(String s, Boolean lineBreak = true) {
	printf("%s%s", s.c_str(), lineBreak ? "\n" : "");
}

static void PrintErr(String s, Boolean lineBreak = true) {
	runtimeError = s;
	scriptState = ERRORED;
	// Capture a stack trace if the VM exists (it won't for compile-time errors).
	if (!IsNull(interpreter.vm())) stackTrace = interpreter.vm().BuildStackTrace();
	ResetRaylibCallbackBridge();
	printf("%s%s", s.c_str(), lineBreak ? "\n" : "");

	// Echo the stack trace to the console too.  The error screen can only show
	// it when the script happened to have a window open; the console always can,
	// and for a headless script it's the only place it would ever appear.
	if (stackTrace.IsList()) {
		for (int i = 0; i < stackTrace.ListCount(); i++) {
			printf("\t%s\n", stackTrace.ListGet(i).ToString().c_str());
		}
	}
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
// Mounting the app payload
//--------------------------------------------------------------------------------

#ifndef PLATFORM_WEB

static void MountAppPayload(const char* scriptPath) {
	const char* lastSlash = strrchr(scriptPath, '/');
#ifdef _WIN32
	const char* lastBackslash = strrchr(scriptPath, '\\');
	if (lastBackslash && (!lastSlash || lastBackslash > lastSlash)) lastSlash = lastBackslash;
#endif
	String dir = lastSlash ? String(scriptPath, (size_t)(lastSlash - scriptPath)) : String(".");

	// Each disk is a *named subdirectory* of the boot script's directory, never
	// that directory itself.  Mounting the script's own directory would put the
	// boot script and the whole library tree on a disk any program can read;
	// a host application should decide what it publishes, one folder at a time.
	fs::Backend* hw = fs::RealDirBackend::Open(dir + "/hw", false);
	if (hw != nullptr) {
		// Hidden: file.children("/") must return exactly the disks the user
		// knows about, so that Mini Micro 1 code enumerating the root sees what
		// it expects.  /hw is undocumented, not secret -- script can read it,
		// list it, and load from it.
		fs::Mount(String("hw"), hw, /*listed*/ false);
	}

	// /sys ships on its own release cycle (the minimicro-sysdisk repo), while
	// /hw versions with the executable.  Keeping them separate avoids either
	// putting host assets in minimicro-sysdisk or building a union mount.
	fs::Backend* sys = fs::RealDirBackend::Open(dir + "/sys", false);
	if (sys != nullptr) fs::Mount(String("sys"), sys);

	if (hw == nullptr && sys == nullptr) return;   // not a disk-based app; say nothing
	printf("Mounted%s%s\n", hw != nullptr ? " /hw" : "", sys != nullptr ? " /sys" : "");
}

#endif

//--------------------------------------------------------------------------------
// Initialize MiniScript
//--------------------------------------------------------------------------------

void InitMiniScript() {
	MiniScript::hostVersion = "0.3";   // a String in MS2, not a double
	MiniScript::hostName = "raylib-miniscript";
	MiniScript::hostInfo = "https://github.com/JoeStrout/raylib-miniscript";
	ResetRaylibCallbackBridge();

	interpreter = Interpreter::New();
	interpreter.set_standardOutput(&Print);
	interpreter.set_errorOutput(&PrintErr);
	interpreter.set_implicitOutput(&Print);

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
	interpreter.Reset(scriptSource);
	interpreter.Compile();

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
		if (!interpreter.Done()) {
			// MS2 does not throw for runtime errors; they are delivered to the
			// errorOutput delegate (PrintErr), which sets the ERRORED state.
			interpreter.RunUntilDone(0.1, true);
		} else {
			scriptState = COMPLETE;
			printf("Script finished\n");
		}
	} else if (IsWindowReady()) {
		// Show loading, error, or completion screen.  Opening a window is the
		// script's job, so there may not be one -- drawing regardless would
		// crash in BeginDrawing.  Without a window these states are reported on
		// the console only, and the main loop below exits.
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
				if (stackTrace.IsList()) {
					for (int i = 0; i < stackTrace.ListCount(); i++) {
						String entry = stackTrace.ListGet(i).ToString();
						DrawText(entry.c_str(), 30, y, 20, GRAY);
						y += 20;
					}
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
	interpreter = nullptr;   // releases the shared InterpreterStorage
	fs::CloseAllMounts();    // a writable backend may have buffered state to flush
}

//--------------------------------------------------------------------------------
// Main
//--------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	// Bring up the MiniScript runtime before ANY Value/String/GC operation
	// (including the env-var setup below and InitMiniScript).  In MS1 strings
	// were refcounted so this was implicit; in MS2 strings are GC-allocated, so
	// the GC and value constants must be initialized first, mirroring the
	// startup sequence in MiniScript's own App entry point.
	MiniScript::GCManager::Init();       // create the GC sets
	MiniScript::value_init_constants();  // Value::magicIsA, selfString, etc.
	MiniScript::ErrorTypes::Init();      // error type prototypes

	SetTargetFPS(60);
	InitAudioDevice();

#ifdef PLATFORM_WEB
	InstallLoadFileHooks();
#endif

	// Set up path environment variables (desktop only)
#ifndef PLATFORM_WEB
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

	// With no script argument, run assets/main.ms -- from the working directory
	// if it's there (the development case: `./build/raylib-miniscript` run from
	// a repo whose assets/ are live, not the post-build copy), otherwise from
	// beside the executable.  That second path is what a packaged app needs: its
	// payload ships next to the binary, and launched from the Finder or a
	// desktop shortcut the working directory is somewhere else entirely.
	String defaultScript = "assets/main.ms";
	if (!FileExists(defaultScript.c_str())) defaultScript = exeDir + "/assets/main.ms";
	const char* scriptPath = (argc > 1) ? argv[1] : defaultScript.c_str();

	// MS_SCRIPT_DIR: directory containing the script being run
	UpdateScriptDir(scriptPath);

	// Mount the app payload.  The boot script's own directory is the payload,
	// so it becomes /hw -- the hardware disk, holding whatever resources the
	// host application ships with (Mini Micro's screen font, bezel, sticker,
	// boot chime).  If that directory has a sys/ inside it, that becomes /sys,
	// the system disk.  Both are read-only.
	//
	// These mounts exist from boot, before any latch, so a host application can
	// address its own resources as /hw/... from its very first line.  That also
	// fixes the working-directory-relative form those paths used to have, which
	// was already broken for a packaged app launched from the Finder.
	//
	// /usr and /usr2 stay unmounted: mounting a user disk is the user's
	// decision, made through a file picker, and Mini Micro is perfectly usable
	// without one.
	//
	// Nothing here restricts anything.  Until a script calls file.enterSandbox,
	// paths that name no mount still reach the host file system as before, so
	// stock raylib-miniscript and Soda are unaffected.
	MountAppPayload(scriptPath);
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
		if (ExitRequested()) break;
		if (IsWindowReady()) {
			if (WindowShouldClose()) break;
		} else if (scriptState == ERRORED || scriptState == COMPLETE) {
			// Headless script: nothing to keep on screen, so we're done.
			break;
		}
	}
#endif

	// Cleanup
	CleanupMiniScript();
	if (IsAudioDeviceReady()) CloseAudioDevice();
	if (IsWindowReady()) CloseWindow();

	// A script that failed should not report success, unless it chose its own
	// result code by calling `exit`.
	if (scriptState == ERRORED && !ExitRequested()) return 1;
	return ExitResultCode();
}
