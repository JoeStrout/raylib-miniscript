# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

## LoadModelFromMesh leaves the mesh live (likely double free)

`LoadModelFromMesh` (src/RModels.cpp) hands raylib a shallow copy of the mesh,
so the new Model points at the same vertex data and `UnloadModel` frees it.  But
the script's Mesh map keeps a live handle, so a script that calls `UnloadMesh` on
it and later `UnloadModel` will likely free that vertex data twice.  The reverse
order is also unsafe: after `UnloadModel`, a `DrawMesh(mesh)` or `UnloadMesh(mesh)`
uses freed data.  (The bundled `assets/rmodels_smoke.ms` avoids this only by never
unloading the mesh.)  Raylib's own
note on `LoadModelFromMesh` warns about this shallow copy.

Possible fix: have `LoadModelFromMesh` take the mesh (`TakeResource`, `rcMesh--`),
so a later `UnloadMesh` is a no-op, and document that the mesh is consumed with an
`// API:` note.  Not yet verified with a test.

---

The entries below were found during the 2026-11-15 review of the API notes
(`WithNote`).  They are from reading the code and are not yet confirmed with tests.

## UpdateAudioStream leaks and miscounts frames

`UpdateAudioStream` (src/RAudio.cpp): the `new TYPE[]` buffer in the
`PROCESS_DATA` macro is never freed, so every call leaks.  It also passes
`data.Count()` as `frameCount`, which is wrong for streams with more than one
channel.  It should be the sample count divided by `stream.channels`.

## GenImageFontAtlas mixes allocators and leaks

`GenImageFontAtlas` (src/RText.cpp): raylib overwrites `*glyphRecs` with its own
`RL_MALLOC` array, so the `new[]` input array leaks.  The returned `updatedRecs`
is then freed with `delete[]` instead of `RL_FREE`.  The input rectangles are
never read; raylib only writes them.

## GetCodepointPrevious reads before the string

`GetCodepointPrevious` (src/RText.cpp) passes the start of `text.c_str()`, so
raylib walks backward into memory before the string.  The result is meaningless
and the read is out of bounds.  It probably needs an index or offset parameter
(or to point at the end of the string).

## UnloadFileData frees a RawData buffer with raylib's free

`UnloadFileData` (src/RCore.cpp) calls raylib's `UnloadFileData` (plain `free`)
on a RawData's buffer.  That buffer came from `BinaryData::New` /
`AllocOrCollect` via `BytesToRawDataValue`, so this bypasses the allocator's
accounting, and it accepts any RawData, not just one from `LoadFileData`.  It
should probably clear the RawData or do nothing.

## ImageKernelConvolution doesn't check kernelSize

`ImageKernelConvolution` (src/RTextures.cpp) passes the script's `kernelSize`
to raylib without checking it against the kernel list's length, so a larger
`kernelSize` reads past the end of the array.

## GetGamepadName doesn't guard against NULL

`GetGamepadName` (src/RCore.cpp) builds its result directly from raylib's
`const char*`.  If raylib returns NULL (e.g. no gamepad), that could crash.
`GetKeyName` already guards against this case.

## Image wrappers that differ from raylib's signatures

In src/RTextures.cpp:
- `ImageResizeCanvas` takes `(image, offsetX, offsetY, newWidth, newHeight, fill)`;
  raylib's order is `(image, newWidth, newHeight, offsetX, offsetY, fill)`.
- `ImageDrawRectangleV` takes a single `rec` instead of `position` and `size`,
  so it duplicates `ImageDrawRectangleRec`.
- `ImageToPOT` has no fill color parameter; it always pads with BLACK.

## DrawSplineCatmullRom accepts too few points

`DrawSplineCatmullRom` (src/RShapes.cpp) only checks for at least 2 points, but
raylib needs at least 4 (`DrawSplineBasis` checks for 4).

## TakeScreenshot doesn't resolve its path

`TakeScreenshot` (src/RCore.cpp) passes the script's filename straight to
raylib without `fs::HostPath` / `HostPathForWrite`, unlike the other file
functions.  It is refused once sandboxed, so this matters only in unsandboxed runs.
