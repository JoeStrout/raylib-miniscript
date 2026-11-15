# Memory Management

How raylib-miniscript manages raylib's resources (textures, meshes, fonts,
sounds and the like): what a script needs to do, and how that differs from
raylib in C.

## For script writers

### Plain values and resources

Most of what the raylib functions return is plain MiniScript data: a Vector3,
Color or Rectangle is an ordinary map, a list of points is an ordinary list,
a list from `LoadImageColors` or `LoadCodepoints` is an ordinary list, and a
RawData (from `LoadFileData`, `LoadWaveSamples`, `CompressData` and so on) is
freed automatically.  The garbage collector takes care of all of
these, as it does for any MiniScript value.

A **resource** is different: it holds memory or GPU objects that raylib
allocated, and, as in C, you free it yourself with the matching `Unload`
function.

| Kind | Made by (for example) | Freed by |
|---|---|---|
| Image | `LoadImage`, `GenImageColor`, `ImageCopy` | `UnloadImage` |
| Texture | `LoadTexture`, `LoadTextureFromImage` | `UnloadTexture` |
| RenderTexture | `LoadRenderTexture` | `UnloadRenderTexture` |
| Font | `LoadFont`, `LoadFontEx`, `MakeFont` | `UnloadFont` |
| Shader | `LoadShader`, `LoadShaderFromMemory` | `UnloadShader` |
| Mesh | `GenMeshCube` and the other `GenMesh*`, `MakeMesh` | `UnloadMesh` |
| Material | `LoadMaterialDefault`, `LoadMaterials` | `UnloadMaterial` |
| Model | `LoadModel`, `LoadModelFromMesh` | `UnloadModel` |
| ModelAnimation | `LoadModelAnimations` | `UnloadModelAnimations` |
| Wave | `LoadWave`, `WaveCopy` | `UnloadWave` |
| Sound | `LoadSound`, `LoadSoundAlias` | `UnloadSound`, `UnloadSoundAlias` |
| Music | `LoadMusicStream` | `UnloadMusicStream` |
| AudioStream | `LoadAudioStream` | `UnloadAudioStream` |

A resource you never unload is not freed: dropping the last reference to it
leaks it (unless you opted in to `autoUnload`; see below).
`resourceCounts` returns how many of each kind are currently loaded, which is
handy for spotting leaks.

### Unloading is safe

A resource map holds a handle, not the resource itself, so copies of the map
(`{} + tex`, or the same map stored in two places) all refer to one resource.
Unloading it through any copy unloads it for all of them, and from then on:

- every copy reads as an unloaded resource;
- drawing with it, or passing it to any other function, does nothing (or
  returns null, or an empty result);
- unloading it again, through any copy, does nothing.

So a mistake in your unloading can make something not draw, but it can't crash
the program or corrupt memory.

### Resources that own other resources

Some resources own others, and unloading the owner unloads what it owns.  A
resource you got from its owner is a **view** of the owner's part: use it
freely while the owner is loaded, but don't unload it yourself.  Once the owner
is unloaded, the view reads as unloaded too.

| Owner | Owned part | How you get it |
|---|---|---|
| Model | its meshes | `GetModelMesh(model, i)` |
| Model | the mesh it was made from | the mesh you passed to `LoadModelFromMesh` |
| Font | its texture | `font.texture` |
| Font | its glyph images | `GetGlyphInfo(font, codepoint).image` |
| RenderTexture | its color texture | `target.texture` |
| Material | its shader | `GetMaterialShader(material)` |

`UnloadMesh` of a mesh a model owns does nothing.  For the other views, an
explicit `Unload` still goes through (as in C), so leave them to their owner.

**Meshes and models.**  `LoadModelFromMesh(mesh)` makes a model that takes
ownership of the mesh's data.  You can go on using `mesh` (draw it, read it
with `GetMeshBuffer`, test collisions, update it with `UpdateMeshBuffer`) for
as long as the model is loaded; `UnloadModel` frees it.  A mesh can belong to
only one model: passing it to `LoadModelFromMesh` a second time returns null.

```
mesh = raylib.GenMeshCube(1, 1, 1)
model = raylib.LoadModelFromMesh(mesh)   // model now owns mesh's data
// ... raylib.DrawModel model, ... or raylib.DrawMesh mesh, ... both fine ...
raylib.UnloadModel model                 // frees the mesh too; don't UnloadMesh it
```

Changes to the Mesh struct itself (`GenMeshTangents`, `UploadMesh`) apply only
to the map you pass in, not to a model built from it, so make them before
`LoadModelFromMesh`.

### Handing a resource over

A few functions take ownership of a resource you pass them.  After the call,
the new owner unloads it, so don't unload it yourself:

- `SetMaterialTexture(material, mapType, texture)`: `UnloadMaterial` unloads the texture.
- `SetMaterialShader(material, shader)`: `UnloadMaterial` unloads the shader.
- `MakeFont(baseSize, glyphs, recs, texture)`: `UnloadFont` unloads the texture.
  The glyph images are copied, so you still own (and unload) the ones you passed.

And a few functions that look similar do **not** take ownership; you still
unload these yourself:

- `SetModelMaterialTexture` and `SetModelMaterialShader`: the model uses the
  texture or shader but never unloads it.  (Neither does `UnloadModel` for a
  model loaded from a file: its materials' textures and shaders are yours to
  unload, if you can reach them.)
- `LoadFontData` returns a list of glyph maps whose images you release with
  `UnloadFontData(glyphs)` (or `UnloadImage` on each).

The default font (`GetFontDefault`) belongs to raylib, which frees it in
`CloseWindow`; never unload it.

### Automatic unloading (opt-in)

`autoUnload(resource)` flags a resource to be unloaded automatically once
nothing refers to it any more, and returns it, so it can wrap the load call:

```
tex = autoUnload(raylib.LoadTexture("player.png"))
```

It is meant for wrapper libraries (Soda, Mini Micro-style classes) that own
what they load.  It works for Image, Texture, Font, Wave, Music, Sound,
AudioStream, RenderTexture and Shader, but not for Mesh, Material, Model and
the other kinds that hand parts to one another, and not for a view (an owned
part, as above).  Flagged resources are unloaded at a safe moment, never in
the middle of a frame, and one from a window or audio device that has since
been closed is simply dropped.

## How this differs from raylib in C

In C, a resource is a plain struct passed by value.  Copies of it hold the same
pointers and GPU ids, and nothing records whether it has been unloaded.  Every
difference below comes from raylib-miniscript keeping each resource in one
shared box behind a handle, and recording which boxes are owned by others.

**Valid C code behaves the same.**  Wherever a C program is correct (it
unloads each resource once, and doesn't use one after unloading it or its
owner), the same calls do the same thing here.  The differences are in what
happens when C code is *wrong*:

| Situation | C | raylib-miniscript |
|---|---|---|
| Unload a resource twice, or through a second copy | double free, usually a crash | does nothing |
| Use a resource after unloading it | use after free: garbage, or a crash | does nothing, or returns null |
| `DrawMesh` of an unloaded mesh | crash (null `vboId`) | does nothing |
| `UnloadMesh(mesh)` after `LoadModelFromMesh(mesh)`, then `UnloadModel` | double free | `UnloadMesh` does nothing; `UnloadModel` frees the mesh once |
| `UnloadMesh` of a model's mesh (`model.meshes[i]` / `GetModelMesh`) | double free at `UnloadModel` | does nothing |
| Use a model's mesh, a font's texture or glyph image, a render texture's texture, or a material's shader after unloading its owner | use after free | reads as unloaded |
| Forge or corrupt a resource (e.g. `img._handle = 12345`) | (not possible) | treated as no resource |

There are a few small differences you could notice in otherwise valid code:

- **One model per mesh.**  C lets two models share one mesh (with
  `LoadModelFromMesh(mesh)` twice), as long as at most one of them is ever
  unloaded.  Here the second call returns null, since two owners of the same
  data would free it twice.  Make a second mesh for a second model.
- **When a model's mesh is freed.**  In C, `UnloadMesh(mesh)` on a mesh given to
  a model frees its data right away (leaving the model dangling, so only safe if
  the model is never drawn or unloaded again).  Here the data stays until
  `UnloadModel`.
- **Opt-in automatic unloading** (`autoUnload`) has no C equivalent.
- **Plain data is garbage collected.**  Functions that return data raylib
  allocated with `Load...` and expects you to free with `Unload...` (file data,
  wave samples, image colors, codepoint lists and the like) mostly return
  ordinary MiniScript values here, so the matching `Unload...` is unneeded, or
  absent, or only empties the value early (`UnloadFileData`,
  `UnloadWaveSamples`).

### Implementation notes

For maintainers; see `src/RaylibTypes.h` for details.

- Each resource map has a `_handle`: a GC handle to a `ResourceBox<T>` holding
  the raylib struct, its kind, and a `live` flag.  Script can't forge one;
  a handle of the wrong kind reads as no resource.
- `Unload*` calls `TakeResource`, which copies the struct out, zeroes it, and
  clears `live`, so each resource is unloaded exactly once.
- A view's box is marked `borrowed` (`MarkBorrowed(view, owner)`), and its map
  holds its owner in `_owner`, keeping the owner from being collected (or
  auto-unloaded) while the view is in use.
- An owner and its views share an `OwnerLife` (a reference-counted
  "still loaded" flag).  Unloading the owner clears it, and a view whose owner
  is gone reads as unloaded from then on (checked in `AnyResourceBoxOf`, which
  every lookup goes through).
- `TransferOwnership(map)` marks a resource handed to a new owner (material
  textures and shaders, a font's texture): it can no longer be auto-unloaded.
- `UnloadMesh` refuses a borrowed mesh.  Other `Unload*` functions do not yet
  check `borrowed`; doing so would make "don't unload a view" safe for every
  kind, as it now is for meshes.
