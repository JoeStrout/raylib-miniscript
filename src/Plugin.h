//
//  Plugin.h
//  raylib-miniscript
//
//  Compile-time plugins.  A folder under plugins/ is compiled into the engine
//  and its hooks are called at the right moments; see plugins/README.md.
//  CMake discovers the folders and generates the table of hooks, so adding a
//  plugin touches neither the build files nor main.cpp.
//

#ifndef PLUGIN_H
#define PLUGIN_H

#include "miniscript.h"
#include "raylib.h"

// What a plugin provides, in one descriptor.  A plugin's plugin.cpp contains
// exactly one MS_PLUGIN(...) line, naming its hooks.  To make a hook conditional,
// pick the name with #define OUTSIDE the macro (a directive inside the macro's
// arguments is undefined behavior and not portable):
//
//     static bool Init() { ...; return true; }   // false: unavailable on this run
//     #ifdef PLATFORM_DESKTOP
//         #define STEAM_UPDATE Update
//     #else
//         #define STEAM_UPDATE nullptr
//     #endif
//     MS_PLUGIN(steam, "1.0", Init, STEAM_UPDATE, nullptr, Shutdown)
//
// The folder name <name> must be a valid C identifier.
struct MSPluginHooks {
	const char* version;     // reported to scripts by `plugins`; "" if none
	bool (*init)();          // required: add intrinsics, once, at interpreter creation.
	                         //   Return false if the plugin cannot work this run (Steam
	                         //   not running...); it is then marked unavailable and its
	                         //   other hooks never run.  Release any SDK resources you
	                         //   acquired.  Intrinsics cannot be unregistered, so either
	                         //   call PluginAddModule last (only on success), or register
	                         //   always and have the functions fail safely (false/null)
	                         //   when the SDK is unavailable.
	void (*update)();        // optional: once per frame, on the main thread
	void (*reset)();         // optional: the interpreter is about to be reset
	void (*shutdown)();      // optional: the app is closing
};

#define MS_PLUGIN(name, version, init, update, reset, shutdown) \
	const MSPluginHooks* MSPlugin_##name() { \
		static const MSPluginHooks hooks = { version, init, update, reset, shutdown }; \
		return &hooks; \
	}

// One row of the generated table (build/plugin_registry.cpp).  The table ends
// with a row whose name is null.
struct MSPluginEntry {
	const char* name;
	const MSPluginHooks* (*get)();
};
extern const MSPluginEntry kMSPlugins[];

// Called from main.cpp.
void PluginsInit();
void PluginsUpdate();
void PluginsReset();
void PluginsShutdown();

// Make a global module (like `physicsCore`) named `name`, whose members `Fill`
// adds to the map it is given.  Call from your init hook:
//
//     static void Fill(MiniScript::ValueDict& m) { ... m.SetValue(String("new"), f.GetFunc()); }
//     static bool Init() { return PluginAddModule<&Fill>("quadtree"); }
//
// Registering under its own name keeps a plugin from clobbering core intrinsics:
// if an intrinsic of that name already exists, logs an error and returns false
// (so returning it from init marks the plugin unavailable).
template<void (*Fill)(MiniScript::ValueDict&)>
bool PluginAddModule(const char* name) {
	using namespace MiniScript;
	if (!IsNull(Intrinsic::GetByName(String(name)))) {
		TraceLog(LOG_ERROR, "PLUGIN: module name '%s' is already taken", name);
		return false;
	}
	Intrinsic f = Intrinsic::Create(String(name));
	f.set_Code([](Context context, IntrinsicResult partialResult) -> IntrinsicResult {
		// Built on first use, then wrapped and GC-rooted (see PhysicsCore.cpp).
		static ValueDict module;
		static Value moduleValue;
		if (moduleValue.IsNull()) {
			Fill(module);
			moduleValue = GCManager::NewMapFromDict(module);
			GCManager::AddRoot(moduleValue);
		}
		return IntrinsicResult(moduleValue);
	});
	return true;
}

#endif // PLUGIN_H
