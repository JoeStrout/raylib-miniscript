//
//  HostPaths.cpp
//  raylib-miniscript
//
//  See HostPaths.h.  Like FileSystem.cpp, path work here is done in std::string
//  rather than MiniScript::String: byte-vs-codepoint indexing differs between
//  them, and paths are bytes.
//

#ifndef PLATFORM_WEB

#include "HostPaths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>

#if _WIN32 || _WIN64
	#define WINDOWS 1
	#include <windows.h>
	#include <direct.h>
	#define PATHSEP '\\'
	#define mkdir(p, m) _mkdir(p)
#else
	#include <pwd.h>
	#include <unistd.h>
	#define PATHSEP '/'
#endif

namespace hostpaths {

//--------------------------------------------------------------------------------
// App identity
//--------------------------------------------------------------------------------

// Function-local static: a String cannot safely be constructed at static-init
// time, before the runtime's string machinery exists.
static String& appIdStorage() {
	static String id("raylib-miniscript");
	return id;
}

void SetAppId(const String& appId) {
	if (appId.empty()) return;
	// A path component, not a path: an appId with a separator in it would put
	// preferences somewhere nobody expects, and "../.." would put them anywhere
	// at all.  This value comes from the app payload rather than from script, so
	// this is a typo check rather than a security boundary -- but it is cheap.
	std::string id(appId.c_str());
	if (id.find('/') != std::string::npos || id.find('\\') != std::string::npos
			|| id == "." || id == "..") {
		fprintf(stderr, "[hostpaths] ignoring invalid appId \"%s\"\n", id.c_str());
		return;
	}
	appIdStorage() = appId;
}

const String& AppId() { return appIdStorage(); }

//--------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------

static std::string Join(const std::string& base, const std::string& leaf) {
	if (base.empty()) return leaf;
	if (leaf.empty()) return base;
	if (base[base.size() - 1] == PATHSEP) return base + leaf;
	return base + std::string(1, PATHSEP) + leaf;
}

static std::string EnvOrEmpty(const char* name) {
	const char* v = getenv(name);
	return (v != nullptr && *v != '\0') ? std::string(v) : std::string();
}

bool IsDirectory(const String& path) {
	struct stat st;
	if (stat(path.c_str(), &st) != 0) return false;
	return (st.st_mode & S_IFMT) == S_IFDIR;
}

bool Exists(const String& path) {
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

bool EnsureDir(const String& path) {
	std::string p(path.c_str());
	if (p.empty()) return false;

	// Walk the path making each missing component in turn.  Start past the root
	// so we never try to create "/" (or "C:\") itself.
	size_t i = 0;
	while (i < p.size() && p[i] == PATHSEP) i++;
#if WINDOWS
	if (p.size() >= 2 && p[1] == ':') i = 2;
	while (i < p.size() && p[i] == PATHSEP) i++;
#endif
	for (; i <= p.size(); i++) {
		if (i < p.size() && p[i] != PATHSEP) continue;
		std::string sofar = p.substr(0, i == 0 ? 1 : i);
		if (sofar.empty()) continue;
		struct stat st;
		if (stat(sofar.c_str(), &st) == 0) {
			if ((st.st_mode & S_IFMT) != S_IFDIR) return false;   // a file is in the way
		} else if (mkdir(sofar.c_str(), 0755) != 0) {
			return false;
		}
	}
	return true;
}

//--------------------------------------------------------------------------------
// The directories themselves
//--------------------------------------------------------------------------------

String HomeDir() {
#if WINDOWS
	std::string h = EnvOrEmpty("USERPROFILE");
	if (h.empty()) {
		std::string drive = EnvOrEmpty("HOMEDRIVE");
		std::string path = EnvOrEmpty("HOMEPATH");
		if (!drive.empty() && !path.empty()) h = drive + path;
	}
	return String(h.c_str());
#else
	std::string h = EnvOrEmpty("HOME");
	if (h.empty()) {
		// HOME can be unset under launchd, cron, and some app bundles.
		struct passwd* pw = getpwuid(getuid());
		if (pw != nullptr && pw->pw_dir != nullptr) h = pw->pw_dir;
	}
	return String(h.c_str());
#endif
}

String AppDataDir() {
	std::string home(HomeDir().c_str());
	std::string id(AppId().c_str());
#if WINDOWS
	std::string base = EnvOrEmpty("APPDATA");
	if (base.empty()) base = Join(home, "AppData\\Roaming");
#elif defined(__APPLE__)
	std::string base = Join(home, "Library/Application Support");
#else
	std::string base = EnvOrEmpty("XDG_DATA_HOME");
	if (base.empty()) base = Join(home, ".local/share");
#endif
	if (base.empty()) return String();
	return String(Join(base, id).c_str());
}

String PrefsPath() {
	String dir = AppDataDir();
	if (dir.empty()) return String();
	return String(Join(std::string(dir.c_str()), "prefs.txt").c_str());
}

#if !WINDOWS && !defined(__APPLE__)
// Linux: honor the user's chosen Documents folder, which the desktop
// environment records as a shell-style assignment in user-dirs.dirs, e.g.
//   XDG_DOCUMENTS_DIR="$HOME/Documents"
static std::string XdgDocumentsDir(const std::string& home) {
	std::string configHome = EnvOrEmpty("XDG_CONFIG_HOME");
	if (configHome.empty()) configHome = Join(home, ".config");
	std::string path = Join(configHome, "user-dirs.dirs");

	FILE* f = fopen(path.c_str(), "r");
	if (f == nullptr) return std::string();

	std::string result;
	char line[1024];
	while (fgets(line, sizeof(line), f) != nullptr) {
		std::string s(line);
		size_t start = s.find_first_not_of(" \t");
		if (start == std::string::npos || s[start] == '#') continue;
		const std::string key = "XDG_DOCUMENTS_DIR=";
		if (s.compare(start, key.size(), key) != 0) continue;
		std::string value = s.substr(start + key.size());
		while (!value.empty() && (value[value.size()-1] == '\n' || value[value.size()-1] == '\r'
				|| value[value.size()-1] == ' ' || value[value.size()-1] == '\t')) {
			value.erase(value.size() - 1);
		}
		if (value.size() >= 2 && value[0] == '"' && value[value.size()-1] == '"') {
			value = value.substr(1, value.size() - 2);
		}
		if (value.compare(0, 5, "$HOME") == 0) value = home + value.substr(5);
		result = value;
		break;
	}
	fclose(f);
	return result;
}
#endif

String DocumentsDir() {
	std::string home(HomeDir().c_str());
	if (home.empty()) return String();

#if !WINDOWS && !defined(__APPLE__)
	std::string xdg = XdgDocumentsDir(home);
	if (!xdg.empty() && IsDirectory(String(xdg.c_str()))) return String(xdg.c_str());
#endif

	std::string docs = Join(home, "Documents");
	if (IsDirectory(String(docs.c_str()))) return String(docs.c_str());

	// No Documents folder at all (a headless account, or a locale where the
	// folder has another name we did not find).  The home directory is a poorer
	// place for a user disk, but it is a real one, and it always exists.
	return String(home.c_str());
}

} // namespace hostpaths

#endif // !PLATFORM_WEB
