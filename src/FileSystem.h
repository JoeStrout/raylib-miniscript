//
//  FileSystem.h
//  raylib-miniscript
//
//  A virtual, sandboxable file system: mount table, storage backend interface,
//  path resolver, and a one-way sandbox latch.
//
//  This layer is the single chokepoint for every script-supplied path.  Nothing
//  else in the host should touch one: the intrinsic bindings hand paths to
//  Resolve() (or to the convenience operations below) and work with what comes
//  back.  A backend never sees a real host path unless it made one itself.
//
//  It mirrors Mini Micro 1's disk layer (Assets/Scripts/Disk/Disk.cs and
//  FileUtils.cs), whose method set has already survived years of real use.
//
//  Off by default.  Until EnterSandbox() is called, resolution is a passthrough
//  to the host file system, so stock raylib-miniscript and Soda behave exactly
//  as they did before this layer existed.
//

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "miniscript.h"
#include "RawData.h"
#include <vector>

namespace fs {

using MiniScript::String;
using MiniScript::BinaryData;

//--------------------------------------------------------------------------------
// FileInfo
//--------------------------------------------------------------------------------

struct FileInfo {
	String date;              // "YYYY-MM-DD HH:MM:SS", local time
	long size = 0;
	bool isDirectory = false;
	String comment;
};

//--------------------------------------------------------------------------------
// Backend: a storage device behind a mount point
//--------------------------------------------------------------------------------

// Every path passed to a Backend is *mount-relative*: '/'-separated, no leading
// slash, and already lexically normalized (no "." or ".." components, no empty
// components).  Backends do no path folding of their own; the resolver has
// already done it, which is what keeps ".." from ever reaching the host.
//
// Mutating methods return false and set outErr on failure.  Those messages are
// for the host log, never for script -- see the note on LogRejection below.
class Backend {
public:
	virtual ~Backend() {}

	// Prepare for disuse (unmount, or app quit).
	virtual void Close() {}

	virtual bool IsWritable() const { return false; }

	// Names (not paths) of the entries immediately within dirPath.  An empty
	// dirPath means the root of this mount.
	virtual bool GetFileNames(const String& dirPath, std::vector<String>& outNames) = 0;

	virtual bool GetFileInfo(const String& path, FileInfo& outInfo) = 0;

	// Caller owns the returned BinaryData.  Null means failure.
	virtual BinaryData* ReadBinary(const String& path) = 0;

	// Default implementation goes through ReadBinary.
	virtual bool ReadText(const String& path, String& outText);

	virtual bool WriteBinary(const String& path, const unsigned char* data, long length, String& outErr);
	virtual bool WriteText(const String& path, const String& text, String& outErr);
	virtual bool MakeDir(const String& path, String& outErr);
	virtual bool Delete(const String& path, String& outErr);
	virtual bool Rename(const String& oldPath, const String& newPath, String& outErr);

	// Where this mount came from, in host terms -- the directory or archive
	// behind it.  Host-only, and for exactly two purposes: remembering the
	// mount in preferences, and naming it in a host-drawn UI.  It must never
	// reach script; that is the whole point of rule 5 in notes/SANDBOXING.md.
	// Backends with no host path (IndexedDB) return an empty String.
	virtual String SourcePath() const { return String(); }

	// Optional fast path for raylib's loaders, which want a real file on disk.
	// A real-directory backend answers with a host path; zip and IndexedDB
	// backends decline, and the caller falls back to a *FromMemory loader over
	// ReadBinary.  The interface deliberately does not assume this case exists.
	virtual bool RealPath(const String& path, String& outRealPath) { (void)path; (void)outRealPath; return false; }
};

//--------------------------------------------------------------------------------
// RealDirBackend: a mount backed by a real host directory
//--------------------------------------------------------------------------------

class RealDirBackend : public Backend {
public:
	// Canonicalizes basePath once, here, and rejects the mount if that fails.
	// Doing it at mount time (rather than per operation) is also what makes the
	// development `sys` symlink into the minimicro-sysdisk checkout work.
	// Returns null on failure; the caller owns the result.
	static RealDirBackend* Open(const String& basePath, bool writable);

	virtual void Close() override {}
	virtual bool IsWritable() const override { return writable; }

	virtual bool GetFileNames(const String& dirPath, std::vector<String>& outNames) override;
	virtual bool GetFileInfo(const String& path, FileInfo& outInfo) override;
	virtual BinaryData* ReadBinary(const String& path) override;
	virtual bool WriteBinary(const String& path, const unsigned char* data, long length, String& outErr) override;
	virtual bool MakeDir(const String& path, String& outErr) override;
	virtual bool Delete(const String& path, String& outErr) override;
	virtual bool Rename(const String& oldPath, const String& newPath, String& outErr) override;
	virtual bool RealPath(const String& path, String& outRealPath) override;
	virtual String SourcePath() const override { return contained ? base : String(); }

	const String& BasePath() const { return base; }

private:
	RealDirBackend(const String& canonicalBase, bool writable, bool contained)
		: base(canonicalBase), writable(writable), contained(contained) {}

	// Map a mount-relative path to a host path, then confirm the canonicalized
	// result is still under our canonicalized base.  Without that second step a
	// symlink inside the mount walks straight out of it.  Returns false if it
	// does not check out; nothing else in this class touches the host without
	// going through here.
	bool NativePath(const String& relPath, String& outNative) const;

	String base;      // canonicalized, no trailing separator; empty when !contained
	bool writable;
	bool contained;   // false only for the unsandboxed passthrough backend

	friend Backend* PassthroughBackend();
};

//--------------------------------------------------------------------------------
// The mount table
//--------------------------------------------------------------------------------

// Host-only.  There is deliberately no intrinsic that reaches these: a script
// that could name a mount target could mount "/" and the sandbox would be over.
// Script-requested mounting arrives later, and goes through a native file
// picker so the *user* names the target.

// Mounts backend as "name" (no slashes, no leading slash), taking ownership of
// it.  Replacing an existing mount closes and deletes the old backend.
// `listed` false hides the mount from a listing of "/" -- see /hw.
void Mount(const String& name, Backend* backend, bool listed = true);

bool Unmount(const String& name);
Backend* MountedBackend(const String& name);
void ListedMountNames(std::vector<String>& outNames);
void CloseAllMounts();

//--------------------------------------------------------------------------------
// The sandbox latch
//--------------------------------------------------------------------------------

// One-way.  There is deliberately no way to leave sandbox mode: before the
// flip, during boot, the host is unrestricted and establishes its mounts; after
// it, every path argument from script is virtual, forever.
void EnterSandbox();
bool IsSandboxed();

//--------------------------------------------------------------------------------
// Path resolution
//--------------------------------------------------------------------------------

// Expand path against the current working directory and fold "." and ".."
// lexically, yielding a canonical virtual path.  No host file system access
// happens here, which is what guarantees a ".." can never reach the host.
// Returns false for an invalid path (including ".." above the root).
//
// When not sandboxed this returns path unchanged: host paths pass through with
// their native separators and drive letters intact.
bool ResolvePath(const String& path, String& outVirtualPath);

// Split a canonical virtual path into its mount and the rest.  Returns null if
// no such mount is present.
Backend* GetMount(const String& virtualPath, String& outRelPath);

struct Resolved {
	Backend* backend = nullptr;
	String virtualPath;   // canonical; this is what may be shown to script
	String relPath;       // mount-relative
	bool ok() const { return backend != nullptr; }
};

// ResolvePath + GetMount.  This is the entry point for nearly every caller.
// A false return means "does not exist" as far as script is concerned; the real
// reason went to the host log.
bool Resolve(const String& path, Resolved& out);

//--------------------------------------------------------------------------------
// Virtual working directory
//--------------------------------------------------------------------------------

// Semantically the `file` module owns the cwd, but the resolver is what needs
// it on every call, so the storage lives here and `file.curdir` / `file.setdir`
// become thin wrappers.  When not sandboxed these proxy to the real getcwd and
// chdir, preserving current behavior exactly.
String Cwd();
bool SetCwd(const String& path);

//--------------------------------------------------------------------------------
// Path string helpers (virtual paths; always '/'-separated)
//--------------------------------------------------------------------------------

String PathCombine(const String& basePath, const String& partialPath);
String GetFileName(const String& path);      // last component
String StripFileName(const String& path);    // everything but the last component

//--------------------------------------------------------------------------------
// Operations over the mount table
//--------------------------------------------------------------------------------
//
// These are the FileUtils-level operations: they resolve paths themselves and
// handle the cases that need more than one mount, or that concern the virtual
// root rather than any single mount.

bool Exists(const String& path, bool& outIsDirectory);
bool Exists(const String& path);

// "/" reports a synthetic directory rather than failing.
bool GetInfo(const String& path, FileInfo& outInfo);

// Listing "/" enumerates the *listed* mount names.
bool ListDir(const String& path, std::vector<String>& outNames);

BinaryData* ReadBinary(const String& path);
bool ReadText(const String& path, String& outText);

// Each returns an empty String on success, or an error message for the host log.
String WriteBinary(const String& path, const unsigned char* data, long length);
String WriteText(const String& path, const String& text);
String MakeDir(const String& path);
String Delete(const String& path);

// Move or copy, possibly across mounts.  Within one writable mount this is a
// rename; across mounts it reads the bytes and writes them to the destination.
String MoveOrCopy(const String& oldPath, const String& newPath, bool deleteSource, bool overwriteDest);

// Resolve a script-supplied path to a real host path, for raylib's loaders --
// which want a real file and cannot be handed bytes.  Returns false if the path
// does not resolve, or if the mount behind it has no real file to offer (a zip
// or IndexedDB backend); those callers fall back to raylib's *FromMemory
// variants over ReadBinary.
//
// This is the only function that hands a real path back to the host, and it
// must never hand one back to *script*: the result goes straight into a raylib
// call, never into a return value or an error message.
bool HostPath(const String& path, String& outHostPath);

// Convenience for the raylib bindings, whose paths always arrive as a Value
// straight from context.GetArg().  Saves a .ToString() at every call site.
bool HostPath(const MiniScript::Value& path, String& outHostPath);

// A literal would otherwise be ambiguous between the two overloads above, since
// both String and Value convert implicitly from const char*.  This resolves it.
bool HostPath(const char* path, String& outHostPath);

// Same, but for a path that is about to be written, deleted, renamed, or
// created.  Additionally requires the mount to be writable.
//
// HostPath alone is NOT enough for a destination: it happily hands back a real
// path inside a read-only mount, and raylib will then write to it -- the
// resolver has no idea what the caller intends to do with the path it returns.
// Every raylib entry point that modifies a file must use this instead.
bool HostPathForWrite(const String& path, String& outHostPath);
bool HostPathForWrite(const MiniScript::Value& path, String& outHostPath);
bool HostPathForWrite(const char* path, String& outHostPath);

//--------------------------------------------------------------------------------
// OpenFile: a file opened for reading and/or writing
//--------------------------------------------------------------------------------

// Like Mini Micro 1's OpenFile, this does all its work in memory and writes
// back on Close().  That is not a shortcut: a zip or IndexedDB backend has no
// seekable stream to hand out, so buffering here is what lets file handles work
// uniformly across every kind of mount.
//
// The consequence, which Mini Micro has lived with for years, is that a handle
// that is never closed never persists its writes.
class OpenFile {
public:
	// Check error after constructing: non-empty means the open failed.
	OpenFile(const Resolved& res, const String& mode);

	bool IsOpen() const { return open; }
	bool IsAtEnd() const { return !open || pos >= buf.size(); }
	long Position() const { return open ? (long)pos : 0; }
	void SetPosition(long p);

	bool IsReadable() const { return readable; }
	bool IsWritable() const { return writable; }

	void Write(const String& text);
	bool ReadToEnd(String& out);
	bool ReadLine(String& out);
	bool ReadChars(int codePointCount, String& out);

	void Close();

	String error;

private:
	Backend* backend = nullptr;
	String relPath;
	std::vector<unsigned char> buf;
	size_t pos = 0;
	bool open = false;
	bool readable = false;
	bool writable = false;
	bool needSave = false;
};

//--------------------------------------------------------------------------------
// Diagnostics
//--------------------------------------------------------------------------------

// A sandbox violation is indistinguishable from any other bad path, by design:
// a distinguishable error would let a program map the host file system by
// probing.  That also makes the sandbox indistinguishable from a bug, so every
// rejection is logged here -- to stderr, visible to whoever is running the
// build, never to script.
void LogRejection(const String& path, const char* reason);

// True when a script-facing entry point must refuse to act because we are
// sandboxed -- for the handful that cannot be made safe by resolving a path,
// because they leak a real path, change the real working directory, or let
// script replace raylib's file I/O underneath the resolver.  Logs the refusal
// on the way out, so a mysterious no-op is findable.
bool RefuseWhenSandboxed(const char* what);

// The passthrough backend used when not sandboxed.  Exposed for tests.
Backend* PassthroughBackend();

} // namespace fs

#endif // FILESYSTEM_H
