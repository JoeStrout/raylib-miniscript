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
#include "MiniscriptInterpreter.h"
#include "MiniscriptTypes.h"
#include <math.h>
#include <string.h>
#include <map>
#include "macros.h"

using namespace MiniScript;

// Helper methods, one per Raylib module (each defined in its own .cpp file)
void AddRAudioMethods(ValueDict raylibModule);
void AddRCoreMethods(ValueDict raylibModule);
void AddRShapesMethods(ValueDict raylibModule);
void AddRTextMethods(ValueDict raylibModule);
void AddRTexturesMethods(ValueDict raylibModule);

// And one more for all the constants
void AddConstants(ValueDict raylibModule);

// Add intrinsics to the interpreter
void AddRaylibIntrinsics() {
	Intrinsic *f;

	// Create accessors for the classes
	f = Intrinsic::Create("RawData");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(RawDataClass()); };

	f = Intrinsic::Create("Image");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(ImageClass()); };

	f = Intrinsic::Create("Texture");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(TextureClass()); };

	f = Intrinsic::Create("Font");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(FontClass()); };

	f = Intrinsic::Create("Wave");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(WaveClass()); };

	f = Intrinsic::Create("Music");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(MusicClass()); };

	f = Intrinsic::Create("Sound");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(SoundClass()); };

	f = Intrinsic::Create("AudioStream");
	f->code = INTRINSIC_LAMBDA { return IntrinsicResult(AudioStreamClass()); };

	// Create and register the main raylib module
	f = Intrinsic::Create("raylib");
	f->code = INTRINSIC_LAMBDA {
		static ValueDict raylibModule;

		if (raylibModule.Count() == 0) {
			AddRAudioMethods(raylibModule);
			AddRCoreMethods(raylibModule);
			AddRShapesMethods(raylibModule);
			AddRTextMethods(raylibModule);
			AddRTexturesMethods(raylibModule);
			AddConstants(raylibModule);
		}

		return IntrinsicResult(raylibModule);
	};
}
