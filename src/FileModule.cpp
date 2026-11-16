//
//  FileModule.cpp
//  raylib-miniscript
//
//  File module intrinsics for MiniScript (desktop only).
//  Based on the file module from command-line MiniScript (ShellIntrinsics.cpp).
//
//  Every path here goes through the fs layer (FileSystem.h) -- nothing in this
//  file opens, stats, or lists anything by itself.  Two rules follow from that
//  and are easy to break by accident:
//
//    1. No real host path may leave this file.  Not in a return value, not in
//       an error message.  file.info().path speaks virtual paths, or the host's
//       directory layout (including the user's home directory name) leaks.
//    2. A sandbox violation is indistinguishable from any other bad path.  The
//       fs layer already guarantees this; do not add an error message here that
//       would tell the two apart.
//
//  Until a script calls file.enterSandbox, fs resolution is a passthrough and
//  everything below behaves exactly as it did before the fs layer existed.
//

#ifndef PLATFORM_WEB

#include "FileModule.h"
#include "miniscript.h"
#include "RawData.h"
#include "FileSystem.h"
#include "UserDisks.h"
#include "MoreIntrinsics.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>
#include <vector>

#if _WIN32 || _WIN64
	#define WINDOWS 1
	#include <windows.h>
	#include <direct.h>
	#define getcwd _getcwd
	#define PATHSEP '\\'
#else
	#include <unistd.h>
	#include <libgen.h>
	#include <limits.h>
	#include <stdlib.h>
	#define PATHSEP '/'
#endif

using namespace MiniScript;

// A GC-backed string Value cannot be constructed at static-init time (before
// GCManager exists), so build the "_handle" key lazily on first use.  Interned
// strings (under 128 bytes) are only SEMI-immortal -- a full collection sweeps
// them -- and this key lives on no rooted prototype, so root it explicitly.
static const Value& _handleKey() {
	static Value k = [] { Value v("_handle"); GCManager::AddRoot(v); return v; }();
	return k;
}

static ValueDict fileModule;
static ValueDict fileHandleClass;

// The module and the FileHandle class prototype, as Values: each wrapped and
// GC-rooted once, where its dictionary is filled (see AddFileModuleIntrinsics).
// A host ValueDict is not reachable by the GC on its own, so wrapping one only
// on the first script access would leave its contents collectable in between.
// Holding the Value also means the accessors below -- and the per-instance
// __isa assignment in file.open -- are plain reads, with nothing to look up.
static Value fileModuleValue;
static Value fileHandleClassValue;

static Value FileHandleClassMap() {
	return fileHandleClassValue;
}

// Pull the fs::OpenFile out of a FileHandle instance, or null if it is not one.
static fs::OpenFile* OpenFileFor(Value self) {
	Value fileWrapper = self.Lookup(_handleKey());
	if (fileWrapper.IsNull() || fileWrapper.Type() != ValueType::Handle) return nullptr;
	return (fs::OpenFile*)fileWrapper.HandlePtr();
}

// The error every FileHandle method (but close and isOpen) returns when its
// handle is closed, or self is not a FileHandle at all.
static IntrinsicResult NotOpenError() {
	return IntrinsicResult(ErrorTypes::FileError("file is not open"));
}

// Like OpenFileFor, but also null when the file has been closed.
static fs::OpenFile* LiveFileFor(Value self) {
	fs::OpenFile* file = OpenFileFor(self);
	if (file == nullptr || !file->IsOpen()) return nullptr;
	return file;
}

//--------------------------------------------------------------------------------
// Path string helpers
//--------------------------------------------------------------------------------
//
// name, parent, and child are pure string operations -- they never touch a file
// system.  Sandboxed, they work on virtual paths, which are '/'-separated on
// every platform.  Unsandboxed, they are working on host paths, so they keep
// their old platform-specific behavior; changing that would alter what stock
// raylib-miniscript and Soda scripts see on Windows for no reason.

static IntrinsicResult intrinsic_getcwd(Context context, IntrinsicResult partialResult) {
	return IntrinsicResult(fs::Cwd());
}

// Shared by setdir and curdir=, which differ only in their parameter name and
// in the name the error message gives.  Null on success, or an error.
static IntrinsicResult ChangeDir(const char* who, Value path) {
	String pathStr;
	if (!path.IsNull()) pathStr = path.ToString();
	if (pathStr.empty() || !fs::SetCwd(pathStr)) {
		return IntrinsicResult(ErrorTypes::FileError(String(who) + ": could not change directory"));
	}
	return IntrinsicResult::Null;
}

static IntrinsicResult intrinsic_setdir(Context context, IntrinsicResult partialResult) {
	return ChangeDir("setdir", context.GetVar("path"));
}

static IntrinsicResult intrinsic_setCurdir(Context context, IntrinsicResult partialResult) {
	return ChangeDir("curdir", context.GetVar("value"));
}

static IntrinsicResult intrinsic_readdir(Context context, IntrinsicResult partialResult) {
	Value path = context.GetVar("path");
	String pathStr = path.ToString();
	if (path.IsNull() || pathStr.empty()) pathStr = fs::Cwd();
	std::vector<String> names;
	ValueList result;
	if (!fs::ListDir(pathStr, names)) {
		return IntrinsicResult(ErrorTypes::FileError("children: cannot read directory"));
	}
	for (size_t i = 0; i < names.size(); i++) result.Add(names[i]);
	return IntrinsicResult(DynamicList(result));
}

static IntrinsicResult intrinsic_basename(Context context, IntrinsicResult partialResult) {
	Value path = context.GetVar("path");
	if (path.IsNull()) return IntrinsicResult::Zero;
	String pathStr = path.ToString();
	if (fs::IsSandboxed()) return IntrinsicResult(fs::GetFileName(pathStr));
#if WINDOWS
	char driveBuf[3];
	char nameBuf[256];
	char extBuf[256];
	_splitpath_s(pathStr.c_str(), driveBuf, sizeof(driveBuf), NULL, 0, nameBuf, sizeof(nameBuf), extBuf, sizeof(extBuf));
	String result = String(nameBuf) + String(extBuf);
#else
	String result(basename((char*)pathStr.c_str()));
#endif
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_dirname(Context context, IntrinsicResult partialResult) {
	Value path = context.GetVar("path");
	if (path.IsNull()) return IntrinsicResult::Zero;
	String pathStr = path.ToString();
	if (fs::IsSandboxed()) {
		// A trailing separator names the same directory, so drop it before
		// taking the parent -- otherwise "/usr/sub/" would report "/usr/sub".
		while (pathStr.LengthB() > 1 && pathStr[pathStr.LengthB()-1] == '/') {
			pathStr = pathStr.SubstringB(0, pathStr.LengthB() - 1);
		}
		return IntrinsicResult(fs::StripFileName(pathStr));
	}
	if (pathStr.LengthB() > 0 && pathStr[pathStr.LengthB()-1] == PATHSEP) {
		pathStr = pathStr.SubstringB(0, pathStr.LengthB() - 1);
	}
#if WINDOWS
	char pathBuf[512];
	_fullpath(pathBuf, pathStr.c_str(), sizeof(pathBuf));
	char driveBuf[3];
	char dirBuf[256];
	_splitpath_s(pathBuf, driveBuf, sizeof(driveBuf), dirBuf, sizeof(dirBuf), NULL, 0, NULL, 0);
	String result = String(driveBuf) + String(dirBuf);
#elif defined(__APPLE__) || defined(__FreeBSD__)
	String result(dirname((char*)pathStr.c_str()));
#else
	char *duplicate = strdup((char*)pathStr.c_str());
	String result(dirname(duplicate));
	free(duplicate);
#endif
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_child(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("parentPath").ToString();
	String filename = context.GetVar("childName").ToString();
	if (fs::IsSandboxed()) return IntrinsicResult(fs::PathCombine(path, filename));
#if WINDOWS
	String pathSep = "\\";
#else
	String pathSep = "/";
#endif
	if (path.EndsWith(pathSep)) return IntrinsicResult(path + filename);
	return IntrinsicResult(path + pathSep + filename);
}

//--------------------------------------------------------------------------------
// Queries
//--------------------------------------------------------------------------------

static IntrinsicResult intrinsic_exists(Context context, IntrinsicResult partialResult) {
	Value path = context.GetVar("path");
	if (path.IsNull()) return IntrinsicResult::Zero;
	return IntrinsicResult(Value::Truth(fs::Exists(path.ToString())));
}

static IntrinsicResult intrinsic_info(Context context, IntrinsicResult partialResult) {
	Value path = context.GetVar("path");
	String pathStr;
	if (!path.IsNull()) pathStr = path.ToString();
	if (pathStr.empty()) pathStr = fs::Cwd();

	fs::FileInfo info;
	if (!fs::GetInfo(pathStr, info)) return IntrinsicResult::Null;

	// The reported path: virtual when sandboxed, so no host layout escapes.
	// Unsandboxed we keep returning the canonical *real* path, which is what
	// this intrinsic has always done and what existing scripts expect.
	String reportedPath;
	if (fs::IsSandboxed()) {
		if (!fs::ResolvePath(pathStr, reportedPath)) return IntrinsicResult::Null;
	} else {
#if WINDOWS
		char pathBuf[512];
		_fullpath(pathBuf, pathStr.c_str(), sizeof(pathBuf));
		reportedPath = String(pathBuf);
#else
		char pathBuf[PATH_MAX];
		if (realpath(pathStr.c_str(), pathBuf) == NULL) reportedPath = pathStr;
		else reportedPath = String(pathBuf);
#endif
	}

	ValueDict map;
	map.SetValue("path", reportedPath);
	map.SetValue("isDirectory", Value::Truth(info.isDirectory));
	map.SetValue("size", Value((double)info.size));
	map.SetValue("date", info.date);
	return IntrinsicResult(DynamicMap(map));
}

//--------------------------------------------------------------------------------
// Mutations
//--------------------------------------------------------------------------------

// File operations return null on success and an error value on failure, as
// in Mini Micro 1 (where the failure was a string).  Messages leave out the
// path, which would be a host path when unsandboxed.
static IntrinsicResult FileResult(const char* who, const String& err) {
	if (err.empty()) return IntrinsicResult::Null;
	return IntrinsicResult(ErrorTypes::FileError(String(who) + ": " + err));
}

static IntrinsicResult intrinsic_mkdir(Context context, IntrinsicResult partialResult) {
	Value path = context.GetVar("path");
	if (path.IsNull()) return FileResult("makedir", "could not create directory");
	return FileResult("makedir", fs::MakeDir(path.ToString()));
}

static IntrinsicResult intrinsic_rename(Context context, IntrinsicResult partialResult) {
	String oldPath = context.GetVar("oldPath").ToString();
	String newPath = context.GetVar("newPath").ToString();
	// overwriteDest: rename(2) replaces an existing destination, and this
	// intrinsic has always done so.
	return FileResult("move", fs::MoveOrCopy(oldPath, newPath, /*deleteSource*/ true, /*overwriteDest*/ true));
}

static IntrinsicResult intrinsic_copy(Context context, IntrinsicResult partialResult) {
	String oldPath = context.GetVar("oldPath").ToString();
	String newPath = context.GetVar("newPath").ToString();
	return FileResult("copy", fs::MoveOrCopy(oldPath, newPath, /*deleteSource*/ false, /*overwriteDest*/ true));
}

static IntrinsicResult intrinsic_remove(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	return FileResult("delete", fs::Delete(path));
}

//--------------------------------------------------------------------------------
// Whole-file reads and writes
//--------------------------------------------------------------------------------

static Value LineValue(const char* bytes, int length) {
	if (length <= 0) return Value::emptyString;
	return Value(String(bytes, (size_t)length));
}

// Split on \n, \r, or \r\n.  A file that does not end with a line terminator
// still has a final line; the previous implementation buffered it and then
// dropped it on the floor.
static void SplitLines(const String& text, ValueList& out) {
	const char* p = text.c_str();
	int len = text.sizeB();
	int start = 0;
	for (int i = 0; i < len; i++) {
		if (p[i] != '\n' && p[i] != '\r') continue;
		out.Add(LineValue(&p[start], i - start));
		if (p[i] == '\r' && i + 1 < len && p[i+1] == '\n') i++;
		start = i + 1;
	}
	if (start < len) out.Add(LineValue(&p[start], len - start));
}

static IntrinsicResult intrinsic_readLines(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	String text;
	if (!fs::ReadText(path, text)) return FileResult("readLines", "cannot open file");
	ValueList list;
	SplitLines(text, list);
	return IntrinsicResult(DynamicList(list));
}

static IntrinsicResult intrinsic_writeLines(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	Value lines = context.GetVar("lines");

	String text;
	if (lines.Type() == ValueType::List) {
		ValueList list = lines.GetList();
		for (int i = 0; i < list.Count(); i++) text += list[i].ToString() + "\n";
	} else {
		text = lines.ToString() + "\n";
	}

	return FileResult("writeLines", fs::WriteText(path, text));
}

static IntrinsicResult intrinsic_loadRaw(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	BinaryData* data = fs::ReadBinary(path);
	if (data == nullptr) return IntrinsicResult::Null;
	return IntrinsicResult(RawDataToValue(data));
}

static IntrinsicResult intrinsic_saveRaw(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	BinaryData* data = ValueToRawData(context.GetVar("data"));
	if (data == nullptr) return FileResult("saveRaw", "data is not a RawData object");
	return FileResult("saveRaw", fs::WriteBinary(path, data->bytes, data->length));
}

//--------------------------------------------------------------------------------
// File handles
//--------------------------------------------------------------------------------

static IntrinsicResult intrinsic_fopen(Context context, IntrinsicResult partialResult) {
	String path = context.GetVar("path").ToString();
	Value modeVal = context.GetVar("mode");
	String mode = modeVal.ToString();

	// This module has always created the file when opening with the default
	// mode, so map those spellings onto fs's "rw+" (read/write, create if
	// missing) rather than its strict "r+" (which requires an existing file).
	if (modeVal.IsNull() || mode.empty()
	    || strcmp(mode.c_str(), "r+") == 0 || strcmp(mode.c_str(), "rw+") == 0) {
		mode = "rw+";
	}

	fs::Resolved resolved;
	if (!fs::Resolve(path, resolved)) return FileResult("open", "could not open file");

	fs::OpenFile* file = new fs::OpenFile(resolved, mode);
	if (!file->error.empty()) {
		delete file;
		return FileResult("open", "could not open file");
	}

	ValueDict instance;
	instance.SetValue(Value::magicIsA, FileHandleClassMap());

	Value fileWrapper = Value::NewHandle(file, [](void* p) { delete (fs::OpenFile*)p; });
	instance.SetValue(_handleKey(), fileWrapper);

	Value result = DynamicMap(instance);
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_fclose(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	// Close is where a handle's writes actually reach the disk, so a script
	// that never closes never persists anything.
	if (file != nullptr) file->Close();
	return IntrinsicResult::Null;
}

static IntrinsicResult intrinsic_isOpen(Context context, IntrinsicResult partialResult) {
	return IntrinsicResult(Value::Truth(LiveFileFor(context.GetVar("self")) != nullptr));
}

static IntrinsicResult intrinsic_fwrite(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	if (file == nullptr) return NotOpenError();
	String data = context.GetVar("data").ToString();
	file->error = String();
	file->Write(data);
	if (!file->error.empty()) return IntrinsicResult::Zero;
	return IntrinsicResult((int)data.sizeB());
}

static IntrinsicResult intrinsic_fwriteLine(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	if (file == nullptr) return NotOpenError();
	String data = context.GetVar("data").ToString();
	file->error = String();
	file->Write(data + "\n");
	if (!file->error.empty()) return IntrinsicResult::Zero;
	return IntrinsicResult((int)data.sizeB() + 1);
}

static IntrinsicResult intrinsic_fread(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	if (file == nullptr) return NotOpenError();
	int count = (int)context.GetVar("codePointCount").IntValue();
	if (count == 0) return IntrinsicResult::EmptyString;
	String result;
	bool ok = (count < 0) ? file->ReadToEnd(result) : file->ReadChars(count, result);
	if (!ok) return IntrinsicResult::Null;
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_freadLine(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	if (file == nullptr) return NotOpenError();
	String result;
	if (!file->ReadLine(result)) return IntrinsicResult::Null;
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_fposition(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	if (file == nullptr) return NotOpenError();
	return IntrinsicResult(Value((double)file->Position()));
}

static IntrinsicResult Seek(Value self, Value pos) {
	fs::OpenFile* file = LiveFileFor(self);
	if (file == nullptr) return NotOpenError();
	file->SetPosition((long)pos.IntValue());
	return IntrinsicResult::Null;
}

static IntrinsicResult intrinsic_fseek(Context context, IntrinsicResult partialResult) {
	return Seek(context.GetVar("self"), context.GetVar("pos"));
}

static IntrinsicResult intrinsic_setPosition(Context context, IntrinsicResult partialResult) {
	return Seek(context.GetVar("self"), context.GetVar("value"));
}

static IntrinsicResult intrinsic_feof(Context context, IntrinsicResult partialResult) {
	fs::OpenFile* file = LiveFileFor(context.GetVar("self"));
	if (file == nullptr) return NotOpenError();
	return IntrinsicResult(Value::Truth(file->IsAtEnd()));
}

//--------------------------------------------------------------------------------
// Sandbox
//--------------------------------------------------------------------------------

// Enter sandbox mode: from here on, only mounted disks are reachable.
//
// Deliberately one-way.  There is no matching intrinsic to leave, and there
// must never be one: a program that could leave the sandbox is not sandboxed.
// A host application calls this once, at the end of its boot script, after it
// has finished setting up.
static IntrinsicResult intrinsic_enterSandbox(Context context, IntrinsicResult partialResult) {
	// Freeze the import search path in the same breath.  `import` resolves
	// against real host directories rather than mounts, so if script could
	// still rewrite env.MS_IMPORT_PATH afterwards it would have a read
	// primitive over every .ms file on the disk.
	FreezeImportPath();
	fs::EnterSandbox();
	return IntrinsicResult::Null;
}

//--------------------------------------------------------------------------------
// Mounting
//--------------------------------------------------------------------------------
//
// Everything here obeys one rule: script may ask for a mount, but never names
// the host target.  file.mountAppData names a subfolder of a root the *host*
// chose; file.mountDropped names an index into files the *user* dropped on the
// window.  Neither takes a host path, and none of these will touch /sys or /hw
// -- userdisks::IsUserMountName is what enforces that, not the callers here.
//
// A path is never returned either: a dropped file is described by its base name
// only, which is exactly what a file picker would show.

static IntrinsicResult intrinsic_mountAppData(Context context, IntrinsicResult partialResult) {
	String mountName = context.GetVar("mountName").ToString();
	String folderName = context.GetVar("folderName").ToString();
	return IntrinsicResult(Value::Truth(userdisks::MountAppData(mountName, folderName)));
}

static IntrinsicResult intrinsic_unmount(Context context, IntrinsicResult partialResult) {
	String mountName = context.GetVar("mountName").ToString();
	return IntrinsicResult(Value::Truth(userdisks::UnmountUserDisk(mountName)));
}

// Files the user has dropped on the window, as maps of {name, isDirectory}.
// Deliberately generic: Mini Micro reads every entry as a disk to mount, but
// another application may want dropped files for something else entirely, so
// the name says "files" rather than "disks".
static IntrinsicResult intrinsic_droppedFiles(Context context, IntrinsicResult partialResult) {
	// Poll here as well as in the main loop, so a script that checks this
	// between frames sees a drop without waiting for the next host tick.
	userdisks::PollDroppedFiles();
	const std::vector<userdisks::DroppedFile>& files = userdisks::DroppedFiles();
	ValueList result;
	for (size_t i = 0; i < files.size(); i++) {
		ValueDict entry;
		entry.SetValue("name", files[i].name);
		entry.SetValue("isDirectory", Value::Truth(files[i].isDirectory));
		result.Add(DynamicMap(entry));
	}
	return IntrinsicResult(DynamicList(result));
}

// Where in the window the last drop landed, as {x, y}.  An application with
// more than one place to put a dropped file has to route on this: a drag comes
// from another application, so no key events have reached us and modifier keys
// read stale at drop time.  Position is the one piece of context every platform
// actually delivers.
static IntrinsicResult intrinsic_dropPosition(Context context, IntrinsicResult partialResult) {
	float x = 0.0f, y = 0.0f;
	userdisks::DropPosition(x, y);
	ValueDict pos;
	pos.SetValue("x", Value(x));
	pos.SetValue("y", Value(y));
	return IntrinsicResult(DynamicMap(pos));
}

static IntrinsicResult intrinsic_mountDropped(Context context, IntrinsicResult partialResult) {
	userdisks::PollDroppedFiles();
	Value index = context.GetVar("index");
	String mountName = context.GetVar("mountName").ToString();
	return IntrinsicResult(Value::Truth(userdisks::MountDropped(index.IntValue(), mountName)));
}

static IntrinsicResult intrinsic_clearDroppedFiles(Context context, IntrinsicResult partialResult) {
	userdisks::ClearDroppedFiles();
	return IntrinsicResult::Null;
}

//--------------------------------------------------------------------------------
// Registration
//--------------------------------------------------------------------------------

static IntrinsicResult intrinsic_FileHandle(Context context, IntrinsicResult partialResult) {
	return IntrinsicResult(FileHandleClassMap());
}

static IntrinsicResult intrinsic_File(Context context, IntrinsicResult partialResult) {
	return IntrinsicResult(fileModuleValue);
}

void AddFileModuleIntrinsics() {
	Intrinsic i;

	// file module

	// Get current working directory
	i = Intrinsic::Create("");
	i.set_Code(&intrinsic_getcwd);
	fileModule.SetValue("curdir", WithNote(i,
		"Get current working directory"));

	// Change current working directory
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_setdir);
	fileModule.SetValue("setdir", WithNote(i,
		"Change the current working directory.  Returns null on success, or an error."));

	// The same change of directory, spelled as an assignment: file.curdir = p.
	// setdir stays too -- this module is Mini Micro's, and there is public code
	// that calls it -- so these two are deliberately the same thing twice.
	//
	// Change current working directory (same as setdir)
	i = Intrinsic::Create("");
	i.AddParam("value");
	i.set_Code(&intrinsic_setCurdir);
	fileModule.SetValue("curdir=", WithNote(i,
		"Change current working directory (same as setdir)"));

	// Get list of file and directory names in the given directory
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_readdir);
	fileModule.SetValue("children", WithNote(i,
		"Get a list of file and directory names in the given directory "
		"(default: the current directory), or an error if it cannot be read"));

	// Get the filename (last path component) of a path string
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_basename);
	fileModule.SetValue("name", WithNote(i,
		"Get the filename (last path component) of a path string"));

	// Get whether a file or directory exists at the given path
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_exists);
	fileModule.SetValue("exists", WithNote(i,
		"Get whether a file or directory exists at the given path"));

	// Get a map of info (path, isDirectory, size, date) about the given path
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_info);
	fileModule.SetValue("info", WithNote(i,
		"Get a map of info (path, isDirectory, size, date) about the given path "
		"(default: the current directory), or null if it does not exist"));

	// Create a directory at the given path
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_mkdir);
	fileModule.SetValue("makedir", WithNote(i,
		"Create a directory at the given path.  Returns null on success, or an error."));

	// Get the parent directory of the given path
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_dirname);
	fileModule.SetValue("parent", WithNote(i,
		"Get the parent directory of the given path"));

	// Combine a parent path and child name into a single path
	i = Intrinsic::Create("");
	i.AddParam("parentPath");
	i.AddParam("childName");
	i.set_Code(&intrinsic_child);
	fileModule.SetValue("child", WithNote(i,
		"Combine a parent path and child name into a single path"));

	// Move (rename) a file or directory
	i = Intrinsic::Create("");
	i.AddParam("oldPath");
	i.AddParam("newPath");
	i.set_Code(&intrinsic_rename);
	fileModule.SetValue("move", WithNote(i,
		"Move (rename) a file or directory, replacing any existing file at newPath.  "
		"Returns null on success, or an error."));

	// Copy a file
	i = Intrinsic::Create("");
	i.AddParam("oldPath");
	i.AddParam("newPath");
	i.set_Code(&intrinsic_copy);
	fileModule.SetValue("copy", WithNote(i,
		"Copy a file, replacing any existing file at newPath.  "
		"Returns null on success, or an error."));

	// Delete a file or empty directory
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_remove);
	fileModule.SetValue("delete", WithNote(i,
		"Delete a file or empty directory.  Returns null on success, or an error."));

	// Open a file; returns a FileHandle, or null on failure
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.AddParam("mode", "r+");
	i.set_Code(&intrinsic_fopen);
	fileModule.SetValue("open", WithNote(i,
		"Open a file; returns a FileHandle, or an error on failure.  The default mode "
		"\"r+\" opens for reading and writing, creating the file if needed."));

	// Read all lines from a text file, returning a list of strings
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_readLines);
	fileModule.SetValue("readLines", WithNote(i,
		"Read all lines from a text file, returning a list of strings "
		"(or an error if the file cannot be read)"));

	// Write a list of strings (or a single string) to a text file
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.AddParam("lines");
	i.set_Code(&intrinsic_writeLines);
	fileModule.SetValue("writeLines", WithNote(i,
		"Write a list of strings (or one string) to a text file, a line each.  "
		"Returns null on success, or an error."));

	// Load a binary file, returning a RawData object
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.set_Code(&intrinsic_loadRaw);
	fileModule.SetValue("loadRaw", WithNote(i,
		"Load a binary file, returning a RawData object (or null on failure)"));

	// Save a RawData object to a binary file
	i = Intrinsic::Create("");
	i.AddParam("path");
	i.AddParam("data");
	i.set_Code(&intrinsic_saveRaw);
	fileModule.SetValue("saveRaw", WithNote(i,
		"Save a RawData object to a binary file.  Returns null on success, or an error."));

	// Enter sandbox mode (one-way; there is no way back)
	i = Intrinsic::Create("");
	i.set_Code(&intrinsic_enterSandbox);
	fileModule.SetValue("enterSandbox", WithNote(i,
		"Enter sandbox mode (one-way; there is no way back)"));

	// Mount a folder of this application's own data as /usr or /usr2
	i = Intrinsic::Create("");
	i.AddParam("mountName", "usr");
	i.AddParam("folderName");
	i.set_Code(&intrinsic_mountAppData);
	fileModule.SetValue("mountAppData", WithNote(i,
		"Mount a folder of this application's own data as /usr or /usr2; "
		"returns true on success"));

	// Unmount /usr or /usr2
	i = Intrinsic::Create("");
	i.AddParam("mountName", "usr");
	i.set_Code(&intrinsic_unmount);
	fileModule.SetValue("unmount", WithNote(i,
		"Unmount /usr or /usr2; returns true on success"));

	// Files the user has dropped on the window: [{name, isDirectory}]
	i = Intrinsic::Create("");
	i.set_Code(&intrinsic_droppedFiles);
	fileModule.SetValue("droppedFiles", WithNote(i,
		"Files the user has dropped on the window: [{name, isDirectory}]"));

	// Where in the window the last drop landed: {x, y}
	i = Intrinsic::Create("");
	i.set_Code(&intrinsic_dropPosition);
	fileModule.SetValue("dropPosition", WithNote(i,
		"Where in the window the last drop landed: {x, y}"));

	// Mount one of those dropped files as /usr or /usr2
	i = Intrinsic::Create("");
	i.AddParam("index", 0);
	i.AddParam("mountName", "usr");
	i.set_Code(&intrinsic_mountDropped);
	fileModule.SetValue("mountDropped", WithNote(i,
		"Mount one of the dropped files (by index into droppedFiles) as /usr or /usr2; "
		"returns true on success"));

	// Forget the dropped files (the next drop clears them anyway)
	i = Intrinsic::Create("");
	i.set_Code(&intrinsic_clearDroppedFiles);
	fileModule.SetValue("clearDroppedFiles", WithNote(i,
		"Forget the dropped files (the next drop clears them anyway)"));

	// FileHandle methods

	// Close the file handle, writing any changes to the disk
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.set_Code(&intrinsic_fclose);
	fileHandleClass.SetValue("close", WithNote(i,
		"Close the file handle, writing any changes to the disk.  "
		"Closing a handle that is already closed does nothing."));

	// Get whether the file handle is still open
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.set_Code(&intrinsic_isOpen);
	fileHandleClass.SetValue("isOpen", WithNote(i,
		"Get whether the file handle is still open"));

	// Write a string to the file
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.AddParam("data");
	i.set_Code(&intrinsic_fwrite);
	fileHandleClass.SetValue("write", WithNote(i,
		"Write a string to the file; returns the number of bytes written (0 on error), "
		"or an error if the file is not open"));

	// Write a string followed by a newline to the file
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.AddParam("data");
	i.set_Code(&intrinsic_fwriteLine);
	fileHandleClass.SetValue("writeLine", WithNote(i,
		"Write a string followed by a newline to the file; returns the number of "
		"bytes written (0 on error), or an error if the file is not open"));

	// Read up to codePointCount characters from the file (or all remaining if -1)
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.AddParam("codePointCount", -1);
	i.set_Code(&intrinsic_fread);
	fileHandleClass.SetValue("read", WithNote(i,
		"Read up to codePointCount characters from the file (or all remaining if -1); "
		"returns null on failure"));

	// Read the next line from the file
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.set_Code(&intrinsic_freadLine);
	fileHandleClass.SetValue("readLine", WithNote(i,
		"Read the next line from the file, or null at the end of the file "
		"(an error if the file is not open)"));

	// Get the current read/write position in the file
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.set_Code(&intrinsic_fposition);
	fileHandleClass.SetValue("position", WithNote(i,
		"Get the current read/write position in the file (an error if the file is not open)"));

	// Move the read/write position within the file
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.AddParam("pos", Value::zero);
	i.set_Code(&intrinsic_fseek);
	fileHandleClass.SetValue("seek", WithNote(i,
		"Move the read/write position within the file (an error if the file is not open)"));

	// The same seek, spelled as an assignment: f.position = n.  seek stays,
	// for the same reason file.setdir does.
	//
	// Move the read/write position within the file (same as seek)
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.AddParam("value", Value::zero);
	i.set_Code(&intrinsic_setPosition);
	fileHandleClass.SetValue("position=", WithNote(i,
		"Move the read/write position within the file (same as seek; an error if the file is not open)"));

	// Get whether the file position is at the end of the file
	i = Intrinsic::Create("");
	i.AddParam("self");
	i.set_Code(&intrinsic_feof);
	fileHandleClass.SetValue("atEnd", WithNote(i,
		"Get whether the file position is at the end of the file (an error if the file is not open)"));

	// Wrap and root both maps now that they are built (see the Values above).
	fileModuleValue = GCManager::NewMapFromDict(fileModule);
	GCManager::AddRoot(fileModuleValue);
	fileHandleClassValue = GCManager::NewMapFromDict(fileHandleClass);
	GCManager::AddRoot(fileHandleClassValue);

	// Register the class under a short name, so str(f) reports
	// {"__isa": FileHandle, ...} rather than dumping every method.
	Intrinsic::AddShortName(FileHandleClassMap(), String("FileHandle"));

	// Register global 'file' and 'FileHandle' intrinsics
	Intrinsic f;
	f = Intrinsic::Create("file");
	f.set_Code(&intrinsic_File);

	f = Intrinsic::Create("FileHandle");
	f.set_Code(&intrinsic_FileHandle);
}

#endif // !PLATFORM_WEB
