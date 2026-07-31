//
//  UserDisks.cpp
//  raylib-miniscript
//
//  See UserDisks.h.
//

#ifndef PLATFORM_WEB

#include "UserDisks.h"
#include "HostPaths.h"
#include "FileSystem.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#if _WIN32 || _WIN64
	#define PATHSEP '\\'
#else
	#define PATHSEP '/'
#endif

namespace userdisks {

//--------------------------------------------------------------------------------
// Module state
//--------------------------------------------------------------------------------

// Function-local statics: a String cannot safely be constructed at static-init
// time, before the runtime's string machinery exists.

static Args& args() { static Args a; return a; }
static String& defaultDiskName() { static String s; return s; }
static std::vector<DroppedFile>& dropQueue() { static std::vector<DroppedFile> q; return q; }
static float dropX = 0.0f;
static float dropY = 0.0f;

static void Log(const char* fmt, ...) {
	// Mount activity goes to the host log, like every other fs diagnostic: it
	// names host paths, which script must never see.
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "[disks] ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

//--------------------------------------------------------------------------------
// key=value files (preferences, and the host options file)
//--------------------------------------------------------------------------------

struct KeyValue {
	std::string key;
	std::string value;
};

static void TrimAscii(std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) { s.clear(); return; }
	size_t end = s.find_last_not_of(" \t\r\n");
	s = s.substr(start, end - start + 1);
}

// Deliberately minimal: one key=value per line, '#' comments, no escaping, no
// quoting.  Values are host paths, which may contain anything except a newline
// -- and a path with a newline in it is not worth a format for.
static bool ReadKeyValueFile(const String& path, std::vector<KeyValue>& out) {
	FILE* f = fopen(path.c_str(), "r");
	if (f == nullptr) return false;
	char line[2048];
	while (fgets(line, sizeof(line), f) != nullptr) {
		std::string s(line);
		size_t hash = s.find('#');
		if (hash == 0) continue;
		size_t eq = s.find('=');
		if (eq == std::string::npos) continue;
		KeyValue kv;
		kv.key = s.substr(0, eq);
		kv.value = s.substr(eq + 1);
		TrimAscii(kv.key);
		TrimAscii(kv.value);
		if (kv.key.empty()) continue;
		out.push_back(kv);
	}
	fclose(f);
	return true;
}

static std::string Lookup(const std::vector<KeyValue>& kvs, const char* key) {
	for (size_t i = 0; i < kvs.size(); i++) {
		if (kvs[i].key == key) return kvs[i].value;
	}
	return std::string();
}

//--------------------------------------------------------------------------------
// Preferences
//--------------------------------------------------------------------------------
//
// Two keys, "usr" and "usr2", each the host path of what is mounted there.
// Written when the user mounts something and removed when they unmount it, so
// the file is a record of the user's standing choice rather than a log of every
// disk that was ever in the drive.  That is Mini Micro 1's behavior, from
// DiskManager.cs, and it is what makes "unmount, quit, relaunch" mean what a
// user expects.

static std::vector<KeyValue>& prefs() {
	static std::vector<KeyValue> p;
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		String path = hostpaths::PrefsPath();
		if (!path.empty()) ReadKeyValueFile(path, p);
	}
	return p;
}

static bool WritePrefs() {
	String dir = hostpaths::AppDataDir();
	String path = hostpaths::PrefsPath();
	if (dir.empty() || path.empty()) return false;
	if (!hostpaths::EnsureDir(dir)) {
		Log("could not create %s; preferences not saved", dir.c_str());
		return false;
	}
	FILE* f = fopen(path.c_str(), "w");
	if (f == nullptr) {
		Log("could not write %s", path.c_str());
		return false;
	}
	fprintf(f, "# raylib-miniscript preferences for %s.  Edited by the app; safe to delete.\n",
		hostpaths::AppId().c_str());
	std::vector<KeyValue>& p = prefs();
	for (size_t i = 0; i < p.size(); i++) {
		fprintf(f, "%s=%s\n", p[i].key.c_str(), p[i].value.c_str());
	}
	fclose(f);
	return true;
}

static void SetPref(const char* key, const String& value) {
	std::vector<KeyValue>& p = prefs();
	for (size_t i = 0; i < p.size(); i++) {
		if (p[i].key == key) {
			p[i].value = value.c_str();
			WritePrefs();
			return;
		}
	}
	KeyValue kv;
	kv.key = key;
	kv.value = value.c_str();
	p.push_back(kv);
	WritePrefs();
}

static void RemovePref(const char* key) {
	std::vector<KeyValue>& p = prefs();
	for (size_t i = 0; i < p.size(); i++) {
		if (p[i].key == key) {
			p.erase(p.begin() + i);
			WritePrefs();
			return;
		}
	}
}

static String GetPref(const char* key) {
	if (args().ignorePrefs) return String();
	return String(Lookup(prefs(), key).c_str());
}

//--------------------------------------------------------------------------------
// Command line and host options
//--------------------------------------------------------------------------------

Args ParseArgs(int argc, char* argv[]) {
	Args result;
	for (int i = 1; i < argc; i++) {
		const char* a = argv[i];
		if (strcmp(a, "-usr") == 0 && i + 1 < argc) {
			result.usrPath = String(argv[++i]);
		} else if (strcmp(a, "-usr2") == 0 && i + 1 < argc) {
			result.usr2Path = String(argv[++i]);
		} else if (strcmp(a, "--ignore-prefs") == 0) {
			result.ignorePrefs = true;
		} else if (result.scriptPath.empty()) {
			result.scriptPath = String(a);
		}
	}
	return result;
}

void SetArgs(const Args& a) { args() = a; }

void LoadHostOptions(const String& payloadDir) {
	if (payloadDir.empty()) return;
	std::string path = std::string(payloadDir.c_str()) + PATHSEP + "hostopts.txt";
	std::vector<KeyValue> opts;
	if (!ReadKeyValueFile(String(path.c_str()), opts)) return;   // no file: nothing to say

	std::string appId = Lookup(opts, "appId");
	if (!appId.empty()) hostpaths::SetAppId(String(appId.c_str()));

	std::string disk = Lookup(opts, "defaultDisk");
	if (!disk.empty()) defaultDiskName() = String(disk.c_str());

	Log("host options: appId=%s defaultDisk=%s",
		hostpaths::AppId().c_str(), disk.empty() ? "(none)" : disk.c_str());
}

//--------------------------------------------------------------------------------
// Mounting
//--------------------------------------------------------------------------------

bool IsUserMountName(const String& name) {
	return name == "usr" || name == "usr2";
}

static const char* PrefKeyFor(const String& mountName) {
	if (mountName == "usr") return "usr";
	if (mountName == "usr2") return "usr2";
	return nullptr;
}

bool MountHostPath(const String& mountName, const String& hostPath, bool remember) {
	if (!IsUserMountName(mountName)) {
		Log("refusing to mount /%s: only /usr and /usr2 are user disks", mountName.c_str());
		return false;
	}
	if (hostPath.empty()) return false;

	if (!hostpaths::Exists(hostPath)) {
		Log("nothing at %s", hostPath.c_str());
		return false;
	}

	if (!hostpaths::IsDirectory(hostPath)) {
		// A .minidisk archive lands here once the zip backend exists (step 6 of
		// notes/SANDBOXING.md); until then a file is simply not a disk.
		Log("%s is not a folder, and disk files are not supported yet", hostPath.c_str());
		return false;
	}

	fs::Backend* backend = fs::RealDirBackend::Open(hostPath, /*writable*/ true);
	if (backend == nullptr) {
		Log("could not open %s", hostPath.c_str());
		return false;
	}

	fs::Mount(mountName, backend);
	Log("mounted /%s from %s", mountName.c_str(), hostPath.c_str());

	// Remember the canonical path rather than the one we were handed: a relative
	// path or one full of symlinks would not survive a change of working
	// directory, and preferences outlive the process that wrote them.
	if (remember) {
		String canonical = backend->SourcePath();
		SetPref(PrefKeyFor(mountName), canonical.empty() ? hostPath : canonical);
	}
	return true;
}

bool MountAppData(const String& mountName, const String& subfolder) {
	if (!IsUserMountName(mountName)) {
		Log("refusing to mount /%s: only /usr and /usr2 are user disks", mountName.c_str());
		return false;
	}

	// A single path component, and nothing clever.  This value can come from
	// script, so this check -- not the caller's good intentions -- is what keeps
	// the mount inside our own application data directory.
	std::string sub(subfolder.c_str());
	if (sub.empty() || sub == "." || sub == ".."
			|| sub.find('/') != std::string::npos
			|| sub.find('\\') != std::string::npos
			|| sub[0] == '.') {
		Log("invalid app data folder name");
		return false;
	}

	String base = hostpaths::AppDataDir();
	if (base.empty()) {
		Log("no application data directory on this system");
		return false;
	}
	std::string full = std::string(base.c_str()) + PATHSEP + sub;
	if (!hostpaths::EnsureDir(String(full.c_str()))) {
		Log("could not create %s", full.c_str());
		return false;
	}

	// Not remembered: this is the application's own decision, made afresh every
	// run, and writing it to preferences would make a program's temporary mount
	// outlive the program.
	return MountHostPath(mountName, String(full.c_str()), /*remember*/ false);
}

bool UnmountUserDisk(const String& mountName) {
	if (!IsUserMountName(mountName)) {
		Log("refusing to unmount /%s", mountName.c_str());
		return false;
	}
	bool wasMounted = fs::Unmount(mountName);
	RemovePref(PrefKeyFor(mountName));
	if (wasMounted) Log("unmounted /%s", mountName.c_str());
	return wasMounted;
}

//--------------------------------------------------------------------------------
// Boot
//--------------------------------------------------------------------------------

static bool MountDefaultDisk() {
	const String& name = defaultDiskName();
	if (name.empty()) return false;    // host did not ask for one

	String docs = hostpaths::DocumentsDir();
	if (docs.empty()) return false;
	std::string full = std::string(docs.c_str()) + PATHSEP + std::string(name.c_str());

	bool existed = hostpaths::Exists(String(full.c_str()));
	if (!hostpaths::EnsureDir(String(full.c_str()))) {
		Log("could not create default disk at %s", full.c_str());
		return false;
	}
	if (!existed) Log("created a new user disk at %s", full.c_str());

	// Step 6 replaces this folder with a user.minidisk archive in the same
	// place; MountHostPath's sniffing is what will make that a one-line change.
	return MountHostPath("usr", String(full.c_str()), /*remember*/ false);
}

void MountAtBoot() {
	// /usr: command line wins over preferences, which win over the default disk.
	if (!args().usrPath.empty()) {
		if (!MountHostPath("usr", args().usrPath, /*remember*/ false)) {
			Log("-usr %s could not be mounted", args().usrPath.c_str());
		}
	} else {
		String remembered = GetPref("usr");
		if (!remembered.empty() && !MountHostPath("usr", remembered, /*remember*/ false)) {
			// The remembered disk is gone: an ejected volume, a deleted folder,
			// a renamed one.  Mini Micro 1 creates a fresh disk at that path;
			// we deliberately do not, because doing so either fails confusingly
			// or leaves a stray disk on a drive the user will remount later.
			// Falling back to the default disk keeps the machine usable.
			Log("remembered /usr (%s) is gone; falling back to the default disk",
				remembered.c_str());
		}
		if (fs::MountedBackend("usr") == nullptr) MountDefaultDisk();
	}

	// /usr2: no default.  A second disk is something the user asked for, and an
	// empty second drive is the normal state.
	String usr2 = args().usr2Path.empty() ? GetPref("usr2") : args().usr2Path;
	if (!usr2.empty() && !MountHostPath("usr2", usr2, /*remember*/ false)) {
		Log("/usr2 (%s) could not be mounted", usr2.c_str());
	}
}

//--------------------------------------------------------------------------------
// Dropped files
//--------------------------------------------------------------------------------

void PollDroppedFiles() {
	// IsFileDropped touches raylib's window state, which does not exist until
	// the script opens a window.
	if (!IsWindowReady() || !IsFileDropped()) return;

	// Every GLFW backend sets the cursor position from the drop before
	// delivering the file list -- performDragOperation on macOS, DragQueryPoint
	// under WM_DROPFILES on Windows, XdndPosition on X11 -- so this is the drop
	// point, not wherever the mouse happened to be last.  It is also all the
	// context a drop carries: no modifier state reaches a window that has not
	// been receiving key events, which is the case throughout a drag from
	// another application.
	Vector2 pos = GetMousePosition();
	dropX = pos.x;
	dropY = pos.y;

	FilePathList files = LoadDroppedFiles();
	std::vector<DroppedFile>& q = dropQueue();
	q.clear();     // a new drop replaces the old one; see ClearDroppedFiles
	for (unsigned int n = 0; n < files.count; n++) {
		DroppedFile f;
		f.hostPath = String(files.paths[n]);
		const char* base = GetFileName(files.paths[n]);
		f.name = String(base != nullptr ? base : files.paths[n]);
		f.isDirectory = hostpaths::IsDirectory(f.hostPath);
		q.push_back(f);
	}
	UnloadDroppedFiles(files);
}

const std::vector<DroppedFile>& DroppedFiles() { return dropQueue(); }

void DropPosition(float& outX, float& outY) { outX = dropX; outY = dropY; }

void ClearDroppedFiles() { dropQueue().clear(); }

bool MountDropped(int index, const String& mountName) {
	std::vector<DroppedFile>& q = dropQueue();
	if (index < 0 || index >= (int)q.size()) {
		// Almost always a handler that cleared the queue before mounting from
		// it, which otherwise looks exactly like an unmountable disk.
		Log("no dropped file at index %d (%d in the queue)", index, (int)q.size());
		return false;
	}
	// A drop is the user naming this target, so it is remembered, exactly as a
	// choice made through a file picker would be.
	return MountHostPath(mountName, q[index].hostPath, /*remember*/ true);
}

} // namespace userdisks

#endif // !PLATFORM_WEB
