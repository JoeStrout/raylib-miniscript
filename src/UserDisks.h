//
//  UserDisks.h
//  raylib-miniscript
//
//  Policy for the user disks, /usr and /usr2: which one is mounted, how it got
//  that way, and how it is remembered across runs.  The fs layer (FileSystem.h)
//  is the mechanism -- mount table, resolver, latch -- and knows nothing about
//  any of this.
//
//  Three ways a user disk gets mounted, and the rule they all obey:
//
//    1. At boot, from preferences or a command-line argument.  Host decisions,
//       made before any script runs.
//    2. By drag and drop.  A drop *is* the user naming a target, exactly as a
//       file picker would be, so script may mount a dropped file -- by index,
//       never by path.  Script sees base names only.
//    3. By script, into this application's own data directory, naming a
//       subfolder within it but never the root.  A Soda game storing save data
//       is the case this exists for.
//
//  In none of them does script supply a host path.  That is the guarantee in
//  notes/SANDBOXING.md ("script may request a mount but never names the
//  target"), and it is why there is no MountHostPath in the script-facing API
//  below even though the host uses one.
//
//  Desktop only.
//

#ifndef USERDISKS_H
#define USERDISKS_H

#ifndef PLATFORM_WEB

#include "miniscript.h"
#include <vector>

namespace userdisks {

using MiniScript::String;

//--------------------------------------------------------------------------------
// Boot
//--------------------------------------------------------------------------------

// Options a host application ships in its payload, read from
// <payload dir>/hostopts.txt -- the same key=value format as the prefs file:
//
//   appId=Mini Micro        names the app data directory and the prefs file
//   defaultDisk=Mini Micro  folder to create under ~/Documents and mount as
//                           /usr on first run; absent means "create nothing"
//
// It is part of the app payload, chosen by whoever built the application, and
// is read once at boot, long before any script runs.  Stock raylib-miniscript
// and Soda ship no such file, get no appId, and create no disk.
void LoadHostOptions(const String& payloadDir);

// Mount /usr and /usr2 as boot decided: a -usr / -usr2 argument if given,
// otherwise what preferences remember, otherwise (for /usr only) the default
// disk if the host asked for one.  Call after LoadHostOptions and after the
// payload mounts, and before the boot script runs.
//
// Command-line arguments are deliberately *not* written to preferences: they
// are a testing and scripting affordance, not a change to what the user chose.
void MountAtBoot();

// Command line, parsed by main before anything else here.  Unrecognized
// arguments are left alone; the first one that is not ours is the script path.
struct Args {
	String usrPath;         // -usr <path>
	String usr2Path;        // -usr2 <path>
	bool ignorePrefs = false;   // --ignore-prefs
	String scriptPath;      // first non-option argument
};
Args ParseArgs(int argc, char* argv[]);
void SetArgs(const Args& args);

//--------------------------------------------------------------------------------
// Mounting
//--------------------------------------------------------------------------------

// The only mount names any of this will touch.  /sys and /hw are the host's,
// established at boot from the app payload, and nothing here -- and therefore
// nothing script can reach -- may replace or unmount them.
bool IsUserMountName(const String& name);

// Mount a host directory (later: or .minidisk archive) as /usr or /usr2.
// Host-only: this is the one function that takes a host path, and it is never
// exposed to script in any form.  `remember` writes the choice to preferences,
// which is right for a user action (a drop, a picker) and wrong for a boot-time
// restore or a command-line argument.
bool MountHostPath(const String& mountName, const String& hostPath, bool remember);

// Mount <app data dir>/<subfolder> as /usr or /usr2, creating it if needed.
// subfolder must be a single path component: no separators, no "..", no leading
// dot.  Script may call this, and it stays safe because the *root* is chosen by
// the host -- the worst a runaway program can do is store data in a folder of
// this application's own.
bool MountAppData(const String& mountName, const String& subfolder);

// Unmount and forget.  Safe to expose to script (for user mounts only): losing
// a disk is recoverable, and a program that unmounts one is merely obnoxious.
bool UnmountUserDisk(const String& mountName);

//--------------------------------------------------------------------------------
// Dropped files
//--------------------------------------------------------------------------------

// raylib hands us dropped paths only *after* the drop completes -- GLFW has no
// drag-enter or drag-over event on any platform, and accepts every drop
// unconditionally -- so there is no way to refuse a file mid-drag.  A file that
// turns out not to be mountable is reported afterwards instead, which is what
// Mini Micro 1 does too.
struct DroppedFile {
	String name;            // base name only; the host path stays in here
	bool isDirectory = false;
	String hostPath;        // never leaves this module
};

// Move any newly dropped files into the queue, replacing whatever was there.
// Called from the main loop and from the script-facing entry points, so a
// script that only polls occasionally still sees a drop promptly.
void PollDroppedFiles();

const std::vector<DroppedFile>& DroppedFiles();

// Where in the window the drop happened, in pixels, as of the last drop.
//
// This is the only thing besides the file list that a drop tells us, and it is
// what an application must route on when it has more than one place to put a
// dropped file.  **Modifier keys are not available at drop time**: the drag
// comes from another application, so no key events have reached this window and
// whatever GLFW last saw is stale.  Position, by contrast, every platform
// delivers -- GLFW's macOS, Win32, and X11 backends all set the cursor position
// from the drop itself.
void DropPosition(float& outX, float& outY);

// Mount queued drop `index` as /usr or /usr2.  Script names the index and the
// mount; the host path comes from the queue, so script never sees or supplies
// one.  This is a user action, so it is remembered in preferences.
bool MountDropped(int index, const String& mountName);

// Drop the queue.  A path dropped ten minutes ago should not still be mountable
// by a program that has only just got around to looking, so the queue is also
// cleared implicitly by the next drop.
void ClearDroppedFiles();

} // namespace userdisks

#endif // !PLATFORM_WEB

#endif // USERDISKS_H
