#include "RaylibTypes.h"

// Resource allocation counters
int rcImage = 0;
int rcTexture = 0;
int rcFont = 0;
int rcWave = 0;
int rcMusic = 0;
int rcSound = 0;
int rcAudioStream = 0;
int rcRenderTexture = 0;
int rcShader = 0;
int rcMesh = 0;
int rcMaterial = 0;
int rcModel = 0;
int rcModelAnimation = 0;

const Value& ImageClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("width"), Value::zero);
		map.SetValue(String("height"), Value::zero);
		map.SetValue(String("mipmaps"), Value::zero);
		map.SetValue(String("format"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Image"));
	}
	return classValue;
}

const Value& TextureClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("id"), Value::zero);
		map.SetValue(String("width"), Value::zero);
		map.SetValue(String("height"), Value::zero);
		map.SetValue(String("mipmaps"), Value::zero);
		map.SetValue(String("format"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Texture"));
	}
	return classValue;
}

const Value& FontClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("texture"), Value::Null);
		map.SetValue(String("baseSize"), Value::zero);
		map.SetValue(String("glyphCount"), Value::zero);
		map.SetValue(String("glyphPadding"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Font"));
	}
	return classValue;
}

const Value& WaveClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("frameCount"), Value::zero);
		map.SetValue(String("sampleRate"), Value::zero);
		map.SetValue(String("sampleSize"), Value::zero);
		map.SetValue(String("channels"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Wave"));
	}
	return classValue;
}

const Value& MusicClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("frameCount"), Value::zero);
		map.SetValue(String("looping"), Value::zero);
		map.SetValue(String("sampleRate"), Value::zero);
		map.SetValue(String("sampleSize"), Value::zero);
		map.SetValue(String("channels"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Music"));
	}
	return classValue;
}

const Value& SoundClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("frameCount"), Value::zero);
		map.SetValue(String("sampleRate"), Value::zero);
		map.SetValue(String("sampleSize"), Value::zero);
		map.SetValue(String("channels"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Sound"));
	}
	return classValue;
}

const Value& AudioStreamClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("sampleRate"), Value::zero);
		map.SetValue(String("sampleSize"), Value::zero);
		map.SetValue(String("channels"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.AudioStream"));
	}
	return classValue;
}

const Value& RenderTextureClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("id"), Value::zero);
		map.SetValue(String("texture"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.RenderTexture"));
	}
	return classValue;
}

const Value& ShaderClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("id"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Shader"));
	}
	return classValue;
}

const Value& MeshClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("vertexCount"), Value::zero);
		map.SetValue(String("triangleCount"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Mesh"));
	}
	return classValue;
}

const Value& MaterialClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("shaderId"), Value::zero);
		map.SetValue(String("_arrayHandle"), Value::Null);
		map.SetValue(String("_arrayCount"), Value::zero);
		map.SetValue(String("_arrayIndex"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Material"));
	}
	return classValue;
}

const Value& ModelClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("meshCount"), Value::zero);
		map.SetValue(String("materialCount"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Model"));
	}
	return classValue;
}

const Value& ModelAnimationClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("_handle"), Value::Null);
		map.SetValue(String("name"), Value::Null);
		map.SetValue(String("boneCount"), Value::zero);
		map.SetValue(String("keyframeCount"), Value::zero);
		map.SetValue(String("_arrayHandle"), Value::Null);
		map.SetValue(String("_arrayCount"), Value::zero);
		map.SetValue(String("_arrayIndex"), Value::zero);
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.ModelAnimation"));
	}
	return classValue;
}

// A frozen Vector3 map, for a class default.  Instances inherit these maps
// rather than copying them, so without freezing, `cam.position.y = 5` on a new
// camera would change the default for every camera.
static Value FrozenVector3(Vector3 v) {
	Value result = Vector3ToValue(v);
	result.Freeze();
	return result;
}

const Value& Camera3DClass() {
	static Value classValue;
	if (classValue.IsNull()) {
		ValueDict map;
		map.SetValue(String("position"), FrozenVector3(Vector3{0, 10, 10}));
		map.SetValue(String("target"), FrozenVector3(Vector3{0, 0, 0}));
		map.SetValue(String("up"), FrozenVector3(Vector3{0, 1, 0}));
		map.SetValue(String("fovy"), Value(45.0));
		map.SetValue(String("projection"), Value(CAMERA_PERSPECTIVE));
		classValue = GCManager::NewMapFromDict(map);   // shares map's storage
		GCManager::AddRoot(classValue);
		Intrinsic::AddShortName(classValue, String("raylib.Camera3D"));
	}
	return classValue;
}

void AddTypeClasses(ValueDict& raylibModule) {
	raylibModule.SetValue("Image", ImageClass());
	raylibModule.SetValue("Texture", TextureClass());
	raylibModule.SetValue("Font", FontClass());
	raylibModule.SetValue("Wave", WaveClass());
	raylibModule.SetValue("Music", MusicClass());
	raylibModule.SetValue("Sound", SoundClass());
	raylibModule.SetValue("AudioStream", AudioStreamClass());
	raylibModule.SetValue("RenderTexture", RenderTextureClass());
	raylibModule.SetValue("Shader", ShaderClass());
	raylibModule.SetValue("Mesh", MeshClass());
	raylibModule.SetValue("Material", MaterialClass());
	raylibModule.SetValue("Model", ModelClass());
	raylibModule.SetValue("ModelAnimation", ModelAnimationClass());
	raylibModule.SetValue("Camera3D", Camera3DClass());
}

// Convert a Raylib Texture to a MiniScript map
// Allocates the Texture on the heap and stores it in a GC handle in _handle
Value TextureToValue(Texture texture) {
	ValueDict map;
	map.SetValue(Value::magicIsA, TextureClass());
	map.SetValue(kHandleKey(), NewNativeHandle(texture));
	map.SetValue(String("id"), Value((int)texture.id));
	map.SetValue(String("width"), Value(texture.width));
	map.SetValue(String("height"), Value(texture.height));
	map.SetValue(String("mipmaps"), Value(texture.mipmaps));
	map.SetValue(String("format"), Value(texture.format));
	return DynamicMap(map);
}

// Extract a Raylib Texture from a MiniScript map
// Returns the Texture from the _handle handle
Texture ValueToTexture(Value value) {
	if (value.Type() != ValueType::Map) {
		// Return empty texture if not a map
		return Texture{0, 0, 0, 0, 0};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Texture* texPtr = NativeHandlePtr<Texture>(handleVal);
	if (texPtr == nullptr) {
		return Texture{0, 0, 0, 0, 0};
	}
	return *texPtr;
}

// Convert a Raylib Image to a MiniScript map
// Allocates the Image on the heap and stores it in a GC handle in _handle
Value ImageToValue(Image image) {
	ValueDict map;
	map.SetValue(Value::magicIsA, ImageClass());
	map.SetValue(kHandleKey(), NewNativeHandle(image));
	map.SetValue(String("width"), Value(image.width));
	map.SetValue(String("height"), Value(image.height));
	map.SetValue(String("mipmaps"), Value(image.mipmaps));
	map.SetValue(String("format"), Value(image.format));
	return DynamicMap(map);
}

// Extract a Raylib Image from a MiniScript map (read-only reference)
const Image& ValueToImage(Value value) {
	static const Image empty = {nullptr, 0, 0, 0, 0};
	if (value.Type() != ValueType::Map) return empty;
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Image* imgPtr = NativeHandlePtr<Image>(handleVal);
	if (imgPtr == nullptr) return empty;
	return *imgPtr;
}

// Extract a mutable pointer to a Raylib Image from a MiniScript map
Image* ValueToImagePtr(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	return NativeHandlePtr<Image>(handleVal);
}

// After mutating an Image, sync its properties back to the MiniScript map
void UpdateImageValue(Value value) {
	if (value.Type() != ValueType::Map) return;
	Image* imgPtr = ValueToImagePtr(value);
	if (!imgPtr) return;
	ValueDict map = value.GetDict();
	map.SetValue(String("width"), Value(imgPtr->width));
	map.SetValue(String("height"), Value(imgPtr->height));
	map.SetValue(String("mipmaps"), Value(imgPtr->mipmaps));
	map.SetValue(String("format"), Value(imgPtr->format));
}

// Convert a Raylib Font to a MiniScript map
Value FontToValue(Font font) {
	ValueDict map;
	map.SetValue(Value::magicIsA, FontClass());
	map.SetValue(kHandleKey(), NewNativeHandle(font));
	map.SetValue(String("texture"), TextureToValue(font.texture));
	map.SetValue(String("baseSize"), Value(font.baseSize));
	map.SetValue(String("glyphCount"), Value(font.glyphCount));
	map.SetValue(String("glyphPadding"), Value(font.glyphPadding));
	return DynamicMap(map);
}

// Extract a Raylib Font from a MiniScript map
Font ValueToFont(Value value) {
	if (value.Type() != ValueType::Map) {
		// Return default font if not a map
		printf("ValueToFont: value is not a map, returning default font\n");
		return GetFontDefault();
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Font* fontPtr = NativeHandlePtr<Font>(handleVal);
	if (fontPtr == nullptr) {
		// If no (live) handle, return default font
		printf("ValueToFont: no live font handle, returning default font\n");
		return GetFontDefault();
	}
	Font font = *fontPtr;
	return font;
}

// Convert a Raylib Wave to a MiniScript map
Value WaveToValue(Wave wave) {
	ValueDict map;
	map.SetValue(Value::magicIsA, WaveClass());
	map.SetValue(kHandleKey(), NewNativeHandle(wave));
	map.SetValue(String("frameCount"), Value((int)wave.frameCount));
	map.SetValue(String("sampleRate"), Value((int)wave.sampleRate));
	map.SetValue(String("sampleSize"), Value((int)wave.sampleSize));
	map.SetValue(String("channels"), Value((int)wave.channels));
	return DynamicMap(map);
}

// Extract a Raylib Wave from a MiniScript map
Wave ValueToWave(Value value) {
	if (value.Type() != ValueType::Map) {
		return Wave{0, 0, 0, 0, NULL};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Wave* wavePtr = NativeHandlePtr<Wave>(handleVal);
	if (wavePtr == nullptr) {
		return Wave{0, 0, 0, 0, NULL};
	}
	return *wavePtr;
}

// Convert a Raylib Music to a MiniScript map
Value MusicToValue(Music music) {
	ValueDict map;
	map.SetValue(Value::magicIsA, MusicClass());
	map.SetValue(kHandleKey(), NewNativeHandle(music));
	map.SetValue(String("frameCount"), Value((int)music.frameCount));
	map.SetValue(String("looping"), Value(music.looping ? 1 : 0));
	map.SetValue(String("sampleRate"), Value((int)music.stream.sampleRate));
	map.SetValue(String("sampleSize"), Value((int)music.stream.sampleSize));
	map.SetValue(String("channels"), Value((int)music.stream.channels));
	return DynamicMap(map);
}

// Extract a Raylib Music from a MiniScript map
Music ValueToMusic(Value value) {
	if (value.Type() != ValueType::Map) {
		return Music{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Music* musicPtr = NativeHandlePtr<Music>(handleVal);
	if (musicPtr == nullptr) {
		return Music{};
	}
	return *musicPtr;
}

// Convert a Raylib Sound to a MiniScript map
Value SoundToValue(Sound sound) {
	ValueDict map;
	map.SetValue(Value::magicIsA, SoundClass());
	map.SetValue(kHandleKey(), NewNativeHandle(sound));
	map.SetValue(String("frameCount"), Value((int)sound.frameCount));
	map.SetValue(String("sampleRate"), Value((int)sound.stream.sampleRate));
	map.SetValue(String("sampleSize"), Value((int)sound.stream.sampleSize));
	map.SetValue(String("channels"), Value((int)sound.stream.channels));
	return DynamicMap(map);
}

// Extract a Raylib Sound from a MiniScript map
Sound ValueToSound(Value value) {
	if (value.Type() != ValueType::Map) {
		return Sound{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Sound* soundPtr = NativeHandlePtr<Sound>(handleVal);
	if (soundPtr == nullptr) {
		return Sound{};
	}
	return *soundPtr;
}

// Convert a Raylib AudioStream to a MiniScript map
Value AudioStreamToValue(AudioStream stream) {
	ValueDict map;
	map.SetValue(Value::magicIsA, AudioStreamClass());
	map.SetValue(kHandleKey(), NewNativeHandle(stream));
	map.SetValue(String("sampleRate"), Value((int)stream.sampleRate));
	map.SetValue(String("sampleSize"), Value((int)stream.sampleSize));
	map.SetValue(String("channels"), Value((int)stream.channels));
	return DynamicMap(map);
}

// Extract a Raylib AudioStream from a MiniScript map
AudioStream ValueToAudioStream(Value value) {
	if (value.Type() != ValueType::Map) {
		return AudioStream{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	AudioStream* streamPtr = NativeHandlePtr<AudioStream>(handleVal);
	if (streamPtr == nullptr) {
		return AudioStream{};
	}
	return *streamPtr;
}

// Convert a Raylib RenderTexture2D to a MiniScript map
// Allocates the RenderTexture2D on the heap and stores it in a GC handle in _handle
Value RenderTextureToValue(RenderTexture2D renderTexture) {
	ValueDict map;
	map.SetValue(Value::magicIsA, RenderTextureClass());
	map.SetValue(kHandleKey(), NewNativeHandle(renderTexture));
	map.SetValue(String("id"), Value((int)renderTexture.id));
	map.SetValue(String("texture"), TextureToValue(renderTexture.texture));
	return DynamicMap(map);
}

// Extract a Raylib RenderTexture2D from a MiniScript map
// Returns the RenderTexture2D from the _handle handle
RenderTexture2D ValueToRenderTexture(Value value) {
	if (value.Type() != ValueType::Map) {
		return RenderTexture2D{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	RenderTexture2D* rtPtr = NativeHandlePtr<RenderTexture2D>(handleVal);
	if (rtPtr == nullptr) {
		return RenderTexture2D{};
	}
	return *rtPtr;
}

// Convert a Raylib Shader to a MiniScript map
Value ShaderToValue(Shader shader) {
	ValueDict map;
	map.SetValue(Value::magicIsA, ShaderClass());
	map.SetValue(kHandleKey(), NewNativeHandle(shader));
	map.SetValue(String("id"), Value((int)shader.id));
	return DynamicMap(map);
}

// Extract a Raylib Shader from a MiniScript map
Shader ValueToShader(Value value) {
	if (value.Type() != ValueType::Map) return Shader{0, NULL};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Shader* shaderPtr = NativeHandlePtr<Shader>(handleVal);
	if (shaderPtr == nullptr) return Shader{0, NULL};
	return *shaderPtr;
}

// Convert a MiniScript map to a Raylib Color
// Expects a map with "r", "g", "b", and optionally "a" keys (0-255);
// or, a 3- or 4-element list in the order [r, g, b, a].
Color ValueToColor(Value value) {
	Color result;

	// Handle HTML-style color string: "#RRGGBB" or "#RRGGBBAA"
	if (value.Type() == ValueType::String) {
		String s = value.ToString();
		if (s.Length() >= 7 && s[0] == '#') {
			unsigned int hex = 0;
			for (int i = 1; i < s.Length() && i < 9; i++) {
				char c = s[i];
				int digit;
				if (c >= '0' && c <= '9') digit = c - '0';
				else if (c >= 'a' && c <= 'f') digit = 10 + c - 'a';
				else if (c >= 'A' && c <= 'F') digit = 10 + c - 'A';
				else break;
				hex = (hex << 4) | digit;
			}
			if (s.Length() >= 9) {
				// #RRGGBBAA
				result.r = (hex >> 24) & 0xFF;
				result.g = (hex >> 16) & 0xFF;
				result.b = (hex >> 8) & 0xFF;
				result.a = hex & 0xFF;
			} else {
				// #RRGGBB
				result.r = (hex >> 16) & 0xFF;
				result.g = (hex >> 8) & 0xFF;
				result.b = hex & 0xFF;
				result.a = 255;
			}
			return result;
		}
	}

	// Handle list format: [r, g, b, a] or [r, g, b]
	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		if (list.Count() >= 3) {
			result.r = (unsigned char)(list[0].IntValue());
			result.g = (unsigned char)(list[1].IntValue());
			result.b = (unsigned char)(list[2].IntValue());
			result.a = list.Count() >= 4 ? (unsigned char)(list[3].IntValue()) : 255;
			return result;
		}
		// If list has fewer than 3 elements, fall through to default
	}

	// Handle map format: {"r": r, "g": g, "b": b, "a": a}
	if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();

		Value rVal = map.Lookup(String("r"), Value::zero);
		Value gVal = map.Lookup(String("g"), Value::zero);
		Value bVal = map.Lookup(String("b"), Value::zero);
		Value aVal = map.Lookup(String("a"), Value::Null);

		result.r = (unsigned char)(rVal.IntValue());
		result.g = (unsigned char)(gVal.IntValue());
		result.b = (unsigned char)(bVal.IntValue());
		result.a = aVal.IsNull() ? 255 : (unsigned char)(aVal.IntValue());

		return result;
	}

	// Default to white if neither list nor map
	return WHITE;
}

// Convert a Raylib Color to a MiniScript map
Value ColorToValue(Color color) {
	ValueDict map;
	map.SetValue(String("r"), Value((int)color.r));
	map.SetValue(String("g"), Value((int)color.g));
	map.SetValue(String("b"), Value((int)color.b));
	map.SetValue(String("a"), Value((int)color.a));
	return DynamicMap(map);
}

// Convert a MiniScript value to a Raylib Rectangle
// Accepts either a map with "x", "y", "width", "height" keys OR a list with 4 elements
Rectangle ValueToRectangle(Value value) {
	if (value.Type() == ValueType::List) {
		// List format: [x, y, width, height]
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		float width = (list.Count() > 2) ? list[2].FloatValue() : 0;
		float height = (list.Count() > 3) ? list[3].FloatValue() : 0;
		return Rectangle{x, y, width, height};
	} else if (value.Type() == ValueType::Map) {
		// Map format: {x: ..., y: ..., width: ..., height: ...}
		ValueDict map = value.GetDict();
		Value xVal = map.Lookup(String("x"), Value::zero);
		Value yVal = map.Lookup(String("y"), Value::zero);
		Value widthVal = map.Lookup(String("width"), Value::zero);
		Value heightVal = map.Lookup(String("height"), Value::zero);

		Rectangle result;
		result.x = xVal.FloatValue();
		result.y = yVal.FloatValue();
		result.width = widthVal.FloatValue();
		result.height = heightVal.FloatValue();

		return result;
	} else {
		// Default to empty rectangle if not a map or list
		return Rectangle{0, 0, 0, 0};
	}
}

// Convert a Raylib Rectangle to a MiniScript map
Value RectangleToValue(Rectangle rect) {
	ValueDict map;
	map.SetValue(String("x"), Value(rect.x));
	map.SetValue(String("y"), Value(rect.y));
	map.SetValue(String("width"), Value(rect.width));
	map.SetValue(String("height"), Value(rect.height));
	return DynamicMap(map);
}

// Convert a MiniScript value to a Raylib Vector2
// Accepts either a map with "x", "y" keys OR a list with 2 elements
Vector2 ValueToVector2(Value value) {
	if (value.Type() == ValueType::List) {
		// List format: [x, y]
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		return Vector2{x, y};
	} else if (value.Type() == ValueType::Map) {
		// Map format: {x: ..., y: ...}
		ValueDict map = value.GetDict();
		Value xVal = map.Lookup(String("x"), Value::zero);
		Value yVal = map.Lookup(String("y"), Value::zero);
		return Vector2{xVal.FloatValue(), yVal.FloatValue()};
	} else {
		// Default to zero vector if not a map or list
		return Vector2{0, 0};
	}
}

// Convert a Raylib Vector2 to a MiniScript map
Value Vector2ToValue(Vector2 vec) {
	ValueDict map;
	map.SetValue(String("x"), Value(vec.x));
	map.SetValue(String("y"), Value(vec.y));
	return DynamicMap(map);
}

Value MeshToValue(Mesh mesh) {
	ValueDict map;
	map.SetValue(Value::magicIsA, MeshClass());
	map.SetValue(kHandleKey(), NewNativeHandle(mesh));
	map.SetValue(String("vertexCount"), Value(mesh.vertexCount));
	map.SetValue(String("triangleCount"), Value(mesh.triangleCount));
	return DynamicMap(map);
}

Mesh ValueToMesh(Value value) {
	if (value.Type() != ValueType::Map) return Mesh{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Mesh* meshPtr = NativeHandlePtr<Mesh>(handleVal);
	if (meshPtr == nullptr) return Mesh{};
	return *meshPtr;
}

Value MaterialToValue(Material material) {
	ValueDict map;
	map.SetValue(Value::magicIsA, MaterialClass());
	map.SetValue(kHandleKey(), NewNativeHandle(material));
	map.SetValue(String("shaderId"), Value((int)material.shader.id));
	map.SetValue(String("_arrayHandle"), Value::Null);
	map.SetValue(String("_arrayCount"), Value::zero);
	map.SetValue(String("_arrayIndex"), Value::zero);
	return DynamicMap(map);
}

Material ValueToMaterial(Value value) {
	if (value.Type() != ValueType::Map) return Material{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Material* materialPtr = NativeHandlePtr<Material>(handleVal);
	if (materialPtr == nullptr) return Material{};
	return *materialPtr;
}

Value ModelToValue(Model model) {
	ValueDict map;
	map.SetValue(Value::magicIsA, ModelClass());
	map.SetValue(kHandleKey(), NewNativeHandle(model));
	map.SetValue(String("meshCount"), Value(model.meshCount));
	map.SetValue(String("materialCount"), Value(model.materialCount));
	return DynamicMap(map);
}

Model ValueToModel(Value value) {
	if (value.Type() != ValueType::Map) return Model{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	Model* modelPtr = NativeHandlePtr<Model>(handleVal);
	if (modelPtr == nullptr) return Model{};
	return *modelPtr;
}

Value ModelAnimationToValue(ModelAnimation anim) {
	ValueDict map;
	map.SetValue(Value::magicIsA, ModelAnimationClass());
	map.SetValue(kHandleKey(), NewNativeHandle(anim));
	map.SetValue(String("name"), Value(String(anim.name)));
	map.SetValue(String("boneCount"), Value(anim.boneCount));
	map.SetValue(String("keyframeCount"), Value(anim.keyframeCount));
	map.SetValue(String("_arrayHandle"), Value::Null);
	map.SetValue(String("_arrayCount"), Value::zero);
	map.SetValue(String("_arrayIndex"), Value::zero);
	return DynamicMap(map);
}

ModelAnimation ValueToModelAnimation(Value value) {
	if (value.Type() != ValueType::Map) return ModelAnimation{};
	ValueDict map = value.GetDict();
	// An item of a LoadModelAnimations array: bounds-check against the array's own count.
	ModelAnimationArray* arr = NativePtrFromMapKey<ModelAnimationArray>(map, String("_arrayHandle"));
	if (arr != nullptr) {
		int index = map.Lookup(String("_arrayIndex"), Value::zero).IntValue();
		if (index < 0 || index >= arr->count) return ModelAnimation{};
		return arr->anims[index];
	}
	Value handleVal = map.Lookup(kHandleKey(), Value::Null);
	ModelAnimation* animPtr = NativeHandlePtr<ModelAnimation>(handleVal);
	if (animPtr == nullptr) return ModelAnimation{};
	return *animPtr;
}

Vector3 ValueToVector3(Value value) {
	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		float z = (list.Count() > 2) ? list[2].FloatValue() : 0;
		return Vector3{x, y, z};
	} else if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();
		float x = map.Lookup(String("x"), Value::zero).FloatValue();
		float y = map.Lookup(String("y"), Value::zero).FloatValue();
		float z = map.Lookup(String("z"), Value::zero).FloatValue();
		return Vector3{x, y, z};
	}
	return Vector3{0, 0, 0};
}

Value Vector3ToValue(Vector3 vec) {
	ValueDict map;
	map.SetValue(String("x"), Value(vec.x));
	map.SetValue(String("y"), Value(vec.y));
	map.SetValue(String("z"), Value(vec.z));
	return DynamicMap(map);
}

Camera3D ValueToCamera3D(Value value) {
	Camera3D camera;
	camera.position = Vector3{0, 10, 10};
	camera.target = Vector3{0, 0, 0};
	camera.up = Vector3{0, 1, 0};
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	if (value.Type() != ValueType::Map) return camera;

	ValueDict map = value.GetDict();

	Value positionVal = map.Lookup(String("position"), Value::Null);
	if (positionVal.Type() == ValueType::Map || positionVal.Type() == ValueType::List) {
		camera.position = ValueToVector3(positionVal);
	} else {
		camera.position.x = map.Lookup(String("positionX"), Value(camera.position.x)).FloatValue();
		camera.position.y = map.Lookup(String("positionY"), Value(camera.position.y)).FloatValue();
		camera.position.z = map.Lookup(String("positionZ"), Value(camera.position.z)).FloatValue();
	}

	Value targetVal = map.Lookup(String("target"), Value::Null);
	if (targetVal.Type() == ValueType::Map || targetVal.Type() == ValueType::List) {
		camera.target = ValueToVector3(targetVal);
	} else {
		camera.target.x = map.Lookup(String("targetX"), Value(camera.target.x)).FloatValue();
		camera.target.y = map.Lookup(String("targetY"), Value(camera.target.y)).FloatValue();
		camera.target.z = map.Lookup(String("targetZ"), Value(camera.target.z)).FloatValue();
	}

	Value upVal = map.Lookup(String("up"), Value::Null);
	if (upVal.Type() == ValueType::Map || upVal.Type() == ValueType::List) {
		camera.up = ValueToVector3(upVal);
	} else {
		camera.up.x = map.Lookup(String("upX"), Value(camera.up.x)).FloatValue();
		camera.up.y = map.Lookup(String("upY"), Value(camera.up.y)).FloatValue();
		camera.up.z = map.Lookup(String("upZ"), Value(camera.up.z)).FloatValue();
	}

	camera.fovy = map.Lookup(String("fovy"), Value(camera.fovy)).FloatValue();
	camera.projection = map.Lookup(String("projection"), Value(camera.projection)).IntValue();

	return camera;
}

Value Camera3DToValue(Camera3D camera) {
	ValueDict map;
	map.SetValue(Value::magicIsA, Camera3DClass());
	map.SetValue(String("position"), Vector3ToValue(camera.position));
	map.SetValue(String("target"), Vector3ToValue(camera.target));
	map.SetValue(String("up"), Vector3ToValue(camera.up));
	map.SetValue(String("fovy"), Value(camera.fovy));
	map.SetValue(String("projection"), Value(camera.projection));

	// Convenience flattened fields for scripts that prefer direct scalars.
	map.SetValue(String("positionX"), Value(camera.position.x));
	map.SetValue(String("positionY"), Value(camera.position.y));
	map.SetValue(String("positionZ"), Value(camera.position.z));
	map.SetValue(String("targetX"), Value(camera.target.x));
	map.SetValue(String("targetY"), Value(camera.target.y));
	map.SetValue(String("targetZ"), Value(camera.target.z));
	map.SetValue(String("upX"), Value(camera.up.x));
	map.SetValue(String("upY"), Value(camera.up.y));
	map.SetValue(String("upZ"), Value(camera.up.z));

	return DynamicMap(map);
}

Vector4 ValueToVector4(Value value) {
	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		float z = (list.Count() > 2) ? list[2].FloatValue() : 0;
		float w = (list.Count() > 3) ? list[3].FloatValue() : 0;
		return Vector4{x, y, z, w};
	} else if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();
		float x = map.Lookup(String("x"), Value::zero).FloatValue();
		float y = map.Lookup(String("y"), Value::zero).FloatValue();
		float z = map.Lookup(String("z"), Value::zero).FloatValue();
		float w = map.Lookup(String("w"), Value::zero).FloatValue();
		return Vector4{x, y, z, w};
	}
	return Vector4{0, 0, 0, 0};
}

Value Vector4ToValue(Vector4 vec) {
	ValueDict map;
	map.SetValue(String("x"), Value(vec.x));
	map.SetValue(String("y"), Value(vec.y));
	map.SetValue(String("z"), Value(vec.z));
	map.SetValue(String("w"), Value(vec.w));
	return DynamicMap(map);
}

Quaternion ValueToQuaternion(Value value) {
	Vector4 v = ValueToVector4(value);
	return Quaternion{v.x, v.y, v.z, v.w};
}

Value QuaternionToValue(Quaternion q) {
	return Vector4ToValue(Vector4{q.x, q.y, q.z, q.w});
}

Matrix ValueToMatrix(Value value) {
	Matrix identity = Matrix{1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};

	auto assignFromFlatList = [&](const ValueList &list) {
		Matrix m = identity;
		if (list.Count() > 0) m.m0 = list[0].FloatValue();
		if (list.Count() > 1) m.m1 = list[1].FloatValue();
		if (list.Count() > 2) m.m2 = list[2].FloatValue();
		if (list.Count() > 3) m.m3 = list[3].FloatValue();
		if (list.Count() > 4) m.m4 = list[4].FloatValue();
		if (list.Count() > 5) m.m5 = list[5].FloatValue();
		if (list.Count() > 6) m.m6 = list[6].FloatValue();
		if (list.Count() > 7) m.m7 = list[7].FloatValue();
		if (list.Count() > 8) m.m8 = list[8].FloatValue();
		if (list.Count() > 9) m.m9 = list[9].FloatValue();
		if (list.Count() > 10) m.m10 = list[10].FloatValue();
		if (list.Count() > 11) m.m11 = list[11].FloatValue();
		if (list.Count() > 12) m.m12 = list[12].FloatValue();
		if (list.Count() > 13) m.m13 = list[13].FloatValue();
		if (list.Count() > 14) m.m14 = list[14].FloatValue();
		if (list.Count() > 15) m.m15 = list[15].FloatValue();
		return m;
	};

	auto assignFromNestedList = [&](const ValueList &rows) {
		Matrix m = identity;
		if (rows.Count() > 0 && rows[0].Type() == ValueType::List) {
			ValueList row = rows[0].GetList();
			if (row.Count() > 0) m.m0 = row[0].FloatValue();
			if (row.Count() > 1) m.m4 = row[1].FloatValue();
			if (row.Count() > 2) m.m8 = row[2].FloatValue();
			if (row.Count() > 3) m.m12 = row[3].FloatValue();
		}
		if (rows.Count() > 1 && rows[1].Type() == ValueType::List) {
			ValueList row = rows[1].GetList();
			if (row.Count() > 0) m.m1 = row[0].FloatValue();
			if (row.Count() > 1) m.m5 = row[1].FloatValue();
			if (row.Count() > 2) m.m9 = row[2].FloatValue();
			if (row.Count() > 3) m.m13 = row[3].FloatValue();
		}
		if (rows.Count() > 2 && rows[2].Type() == ValueType::List) {
			ValueList row = rows[2].GetList();
			if (row.Count() > 0) m.m2 = row[0].FloatValue();
			if (row.Count() > 1) m.m6 = row[1].FloatValue();
			if (row.Count() > 2) m.m10 = row[2].FloatValue();
			if (row.Count() > 3) m.m14 = row[3].FloatValue();
		}
		if (rows.Count() > 3 && rows[3].Type() == ValueType::List) {
			ValueList row = rows[3].GetList();
			if (row.Count() > 0) m.m3 = row[0].FloatValue();
			if (row.Count() > 1) m.m7 = row[1].FloatValue();
			if (row.Count() > 2) m.m11 = row[2].FloatValue();
			if (row.Count() > 3) m.m15 = row[3].FloatValue();
		}
		return m;
	};

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		if (list.Count() > 0 && list[0].Type() == ValueType::List) {
			return assignFromNestedList(list);
		}
		return assignFromFlatList(list);
	}

	if (value.Type() == ValueType::Map) {
		ValueDict map = value.GetDict();
		bool hasLegacyFields =
			map.Lookup(String("m0"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m1"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m2"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m3"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m4"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m5"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m6"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m7"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m8"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m9"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m10"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m11"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m12"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m13"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m14"), Value::Null).Type() != ValueType::Null ||
			map.Lookup(String("m15"), Value::Null).Type() != ValueType::Null;

		if (hasLegacyFields) {
			Matrix m;
			m.m0 = map.Lookup(String("m0"), Value::zero).FloatValue();
			m.m1 = map.Lookup(String("m1"), Value::zero).FloatValue();
			m.m2 = map.Lookup(String("m2"), Value::zero).FloatValue();
			m.m3 = map.Lookup(String("m3"), Value::zero).FloatValue();
			m.m4 = map.Lookup(String("m4"), Value::zero).FloatValue();
			m.m5 = map.Lookup(String("m5"), Value::zero).FloatValue();
			m.m6 = map.Lookup(String("m6"), Value::zero).FloatValue();
			m.m7 = map.Lookup(String("m7"), Value::zero).FloatValue();
			m.m8 = map.Lookup(String("m8"), Value::zero).FloatValue();
			m.m9 = map.Lookup(String("m9"), Value::zero).FloatValue();
			m.m10 = map.Lookup(String("m10"), Value::zero).FloatValue();
			m.m11 = map.Lookup(String("m11"), Value::zero).FloatValue();
			m.m12 = map.Lookup(String("m12"), Value::zero).FloatValue();
			m.m13 = map.Lookup(String("m13"), Value::zero).FloatValue();
			m.m14 = map.Lookup(String("m14"), Value::zero).FloatValue();
			m.m15 = map.Lookup(String("m15"), Value::zero).FloatValue();
			return m;
		}

		Value elemValue = map.Lookup(String("elem"), Value::Null);
		if (elemValue.Type() == ValueType::List) {
			return assignFromNestedList(elemValue.GetList());
		}
	}

	return identity;
}

Value MatrixToValue(Matrix mat) {
	ValueDict result;

	ValueList row0;
	row0.Add(Value(mat.m0));
	row0.Add(Value(mat.m4));
	row0.Add(Value(mat.m8));
	row0.Add(Value(mat.m12));

	ValueList row1;
	row1.Add(Value(mat.m1));
	row1.Add(Value(mat.m5));
	row1.Add(Value(mat.m9));
	row1.Add(Value(mat.m13));

	ValueList row2;
	row2.Add(Value(mat.m2));
	row2.Add(Value(mat.m6));
	row2.Add(Value(mat.m10));
	row2.Add(Value(mat.m14));

	ValueList row3;
	row3.Add(Value(mat.m3));
	row3.Add(Value(mat.m7));
	row3.Add(Value(mat.m11));
	row3.Add(Value(mat.m15));

	ValueList elem;
	elem.Add(DynamicList(row0));
	elem.Add(DynamicList(row1));
	elem.Add(DynamicList(row2));
	elem.Add(DynamicList(row3));

	result.SetValue(String("rows"), Value(4));
	result.SetValue(String("columns"), Value(4));
	result.SetValue(String("elem"), DynamicList(elem));
	return DynamicMap(result);
}

BoundingBox ValueToBoundingBox(Value value) {
	if (value.Type() != ValueType::Map) return BoundingBox{};
	ValueDict map = value.GetDict();
	BoundingBox box;
	box.min = ValueToVector3(map.Lookup(String("min"), Value::Null));
	box.max = ValueToVector3(map.Lookup(String("max"), Value::Null));
	return box;
}

Value BoundingBoxToValue(BoundingBox box) {
	ValueDict map;
	map.SetValue(String("min"), Vector3ToValue(box.min));
	map.SetValue(String("max"), Vector3ToValue(box.max));
	return DynamicMap(map);
}

Ray ValueToRay(Value value) {
	if (value.Type() != ValueType::Map) return Ray{};
	ValueDict map = value.GetDict();
	Ray ray;
	ray.position = ValueToVector3(map.Lookup(String("position"), map.Lookup(String("origin"), Value::Null)));
	ray.direction = ValueToVector3(map.Lookup(String("direction"), Value::Null));
	return ray;
}

Value RayToValue(Ray ray) {
	ValueDict map;
	map.SetValue(String("position"), Vector3ToValue(ray.position));
	map.SetValue(String("direction"), Vector3ToValue(ray.direction));
	return DynamicMap(map);
}

Value RayCollisionToValue(RayCollision collision) {
	ValueDict map;
	map.SetValue(String("hit"), Value(collision.hit));
	map.SetValue(String("distance"), Value(collision.distance));
	map.SetValue(String("point"), Vector3ToValue(collision.point));
	map.SetValue(String("normal"), Vector3ToValue(collision.normal));
	return DynamicMap(map);
}
