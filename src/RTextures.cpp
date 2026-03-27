//
//  RTextures.cpp
//  MSRLWeb
//
//  Raylib Textures module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include "macros.h"
#include <iostream>

using namespace MiniScript;

void AddRTexturesMethods(ValueDict raylibModule) {
	Intrinsic *i;

	// Image loading

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		Image img = LoadImage(path.c_str());
		if (!IsImageValid(img)) return IntrinsicResult::Null;
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("LoadImage", i->GetFunc());

	// Image generation

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("direction", Value::zero);
	i->AddParam("start", ColorToValue(BLACK));
	i->AddParam("end", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		int direction = context->GetVar(String("direction")).IntValue();
		Color start = ValueToColor(context->GetVar(String("start")));
		Color end = ValueToColor(context->GetVar(String("end")));
		Image img = GenImageGradientLinear(width, height, direction, start, end);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageGradientLinear", i->GetFunc());

	// Image management

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image img = ValueToImage(context->GetVar(String("image")));
		UnloadImage(img);
		// Free the heap-allocated Image struct
		ValueDict map = context->GetVar(String("image")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Image* imgPtr = (Image*)ValueToPointer(handleVal);
		delete imgPtr;
		rcImage--;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadImage", i->GetFunc());

	// Texture loading

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		Texture tex = LoadTexture(path.c_str());
		if (!IsTextureValid(tex)) return IntrinsicResult::Null;
		rcTexture++;
		return IntrinsicResult(TextureToValue(tex));
	};
	raylibModule.SetValue("LoadTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image img = ValueToImage(context->GetVar(String("image")));
		Texture tex = LoadTextureFromImage(img);
		rcTexture++;
		return IntrinsicResult(TextureToValue(tex));
	};
	raylibModule.SetValue("LoadTextureFromImage", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		UnloadTexture(tex);
		// Free the heap-allocated Texture struct
		ValueDict map = context->GetVar(String("texture")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Texture* texPtr = (Texture*)ValueToPointer(handleVal);
		delete texPtr;
		rcTexture--;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadTexture", i->GetFunc());

	// Texture drawing

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("posX", Value::zero);
	i->AddParam("posY", Value::zero);
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		int posX = context->GetVar(String("posX")).IntValue();
		int posY = context->GetVar(String("posY")).IntValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawTexture(tex, posX, posY, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawTextureV(tex, position, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTextureV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("rotation", Value::zero);
	i->AddParam("scale", Value(1.0));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		float rotation = context->GetVar(String("rotation")).FloatValue();
		float scale = context->GetVar(String("scale")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawTextureEx(tex, position, rotation, scale, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTextureEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("source");
	i->AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		Rectangle source = ValueToRectangle(context->GetVar(String("source")));
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawTextureRec(tex, source, position, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTextureRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("source");
	i->AddParam("dest");
	i->AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("rotation", Value::zero);
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		Rectangle source = ValueToRectangle(context->GetVar(String("source")));
		Rectangle dest = ValueToRectangle(context->GetVar(String("dest")));
		Vector2 origin = ValueToVector2(context->GetVar(String("origin")));
		float rotation = context->GetVar(String("rotation")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawTexturePro(tex, source, dest, origin, rotation, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTexturePro", i->GetFunc());

	// More image generation functions

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		Image img = GenImageColor(width, height, color);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageColor", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("density", Value(0.5));
	i->AddParam("inner", ColorToValue(WHITE));
	i->AddParam("outer", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		float density = context->GetVar(String("density")).FloatValue();
		Color inner = ValueToColor(context->GetVar(String("inner")));
		Color outer = ValueToColor(context->GetVar(String("outer")));
		Image img = GenImageGradientRadial(width, height, density, inner, outer);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageGradientRadial", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("density", Value(0.5));
	i->AddParam("inner", ColorToValue(WHITE));
	i->AddParam("outer", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		float density = context->GetVar(String("density")).FloatValue();
		Color inner = ValueToColor(context->GetVar(String("inner")));
		Color outer = ValueToColor(context->GetVar(String("outer")));
		Image img = GenImageGradientSquare(width, height, density, inner, outer);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageGradientSquare", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("checksX", Value(8));
	i->AddParam("checksY", Value(8));
	i->AddParam("col1", ColorToValue(WHITE));
	i->AddParam("col2", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		int checksX = context->GetVar(String("checksX")).IntValue();
		int checksY = context->GetVar(String("checksY")).IntValue();
		Color col1 = ValueToColor(context->GetVar(String("col1")));
		Color col2 = ValueToColor(context->GetVar(String("col2")));
		Image img = GenImageChecked(width, height, checksX, checksY, col1, col2);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageChecked", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("factor", Value(0.5));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		float factor = context->GetVar(String("factor")).FloatValue();
		Image img = GenImageWhiteNoise(width, height, factor);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageWhiteNoise", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("tileSize", Value(32));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		int tileSize = context->GetVar(String("tileSize")).IntValue();
		Image img = GenImageCellular(width, height, tileSize);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	};
	raylibModule.SetValue("GenImageCellular", i->GetFunc());

	// Image manipulation

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image img = ValueToImage(context->GetVar(String("image")));
		Image copy = ImageCopy(img);
		rcImage++;
		return IntrinsicResult(ImageToValue(copy));
	};
	raylibModule.SetValue("ImageCopy", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("crop");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		Rectangle crop = ValueToRectangle(context->GetVar(String("crop")));
		ImageCrop(img, crop);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageCrop", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("newWidth");
	i->AddParam("newHeight");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		int newWidth = context->GetVar(String("newWidth")).IntValue();
		int newHeight = context->GetVar(String("newHeight")).IntValue();
		ImageResize(img, newWidth, newHeight);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageResize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("newWidth");
	i->AddParam("newHeight");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		int newWidth = context->GetVar(String("newWidth")).IntValue();
		int newHeight = context->GetVar(String("newHeight")).IntValue();
		ImageResizeNN(img, newWidth, newHeight);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageResizeNN", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context->GetVar(String("image")));
		if (!img) return IntrinsicResult::Null;
		ImageFlipVertical(img);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageFlipVertical", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context->GetVar(String("image")));
		if (!img) return IntrinsicResult::Null;
		ImageFlipHorizontal(img);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageFlipHorizontal", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		ImageRotateCW(img);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageRotateCW", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		ImageRotateCCW(img);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageRotateCCW", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context->GetVar(String("image")));
		if (!img) return IntrinsicResult::Null;
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageColorTint(img, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageColorTint", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context->GetVar(String("image")));
		if (!img) return IntrinsicResult::Null;
		ImageColorInvert(img);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageColorInvert", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		ImageColorGrayscale(img);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageColorGrayscale", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("contrast");
	i->code = INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context->GetVar(String("image")));
		if (!img) return IntrinsicResult::Null;
		float contrast = context->GetVar(String("contrast")).FloatValue();
		ImageColorContrast(img, contrast);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageColorContrast", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("brightness");
	i->code = INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context->GetVar(String("image")));
		if (!img) return IntrinsicResult::Null;
		int brightness = context->GetVar(String("brightness")).IntValue();
		ImageColorBrightness(img, brightness);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageColorBrightness", i->GetFunc());

	// Image drawing functions

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageClearBackground(dst, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageClearBackground", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("x", Value::zero);
	i->AddParam("y", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawPixel(dst, x, y, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawPixel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawPixelV(dst, position, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawPixelV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("startPosX", Value::zero);
	i->AddParam("startPosY", Value::zero);
	i->AddParam("endPosX", Value::zero);
	i->AddParam("endPosY", Value::zero);
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		int startPosX = context->GetVar(String("startPosX")).IntValue();
		int startPosY = context->GetVar(String("startPosY")).IntValue();
		int endPosX = context->GetVar(String("endPosX")).IntValue();
		int endPosY = context->GetVar(String("endPosY")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawLine(dst, startPosX, startPosY, endPosX, endPosY, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawLine", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("start", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("end", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 start = ValueToVector2(context->GetVar(String("start")));
		Vector2 end = ValueToVector2(context->GetVar(String("end")));
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawLineV(dst, start, end, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawLineV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("centerX", Value(100));
	i->AddParam("centerY", Value(100));
	i->AddParam("radius", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		int radius = context->GetVar(String("radius")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawCircle(dst, centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawCircle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i->AddParam("radius", Value(32));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		int radius = context->GetVar(String("radius")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawCircleV(dst, center, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawCircleV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("posX", Value::zero);
	i->AddParam("posY", Value::zero);
	i->AddParam("width", Value(256));
	i->AddParam("height", Value(256));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		int posX = context->GetVar(String("posX")).IntValue();
		int posY = context->GetVar(String("posY")).IntValue();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawRectangle(dst, posX, posY, width, height, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawRectangle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("rec");
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawRectangleRec(dst, rec, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawRectangleRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("rec");
	i->AddParam("thick", Value(1));
	i->AddParam("color", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		int thick = context->GetVar(String("thick")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawRectangleLines(dst, rec, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawRectangleLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("src");
	i->AddParam("srcRec");
	i->AddParam("dstRec");
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		const Image& src = ValueToImage(context->GetVar(String("src")));
		Rectangle srcRec = ValueToRectangle(context->GetVar(String("srcRec")));
		Rectangle dstRec = ValueToRectangle(context->GetVar(String("dstRec")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		ImageDraw(dst, src, srcRec, dstRec, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDraw", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("text");
	i->AddParam("posX", Value::zero);
	i->AddParam("posY", Value::zero);
	i->AddParam("fontSize", Value(20));
	i->AddParam("color", ColorToValue(BLACK));
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		String text = context->GetVar(String("text")).ToString();
		int posX = context->GetVar(String("posX")).IntValue();
		int posY = context->GetVar(String("posY")).IntValue();
		int fontSize = context->GetVar(String("fontSize")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawText(dst, text.c_str(), posX, posY, fontSize, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawText", i->GetFunc());

	// Texture configuration

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("filter");
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		int filter = context->GetVar(String("filter")).IntValue();
		SetTextureFilter(tex, filter);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetTextureFilter", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("wrap");
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		int wrap = context->GetVar(String("wrap")).IntValue();
		SetTextureWrap(tex, wrap);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetTextureWrap", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context->GetVar(String("texture")));
		GenTextureMipmaps(&tex);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("GenTextureMipmaps", i->GetFunc());

	// RenderTexture2D loading/unloading

	i = Intrinsic::Create("");
	i->AddParam("width", Value(960));
	i->AddParam("height", Value(640));
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		RenderTexture2D renderTexture = LoadRenderTexture(width, height);
		rcRenderTexture++;
		return IntrinsicResult(RenderTextureToValue(renderTexture));
	};
	raylibModule.SetValue("LoadRenderTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("target");
	i->code = INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context->GetVar(String("target")));
		UnloadRenderTexture(target);
		// Free the heap-allocated RenderTexture2D struct
		ValueDict map = context->GetVar(String("target")).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		RenderTexture2D* rtPtr = (RenderTexture2D*)ValueToPointer(handleVal);
		delete rtPtr;
		rcRenderTexture--;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadRenderTexture", i->GetFunc());

	// RenderTexture2D drawing

	i = Intrinsic::Create("");
	i->AddParam("target");
	i->code = INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context->GetVar(String("target")));
		BeginTextureMode(target);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("BeginTextureMode", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		EndTextureMode();
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("EndTextureMode", i->GetFunc());

	// Color manipulation functions

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->AddParam("alpha");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		float alpha = context->GetVar(String("alpha")).FloatValue();
		Color result = ColorAlpha(color, alpha);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorAlpha", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("src");
	i->AddParam("tint");
	i->code = INTRINSIC_LAMBDA {
		Color dst = ValueToColor(context->GetVar(String("dst")));
		Color src = ValueToColor(context->GetVar(String("src")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		Color result = ColorAlphaBlend(dst, src, tint);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorAlphaBlend", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->AddParam("factor");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		float factor = context->GetVar(String("factor")).FloatValue();
		Color result = ColorBrightness(color, factor);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorBrightness", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->AddParam("contrast");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		float contrast = context->GetVar(String("contrast")).FloatValue();
		Color result = ColorContrast(color, contrast);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorContrast", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("hue");
	i->AddParam("saturation");
	i->AddParam("value");
	i->code = INTRINSIC_LAMBDA {
		float hue = context->GetVar(String("hue")).FloatValue();
		float saturation = context->GetVar(String("saturation")).FloatValue();
		float value = context->GetVar(String("value")).FloatValue();
		Color result = ColorFromHSV(hue, saturation, value);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorFromHSV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("normalized");
	i->code = INTRINSIC_LAMBDA {
		ValueDict normalized = context->GetVar(String("normalized")).GetDict();
		Vector4 vec;
		vec.x = normalized.Lookup(String("x"), Value::zero).FloatValue();
		vec.y = normalized.Lookup(String("y"), Value::zero).FloatValue();
		vec.z = normalized.Lookup(String("z"), Value::zero).FloatValue();
		vec.w = normalized.Lookup(String("w"), Value(1.0)).FloatValue();
		Color result = ColorFromNormalized(vec);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorFromNormalized", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("col1");
	i->AddParam("col2");
	i->code = INTRINSIC_LAMBDA {
		Color col1 = ValueToColor(context->GetVar(String("col1")));
		Color col2 = ValueToColor(context->GetVar(String("col2")));
		bool result = ColorIsEqual(col1, col2);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("ColorIsEqual", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color1");
	i->AddParam("color2");
	i->AddParam("amount");
	i->code = INTRINSIC_LAMBDA {
		Color color1 = ValueToColor(context->GetVar(String("color1")));
		Color color2 = ValueToColor(context->GetVar(String("color2")));
		float amount = context->GetVar(String("amount")).FloatValue();
		Color result = ColorLerp(color1, color2, amount);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorLerp", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		Vector4 result = ColorNormalize(color);
		ValueDict resultDict;
		resultDict.SetValue(String("x"), Value(result.x));
		resultDict.SetValue(String("y"), Value(result.y));
		resultDict.SetValue(String("z"), Value(result.z));
		resultDict.SetValue(String("w"), Value(result.w));
		return IntrinsicResult(Value(resultDict));
	};
	raylibModule.SetValue("ColorNormalize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->AddParam("tint");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		Color tint = ValueToColor(context->GetVar(String("tint")));
		Color result = ColorTint(color, tint);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("ColorTint", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		Vector3 result = ColorToHSV(color);
		ValueDict resultDict;
		resultDict.SetValue(String("x"), Value(result.x));
		resultDict.SetValue(String("y"), Value(result.y));
		resultDict.SetValue(String("z"), Value(result.z));
		return IntrinsicResult(Value(resultDict));
	};
	raylibModule.SetValue("ColorToHSV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		int result = ColorToInt(color);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("ColorToInt", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("color");
	i->AddParam("alpha");
	i->code = INTRINSIC_LAMBDA {
		Color color = ValueToColor(context->GetVar(String("color")));
		float alpha = context->GetVar(String("alpha")).FloatValue();
		Color result = Fade(color, alpha);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("Fade", i->GetFunc());

	// Pixel/Color accessor functions

	i = Intrinsic::Create("");
	i->AddParam("hexValue");
	i->code = INTRINSIC_LAMBDA {
		unsigned int hexValue = context->GetVar(String("hexValue")).IntValue();
		Color result = GetColor(hexValue);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("GetColor", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("srcPtr");
	i->AddParam("format");
	i->code = INTRINSIC_LAMBDA {
		// srcPtr should be RawData
		BinaryData* data = ValueToRawData(context->GetVar(String("srcPtr")));
		if (!data) return IntrinsicResult::Null;
		int format = context->GetVar(String("format")).IntValue();
		Color result = GetPixelColor(data->bytes, format);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("GetPixelColor", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("width");
	i->AddParam("height");
	i->AddParam("format");
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		int format = context->GetVar(String("format")).IntValue();
		int result = GetPixelDataSize(width, height, format);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("GetPixelDataSize", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dstPtr");
	i->AddParam("color");
	i->AddParam("format");
	i->code = INTRINSIC_LAMBDA {
		// dstPtr should be RawData
		BinaryData* data = ValueToRawData(context->GetVar(String("dstPtr")));
		if (!data) return IntrinsicResult::Null;
		Color color = ValueToColor(context->GetVar(String("color")));
		int format = context->GetVar(String("format")).IntValue();
		SetPixelColor(data->bytes, color, format);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("SetPixelColor", i->GetFunc());

	// Additional image generation functions

	i = Intrinsic::Create("");
	i->AddParam("width");
	i->AddParam("height");
	i->AddParam("offsetX");
	i->AddParam("offsetY");
	i->AddParam("scale");
	i->code = INTRINSIC_LAMBDA {
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		float offsetX = context->GetVar(String("offsetX")).FloatValue();
		float offsetY = context->GetVar(String("offsetY")).FloatValue();
		float scale = context->GetVar(String("scale")).FloatValue();
		Image result = GenImagePerlinNoise(width, height, offsetX, offsetY, scale);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("GenImagePerlinNoise", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("text");
	i->AddParam("fontSize");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		String text = context->GetVar(String("text")).ToString();
		int fontSize = context->GetVar(String("fontSize")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		Image result = ImageText(text.c_str(), fontSize, color);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("GenImageText", i->GetFunc());

	// Validation functions

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		bool result = IsImageValid(image);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("IsImageValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("target");
	i->code = INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context->GetVar(String("target")));
		bool result = IsRenderTextureValid(target);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("IsRenderTextureValid", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		bool result = IsTextureValid(texture);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("IsTextureValid", i->GetFunc());

	// Additional image loading functions

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("frames");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		int frames = 0;
		Image result = LoadImageAnim(path.c_str(), &frames);
		rcImage++;
		// Return map with image and frames
		ValueDict resultDict;
		resultDict.SetValue(String("image"), ImageToValue(result));
		resultDict.SetValue(String("frames"), Value(frames));
		return IntrinsicResult(Value(resultDict));
	};
	raylibModule.SetValue("LoadImageAnim", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileType");
	i->AddParam("fileData");
	i->AddParam("frames");
	i->code = INTRINSIC_LAMBDA {
		String fileType = context->GetVar(String("fileType")).ToString();
		BinaryData* data = ValueToRawData(context->GetVar(String("fileData")));
		if (!data) return IntrinsicResult::Null;
		int frames = 0;
		Image result = LoadImageAnimFromMemory(fileType.c_str(), data->bytes, data->length, &frames);
		rcImage++;
		// Return map with image and frames
		ValueDict resultDict;
		resultDict.SetValue(String("image"), ImageToValue(result));
		resultDict.SetValue(String("frames"), Value(frames));
		return IntrinsicResult(Value(resultDict));
	};
	raylibModule.SetValue("LoadImageAnimFromMemory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		int colorCount = image.width * image.height;
		Color* colors = LoadImageColors(image);
		// Convert to MiniScript list
		ValueList result;
		for (int i = 0; i < colorCount; i++) {
			result.Add(ColorToValue(colors[i]));
		}
		UnloadImageColors(colors);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadImageColors", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileType");
	i->AddParam("fileData");
	i->code = INTRINSIC_LAMBDA {
		String fileType = context->GetVar(String("fileType")).ToString();
		BinaryData* data = ValueToRawData(context->GetVar(String("fileData")));
		if (!data) return IntrinsicResult::Null;
		Image result = LoadImageFromMemory(fileType.c_str(), data->bytes, data->length);
		if (!IsImageValid(result)) return IntrinsicResult::Null;
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("LoadImageFromMemory", i->GetFunc());

	i = Intrinsic::Create("");
	i->code = INTRINSIC_LAMBDA {
		Image result = LoadImageFromScreen();
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("LoadImageFromScreen", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->code = INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		Image result = LoadImageFromTexture(texture);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("LoadImageFromTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		Image img = ValueToImage(context->GetVar(String("image")));
		String path = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(ExportImage(img, path.c_str()));
	};
	raylibModule.SetValue("ExportImage", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("fileType");
	i->code = INTRINSIC_LAMBDA {
		Image img = ValueToImage(context->GetVar(String("image")));
		String fileType = context->GetVar(String("fileType")).ToString();
		int dataSize = 0;
		unsigned char* data = ExportImageToMemory(img, fileType.c_str(), &dataSize);
		if (!data) return IntrinsicResult::Null;
		BinaryData* bd = new BinaryData(data, dataSize, true);
		return IntrinsicResult(RawDataToValue(bd));
	};
	raylibModule.SetValue("ExportImageToMemory", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("fileName");
	i->code = INTRINSIC_LAMBDA {
		Image img = ValueToImage(context->GetVar(String("image")));
		String path = context->GetVar(String("fileName")).ToString();
		return IntrinsicResult(ExportImageAsCode(img, path.c_str()));
	};
	raylibModule.SetValue("ExportImageAsCode", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("colorCount");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		int colorCount = context->GetVar(String("colorCount")).IntValue();
		Color* colors = LoadImagePalette(image, colorCount, &colorCount);
		// Convert to MiniScript list
		ValueList result;
		for (int i = 0; i < colorCount; i++) {
			result.Add(ColorToValue(colors[i]));
		}
		UnloadImagePalette(colors);
		return IntrinsicResult(Value(result));
	};
	raylibModule.SetValue("LoadImagePalette", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("fileName");
	i->AddParam("width");
	i->AddParam("height");
	i->AddParam("format");
	i->AddParam("headerSize");
	i->code = INTRINSIC_LAMBDA {
		String path = context->GetVar(String("fileName")).ToString();
		int width = context->GetVar(String("width")).IntValue();
		int height = context->GetVar(String("height")).IntValue();
		int format = context->GetVar(String("format")).IntValue();
		int headerSize = context->GetVar(String("headerSize")).IntValue();
		Image result = LoadImageRaw(path.c_str(), width, height, format, headerSize);
		if (!IsImageValid(result)) return IntrinsicResult::Null;
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("LoadImageRaw", i->GetFunc());

	// Memory management functions (no-ops as MiniScript handles memory)

	i = Intrinsic::Create("");
	i->AddParam("colors");
	i->code = INTRINSIC_LAMBDA {
		// No-op in MiniScript - memory is managed automatically
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadImageColors", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("palette");
	i->code = INTRINSIC_LAMBDA {
		// No-op in MiniScript - memory is managed automatically
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UnloadImagePalette", i->GetFunc());

	// Image manipulation - Alpha/Color

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("threshold");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		float threshold = context->GetVar(String("threshold")).FloatValue();
		Rectangle result = GetImageAlphaBorder(*image, threshold);
		return IntrinsicResult(RectangleToValue(result));
	};
	raylibModule.SetValue("GetImageAlphaBorder", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("x");
	i->AddParam("y");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		int x = context->GetVar(String("x")).IntValue();
		int y = context->GetVar(String("y")).IntValue();
		Color result = GetImageColor(image, x, y);
		return IntrinsicResult(ColorToValue(result));
	};
	raylibModule.SetValue("GetImageColor", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("color");
	i->AddParam("threshold");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		Color color = ValueToColor(context->GetVar(String("color")));
		float threshold = context->GetVar(String("threshold")).FloatValue();
		ImageAlphaClear(image, color, threshold);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageAlphaClear", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("threshold");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		float threshold = context->GetVar(String("threshold")).FloatValue();
		ImageAlphaCrop(image, threshold);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageAlphaCrop", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("alphaMask");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		Image alphaMask = ValueToImage(context->GetVar(String("alphaMask")));
		ImageAlphaMask(image, alphaMask);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageAlphaMask", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		ImageAlphaPremultiply(image);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageAlphaPremultiply", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("color");
	i->AddParam("replace");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		Color color = ValueToColor(context->GetVar(String("color")));
		Color replace = ValueToColor(context->GetVar(String("replace")));
		ImageColorReplace(image, color, replace);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageColorReplace", i->GetFunc());

	// Image manipulation - Processing

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("blurSize");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		int blurSize = context->GetVar(String("blurSize")).IntValue();
		ImageBlurGaussian(image, blurSize);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageBlurGaussian", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("rBpp");
	i->AddParam("gBpp");
	i->AddParam("bBpp");
	i->AddParam("aBpp");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		int rBpp = context->GetVar(String("rBpp")).IntValue();
		int gBpp = context->GetVar(String("gBpp")).IntValue();
		int bBpp = context->GetVar(String("bBpp")).IntValue();
		int aBpp = context->GetVar(String("aBpp")).IntValue();
		ImageDither(image, rBpp, gBpp, bBpp, aBpp);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDither", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("newFormat");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		int newFormat = context->GetVar(String("newFormat")).IntValue();
		ImageFormat(image, newFormat);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageFormat", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("channel");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		int channel = context->GetVar(String("channel")).IntValue();
		Image result = ImageFromChannel(image, channel);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("ImageFromChannel", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("rec");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Image result = ImageFromImage(image, rec);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("ImageFromImage", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("kernel");
	i->AddParam("kernelSize");
	i->code = INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context->GetVar(String("image")));
		if (!image) return IntrinsicResult::Null;
		ValueList kernelList = context->GetVar(String("kernel")).GetList();
		int kernelSize = context->GetVar(String("kernelSize")).IntValue();
		// Convert kernel list to float array
		float* kernel = new float[kernelList.Count()];
		for (int i = 0; i < kernelList.Count(); i++) {
			kernel[i] = kernelList[i].FloatValue();
		}
		ImageKernelConvolution(image, kernel, kernelSize);
		delete[] kernel;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageKernelConvolution", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		ImageMipmaps(image);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageMipmaps", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("offsetX");
	i->AddParam("offsetY");
	i->AddParam("newWidth");
	i->AddParam("newHeight");
	i->AddParam("fill");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		int offsetX = context->GetVar(String("offsetX")).IntValue();
		int offsetY = context->GetVar(String("offsetY")).IntValue();
		int newWidth = context->GetVar(String("newWidth")).IntValue();
		int newHeight = context->GetVar(String("newHeight")).IntValue();
		Color fill = ValueToColor(context->GetVar(String("fill")));
		ImageResizeCanvas(image, newWidth, newHeight, offsetX, offsetY, fill);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageResizeCanvas", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("degrees");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		int degrees = context->GetVar(String("degrees")).IntValue();
		ImageRotate(image, degrees);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageRotate", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->code = INTRINSIC_LAMBDA {
		Value imageVal = context->GetVar(String("image"));
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		ImageToPOT(image, BLACK);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageToPOT", i->GetFunc());

	// Image drawing functions

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("centerX");
	i->AddParam("centerY");
	i->AddParam("radius");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		int centerX = context->GetVar(String("centerX")).IntValue();
		int centerY = context->GetVar(String("centerY")).IntValue();
		int radius = context->GetVar(String("radius")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawCircleLines(dst, centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawCircleLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("center");
	i->AddParam("radius");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 center = ValueToVector2(context->GetVar(String("center")));
		int radius = context->GetVar(String("radius")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawCircleLinesV(dst, center, radius, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawCircleLinesV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("start");
	i->AddParam("end");
	i->AddParam("thick");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 start = ValueToVector2(context->GetVar(String("start")));
		Vector2 end = ValueToVector2(context->GetVar(String("end")));
		int thick = context->GetVar(String("thick")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawLineEx(dst, start, end, thick, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawLineEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("rec");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawRectangleV(dst, CLITERAL(Vector2){rec.x, rec.y}, CLITERAL(Vector2){rec.width, rec.height}, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawRectangleV", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("font");
	i->AddParam("text");
	i->AddParam("position");
	i->AddParam("fontSize");
	i->AddParam("spacing");
	i->AddParam("tint");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Font font = ValueToFont(context->GetVar(String("font")));
		String text = context->GetVar(String("text")).ToString();
		Vector2 position = ValueToVector2(context->GetVar(String("position")));
		float fontSize = context->GetVar(String("fontSize")).FloatValue();
		float spacing = context->GetVar(String("spacing")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		ImageDrawTextEx(dst, font, text.c_str(), position, fontSize, spacing, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawTextEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("v1");
	i->AddParam("v2");
	i->AddParam("v3");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		Vector2 v3 = ValueToVector2(context->GetVar(String("v3")));
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawTriangle(dst, v1, v2, v3, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawTriangle", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("v1");
	i->AddParam("v2");
	i->AddParam("v3");
	i->AddParam("c1");
	i->AddParam("c2");
	i->AddParam("c3");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		Vector2 v3 = ValueToVector2(context->GetVar(String("v3")));
		Color c1 = ValueToColor(context->GetVar(String("c1")));
		Color c2 = ValueToColor(context->GetVar(String("c2")));
		Color c3 = ValueToColor(context->GetVar(String("c3")));
		ImageDrawTriangleEx(dst, v1, v2, v3, c1, c2, c3);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawTriangleEx", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("points");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;
		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawTriangleFan(dst, points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawTriangleFan", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("v1");
	i->AddParam("v2");
	i->AddParam("v3");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		Vector2 v1 = ValueToVector2(context->GetVar(String("v1")));
		Vector2 v2 = ValueToVector2(context->GetVar(String("v2")));
		Vector2 v3 = ValueToVector2(context->GetVar(String("v3")));
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawTriangleLines(dst, v1, v2, v3, color);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawTriangleLines", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("dst");
	i->AddParam("points");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context->GetVar(String("dst")));
		if (!dst) return IntrinsicResult::Null;
		ValueList pointsList = context->GetVar(String("points")).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;
		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}
		Color color = ValueToColor(context->GetVar(String("color")));
		ImageDrawTriangleStrip(dst, points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("ImageDrawTriangleStrip", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("text");
	i->AddParam("fontSize");
	i->AddParam("color");
	i->code = INTRINSIC_LAMBDA {
		String text = context->GetVar(String("text")).ToString();
		int fontSize = context->GetVar(String("fontSize")).IntValue();
		Color color = ValueToColor(context->GetVar(String("color")));
		Image result = ImageText(text.c_str(), fontSize, color);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("ImageText", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("font");
	i->AddParam("text");
	i->AddParam("fontSize");
	i->AddParam("spacing");
	i->AddParam("tint");
	i->code = INTRINSIC_LAMBDA {
		Font font = ValueToFont(context->GetVar(String("font")));
		String text = context->GetVar(String("text")).ToString();
		float fontSize = context->GetVar(String("fontSize")).FloatValue();
		float spacing = context->GetVar(String("spacing")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		Image result = ImageTextEx(font, text.c_str(), fontSize, spacing, tint);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	};
	raylibModule.SetValue("ImageTextEx", i->GetFunc());

	// Additional texture functions

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("nPatchInfo");
	i->AddParam("dest");
	i->AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i->AddParam("rotation", Value::zero);
	i->AddParam("tint", ColorToValue(WHITE));
	i->code = INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		ValueDict nPatchDict = context->GetVar(String("nPatchInfo")).GetDict();
		NPatchInfo nPatchInfo;
		ValueDict sourceDict = nPatchDict.Lookup(String("source"), Value::null).GetDict();
		nPatchInfo.source.x = sourceDict.Lookup(String("x"), Value::zero).FloatValue();
		nPatchInfo.source.y = sourceDict.Lookup(String("y"), Value::zero).FloatValue();
		nPatchInfo.source.width = sourceDict.Lookup(String("width"), Value::zero).FloatValue();
		nPatchInfo.source.height = sourceDict.Lookup(String("height"), Value::zero).FloatValue();
		nPatchInfo.left = nPatchDict.Lookup(String("left"), Value::zero).IntValue();
		nPatchInfo.top = nPatchDict.Lookup(String("top"), Value::zero).IntValue();
		nPatchInfo.right = nPatchDict.Lookup(String("right"), Value::zero).IntValue();
		nPatchInfo.bottom = nPatchDict.Lookup(String("bottom"), Value::zero).IntValue();
		nPatchInfo.layout = nPatchDict.Lookup(String("layout"), Value::zero).IntValue();
		Rectangle dest = ValueToRectangle(context->GetVar(String("dest")));
		Vector2 origin = ValueToVector2(context->GetVar(String("origin")));
		float rotation = context->GetVar(String("rotation")).FloatValue();
		Color tint = ValueToColor(context->GetVar(String("tint")));
		DrawTextureNPatch(texture, nPatchInfo, dest, origin, rotation, tint);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("DrawTextureNPatch", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("pixels");
	i->code = INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		BinaryData* data = ValueToRawData(context->GetVar(String("pixels")));
		if (!data) return IntrinsicResult::Null;
		UpdateTexture(texture, data->bytes);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateTexture", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("texture");
	i->AddParam("rec");
	i->AddParam("pixels");
	i->code = INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context->GetVar(String("texture")));
		Rectangle rec = ValueToRectangle(context->GetVar(String("rec")));
		BinaryData* data = ValueToRawData(context->GetVar(String("pixels")));
		if (!data) return IntrinsicResult::Null;
		UpdateTextureRec(texture, rec, data->bytes);
		return IntrinsicResult::Null;
	};
	raylibModule.SetValue("UpdateTextureRec", i->GetFunc());

	i = Intrinsic::Create("");
	i->AddParam("image");
	i->AddParam("layout");
	i->code = INTRINSIC_LAMBDA {
		Image image = ValueToImage(context->GetVar(String("image")));
		int layout = context->GetVar(String("layout")).IntValue();
		Texture2D result = LoadTextureCubemap(image, layout);
		rcTexture++;
		return IntrinsicResult(TextureToValue(result));
	};
	raylibModule.SetValue("LoadTextureCubemap", i->GetFunc());
}
