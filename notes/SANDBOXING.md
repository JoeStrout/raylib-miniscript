# Sandboxing the File System

A plan for a virtual, sandboxed file system in **raylib-miniscript**, which
Mini Micro 2 needs and Soda can ignore.  (Eventually Summoner will use sandboxing
as well, with slightly different semantics we will address later.)

## The guarantee

In Mini Micro, code runs in a sandbox. Paths are virtual, always `/`-separated
(on every platform), and name one of:

| Mount   | Contents                          | Access    |
|---------|-----------------------------------|-----------|
| `/sys`  | the system disk, shipped with the app | read-only |
| `/usr`  | a user disk, mounted by the user  | read-write |
| `/usr2` | a second user disk                | read-write |

No runaway or malicious program can read or write anything else. Crucially,
**a program cannot choose what gets mounted** — only the user can, through a
GUI. Mini Micro 2 must keep this, and Mini Micro is usable with `/usr` and
`/usr2` unmounted, so there is no "default writable directory" to arrange.

Any of these disks may be either a part of the host file system (i.e. a specific host folder), or may be a Zip file (often with a ".minidisk" extension) acting as a complete disk.

## Why this has to be in the host

Shadowing the file intrinsics from MiniScript does not work: `intrinsics.file`
hands back the originals. Nor is there an in-language privilege boundary to
build on — Mini Micro's shell and the user's program share one interpreter, so
anything the shell can call, a runaway program can call.

The boundary has to be C++, and it has to be *below* the intrinsic bindings, so
that reaching the original function gains nothing.

## Architecture

### One chokepoint

A new `fs` layer owning the mount table and one resolution function. Every
path-taking intrinsic goes through it; nothing else in the host touches a
script-supplied path. The rest of this document is mostly about making sure
nothing bypasses it.

### A one-way latch

`fs.enterSandbox` sets a static flag that nothing can clear — there is
deliberately no intrinsic to leave sandbox mode. Before the flip, during boot,
the host is unrestricted and establishes its mounts; after it, every path
argument from script is virtual.

Off by default, so stock raylib-miniscript and Soda are unaffected.

### Mounts are backends, not path prefixes

A mount cannot be "a real directory to prepend," because several mounts are not
directories at all:

- a real directory (desktop `/sys`, `/hw`, and a folder mounted as `/usr`)
- a `.minidisk` file, which is a zip — mountable as `/usr` or `/usr2` on
  **desktop as well as web**, and **writable** on desktop
- IndexedDB-backed storage (web `/usr2`; persistent writable storage is a new
  Mini Micro 2 feature)

So a mount implements a small interface — read bytes, write bytes, list a
directory, stat, make directory, delete, rename, close — plus a **read-only
flag**.

That interface already exists and is proven: Mini Micro 1's `Disk` abstract
class, in `Assets/Scripts/Disk/Disk.cs`. Its subclasses are exactly the backends
we need — `RealFileDisk`, `ZipDisk` (writable, from a file), `ReadOnlyZipDisk`
(from a byte array), `WebURLZipDisk`. Mirror it rather than inventing one; the
method set has already survived contact with real Mini Micro use.

Writable zip mounts are a requirement, and `ZipDisk.cs` shows the approach:
**save on every mutation, immediately.** Each of `WriteText`, `WriteBinary`,
`MakeDir`, `Delete`, and `Rename` writes the archive out and then reopens it.
DotNetZip's `Save()` over the file it was read from goes through a temporary
file and replaces the original, so an interrupted save cannot destroy the disk;
whatever zip library we pick must do the same, or we do it ourselves.

There is no dirty-buffering or periodic-flush scheme, and we should not invent
one: saving per operation means no data-loss window, and Mini Micro has run this
way for years. The cost is rewriting the archive on each write, which is fine
for the small disks people actually use — revisit only if large `.minidisk`
files turn out to be common.

One interop detail from `ZipDisk.CreateImpliedFolders`: many zip utilities omit
entries for directories. Mini Micro synthesizes the missing ones when opening an
archive, and our backend needs to do the same or directory listings will be
wrong for any disk built by another tool.

Getting this right at the start matters more than anything else here. If
resolution returns a real path, zip and IndexedDB mounts can never work, and
finding that out after every call site is written means rewriting all of them.

### The real-path fast path

Raylib's loaders (`LoadTexture`, `LoadSound`, `LoadFont`, ...) want a real file
on disk. So a backend may *optionally* offer "here is a real path for this
file." The real-directory backend does; zip and IndexedDB decline, and those
loaders fall back to raylib's memory variants (`LoadImageFromMemory`,
`LoadFontFromMemory`, `LoadMusicStreamFromMemory`, and so on) using bytes from
the backend.

Desktop real-directory mounts thus stay as cheap as string translation, without
the interface assuming that case.

### Path resolution rules

Each of these has bitten somebody's sandbox before:

1. Reject `\` as a separator. The separator is `/`, always, on every platform.
2. Fold `.` and `..` **lexically, on the virtual path, before** touching the
   real file system. A `..` must never reach the host.
3. After mapping to a real path, `realpath()` it and confirm it is still under
   the `realpath()`'d mount root. Without this, a symlink inside `/usr` walks
   straight out. Canonicalize each mount root once, at mount time — which also
   handles the development `sys` symlink into the minimicro-sysdisk checkout.
4. Enforce the read-only flag on *every* mutating operation, not just opening
   for write: `delete`, `move`, `makedir`, `saveRaw`, and copy destinations.
5. Never let a real path back into script. `file.curdir`, `file.info().path`,
   and every error message must speak virtual paths, or the host layout leaks —
   including the user's home directory name.
6. A sandbox violation is indistinguishable from any other invalid path. There
   is no distinct "permission denied": `/etc/passwd` and `/nope` both simply do
   not exist, `file.exists` is false for both, and opening either fails the same
   way. A distinguishable error would let a program map the host file system by
   probing.

The current working directory is virtual too, owned by the `file` module.

## Mount points

`/sys` and `/hw` are mounted at boot, before the latch. `/usr` and `/usr2` are
whatever the user last had mounted (remembered in a prefs file), or nothing.

### `/hw` — the hardware disk

Mini Micro's own resources — the screen font, the bezel, the sticker, the boot
chime — are loaded through the same raylib loaders as everything else, but they
are not part of the virtual file system the user sees. Rather than carve out an
exception for them, mount them: a **hidden, read-only** mount at `/hw`, sourced
from the app payload.

Hidden means a `listed` flag on the mount, false here, so `file.children("/")`
returns exactly `/sys`, `/usr`, `/usr2` and Mini Micro 1 code that enumerates
the root sees what it expects. In every other respect `/hw` is an ordinary
read-only mount: script can read it, list it, and load from it. It is
undocumented, not secret; nothing breaks if a user finds it.

Each disk is a **named subdirectory** of the boot script's directory — `hw/`,
`sys/` — and never that directory itself. Mounting the script's own directory
would publish the boot script and the whole library tree on a disk any program
can read; a host application should choose what it exposes, one folder at a
time. Whatever sits directly beside those subdirectories is unreachable, and
cannot be reached by climbing out of one either, since `..` is folded before any
mount is consulted.

Keeping it separate from `/sys` is deliberate. `/sys` comes from the
minimicro-sysdisk repo, on its own release cycle; `/hw` versions with the
executable. Merging them means either putting host assets in minimicro-sysdisk
or building a union mount, which brings name-collision rules nobody wants.

Note that boot-time preloading would *mostly* work instead — resources loaded
before the latch become GPU handles, which survive it. It is not enough, and
should not be the mechanism: it holds only until something loads a hardware
resource lazily. `ScreenFontLarge.png` sits in the payload already, waiting to
break after lockdown in a path nobody tests.

### Mounting stays the user's

A `mount(name, realPath)` intrinsic would give the whole thing away — malicious
code just mounts `/`. The rule that preserves the guarantee is that **script may
request a mount but never names the target**. Three things satisfy that rule:

- **A file picker**: an intrinsic that opens a *native* directory/file picker
  and mounts whatever the user chooses. Code can pester the user with a dialog;
  it cannot pick a directory. This is the same guarantee Mini Micro 1 gets from
  its Unity file dialog.
- **Drag and drop**: a drop *is* the user naming a target, exactly as a picker
  is. Script mounts a dropped file by index and sees only its base name.
- **An app-data mount**: script names a subfolder of a root the *host* chose
  (this application's own data directory), never the root and never a path. The
  worst a runaway program can do with it is store data in a folder of the
  application's own.

Mount targets are restricted to `/usr` and `/usr2` throughout. `/sys` and `/hw`
are the host's, established at boot from the app payload, and nothing
script-reachable may replace or unmount them.

Unmounting is safe to expose directly, for user disks.

## Entry points

### Route through the resolver

- The `file` module: 32 intrinsics including the `FileHandle` methods.
- Raylib loaders taking a file name: `LoadTexture`, `LoadImage`, `LoadSound`,
  `LoadMusicStream`, `LoadFont`, `LoadFontEx`, `LoadShader`, `LoadFileText`,
  `LoadFileData`, `SaveFileText`, `SaveFileData`, `ExportImage`,
  `ExportImageAsCode`.

`LoadFileText`/`LoadFileData` and `SaveFileText`/`SaveFileData` deserve
emphasis: today they are unrestricted read and write primitives over raw host
paths. Resolution happens in the binding layer, so raylib itself never sees a
virtual path.

### Reject when sandboxed

- `TakeScreenshot` — writes to the real working directory; raylib refuses paths
  in it anyway.
- `ChangeDirectory`, `GetWorkingDirectory` — the virtual cwd belongs to the
  `file` module; these leak real paths and desync it.
- `SetLoadFileDataCallback`, `SetSaveFileDataCallback`,
  `SetLoadFileTextCallback`, `SetSaveFileTextCallback` — these let script
  replace raylib's file I/O *beneath* the resolver.

### `import`

`import` already refuses a `/` in the library name, but `env` is a writable map,
so `env.MS_IMPORT_PATH = "/etc"` followed by `import "passwd"` reads any `*.ms`
on the disk. Narrow, but an escape. Freeze `MS_IMPORT_PATH`, `MS_EXE_DIR`, and
`MS_SCRIPT_DIR` at lockdown, or resolve import directories through the same
function.

### `http`

The `http` module must reject the `file:` protocol — otherwise it is a read
primitive over the whole disk. A protocol allow-list (`http`, `https`) is
better than a `file:` deny-list.

Note that HTTP remains an exfiltration channel regardless; Mini Micro 1 allows
it too, so this is accepted rather than solved. `OpenURL` hands a string to the
system browser and raylib's own documentation warns against passing it anything
untrusted — reject it under sandbox.

### Confirmed absent

`main.cpp` registers only the raylib, file, "more", and http intrinsics, so
MiniScript 2's `ShellIntrinsics` — with its `exec` — never reaches the VM.
Worth a comment there so it stays that way.

## Implementation order

1. **The `fs` layer**: mount table, backend interface, resolver, latch. Real
   directory backend only. No script-visible mounting.
2. **Mount `/hw` and `/sys` read-only at boot; in Mini Micro 2, we willlatch
   at the end of `main.ms`.**
   `/usr` and `/usr2` unmounted — a usable machine. Mini Micro's own resource
   paths move to `/hw/...`, which also fixes their current
   working-directory-relative form, already broken for a packaged app launched
   from the Finder.
3. **Route the `file` module** through the resolver; virtual cwd; read-only
   enforcement; make sure no real path escapes in a return value or an error.
4. **Route the raylib loaders**, and reject the entry points listed above.
   `import` and `http` hardening lands here too.
5. **User disks**: `/usr` and `/usr2` become mountable (to real directories) —
   remembered across runs in a prefs file, mountable by drag and drop, and
   mountable by script into this application's own data directory. A native
   picker last, since the other three cover the real cases.
6. **Zip backend**, read and write, so a `.minidisk` can be mounted as `/usr`
   or `/usr2`.
7. **Web**: `/usr` from a `.minidisk` prepared next to the web build; `/usr2`
   backed by IndexedDB. Note the file module is desktop-only today.

Steps 1 and 2 are the ones that shake out path-translation bugs while escapes
are still cheap to fix.

### Status

Steps 1 through 5 are done, except the file picker (see "Step 5 notes"), which
is deliberately last.

Steps 1 through 4 (`src/FileSystem.{h,cpp}`, `tests/fs_tests.cpp`,
`src/FileModule.cpp`, the raylib binding files, `src/MoreIntrinsics.cpp`,
`src/HttpModule.cpp`); step 5 (`src/HostPaths.{h,cpp}`, `src/UserDisks.{h,cpp}`,
`src/main.cpp`, `assets/diskdrop.ms`).

Step 2 had to pull the *routing* half of step 4 forward: moving Mini Micro's
resource paths to `/hw/...` is impossible unless the raylib loaders understand
`/hw`. So these now resolve through `fs::HostPath`: `LoadImage`, `LoadTexture`,
`LoadImageAnim`, `LoadImageRaw`, `ExportImage`, `ExportImageAsCode`, `LoadWave`,
`LoadSound`, `LoadMusicStream`, `LoadFont`, `LoadFontEx`, `LoadShader`,
`LoadFileText`, `LoadFileData`, `SaveFileData`, `SaveFileText`,
`ExportDataAsCode`.

The host mounts `<boot script dir>/hw` and `<boot script dir>/sys`, each only if
present, and says nothing if neither is — so a plain raylib-miniscript or Soda
app never acquires a disk it did not ask for.

One design point changed along the way: **mounts resolve before the latch, not
only after it.** They have to — a host application loads its own resources from
`/hw` during boot, before it latches. So the latch's only effect is to remove
the fallback to the host file system for paths that name no mount. That is a
smaller and much easier thing to reason about than two resolution modes.

### Step 5 notes

The mechanism (mount table, backends, resolver) stayed in `fs`; everything about
*which* disk is in the drive and *why* went into a new `userdisks` module, with
`hostpaths` under it for the platform conventions raylib does not provide.

**Preferences** live in `<app data dir>/prefs.txt`, a key=value file with two
keys, `usr` and `usr2`, each the canonical host path of what is mounted there.
Written when the user mounts something, removed when they unmount it — Mini
Micro 1's behavior from `DiskManager.cs`, and what makes "unmount, quit,
relaunch" mean what a user expects. A remembered path that fails to mount is
*kept*: an ejected USB drive should come back next time.

The app data directory is per-*application*, not per-engine, so Mini Micro 2 and
a Soda game never share preferences. That means the engine has to be told who it
is, which is what **`hostopts.txt`** in the app payload is for:

```
appId=Mini Micro
defaultDisk=Mini Micro
```

`appId` names the app data directory; `defaultDisk` names a folder to create
under `~/Documents` and mount as `/usr` on first run. It ships beside `hw/` and
`sys/`, is read once at boot before any script runs, and is absent for stock
raylib-miniscript and Soda — which therefore get no app identity and no disk.

One deliberate divergence from Mini Micro 1: when the remembered `/usr` is gone,
MS1 creates a fresh disk at that path. We fall back to the default disk instead.
Recreating in place either fails confusingly or leaves a stray disk on a drive
the user is about to remount.

`-usr <path>`, `-usr2 <path>`, and `--ignore-prefs` are parsed by `main`; the
first argument that is not one of those is the script path, as before. Command
line mounts are **not** written to preferences: they are a testing affordance,
not a change to what the user chose.

**Drag and drop** turned out to be the cheapest of the three mounting routes and
the most useful, so it landed before the picker. The host drains raylib's
dropped-file state each tick into a queue; script sees base names and an
`isDirectory` flag through `file.droppedFiles`, and mounts by index with
`file.mountDropped`. A new drop replaces the queue, and `file.clearDroppedFiles`
empties it, so a path dropped ten minutes ago cannot still be mounted by a
program that has only just looked.

The name is `droppedFiles`, not `droppedDisks`: Mini Micro reads every entry as
a disk, but another raylib-miniscript game may want dropped files for importing
a model or a texture, and that will want a different handler over the same
queue.

There is **no drag-time accept/reject callback, and cannot be one** without
going below raylib. GLFW exposes only `glfwSetDropCallback`, which fires after
the drop completes, and its platform code accepts every drop unconditionally:
`NSDragOperationGeneric` on macOS, `DragAcceptFiles` + `WM_DROPFILES` on Windows
(the refusable path is `IDropTarget`, which GLFW does not implement), and XDND
on X11 answered internally without consulting the app. A file that turns out not
to be mountable is therefore reported *after* the drop, which is what Mini Micro
1 does anyway (`ShowFeedback("Not a mountable disk")`).

**Modifier keys are not available at drop time either**, which is worth knowing
before designing any UI around a drop. The first cut of `diskdrop.ms` used
shift-drop to choose `/usr2` and it silently always chose `/usr`: the drag comes
from another application, so this window has received no key events and GLFW's
key state is whatever it last saw. There is no portable fix — Win32's
`WM_DROPFILES` carries no modifiers at all, and X11's XDND modifier state is not
forwarded by GLFW.

**Drop position is available**, and is the thing to route on. Every GLFW backend
sets the cursor position from the drop before delivering the file list:
`performDragOperation` on macOS, `DragQueryPoint` on Windows, `XdndPosition` on
X11. `userdisks` captures it when it drains the queue and `file.dropPosition`
reports it as `{x, y}`, so `diskdrop.ms` now routes by which half of the window
received the drop. Mini Micro 2 will want the same thing against its disk-slot
UI.

**`file.mountAppData(mountName, folderName)`** is the third route, and the one
for a Soda game keeping save data. `folderName` must be a single path component
— the check in `MountAppData`, not the caller's good intentions, is what keeps
the mount inside our own app data directory. It is allowed after the latch:
it cannot escape the sandbox, and a program that swaps the user's disk out from
under them is obnoxious rather than dangerous — it could already just call
`file.unmount`.

`userdisks::IsUserMountName` gates every script-reachable mount and unmount to
`usr`/`usr2`. Without it, `file.mountAppData("sys", ...)` would let a program
replace the system disk with a folder it controls, and `file.unmount("hw")`
would take Mini Micro's own resources away mid-run.

`Backend::SourcePath()` was added so preferences can record what a mount came
from. It is host-only and must stay that way: it returns a real host path, and
rule 5 says none of those reach script.

**Writing through the file module while sandboxed is now exercised** — it was
the outstanding item from steps 3 and 4, and it works: `writeLines`,
`readLines`, `file.open`/`write`/`close`, `makedir`, `delete`, and `children`
all round-trip against a mounted `/usr`, with `/usr/../../etc/passwd` still
rejected.

`assets/diskdrop.ms` is the manual test: drop a folder on the left half of the
window to mount it as `/usr`, on the right half for `/usr2`; `U` and `I`
unmount.

Still to do here: the native picker. tinyfiledialogs is still the plan, and the
reason it is tolerable despite Mini Micro 1's Linux crash history is that it
shells out — `osascript` on macOS, `zenity`/`kdialog` on Linux — so a toolkit
crash kills a helper process, not us. Windows uses in-process `comdlg32`. It
blocks the main loop while open (so did MS1), and on a machine with no dialog
helper installed there is simply no dialog, which is why drag and drop needs to
remain a first-class route rather than a convenience.

### Step 4 notes

**The reject list turned out to be much longer than this document assumed.**
Beyond the loaders, RCore exposes a whole `File*`/`Directory*` family —
`FileRename`, `FileRemove`, `FileCopy`, `FileMove`, `FileTextReplace`,
`FileTextFindIndex`, `FileExists`, `DirectoryExists`, `GetFileLength`,
`GetFileModTime`, `MakeDirectory`, `IsPathFile` — each an unrestricted host file
system call. All are routed now. The pure string helpers next to them
(`GetFileExtension`, `GetFileName`, `GetDirectoryPath`, `GetPrevDirectoryPath`,
`IsFileNameValid`) touch no file system and are left alone.

Refused when sandboxed, beyond the list above: `GetApplicationDirectory` and
`LoadDroppedFiles`, both of which hand real host paths straight to script, and
`LoadDirectoryFilesEx` / `GetDirectoryFileCountEx`, whose filter-and-recurse
semantics have no clean virtual answer. Plain `LoadDirectoryFiles` and
`GetDirectoryFileCount` do have one, so they list through `fs` and return
*virtual* paths rather than being refused.

**A resolver cannot tell reads from writes, and that was a real hole.**
`fs::HostPath` resolved a path and handed back a real one — including inside a
read-only mount, because it has no idea what the caller intends to do with it.
`raylib.FileRemove("/sys/a.txt")` therefore deleted a file on the read-only
system disk, and every `Save*`/`Export*` entry point had the same problem from
the moment step 2 routed them. Destinations now use `fs::HostPathForWrite`,
which additionally requires `IsWritable`. The lesson generalizes: **any future
binding that resolves a path it is about to modify must use the write-checked
variant**, and the read-only flag is only enforced if the call site asks for it.

`import` search directories are frozen at the latch rather than resolved through
`fs`: they are real host directories chosen by the host during boot, and a
library name may not contain a separator, so nothing outside them is reachable.
The name check now rejects `\` and `..` as well as `/`. Verified with a control:
the same script without the latch *does* import from a repointed
`MS_IMPORT_PATH`, and with it does not.

`http.post` is restricted to `http` and `https` by allow-list, and this applies
whether or not the sandbox has latched — a network call is named for its
protocol, and there was never a reason for it to open a `file:` URL.

### Still outstanding

- The `*FromMemory` fallback for backends that decline `RealPath`. Deferred to
  step 6 deliberately: every backend today is a real directory, so there is
  nothing to exercise it against, and it is needed exactly when the zip backend
  lands. Until then a zip mount would simply fail to load through raylib.
- The native file picker (step 5); see the step 5 notes.
- Drag and drop is verified to compile and run, but the drop itself has only
  been exercised by hand — `assets/diskdrop.ms` is the way to do that.

### Step 3 notes

The `file` module now goes through the resolver for everything. Two behavior
changes came with it, both deliberate:

- **File handles buffer in memory and write back on `close`.** That is forced by
  the backend interface (a zip mount has no seekable stream to hand out), and it
  is what Mini Micro 1 has always done. The cost is that a handle which is never
  closed never persists. `file.open` with the default mode still creates a
  missing file, as it always has, by mapping onto fs's `rw+`.
- **`FileHandle.read` counts code points, not bytes**, and its parameter is now
  `codePointCount`. This follows Mini Micro; **MiniScript 2's own
  `ShellIntrinsics` counts bytes**, so the two `file` modules now disagree. That
  is a porting-friction item worth taking back to MS2 rather than leaving as a
  silent divergence — MS1 code that reads a fixed count from a UTF-8 file
  mis-slices under byte semantics.

`FileHandle.seek(pos)` was added; MS2's `ShellIntrinsics` already had it and we
did not. MiniScript 2 has no assignment hooks, so Mini Micro 1's assignable
`f.position` cannot be reproduced and `seek` is the replacement.

`name`, `parent`, and `child` are pure string operations, so they switch on the
latch: virtual `/`-separated paths when sandboxed, their previous
platform-specific behavior when not. Unsandboxed `file.info().path` likewise
still reports the canonical *real* path, because that is what it has always
done; sandboxed it reports the virtual path.

One real bug fixed in passing: `readLines` buffered the final line of a file
that did not end in a newline and then discarded it.

Not yet exercised end to end: **writing through the file module while
sandboxed.** Every mount today is read-only, so there is no writable disk to
test against until step 5 mounts one. The path is covered in `fs_tests` at the
fs layer, and unsandboxed writes are covered through the intrinsics, but the
combination is not.

## Reference

Mini Micro 1's disk layer, in
`~/svnrepo/stroutandsons/MiniScript/MiniMicro/Assets/Scripts/Disk/`:

- `Disk.cs` — the abstract backend interface to mirror
- `ZipDisk.cs` — writable zip, saving on every mutation; implied-folder repair
- `ReadOnlyZipDisk.cs` — the same from a byte array rather than a file
- `WebURLZipDisk.cs`, `TextAssetZipDisk.cs` — other read-only sources
- `RealFileDisk.cs` — the real-directory backend

It uses the Ionic DotNetZip library, which we obviously cannot; the behavior to
reproduce is documented above.

## Debugging

Because a violation is indistinguishable from a bad path, the sandbox will
sometimes be indistinguishable from a bug. The host should log rejected paths
and the reason to stderr — visible to whoever is running the build, never to
script — or the first mysterious "file not found" during the Mini Micro shell's
own development costs an afternoon.
