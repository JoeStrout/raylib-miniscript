# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

## LoadModelFromMesh leaves the mesh live (likely double free)

`LoadModelFromMesh` (src/RModels.cpp) hands raylib a shallow copy of the mesh,
so the new Model points at the same vertex data and `UnloadModel` frees it.  But
the script's Mesh map keeps a live handle, so a script that calls `UnloadMesh` on
it and later `UnloadModel` will likely free that vertex data twice.  Raylib's own
note on `LoadModelFromMesh` warns about this shallow copy.

Possible fix: have `LoadModelFromMesh` take the mesh (`TakeResource`, `rcMesh--`),
so a later `UnloadMesh` is a no-op, and document that the mesh is consumed with an
`// API:` note.  Not yet verified with a test.
