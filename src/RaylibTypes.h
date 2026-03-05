#include "raylib.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"

using namespace MiniScript;

// MiniScript classes (maps) that represent various Raylib structs
ValueDict ImageClass();
ValueDict TextureClass();
ValueDict FontClass();
ValueDict WaveClass();
ValueDict MusicClass();
ValueDict SoundClass();
ValueDict AudioStreamClass();
ValueDict RenderTextureClass();

// Convert a Raylib Texture to a MiniScript map
// Allocates the Texture on the heap and stores pointer in _handle
Value TextureToValue(Texture texture);

// Extract a Raylib Texture from a MiniScript map
// Returns the Texture by dereferencing the _handle pointer
Texture ValueToTexture(Value value);

// Convert a Raylib Image to a MiniScript map
// Allocates the Image on the heap and stores pointer in _handle
Value ImageToValue(Image image);

// Extract a Raylib Image from a MiniScript map
// Returns the Image by dereferencing the _handle pointer
Image ValueToImage(Value value);

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
// Allocates the RenderTexture2D on the heap and stores pointer in _handle
Value RenderTextureToValue(RenderTexture2D renderTexture);

// Extract a Raylib RenderTexture2D from a MiniScript map
// Returns the RenderTexture2D by dereferencing the _handle pointer
RenderTexture2D ValueToRenderTexture(Value value);

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

// Convert a raw pointer to a MiniScript value
inline Value PointerToValue(void* ptr) {
	return Value((double)(intptr_t)ptr);
}

// Convert a MiniScript value (containing a pointer) to a raw pointer
inline void* ValueToPointer(Value v) {
	return (void*)(intptr_t)v.DoubleValue();
}



