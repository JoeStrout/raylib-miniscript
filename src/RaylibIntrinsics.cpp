//
//  RaylibIntrinsics.cpp
//  raylib-miniscript
//
//  Raylib intrinsics for MiniScript
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "Matrix.h"
#include "raylib.h"
#include "miniscript.h"
#include <math.h>
#include <string.h>
#include <map>
#include "macros.h"

using namespace MiniScript;

// Helper methods, one per Raylib module (each defined in its own .cpp file)
void AddRAudioMethods(ValueDict& raylibModule);
void AddRCoreMethods(ValueDict& raylibModule);
void AddRModelsMethods(ValueDict& raylibModule);
void AddRMathMethods(ValueDict& raylibModule);
void AddRShapesMethods(ValueDict& raylibModule);
void AddRTextMethods(ValueDict& raylibModule);
void AddRTexturesMethods(ValueDict& raylibModule);

// And one more for all the constants
void AddConstants(ValueDict& raylibModule);

// Add intrinsics to the interpreter
void AddRaylibIntrinsics() {
	Intrinsic f;

	// Create accessors for the classes
	f = Intrinsic::Create("RawData");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(RawDataClass()); });

	f = Intrinsic::Create("Matrix");
	f.set_Code(INTRINSIC_LAMBDA { return IntrinsicResult(MatrixClass()); });

	// Create and register the main raylib module.  Built on first use, then
	// wrapped and GC-rooted once; every later reference to the global `raylib`
	// is a plain read of that Value.  (A host ValueDict is not reachable by the
	// GC on its own, so the wrap has to happen here, where the dictionary is
	// filled -- and keeping the Value means this accessor, which runs on every
	// `raylib.Foo` that did not cache the module, costs nothing.)
	f = Intrinsic::Create("raylib");
	f.set_Code(INTRINSIC_LAMBDA {
		static ValueDict raylibModule;
		static Value raylibModuleValue;

		if (raylibModuleValue.IsNull()) {
			AddRAudioMethods(raylibModule);
			AddRCoreMethods(raylibModule);
			AddRModelsMethods(raylibModule);
			AddRMathMethods(raylibModule);
			AddRShapesMethods(raylibModule);
			AddRTextMethods(raylibModule);
			AddRTexturesMethods(raylibModule);
			AddConstants(raylibModule);
			AddTypeClasses(raylibModule);
			raylibModuleValue = GCManager::NewMapFromDict(raylibModule);
			GCManager::AddRoot(raylibModuleValue);
		}

		return IntrinsicResult(raylibModuleValue);
	});
}
