//
//  RText.cpp
//  MSRLWeb
//
//  Raylib Text module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "FileSystem.h"
#include "RawData.h"
#include "raylib.h"
#include "miniscript.h"
#include "unicodeUtil.h"
#include "macros.h"
#include <string>
#include <vector>

using namespace MiniScript;

// Helper function to extract codepoints from either a list of ints or a UTF-8 string
// Returns allocated int array of codepoints, or nullptr if value is null
// Sets codepointCount to the number of codepoints
// Caller must delete[] the returned array
static int* GetCodepointsFromValue(Value value, int* codepointCount) {
	*codepointCount = 0;

	if (value.Type() == ValueType::Null) {
		return nullptr;
	}

	if (value.Type() == ValueType::String) {
		// Use MiniScript's UTF-8 utilities to parse the string
		String str = value.ToString();
		int count = str.Length();  // Character count in the string
		if (count == 0) {
			*codepointCount = 0;
			return nullptr;
		}

		// Allocate buffer for codepoints
		int* codepoints = new int[count];

		// Iterate over UTF-8 string and decode each codepoint
		unsigned char* ptr = (unsigned char*)str.data();
		for (int i = 0; i < count; i++) {
			codepoints[i] = (int)UTF8DecodeAndAdvance(&ptr);
		}

		*codepointCount = count;
		return codepoints;
	}

	if (value.Type() == ValueType::List) {
		ValueList list = value.GetList();
		int count = list.Count();
		if (count == 0) {
			*codepointCount = 0;
			return nullptr;
		}

		int* codepoints = new int[count];
		for (int i = 0; i < count; i++) {
			codepoints[i] = list[i].IntValue();
		}

		*codepointCount = count;
		return codepoints;
	}

	return nullptr;
}

void AddRTextMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Font loading

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Font font = LoadFont(path.c_str());
		if (!IsFontValid(font)) return IntrinsicResult::Null;
		rcFont++;
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("LoadFont", WithNote(i,
		"Load a Font from file into GPU memory (VRAM).  Returns null on failure"));

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("fontSize", Value(20));
	i.AddParam("codepoints", Value::Null);
	i.AddParam("codepointCount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		int fontSize = context.GetArg(1).IntValue();
		Value codepointsVal = context.GetArg(2);

		// Support both list of ints and UTF-8 string for codepoints
		int codepointCount = 0;
		int* codepoints = GetCodepointsFromValue(codepointsVal, &codepointCount);

		Font font = LoadFontEx(path.c_str(), fontSize, codepoints, codepointCount);

		if (codepoints) delete[] codepoints;

		if (!IsFontValid(font)) return IntrinsicResult::Null;
		rcFont++;
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("LoadFontEx", WithNote(i,
		"Load a font from file at fontSize (in pixels of height), with the given codepoints (a list "
		"of ints or a string of characters); null codepoints loads the default character set "
		"(32..126).  codepointCount is ignored.  Returns null on failure"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("key", ColorToValue(Color{255, 0, 255, 255}));
	i.AddParam("firstChar", Value(32));
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		Color key = ValueToColor(context.GetArg(1));
		Value firstCharVal = context.GetArg(2);
		int firstChar;
		if (firstCharVal.Type() == ValueType::String) {
			String s = firstCharVal.ToString();
			firstChar = s.empty() ? 32 : (int)s[0];
		} else {
			firstChar = firstCharVal.IntValue();
		}
		Font font = LoadFontFromImage(image, key, firstChar);
		rcFont++;
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("LoadFontFromImage", WithNote(i,
		"Load a font from an Image (XNA style): glyphs separated by key color, starting at "
		"firstChar (a codepoint or a one-character string)"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		return IntrinsicResult(IsFontValid(font));
	});
	raylibModule.SetValue("IsFontValid", WithNote(i,
		"Check if font is valid (font data loaded).  WARNING: GPU texture not checked"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font;
		if (!TakeResource(context.GetArg(0), &font)) return IntrinsicResult::Null;
		UnloadFont(font);
		rcFont--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadFont", WithNote(i,
		"Unload Font from GPU memory (VRAM)"));

	i = Intrinsic::Create("");
	i.AddParam("baseSize");
	i.AddParam("glyphs");
	i.AddParam("recs");
	i.AddParam("texture");
	i.AddParam("glyphPadding", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int baseSize = context.GetArg(0).IntValue();
		ValueList glyphsList = context.GetArg(1).GetList();
		ValueList recsList = context.GetArg(2).GetList();
		Texture2D texture = ValueToTexture(context.GetArg(3));
		int glyphPadding = context.GetArg(4).IntValue();

		int glyphCount = glyphsList.Count();
		if (glyphCount == 0 || glyphCount != recsList.Count()) return IntrinsicResult::Null;

		// Build the Font struct
		Font font = { 0 };
		font.baseSize = baseSize;
		font.glyphCount = glyphCount;
		font.glyphPadding = glyphPadding;
		font.texture = texture;

		// Allocate and populate recs array
		font.recs = (Rectangle*)RL_MALLOC(glyphCount * sizeof(Rectangle));
		for (int i = 0; i < glyphCount; i++) {
			font.recs[i] = ValueToRectangle(recsList[i]);
		}

		// Allocate and populate glyphs array
		font.glyphs = (GlyphInfo*)RL_MALLOC(glyphCount * sizeof(GlyphInfo));
		for (int i = 0; i < glyphCount; i++) {
			ValueDict glyphDict = glyphsList[i].GetDict();
			font.glyphs[i].value = glyphDict.Lookup(CACHED_KEY("value"), Value::zero).IntValue();
			font.glyphs[i].offsetX = glyphDict.Lookup(CACHED_KEY("offsetX"), Value::zero).IntValue();
			font.glyphs[i].offsetY = glyphDict.Lookup(CACHED_KEY("offsetY"), Value::zero).IntValue();
			font.glyphs[i].advanceX = glyphDict.Lookup(CACHED_KEY("advanceX"), Value::zero).IntValue();
			// Copy the pixels: UnloadFont frees them, and the script's glyph images
			// keep their own buffers (released by UnloadImage / UnloadFontData).
			font.glyphs[i].image = ImageCopy(ValueToImage(glyphDict.Lookup(CACHED_KEY("image"), Value::Null)));
		}

		if (!IsFontValid(font)) {
			for (int i = 0; i < glyphCount; i++) UnloadImage(font.glyphs[i].image);
			RL_FREE(font.glyphs);
			RL_FREE(font.recs);
			return IntrinsicResult::Null;
		}
		TransferOwnership(context.GetArg(3));		// UnloadFont unloads it now
		rcFont++;
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("MakeFont", WithNote(i,
		"Build a Font from its parts, e.g. for an SDF font.  glyphs is a list of maps (value, offsetX, "
		"offsetY, advanceX, image), recs a matching list of each glyph's rectangle in texture.  The "
		"font takes over texture, so UnloadFont unloads it; glyph images are copied.  Returns null if "
		"the lists are empty or differ in length, or the font is not valid"));

	// Text drawing

	i = Intrinsic::Create("");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int posX = context.GetArg(0).IntValue();
		int posY = context.GetArg(1).IntValue();
		DrawFPS(posX, posY);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawFPS", WithNote(i,
		"Draw current FPS, using the default font"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("fontSize", Value(20));
	i.AddParam("color", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int posX = context.GetArg(1).IntValue();
		int posY = context.GetArg(2).IntValue();
		int fontSize = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		DrawText(text.c_str(), posX, posY, fontSize, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawText", WithNote(i,
		"Draw text using the default font.  If fontSize is smaller than the font's base size (10), "
		"the base size is used.  Character spacing is proportional to fontSize"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("text");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("fontSize", Value(20));
	i.AddParam("spacing", Value::zero);
	i.AddParam("tint", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		String text = context.GetArg(1).ToString();
		Vector2 position = ValueToVector2(context.GetArg(2));
		float fontSize = context.GetArg(3).FloatValue();
		float spacing = context.GetArg(4).FloatValue();
		Color tint = ValueToColor(context.GetArg(5));
		DrawTextEx(font, text.c_str(), position, fontSize, spacing, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextEx", WithNote(i,
		"Draw text using Font.  Character spacing is NOT proportional to fontSize"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("text");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("fontSize", Value(20));
	i.AddParam("spacing", Value::zero);
	i.AddParam("tint", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		String text = context.GetArg(1).ToString();
		Vector2 position = ValueToVector2(context.GetArg(2));
		Vector2 origin = ValueToVector2(context.GetArg(3));
		float rotation = context.GetArg(4).FloatValue();
		float fontSize = context.GetArg(5).FloatValue();
		float spacing = context.GetArg(6).FloatValue();
		Color tint = ValueToColor(context.GetArg(7));
		DrawTextPro(font, text.c_str(), position, origin, rotation, fontSize, spacing, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextPro", WithNote(i,
		"Draw text using Font, rotated (in degrees) around origin"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("fontSize", Value(20));
	i.AddParam("tint", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		Vector2 position = ValueToVector2(context.GetArg(2));
		float fontSize = context.GetArg(3).FloatValue();
		Color tint = ValueToColor(context.GetArg(4));
		DrawTextCodepoint(font, codepoint, position, fontSize, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextCodepoint", WithNote(i,
		"Draw one character, given as an int codepoint"));

	// Text measurement

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("fontSize", Value(20));
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int fontSize = context.GetArg(1).IntValue();
		int width = MeasureText(text.c_str(), fontSize);
		return IntrinsicResult(Value(width));
	});
	raylibModule.SetValue("MeasureText", WithNote(i,
		"Measure text width in pixels, for the default font"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("text");
	i.AddParam("fontSize", Value(20));
	i.AddParam("spacing", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		String text = context.GetArg(1).ToString();
		float fontSize = context.GetArg(2).FloatValue();
		float spacing = context.GetArg(3).FloatValue();
		Vector2 size = MeasureTextEx(font, text.c_str(), fontSize, spacing);
		Value result = Vector2ToValue(size);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("MeasureTextEx", WithNote(i,
		"Measure text size for Font; returns a map with x (width) and y (height)"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoints");
	i.AddParam("fontSize", Value(20));
	i.AddParam("spacing", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		float fontSize = context.GetArg(2).FloatValue();
		float spacing = context.GetArg(3).FloatValue();

		// Support both list of ints and UTF-8 string for codepoints
		int count = 0;
		int* codepoints = GetCodepointsFromValue(context.GetArg(1), &count);
		if (!codepoints || count == 0) return IntrinsicResult(Vector2ToValue(Vector2{0, 0}));

		Vector2 size = MeasureTextCodepoints(font, codepoints, count, fontSize, spacing);
		delete[] codepoints;
		return IntrinsicResult(Vector2ToValue(size));
	});
	raylibModule.SetValue("MeasureTextCodepoints", WithNote(i,
		"Measure size of codepoints (a list of ints or a string) for Font; returns a map with x "
		"(width) and y (height)"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		int index = GetGlyphIndex(font, codepoint);
		return IntrinsicResult(Value(index));
	});
	raylibModule.SetValue("GetGlyphIndex", WithNote(i,
		"Get the glyph index of a Unicode codepoint in font.  If codepoint is not found in the "
		"font, it falls back to '?'"));

	// Additional font and text functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = GetFontDefault();
		Value result = FontToValue(font);
		MarkBorrowed(result, Value::Null);		// raylib's own; CloseWindow unloads it
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("GetFontDefault", WithNote(i,
		"Get the default font"));

	i = Intrinsic::Create("");
	i.AddParam("spacing");
	i.set_Code(INTRINSIC_LAMBDA {
		int spacing = context.GetArg(0).IntValue();
		SetTextLineSpacing(spacing);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTextLineSpacing", WithNote(i,
		"Set vertical line spacing when drawing with line-breaks"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		Rectangle rec = GetGlyphAtlasRec(font, codepoint);
		return IntrinsicResult(RectangleToValue(rec));
	});
	raylibModule.SetValue("GetGlyphAtlasRec", WithNote(i,
		"Get glyph rectangle in font atlas for a codepoint (Unicode character).  If codepoint is "
		"not found in the font, it falls back to '?'"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		GlyphInfo info = GetGlyphInfo(font, codepoint);

		ValueDict result;
		result.SetValue(CACHED_KEY("value"), Value(info.value));
		result.SetValue(CACHED_KEY("offsetX"), Value(info.offsetX));
		result.SetValue(CACHED_KEY("offsetY"), Value(info.offsetY));
		result.SetValue(CACHED_KEY("advanceX"), Value(info.advanceX));
		Value image = ImageToValue(info.image);
		MarkBorrowed(image, context.GetArg(0));		// the font's glyph image
		result.SetValue(CACHED_KEY("image"), image);
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetGlyphInfo", WithNote(i,
		"Get glyph info for a codepoint (Unicode character), as a map with value, offsetX, "
		"offsetY, advanceX and image.  If codepoint is not found in the font, it falls back to '?'"));

	// UTF-8 and codepoint functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int count = GetCodepointCount(text.c_str());
		return IntrinsicResult(Value(count));
	});
	raylibModule.SetValue("GetCodepointCount", WithNote(i,
		"Get the number of characters (codepoints) in text.  An invalid UTF-8 sequence counts as "
		"one '?' (0x3f) codepoint"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("codepointSize", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int codepointSize = 0;
		int codepoint = GetCodepoint(text.c_str(), &codepointSize);

		ValueDict result;
		result.SetValue(CACHED_KEY("codepoint"), Value(codepoint));
		result.SetValue(CACHED_KEY("codepointSize"), Value(codepointSize));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetCodepoint", WithNote(i,
		"Get the first codepoint in text.  Returns a map with codepoint and codepointSize (number "
		"of UTF-8 bytes it used).  An invalid UTF-8 byte gives a '?' (0x3f) codepoint.  The "
		"codepointSize parameter is ignored"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("codepointSize", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int codepointSize = 0;
		int codepoint = GetCodepointNext(text.c_str(), &codepointSize);

		ValueDict result;
		result.SetValue(CACHED_KEY("codepoint"), Value(codepoint));
		result.SetValue(CACHED_KEY("codepointSize"), Value(codepointSize));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetCodepointNext", WithNote(i,
		"Get the first codepoint in text.  Returns a map with codepoint and codepointSize (number "
		"of UTF-8 bytes it used).  The codepointSize parameter is ignored"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("codepointSize", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int codepointSize = 0;
		int codepoint = GetCodepointPrevious(text.c_str(), &codepointSize);

		ValueDict result;
		result.SetValue(CACHED_KEY("codepoint"), Value(codepoint));
		result.SetValue(CACHED_KEY("codepointSize"), Value(codepointSize));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetCodepointPrevious", WithNote(i,
		"Get the codepoint before the start of text, as a map with codepoint and codepointSize "
		"(number of UTF-8 bytes it used).  WARNING: text has nothing before its start, so the "
		"result is not meaningful.  The codepointSize parameter is ignored"));

	i = Intrinsic::Create("");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		int codepoint = context.GetArg(0).IntValue();
		int utf8Size = 0;
		const char* utf8 = CodepointToUTF8(codepoint, &utf8Size);
		return IntrinsicResult(Value(String(utf8, utf8Size)));
	});
	raylibModule.SetValue("CodepointToUTF8", WithNote(i,
		"Get the character for a Unicode codepoint, as a string"));

	i = Intrinsic::Create("");
	i.AddParam("text1");
	i.AddParam("text2");
	i.set_Code(INTRINSIC_LAMBDA {
		String text1 = context.GetArg(0).ToString();
		String text2 = context.GetArg(1).ToString();
		bool equal = TextIsEqual(text1.c_str(), text2.c_str());
		return IntrinsicResult(Value(equal));
	});
	raylibModule.SetValue("TextIsEqual", WithNote(i,
		"Check if two text strings are equal"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		unsigned int length = TextLength(text.c_str());
		return IntrinsicResult(Value((int)length));
	});
	raylibModule.SetValue("TextLength", WithNote(i,
		"Get the length of a string in bytes (of UTF-8), not characters"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.set_Code(INTRINSIC_LAMBDA {
		String src = context.GetArg(1).ToString();
		// MiniScript strings are immutable, so we just return the source string
		// The actual TextCopy in raylib copies to a pre-allocated buffer
		return IntrinsicResult(Value(src));
	});
	raylibModule.SetValue("TextCopy", WithNote(i,
		"Returns src unchanged (dst is ignored, since MiniScript strings cannot be modified)"));

	// Memory-related functions

	i = Intrinsic::Create("");
	i.AddParam("fileType");
	i.AddParam("fileData");
	i.AddParam("fontSize");
	i.AddParam("codepoints", Value::Null);
	i.AddParam("codepointCount", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		String fileType = context.GetArg(0).ToString();
		BinaryData* data = ValueToRawData(context.GetArg(1));
		if (!data) return IntrinsicResult::Null;

		int fontSize = context.GetArg(2).IntValue();
		Value codepointsVal = context.GetArg(3);
		int codepointCount = 0;

		// Support both list of ints and UTF-8 string for codepoints
		int* codepoints = GetCodepointsFromValue(codepointsVal, &codepointCount);

		Font font = LoadFontFromMemory(fileType.c_str(), data->bytes, data->length, fontSize, codepoints, codepointCount);

		if (codepoints) delete[] codepoints;

		if (!IsFontValid(font)) return IntrinsicResult::Null;
		rcFont++;
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("LoadFontFromMemory", WithNote(i,
		"Load a font from a RawData buffer.  fileType is the file extension, e.g. \".ttf\".  "
		"codepoints is a list of ints or a string (null for the default set, 32..126); "
		"codepointCount is ignored.  Returns null on failure"));

	i = Intrinsic::Create("");
	i.AddParam("fileData");
	i.AddParam("fontSize");
	i.AddParam("codepoints", Value::Null);
	i.AddParam("codepointCount", Value::zero);
	i.AddParam("type", Value::zero);  // FONT_DEFAULT
	i.set_Code(INTRINSIC_LAMBDA {
		BinaryData* data = ValueToRawData(context.GetArg(0));
		if (!data) return IntrinsicResult::Null;

		int fontSize = context.GetArg(1).IntValue();
		Value codepointsVal = context.GetArg(2);
		int type = context.GetArg(4).IntValue();

		// Support both list of ints and UTF-8 string for codepoints
		int codepointCount = 0;
		int* codepoints = GetCodepointsFromValue(codepointsVal, &codepointCount);

		int glyphCount = 0;
		GlyphInfo* glyphs = LoadFontData(data->bytes, data->length, fontSize, codepoints, codepointCount, type, &glyphCount);

		if (codepoints) delete[] codepoints;

		// Convert to MiniScript list
		ValueList result;
		if (glyphs) {
			for (int i = 0; i < glyphCount; i++) {
				ValueDict glyphDict;
				glyphDict.SetValue(CACHED_KEY("value"), Value(glyphs[i].value));
				glyphDict.SetValue(CACHED_KEY("offsetX"), Value(glyphs[i].offsetX));
				glyphDict.SetValue(CACHED_KEY("offsetY"), Value(glyphs[i].offsetY));
				glyphDict.SetValue(CACHED_KEY("advanceX"), Value(glyphs[i].advanceX));
				// The script now owns this pixel buffer (release with UnloadImage or UnloadFontData).
				glyphDict.SetValue(CACHED_KEY("image"), ImageToValue(glyphs[i].image));
				rcImage++;
				result.Add(DynamicMap(glyphDict));
			}
			// Free only the array: raylib's UnloadFontData would also free the images handed out above.
			RL_FREE(glyphs);
		}
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadFontData", WithNote(i,
		"Load glyph data from TTF font data (a RawData), for use with GenImageFontAtlas or "
		"MakeFont.  codepoints is a list of ints or a string (null for 32..126); codepointCount is "
		"ignored.  type: 0 = default, 1 = bitmap, 2 = SDF.  Returns a list of glyph maps (value, "
		"offsetX, offsetY, advanceX, image); release the images with UnloadFontData"));

	i = Intrinsic::Create("");
	i.AddParam("glyphs");
	i.set_Code(INTRINSIC_LAMBDA {
		// glyphs is the list of maps returned by LoadFontData; each owns its image.
		// Entries already unloaded (or not glyph maps) are skipped.
		Value glyphsVal = context.GetArg(0);
		if (glyphsVal.Type() != ValueType::List) return IntrinsicResult::Null;
		ValueList glyphsList = glyphsVal.GetList();
		for (int n = 0; n < glyphsList.Count(); n++) {
			Value g = glyphsList[n];
			if (g.Type() != ValueType::Map) continue;
			Image img;
			if (!TakeResource(g.GetDict().Lookup(CACHED_KEY("image"), Value::Null), &img)) continue;
			UnloadImage(img);
			rcImage--;
		}
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadFontData", WithNote(i,
		"Unload the glyph images in a list of glyph maps returned by LoadFontData"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int count = 0;
		int* codepoints = LoadCodepoints(text.c_str(), &count);

		// Convert to MiniScript list
		ValueList result;
		if (codepoints) {
			for (int i = 0; i < count; i++) {
				result.Add(Value(codepoints[i]));
			}
			UnloadCodepoints(codepoints);
		}
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadCodepoints", WithNote(i,
		"Get all codepoints in a text string, as a list of ints"));

	i = Intrinsic::Create("");
	i.AddParam("codepoints");
	i.set_Code(INTRINSIC_LAMBDA {
		// In our implementation, codepoints is a MiniScript list
		// We don't need to explicitly free it as MiniScript manages the memory
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadCodepoints", WithNote(i,
		"Does nothing (codepoint lists are freed automatically); kept for raylib compatibility"));

	i = Intrinsic::Create("");
	i.AddParam("codepoints");
	i.set_Code(INTRINSIC_LAMBDA {
		Value codepointsVal = context.GetArg(0);

		// Support both list of ints and UTF-8 string for codepoints
		int count = 0;
		int* codepoints = GetCodepointsFromValue(codepointsVal, &count);
		if (!codepoints || count == 0) return IntrinsicResult(Value(""));

		char* utf8 = LoadUTF8(codepoints, count);
		String result(utf8);
		UnloadUTF8(utf8);
		delete[] codepoints;

		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("LoadUTF8", WithNote(i,
		"Encode codepoints (a list of ints) into a UTF-8 string"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		// In our implementation, text is a MiniScript string
		// We don't need to explicitly free it as MiniScript manages the memory
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadUTF8", WithNote(i,
		"Does nothing (strings are freed automatically); kept for raylib compatibility"));

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoints");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("fontSize", Value(20));
	i.AddParam("spacing", Value::zero);
	i.AddParam("tint", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		Value codepointsVal = context.GetArg(1);
		Vector2 position = ValueToVector2(context.GetArg(2));
		float fontSize = context.GetArg(3).FloatValue();
		float spacing = context.GetArg(4).FloatValue();
		Color tint = ValueToColor(context.GetArg(5));

		// Support both list of ints and UTF-8 string for codepoints
		int count = 0;
		int* codepoints = GetCodepointsFromValue(codepointsVal, &count);
		if (!codepoints || count == 0) return IntrinsicResult::Null;

		DrawTextCodepoints(font, codepoints, count, position, fontSize, spacing, tint);
		delete[] codepoints;

		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextCodepoints", WithNote(i,
		"Draw multiple characters, given as a list of int codepoints or a string"));

	i = Intrinsic::Create("");
	i.AddParam("glyphs");
	i.AddParam("glyphRecs");
	i.AddParam("fontSize");
	i.AddParam("padding");
	i.AddParam("packMethod");
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList glyphsList = context.GetArg(0).GetList();
		ValueList recsList = context.GetArg(1).GetList();
		int fontSize = context.GetArg(2).IntValue();
		int padding = context.GetArg(3).IntValue();
		int packMethod = context.GetArg(4).IntValue();

		int glyphCount = glyphsList.Count();
		if (glyphCount == 0 || glyphCount != recsList.Count()) return IntrinsicResult::Null;

		std::vector<GlyphInfo> glyphs(glyphCount);
		for (int i = 0; i < glyphCount; i++) {
			ValueDict glyphDict = glyphsList[i].GetDict();
			glyphs[i].value = glyphDict.Lookup(CACHED_KEY("value"), Value::zero).IntValue();
			glyphs[i].offsetX = glyphDict.Lookup(CACHED_KEY("offsetX"), Value::zero).IntValue();
			glyphs[i].offsetY = glyphDict.Lookup(CACHED_KEY("offsetY"), Value::zero).IntValue();
			glyphs[i].advanceX = glyphDict.Lookup(CACHED_KEY("advanceX"), Value::zero).IntValue();
			glyphs[i].image = ValueToImage(glyphDict.Lookup(CACHED_KEY("image"), Value::Null));
		}

		// raylib never reads glyphRecs: it allocates the rectangle array itself
		// (with RL_MALLOC) and hands it back through this pointer.
		Rectangle* recs = nullptr;
		Image atlas = GenImageFontAtlas(glyphs.data(), &recs, glyphCount, fontSize, padding, packMethod);

		// Write the rectangles back into the MiniScript list
		if (recs != nullptr) {
			for (int i = 0; i < glyphCount; i++) {
				recsList[i] = RectangleToValue(recs[i]);
			}
			RL_FREE(recs);
		}

		rcImage++;
		return IntrinsicResult(ImageToValue(atlas));
	});
	raylibModule.SetValue("GenImageFontAtlas", WithNote(i,
		"Generate a font atlas Image from glyphs (a list of glyph maps, as from LoadFontData).  "
		"glyphRecs is a list of the same length, which is filled with each glyph's rectangle in "
		"the atlas.  packMethod: 0 = default, 1 = Skyline.  Returns null if the lists are empty "
		"or differ in length"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("args", Value::make_list(0));
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		ValueList args = context.GetArg(1).GetList();

		// Simple implementation: replace %s, %d, %f with args in order
		std::string result = text.c_str();
		int argIndex = 0;

		// "%%" is replaced even after the args run out; a specifier with no arg
		// left stays as written.
		for (size_t i = 0; i + 1 < result.length(); i++) {
			if (result[i] == '%') {
				char specifier = result[i + 1];
				std::string replacement;
				if (specifier != '%' && argIndex >= args.Count()) continue;

				if (specifier == 's') {
					replacement = args[argIndex].ToString().c_str();
					argIndex++;
				} else if (specifier == 'd' || specifier == 'i') {
					char buf[32];
					snprintf(buf, sizeof(buf), "%d", args[argIndex].IntValue());
					replacement = buf;
					argIndex++;
				} else if (specifier == 'f') {
					char buf[32];
					snprintf(buf, sizeof(buf), "%f", args[argIndex].FloatValue());
					replacement = buf;
					argIndex++;
				} else if (specifier == '%') {
					replacement = "%";
				} else {
					continue;
				}

				result.replace(i, 2, replacement);
				i += replacement.length() - 1;
			}
		}

		return IntrinsicResult(Value(String(result.c_str())));
	});
	raylibModule.SetValue("TextFormat", WithNote(i,
		"Format text, replacing each %s, %d (or %i) and %f with the next value from args, and %% "
		"with %"));

	// Text manipulation functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("search");
	i.set_Code(INTRINSIC_LAMBDA {
		// Hold the Strings in named locals: ToString() returns a temporary whose
		// c_str() would otherwise dangle before TextFindIndex reads it.
		String text = context.GetArg(0).ToString();
		String search = context.GetArg(1).ToString();
		int result = TextFindIndex(text.c_str(), search.c_str());
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("TextFindIndex", WithNote(i,
		"Find first occurrence of search within text; returns its byte index, or -1 if not found"));
	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("begin");
	i.AddParam("end");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String beginStr = context.GetArg(1).ToString();
		String endStr = context.GetArg(2).ToString();
		const char* result = GetTextBetween(textStr.c_str(), beginStr.c_str(), endStr.c_str());
		String ret(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("GetTextBetween", WithNote(i,
		"Get the text between the first occurrence of begin and the next end after it (\"\" if "
		"not found).  Result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("search");
	i.AddParam("replacement");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String searchStr = context.GetArg(1).ToString();
		String replacementStr = context.GetArg(2).ToString();
		const char* result = TextReplace(textStr.c_str(), searchStr.c_str(), replacementStr.c_str());
		String ret(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextReplace", WithNote(i,
		"Replace all occurrences of search in text with replacement.  Result is limited to 1023 "
		"bytes; use TextReplaceAlloc for longer text"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("search");
	i.AddParam("replacement");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String searchStr = context.GetArg(1).ToString();
		String replacementStr = context.GetArg(2).ToString();
		char* result = TextReplaceAlloc(textStr.c_str(), searchStr.c_str(), replacementStr.c_str());
		if (result == nullptr) return IntrinsicResult::Null;
		String ret(result);
		MemFree(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextReplaceAlloc", WithNote(i,
		"Replace all occurrences of search in text with replacement, with no length limit"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("begin");
	i.AddParam("end");
	i.AddParam("replacement");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String beginStr = context.GetArg(1).ToString();
		String endStr = context.GetArg(2).ToString();
		String replacementStr = context.GetArg(3).ToString();
		const char* result = TextReplaceBetween(textStr.c_str(), beginStr.c_str(), endStr.c_str(), replacementStr.c_str());
		String ret(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextReplaceBetween", WithNote(i,
		"Replace the text between begin and end (keeping begin and end) with replacement; an "
		"empty or null replacement just removes it.  Result is limited to 1023 bytes; use "
		"TextReplaceBetweenAlloc for longer text"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("begin");
	i.AddParam("end");
	i.AddParam("replacement");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String beginStr = context.GetArg(1).ToString();
		String endStr = context.GetArg(2).ToString();
		String replacementStr = context.GetArg(3).ToString();
		char* result = TextReplaceBetweenAlloc(textStr.c_str(), beginStr.c_str(), endStr.c_str(), replacementStr.c_str());
		if (result == nullptr) return IntrinsicResult::Null;
		String ret(result);
		MemFree(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextReplaceBetweenAlloc", WithNote(i,
		"Replace the text between begin and end (keeping begin and end) with replacement, with no "
		"length limit; an empty or null replacement just removes it"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("insert");
	i.AddParam("position");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String insertStr = context.GetArg(1).ToString();
		int position = context.GetArg(2).IntValue();
		const char* result = TextInsert(textStr.c_str(), insertStr.c_str(), position);
		String ret(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextInsert", WithNote(i,
		"Insert text at a byte position, moving the rest forward.  Result is limited to 1023 "
		"bytes; use TextInsertAlloc for longer text"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("position", Value::zero);
	i.AddParam("length", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		int position = context.GetArg(1).IntValue();
		int length = context.GetArg(2).IntValue();
		String ret(TextSubtext(textStr.c_str(), position, length));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextSubtext", WithNote(i,
		"Get part of a string: length bytes from byte position (bytes of UTF-8, not characters)"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextRemoveSpaces(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextRemoveSpaces", WithNote(i,
		"Remove all the spaces from a string.  Result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("insert");
	i.AddParam("position");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String insertStr = context.GetArg(1).ToString();
		int position = context.GetArg(2).IntValue();
		char* result = TextInsertAlloc(textStr.c_str(), insertStr.c_str(), position);
		if (result == nullptr) return IntrinsicResult::Null;
		String ret(result);
		MemFree(result);
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextInsertAlloc", WithNote(i,
		"Insert text at a byte position, moving the rest forward, with no length limit"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("delimiter");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String delimiterStr = context.GetArg(1).ToString();
		if (delimiterStr.LengthB() == 0) return IntrinsicResult::Null;

		char delimiter = delimiterStr.data()[0];
		int count = 0;
		const char** parts = (const char**)TextSplit(textStr.c_str(), delimiter, &count);

		ValueList result;
		for (int i = 0; i < count; i++) {
			result.Add(Value(String(parts[i])));
		}
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("TextSplit", WithNote(i,
		"Split text at each occurrence of delimiter (its first character only) into a list of at "
		"most 128 strings; null if delimiter is empty"));

	i = Intrinsic::Create("");
	i.AddParam("textList");
	i.AddParam("delimiter", "");
	i.set_Code(INTRINSIC_LAMBDA {
		ValueList textList = context.GetArg(0).GetList();
		String delimiterStr = context.GetArg(1).ToString();

		int count = textList.Count();
		if (count == 0) return IntrinsicResult(String());

		char** parts = new char*[count];
		for (int i = 0; i < count; i++) {
			String str = textList[i].ToString();
			parts[i] = (char*)str.c_str();
		}

		String ret(TextJoin(parts, count, delimiterStr.c_str()));
		delete[] parts;
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextJoin", WithNote(i,
		"Join a list of strings with delimiter.  Result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("append");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String appendStr = context.GetArg(1).ToString();
		// In MiniScript, we just return the concatenated string
		String result = textStr + appendStr;
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("TextAppend", WithNote(i,
		"Returns text with append added to the end"));

	// Text case conversion functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToUpper(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToUpper", WithNote(i,
		"Get upper case version of provided string.  Only basic (ASCII) letters are converted; "
		"result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToLower(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToLower", WithNote(i,
		"Get lower case version of provided string.  Only basic (ASCII) letters are converted; "
		"result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToPascal(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToPascal", WithNote(i,
		"Get PascalCase version of provided string (from snake_case).  Only basic (ASCII) "
		"characters are handled; result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToSnake(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToSnake", WithNote(i,
		"Get snake_case version of provided string.  Only basic (ASCII) characters are handled; "
		"result is limited to 1023 bytes"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToCamel(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToCamel", WithNote(i,
		"Get camelCase version of provided string (from snake_case).  Only basic (ASCII) "
		"characters are handled; result is limited to 1023 bytes"));

	// Text to value conversion functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int result = TextToInteger(text.c_str());
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("TextToInteger", WithNote(i,
		"Get integer value from text"));

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		float result = TextToFloat(text.c_str());
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("TextToFloat", WithNote(i,
		"Get float value from text.  Only '.' is understood as decimal point"));

	// Text line loading functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		int count = 0;
		char** lines = LoadTextLines(textStr.c_str(), &count);

		ValueList result;
		for (int i = 0; i < count; i++) {
			result.Add(Value(String(lines[i])));
		}
		UnloadTextLines(lines, count);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadTextLines", WithNote(i,
		"Split text into a list of lines, at each line feed"));

	i = Intrinsic::Create("");
	i.AddParam("lines");
	i.set_Code(INTRINSIC_LAMBDA {
		// In our implementation, lines is a MiniScript list
		// We don't need to explicitly free it as MiniScript manages the memory
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadTextLines", WithNote(i,
		"Does nothing (lists are freed automatically); kept for raylib compatibility"));
}
