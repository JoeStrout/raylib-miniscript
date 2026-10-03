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
// Allocates the Texture on the heap and stores it in a GC handle in _handle
Value TextureToValue(Texture texture);

// Extract a Raylib Texture from a MiniScript map
// Returns the Texture from the _handle handle
Texture ValueToTexture(Value value);

// Convert a Raylib Image to a MiniScript map
// Allocates the Image on the heap and stores it in a GC handle in _handle
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
// Allocates the RenderTexture2D on the heap and stores it in a GC handle in _handle
Value RenderTextureToValue(RenderTexture2D renderTexture);

// Extract a Raylib RenderTexture2D from a MiniScript map
// Returns the RenderTexture2D from the _handle handle
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

// Native raylib structs live in GC handles (Value::NewHandle), which script code
// cannot forge from a number.  A script can still copy a real handle, so two more
// guards apply:
//  - the finalizer pointer doubles as a type tag (each T has its own
//    FreeNative<T>), so a Sound handle copied into an Image map is rejected;
//  - `live` goes false on Unload*, so every copy of the handle dies together
//    instead of dangling.
// The finalizer frees only the box, never the raylib resource: resources are
// still released by the explicit Unload* call.
template<typename T> struct NativeBox { T obj; bool live; };

template<typename T> void FreeNative(void* p) { delete (NativeBox<T>*)p; }

// The "_handle" key, built lazily (a string Value can't be made at static-init
// time) and GC-rooted.
inline const Value& kHandleKey() {
	static Value k = [] { Value v("_handle"); GCManager::AddRoot(v); return v; }();
	return k;
}

template<typename T> Value NewNativeHandle(const T& v) {
	return Value::NewHandle(new NativeBox<T>{v, true}, &FreeNative<T>);
}

template<typename T> NativeBox<T>* NativeBoxOf(Value h) {
	if (!h.IsHandle()) return nullptr;
	GCHandle gh = GCManager::GetHandle(h);
	if (gh.Callback != &FreeNative<T>) return nullptr;
	return (NativeBox<T>*)gh.UserData;
}

// The live object behind a handle Value, or null.
template<typename T> T* NativeHandlePtr(Value h) {
	NativeBox<T>* box = NativeBoxOf<T>(h);
	return (box && box->live) ? &box->obj : nullptr;
}

// The live object behind a map's _handle, or null.
template<typename T> T* NativePtrFromMap(Value m) {
	if (m.Type() != ValueType::Map) return nullptr;
	return NativeHandlePtr<T>(m.GetDict().Lookup(kHandleKey(), Value::Null));
}

// As above, for a handle stored under another key of an already-extracted dict.
template<typename T> T* NativePtrFromMapKey(ValueDict map, const String& key) {
	return NativeHandlePtr<T>(map.Lookup(Value(key), Value::Null));
}

// For Unload*: mark the object dead and clear the map's _handle.  Returns the
// object (valid until the GC frees the box), or null if it was already dead or
// invalid -- so a counter decremented on a non-null result drops at most once.
template<typename T> T* TakeNative(Value m) {
	if (m.Type() != ValueType::Map) return nullptr;
	ValueDict map = m.GetDict();
	NativeBox<T>* box = NativeBoxOf<T>(map.Lookup(kHandleKey(), Value::Null));
	if (!box || !box->live) return nullptr;
	box->live = false;
	map.SetValue(kHandleKey(), Value::Null);
	return &box->obj;
}

// A raylib-allocated array of ModelAnimation (LoadModelAnimations).  Each item
// map refers to the array through an `_arrayHandle`, plus an index; the count
// comes from here, never from the (script-writable) `_arrayCount` field.
struct ModelAnimationArray { ModelAnimation* anims; int count; };

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


