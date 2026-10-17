#pragma once

// Include MiniScript before raylib: raylib.h #defines PI as a macro, which
// otherwise clobbers MiniScript's Math::PI constant when its headers are parsed.
#include "miniscript.h"
#include "raylib.h"

using namespace MiniScript;

// MiniScript classes (maps) that represent various Raylib structs.
//
// Each returns the class prototype as a Value: built, wrapped and GC-rooted on
// first use, then handed back unchanged.  Returning the Value rather than the
// ValueDict is what keeps the per-instance `__isa` assignments below a plain
// read -- StaticMap would cost a hashtable lookup per object created -- and it
// gives each class the single stable identity `isa` compares against.
//
// Each is registered under the short name `raylib.Image`, `raylib.Sound`, and
// so on -- qualified, because that is how script names them now that they live
// in the raylib module rather than in globals.  An unqualified short name would
// print `{"__isa": Image, ...}` for both a raylib image and a script's own
// class of the same name (soda's Image, asteroids' Sound), which is exactly the
// confusion that moving them into the module was meant to end.
const Value& ImageClass();
const Value& TextureClass();
const Value& FontClass();
const Value& WaveClass();
const Value& MusicClass();
const Value& SoundClass();
const Value& AudioStreamClass();
const Value& RenderTextureClass();
const Value& ShaderClass();
const Value& MeshClass();
const Value& MaterialClass();
const Value& ModelClass();
const Value& ModelAnimationClass();
const Value& Camera3DClass();

// Install the above classes into the given (raylib) module map
void AddTypeClasses(ValueDict& raylibModule);

// Convert a Raylib Texture to a MiniScript map
// Keeps the Texture in a resource handle in _handle
Value TextureToValue(Texture texture);

// Extract a Raylib Texture from a MiniScript map
// Returns a copy of the Texture in its resource handle
Texture ValueToTexture(Value value);

// Convert a Raylib Image to a MiniScript map
// Keeps the Image in a resource handle in _handle
Value ImageToValue(Image image);

// Extract a Raylib Image from a MiniScript map (read-only reference)
const Image& ValueToImage(Value value);

// Extract a mutable pointer to a Raylib Image from a MiniScript map
Image* ValueToImagePtr(Value value);

// After mutating an Image, sync its properties back to the MiniScript map
void UpdateImageValue(Value value);

// Convert a Raylib Font to a MiniScript map
Value FontToValue(Font font);

// Extract a Raylib Font from a MiniScript map
Font ValueToFont(Value value);

// Convert a Raylib Wave to a MiniScript map
Value WaveToValue(Wave wave);

// Extract a Raylib Wave from a MiniScript map
Wave ValueToWave(Value value);

// Convert a Raylib Music to a MiniScript map
Value MusicToValue(Music music);

// Extract a Raylib Music from a MiniScript map
Music ValueToMusic(Value value);

// Convert a Raylib Sound to a MiniScript map
Value SoundToValue(Sound sound);

// Extract a Raylib Sound from a MiniScript map
Sound ValueToSound(Value value);

// Convert a Raylib AudioStream to a MiniScript map
Value AudioStreamToValue(AudioStream stream);

// Extract a Raylib AudioStream from a MiniScript map
AudioStream ValueToAudioStream(Value value);

// Convert a Raylib RenderTexture2D to a MiniScript map
// Keeps the RenderTexture2D in a resource handle in _handle
Value RenderTextureToValue(RenderTexture2D renderTexture);

// Extract a Raylib RenderTexture2D from a MiniScript map
// Returns a copy of the RenderTexture2D in its resource handle
RenderTexture2D ValueToRenderTexture(Value value);

// Convert a Raylib Shader to a MiniScript map
Value ShaderToValue(Shader shader);

// Extract a Raylib Shader from a MiniScript map
Shader ValueToShader(Value value);

// Convert a Raylib Mesh to a MiniScript map
Value MeshToValue(Mesh mesh);

// Extract a Raylib Mesh from a MiniScript map
Mesh ValueToMesh(Value value);

// Convert a Raylib Material to a MiniScript map
Value MaterialToValue(Material material);

// Extract a Raylib Material from a MiniScript map
Material ValueToMaterial(Value value);

// Convert a Raylib Model to a MiniScript map
Value ModelToValue(Model model);

// Extract a Raylib Model from a MiniScript map
Model ValueToModel(Value value);

// Convert a Raylib ModelAnimation to a MiniScript map
Value ModelAnimationToValue(ModelAnimation anim);

// Extract a Raylib ModelAnimation from a MiniScript map
ModelAnimation ValueToModelAnimation(Value value);

// Convert a MiniScript map to a Raylib Color
// Expects a map with "r", "g", "b", and optionally "a" keys (0-255);
// or, a 3- or 4-element list in the order [r, g, b, a].
Color ValueToColor(Value value);

// Convert a Raylib Color to a MiniScript map
Value ColorToValue(Color color);

// Convert a MiniScript value to a Raylib Rectangle
// Accepts either a map with "x", "y", "width", "height" keys OR a list with 4 elements
Rectangle ValueToRectangle(Value value);

// Convert a Raylib Rectangle to a MiniScript map
Value RectangleToValue(Rectangle rect);

// Convert a MiniScript value to a Raylib Vector2
// Accepts either a map with "x", "y" keys OR a list with 2 elements
Vector2 ValueToVector2(Value value);

// Convert a Raylib Vector2 to a MiniScript map
Value Vector2ToValue(Vector2 vec);

// Convert a MiniScript value to a Raylib Vector3
// Accepts either a map with "x", "y", "z" keys OR a list with 3 elements
Vector3 ValueToVector3(Value value);

// Convert a Raylib Vector3 to a MiniScript map
Value Vector3ToValue(Vector3 vec);

// Convert a MiniScript value to a Raylib Camera3D
// Accepts a map with {position,target,up,fovy,projection}
Camera3D ValueToCamera3D(Value value);

// Convert a Raylib Camera3D to a MiniScript map
Value Camera3DToValue(Camera3D camera);

// Convert a MiniScript value to a Raylib Vector4
// Accepts either a map with "x", "y", "z", "w" keys OR a list with 4 elements
Vector4 ValueToVector4(Value value);

// Convert a Raylib Vector4 to a MiniScript map
Value Vector4ToValue(Vector4 vec);

// Convert a MiniScript value to a Raylib Quaternion
// Accepts either a map with "x", "y", "z", "w" keys OR a list with 4 elements
Quaternion ValueToQuaternion(Value value);

// Convert a Raylib Quaternion to a MiniScript map
Value QuaternionToValue(Quaternion q);


// Convert a MiniScript value to a Raylib Matrix
// Accepts a matrixUtil-compatible map with {rows,columns,elem},
// a legacy map with "m0".."m15" keys, or list forms.
Matrix ValueToMatrix(Value value);

// Convert a Raylib Matrix to a MiniScript matrixUtil-compatible map
Value MatrixToValue(Matrix mat);

// Convert a MiniScript value to a Raylib BoundingBox
// Accepts map form with "min" and "max" Vector3 values
BoundingBox ValueToBoundingBox(Value value);

// Convert a Raylib BoundingBox to a MiniScript map
Value BoundingBoxToValue(BoundingBox box);

// Convert a MiniScript value to a Raylib Ray
// Accepts map form with "position" and "direction" Vector3 values
Ray ValueToRay(Value value);

// Convert a Raylib Ray to a MiniScript map
Value RayToValue(Ray ray);

// Convert a Raylib RayCollision to a MiniScript map
Value RayCollisionToValue(RayCollision collision);

//--------------------------------------------------------------------------------
// Resource handles
//
// Every raylib resource map (Image, Texture, Sound, ...) keeps its C struct in
// `_handle`, as an opaque MiniScript handle that script can neither forge nor
// look inside.  The handle owns a ResourceBox: the struct plus a tag naming its
// type, so that moving one map's `_handle` into a map of another type (an
// Image's into a Texture) is refused rather than reinterpreting the struct.
//
// The box is freed when its handle is collected.  The raylib resource itself
// (GPU texture, audio buffer, ...) is not: that still takes an explicit Unload*,
// as in the C API.  Unloading empties the box (TakeResource), so any other map
// sharing the same handle reads as an empty resource, never a stale one.
//--------------------------------------------------------------------------------

struct rlRenderBatch;	// rlgl.h

enum class ResourceKind : int {
	Image = 1, Texture, Font, Wave, Music, Sound, AudioStream, RenderTexture,
	Shader, Mesh, Material, Model, ModelAnimation, ModelAnimationArray,
	AutomationEventList, RenderBatch
};

// The array LoadModelAnimations returns.  Each ModelAnimation map it produces
// holds the whole array's handle, plus its own `_arrayIndex` into it.
struct ModelAnimationArray {
	ModelAnimation* animations;
	int count;
};

template<typename T> struct ResourceKindOf;
#define RESOURCE_KIND(T, K) \
	template<> struct ResourceKindOf<T> { static const ResourceKind kind = ResourceKind::K; };
RESOURCE_KIND(Image, Image)
RESOURCE_KIND(Texture, Texture)
RESOURCE_KIND(Font, Font)
RESOURCE_KIND(Wave, Wave)
RESOURCE_KIND(Music, Music)
RESOURCE_KIND(Sound, Sound)
RESOURCE_KIND(AudioStream, AudioStream)
RESOURCE_KIND(RenderTexture, RenderTexture)
RESOURCE_KIND(Shader, Shader)
RESOURCE_KIND(Mesh, Mesh)
RESOURCE_KIND(Material, Material)
RESOURCE_KIND(Model, Model)
RESOURCE_KIND(ModelAnimation, ModelAnimation)
RESOURCE_KIND(ModelAnimationArray, ModelAnimationArray)
RESOURCE_KIND(AutomationEventList, AutomationEventList)
RESOURCE_KIND(rlRenderBatch, RenderBatch)
#undef RESOURCE_KIND

struct ResourceHeader {
	ResourceKind kind;
	bool live;			// false once unloaded
};

template<typename T> struct ResourceBox : ResourceHeader {
	T res;
};

// Wrap a freshly allocated box in a handle Value.
Value WrapResourceBox(ResourceHeader* box);

// The box behind a map's `_handle`, if that is a resource handle of the given
// kind (live or not); otherwise null.
ResourceHeader* ResourceBoxOf(Value map, ResourceKind kind);

// The raw `_handle` Value of a map (null if it is not a map, or has none).
Value ResourceHandleOf(Value map);

// Make a handle owning a copy of the given raylib struct.
template<typename T> Value NewResourceHandle(const T& res) {
	ResourceBox<T>* box = new ResourceBox<T>();
	box->kind = ResourceKindOf<T>::kind;
	box->live = true;
	box->res = res;
	return WrapResourceBox(box);
}

// The resource behind a map's `_handle`, or null if the map has none, holds
// some other kind of handle, or has been unloaded.  Writes through the pointer
// are seen by every map sharing the handle.
template<typename T> T* ResourcePtr(Value map) {
	ResourceHeader* box = ResourceBoxOf(map, ResourceKindOf<T>::kind);
	if (box == nullptr || !box->live) return nullptr;
	return &static_cast<ResourceBox<T>*>(box)->res;
}

// For Unload*: copy the resource out and empty its box, so that the caller can
// unload it exactly once.  Returns false, leaving *out alone, if there is no
// live resource to unload.
template<typename T> bool TakeResource(Value map, T* out) {
	ResourceHeader* box = ResourceBoxOf(map, ResourceKindOf<T>::kind);
	if (box == nullptr || !box->live) return false;
	T& res = static_cast<ResourceBox<T>*>(box)->res;
	*out = res;
	res = T{};
	box->live = false;
	return true;
}

// Keeps one resource handle alive while raylib holds a pointer into its box
// (the active render batch, the active automation event list): the handle is
// a GC root until cleared or replaced.
class PinnedResource {
public:
	void Set(Value map);	// pin map's `_handle`; a non-resource pins nothing
	void Clear();
private:
	Value handle;
};

// Resource allocation counters (for leak detection by MiniScript users)
extern int rcImage;
extern int rcTexture;
extern int rcFont;
extern int rcWave;
extern int rcMusic;
extern int rcSound;
extern int rcAudioStream;
extern int rcRenderTexture;
extern int rcShader;
extern int rcMesh;
extern int rcMaterial;
extern int rcModel;
extern int rcModelAnimation;


