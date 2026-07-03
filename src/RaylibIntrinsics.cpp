//
//  RaylibIntrinsics.cpp
//  raylib-miniscript
//
//  Raylib intrinsics for MiniScript
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
#include "miniscript.h"
#include <math.h>
#include <string.h>
#include <map>
#include "macros.h"

using namespace MiniScript;

// Helper methods, one per Raylib module (each defined in its own .cpp file)
void AddRAudioMethods(ValueDict raylibModule);
void AddRCoreMethods(ValueDict raylibModule);
void AddRModelsMethods(ValueDict raylibModule);
void AddRMathMethods(ValueDict raylibModule);
void AddRShapesMethods(ValueDict raylibModule);
void AddRTextMethods(ValueDict raylibModule);
void AddRTexturesMethods(ValueDict raylibModule);

// And one more for all the constants
void AddConstants(ValueDict raylibModule);

// Add intrinsics to the interpreter
void AddRaylibIntrinsics() {
	Intrinsic f;

	// Create accessors for the classes
	f = Intrinsic::Create("RawData");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(RawDataClass())); });

	f = Intrinsic::Create("Image");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(ImageClass())); });

	f = Intrinsic::Create("Texture");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(TextureClass())); });

	f = Intrinsic::Create("Font");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(FontClass())); });

	f = Intrinsic::Create("Wave");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(WaveClass())); });

	f = Intrinsic::Create("Music");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(MusicClass())); });

	f = Intrinsic::Create("Sound");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(SoundClass())); });

	f = Intrinsic::Create("AudioStream");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(AudioStreamClass())); });

	f = Intrinsic::Create("Shader");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(ShaderClass())); });

	f = Intrinsic::Create("Mesh");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(MeshClass())); });

	f = Intrinsic::Create("Material");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(MaterialClass())); });

	f = Intrinsic::Create("Model");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(ModelClass())); });

	f = Intrinsic::Create("ModelAnimation");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(ModelAnimationClass())); });

	f = Intrinsic::Create("Camera3D");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(StaticMap(Camera3DClass())); });

	// Create and register the main raylib module
	f = Intrinsic::Create("raylib");
	f.set_Code(INTRINSIC_LAMBDA {
		static ValueDict raylibModule;

		if (raylibModule.Count() == 0) {
			AddRAudioMethods(raylibModule);
			AddRCoreMethods(raylibModule);
			AddRModelsMethods(raylibModule);
			AddRMathMethods(raylibModule);
			AddRShapesMethods(raylibModule);
			AddRTextMethods(raylibModule);
			AddRTexturesMethods(raylibModule);
			AddConstants(raylibModule);
		}

		return IntrinsicResult(StaticMap(raylibModule));
	});
}
