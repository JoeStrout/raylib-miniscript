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
#include "Profiler.h"
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
	raylibModule.SetValue("LoadImage", WithNote(i,
		"Load image from file into CPU memory (RAM); returns null on failure"));

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
	raylibModule.SetValue("GenImageGradientLinear", WithNote(i,
		"Generate image: linear gradient. direction is in degrees: 0 is vertical (from top to "
		"bottom), 90 is horizontal (from left to right); the gradient rotates counter-clockwise "
		"by the specified amount"));

	// Image management

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img;
		if (!TakeResource(context.GetArg(0), &img)) return IntrinsicResult::Null;
		UnloadImage(img);
		rcImage--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadImage", WithNote(i,
		"Unload image from CPU memory (RAM)"));

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
	raylibModule.SetValue("LoadTexture", WithNote(i,
		"Load texture from file into GPU memory (VRAM); returns null on failure"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		Texture tex = LoadTextureFromImage(img);
		rcTexture++;
		return IntrinsicResult(TextureToValue(tex));
	});
	raylibModule.SetValue("LoadTextureFromImage", WithNote(i,
		"Load a texture from image data. The image is not affected and remains usable"));

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex;
		if (!TakeResource(context.GetArg(0), &tex)) return IntrinsicResult::Null;
		UnloadTexture(tex);
		rcTexture--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadTexture", WithNote(i,
		"Unload texture from GPU memory (VRAM)"));

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
	raylibModule.SetValue("DrawTexture", WithNote(i,
		"Draw a texture"));

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
	raylibModule.SetValue("DrawTextureV", WithNote(i,
		"Draw a texture with position defined as a Vector2"));

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
	raylibModule.SetValue("DrawTextureEx", WithNote(i,
		"Draw a texture with rotation (degrees) and scale. NOTE: Rotation is around the top-left "
		"corner"));

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("rec");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		Vector2 position = ValueToVector2(context.GetArg(2));
		Color tint = ValueToColor(context.GetArg(3));
		DrawTextureRec(tex, rec, position, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextureRec", WithNote(i,
		"Draw a part of a texture (defined by a rectangle)"));

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
	raylibModule.SetValue("DrawTexturePro", WithNote(i,
		"Draw a part of a texture (source rectangle) into a destination rectangle, with origin "
		"and rotation (degrees). NOTE: origin is relative to the destination rectangle"));

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
	raylibModule.SetValue("GenImageColor", WithNote(i,
		"Generate image: plain color"));

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
	raylibModule.SetValue("GenImageGradientRadial", WithNote(i,
		"Generate image: radial gradient"));

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
	raylibModule.SetValue("GenImageGradientSquare", WithNote(i,
		"Generate image: square gradient"));

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
	raylibModule.SetValue("GenImageChecked", WithNote(i,
		"Generate image: checkerboard"));

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
	raylibModule.SetValue("GenImageWhiteNoise", WithNote(i,
		"Generate image: white noise; factor (0-1) is the fraction of white pixels"));

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
	raylibModule.SetValue("GenImageCellular", WithNote(i,
		"Generate image: cellular algorithm. Bigger tileSize means bigger cells"));

	// Image manipulation

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		Image copy = ImageCopy(img);
		rcImage++;
		return IntrinsicResult(ImageToValue(copy));
	});
	raylibModule.SetValue("ImageCopy", WithNote(i,
		"Copy an image to a new image"));

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
	raylibModule.SetValue("ImageCrop", WithNote(i,
		"Crop an image (in place) to the area defined by a rectangle. The rectangle is clamped "
		"to the image bounds"));

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
	raylibModule.SetValue("ImageResize", WithNote(i,
		"Resize an image (in place) to a new size, using high-quality bicubic filtering"));

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
	raylibModule.SetValue("ImageResizeNN", WithNote(i,
		"Resize an image (in place) to a new size using nearest-neighbor scaling"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		ImageFlipVertical(img);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageFlipVertical", WithNote(i,
		"Flip image vertically"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		ImageFlipHorizontal(img);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageFlipHorizontal", WithNote(i,
		"Flip image horizontally"));

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
	raylibModule.SetValue("ImageRotateCW", WithNote(i,
		"Rotate image clockwise 90 degrees"));

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
	raylibModule.SetValue("ImageRotateCCW", WithNote(i,
		"Rotate image counter-clockwise 90 degrees"));

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
	raylibModule.SetValue("ImageColorTint", WithNote(i,
		"Modify image color: tint"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		ImageColorInvert(img);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorInvert", WithNote(i,
		"Modify image color: invert"));

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
	raylibModule.SetValue("ImageColorGrayscale", WithNote(i,
		"Modify image color: grayscale"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("contrast");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* img = ValueToImagePtr(context.GetArg(0));
		if (!img) return IntrinsicResult::Null;
		int contrast = context.GetArg(1).IntValue();
		ImageColorContrast(img, contrast);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageColorContrast", WithNote(i,
		"Modify image color: contrast (-100 to 100)"));

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
	raylibModule.SetValue("ImageColorBrightness", WithNote(i,
		"Modify image color: brightness (-255 to 255)"));

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
	raylibModule.SetValue("ImageClearBackground", WithNote(i,
		"Clear image background with given color"));

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
	raylibModule.SetValue("ImageDrawPixel", WithNote(i,
		"Draw pixel within an image. NOTE: Compressed image formats not supported"));

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
	raylibModule.SetValue("ImageDrawPixelV", WithNote(i,
		"Draw pixel within an image (Vector version)"));

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
	raylibModule.SetValue("ImageDrawLine", WithNote(i,
		"Draw line within an image"));

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
	raylibModule.SetValue("ImageDrawLineV", WithNote(i,
		"Draw line within an image (Vector version)"));

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
	raylibModule.SetValue("ImageDrawCircle", WithNote(i,
		"Draw circle within an image"));

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
	raylibModule.SetValue("ImageDrawCircleV", WithNote(i,
		"Draw circle within an image (Vector version)"));

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
	raylibModule.SetValue("ImageDrawRectangle", WithNote(i,
		"Draw rectangle within an image"));

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
	raylibModule.SetValue("ImageDrawRectangleRec", WithNote(i,
		"Draw rectangle within an image"));

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
		ImageDrawRectangleLines(dst, posX, posY, width, height, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangleLines", WithNote(i,
		"Draw rectangle lines within an image"));

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
		ImageDrawRectangleLinesEx(dst, rec, thick, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangleLinesEx", WithNote(i,
		"Draw rectangle lines within an image with line thickness"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("rec");
	i.AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		Vector2 origin = ValueToVector2(context.GetArg(2));
		float rotation = context.GetArg(3).FloatValue();
		Color color = ValueToColor(context.GetArg(4));
		ImageDrawRectanglePro(dst, rec, origin, rotation, color);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectanglePro", WithNote(i,
		"Draw a color-filled rectangle with origin and rotation (degrees) within an image"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("rec");
	i.AddParam("col1");
	i.AddParam("col2");
	i.AddParam("col3");
	i.AddParam("col4");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Rectangle rec = ValueToRectangle(context.GetArg(1));
		Color col1 = ValueToColor(context.GetArg(2));
		Color col2 = ValueToColor(context.GetArg(3));
		Color col3 = ValueToColor(context.GetArg(4));
		Color col4 = ValueToColor(context.GetArg(5));
		ImageDrawRectangleGradientEx(dst, rec, col1, col2, col3, col4);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawRectangleGradientEx", WithNote(i,
		"Draw rectangle with gradient colors within an image, counter-clockwise color order"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.AddParam("posX", Value::zero);
	i.AddParam("posY", Value::zero);
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		const Image& src = ValueToImage(context.GetArg(1));
		int posX = context.GetArg(2).IntValue();
		int posY = context.GetArg(3).IntValue();
		Color tint = ValueToColor(context.GetArg(4));
		ImageDrawImage(dst, src, posX, posY, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawImage", WithNote(i,
		"Draw an image within an image"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("scale", Value(1.0));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		const Image& src = ValueToImage(context.GetArg(1));
		Vector2 position = ValueToVector2(context.GetArg(2));
		float rotation = context.GetArg(3).FloatValue();
		float scale = context.GetArg(4).FloatValue();
		Color tint = ValueToColor(context.GetArg(5));
		ImageDrawImageEx(dst, src, position, rotation, scale, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawImageEx", WithNote(i,
		"Draw an image with scaling and rotation (degrees) within an image. NOTE: Rotation is "
		"around the top-left corner"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.AddParam("srcRec");
	i.AddParam("position", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		const Image& src = ValueToImage(context.GetArg(1));
		Rectangle srcRec = ValueToRectangle(context.GetArg(2));
		Vector2 position = ValueToVector2(context.GetArg(3));
		Color tint = ValueToColor(context.GetArg(4));
		ImageDrawImageRec(dst, src, srcRec, position, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawImageRec", WithNote(i,
		"Draw a part of an image defined by a rectangle within an image"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("src");
	i.AddParam("srcRec");
	i.AddParam("dstRec");
	i.AddParam("origin", Vector2ToValue(Vector2{0, 0}));
	i.AddParam("rotation", Value::zero);
	i.AddParam("tint", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		const Image& src = ValueToImage(context.GetArg(1));
		Rectangle srcRec = ValueToRectangle(context.GetArg(2));
		Rectangle dstRec = ValueToRectangle(context.GetArg(3));
		Vector2 origin = ValueToVector2(context.GetArg(4));
		float rotation = context.GetArg(5).FloatValue();
		Color tint = ValueToColor(context.GetArg(6));
		ImageDrawImagePro(dst, src, srcRec, dstRec, origin, rotation, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawImagePro", WithNote(i,
		"Draw a part of an image defined by a rectangle into destination rectangle, with scaling, "
		"within an image. NOTE: origin and rotation are currently ignored by raylib"));

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
	raylibModule.SetValue("ImageDrawText", WithNote(i,
		"Draw text (default font) within an image"));

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
	raylibModule.SetValue("SetTextureFilter", WithNote(i,
		"Set texture scaling filter mode (a TEXTURE_FILTER_* value)"));

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.AddParam("wrap");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture tex = ValueToTexture(context.GetArg(0));
		int wrap = context.GetArg(1).IntValue();
		SetTextureWrap(tex, wrap);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("SetTextureWrap", WithNote(i,
		"Set texture wrapping mode (a TEXTURE_WRAP_* value)"));

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Value texValue = context.GetArg(0);
		Texture* texPtr = ResourcePtr<Texture>(texValue);
		if (texPtr == nullptr) return IntrinsicResult::Null;
		GenTextureMipmaps(texPtr);		// sets the mipmap count SetTextureFilter checks
		UpdateTextureValue(texValue);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("GenTextureMipmaps", WithNote(i,
		"Generate GPU mipmaps for a texture"));

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
	raylibModule.SetValue("LoadRenderTexture", WithNote(i,
		"Load RGBA texture for rendering (framebuffer)"));

	i = Intrinsic::Create("");
	i.AddParam("width", Value(960));
	i.AddParam("height", Value(640));
	i.AddParam("format", Value(PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		int format = context.GetArg(2).IntValue();
		RenderTexture2D renderTexture = LoadRenderTextureEx(width, height, format);
		rcRenderTexture++;
		return IntrinsicResult(RenderTextureToValue(renderTexture));
	});
	raylibModule.SetValue("LoadRenderTextureEx", WithNote(i,
		"Load texture for rendering (framebuffer), with specific pixel format (a PIXELFORMAT_* "
		"value)"));

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.set_Code(INTRINSIC_LAMBDA {
		RenderTexture2D target;
		if (!TakeResource(context.GetArg(0), &target)) return IntrinsicResult::Null;
		UnloadRenderTexture(target);
		rcRenderTexture--;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadRenderTexture", WithNote(i,
		"Unload render texture from GPU memory (VRAM)"));

	// RenderTexture2D drawing

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.set_Code(INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context.GetArg(0));
		profiler::TimedScope timed("raylib.BeginTextureMode");
		BeginTextureMode(target);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("BeginTextureMode", WithNote(i,
		"Begin drawing to render texture"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		profiler::TimedScope timed("raylib.EndTextureMode");
		EndTextureMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("EndTextureMode", WithNote(i,
		"End drawing to render texture"));

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
	raylibModule.SetValue("ColorAlpha", WithNote(i,
		"Get color with alpha applied, alpha goes from 0.0 to 1.0"));

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
	raylibModule.SetValue("ColorAlphaBlend", WithNote(i,
		"Get src alpha-blended into dst color with tint"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("factor");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float factor = context.GetArg(1).FloatValue();
		Color result = ColorBrightness(color, factor);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorBrightness", WithNote(i,
		"Get color with brightness correction, brightness factor goes from -1.0 to 1.0"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("contrast");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float contrast = context.GetArg(1).FloatValue();
		Color result = ColorContrast(color, contrast);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorContrast", WithNote(i,
		"Get color with contrast correction, contrast goes from -1.0 to 1.0"));

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
	raylibModule.SetValue("ColorFromHSV", WithNote(i,
		"Get a color from HSV values. Hue is in degrees [0..360]; saturation and value are "
		"[0.0..1.0]. NOTE: Color->HSV->Color conversion may not yield exactly the same color due "
		"to rounding errors"));

	i = Intrinsic::Create("");
	i.AddParam("normalized");
	i.set_Code(INTRINSIC_LAMBDA {
		ValueDict normalized = context.GetArg(0).GetDict();
		Vector4 vec;
		vec.x = normalized.Lookup(CACHED_KEY("x"), Value::zero).FloatValue();
		vec.y = normalized.Lookup(CACHED_KEY("y"), Value::zero).FloatValue();
		vec.z = normalized.Lookup(CACHED_KEY("z"), Value::zero).FloatValue();
		vec.w = normalized.Lookup(CACHED_KEY("w"), Value(1.0)).FloatValue();
		Color result = ColorFromNormalized(vec);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorFromNormalized", WithNote(i,
		"Get color from a map of normalized values [0..1]: x=red, y=green, z=blue, w=alpha "
		"(w defaults to 1)"));

	i = Intrinsic::Create("");
	i.AddParam("col1");
	i.AddParam("col2");
	i.set_Code(INTRINSIC_LAMBDA {
		Color col1 = ValueToColor(context.GetArg(0));
		Color col2 = ValueToColor(context.GetArg(1));
		bool result = ColorIsEqual(col1, col2);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("ColorIsEqual", WithNote(i,
		"Check if two colors are equal"));

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
	raylibModule.SetValue("ColorLerp", WithNote(i,
		"Get color linearly interpolated between two colors, amount [0.0..1.0]"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		Vector4 result = ColorNormalize(color);
		ValueDict resultDict;
		resultDict.SetValue(CACHED_KEY("x"), Value(result.x));
		resultDict.SetValue(CACHED_KEY("y"), Value(result.y));
		resultDict.SetValue(CACHED_KEY("z"), Value(result.z));
		resultDict.SetValue(CACHED_KEY("w"), Value(result.w));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("ColorNormalize", WithNote(i,
		"Get color normalized as a map of values [0..1]: x=red, y=green, z=blue, w=alpha"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("tint");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		Color tint = ValueToColor(context.GetArg(1));
		Color result = ColorTint(color, tint);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("ColorTint", WithNote(i,
		"Get color multiplied with another color"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		Vector3 result = ColorToHSV(color);
		ValueDict resultDict;
		resultDict.SetValue(CACHED_KEY("x"), Value(result.x));
		resultDict.SetValue(CACHED_KEY("y"), Value(result.y));
		resultDict.SetValue(CACHED_KEY("z"), Value(result.z));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("ColorToHSV", WithNote(i,
		"Get HSV values for a color, as a map: x=hue in degrees [0..360], y=saturation and "
		"z=value in [0..1]"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		int result = ColorToInt(color);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("ColorToInt", WithNote(i,
		"Get a color as a 32-bit integer (0xRRGGBBAA); may be negative when red >= 128"));

	i = Intrinsic::Create("");
	i.AddParam("color");
	i.AddParam("alpha");
	i.set_Code(INTRINSIC_LAMBDA {
		Color color = ValueToColor(context.GetArg(0));
		float alpha = context.GetArg(1).FloatValue();
		Color result = Fade(color, alpha);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("Fade", WithNote(i,
		"Get color with alpha applied, alpha goes from 0.0 to 1.0"));

	// Pixel/Color accessor functions

	i = Intrinsic::Create("");
	i.AddParam("hexValue");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int hexValue = context.GetArg(0).IntValue();
		Color result = GetColor(hexValue);
		return IntrinsicResult(ColorToValue(result));
	});
	raylibModule.SetValue("GetColor", WithNote(i,
		"Get a color from a 32-bit integer value (0xRRGGBBAA)"));

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
	raylibModule.SetValue("GetPixelColor", WithNote(i,
		"Get the color of the pixel at the start of a RawData, interpreted in the given pixel "
		"format (a PIXELFORMAT_* value)"));

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
	raylibModule.SetValue("GetPixelDataSize", WithNote(i,
		"Get pixel data size in bytes for the given width, height, and pixel format (works for "
		"image or texture data)"));

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
	raylibModule.SetValue("SetPixelColor", WithNote(i,
		"Write a color, in the given pixel format (a PIXELFORMAT_* value), to the start of a "
		"RawData"));

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
	raylibModule.SetValue("GenImagePerlinNoise", WithNote(i,
		"Generate image: perlin noise"));

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
	raylibModule.SetValue("GenImageText", WithNote(i,
		"Generate image: text drawn in the default font (same as ImageText)"));

	// Validation functions

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image image = ValueToImage(context.GetArg(0));
		bool result = IsImageValid(image);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("IsImageValid", WithNote(i,
		"Check if an image is valid (has data and a valid size and format)"));

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.set_Code(INTRINSIC_LAMBDA {
		RenderTexture2D target = ValueToRenderTexture(context.GetArg(0));
		bool result = IsRenderTextureValid(target);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("IsRenderTextureValid", WithNote(i,
		"Check if render texture is valid (loaded in GPU)"));

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		bool result = IsTextureValid(texture);
		return IntrinsicResult(Value(result));
	});
	raylibModule.SetValue("IsTextureValid", WithNote(i,
		"Check if texture is valid (loaded in GPU)"));

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
		resultDict.SetValue(CACHED_KEY("image"), ImageToValue(result));
		resultDict.SetValue(CACHED_KEY("frames"), Value(frames));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("LoadImageAnim", WithNote(i,
		"Load animated image data (e.g. GIF). Returns a map with 'image' and 'frames' (the frame "
		"count). The image's pixel data holds all frames back to back, in RGBA format; frame "
		"delays are discarded. The frames argument is ignored"));

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
		resultDict.SetValue(CACHED_KEY("image"), ImageToValue(result));
		resultDict.SetValue(CACHED_KEY("frames"), Value(frames));
		return IntrinsicResult(DynamicMap(resultDict));
	});
	raylibModule.SetValue("LoadImageAnimFromMemory", WithNote(i,
		"Load animated image data from a RawData; fileType is the extension, e.g. \".gif\". "
		"Returns a map with 'image' and 'frames' (the frame count). The image's pixel data holds "
		"all frames back to back, in RGBA format; frame delays are discarded. The frames argument "
		"is ignored"));

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
	raylibModule.SetValue("LoadImageColors", WithNote(i,
		"Get the pixel colors of an image as a list of colors, row by row from the top-left"));

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
	raylibModule.SetValue("LoadImageFromMemory", WithNote(i,
		"Load image from file data in a RawData; fileType is the extension, e.g. \".png\" "
		"(must be lower-case). Returns null on failure"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		Image result = LoadImageFromScreen();
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("LoadImageFromScreen", WithNote(i,
		"Load image from screen buffer (screenshot)"));

	i = Intrinsic::Create("");
	i.AddParam("texture");
	i.set_Code(INTRINSIC_LAMBDA {
		Texture2D texture = ValueToTexture(context.GetArg(0));
		Image result = LoadImageFromTexture(texture);
		rcImage++;
		return IntrinsicResult(ImageToValue(result));
	});
	raylibModule.SetValue("LoadImageFromTexture", WithNote(i,
		"Load image from GPU texture data. NOTE: Compressed texture formats not supported"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult::Zero;
		return IntrinsicResult(ExportImage(img, path.c_str()));
	});
	raylibModule.SetValue("ExportImage", WithNote(i,
		"Export image data to file; the file format depends on the fileName extension. Returns "
		"true on success"));

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
	raylibModule.SetValue("ExportImageToMemory", WithNote(i,
		"Export an image to a RawData in the file format named by fileType (e.g. \".png\"), or "
		"null on failure"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("fileName");
	i.set_Code(INTRINSIC_LAMBDA {
		Image img = ValueToImage(context.GetArg(0));
		String path;
		if (!fs::HostPathForWrite(context.GetArg(1), path)) return IntrinsicResult::Zero;
		return IntrinsicResult(ExportImageAsCode(img, path.c_str()));
	});
	raylibModule.SetValue("ExportImageAsCode", WithNote(i,
		"Export image as a C code file (.h) defining an array of bytes. Returns true on success"));

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
	raylibModule.SetValue("LoadImagePalette", WithNote(i,
		"Get the distinct colors used in an image, as a list of up to colorCount colors"));

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
	raylibModule.SetValue("LoadImageRaw", WithNote(i,
		"Load an image from a raw pixel data file, with the given size, pixel format (a "
		"PIXELFORMAT_* value), and number of header bytes to skip. Returns null on failure"));

	// Memory management functions (no-ops as MiniScript handles memory)

	i = Intrinsic::Create("");
	i.AddParam("colors");
	i.set_Code(INTRINSIC_LAMBDA {
		// No-op in MiniScript - memory is managed automatically
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadImageColors", WithNote(i,
		"Does nothing; provided for compatibility (LoadImageColors returns an ordinary list)"));

	i = Intrinsic::Create("");
	i.AddParam("palette");
	i.set_Code(INTRINSIC_LAMBDA {
		// No-op in MiniScript - memory is managed automatically
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("UnloadImagePalette", WithNote(i,
		"Does nothing; provided for compatibility (LoadImagePalette returns an ordinary list)"));

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
	raylibModule.SetValue("GetImageAlphaBorder", WithNote(i,
		"Get the rectangle bounding the image's non-transparent pixels; threshold is an alpha "
		"level from 0.0 to 1.0"));

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
	raylibModule.SetValue("GetImageColor", WithNote(i,
		"Get image pixel color at (x, y) position"));

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
	raylibModule.SetValue("ImageAlphaClear", WithNote(i,
		"Replace pixels with alpha at or below threshold (0.0 to 1.0) with the given color"));

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
	raylibModule.SetValue("ImageAlphaCrop", WithNote(i,
		"Crop image (in place) to its non-transparent area; threshold is an alpha level from 0.0 "
		"to 1.0"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.AddParam("alphaMask");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		Image alphaMask = ValueToImage(context.GetArg(1));
		ImageAlphaMask(image, alphaMask);
		UpdateImageValue(context.GetArg(0));	// the format changes
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageAlphaMask", WithNote(i,
		"Apply alpha mask to image (in place); the image becomes GRAY_ALPHA or RGBA format. "
		"alphaMask should be the same size as image"));

	i = Intrinsic::Create("");
	i.AddParam("image");
	i.set_Code(INTRINSIC_LAMBDA {
		Image* image = ValueToImagePtr(context.GetArg(0));
		if (!image) return IntrinsicResult::Null;
		ImageAlphaPremultiply(image);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageAlphaPremultiply", WithNote(i,
		"Premultiply alpha channel"));

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
	raylibModule.SetValue("ImageColorReplace", WithNote(i,
		"Modify image color: replace color"));

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
	raylibModule.SetValue("ImageBlurGaussian", WithNote(i,
		"Apply Gaussian blur (box blur approximation) to image"));

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
		UpdateImageValue(context.GetArg(0));	// the format changes
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDither", WithNote(i,
		"Dither image data to 16bpp or lower (Floyd-Steinberg dithering), given bits per channel "
		"for r, g, b, a"));

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
	raylibModule.SetValue("ImageFormat", WithNote(i,
		"Convert image data (in place) to desired pixel format (a PIXELFORMAT_* value)"));

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
	raylibModule.SetValue("ImageFromChannel", WithNote(i,
		"Create a grayscale image from a selected channel (0-3 for R, G, B, A) of another image"));

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
	raylibModule.SetValue("ImageFromImage", WithNote(i,
		"Create an image from a rectangular piece of another image"));

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
	raylibModule.SetValue("ImageKernelConvolution", WithNote(i,
		"Apply custom square convolution kernel to image. kernel is a list of numbers, row by row; "
		"kernelSize is the total number of values (e.g. 9 for a 3x3 kernel)"));

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
	raylibModule.SetValue("ImageMipmaps", WithNote(i,
		"Generate all mipmap levels for an image (in place); the image's pixel data grows to "
		"include them. Works with power-of-two and other sizes"));

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
	raylibModule.SetValue("ImageResizeCanvas", WithNote(i,
		"Resize canvas (in place) and fill new area with color. NOTE: offset is relative to the "
		"top-left corner of the original image"));

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
	raylibModule.SetValue("ImageRotate", WithNote(i,
		"Rotate image (in place) by the given degrees"));

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
	raylibModule.SetValue("ImageToPOT", WithNote(i,
		"Convert image (in place) to power-of-two size, padding with black. Can be useful on "
		"OpenGL ES 2.0 (web)"));

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
	raylibModule.SetValue("ImageDrawCircleLines", WithNote(i,
		"Draw circle outline within an image"));

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
	raylibModule.SetValue("ImageDrawCircleLinesV", WithNote(i,
		"Draw circle outline within an image (Vector version)"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("center");
	i.AddParam("radius");
	i.AddParam("inner", ColorToValue(WHITE));
	i.AddParam("outer", ColorToValue(BLACK));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		Vector2 center = ValueToVector2(context.GetArg(1));
		float radius = context.GetArg(2).FloatValue();
		Color inner = ValueToColor(context.GetArg(3));
		Color outer = ValueToColor(context.GetArg(4));
		ImageDrawCircleGradient(dst, center, radius, inner, outer);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawCircleGradient", WithNote(i,
		"Draw a gradient-filled circle within an image"));

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
	raylibModule.SetValue("ImageDrawLineEx", WithNote(i,
		"Draw a line with given thickness within an image"));

	i = Intrinsic::Create("");
	i.AddParam("dst");
	i.AddParam("points");
	i.AddParam("color", ColorToValue(WHITE));
	i.set_Code(INTRINSIC_LAMBDA {
		Image* dst = ValueToImagePtr(context.GetArg(0));
		if (!dst) return IntrinsicResult::Null;
		ValueList pointsList = context.GetArg(1).GetList();
		int pointCount = pointsList.Count();
		if (pointCount < 2) return IntrinsicResult::Null;
		Vector2* points = new Vector2[pointCount];
		for (int i = 0; i < pointCount; i++) {
			points[i] = ValueToVector2(pointsList[i]);
		}
		Color color = ValueToColor(context.GetArg(2));
		ImageDrawLineStrip(dst, points, pointCount, color);
		delete[] points;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawLineStrip", WithNote(i,
		"Draw a sequence of connected lines within an image, given a list of at least 2 points"));

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
	raylibModule.SetValue("ImageDrawRectangleV", WithNote(i,
		"Draw rectangle within an image; rec gives the position (x, y) and size (width, height)"));

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
	raylibModule.SetValue("ImageDrawTextEx", WithNote(i,
		"Draw text (custom font) within an image"));

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
	raylibModule.SetValue("ImageDrawTriangle", WithNote(i,
		"Draw triangle within an image"));

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
		ImageDrawTriangleGradient(dst, v1, v2, v3, c1, c2, c3);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("ImageDrawTriangleGradient", WithNote(i,
		"Draw triangle with interpolated colors within an image"));

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
	raylibModule.SetValue("ImageDrawTriangleFan", WithNote(i,
		"Draw a triangle fan defined by a list of at least 3 points within an image (first point "
		"is the center)"));

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
	raylibModule.SetValue("ImageDrawTriangleLines", WithNote(i,
		"Draw triangle outline within an image"));

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
	raylibModule.SetValue("ImageDrawTriangleStrip", WithNote(i,
		"Draw a triangle strip defined by a list of at least 3 points within an image"));

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
	raylibModule.SetValue("ImageText", WithNote(i,
		"Create an image from text (default font)"));

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
	raylibModule.SetValue("ImageTextEx", WithNote(i,
		"Create an image from text (custom font)"));

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
		ValueDict sourceDict = nPatchDict.Lookup(CACHED_KEY("source"), Value::Null).GetDict();
		nPatchInfo.source.x = sourceDict.Lookup(CACHED_KEY("x"), Value::zero).FloatValue();
		nPatchInfo.source.y = sourceDict.Lookup(CACHED_KEY("y"), Value::zero).FloatValue();
		nPatchInfo.source.width = sourceDict.Lookup(CACHED_KEY("width"), Value::zero).FloatValue();
		nPatchInfo.source.height = sourceDict.Lookup(CACHED_KEY("height"), Value::zero).FloatValue();
		nPatchInfo.left = nPatchDict.Lookup(CACHED_KEY("left"), Value::zero).IntValue();
		nPatchInfo.top = nPatchDict.Lookup(CACHED_KEY("top"), Value::zero).IntValue();
		nPatchInfo.right = nPatchDict.Lookup(CACHED_KEY("right"), Value::zero).IntValue();
		nPatchInfo.bottom = nPatchDict.Lookup(CACHED_KEY("bottom"), Value::zero).IntValue();
		nPatchInfo.layout = nPatchDict.Lookup(CACHED_KEY("layout"), Value::zero).IntValue();
		Rectangle dest = ValueToRectangle(context.GetArg(2));
		Vector2 origin = ValueToVector2(context.GetArg(3));
		float rotation = context.GetArg(4).FloatValue();
		Color tint = ValueToColor(context.GetArg(5));
		DrawTextureNPatch(texture, nPatchInfo, dest, origin, rotation, tint);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("DrawTextureNPatch", WithNote(i,
		"Draw a texture (or part of it) that stretches or shrinks nicely using n-patch info: a "
		"map with source (rectangle), left, top, right, bottom, and layout"));

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
	raylibModule.SetValue("UpdateTexture", WithNote(i,
		"Update GPU texture with new pixel data from a RawData. NOTE: pixels must match the "
		"texture's format and contain at least as many pixels as the texture"));

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
	raylibModule.SetValue("UpdateTextureRec", WithNote(i,
		"Update GPU texture rectangle with new pixel data from a RawData. NOTE: pixels must match "
		"the texture's format and contain as many pixels as rec; rec must fit completely within "
		"the texture"));

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
	raylibModule.SetValue("LoadTextureCubemap", WithNote(i,
		"Load cubemap from image; layout is a CUBEMAP_LAYOUT_* value"));
}
