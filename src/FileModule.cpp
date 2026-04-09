//
//  FileModule.cpp
//  raylib-miniscript
//
//  File module intrinsics for MiniScript (desktop only).
//  Based on the file module from command-line MiniScript (ShellIntrinsics.cpp).
//

#ifndef PLATFORM_WEB

#include "FileModule.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "RawData.h"
#include "SimpleString.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>

#if _WIN32 || _WIN64
	#define WINDOWS 1
	#include <windows.h>
	#include <Shlwapi.h>
	#include <Fileapi.h>
	#include <direct.h>
	#define getcwd _getcwd
	#define PATHSEP '\\'
#else
	#include <fcntl.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <libgen.h>
	#include <sys/stat.h>
	#include <stdlib.h>
	#if defined(__APPLE__) || defined(__FreeBSD__)
		#include <copyfile.h>
	#else
		#include <sys/sendfile.h>
	#endif
	#define PATHSEP '/'
#endif

using namespace MiniScript;

static Value _handle("_handle");

// RefCountedStorage class to wrap a FILE*
class FileHandleStorage : public RefCountedStorage {
public:
	FileHandleStorage(FILE *file) : f(file) {}
	virtual ~FileHandleStorage() { if (f) fclose(f); }
	FILE *f;
};

// Hidden (unnamed) intrinsics, only accessible via the file module
static Intrinsic *i_getcwd = nullptr;
static Intrinsic *i_chdir = nullptr;
static Intrinsic *i_readdir = nullptr;
static Intrinsic *i_basename = nullptr;
static Intrinsic *i_dirname = nullptr;
static Intrinsic *i_child = nullptr;
static Intrinsic *i_exists = nullptr;
static Intrinsic *i_info = nullptr;
static Intrinsic *i_mkdir = nullptr;
static Intrinsic *i_copy = nullptr;
static Intrinsic *i_readLines = nullptr;
static Intrinsic *i_writeLines = nullptr;
static Intrinsic *i_loadRaw = nullptr;
static Intrinsic *i_saveRaw = nullptr;
static Intrinsic *i_rename = nullptr;
static Intrinsic *i_remove = nullptr;
static Intrinsic *i_fopen = nullptr;
static Intrinsic *i_fclose = nullptr;
static Intrinsic *i_isOpen = nullptr;
static Intrinsic *i_fwrite = nullptr;
static Intrinsic *i_fwriteLine = nullptr;
static Intrinsic *i_fread = nullptr;
static Intrinsic *i_freadLine = nullptr;
static Intrinsic *i_fposition = nullptr;
static Intrinsic *i_feof = nullptr;

// Copy a file.  Return 0 on success, or some value < 0 on error.
static int CopyFileHelper(const char* source, const char* destination) {
#if WINDOWS
	bool success = CopyFile(source, destination, false);
	return success ? 0 : -1;
#elif defined(__APPLE__) || defined(__FreeBSD__)
	int input, output;
	if ((input = open(source, O_RDONLY)) == -1) return -1;
	if ((output = creat(destination, 0660)) == -1) {
		close(input);
		return -1;
	}
	int result = fcopyfile(input, output, 0, COPYFILE_ALL);
	close(input);
	close(output);
	return result;
#else
	String command = String("cp -p \"") + source + "\" \"" + destination + "\"";
	return system(command.c_str());
#endif
}

static IntrinsicResult intrinsic_getcwd(Context *context, IntrinsicResult partialResult) {
	char buf[1024];
	getcwd(buf, sizeof(buf));
	return IntrinsicResult(String(buf));
}

static IntrinsicResult intrinsic_chdir(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	if (path.IsNull()) return IntrinsicResult(Value::zero);
	String pathStr = path.ToString();
	bool ok = false;
	if (!pathStr.empty()) {
		if (chdir(pathStr.c_str()) == 0) ok = true;
	}
	return IntrinsicResult(Value::Truth(ok));
}

static IntrinsicResult intrinsic_readdir(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	String pathStr = path.ToString();
	if (path.IsNull() || pathStr.empty()) pathStr = ".";
	ValueList result;
#if WINDOWS
	pathStr += "\\*";
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(pathStr.c_str(), &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			String name(data.cFileName);
			if (name == "." || name == "..") continue;
			result.Add(name);
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
#else
	DIR *dir = opendir(pathStr.c_str());
	if (dir != NULL) {
		while (struct dirent *entry = readdir(dir)) {
			String name(entry->d_name);
			if (name == "." || name == "..") continue;
			result.Add(name);
		}
		closedir(dir);
	}
#endif
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_basename(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	if (path.IsNull()) return IntrinsicResult(Value::zero);
	String pathStr = path.ToString();
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

static String dirnameHelper(String pathStr) {
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
	return result;
}

static IntrinsicResult intrinsic_dirname(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	if (path.IsNull()) return IntrinsicResult(Value::zero);
	String pathStr = path.ToString();
	if (pathStr.LengthB() > 0 && pathStr[pathStr.LengthB()-1] == PATHSEP) {
		pathStr = pathStr.SubstringB(0, pathStr.LengthB() - 1);
	}
	return IntrinsicResult(dirnameHelper(pathStr));
}

static IntrinsicResult intrinsic_exists(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	if (path.IsNull()) return IntrinsicResult(Value::null);
	String pathStr = path.ToString();
#if WINDOWS
	char pathBuf[512];
	_fullpath(pathBuf, pathStr.c_str(), sizeof(pathBuf));
	WIN32_FIND_DATA FindFileData;
	HANDLE handle = FindFirstFile(pathBuf, &FindFileData);
	bool found = handle != INVALID_HANDLE_VALUE;
	if (found) FindClose(handle);
#else
	bool found = (access(pathStr.c_str(), F_OK) != -1);
#endif
	return IntrinsicResult(Value::Truth(found));
}

static String timestampToString(const struct tm& t) {
	String result = String::Format(1900 + t.tm_year) + "-";
	if (t.tm_mon < 10) result += "0";
	result += String::Format(1 + t.tm_mon) + "-";
	if (t.tm_mday < 10) result += "0";
	result += String::Format(t.tm_mday) + " ";
	if (t.tm_hour < 10) result += "0";
	result += String::Format(t.tm_hour) + ":";
	if (t.tm_min < 10) result += "0";
	result += String::Format(t.tm_min) + ":";
	if (t.tm_sec < 10) result += "0";
	result += String::Format(t.tm_sec);
	return result;
}

static IntrinsicResult intrinsic_info(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	String pathStr;
	if (!path.IsNull()) pathStr = path.ToString();
	if (pathStr.empty()) {
		char buf[1024];
		getcwd(buf, sizeof(buf));
		pathStr = buf;
	}
	struct tm t;
#if WINDOWS
	char pathBuf[512];
	_fullpath(pathBuf, pathStr.c_str(), sizeof(pathBuf));
	struct _stati64 stats;
	if (_stati64(pathBuf, &stats) != 0) return IntrinsicResult(Value::null);
	ValueDict map;
	map.SetValue("path", String(pathBuf));
	map.SetValue("isDirectory", (stats.st_mode & _S_IFDIR) != 0);
	map.SetValue("size", stats.st_size);
	gmtime_s(&t, &stats.st_mtime);
#else
	char pathBuf[PATH_MAX];
	realpath(pathStr.c_str(), pathBuf);
	struct stat stats;
	if (stat(pathStr.c_str(), &stats) < 0) return IntrinsicResult(Value::null);
	ValueDict map;
	map.SetValue("path", String(pathBuf));
	map.SetValue("isDirectory", S_ISDIR(stats.st_mode));
	map.SetValue("size", stats.st_size);
	#if defined(__APPLE__) || defined(__FreeBSD__)
		tzset();
		localtime_r(&(stats.st_mtimespec.tv_sec), &t);
	#else
		localtime_r(&stats.st_mtime, &t);
	#endif
#endif
	map.SetValue("date", timestampToString(t));
	Value result(map);
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_mkdir(Context *context, IntrinsicResult partialResult) {
	Value path = context->GetVar("path");
	if (path.IsNull()) return IntrinsicResult(Value::null);
	String pathStr = path.ToString();
#if WINDOWS
	char pathBuf[512];
	_fullpath(pathBuf, pathStr.c_str(), sizeof(pathBuf));
	bool result = CreateDirectory(pathBuf, NULL);
#else
	bool result = (mkdir(pathStr.c_str(), 0755) == 0);
#endif
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_child(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("parentPath").ToString();
	String filename = context->GetVar("childName").ToString();
#if WINDOWS
	String pathSep = "\\";
#else
	String pathSep = "/";
#endif
	if (path.EndsWith(pathSep)) return IntrinsicResult(path + filename);
	return IntrinsicResult(path + pathSep + filename);
}

static IntrinsicResult intrinsic_rename(Context *context, IntrinsicResult partialResult) {
	String oldPath = context->GetVar("oldPath").ToString();
	String newPath = context->GetVar("newPath").ToString();
	int err = rename(oldPath.c_str(), newPath.c_str());
	return IntrinsicResult(Value::Truth(err == 0));
}

static IntrinsicResult intrinsic_copy(Context *context, IntrinsicResult partialResult) {
	String oldPath = context->GetVar("oldPath").ToString();
	String newPath = context->GetVar("newPath").ToString();
	int result = CopyFileHelper(oldPath.c_str(), newPath.c_str());
	return IntrinsicResult(Value::Truth(result == 0));
}

static IntrinsicResult intrinsic_remove(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
#if WINDOWS
	bool isDir = false;
	struct _stati64 stats;
	if (_stati64(path.c_str(), &stats) == 0) {
		isDir = ((stats.st_mode & _S_IFDIR) != 0);
	}
	bool ok;
	if (isDir) ok = RemoveDirectory(path.c_str());
	else ok = DeleteFile(path.c_str());
	int err = ok ? 0 : 1;
#else
	int err = remove(path.c_str());
#endif
	return IntrinsicResult(Value::Truth(err == 0));
}

static IntrinsicResult intrinsic_fopen(Context *context, IntrinsicResult partialResult);
static ValueDict& FileHandleClass();

static IntrinsicResult intrinsic_fopen(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
	Value modeVal = context->GetVar("mode");
	String mode = modeVal.ToString();
	FILE *handle;
	if (modeVal.IsNull() || mode.empty() || mode == "rw+" || mode == "r+") {
		handle = fopen(path.c_str(), "r+");
		if (handle == NULL) handle = fopen(path.c_str(), "w+");
	} else {
		handle = fopen(path.c_str(), mode.c_str());
	}
	if (handle == NULL) return IntrinsicResult::Null;

	ValueDict instance;
	instance.SetValue(Value::magicIsA, FileHandleClass());

	Value fileWrapper = Value::NewHandle(new FileHandleStorage(handle));
	instance.SetValue(_handle, fileWrapper);

	Value result(instance);
	instance.SetValue(result, fileWrapper);

	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_fclose(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult(Value::zero);
	fclose(handle);
	storage->f = NULL;
	return IntrinsicResult(Value::one);
}

static IntrinsicResult intrinsic_isOpen(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	return IntrinsicResult(Value::Truth(storage->f != NULL));
}

static IntrinsicResult intrinsic_fwrite(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	String data = context->GetVar("data").ToString();
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult(Value::zero);
	size_t written = fwrite(data.c_str(), 1, data.sizeB(), handle);
	return IntrinsicResult((int)written);
}

static IntrinsicResult intrinsic_fwriteLine(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	String data = context->GetVar("data").ToString();
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult(Value::zero);
	size_t written = fwrite(data.c_str(), 1, data.sizeB(), handle);
	written += fwrite("\n", 1, 1, handle);
	return IntrinsicResult((int)written);
}

static String ReadFileHelper(FILE *handle, long bytesToRead) {
	char buf[1024];
	String result;
	while (!feof(handle) && (bytesToRead != 0)) {
		size_t read = fread(buf, 1, bytesToRead > 0 && bytesToRead < 1024 ? bytesToRead : 1024, handle);
		if (bytesToRead > 0) bytesToRead -= read;
		result += String(buf, read);
	}
	return result;
}

static IntrinsicResult intrinsic_fread(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	long bytesToRead = context->GetVar("byteCount").IntValue();
	if (bytesToRead == 0) return IntrinsicResult(Value::emptyString);
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult(Value::zero);
	String result = ReadFileHelper(handle, bytesToRead);
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_fposition(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult::Null;
	return IntrinsicResult(ftell(handle));
}

static IntrinsicResult intrinsic_feof(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult::Null;
	return IntrinsicResult(Value::Truth(feof(handle) != 0));
}

static IntrinsicResult intrinsic_freadLine(Context *context, IntrinsicResult partialResult) {
	Value self = context->GetVar("self");
	Value fileWrapper = self.Lookup(_handle);
	if (fileWrapper.IsNull() or fileWrapper.type != ValueType::Handle) return IntrinsicResult::Null;
	FileHandleStorage *storage = (FileHandleStorage*)fileWrapper.data.ref;
	FILE *handle = storage->f;
	if (handle == NULL) return IntrinsicResult::Null;

	char buf[1024];
	char *str = fgets(buf, sizeof(buf), handle);
	if (str == NULL) return IntrinsicResult::Null;
	for (long i = 0; i < (long)sizeof(buf); i++) {
		if (buf[i] == '\n') {
			buf[i] = 0;
			break;
		}
	}
	String result(buf);
	return IntrinsicResult(result);
}

static IntrinsicResult intrinsic_loadRaw(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
	FILE *handle = fopen(path.c_str(), "rb");
	if (handle == NULL) return IntrinsicResult::Null;

	fseek(handle, 0, SEEK_END);
	long size = ftell(handle);
	fseek(handle, 0, SEEK_SET);

	MiniScript::BinaryData* data = new MiniScript::BinaryData((int)size);
	fread(data->bytes, 1, (size_t)size, handle);
	fclose(handle);

	return IntrinsicResult(MiniScript::RawDataToValue(data));
}

static IntrinsicResult intrinsic_saveRaw(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
	MiniScript::BinaryData* data = MiniScript::ValueToRawData(context->GetVar("data"));
	if (data == nullptr) return IntrinsicResult(String("saveRaw: data is not a RawData object"));

	FILE *handle = fopen(path.c_str(), "wb");
	if (handle == NULL) return IntrinsicResult(String("saveRaw: could not open file for writing: ") + path);

	size_t written = fwrite(data->bytes, 1, (size_t)data->length, handle);
	fclose(handle);

	if ((int)written != data->length) return IntrinsicResult(String("saveRaw: write error"));
	return IntrinsicResult::Null;
}

static IntrinsicResult intrinsic_readLines(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
	FILE *handle = fopen(path.c_str(), "r");
	if (handle == NULL) return IntrinsicResult::Null;

	ValueList list;
	char buf[1024];
	String partialLine;
	while (!feof(handle)) {
		size_t bytesRead = fread(buf, 1, sizeof(buf), handle);
		if (bytesRead == 0) break;
		int lineStart = 0;
		for (int i = 0; i < (int)bytesRead; i++) {
			if (buf[i] == '\n' || buf[i] == '\r') {
				String line(&buf[lineStart], i - lineStart);
				if (!partialLine.empty()) {
					line = partialLine + line;
					partialLine = "";
				}
				list.Add(line);
				if (buf[i] == '\n' && i+1 < (int)bytesRead && buf[i+1] == '\r') i++;
				if (i+1 < (int)bytesRead && buf[i+1] == 0) i++;
				lineStart = i + 1;
			}
		}
		if (lineStart < (int)bytesRead) {
			partialLine = String(&buf[lineStart], bytesRead - lineStart);
		}
	}
	fclose(handle);
	return IntrinsicResult(list);
}

static IntrinsicResult intrinsic_writeLines(Context *context, IntrinsicResult partialResult) {
	String path = context->GetVar("path").ToString();
	Value lines = context->GetVar("lines");

	FILE *handle = fopen(path.c_str(), "w");
	if (handle == NULL) return IntrinsicResult::Null;

	size_t written = 0;
	if (lines.type == ValueType::List) {
		ValueList list = lines.GetList();
		for (int i = 0; i < list.Count(); i++) {
			String data = list[i].ToString();
			written += fwrite(data.c_str(), 1, data.sizeB(), handle);
			written += fwrite("\n", 1, 1, handle);
		}
	} else {
		String data = lines.ToString();
		written = fwrite(data.c_str(), 1, data.sizeB(), handle);
		written += fwrite("\n", 1, 1, handle);
	}

	fclose(handle);
	return IntrinsicResult((int)written);
}

static bool disallowAssignment(ValueDict& dict, Value key, Value value) {
	return true;
}

static ValueDict& FileHandleClass() {
	static ValueDict result;
	if (result.Count() == 0) {
		result.SetValue("close", i_fclose->GetFunc());
		result.SetValue("isOpen", i_isOpen->GetFunc());
		result.SetValue("write", i_fwrite->GetFunc());
		result.SetValue("writeLine", i_fwriteLine->GetFunc());
		result.SetValue("read", i_fread->GetFunc());
		result.SetValue("readLine", i_freadLine->GetFunc());
		result.SetValue("position", i_fposition->GetFunc());
		result.SetValue("atEnd", i_feof->GetFunc());
	}
	return result;
}

static IntrinsicResult intrinsic_FileHandle(Context *context, IntrinsicResult partialResult) {
	return IntrinsicResult(FileHandleClass());
}

static IntrinsicResult intrinsic_File(Context *context, IntrinsicResult partialResult) {
	static ValueDict fileModule;

	if (fileModule.Count() == 0) {
		fileModule.SetValue("curdir", i_getcwd->GetFunc());
		fileModule.SetValue("setdir", i_chdir->GetFunc());
		fileModule.SetValue("children", i_readdir->GetFunc());
		fileModule.SetValue("name", i_basename->GetFunc());
		fileModule.SetValue("exists", i_exists->GetFunc());
		fileModule.SetValue("info", i_info->GetFunc());
		fileModule.SetValue("makedir", i_mkdir->GetFunc());
		fileModule.SetValue("parent", i_dirname->GetFunc());
		fileModule.SetValue("child", i_child->GetFunc());
		fileModule.SetValue("move", i_rename->GetFunc());
		fileModule.SetValue("copy", i_copy->GetFunc());
		fileModule.SetValue("delete", i_remove->GetFunc());
		fileModule.SetValue("open", i_fopen->GetFunc());
		fileModule.SetValue("readLines", i_readLines->GetFunc());
		fileModule.SetValue("writeLines", i_writeLines->GetFunc());
		fileModule.SetValue("loadRaw", i_loadRaw->GetFunc());
		fileModule.SetValue("saveRaw", i_saveRaw->GetFunc());
		// Unlike command-line MiniScript, we will allow assignment
		// to the `file` module, so Soda etc. can add additional
		// file APIs.
		//fileModule.SetAssignOverride(disallowAssignment);
	}

	return IntrinsicResult(fileModule);
}

void AddFileModuleIntrinsics() {
	Intrinsic *f;

	f = Intrinsic::Create("file");
	f->code = &intrinsic_File;

	f = Intrinsic::Create("FileHandle");
	f->code = &intrinsic_FileHandle;

	i_getcwd = Intrinsic::Create("");
	i_getcwd->code = &intrinsic_getcwd;

	i_chdir = Intrinsic::Create("");
	i_chdir->AddParam("path");
	i_chdir->code = &intrinsic_chdir;

	i_readdir = Intrinsic::Create("");
	i_readdir->AddParam("path");
	i_readdir->code = &intrinsic_readdir;

	i_basename = Intrinsic::Create("");
	i_basename->AddParam("path");
	i_basename->code = &intrinsic_basename;

	i_dirname = Intrinsic::Create("");
	i_dirname->AddParam("path");
	i_dirname->code = &intrinsic_dirname;

	i_child = Intrinsic::Create("");
	i_child->AddParam("parentPath");
	i_child->AddParam("childName");
	i_child->code = &intrinsic_child;

	i_exists = Intrinsic::Create("");
	i_exists->AddParam("path");
	i_exists->code = &intrinsic_exists;

	i_info = Intrinsic::Create("");
	i_info->AddParam("path");
	i_info->code = &intrinsic_info;

	i_mkdir = Intrinsic::Create("");
	i_mkdir->AddParam("path");
	i_mkdir->code = &intrinsic_mkdir;

	i_rename = Intrinsic::Create("");
	i_rename->AddParam("oldPath");
	i_rename->AddParam("newPath");
	i_rename->code = &intrinsic_rename;

	i_copy = Intrinsic::Create("");
	i_copy->AddParam("oldPath");
	i_copy->AddParam("newPath");
	i_copy->code = &intrinsic_copy;

	i_remove = Intrinsic::Create("");
	i_remove->AddParam("path");
	i_remove->code = &intrinsic_remove;

	i_fopen = Intrinsic::Create("");
	i_fopen->AddParam("path");
	i_fopen->AddParam("mode", "r+");
	i_fopen->code = &intrinsic_fopen;

	i_fclose = Intrinsic::Create("");
	i_fclose->code = &intrinsic_fclose;

	i_isOpen = Intrinsic::Create("");
	i_isOpen->code = &intrinsic_isOpen;

	i_fwrite = Intrinsic::Create("");
	i_fwrite->AddParam("data");
	i_fwrite->code = &intrinsic_fwrite;

	i_fwriteLine = Intrinsic::Create("");
	i_fwriteLine->AddParam("data");
	i_fwriteLine->code = &intrinsic_fwriteLine;

	i_fread = Intrinsic::Create("");
	i_fread->AddParam("byteCount", -1);
	i_fread->code = &intrinsic_fread;

	i_freadLine = Intrinsic::Create("");
	i_freadLine->code = &intrinsic_freadLine;

	i_feof = Intrinsic::Create("");
	i_feof->code = &intrinsic_feof;

	i_fposition = Intrinsic::Create("");
	i_fposition->code = &intrinsic_fposition;

	i_readLines = Intrinsic::Create("");
	i_readLines->AddParam("path");
	i_readLines->code = &intrinsic_readLines;

	i_writeLines = Intrinsic::Create("");
	i_writeLines->AddParam("path");
	i_writeLines->AddParam("lines");
	i_writeLines->code = &intrinsic_writeLines;

	i_loadRaw = Intrinsic::Create("");
	i_loadRaw->AddParam("path");
	i_loadRaw->code = &intrinsic_loadRaw;

	i_saveRaw = Intrinsic::Create("");
	i_saveRaw->AddParam("path");
	i_saveRaw->AddParam("data");
	i_saveRaw->code = &intrinsic_saveRaw;
}

#endif // !PLATFORM_WEB
