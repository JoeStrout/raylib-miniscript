//
//  Plugin.cpp
//  raylib-miniscript
//
//  Walks the generated plugin table (kMSPlugins) at the engine's lifecycle points,
//  and gives scripts the `plugins` map for finding out what loaded.
//

#include "Plugin.h"
#include "PluginEvents.h"
#include "macros.h"
#include "raylib.h"
#include <vector>

namespace {

struct PluginState {
	const char* name;
	const MSPluginHooks* hooks;
	bool loaded;
};

std::vector<PluginState>& States() {
	static std::vector<PluginState> states;
	return states;
}

// plugins -> {name: {"loaded": 1 or 0, "version": "..."}}, built on first use.
void AddPluginsIntrinsic() {
	using namespace MiniScript;
	Intrinsic f = Intrinsic::Create("plugins");
	f.set_Code(INTRINSIC_LAMBDA {
		static Value pluginsValue;
		if (pluginsValue.IsNull()) {
			ValueDict all;
			for (const PluginState& p : States()) {
				ValueDict info;
				info.SetValue(String("loaded"), Value(p.loaded ? 1 : 0));
				info.SetValue(String("version"), Value(String(p.hooks->version ? p.hooks->version : "")));
				all.SetValue(String(p.name), GCManager::NewMapFromDict(info));
			}
			pluginsValue = GCManager::NewMapFromDict(all);
			GCManager::AddRoot(pluginsValue);
		}
		return IntrinsicResult(pluginsValue);
	});
}

} // namespace

void PluginsInit() {
	PluginsShutDownFlag() = false;
	std::vector<PluginState>& states = States();
	states.clear();
	for (const MSPluginEntry* e = kMSPlugins; e->name != nullptr; e++) {
		PluginState st = { e->name, e->get(), false };
		TraceLog(LOG_INFO, "PLUGIN: Loading %s", st.name);
		st.loaded = st.hooks->init != nullptr && st.hooks->init();
		if (!st.loaded) TraceLog(LOG_WARNING, "PLUGIN: %s unavailable (init failed)", st.name);
		states.push_back(st);
	}
	AddPluginsIntrinsic();
}

void PluginsUpdate() {
	for (const PluginState& p : States()) {
		if (p.loaded && p.hooks->update) p.hooks->update();
	}
}

void PluginsReset() {
	for (const PluginState& p : States()) {
		if (p.loaded && p.hooks->reset) p.hooks->reset();
	}
}

void PluginsShutdown() {
	// Reverse order, so a plugin can rely on those loaded before it.
	std::vector<PluginState>& states = States();
	for (size_t i = states.size(); i > 0; i--) {
		const PluginState& p = states[i - 1];
		if (p.loaded && p.hooks->shutdown) p.hooks->shutdown();
	}
	PluginsShutDownFlag() = true;
}
