#include "RaylibTypes.h"

ValueDict ImageClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("width"), Value::zero);
		map.SetValue(String("height"), Value::zero);
		map.SetValue(String("mipmaps"), Value::zero);
		map.SetValue(String("format"), Value::zero);
	}
	return map;
}

ValueDict TextureClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("id"), Value::zero);
		map.SetValue(String("width"), Value::zero);
		map.SetValue(String("height"), Value::zero);
		map.SetValue(String("mipmaps"), Value::zero);
		map.SetValue(String("format"), Value::zero);
	}
	return map;
}

ValueDict FontClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("texture"), Value::null);
		map.SetValue(String("baseSize"), Value::zero);
		map.SetValue(String("glyphCount"), Value::zero);
		map.SetValue(String("glyphPadding"), Value::zero);
	}
	return map;
}

ValueDict WaveClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("frameCount"), Value::zero);
		map.SetValue(String("sampleRate"), Value::zero);
		map.SetValue(String("sampleSize"), Value::zero);
		map.SetValue(String("channels"), Value::zero);
	}
	return map;
}

ValueDict MusicClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("frameCount"), Value::zero);
		map.SetValue(String("looping"), Value::zero);
	}
	return map;
}

ValueDict SoundClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("frameCount"), Value::zero);
	}
	return map;
}

ValueDict AudioStreamClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("sampleRate"), Value::zero);
		map.SetValue(String("sampleSize"), Value::zero);
		map.SetValue(String("channels"), Value::zero);
	}
	return map;
}

ValueDict RenderTextureClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("id"), Value::zero);
		map.SetValue(String("texture"), Value::zero);
	}
	return map;
}

ValueDict MeshClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("vertexCount"), Value::zero);
		map.SetValue(String("triangleCount"), Value::zero);
	}
	return map;
}

ValueDict MaterialClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("shaderId"), Value::zero);
		map.SetValue(String("_arrayHandle"), Value::zero);
		map.SetValue(String("_arrayCount"), Value::zero);
		map.SetValue(String("_arrayIndex"), Value::zero);
	}
	return map;
}

ValueDict ModelClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("meshCount"), Value::zero);
		map.SetValue(String("materialCount"), Value::zero);
	}
	return map;
}

ValueDict ModelAnimationClass() {
	static ValueDict map;
	if (map.Count() == 0) {
		map.SetValue(String("_handle"), Value::zero);
		map.SetValue(String("name"), Value::null);
		map.SetValue(String("boneCount"), Value::zero);
		map.SetValue(String("keyframeCount"), Value::zero);
		map.SetValue(String("_arrayHandle"), Value::zero);
		map.SetValue(String("_arrayCount"), Value::zero);
		map.SetValue(String("_arrayIndex"), Value::zero);
	}
	return map;
}

// Convert a Raylib Texture to a MiniScript map
// Allocates the Texture on the heap and stores pointer in _handle
Value TextureToValue(Texture texture) {
	Texture* texPtr = new Texture(texture);
	ValueDict map;
	map.SetValue(Value::magicIsA, TextureClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)texPtr));
	map.SetValue(String("id"), Value((int)texture.id));
	map.SetValue(String("width"), Value(texture.width));
	map.SetValue(String("height"), Value(texture.height));
	map.SetValue(String("mipmaps"), Value(texture.mipmaps));
	map.SetValue(String("format"), Value(texture.format));
	return Value(map);
}

// Extract a Raylib Texture from a MiniScript map
// Returns the Texture by dereferencing the _handle pointer
Texture ValueToTexture(Value value) {
	if (value.type != ValueType::Map) {
		// Return empty texture if not a map
		return Texture{0, 0, 0, 0, 0};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Texture* texPtr = (Texture*)ValueToPointer(handleVal);
	if (texPtr == nullptr) {
		return Texture{0, 0, 0, 0, 0};
	}
	return *texPtr;
}

// Convert a Raylib Image to a MiniScript map
// Allocates the Image on the heap and stores pointer in _handle
Value ImageToValue(Image image) {
	Image* imgPtr = new Image(image);
	ValueDict map;
	map.SetValue(Value::magicIsA, ImageClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)imgPtr));
	map.SetValue(String("width"), Value(image.width));
	map.SetValue(String("height"), Value(image.height));
	map.SetValue(String("mipmaps"), Value(image.mipmaps));
	map.SetValue(String("format"), Value(image.format));
	return Value(map);
}

// Extract a Raylib Image from a MiniScript map (read-only reference)
const Image& ValueToImage(Value value) {
	static const Image empty = {nullptr, 0, 0, 0, 0};
	if (value.type != ValueType::Map) return empty;
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Image* imgPtr = (Image*)ValueToPointer(handleVal);
	if (imgPtr == nullptr) return empty;
	return *imgPtr;
}

// Extract a mutable pointer to a Raylib Image from a MiniScript map
Image* ValueToImagePtr(Value value) {
	if (value.type != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	return (Image*)ValueToPointer(handleVal);
}

// After mutating an Image, sync its properties back to the MiniScript map
void UpdateImageValue(Value value) {
	if (value.type != ValueType::Map) return;
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
	Font* fontPtr = new Font(font);
	ValueDict map;
	map.SetValue(Value::magicIsA, FontClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)fontPtr));
	map.SetValue(String("texture"), TextureToValue(font.texture));
	map.SetValue(String("baseSize"), Value(font.baseSize));
	map.SetValue(String("glyphCount"), Value(font.glyphCount));
	map.SetValue(String("glyphPadding"), Value(font.glyphPadding));
	return Value(map);
}

// Extract a Raylib Font from a MiniScript map
Font ValueToFont(Value value) {
	if (value.type != ValueType::Map) {
		// Return default font if not a map
		printf("ValueToFont: value is not a map, returning default font\n");
		return GetFontDefault();
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	intptr_t handle = (intptr_t)ValueToPointer(handleVal);
	if (handle == 0) {
		// If no handle, return default font
		printf("ValueToFont: handle is 0, returning default font\n");
		return GetFontDefault();
	}
	Font* fontPtr = (Font*)handle;
	if (fontPtr == nullptr) {
		printf("ValueToFont: fontPtr is null, returning default font\n");
		return GetFontDefault();
	}
	Font font = *fontPtr;
	return font;
}

// Convert a Raylib Wave to a MiniScript map
Value WaveToValue(Wave wave) {
	Wave* wavePtr = new Wave(wave);
	ValueDict map;
	map.SetValue(Value::magicIsA, WaveClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)wavePtr));
	map.SetValue(String("frameCount"), Value((int)wave.frameCount));
	map.SetValue(String("sampleRate"), Value((int)wave.sampleRate));
	map.SetValue(String("sampleSize"), Value((int)wave.sampleSize));
	map.SetValue(String("channels"), Value((int)wave.channels));
	return Value(map);
}

// Extract a Raylib Wave from a MiniScript map
Wave ValueToWave(Value value) {
	if (value.type != ValueType::Map) {
		return Wave{0, 0, 0, 0, NULL};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Wave* wavePtr = (Wave*)ValueToPointer(handleVal);
	if (wavePtr == nullptr) {
		return Wave{0, 0, 0, 0, NULL};
	}
	return *wavePtr;
}

// Convert a Raylib Music to a MiniScript map
Value MusicToValue(Music music) {
	Music* musicPtr = new Music(music);
	ValueDict map;
	map.SetValue(Value::magicIsA, MusicClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)musicPtr));
	map.SetValue(String("frameCount"), Value((int)music.frameCount));
	map.SetValue(String("looping"), Value(music.looping ? 1 : 0));
	return Value(map);
}

// Extract a Raylib Music from a MiniScript map
Music ValueToMusic(Value value) {
	if (value.type != ValueType::Map) {
		return Music{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Music* musicPtr = (Music*)ValueToPointer(handleVal);
	if (musicPtr == nullptr) {
		return Music{};
	}
	return *musicPtr;
}

// Convert a Raylib Sound to a MiniScript map
Value SoundToValue(Sound sound) {
	Sound* soundPtr = new Sound(sound);
	ValueDict map;
	map.SetValue(Value::magicIsA, SoundClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)soundPtr));
	map.SetValue(String("frameCount"), Value((int)sound.frameCount));
	return Value(map);
}

// Extract a Raylib Sound from a MiniScript map
Sound ValueToSound(Value value) {
	if (value.type != ValueType::Map) {
		return Sound{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Sound* soundPtr = (Sound*)ValueToPointer(handleVal);
	if (soundPtr == nullptr) {
		return Sound{};
	}
	return *soundPtr;
}

// Convert a Raylib AudioStream to a MiniScript map
Value AudioStreamToValue(AudioStream stream) {
	AudioStream* streamPtr = new AudioStream(stream);
	ValueDict map;
	map.SetValue(Value::magicIsA, AudioStreamClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)streamPtr));
	map.SetValue(String("sampleRate"), Value((int)stream.sampleRate));
	map.SetValue(String("sampleSize"), Value((int)stream.sampleSize));
	map.SetValue(String("channels"), Value((int)stream.channels));
	return Value(map);
}

// Extract a Raylib AudioStream from a MiniScript map
AudioStream ValueToAudioStream(Value value) {
	if (value.type != ValueType::Map) {
		return AudioStream{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	AudioStream* streamPtr = (AudioStream*)ValueToPointer(handleVal);
	if (streamPtr == nullptr) {
		return AudioStream{};
	}
	return *streamPtr;
}

// Convert a Raylib RenderTexture2D to a MiniScript map
// Allocates the RenderTexture2D on the heap and stores pointer in _handle
Value RenderTextureToValue(RenderTexture2D renderTexture) {
	RenderTexture2D* rtPtr = new RenderTexture2D(renderTexture);
	ValueDict map;
	map.SetValue(Value::magicIsA, RenderTextureClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)rtPtr));
	map.SetValue(String("id"), Value((int)renderTexture.id));
	map.SetValue(String("texture"), TextureToValue(renderTexture.texture));
	return Value(map);
}

// Extract a Raylib RenderTexture2D from a MiniScript map
// Returns the RenderTexture2D by dereferencing the _handle pointer
RenderTexture2D ValueToRenderTexture(Value value) {
	if (value.type != ValueType::Map) {
		return RenderTexture2D{};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	RenderTexture2D* rtPtr = (RenderTexture2D*)ValueToPointer(handleVal);
	if (rtPtr == nullptr) {
		return RenderTexture2D{};
	}
	return *rtPtr;
}

// Convert a MiniScript map to a Raylib Color
// Expects a map with "r", "g", "b", and optionally "a" keys (0-255);
// or, a 3- or 4-element list in the order [r, g, b, a].
Color ValueToColor(Value value) {
	Color result;

	// Handle list format: [r, g, b, a] or [r, g, b]
	if (value.type == ValueType::List) {
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
	if (value.type == ValueType::Map) {
		ValueDict map = value.GetDict();

		Value rVal = map.Lookup(String("r"), Value::zero);
		Value gVal = map.Lookup(String("g"), Value::zero);
		Value bVal = map.Lookup(String("b"), Value::zero);
		Value aVal = map.Lookup(String("a"), Value::null);

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
	return Value(map);
}

// Convert a MiniScript value to a Raylib Rectangle
// Accepts either a map with "x", "y", "width", "height" keys OR a list with 4 elements
Rectangle ValueToRectangle(Value value) {
	if (value.type == ValueType::List) {
		// List format: [x, y, width, height]
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		float width = (list.Count() > 2) ? list[2].FloatValue() : 0;
		float height = (list.Count() > 3) ? list[3].FloatValue() : 0;
		return Rectangle{x, y, width, height};
	} else if (value.type == ValueType::Map) {
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
	return Value(map);
}

// Convert a MiniScript value to a Raylib Vector2
// Accepts either a map with "x", "y" keys OR a list with 2 elements
Vector2 ValueToVector2(Value value) {
	if (value.type == ValueType::List) {
		// List format: [x, y]
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		return Vector2{x, y};
	} else if (value.type == ValueType::Map) {
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
	return Value(map);
}

Value MeshToValue(Mesh mesh) {
	Mesh* meshPtr = new Mesh(mesh);
	ValueDict map;
	map.SetValue(Value::magicIsA, MeshClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)meshPtr));
	map.SetValue(String("vertexCount"), Value(mesh.vertexCount));
	map.SetValue(String("triangleCount"), Value(mesh.triangleCount));
	return Value(map);
}

Mesh ValueToMesh(Value value) {
	if (value.type != ValueType::Map) return Mesh{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Mesh* meshPtr = (Mesh*)ValueToPointer(handleVal);
	if (meshPtr == nullptr) return Mesh{};
	return *meshPtr;
}

Value MaterialToValue(Material material) {
	Material* materialPtr = new Material(material);
	ValueDict map;
	map.SetValue(Value::magicIsA, MaterialClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)materialPtr));
	map.SetValue(String("shaderId"), Value((int)material.shader.id));
	map.SetValue(String("_arrayHandle"), Value::zero);
	map.SetValue(String("_arrayCount"), Value::zero);
	map.SetValue(String("_arrayIndex"), Value::zero);
	return Value(map);
}

Material ValueToMaterial(Value value) {
	if (value.type != ValueType::Map) return Material{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Material* materialPtr = (Material*)ValueToPointer(handleVal);
	if (materialPtr == nullptr) return Material{};
	return *materialPtr;
}

Value ModelToValue(Model model) {
	Model* modelPtr = new Model(model);
	ValueDict map;
	map.SetValue(Value::magicIsA, ModelClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)modelPtr));
	map.SetValue(String("meshCount"), Value(model.meshCount));
	map.SetValue(String("materialCount"), Value(model.materialCount));
	return Value(map);
}

Model ValueToModel(Value value) {
	if (value.type != ValueType::Map) return Model{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Model* modelPtr = (Model*)ValueToPointer(handleVal);
	if (modelPtr == nullptr) return Model{};
	return *modelPtr;
}

Value ModelAnimationToValue(ModelAnimation anim) {
	ModelAnimation* animPtr = new ModelAnimation(anim);
	ValueDict map;
	map.SetValue(Value::magicIsA, ModelAnimationClass());
	map.SetValue(String("_handle"), Value((double)(intptr_t)animPtr));
	map.SetValue(String("name"), Value(String(anim.name)));
	map.SetValue(String("boneCount"), Value(anim.boneCount));
	map.SetValue(String("keyframeCount"), Value(anim.keyframeCount));
	map.SetValue(String("_arrayHandle"), Value::zero);
	map.SetValue(String("_arrayCount"), Value::zero);
	map.SetValue(String("_arrayIndex"), Value::zero);
	return Value(map);
}

ModelAnimation ValueToModelAnimation(Value value) {
	if (value.type != ValueType::Map) return ModelAnimation{};
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	ModelAnimation* animPtr = (ModelAnimation*)ValueToPointer(handleVal);
	if (animPtr == nullptr) return ModelAnimation{};
	return *animPtr;
}

Vector3 ValueToVector3(Value value) {
	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		float x = (list.Count() > 0) ? list[0].FloatValue() : 0;
		float y = (list.Count() > 1) ? list[1].FloatValue() : 0;
		float z = (list.Count() > 2) ? list[2].FloatValue() : 0;
		return Vector3{x, y, z};
	} else if (value.type == ValueType::Map) {
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
	return Value(map);
}

Matrix ValueToMatrix(Value value) {
	if (value.type == ValueType::List) {
		ValueList list = value.GetList();
		Matrix m = Matrix{1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1};
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
	}

	if (value.type == ValueType::Map) {
		ValueDict map = value.GetDict();
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

	return Matrix{1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};
}

Value MatrixToValue(Matrix mat) {
	ValueDict result;
	result.SetValue(String("m0"), Value(mat.m0));
	result.SetValue(String("m1"), Value(mat.m1));
	result.SetValue(String("m2"), Value(mat.m2));
	result.SetValue(String("m3"), Value(mat.m3));
	result.SetValue(String("m4"), Value(mat.m4));
	result.SetValue(String("m5"), Value(mat.m5));
	result.SetValue(String("m6"), Value(mat.m6));
	result.SetValue(String("m7"), Value(mat.m7));
	result.SetValue(String("m8"), Value(mat.m8));
	result.SetValue(String("m9"), Value(mat.m9));
	result.SetValue(String("m10"), Value(mat.m10));
	result.SetValue(String("m11"), Value(mat.m11));
	result.SetValue(String("m12"), Value(mat.m12));
	result.SetValue(String("m13"), Value(mat.m13));
	result.SetValue(String("m14"), Value(mat.m14));
	result.SetValue(String("m15"), Value(mat.m15));
	return Value(result);
}

BoundingBox ValueToBoundingBox(Value value) {
	if (value.type != ValueType::Map) return BoundingBox{};
	ValueDict map = value.GetDict();
	BoundingBox box;
	box.min = ValueToVector3(map.Lookup(String("min"), Value::null));
	box.max = ValueToVector3(map.Lookup(String("max"), Value::null));
	return box;
}

Value BoundingBoxToValue(BoundingBox box) {
	ValueDict map;
	map.SetValue(String("min"), Vector3ToValue(box.min));
	map.SetValue(String("max"), Vector3ToValue(box.max));
	return Value(map);
}

Ray ValueToRay(Value value) {
	if (value.type != ValueType::Map) return Ray{};
	ValueDict map = value.GetDict();
	Ray ray;
	ray.position = ValueToVector3(map.Lookup(String("position"), map.Lookup(String("origin"), Value::null)));
	ray.direction = ValueToVector3(map.Lookup(String("direction"), Value::null));
	return ray;
}

Value RayToValue(Ray ray) {
	ValueDict map;
	map.SetValue(String("position"), Vector3ToValue(ray.position));
	map.SetValue(String("direction"), Vector3ToValue(ray.direction));
	return Value(map);
}

Value RayCollisionToValue(RayCollision collision) {
	ValueDict map;
	map.SetValue(String("hit"), Value(collision.hit));
	map.SetValue(String("distance"), Value(collision.distance));
	map.SetValue(String("point"), Vector3ToValue(collision.point));
	map.SetValue(String("normal"), Vector3ToValue(collision.normal));
	return Value(map);
}
