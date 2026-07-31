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

`/sys` and `/hw` are mounted at boot, before the latch. `/usr` and `/usr2`
start unmounted.

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
request a mount but never names the target**: an intrinsic that opens a
*native* directory/file picker and mounts whatever the user chooses. Code can
pester the user with a dialog; it cannot pick a directory.

This is the same guarantee Mini Micro 1 gets from its Unity file dialog. Raylib
has no picker, so it means vendoring one — tinyfiledialogs is a single
public-domain C file and the usual choice.

Unmounting is safe to expose directly.

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
5. **The mount dialog**: vendor a native picker; `/usr` and `/usr2` become
   mountable (to real directories) by the user.
6. **Zip backend**, read and write, so a `.minidisk` can be mounted as `/usr`
   or `/usr2`.
7. **Web**: `/usr` from a `.minidisk` prepared next to the web build; `/usr2`
   backed by IndexedDB. Note the file module is desktop-only today.

Steps 1 and 2 are the ones that shake out path-translation bugs while escapes
are still cheap to fix.

### Status

Steps 1, 2 and 3 are done (`src/FileSystem.{h,cpp}`, `tests/fs_tests.cpp`,
`src/FileModule.cpp`).

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

What step 4 still owes:

- The remaining path-taking loaders: `LoadModel`, `LoadModelAnimations`,
  `LoadMaterials`, `ExportMesh`, `ExportMeshAsCode`, `LoadTextureCubemap`,
  `LoadDirectoryFiles`, `LoadDirectoryFilesEx`, `LoadAutomationEventList`,
  `ExportAutomationEventList`, `LoadVrStereoConfig`.
- The whole "Reject when sandboxed" list above, none of which is done.
- The `*FromMemory` fallback for backends that decline `RealPath` — untestable
  until the zip backend exists (step 6), since every backend today is a real
  directory.
- `import` and `http` hardening.

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
