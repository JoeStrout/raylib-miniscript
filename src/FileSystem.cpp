//
//  FileSystem.cpp
//  raylib-miniscript
//
//  See FileSystem.h.  Path manipulation in here is done in std::string rather
//  than MiniScript::String: the byte-vs-codepoint indexing rules differ between
//  the two, and path folding is one place where getting that wrong is a
//  security bug rather than a display glitch.  Conversion happens at the API
//  boundary only.
//

#include "FileSystem.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>

#if _WIN32 || _WIN64
	#define WINDOWS 1
	#include <windows.h>
	#include <direct.h>
	#include <sys/stat.h>
	#define PATHSEP '\\'
	#define getcwd _getcwd
	#define chdir _chdir
#else
	#include <dirent.h>
	#include <limits.h>
	#include <stdlib.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#define PATHSEP '/'
#endif

namespace fs {

//--------------------------------------------------------------------------------
// Module state
//--------------------------------------------------------------------------------

// Function-local statics: a String cannot safely be constructed at static-init
// time, before the runtime's string machinery exists.

struct MountEntry {
	String name;
	Backend* backend;
	bool listed;
};

static std::vector<MountEntry>& mounts() {
	static std::vector<MountEntry> m;
	return m;
}

static std::string& virtualCwd() {
	static std::string cwd = "/";
	return cwd;
}

static bool sandboxed = false;

//--------------------------------------------------------------------------------
// Diagnostics
//--------------------------------------------------------------------------------

void LogRejection(const String& path, const char* reason) {
	fprintf(stderr, "[fs] rejected \"%s\": %s\n", path.c_str(), reason);
}

//--------------------------------------------------------------------------------
// Host path helpers (native separators)
//--------------------------------------------------------------------------------

// Canonicalize an existing host path.  Fails if it does not exist (on POSIX;
// _fullpath is purely lexical, so on Windows it succeeds either way).
static bool RealPathOfExisting(const std::string& path, std::string& out) {
#if WINDOWS
	char buf[MAX_PATH];
	// _fullpath canonicalizes but does not resolve symlinks or junctions.  A
	// Windows build that must resist reparse-point escapes needs
	// GetFinalPathNameByHandle here instead.
	if (_fullpath(buf, path.c_str(), sizeof(buf)) == NULL) return false;
	out = buf;
	return true;
#else
	char buf[PATH_MAX];
	if (realpath(path.c_str(), buf) == NULL) return false;
	out = buf;
	return true;
#endif
}

// Canonicalize a host path that may not exist yet, by canonicalizing its parent
// and re-appending the final component.  Creating a file needs this: the target
// is not there to resolve, but its directory is, and that is the part a symlink
// could use to escape.
static bool CanonicalizeHostPath(const std::string& path, std::string& out) {
	if (RealPathOfExisting(path, out)) return true;

	size_t slash = path.find_last_of(PATHSEP);
	if (slash == std::string::npos) return false;
	std::string parent = path.substr(0, slash);
	std::string name = path.substr(slash + 1);
	if (parent.empty()) parent = std::string(1, PATHSEP);
	if (name.empty() || name == "." || name == "..") return false;

	std::string canonParent;
	if (!RealPathOfExisting(parent, canonParent)) return false;
	if (!canonParent.empty() && canonParent[canonParent.size() - 1] == PATHSEP) out = canonParent + name;
	else out = canonParent + PATHSEP + name;
	return true;
}

// Is `path` the root itself, or inside it?  The component-boundary check is what
// keeps a mount at /Users/joe/usr from also matching /Users/joe/usr2.
static bool IsUnderRoot(const std::string& path, const std::string& root) {
	if (root.empty()) return true;
	if (path.size() < root.size()) return false;
	if (path.compare(0, root.size(), root) != 0) return false;
	if (path.size() == root.size()) return true;
	if (root[root.size() - 1] == PATHSEP) return true;   // root is "/" or similar
	return path[root.size()] == PATHSEP;
}

static String TimestampToString(const struct tm& t) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
		1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	return String(buf);
}

//--------------------------------------------------------------------------------
// Backend defaults
//--------------------------------------------------------------------------------

bool Backend::ReadText(const String& path, String& outText) {
	BinaryData* data = ReadBinary(path);
	if (data == nullptr) return false;
	outText = String((const char*)data->bytes, (size_t)data->length);
	delete data;
	return true;
}

static const char* kReadOnly = "disk is read-only";

bool Backend::WriteBinary(const String& path, const unsigned char* data, long length, String& outErr) {
	(void)path; (void)data; (void)length;
	outErr = kReadOnly;
	return false;
}

bool Backend::WriteText(const String& path, const String& text, String& outErr) {
	return WriteBinary(path, (const unsigned char*)text.c_str(), (long)text.sizeB(), outErr);
}

bool Backend::MakeDir(const String& path, String& outErr) {
	(void)path;
	outErr = kReadOnly;
	return false;
}

bool Backend::Delete(const String& path, String& outErr) {
	(void)path;
	outErr = kReadOnly;
	return false;
}

bool Backend::Rename(const String& oldPath, const String& newPath, String& outErr) {
	(void)oldPath; (void)newPath;
	outErr = kReadOnly;
	return false;
}

//--------------------------------------------------------------------------------
// RealDirBackend
//--------------------------------------------------------------------------------

RealDirBackend* RealDirBackend::Open(const String& basePath, bool writable) {
	std::string canon;
	if (!RealPathOfExisting(std::string(basePath.c_str()), canon)) return nullptr;
	// Strip a trailing separator so the component-boundary test below is exact.
	// "/" itself has nothing to strip, and IsUnderRoot handles it separately.
	while (canon.size() > 1 && canon[canon.size() - 1] == PATHSEP) canon.erase(canon.size() - 1);
	return new RealDirBackend(String(canon.c_str()), writable, true);
}

bool RealDirBackend::NativePath(const String& relPath, String& outNative) const {
	if (!contained) {
		// Passthrough: hand the host exactly what it would have received before
		// this layer existed, native separators and all.
		outNative = relPath;
		return true;
	}

	std::string rel(relPath.c_str());
#if WINDOWS
	for (size_t i = 0; i < rel.size(); i++) if (rel[i] == '/') rel[i] = PATHSEP;
#endif
	std::string root(base.c_str());
	std::string full = root;
	if (!rel.empty()) {
		if (full.empty() || full[full.size() - 1] != PATHSEP) full += PATHSEP;
		full += rel;
	}

	std::string canon;
	// A path we cannot canonicalize is simply one that is not there -- an
	// ordinary miss, not worth logging.
	if (!CanonicalizeHostPath(full, canon)) return false;

	if (!IsUnderRoot(canon, root)) {
		// This one is worth logging: a symlink (or something stranger) inside
		// the mount points back out of it.
		LogRejection(relPath, "resolves outside its mount root");
		return false;
	}
	outNative = String(canon.c_str());
	return true;
}

bool RealDirBackend::RealPath(const String& path, String& outRealPath) {
	return NativePath(path, outRealPath);
}

bool RealDirBackend::GetFileNames(const String& dirPath, std::vector<String>& outNames) {
	String native;
	if (!NativePath(dirPath, native)) return false;
#if WINDOWS
	std::string pattern = std::string(native.c_str()) + "\\*";
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(pattern.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE) return false;
	do {
		if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0) continue;
		outNames.push_back(String(data.cFileName));
	} while (FindNextFile(hFind, &data) != 0);
	FindClose(hFind);
	return true;
#else
	DIR* dir = opendir(native.c_str());
	if (dir == NULL) return false;
	while (struct dirent* entry = readdir(dir)) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		outNames.push_back(String(entry->d_name));
	}
	closedir(dir);
	return true;
#endif
}

bool RealDirBackend::GetFileInfo(const String& path, FileInfo& outInfo) {
	String native;
	if (!NativePath(path, native)) return false;
#if WINDOWS
	struct _stati64 stats;
	if (_stati64(native.c_str(), &stats) != 0) return false;
	outInfo.isDirectory = (stats.st_mode & _S_IFDIR) != 0;
	outInfo.size = (long)stats.st_size;
	struct tm t;
	localtime_s(&t, &stats.st_mtime);
#else
	struct stat stats;
	if (stat(native.c_str(), &stats) < 0) return false;
	outInfo.isDirectory = S_ISDIR(stats.st_mode);
	outInfo.size = (long)stats.st_size;
	struct tm t;
	tzset();
	#if defined(__APPLE__) || defined(__FreeBSD__)
		localtime_r(&(stats.st_mtimespec.tv_sec), &t);
	#else
		localtime_r(&stats.st_mtime, &t);
	#endif
#endif
	outInfo.date = TimestampToString(t);
	return true;
}

BinaryData* RealDirBackend::ReadBinary(const String& path) {
	String native;
	if (!NativePath(path, native)) return nullptr;
	FILE* f = fopen(native.c_str(), "rb");
	if (f == NULL) return nullptr;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size < 0) { fclose(f); return nullptr; }

	BinaryData* data = new BinaryData((int)size);
	if (size > 0 && fread(data->bytes, 1, (size_t)size, f) != (size_t)size) {
		fclose(f);
		delete data;
		return nullptr;
	}
	fclose(f);
	return data;
}

bool RealDirBackend::WriteBinary(const String& path, const unsigned char* data, long length, String& outErr) {
	if (!writable) { outErr = kReadOnly; return false; }
	String native;
	if (!NativePath(path, native)) { outErr = "invalid path"; return false; }
	FILE* f = fopen(native.c_str(), "wb");
	if (f == NULL) { outErr = "could not open file for writing"; return false; }
	size_t written = (length > 0) ? fwrite(data, 1, (size_t)length, f) : 0;
	fclose(f);
	if ((long)written != length) { outErr = "write error"; return false; }
	return true;
}

bool RealDirBackend::MakeDir(const String& path, String& outErr) {
	if (!writable) { outErr = kReadOnly; return false; }
	String native;
	if (!NativePath(path, native)) { outErr = "invalid path"; return false; }
#if WINDOWS
	bool ok = CreateDirectory(native.c_str(), NULL) != 0;
#else
	bool ok = (mkdir(native.c_str(), 0755) == 0);
#endif
	if (!ok) { outErr = "could not create directory"; return false; }
	return true;
}

bool RealDirBackend::Delete(const String& path, String& outErr) {
	if (!writable) { outErr = kReadOnly; return false; }
	FileInfo info;
	if (!GetFileInfo(path, info)) { outErr = "file not found"; return false; }
	String native;
	if (!NativePath(path, native)) { outErr = "invalid path"; return false; }
#if WINDOWS
	bool ok = info.isDirectory ? (RemoveDirectory(native.c_str()) != 0)
	                           : (DeleteFile(native.c_str()) != 0);
#else
	bool ok = (remove(native.c_str()) == 0);
#endif
	if (!ok) { outErr = "could not delete"; return false; }
	return true;
}

bool RealDirBackend::Rename(const String& oldPath, const String& newPath, String& outErr) {
	if (!writable) { outErr = kReadOnly; return false; }
	String oldNative, newNative;
	if (!NativePath(oldPath, oldNative) || !NativePath(newPath, newNative)) {
		outErr = "invalid path";
		return false;
	}
	if (rename(oldNative.c_str(), newNative.c_str()) != 0) { outErr = "could not rename"; return false; }
	return true;
}

Backend* PassthroughBackend() {
	// Unsandboxed mode still goes through a Backend, so that every call site is
	// written exactly once.  An "if (IsSandboxed())" branch at each of the
	// several dozen path-taking intrinsics would be precisely the bypass
	// surface this layer exists to remove.
	static RealDirBackend* passthrough = new RealDirBackend(String(""), true, false);
	return passthrough;
}

//--------------------------------------------------------------------------------
// Mount table
//--------------------------------------------------------------------------------

void Mount(const String& name, Backend* backend, bool listed) {
	std::string n(name.c_str());
	if (n.empty() || n.find('/') != std::string::npos || n.find('\\') != std::string::npos) {
		LogRejection(name, "invalid mount name");
		delete backend;
		return;
	}
	std::vector<MountEntry>& m = mounts();
	for (size_t i = 0; i < m.size(); i++) {
		if (n == m[i].name.c_str()) {
			if (m[i].backend != backend && m[i].backend != nullptr) {
				m[i].backend->Close();
				delete m[i].backend;
			}
			m[i].backend = backend;
			m[i].listed = listed;
			return;
		}
	}
	MountEntry entry;
	entry.name = name;
	entry.backend = backend;
	entry.listed = listed;
	m.push_back(entry);
}

bool Unmount(const String& name) {
	std::string n(name.c_str());
	std::vector<MountEntry>& m = mounts();
	for (size_t i = 0; i < m.size(); i++) {
		if (n == m[i].name.c_str()) {
			if (m[i].backend != nullptr) {
				m[i].backend->Close();
				delete m[i].backend;
			}
			m.erase(m.begin() + i);
			return true;
		}
	}
	return false;
}

Backend* MountedBackend(const String& name) {
	std::string n(name.c_str());
	std::vector<MountEntry>& m = mounts();
	for (size_t i = 0; i < m.size(); i++) {
		if (n == m[i].name.c_str()) return m[i].backend;
	}
	return nullptr;
}

void ListedMountNames(std::vector<String>& outNames) {
	std::vector<MountEntry>& m = mounts();
	for (size_t i = 0; i < m.size(); i++) {
		if (m[i].listed) outNames.push_back(m[i].name);
	}
}

void CloseAllMounts() {
	std::vector<MountEntry>& m = mounts();
	for (size_t i = 0; i < m.size(); i++) {
		if (m[i].backend != nullptr) {
			m[i].backend->Close();
			delete m[i].backend;
		}
	}
	m.clear();
}

//--------------------------------------------------------------------------------
// The latch
//--------------------------------------------------------------------------------

void EnterSandbox() { sandboxed = true; }
bool IsSandboxed() { return sandboxed; }

//--------------------------------------------------------------------------------
// Path string helpers
//--------------------------------------------------------------------------------

String PathCombine(const String& basePath, const String& partialPath) {
	std::string base(basePath.c_str());
	std::string partial(partialPath.c_str());
	if (base == "/" && !partial.empty() && partial[0] == '/') return partialPath;
	if (base.empty() || base[base.size() - 1] != '/') base += '/';
	return String((base + partial).c_str());
}

String GetFileName(const String& path) {
	std::string p(path.c_str());
	size_t pos = p.find_last_of('/');
	if (pos == std::string::npos) return path;
	return String(p.substr(pos + 1).c_str());
}

String StripFileName(const String& path) {
	std::string p(path.c_str());
	size_t pos = p.find_last_of('/');
	if (pos == std::string::npos) return path;
	if (pos == 0) return String("/");
	return String(p.substr(0, pos).c_str());
}

//--------------------------------------------------------------------------------
// Resolution
//--------------------------------------------------------------------------------

bool ResolvePath(const String& path, String& outVirtualPath) {
	if (!sandboxed) {
		// Host paths pass through untouched: normalizing here would mangle
		// native separators and Windows drive letters, changing behavior for
		// every host that is not Mini Micro.
		outVirtualPath = path;
		return true;
	}

	std::string p(path.c_str());

	// The separator is '/', always, on every platform.  A backslash is not a
	// separator we translate; it is a path we refuse.
	if (p.find('\\') != std::string::npos) {
		LogRejection(path, "backslash is not a path separator");
		return false;
	}

	if (p.empty() || p[0] != '/') {
		std::string base = virtualCwd();
		if (base.empty() || base[base.size() - 1] != '/') base += '/';
		p = base + p;
	}

	// Fold "." and ".." lexically, on the virtual path, before anything touches
	// the host.  This is what guarantees a ".." never reaches the file system.
	std::vector<std::string> stack;
	size_t i = 0;
	while (i < p.size()) {
		size_t next = p.find('/', i);
		if (next == std::string::npos) next = p.size();
		std::string part = p.substr(i, next - i);
		i = next + 1;
		if (part.empty() || part == ".") continue;
		if (part == "..") {
			if (stack.empty()) {
				// Above the root.  Mini Micro calls this an invalid path, and
				// it is indistinguishable from any other invalid path -- which
				// is the point.
				LogRejection(path, "path goes above the root");
				return false;
			}
			stack.pop_back();
			continue;
		}
		stack.push_back(part);
	}

	std::string result;
	for (size_t k = 0; k < stack.size(); k++) {
		result += '/';
		result += stack[k];
	}
	if (result.empty()) result = "/";
	outVirtualPath = String(result.c_str());
	return true;
}

Backend* GetMount(const String& virtualPath, String& outRelPath) {
	if (!sandboxed) {
		outRelPath = virtualPath;
		return PassthroughBackend();
	}

	std::string p(virtualPath.c_str());
	if (p.empty() || p[0] != '/') return nullptr;

	size_t slash = p.find('/', 1);
	std::string name = (slash == std::string::npos) ? p.substr(1) : p.substr(1, slash - 1);
	std::string rel = (slash == std::string::npos) ? std::string() : p.substr(slash + 1);
	if (name.empty()) return nullptr;   // the virtual root is not a mount

	std::vector<MountEntry>& m = mounts();
	for (size_t k = 0; k < m.size(); k++) {
		if (name == m[k].name.c_str()) {
			outRelPath = String(rel.c_str());
			return m[k].backend;
		}
	}
	return nullptr;
}

bool Resolve(const String& path, Resolved& out) {
	String virtualPath;
	if (!ResolvePath(path, virtualPath)) return false;
	String relPath;
	Backend* backend = GetMount(virtualPath, relPath);
	if (backend == nullptr) {
		LogRejection(virtualPath, "no such mount");
		return false;
	}
	out.backend = backend;
	out.virtualPath = virtualPath;
	out.relPath = relPath;
	return true;
}

//--------------------------------------------------------------------------------
// Virtual working directory
//--------------------------------------------------------------------------------

String Cwd() {
	if (!sandboxed) {
		char buf[PATH_MAX];
		if (getcwd(buf, sizeof(buf)) == NULL) return String("");
		return String(buf);
	}
	return String(virtualCwd().c_str());
}

bool SetCwd(const String& path) {
	if (!sandboxed) {
		if (path.empty()) return false;
		return chdir(path.c_str()) == 0;
	}
	String virtualPath;
	if (!ResolvePath(path, virtualPath)) return false;
	// Only an existing directory may become the cwd -- otherwise a later
	// relative path would resolve against somewhere that is not there.
	FileInfo info;
	if (!GetInfo(virtualPath, info) || !info.isDirectory) return false;
	virtualCwd() = std::string(virtualPath.c_str());
	return true;
}

//--------------------------------------------------------------------------------
// Operations over the mount table
//--------------------------------------------------------------------------------

static bool IsVirtualRoot(const String& virtualPath) {
	return sandboxed && strcmp(virtualPath.c_str(), "/") == 0;
}

bool GetInfo(const String& path, FileInfo& outInfo) {
	String virtualPath;
	if (!ResolvePath(path, virtualPath)) return false;
	if (IsVirtualRoot(virtualPath)) {
		// The root is not backed by any mount; it is the table itself.
		outInfo.isDirectory = true;
		outInfo.size = 0;
		return true;
	}
	String relPath;
	Backend* backend = GetMount(virtualPath, relPath);
	if (backend == nullptr) return false;
	return backend->GetFileInfo(relPath, outInfo);
}

bool Exists(const String& path, bool& outIsDirectory) {
	FileInfo info;
	outIsDirectory = false;
	if (!GetInfo(path, info)) return false;
	outIsDirectory = info.isDirectory;
	return true;
}

bool Exists(const String& path) {
	bool ignored;
	return Exists(path, ignored);
}

bool ListDir(const String& path, std::vector<String>& outNames) {
	String virtualPath;
	if (!ResolvePath(path, virtualPath)) return false;
	if (IsVirtualRoot(virtualPath)) {
		// Hidden mounts (/hw) are absent here, so that code enumerating "/"
		// sees exactly the disks Mini Micro documents.
		ListedMountNames(outNames);
		return true;
	}
	String relPath;
	Backend* backend = GetMount(virtualPath, relPath);
	if (backend == nullptr) return false;
	return backend->GetFileNames(relPath, outNames);
}

BinaryData* ReadBinary(const String& path) {
	Resolved r;
	if (!Resolve(path, r)) return nullptr;
	return r.backend->ReadBinary(r.relPath);
}

bool ReadText(const String& path, String& outText) {
	Resolved r;
	if (!Resolve(path, r)) return false;
	return r.backend->ReadText(r.relPath, outText);
}

String WriteBinary(const String& path, const unsigned char* data, long length) {
	Resolved r;
	if (!Resolve(path, r)) return String("invalid path");
	String err;
	if (!r.backend->WriteBinary(r.relPath, data, length, err)) return err;
	return String();
}

String WriteText(const String& path, const String& text) {
	Resolved r;
	if (!Resolve(path, r)) return String("invalid path");
	String err;
	if (!r.backend->WriteText(r.relPath, text, err)) return err;
	return String();
}

String MakeDir(const String& path) {
	Resolved r;
	if (!Resolve(path, r)) return String("invalid path");
	String err;
	if (!r.backend->MakeDir(r.relPath, err)) return err;
	return String();
}

String Delete(const String& path) {
	Resolved r;
	if (!Resolve(path, r)) return String("invalid path");
	String err;
	if (!r.backend->Delete(r.relPath, err)) return err;
	return String();
}

String MoveOrCopy(const String& oldPath, const String& newPath, bool deleteSource, bool overwriteDest) {
	Resolved src, dst;
	if (!Resolve(oldPath, src)) return String("source not found");
	if (!Resolve(newPath, dst)) return String("target disk not found");
	if (strcmp(src.virtualPath.c_str(), dst.virtualPath.c_str()) == 0) return String();  // nothing to do
	if (!dst.backend->IsWritable()) return String("target disk is not writeable");

	// If the destination names a directory, move into it under the source name.
	FileInfo dstInfo;
	if (dst.backend->GetFileInfo(dst.relPath, dstInfo)) {
		if (dstInfo.isDirectory) {
			dst.relPath = PathCombine(dst.relPath, GetFileName(src.virtualPath));
			dst.virtualPath = PathCombine(dst.virtualPath, GetFileName(src.virtualPath));
			if (dst.backend->GetFileInfo(dst.relPath, dstInfo) && !overwriteDest) {
				return String("target file already exists");
			}
		} else if (!overwriteDest) {
			return String("target file already exists");
		}
	}

	if (src.backend == dst.backend && deleteSource) {
		// Same mount: a rename is both cheaper and atomic.
		String err;
		if (overwriteDest) dst.backend->Delete(dst.relPath, err);
		if (!src.backend->Rename(src.relPath, dst.relPath, err)) return err;
		return String();
	}

	// Different mounts, or a copy was asked for: move the bytes.
	BinaryData* data = src.backend->ReadBinary(src.relPath);
	if (data == nullptr) return String("could not read source");
	String err;
	bool ok = dst.backend->WriteBinary(dst.relPath, data->bytes, data->length, err);
	delete data;
	if (!ok) return err;
	if (deleteSource) {
		String ignored;
		src.backend->Delete(src.relPath, ignored);   // it is OK if this fails
	}
	return String();
}

//--------------------------------------------------------------------------------
// OpenFile
//--------------------------------------------------------------------------------

OpenFile::OpenFile(const Resolved& res, const String& mode) {
	backend = res.backend;
	relPath = res.relPath;
	if (backend == nullptr) { error = "file not found"; return; }

	std::string m(mode.c_str());
	for (size_t i = 0; i < m.size(); i++) {
		if (m[i] >= 'A' && m[i] <= 'Z') m[i] = (char)(m[i] - 'A' + 'a');
	}
	if (m.find('b') != std::string::npos) { error = "binary mode not supported"; return; }

	// Load whatever is already there, for the modes that keep it.
	bool loaded = false;
	if (m == "r" || m == "r+" || m == "rw+" || m == "a" || m == "a+") {
		BinaryData* data = backend->ReadBinary(relPath);
		if (data != nullptr) {
			buf.assign(data->bytes, data->bytes + data->length);
			delete data;
			loaded = true;
		}
	}

	if (m == "r") {
		if (!loaded) { error = "file not found"; return; }
		readable = true;
	} else if (m == "r+") {
		if (!loaded) { error = "file not found"; return; }
		readable = writable = true;
	} else if (m == "w") {
		writable = needSave = true;
	} else if (m == "w+") {
		readable = writable = needSave = true;
	} else if (m == "rw+") {
		// Like "a+", but positioned at the start rather than the end.
		readable = writable = true;
	} else if (m == "a") {
		writable = true;
		pos = buf.size();
	} else if (m == "a+") {
		readable = writable = true;
		pos = buf.size();
	} else {
		error = String("invalid file mode (") + mode + ")";
		return;
	}
	open = true;
}

void OpenFile::SetPosition(long p) {
	if (!open) return;
	if (p < 0) p = 0;
	if ((size_t)p > buf.size()) p = (long)buf.size();
	pos = (size_t)p;
}

void OpenFile::Write(const String& text) {
	if (text.empty()) return;
	if (!open) { error = "file is not open"; return; }
	if (!writable) { error = "stream is not writeable"; return; }
	if (!backend->IsWritable()) { error = "disk is not writeable"; return; }

	const unsigned char* bytes = (const unsigned char*)text.c_str();
	size_t length = (size_t)text.sizeB();
	if (pos + length > buf.size()) buf.resize(pos + length);
	memcpy(&buf[pos], bytes, length);
	pos += length;
	needSave = true;
}

bool OpenFile::ReadToEnd(String& out) {
	if (!open) { error = "file is not open"; return false; }
	if (!readable) { error = "stream is not readable"; return false; }
	if (pos >= buf.size()) {
		out = String("");
		return true;
	}
	out = String((const char*)&buf[pos], buf.size() - pos);
	pos = buf.size();
	return true;
}

bool OpenFile::ReadLine(String& out) {
	if (!open) { error = "file is not open"; return false; }
	if (!readable) { error = "stream is not readable"; return false; }
	if (pos >= buf.size()) return false;

	// Accept 10, 13, or 13,10 as the terminator.
	size_t eol = pos;
	while (eol < buf.size() && buf[eol] != 10 && buf[eol] != 13) eol++;
	out = String((const char*)&buf[pos], eol - pos);
	if (eol < buf.size() && buf[eol] == 13 && eol + 1 < buf.size() && buf[eol + 1] == 10) eol++;
	pos = (eol < buf.size()) ? eol + 1 : buf.size();
	return true;
}

bool OpenFile::ReadChars(int codePointCount, String& out) {
	if (!open) { error = "file is not open"; return false; }
	if (!readable) { error = "stream is not readable"; return false; }
	if (pos >= buf.size()) return false;

	// Count code points, not bytes: in UTF-8 a character starts with the high
	// bit clear or the top two bits set, and continuation bytes are 0b10xxxxxx.
	size_t end = pos;
	int count = 0;
	while (end < buf.size() && count < codePointCount) {
		end++;
		count++;
		while (end < buf.size() && (buf[end] & 0xC0) == 0x80) end++;
	}
	out = String((const char*)&buf[pos], end - pos);
	pos = end;
	return true;
}

void OpenFile::Close() {
	if (!open) return;
	if (writable && needSave) {
		String err;
		const unsigned char* bytes = buf.empty() ? (const unsigned char*)"" : &buf[0];
		if (!backend->WriteBinary(relPath, bytes, (long)buf.size(), err)) error = err;
	}
	open = false;
	buf.clear();
	pos = 0;
}

} // namespace fs
