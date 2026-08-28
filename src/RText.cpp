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
	raylibModule.SetValue("LoadFont", i.GetFunc());

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
	raylibModule.SetValue("LoadFontEx", i.GetFunc());

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
	raylibModule.SetValue("LoadFontFromImage", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		return IntrinsicResult(IsFontValid(font));
	});
	raylibModule.SetValue("IsFontValid", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		UnloadFont(font);
		// Free the heap-allocated Font struct
		ValueDict map = context.GetArg(0).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Font* fontPtr = (Font*)ValueToPointer(handleVal);
		delete fontPtr;
		rcFont--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadFont", i.GetFunc());

	// Construct a Font from individual components (glyphs, recs, texture)
	// This is needed for manual font construction, e.g. SDF fonts
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
			font.glyphs[i].value = glyphDict.Lookup(String("value"), Value::zero).IntValue();
			font.glyphs[i].offsetX = glyphDict.Lookup(String("offsetX"), Value::zero).IntValue();
			font.glyphs[i].offsetY = glyphDict.Lookup(String("offsetY"), Value::zero).IntValue();
			font.glyphs[i].advanceX = glyphDict.Lookup(String("advanceX"), Value::zero).IntValue();
			font.glyphs[i].image = ValueToImage(glyphDict.Lookup(String("image"), Value::Null));
		}

		if (!IsFontValid(font)) return IntrinsicResult::Null;
		rcFont++;
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("MakeFont", i.GetFunc());

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
	raylibModule.SetValue("DrawFPS", i.GetFunc());

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
	raylibModule.SetValue("DrawText", i.GetFunc());

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
	raylibModule.SetValue("DrawTextEx", i.GetFunc());

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
	raylibModule.SetValue("DrawTextPro", i.GetFunc());

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
	raylibModule.SetValue("DrawTextCodepoint", i.GetFunc());

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
	raylibModule.SetValue("MeasureText", i.GetFunc());

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
	raylibModule.SetValue("MeasureTextEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		int index = GetGlyphIndex(font, codepoint);
		return IntrinsicResult(Value(index));
	});
	raylibModule.SetValue("GetGlyphIndex", i.GetFunc());

	// Additional font and text functions

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = GetFontDefault();
		return IntrinsicResult(FontToValue(font));
	});
	raylibModule.SetValue("GetFontDefault", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("spacing");
	i.set_Code(INTRINSIC_LAMBDA {
		int spacing = context.GetArg(0).IntValue();
		SetTextLineSpacing(spacing);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTextLineSpacing", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		Rectangle rec = GetGlyphAtlasRec(font, codepoint);
		return IntrinsicResult(RectangleToValue(rec));
	});
	raylibModule.SetValue("GetGlyphAtlasRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		int codepoint = context.GetArg(1).IntValue();
		GlyphInfo info = GetGlyphInfo(font, codepoint);

		ValueDict result;
		result.SetValue(String("value"), Value(info.value));
		result.SetValue(String("offsetX"), Value(info.offsetX));
		result.SetValue(String("offsetY"), Value(info.offsetY));
		result.SetValue(String("advanceX"), Value(info.advanceX));
		result.SetValue(String("image"), ImageToValue(info.image));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetGlyphInfo", i.GetFunc());

	// UTF-8 and codepoint functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int count = GetCodepointCount(text.c_str());
		return IntrinsicResult(Value(count));
	});
	raylibModule.SetValue("GetCodepointCount", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("codepointSize", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int codepointSize = 0;
		int codepoint = GetCodepoint(text.c_str(), &codepointSize);

		ValueDict result;
		result.SetValue(String("codepoint"), Value(codepoint));
		result.SetValue(String("codepointSize"), Value(codepointSize));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetCodepoint", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("codepointSize", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int codepointSize = 0;
		int codepoint = GetCodepointNext(text.c_str(), &codepointSize);

		ValueDict result;
		result.SetValue(String("codepoint"), Value(codepoint));
		result.SetValue(String("codepointSize"), Value(codepointSize));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetCodepointNext", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("codepointSize", Value::Null);
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int codepointSize = 0;
		int codepoint = GetCodepointPrevious(text.c_str(), &codepointSize);

		ValueDict result;
		result.SetValue(String("codepoint"), Value(codepoint));
		result.SetValue(String("codepointSize"), Value(codepointSize));
		return IntrinsicResult(DynamicMap(result));
	});
	raylibModule.SetValue("GetCodepointPrevious", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("codepoint");
	i.set_Code(INTRINSIC_LAMBDA {
		int codepoint = context.GetArg(0).IntValue();
		int utf8Size = 0;
		const char* utf8 = CodepointToUTF8(codepoint, &utf8Size);
		return IntrinsicResult(Value(String(utf8, utf8Size)));
	});
	raylibModule.SetValue("CodepointToUTF8", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text1");
	i.AddParam("text2");
	i.set_Code(INTRINSIC_LAMBDA {
		String text1 = context.GetArg(0).ToString();
		String text2 = context.GetArg(1).ToString();
		bool equal = TextIsEqual(text1.c_str(), text2.c_str());
		return IntrinsicResult(Value(equal));
	});
	raylibModule.SetValue("TextIsEqual", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		unsigned int length = TextLength(text.c_str());
		return IntrinsicResult(Value((int)length));
	});
	raylibModule.SetValue("TextLength", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.set_Code(INTRINSIC_LAMBDA {
		String src = context.GetArg(1).ToString();
		// MiniScript strings are immutable, so we just return the source string
		// The actual TextCopy in raylib copies to a pre-allocated buffer
		return IntrinsicResult(Value(src));
	});
	raylibModule.SetValue("TextCopy", i.GetFunc());

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
	raylibModule.SetValue("LoadFontFromMemory", i.GetFunc());

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
				glyphDict.SetValue(String("value"), Value(glyphs[i].value));
				glyphDict.SetValue(String("offsetX"), Value(glyphs[i].offsetX));
				glyphDict.SetValue(String("offsetY"), Value(glyphs[i].offsetY));
				glyphDict.SetValue(String("advanceX"), Value(glyphs[i].advanceX));
				glyphDict.SetValue(String("image"), ImageToValue(glyphs[i].image));
				result.Add(DynamicMap(glyphDict));
			}
		}
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadFontData", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("glyphs");
	i.set_Code(INTRINSIC_LAMBDA {
		// In our implementation, glyphs is a list of dictionaries
		// We don't need to explicitly free them as MiniScript manages the memory
		// This is a no-op for our purposes
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadFontData", i.GetFunc());

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
	raylibModule.SetValue("LoadCodepoints", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("codepoints");
	i.set_Code(INTRINSIC_LAMBDA {
		// In our implementation, codepoints is a MiniScript list
		// We don't need to explicitly free it as MiniScript manages the memory
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadCodepoints", i.GetFunc());

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
	raylibModule.SetValue("LoadUTF8", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		// In our implementation, text is a MiniScript string
		// We don't need to explicitly free it as MiniScript manages the memory
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadUTF8", i.GetFunc());

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
	raylibModule.SetValue("DrawTextCodepoints", i.GetFunc());

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

		// Convert lists to arrays
		GlyphInfo* glyphs = new GlyphInfo[glyphCount];
		Rectangle* recs = new Rectangle[glyphCount];

		for (int i = 0; i < glyphCount; i++) {
			ValueDict glyphDict = glyphsList[i].GetDict();
			glyphs[i].value = glyphDict.Lookup(String("value"), Value::zero).IntValue();
			glyphs[i].offsetX = glyphDict.Lookup(String("offsetX"), Value::zero).IntValue();
			glyphs[i].offsetY = glyphDict.Lookup(String("offsetY"), Value::zero).IntValue();
			glyphs[i].advanceX = glyphDict.Lookup(String("advanceX"), Value::zero).IntValue();
			glyphs[i].image = ValueToImage(glyphDict.Lookup(String("image"), Value::Null));

			recs[i] = ValueToRectangle(recsList[i]);
		}

		// GenImageFontAtlas modifies the recs array, so we need to pass a pointer
		Rectangle** recsPtr = new Rectangle*;
		*recsPtr = recs;

		Image atlas = GenImageFontAtlas(glyphs, recsPtr, glyphCount, fontSize, padding, packMethod);

		// Write the updated recs back into the MiniScript list
		Rectangle* updatedRecs = *recsPtr;
		for (int i = 0; i < glyphCount; i++) {
			recsList[i] = RectangleToValue(updatedRecs[i]);
		}

		delete[] glyphs;
		delete[] updatedRecs;
		delete recsPtr;

		rcImage++;
		return IntrinsicResult(ImageToValue(atlas));
	});
	raylibModule.SetValue("GenImageFontAtlas", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("args", Value::make_list(0));
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		ValueList args = context.GetArg(1).GetList();

		// Simple implementation: replace %s, %d, %f with args in order
		std::string result = text.c_str();
		int argIndex = 0;

		for (size_t i = 0; i < result.length() - 1 && argIndex < args.Count(); i++) {
			if (result[i] == '%') {
				char specifier = result[i + 1];
				std::string replacement;

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
	raylibModule.SetValue("TextFormat", i.GetFunc());

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
	raylibModule.SetValue("TextFindIndex", i.GetFunc());
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
	raylibModule.SetValue("GetTextBetween", i.GetFunc());

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
	raylibModule.SetValue("TextReplace", i.GetFunc());

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
	raylibModule.SetValue("TextReplaceAlloc", i.GetFunc());

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
	raylibModule.SetValue("TextReplaceBetween", i.GetFunc());

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
	raylibModule.SetValue("TextReplaceBetweenAlloc", i.GetFunc());

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
	raylibModule.SetValue("TextInsert", i.GetFunc());

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
	raylibModule.SetValue("TextInsertAlloc", i.GetFunc());

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
	raylibModule.SetValue("TextSplit", i.GetFunc());

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
	raylibModule.SetValue("TextJoin", i.GetFunc());

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
	raylibModule.SetValue("TextAppend", i.GetFunc());

	// Text case conversion functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToUpper(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToUpper", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToLower(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToLower", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToPascal(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToPascal", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToSnake(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToSnake", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String textStr = context.GetArg(0).ToString();
		String ret(TextToCamel(textStr.c_str()));
		return IntrinsicResult(ret);
	});
	raylibModule.SetValue("TextToCamel", i.GetFunc());

	// Text to value conversion functions

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int result = TextToInteger(text.c_str());
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("TextToInteger", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		float result = TextToFloat(text.c_str());
		return IntrinsicResult(result);
	});
	raylibModule.SetValue("TextToFloat", i.GetFunc());

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
	raylibModule.SetValue("LoadTextLines", i.GetFunc());
}
