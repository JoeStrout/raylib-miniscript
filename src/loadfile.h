//
// loadfile.h
// raylib-miniscript
//
// raylib LoadFileData/LoadFileText hooks (web only)
//

#ifndef LOADFILE_H
#define LOADFILE_H

#ifdef PLATFORM_WEB
// Install LoadFileData/LoadFileText callbacks for web fetch
void InstallLoadFileHooks();
#endif

#endif
