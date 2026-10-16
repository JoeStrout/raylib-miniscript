# Upgrading to raylib 6.2

raylib-miniscript now builds against raylib 6.2 (previously 6.0). Our wrappers
follow raylib's API, so raylib's breaking changes pass through to scripts.
This note lists everything that may need attention in existing code.

## Changes that fail silently

These still run without error, but behave differently. Search your code for them first.

### `ChangeDirectory` now returns 0 on success

Before, it returned true (1) on success. Now it returns 0 on success and
nonzero on failure, as C's `chdir` does. An old check like

```miniscript
if raylib.ChangeDirectory(path) then ...      // now true on FAILURE
```

must become

```miniscript
if raylib.ChangeDirectory(path) == 0 then ...
```

This now matches `FileRename`, `FileRemove`, `FileCopy`, `FileMove` and
`FileTextReplace`, which already returned 0 on success.

### `DrawCapsule` / `DrawCapsuleWires`: `rings` and `slices` swapped

The order changed from `(..., radius, slices, rings, color)` to
`(..., radius, rings, slices, color)`. Positional calls must swap those two
arguments. Defaults are unchanged (rings=8, slices=16), so calls that omit
them are unaffected.

### `ImageColorContrast` takes an integer

`contrast` is now an int in -100..100; fractional values are truncated.

## Renamed or removed functions (fail loudly)

Calls to the old names fail with "Undefined Identifier", so these are easy to find.

| Old (6.0) | New (6.2) | Notes |
|---|---|---|
| `GetSplinePointBezierQuad` | `GetSplinePointBezierQuadratic` | Same arguments |
| `ImageDrawTriangleEx` | `ImageDrawTriangleGradient` | Same arguments |
| `ImageDraw(dst, src, srcRec, dstRec, tint)` | `ImageDrawImagePro(dst, src, srcRec, dstRec, origin, rotation, tint)` | `origin` and `rotation` come before `tint`, so a call that passed `tint` now must add `[0,0], 0` before it. See also `ImageDrawImage`, `ImageDrawImageRec`, `ImageDrawImageEx`. |
| `ImageDrawRectangleLines(dst, rec, thick, color)` | `ImageDrawRectangleLinesEx(dst, rec, thick, color)` | The name `ImageDrawRectangleLines` still exists but now takes `(dst, posX, posY, width, height, color)`, so an old call **won't error**: it misreads the rect as `posX` and draws garbage. Search for it. |

## Renamed parameters (documentation only)

MiniScript passes arguments by position, so these renames only affect the docs:

- `DrawRectangleLinesEx`, `DrawRectangleRoundedLinesEx`, `DrawPolyLinesEx`: `lineThick` → `thick`
- `DrawCylinder`, `DrawCylinderWires`: `slices` → `sides`
- `SetShapesTexture`, `DrawTextureRec`, `DrawBillboardRec`, `DrawBillboardPro`: `source` → `rec`
- `UpdateSound`: `sampleCount` → `frameCount` (it always meant frames)

## New functions

- **Shapes:** `DrawCircleLinesEx`, `DrawCircleSectorLinesEx`, `DrawEllipseLinesEx`,
  `DrawRingLinesEx`, `DrawTriangleLinesEx` (outlines with thickness),
  `DrawTriangleGradient`
- **Images:** `ImageDrawImage`, `ImageDrawImageEx` (rotation + scale),
  `ImageDrawImageRec`, `ImageDrawImagePro`, `ImageDrawRectanglePro`,
  `ImageDrawRectangleGradientEx`, `ImageDrawCircleGradient`, `ImageDrawLineStrip`
- **Textures:** `LoadRenderTextureEx(width, height, format)`
- **Files:** `IsPathDirectory`, `IsPathAbsolute`, `IsFileHidden`
- **Constants:** `RAYLIB_VERSION` (e.g. `"6.2"`), `RAYLIB_VERSION_MAJOR`,
  `RAYLIB_VERSION_MINOR`, `RAYLIB_VERSION_PATCH`, giving the raylib version
  this build was compiled against

## Known quirks in raylib 6.2 itself

- `ImageDrawImagePro` accepts `origin` and `rotation` but **ignores** them
  (marked TODO upstream). Use `ImageDrawImageEx` to draw a rotated image.
- `ImageDrawRectanglePro` rotates around `rec.xy + origin`. `DrawRectanglePro`
  instead treats `rec.xy` as the pivot point.
- raylib 6.2 declares `ImageDrawTextPro`, but the function body is empty, so
  we don't wrap it.
- raylib's `IsPathDirectory` returns true for any path that isn't a regular file,
  even one that doesn't exist. Our wrapper also requires the directory to exist.
- Triangle functions (`DrawTriangleGradient`, `DrawTriangleLinesEx`, …) expect
  counter-clockwise vertex order.

## `import` now matches MS2 (not raylib-related)

- `import` now **returns** the module map, as well as storing it in a variable
  named after the module, so `m = import(name)` works. Before, it returned null.
- On failure (library not found, a path instead of a name, an empty name),
  `import` now returns a file error value instead of raising a runtime error.
  A bare `import "foo"` statement still halts the program. But with
  `x = import("foo")`, the program keeps running and `x` gets the error, so
  check it with `isa error`. Compile errors in the module already worked this way.

## For developers building from source

- After moving the `raylib` submodule to a new commit, the build scripts now
  rebuild `libraylib.a` / `libraylib.web.a` automatically. Each script records
  the raylib commit it built from in `build/raylib-commit.txt` or
  `build-web/raylib-commit.txt`. Previously both scripts could keep linking a
  stale library.
- macOS now also links `-framework QuartzCore`, which raylib 6.2's GLFW needs.
- The host callback for `SetSaveFileDataCallback` now takes `const void*` data.
