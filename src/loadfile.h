//
// loadfile.h
// raylib-miniscript
//
// raylib LoadFileData/LoadFileText hooks (web only)
//

#ifndef LOADFILE_H
#define LOADFILE_H

#ifdef PLATFORM_WEB
// Install LoadFileData/LoadFileText callbacks.  Each looks in the in-memory file
// system first (an Emscripten file package preloaded by the page) and fetch()es
// only when no package is loaded.
void InstallLoadFileHooks();

// True when a file package is loaded, i.e. the in-memory file system has an
// "assets" directory.  Decided once; after that, a file missing from the
// package is simply missing and costs no network request.
bool WebPackageLoaded();

// Read a file from the in-memory file system.  Returns a malloc'd buffer (one
// extra NUL byte after the data, so it also serves as a C string) and sets
// *size, or returns nullptr if there is no such regular file.
unsigned char* ReadPackagedFile(const char* path, int* size);
#endif

#endif
