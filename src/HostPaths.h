//
//  HostPaths.h
//  raylib-miniscript
//
//  Where this application keeps things on the host: its preferences, its
//  application-support directory, and the user's Documents folder.  raylib
//  offers nothing here, so these are the platform conventions spelled out.
//
//  Host-only.  Nothing in here is reachable from script, and none of these
//  paths is ever mounted as-is -- the prefs file lives *outside* every mount,
//  which is what keeps a sandboxed program from rewriting its own mount table
//  by editing preferences.
//
//  Desktop only: the web build has no host file system to speak of.
//

#ifndef HOSTPATHS_H
#define HOSTPATHS_H

#ifndef PLATFORM_WEB

#include "miniscript.h"

namespace hostpaths {

using MiniScript::String;

// The application's identity, which names its application-support directory and
// therefore its preferences.  Mini Micro 2 and a Soda game must not share
// either, so this is per-application rather than per-engine.
//
// Defaults to the executable's base name; a host application overrides it by
// setting `appId` in its hostopts file (see UserDisks.h).  Setting it after
// anything has read a preference would silently split the prefs file in two, so
// it is set once, during boot, before the first PrefsPath() call.
void SetAppId(const String& appId);
const String& AppId();

// The user's home directory.  Empty if the platform will not say.
String HomeDir();

// Per-user, per-application storage for data the user does not manage directly:
//   macOS    ~/Library/Application Support/<appId>
//   Linux    $XDG_DATA_HOME/<appId>, or ~/.local/share/<appId>
//   Windows  %APPDATA%\<appId>
// Not created by this call; see EnsureDir.
String AppDataDir();

// Where preferences live: prefs.txt inside AppDataDir.  Putting it there rather
// than in ~/Library/Preferences avoids the macOS plist machinery (and its
// caching daemon, which does not expect a file edited behind its back).
String PrefsPath();

// The user's documents folder -- a place they *do* manage directly, which is
// why the default user disk goes here rather than into application support.
//   macOS/Windows  ~/Documents
//   Linux          XDG_DOCUMENTS_DIR from ~/.config/user-dirs.dirs, else ~/Documents
// Falls back to the home directory if there is no Documents folder at all.
String DocumentsDir();

// mkdir -p.  True if the directory exists when we are done, whether or not we
// were the ones who made it.
bool EnsureDir(const String& path);

// True if path names an existing directory.
bool IsDirectory(const String& path);

// True if path names something that exists (file or directory).
bool Exists(const String& path);

} // namespace hostpaths

#endif // !PLATFORM_WEB

#endif // HOSTPATHS_H
