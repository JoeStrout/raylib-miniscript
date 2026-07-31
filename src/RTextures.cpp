//
//  RTextures.cpp
//  MSRLWeb
//
//  Raylib Textures module intrinsics
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "FileSystem.h"
#include "RawData.h"
#include "raylib.h"
#include "miniscript.h"
#include "macros.h"
#include <iostream>

using namespace MiniScript;

void AddRTexturesMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Image loading

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Image img = LoadImage(path.c_str());
		if (!IsImageValid(img)) return IntrinsicResult::Null;
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("LoadImage", i.GetFunc());

	// Image generation

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("direction", Value::zero);
	i.AddParam("start", ColorToValue(BLACK));
	i.AddParam("end", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		int direction = context.GetArg(2).IntValue();
		Color start = ValueToColor(context.GetArg(3));
		Color end = ValueToColor(context.GetArg(4));
		Image img = GenImageGradientLinear(width, height, direction, start, end);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageGradientLinear", i.GetFunc());

	// Image management

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		UnloadImage(img);
		// Free the heap-allocated Image struct
		ValueDict map = context.GetArg(0).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Image* imgPtr = (Image*)ValueToPointer(handleVal);
		delete imgPtr;
		rcImage--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadImage", i.GetFunc());

	// Texture loading

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		Texture tex = LoadTexture(path.c_str());
		if (!IsTextureValid(tex)) return IntrinsicResult::Null;
		rcTexture++;
		return IntrinsicResult(TextureToValue(tex));
	});
	raylibModule.SetValue("LoadTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		Texture tex = LoadTextureFromImage(img);
		rcTexture++;
		return IntrinsicResult(TextureToValue(tex));
	});
	raylibModule.SetValue("LoadTextureFromImage", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		UnloadTexture(tex);
		// Free the heap-allocated Texture struct
		ValueDict map = context.GetArg(0).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		Texture* texPtr = (Texture*)ValueToPointer(handleVal);
		delete texPtr;
		rcTexture--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadTexture", i.GetFunc());

	// Texture drawing

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		int posX = context.GetArg(1).IntValue();
		int posY = context.GetArg(2).IntValue();
		Color tint = ValueToColor(context.GetArg(3));
		DrawTexture(tex, posX, posY, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		Vector2 position = ValueToVector2(context.GetArg(1));
		Color tint = ValueToColor(context.GetArg(2));
		DrawTextureV(tex, position, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextureV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("scale", Value(1.0));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		Vector2 position = ValueToVector2(context.GetArg(1));
		float rotation = context.GetArg(2).FloatValue();
		float scale = context.GetArg(3).FloatValue();
		Color tint = ValueToColor(context.GetArg(4));
		DrawTextureEx(tex, position, rotation, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextureEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("source");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		Rectangle source = ValueToRectangle(context.GetArg(1));
		Vector2 position = ValueToVector2(context.GetArg(2));
		Color tint = ValueToColor(context.GetArg(3));
		DrawTextureRec(tex, source, position, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextureRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("source");
	i.AddParam("dest");
	i.AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		Rectangle source = ValueToRectangle(context.GetArg(1));
		Rectangle dest = ValueToRectangle(context.GetArg(2));
		Vector2 origin = ValueToVector2(context.GetArg(3));
		float rotation = context.GetArg(4).FloatValue();
		Color tint = ValueToColor(context.GetArg(5));
		DrawTexturePro(tex, source, dest, origin, rotation, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTexturePro", i.GetFunc());

	// More image generation functions

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		Color color = ValueToColor(context.GetArg(2));
		Image img = GenImageColor(width, height, color);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageColor", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("density", Value(0.5));
	i.AddParam("inner", ColorToValue(WHITE));
	i.AddParam("outer", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		float density = context.GetArg(2).FloatValue();
		Color inner = ValueToColor(context.GetArg(3));
		Color outer = ValueToColor(context.GetArg(4));
		Image img = GenImageGradientRadial(width, height, density, inner, outer);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageGradientRadial", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("density", Value(0.5));
	i.AddParam("inner", ColorToValue(WHITE));
	i.AddParam("outer", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		float density = context.GetArg(2).FloatValue();
		Color inner = ValueToColor(context.GetArg(3));
		Color outer = ValueToColor(context.GetArg(4));
		Image img = GenImageGradientSquare(width, height, density, inner, outer);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageGradientSquare", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("checksX", Value(8));
	i.AddParam("checksY", Value(8));
	i.AddParam("col1", ColorToValue(WHITE));
	i.AddParam("col2", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		int checksX = context.GetArg(2).IntValue();
		int checksY = context.GetArg(3).IntValue();
		Color col1 = ValueToColor(context.GetArg(4));
		Color col2 = ValueToColor(context.GetArg(5));
		Image img = GenImageChecked(width, height, checksX, checksY, col1, col2);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageChecked", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("factor", Value(0.5));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		float factor = context.GetArg(2).FloatValue();
		Image img = GenImageWhiteNoise(width, height, factor);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageWhiteNoise", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("tileSize", Value(32));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		int tileSize = context.GetArg(2).IntValue();
		Image img = GenImageCellular(width, height, tileSize);
		rcImage++;
		return IntrinsicResult(ImageToValue(img));
	});
	raylibModule.SetValue("GenImageCellular", i.GetFunc());

	// Image manipulation

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		Image copy = ImageCopy(img);
		rcImage++;
		return IntrinsicResult(ImageToValue(copy));
	});
	raylibModule.SetValue("ImageCopy", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("crop");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		Rectangle crop = ValueToRectangle(context.GetArg(1));
		ImageCrop(img, crop);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageCrop", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("newWidth");
	i.AddParam("newHeight");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		int newWidth = context.GetArg(1).IntValue();
		int newHeight = context.GetArg(2).IntValue();
		ImageResize(img, newWidth, newHeight);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageResize", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("newWidth");
	i.AddParam("newHeight");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		int newWidth = context.GetArg(1).IntValue();
		int newHeight = context.GetArg(2).IntValue();
		ImageResizeNN(img, newWidth, newHeight);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageResizeNN", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		ImageFlipVertical(img);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageFlipVertical", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		ImageFlipHorizontal(img);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageFlipHorizontal", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		ImageRotateCW(img);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageRotateCW", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		ImageRotateCCW(img);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageRotateCCW", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		Color color = ValueToColor(context.GetArg(1));
		ImageColorTint(img, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorTint", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		ImageColorInvert(img);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorInvert", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* img = ValueToImagePtr(imageVal);
		if (!img) return IntrinsicResult::Null;
		ImageColorGrayscale(img);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorGrayscale", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("contrast");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		float contrast = context.GetArg(1).FloatValue();
		ImageColorContrast(img, contrast);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorContrast", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("brightness");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		int brightness = context.GetArg(1).IntValue();
		ImageColorBrightness(img, brightness);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorBrightness", i.GetFunc());

	// Image drawing functions

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Color color = ValueToColor(context.GetArg(1));
		ImageClearBackground(dst, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageClearBackground", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		int x = context.GetArg(1).IntValue();
		int y = context.GetArg(2).IntValue();
		Color color = ValueToColor(context.GetArg(3));
		ImageDrawPixel(dst, x, y, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawPixel", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 position = ValueToVector2(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		ImageDrawPixelV(dst, position, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawPixelV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("startPosX", Value::zero);
	i.AddParam("startPosY", Value::zero);
	i.AddParam("endPosX", Value::zero);
	i.AddParam("endPosY", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		int startPosX = context.GetArg(1).IntValue();
		int startPosY = context.GetArg(2).IntValue();
		int endPosX = context.GetArg(3).IntValue();
		int endPosY = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		ImageDrawLine(dst, startPosX, startPosY, endPosX, endPosY, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawLine", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("start", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("end", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 start = ValueToVector2(context.GetArg(1));
		Vector2 end = ValueToVector2(context.GetArg(2));
		Color color = ValueToColor(context.GetArg(3));
		ImageDrawLineV(dst, start, end, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawLineV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("centerX", Value(100));
	i.AddParam("centerY", Value(100));
	i.AddParam("radius", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		int centerX = context.GetArg(1).IntValue();
		int centerY = context.GetArg(2).IntValue();
		int radius = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		ImageDrawCircle(dst, centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawCircle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("center", Vector2ToValue(Vector2{100, 100}));
	i.AddParam("radius", Value(32));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 center = ValueToVector2(context.GetArg(1));
		int radius = context.GetArg(2).IntValue();
		Color color = ValueToColor(context.GetArg(3));
		ImageDrawCircleV(dst, center, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawCircleV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("width", Value(256));
	i.AddParam("height", Value(256));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		int posX = context.GetArg(1).IntValue();
		int posY = context.GetArg(2).IntValue();
		int width = context.GetArg(3).IntValue();
		int height = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		ImageDrawRectangle(dst, posX, posY, width, height, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("rec");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		ImageDrawRectangleRec(dst, rec, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangleRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("rec");
	i.AddParam("thick", Value(1));
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		int thick = context.GetArg(2).IntValue();
		Color color = ValueToColor(context.GetArg(3));
		ImageDrawRectangleLines(dst, rec, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangleLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.AddParam("srcRec");
	i.AddParam("dstRec");
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		const Image& src = ValueToImage(context.GetArg(1));
		Rectangle srcRec = ValueToRectangle(context.GetArg(2));
		Rectangle dstRec = ValueToRectangle(context.GetArg(3));
		Color tint = ValueToColor(context.GetArg(4));
		ImageDraw(dst, src, srcRec, dstRec, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDraw", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("text");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("fontSize", Value(20));
	i.AddParam("color", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		String text = context.GetArg(1).ToString();
		int posX = context.GetArg(2).IntValue();
		int posY = context.GetArg(3).IntValue();
		int fontSize = context.GetArg(4).IntValue();
		Color color = ValueToColor(context.GetArg(5));
		ImageDrawText(dst, text.c_str(), posX, posY, fontSize, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawText", i.GetFunc());

	// Texture configuration

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("filter");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		int filter = context.GetArg(1).IntValue();
		SetTextureFilter(tex, filter);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTextureFilter", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("wrap");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		int wrap = context.GetArg(1).IntValue();
		SetTextureWrap(tex, wrap);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTextureWrap", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		GenTextureMipmaps(&tex);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("GenTextureMipmaps", i.GetFunc());

	// RenderTexture2D loading/unloading

	i = Intrinsic::Create("");
	i.AddParam("width", Value(960));
	i.AddParam("height", Value(640));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		RenderTexture2D renderTexture = LoadRenderTexture(width, height);
		rcRenderTexture++;
		return IntrinsicResult(RenderTextureToValue(renderTexture));
	});
	raylibModule.SetValue("LoadRenderTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.set_Code(INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context.GetArg(0));
		UnloadRenderTexture(target);
		// Free the heap-allocated RenderTexture2D struct
		ValueDict map = context.GetArg(0).GetDict();
		Value handleVal = map.Lookup(String("_handle"), Value::zero);
		RenderTexture2D* rtPtr = (RenderTexture2D*)ValueToPointer(handleVal);
		delete rtPtr;
		rcRenderTexture--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadRenderTexture", i.GetFunc());

	// RenderTexture2D drawing

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.set_Code(INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context.GetArg(0));
		BeginTextureMode(target);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginTextureMode", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		EndTextureMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndTextureMode", i.GetFunc());

	// Color manipulation functions

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("alpha");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float alpha = context.GetArg(1).FloatValue();
		Color result = ColorAlpha(color, alpha);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorAlpha", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.AddParam("tint");
	i.set_Code(INTRINSIC_LAMBDA {
		Color dst = ValueToColor(context.GetArg(0));
		Color src = ValueToColor(context.GetArg(1));
		Color tint = ValueToColor(context.GetArg(2));
		Color result = ColorAlphaBlend(dst, src, tint);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorAlphaBlend", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("factor");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float factor = context.GetArg(1).FloatValue();
		Color result = ColorBrightness(color, factor);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorBrightness", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("contrast");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float contrast = context.GetArg(1).FloatValue();
		Color result = ColorContrast(color, contrast);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorContrast", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("hue");
	i.AddParam("saturation");
	i.AddParam("value");
	i.set_Code(INTRINSIC_LAMBDA {
		float hue = context.GetArg(0).FloatValue();
		float saturation = context.GetArg(1).FloatValue();
		float value = context.GetArg(2).FloatValue();
		Color result = ColorFromHSV(hue, saturation, value);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorFromHSV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("normalized");
	i.set_Code(INTRINSIC_LAMBDA {
		ValueDict normalized = context.GetArg(0).GetDict();
		Vector4 vec;
		vec.x = normalized.Lookup(String("x"), Value::zero).FloatValue();
		vec.y = normalized.Lookup(String("y"), Value::zero).FloatValue();
		vec.z = normalized.Lookup(String("z"), Value::zero).FloatValue();
		vec.w = normalized.Lookup(String("w"), Value(1.0)).FloatValue();
		Color result = ColorFromNormalized(vec);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorFromNormalized", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("col1");
	i.AddParam("col2");
	i.set_Code(INTRINSIC_LAMBDA {
		Color col1 = ValueToColor(context.GetArg(0));
		Color col2 = ValueToColor(context.GetArg(1));
		bool result = ColorIsEqual(col1, col2);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("ColorIsEqual", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color1");
	i.AddParam("color2");
	i.AddParam("amount");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color1 = ValueToColor(context.GetArg(0));
		Color color2 = ValueToColor(context.GetArg(1));
		float amount = context.GetArg(2).FloatValue();
		Color result = ColorLerp(color1, color2, amount);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorLerp", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		Vector4 result = ColorNormalize(color);
		ValueDict resultDict;
		resultDict.SetValue(String("x"), Value(result.x));
		resultDict.SetValue(String("y"), Value(result.y));
		resultDict.SetValue(String("z"), Value(result.z));
		resultDict.SetValue(String("w"), Value(result.w));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("ColorNormalize", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("tint");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		Color tint = ValueToColor(context.GetArg(1));
		Color result = ColorTint(color, tint);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorTint", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		Vector3 result = ColorToHSV(color);
		ValueDict resultDict;
		resultDict.SetValue(String("x"), Value(result.x));
		resultDict.SetValue(String("y"), Value(result.y));
		resultDict.SetValue(String("z"), Value(result.z));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("ColorToHSV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		int result = ColorToInt(color);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("ColorToInt", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("alpha");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float alpha = context.GetArg(1).FloatValue();
		Color result = Fade(color, alpha);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("Fade", i.GetFunc());

	// Pixel/Color accessor functions

	i = Intrinsic::Create("");
	i.AddParam("hexValue");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int hexValue = context.GetArg(0).IntValue();
		Color result = GetColor(hexValue);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("GetColor", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("srcPtr");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		// srcPtr should be RawData
		BinaryData* data = ValueToRawData(context.GetArg(0));
		if (!data) return IntrinsicResult::Null;
		int format = context.GetArg(1).IntValue();
		Color result = GetPixelColor(data->bytes, format);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("GetPixelColor", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		int format = context.GetArg(2).IntValue();
		int result = GetPixelDataSize(width, height, format);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("GetPixelDataSize", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dstPtr");
	i.AddParam("color");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		// dstPtr should be RawData
		BinaryData* data = ValueToRawData(context.GetArg(0));
		if (!data) return IntrinsicResult::Null;
		Color color = ValueToColor(context.GetArg(1));
		int format = context.GetArg(2).IntValue();
		SetPixelColor(data->bytes, color, format);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetPixelColor", i.GetFunc());

	// Additional image generation functions

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("offsetX");
	i.AddParam("offsetY");
	i.AddParam("scale");
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		float offsetX = context.GetArg(2).FloatValue();
		float offsetY = context.GetArg(3).FloatValue();
		float scale = context.GetArg(4).FloatValue();
		Image result = GenImagePerlinNoise(width, height, offsetX, offsetY, scale);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("GenImagePerlinNoise", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("fontSize");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int fontSize = context.GetArg(1).IntValue();
		Color color = ValueToColor(context.GetArg(2));
		Image result = ImageText(text.c_str(), fontSize, color);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("GenImageText", i.GetFunc());

	// Validation functions

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		bool result = IsImageValid(image);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("IsImageValid", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.set_Code(INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context.GetArg(0));
		bool result = IsRenderTextureValid(target);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("IsRenderTextureValid", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		bool result = IsTextureValid(texture);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("IsTextureValid", i.GetFunc());

	// Additional image loading functions

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("frames");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		int frames = 0;
		Image result = LoadImageAnim(path.c_str(), &frames);
		rcImage++;
		// Return map with image and frames
		ValueDict resultDict;
		resultDict.SetValue(String("image"), ImageToValue(result));
		resultDict.SetValue(String("frames"), Value(frames));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("LoadImageAnim", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("fileType");
	i.AddParam("fileData");
	i.AddParam("frames");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileType = context.GetArg(0).ToString();
		BinaryData* data = ValueToRawData(context.GetArg(1));
		if (!data) return IntrinsicResult::Null;
		int frames = 0;
		Image result = LoadImageAnimFromMemory(fileType.c_str(), data->bytes, data->length, &frames);
		rcImage++;
		// Return map with image and frames
		ValueDict resultDict;
		resultDict.SetValue(String("image"), ImageToValue(result));
		resultDict.SetValue(String("frames"), Value(frames));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("LoadImageAnimFromMemory", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		int colorCount = image.width * image.height;
		Color* colors = LoadImageColors(image);
		// Convert to MiniScript list
		ValueList result;
		for (int i = 0; i < colorCount; i++) {
			result.Add(ColorToValue(colors[i]));
		}
		UnloadImageColors(colors);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadImageColors", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("fileType");
	i.AddParam("fileData");
	i.set_Code(INTRINSIC_LAMBDA {
		String fileType = context.GetArg(0).ToString();
		BinaryData* data = ValueToRawData(context.GetArg(1));
		if (!data) return IntrinsicResult::Null;
		Image result = LoadImageFromMemory(fileType.c_str(), data->bytes, data->length);
		if (!IsImageValid(result)) return IntrinsicResult::Null;
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("LoadImageFromMemory", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Image result = LoadImageFromScreen();
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("LoadImageFromScreen", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		Image result = LoadImageFromTexture(texture);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("LoadImageFromTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult(Value::Truth(false));
		return IntrinsicResult(ExportImage(img, path.c_str()));
	});
	raylibModule.SetValue("ExportImage", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("fileType");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		String fileType = context.GetArg(1).ToString();
		int dataSize = 0;
		unsigned char* data = ExportImageToMemory(img, fileType.c_str(), &dataSize);
		if (!data) return IntrinsicResult::Null;
		BinaryData* bd = new BinaryData(data, dataSize, true);
		return IntrinsicResult(RawDataToValue(bd));
	});
	raylibModule.SetValue("ExportImageToMemory", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult(Value::Truth(false));
		return IntrinsicResult(ExportImageAsCode(img, path.c_str()));
	});
	raylibModule.SetValue("ExportImageAsCode", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("colorCount");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		int colorCount = context.GetArg(1).IntValue();
		Color* colors = LoadImagePalette(image, colorCount, &colorCount);
		// Convert to MiniScript list
		ValueList result;
		for (int i = 0; i < colorCount; i++) {
			result.Add(ColorToValue(colors[i]));
		}
		UnloadImagePalette(colors);
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("LoadImagePalette", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("fileName");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("format");
	i.AddParam("headerSize");
	i.set_Code(INTRINSIC_LAMBDA {
		String path;
		if (!fs::HostPath(context.GetArg(0), path)) return IntrinsicResult::Null;
		int width = context.GetArg(1).IntValue();
		int height = context.GetArg(2).IntValue();
		int format = context.GetArg(3).IntValue();
		int headerSize = context.GetArg(4).IntValue();
		Image result = LoadImageRaw(path.c_str(), width, height, format, headerSize);
		if (!IsImageValid(result)) return IntrinsicResult::Null;
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("LoadImageRaw", i.GetFunc());

	// Memory management functions (no-ops as MiniScript handles memory)

	i = Intrinsic::Create("");
	i.AddParam("colors");
	i.set_Code(INTRINSIC_LAMBDA {
		// No-op in MiniScript - memory is managed automatically
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadImageColors", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("palette");
	i.set_Code(INTRINSIC_LAMBDA {
		// No-op in MiniScript - memory is managed automatically
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadImagePalette", i.GetFunc());

	// Image manipulation - Alpha/Color

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("threshold");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		float threshold = context.GetArg(1).FloatValue();
		Rectangle result = GetImageAlphaBorder(*image, threshold);
		return IntrinsicResult(RectangleToValue(result));
	});
	raylibModule.SetValue("GetImageAlphaBorder", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("x");
	i.AddParam("y");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		int x = context.GetArg(1).IntValue();
		int y = context.GetArg(2).IntValue();
		Color result = GetImageColor(image, x, y);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("GetImageColor", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("color");
	i.AddParam("threshold");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		Color color = ValueToColor(context.GetArg(1));
		float threshold = context.GetArg(2).FloatValue();
		ImageAlphaClear(image, color, threshold);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageAlphaClear", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("threshold");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		float threshold = context.GetArg(1).FloatValue();
		ImageAlphaCrop(image, threshold);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageAlphaCrop", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("alphaMask");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		Image alphaMask = ValueToImage(context.GetArg(1));
		ImageAlphaMask(image, alphaMask);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageAlphaMask", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		ImageAlphaPremultiply(image);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageAlphaPremultiply", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("color");
	i.AddParam("replace");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		Color color = ValueToColor(context.GetArg(1));
		Color replace = ValueToColor(context.GetArg(2));
		ImageColorReplace(image, color, replace);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorReplace", i.GetFunc());

	// Image manipulation - Processing

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("blurSize");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		int blurSize = context.GetArg(1).IntValue();
		ImageBlurGaussian(image, blurSize);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageBlurGaussian", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("rBpp");
	i.AddParam("gBpp");
	i.AddParam("bBpp");
	i.AddParam("aBpp");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		int rBpp = context.GetArg(1).IntValue();
		int gBpp = context.GetArg(2).IntValue();
		int bBpp = context.GetArg(3).IntValue();
		int aBpp = context.GetArg(4).IntValue();
		ImageDither(image, rBpp, gBpp, bBpp, aBpp);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDither", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("newFormat");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		int newFormat = context.GetArg(1).IntValue();
		ImageFormat(image, newFormat);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageFormat", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("channel");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		int channel = context.GetArg(1).IntValue();
		Image result = ImageFromChannel(image, channel);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("ImageFromChannel", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("rec");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		Image result = ImageFromImage(image, rec);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("ImageFromImage", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("kernel");
	i.AddParam("kernelSize");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		ValueList kernelList = context.GetArg(1).GetList();
		int kernelSize = context.GetArg(2).IntValue();
		// Convert kernel list to float array
		float* kernel = new float[kernelList.Count()];
		for (int i = 0; i < kernelList.Count(); i++) {
			kernel[i] = kernelList[i].FloatValue();
		}
		ImageKernelConvolution(image, kernel, kernelSize);
		delete[] kernel;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageKernelConvolution", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		ImageMipmaps(image);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageMipmaps", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("offsetX");
	i.AddParam("offsetY");
	i.AddParam("newWidth");
	i.AddParam("newHeight");
	i.AddParam("fill");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		int offsetX = context.GetArg(1).IntValue();
		int offsetY = context.GetArg(2).IntValue();
		int newWidth = context.GetArg(3).IntValue();
		int newHeight = context.GetArg(4).IntValue();
		Color fill = ValueToColor(context.GetArg(5));
		ImageResizeCanvas(image, newWidth, newHeight, offsetX, offsetY, fill);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageResizeCanvas", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("degrees");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		int degrees = context.GetArg(1).IntValue();
		ImageRotate(image, degrees);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageRotate", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Value imageVal = context.GetArg(0);
		Image* image = ValueToImagePtr(imageVal);
		if (!image) return IntrinsicResult::Null;
		ImageToPOT(image, BLACK);
		UpdateImageValue(imageVal);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageToPOT", i.GetFunc());

	// Image drawing functions

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("centerX");
	i.AddParam("centerY");
	i.AddParam("radius");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		int centerX = context.GetArg(1).IntValue();
		int centerY = context.GetArg(2).IntValue();
		int radius = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		ImageDrawCircleLines(dst, centerX, centerY, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawCircleLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("center");
	i.AddParam("radius");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 center = ValueToVector2(context.GetArg(1));
		int radius = context.GetArg(2).IntValue();
		Color color = ValueToColor(context.GetArg(3));
		ImageDrawCircleLinesV(dst, center, radius, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawCircleLinesV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("start");
	i.AddParam("end");
	i.AddParam("thick");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 start = ValueToVector2(context.GetArg(1));
		Vector2 end = ValueToVector2(context.GetArg(2));
		int thick = context.GetArg(3).IntValue();
		Color color = ValueToColor(context.GetArg(4));
		ImageDrawLineEx(dst, start, end, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawLineEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("rec");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		Color color = ValueToColor(context.GetArg(2));
		ImageDrawRectangleV(dst, CLITERAL(Vector2){rec.x, rec.y}, CLITERAL(Vector2){rec.width, rec.height}, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangleV", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("font");
	i.AddParam("text");
	i.AddParam("position");
	i.AddParam("fontSize");
	i.AddParam("spacing");
	i.AddParam("tint");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Font font = ValueToFont(context.GetArg(1));
		String text = context.GetArg(2).ToString();
		Vector2 position = ValueToVector2(context.GetArg(3));
		float fontSize = context.GetArg(4).FloatValue();
		float spacing = context.GetArg(5).FloatValue();
		Color tint = ValueToColor(context.GetArg(6));
		ImageDrawTextEx(dst, font, text.c_str(), position, fontSize, spacing, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTextEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 v1 = ValueToVector2(context.GetArg(1));
		Vector2 v2 = ValueToVector2(context.GetArg(2));
		Vector2 v3 = ValueToVector2(context.GetArg(3));
		Color color = ValueToColor(context.GetArg(4));
		ImageDrawTriangle(dst, v1, v2, v3, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTriangle", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("c1");
	i.AddParam("c2");
	i.AddParam("c3");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 v1 = ValueToVector2(context.GetArg(1));
		Vector2 v2 = ValueToVector2(context.GetArg(2));
		Vector2 v3 = ValueToVector2(context.GetArg(3));
		Color c1 = ValueToColor(context.GetArg(4));
		Color c2 = ValueToColor(context.GetArg(5));
		Color c3 = ValueToColor(context.GetArg(6));
		ImageDrawTriangleEx(dst, v1, v2, v3, c1, c2, c3);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTriangleEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("points");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		ValueList pointsList = context.GetArg(1).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;
		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}
		Color color = ValueToColor(context.GetArg(2));
		ImageDrawTriangleFan(dst, points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTriangleFan", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("v1");
	i.AddParam("v2");
	i.AddParam("v3");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 v1 = ValueToVector2(context.GetArg(1));
		Vector2 v2 = ValueToVector2(context.GetArg(2));
		Vector2 v3 = ValueToVector2(context.GetArg(3));
		Color color = ValueToColor(context.GetArg(4));
		ImageDrawTriangleLines(dst, v1, v2, v3, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTriangleLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("points");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		ValueList pointsList = context.GetArg(1).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 3) return IntrinsicResult::Null;
		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}
		Color color = ValueToColor(context.GetArg(2));
		ImageDrawTriangleStrip(dst, points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTriangleStrip", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("text");
	i.AddParam("fontSize");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		String text = context.GetArg(0).ToString();
		int fontSize = context.GetArg(1).IntValue();
		Color color = ValueToColor(context.GetArg(2));
		Image result = ImageText(text.c_str(), fontSize, color);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("ImageText", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("font");
	i.AddParam("text");
	i.AddParam("fontSize");
	i.AddParam("spacing");
	i.AddParam("tint");
	i.set_Code(INTRINSIC_LAMBDA {
		Font font = ValueToFont(context.GetArg(0));
		String text = context.GetArg(1).ToString();
		float fontSize = context.GetArg(2).FloatValue();
		float spacing = context.GetArg(3).FloatValue();
		Color tint = ValueToColor(context.GetArg(4));
		Image result = ImageTextEx(font, text.c_str(), fontSize, spacing, tint);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("ImageTextEx", i.GetFunc());

	// Additional texture functions

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("nPatchInfo");
	i.AddParam("dest");
	i.AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		ValueDict nPatchDict = context.GetArg(1).GetDict();
		NPatchInfo nPatchInfo;
		ValueDict sourceDict = nPatchDict.Lookup(String("source"), Value::Null).GetDict();
		nPatchInfo.source.x = sourceDict.Lookup(String("x"), Value::zero).FloatValue();
		nPatchInfo.source.y = sourceDict.Lookup(String("y"), Value::zero).FloatValue();
		nPatchInfo.source.width = sourceDict.Lookup(String("width"), Value::zero).FloatValue();
		nPatchInfo.source.height = sourceDict.Lookup(String("height"), Value::zero).FloatValue();
		nPatchInfo.left = nPatchDict.Lookup(String("left"), Value::zero).IntValue();
		nPatchInfo.top = nPatchDict.Lookup(String("top"), Value::zero).IntValue();
		nPatchInfo.right = nPatchDict.Lookup(String("right"), Value::zero).IntValue();
		nPatchInfo.bottom = nPatchDict.Lookup(String("bottom"), Value::zero).IntValue();
		nPatchInfo.layout = nPatchDict.Lookup(String("layout"), Value::zero).IntValue();
		Rectangle dest = ValueToRectangle(context.GetArg(2));
		Vector2 origin = ValueToVector2(context.GetArg(3));
		float rotation = context.GetArg(4).FloatValue();
		Color tint = ValueToColor(context.GetArg(5));
		DrawTextureNPatch(texture, nPatchInfo, dest, origin, rotation, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextureNPatch", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("pixels");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		BinaryData* data = ValueToRawData(context.GetArg(1));
		if (!data) return IntrinsicResult::Null;
		UpdateTexture(texture, data->bytes);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("rec");
	i.AddParam("pixels");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		BinaryData* data = ValueToRawData(context.GetArg(2));
		if (!data) return IntrinsicResult::Null;
		UpdateTextureRec(texture, rec, data->bytes);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UpdateTextureRec", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("layout");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		int layout = context.GetArg(1).IntValue();
		Texture2D result = LoadTextureCubemap(image, layout);
		rcTexture++;
		return IntrinsicResult(TextureToValue(result));
	});
	raylibModule.SetValue("LoadTextureCubemap", i.GetFunc());
}
