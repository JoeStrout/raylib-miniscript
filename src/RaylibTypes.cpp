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

// Extract a Raylib Image from a MiniScript map
// Returns the Image by dereferencing the _handle pointer
Image ValueToImage(Value value) {
	if (value.type != ValueType::Map) {
		// Return empty image if not a map
		return Image{nullptr, 0, 0, 0, 0};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Image* imgPtr = (Image*)ValueToPointer(handleVal);
	if (imgPtr == nullptr) {
		return Image{nullptr, 0, 0, 0, 0};
	}
	return *imgPtr;
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
		return Wave{NULL, 0, 0, 0, 0};
	}
	ValueDict map = value.GetDict();
	Value handleVal = map.Lookup(String("_handle"), Value::zero);
	Wave* wavePtr = (Wave*)ValueToPointer(handleVal);
	if (wavePtr == nullptr) {
		return Wave{NULL, 0, 0, 0, 0};
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
