//
//  RLgl.cpp
//  raylib-miniscript
//
//  rlgl module intrinsics (low-level OpenGL abstraction layer)
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "RawData.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "miniscript.h"
#include "macros.h"
#include <vector>

// rlgl's own rlGetActiveFramebuffer is compiled only for GL 3.3, ES 3 and the
// software renderer (see the guard in rlgl.h); on the ES2/WebGL1 web build it
// is a stub that always returns 0.  ES2 can answer the same question perfectly
// well -- it just spells the enum GL_FRAMEBUFFER_BINDING rather than
// GL_DRAW_FRAMEBUFFER_BINDING (both are 0x8CA6) -- so supply it ourselves
// there.  Script sees one function that works on every backend.
#ifdef PLATFORM_WEB
#include <GLES2/gl2.h>
static unsigned int GetActiveFramebuffer() {
	GLint fboId = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboId);
	return (unsigned int)fboId;
}
#else
static unsigned int GetActiveFramebuffer() {
	return rlGetActiveFramebuffer();
}
#endif

using namespace MiniScript;

// Defined in RCore.cpp
const void* PackUniformValue(Value value, int uniformType, int& count, std::vector<unsigned char>& storage);

static IntrinsicResult RaiseError(Context context, const char* msg) {
	context.vm.RaiseRuntimeError(msg);
	return IntrinsicResult::Null;
}

// The buffer of a RawData argument, and its length; null (length 0) for
// anything else.
static unsigned char* RawDataBuffer(Value value, int* outLength) {
	*outLength = 0;
	if (value.Type() != ValueType::Map) return nullptr;
	BinaryData* data = ValueToRawData(value);
	if (data == nullptr || data->bytes == nullptr) return nullptr;
	*outLength = data->length;
	return data->bytes;
}

// Bytes rlLoadTexture reads for a texture with the given mipmap chain
static int TextureDataSize(int width, int height, int format, int mipmapCount) {
	int total = 0;
	for (int m = 0; m < mipmapCount; m++) {
		total += GetPixelDataSize(width, height, format);
		width /= 2;
		height /= 2;
		if (width < 1) width = 1;
		if (height < 1) height = 1;
	}
	return total;
}

// Bytes rlLoadTextureCubemap reads: 6 faces per mipmap level
static int CubemapDataSize(int size, int format, int mipmapCount) {
	int total = 0;
	for (int m = 0; m < mipmapCount; m++) {
		total += 6 * GetPixelDataSize(size, size, format);
		size /= 2;
		if (size < 1) size = 1;
	}
	return total;
}

// Custom render batches are heap-allocated and held in a map as a raw pointer
// (freed by rlUnloadRenderBatch, like the other raylib types).  We track the
// active one, since rlgl offers no way to ask, so that unloading it can first
// switch back to the default batch.
static rlRenderBatch* activeRenderBatch = nullptr;

static rlRenderBatch* ValueToRenderBatch(Value value) {
	if (value.Type() != ValueType::Map) return nullptr;
	ValueDict map = value.GetDict();
	return (rlRenderBatch*)ValueToPointer(map.Lookup(String("_handle"), Value::zero));
}

// rlSetShader keeps the locs pointer it is given, so the locations must live
// somewhere stable; this is that somewhere.
static int shaderLocs[RL_MAX_SHADER_LOCATIONS];

void AddRLglMethods(ValueDict& raylibModule) {
	Intrinsic i;

	// Blend factors (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("glSrcFactor");
	i.AddParam("glDstFactor");
	i.AddParam("glEquation");
	i.set_Code(INTRINSIC_LAMBDA {
		int glSrcFactor = context.GetArg(0).IntValue();
		int glDstFactor = context.GetArg(1).IntValue();
		int glEquation = context.GetArg(2).IntValue();
		rlSetBlendFactors(glSrcFactor, glDstFactor, glEquation);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetBlendFactors", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("glSrcRGB");
	i.AddParam("glDstRGB");
	i.AddParam("glSrcAlpha");
	i.AddParam("glDstAlpha");
	i.AddParam("glEqRGB");
	i.AddParam("glEqAlpha");
	i.set_Code(INTRINSIC_LAMBDA {
		int glSrcRGB = context.GetArg(0).IntValue();
		int glDstRGB = context.GetArg(1).IntValue();
		int glSrcAlpha = context.GetArg(2).IntValue();
		int glDstAlpha = context.GetArg(3).IntValue();
		int glEqRGB = context.GetArg(4).IntValue();
		int glEqAlpha = context.GetArg(5).IntValue();
		rlSetBlendFactorsSeparate(glSrcRGB, glDstRGB, glSrcAlpha, glDstAlpha, glEqRGB, glEqAlpha);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetBlendFactorsSeparate", i.GetFunc());

	// Matrix operations (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("mode");
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlMatrixMode(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlMatrixMode", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlPushMatrix();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlPushMatrix", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlPopMatrix();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlPopMatrix", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlLoadIdentity();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlLoadIdentity", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("z", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		rlTranslatef(x, y, z);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlTranslatef", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("angle", Value::zero);
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("z", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float angle = context.GetArg(0).FloatValue();
		float x = context.GetArg(1).FloatValue();
		float y = context.GetArg(2).FloatValue();
		float z = context.GetArg(3).FloatValue();
		rlRotatef(angle, x, y, z);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlRotatef", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value(1));
	i.AddParam("y", Value(1));
	i.AddParam("z", Value(1));
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		rlScalef(x, y, z);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlScalef", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("matf");
	i.set_Code(INTRINSIC_LAMBDA {
		Value listVal = context.GetArg(0);
		ValueList list = listVal.GetList();
		float matf[16];
		for (int j = 0; j < 16; j++) {
			matf[j] = (j < list.Count()) ? list[j].FloatValue() : 0.0f;
		}
		rlMultMatrixf(matf);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlMultMatrixf", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("left");
	i.AddParam("right");
	i.AddParam("bottom");
	i.AddParam("top");
	i.AddParam("znear");
	i.AddParam("zfar");
	i.set_Code(INTRINSIC_LAMBDA {
		double left = context.GetArg(0).FloatValue();
		double right = context.GetArg(1).FloatValue();
		double bottom = context.GetArg(2).FloatValue();
		double top = context.GetArg(3).FloatValue();
		double znear = context.GetArg(4).FloatValue();
		double zfar = context.GetArg(5).FloatValue();
		rlFrustum(left, right, bottom, top, znear, zfar);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlFrustum", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("left");
	i.AddParam("right");
	i.AddParam("bottom");
	i.AddParam("top");
	i.AddParam("znear");
	i.AddParam("zfar");
	i.set_Code(INTRINSIC_LAMBDA {
		double left = context.GetArg(0).FloatValue();
		double right = context.GetArg(1).FloatValue();
		double bottom = context.GetArg(2).FloatValue();
		double top = context.GetArg(3).FloatValue();
		double znear = context.GetArg(4).FloatValue();
		double zfar = context.GetArg(5).FloatValue();
		rlOrtho(left, right, bottom, top, znear, zfar);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlOrtho", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x");
	i.AddParam("y");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		rlViewport(x, y, width, height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlViewport", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("nearPlane");
	i.AddParam("farPlane");
	i.set_Code(INTRINSIC_LAMBDA {
		double nearPlane = context.GetArg(0).FloatValue();
		double farPlane = context.GetArg(1).FloatValue();
		rlSetClipPlanes(nearPlane, farPlane);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetClipPlanes", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetCullDistanceNear()));
	});
	raylibModule.SetValue("rlGetCullDistanceNear", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetCullDistanceFar()));
	});
	raylibModule.SetValue("rlGetCullDistanceFar", i.GetFunc());

	// Get the currently active render texture (fbo); 0 for the default framebuffer
	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((double)GetActiveFramebuffer()));
	});
	raylibModule.SetValue("rlGetActiveFramebuffer", i.GetFunc());

	// Render state toggles (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableBackfaceCulling();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableBackfaceCulling", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableBackfaceCulling();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableBackfaceCulling", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableDepthTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableDepthTest", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableDepthTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableDepthTest", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableDepthMask();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableDepthMask", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableDepthMask();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableDepthMask", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableWireMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableWireMode", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableWireMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableWireMode", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableSmoothLines();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableSmoothLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableSmoothLines();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableSmoothLines", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width", Value(1));
	i.set_Code(INTRINSIC_LAMBDA {
		float width = context.GetArg(0).FloatValue();
		rlSetLineWidth(width);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetLineWidth", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetLineWidth()));
	});
	raylibModule.SetValue("rlGetLineWidth", i.GetFunc());

	// Render batch (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDrawRenderBatchActive();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawRenderBatchActive", i.GetFunc());

	// Get/set matrices (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixModelview()));
	});
	raylibModule.SetValue("rlGetMatrixModelview", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixProjection()));
	});
	raylibModule.SetValue("rlGetMatrixProjection", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("proj");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix proj = ValueToMatrix(context.GetArg(0));
		rlSetMatrixProjection(proj);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixProjection", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("view");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix view = ValueToMatrix(context.GetArg(0));
		rlSetMatrixModelview(view);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixModelview", i.GetFunc());


	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixTransform()));
	});
	raylibModule.SetValue("rlGetMatrixTransform", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("eye", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int eye = context.GetArg(0).IntValue();
		return IntrinsicResult(MatrixToValue(rlGetMatrixProjectionStereo(eye)));
	});
	raylibModule.SetValue("rlGetMatrixProjectionStereo", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("eye", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int eye = context.GetArg(0).IntValue();
		return IntrinsicResult(MatrixToValue(rlGetMatrixViewOffsetStereo(eye)));
	});
	raylibModule.SetValue("rlGetMatrixViewOffsetStereo", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("right");
	i.AddParam("left");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix right = ValueToMatrix(context.GetArg(0));
		Matrix left = ValueToMatrix(context.GetArg(1));
		rlSetMatrixProjectionStereo(right, left);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixProjectionStereo", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("right");
	i.AddParam("left");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix right = ValueToMatrix(context.GetArg(0));
		Matrix left = ValueToMatrix(context.GetArg(1));
		rlSetMatrixViewOffsetStereo(right, left);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixViewOffsetStereo", i.GetFunc());

	// Vertex level operations (rlgl immediate mode)

	i = Intrinsic::Create("");
	i.AddParam("mode", Value(RL_TRIANGLES));
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlBegin(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlBegin", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnd();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnd", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		rlVertex2i(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlVertex2i", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		rlVertex2f(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlVertex2f", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("z", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		rlVertex3f(x, y, z);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlVertex3f", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		rlTexCoord2f(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlTexCoord2f", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.AddParam("z", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		rlNormal3f(x, y, z);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlNormal3f", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("r", Value(255));
	i.AddParam("g", Value(255));
	i.AddParam("b", Value(255));
	i.AddParam("a", Value(255));
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned char r = (unsigned char)context.GetArg(0).IntValue();
		unsigned char g = (unsigned char)context.GetArg(1).IntValue();
		unsigned char b = (unsigned char)context.GetArg(2).IntValue();
		unsigned char a = (unsigned char)context.GetArg(3).IntValue();
		rlColor4ub(r, g, b, a);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlColor4ub", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::one);
	i.AddParam("y", Value::one);
	i.AddParam("z", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		rlColor3f(x, y, z);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlColor3f", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x", Value::one);
	i.AddParam("y", Value::one);
	i.AddParam("z", Value::one);
	i.AddParam("w", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		float z = context.GetArg(2).FloatValue();
		float w = context.GetArg(3).FloatValue();
		rlColor4f(x, y, z, w);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlColor4f", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlSetTexture(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("vCount");
	i.set_Code(INTRINSIC_LAMBDA {
		int vCount = context.GetArg(0).IntValue();
		return IntrinsicResult(rlCheckRenderBatchLimit(vCount));
	});
	raylibModule.SetValue("rlCheckRenderBatchLimit", i.GetFunc());

	// More render state (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableColorBlend();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableColorBlend", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableColorBlend();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableColorBlend", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("r", Value::one);
	i.AddParam("g", Value::one);
	i.AddParam("b", Value::one);
	i.AddParam("a", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		bool r = context.GetArg(0).IntValue() != 0;
		bool g = context.GetArg(1).IntValue() != 0;
		bool b = context.GetArg(2).IntValue() != 0;
		bool a = context.GetArg(3).IntValue() != 0;
		rlColorMask(r, g, b, a);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlColorMask", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mode", Value(RL_CULL_FACE_BACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlSetCullFace(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetCullFace", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableScissorTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableScissorTest", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableScissorTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableScissorTest", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("x");
	i.AddParam("y");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		int width = context.GetArg(2).IntValue();
		int height = context.GetArg(3).IntValue();
		rlScissor(x, y, width, height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlScissor", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnablePointMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnablePointMode", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisablePointMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisablePointMode", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableStereoRender();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableStereoRender", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableStereoRender();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableStereoRender", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(rlIsStereoRenderEnabled());
	});
	raylibModule.SetValue("rlIsStereoRenderEnabled", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("r", Value::zero);
	i.AddParam("g", Value::zero);
	i.AddParam("b", Value::zero);
	i.AddParam("a", Value(255));
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned char r = (unsigned char)context.GetArg(0).IntValue();
		unsigned char g = (unsigned char)context.GetArg(1).IntValue();
		unsigned char b = (unsigned char)context.GetArg(2).IntValue();
		unsigned char a = (unsigned char)context.GetArg(3).IntValue();
		rlClearColor(r, g, b, a);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlClearColor", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlClearScreenBuffers();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlClearScreenBuffers", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlCheckErrors();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlCheckErrors", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("mode", Value(RL_BLEND_ALPHA));
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlSetBlendMode(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetBlendMode", i.GetFunc());

	// Quick and dirty cube/quad buffers load->draw->unload

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlLoadDrawCube();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlLoadDrawCube", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlLoadDrawQuad();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlLoadDrawQuad", i.GetFunc());

	// rlgl queries

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetVersion()));
	});
	raylibModule.SetValue("rlGetVersion", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		rlSetFramebufferWidth(width);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetFramebufferWidth", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetFramebufferWidth()));
	});
	raylibModule.SetValue("rlGetFramebufferWidth", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
		int height = context.GetArg(0).IntValue();
		rlSetFramebufferHeight(height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetFramebufferHeight", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetFramebufferHeight()));
	});
	raylibModule.SetValue("rlGetFramebufferHeight", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlGetTextureIdDefault()));
	});
	raylibModule.SetValue("rlGetTextureIdDefault", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlGetShaderIdDefault()));
	});
	raylibModule.SetValue("rlGetShaderIdDefault", i.GetFunc());

	// Returns a list of RL_MAX_SHADER_LOCATIONS ints (a copy; changing it
	// does not affect the default shader)
	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		int* locs = rlGetShaderLocsDefault();
		ValueList result;
		if (locs) {
			for (int j = 0; j < RL_MAX_SHADER_LOCATIONS; j++) result.Add(Value(locs[j]));
		}
		return IntrinsicResult(DynamicList(result));
	});
	raylibModule.SetValue("rlGetShaderLocsDefault", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int format = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(Value(rlGetPixelFormatName(format)));
	});
	raylibModule.SetValue("rlGetPixelFormatName", i.GetFunc());

	// Texture state (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("slot", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int slot = context.GetArg(0).IntValue();
		rlActiveTextureSlot(slot);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlActiveTextureSlot", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableTexture(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableTexture();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableTextureCubemap(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableTextureCubemap", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableTextureCubemap();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableTextureCubemap", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("param");
	i.AddParam("value");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		int param = context.GetArg(1).IntValue();
		int value = context.GetArg(2).IntValue();
		rlTextureParameters(id, param, value);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlTextureParameters", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("param");
	i.AddParam("value");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		int param = context.GetArg(1).IntValue();
		int value = context.GetArg(2).IntValue();
		rlCubemapParameters(id, param, value);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlCubemapParameters", i.GetFunc());

	// Shader and framebuffer state (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableShader(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableShader", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableShader();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableShader", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableFramebuffer(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableFramebuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableFramebuffer();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableFramebuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.AddParam("framebuffer");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int target = (unsigned int)context.GetArg(0).IntValue();
		unsigned int framebuffer = (unsigned int)context.GetArg(1).IntValue();
		rlBindFramebuffer(target, framebuffer);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlBindFramebuffer", i.GetFunc());

	// Copies the read framebuffer to the draw framebuffer (see rlBindFramebuffer
	// with RL_READ_FRAMEBUFFER / RL_DRAW_FRAMEBUFFER).  bufferMask is a GL mask:
	// 0x4000 = color (the default), 0x0100 = depth, 0x0400 = stencil.
	// Desktop only; does nothing on web (WebGL1).
	i = Intrinsic::Create("");
	i.AddParam("srcX");
	i.AddParam("srcY");
	i.AddParam("srcWidth");
	i.AddParam("srcHeight");
	i.AddParam("dstX");
	i.AddParam("dstY");
	i.AddParam("dstWidth");
	i.AddParam("dstHeight");
	i.AddParam("bufferMask", Value(0x4000));
	i.set_Code(INTRINSIC_LAMBDA {
		int srcX = context.GetArg(0).IntValue();
		int srcY = context.GetArg(1).IntValue();
		int srcWidth = context.GetArg(2).IntValue();
		int srcHeight = context.GetArg(3).IntValue();
		int dstX = context.GetArg(4).IntValue();
		int dstY = context.GetArg(5).IntValue();
		int dstWidth = context.GetArg(6).IntValue();
		int dstHeight = context.GetArg(7).IntValue();
		int bufferMask = context.GetArg(8).IntValue();
		rlBlitFramebuffer(srcX, srcY, srcWidth, srcHeight, dstX, dstY, dstWidth, dstHeight, bufferMask);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlBlitFramebuffer", i.GetFunc());

	// Activates `count` color attachments (up to 8) for multiple render
	// targets.  Desktop only; does nothing on web (WebGL1).
	i = Intrinsic::Create("");
	i.AddParam("count", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int count = context.GetArg(0).IntValue();
		rlActiveDrawBuffers(count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlActiveDrawBuffers", i.GetFunc());

	// Custom render batches (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("numBuffers", Value(RL_DEFAULT_BATCH_BUFFERS));
#ifdef PLATFORM_WEB
	i.AddParam("bufferElements", Value(2048));	// rlgl's default for ES2
#else
	i.AddParam("bufferElements", Value(RL_DEFAULT_BATCH_BUFFER_ELEMENTS));
#endif
	i.set_Code(INTRINSIC_LAMBDA {
		int numBuffers = context.GetArg(0).IntValue();
		int bufferElements = context.GetArg(1).IntValue();
		if (numBuffers < 1 || bufferElements < 1) return IntrinsicResult::Null;
		rlRenderBatch batch = rlLoadRenderBatch(numBuffers, bufferElements);
		if (batch.vertexBuffer == nullptr) return IntrinsicResult::Null;
		ValueDict map;
		map.SetValue(String("_handle"), PointerToValue(new rlRenderBatch(batch)));
		map.SetValue(String("bufferCount"), Value(batch.bufferCount));
		return IntrinsicResult(DynamicMap(map));
	});
	raylibModule.SetValue("rlLoadRenderBatch", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("batch");
	i.set_Code(INTRINSIC_LAMBDA {
		Value batchValue = context.GetArg(0);
		rlRenderBatch* batch = ValueToRenderBatch(batchValue);
		if (batch == nullptr) return IntrinsicResult::Null;
		if (batch == activeRenderBatch) {
			rlSetRenderBatchActive(nullptr);
			activeRenderBatch = nullptr;
		}
		rlUnloadRenderBatch(*batch);
		delete batch;
		batchValue.GetDict().SetValue(String("_handle"), Value::zero);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadRenderBatch", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("batch");
	i.set_Code(INTRINSIC_LAMBDA {
		rlRenderBatch* batch = ValueToRenderBatch(context.GetArg(0));
		if (batch == nullptr) return IntrinsicResult::Null;
		rlDrawRenderBatch(batch);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawRenderBatch", i.GetFunc());

	// batch = null goes back to rlgl's default batch
	i = Intrinsic::Create("");
	i.AddParam("batch");
	i.set_Code(INTRINSIC_LAMBDA {
		rlRenderBatch* batch = ValueToRenderBatch(context.GetArg(0));
		rlSetRenderBatchActive(batch);
		activeRenderBatch = batch;
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetRenderBatchActive", i.GetFunc());

	// Vertex buffer state (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("vaoId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vaoId = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(rlEnableVertexArray(vaoId));
	});
	raylibModule.SetValue("rlEnableVertexArray", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableVertexArray();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexArray", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableVertexBuffer(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableVertexBuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableVertexBuffer();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexBuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableVertexBufferElement(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableVertexBufferElement", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableVertexBufferElement();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexBufferElement", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("index");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		rlEnableVertexAttribute(index);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableVertexAttribute", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("index");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		rlDisableVertexAttribute(index);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexAttribute", i.GetFunc());

	// Vertex buffer management (rlgl).  Buffer data is given as RawData.

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlLoadVertexArray()));
	});
	raylibModule.SetValue("rlLoadVertexArray", i.GetFunc());

	// buffer may be null to allocate `size` bytes without filling them;
	// size <= 0 means the whole RawData.
	i = Intrinsic::Create("");
	i.AddParam("buffer");
	i.AddParam("size", Value::zero);
	i.AddParam("dynamic", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(0), &length);
		int size = context.GetArg(1).IntValue();
		bool dynamic = context.GetArg(2).IntValue() != 0;
		if (size <= 0) size = length;
		if (bytes != nullptr && size > length) return RaiseError(context, "rlLoadVertexBuffer: size exceeds buffer length");
		return IntrinsicResult(Value((int)rlLoadVertexBuffer(bytes, size, dynamic)));
	});
	raylibModule.SetValue("rlLoadVertexBuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("buffer");
	i.AddParam("size", Value::zero);
	i.AddParam("dynamic", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(0), &length);
		int size = context.GetArg(1).IntValue();
		bool dynamic = context.GetArg(2).IntValue() != 0;
		if (size <= 0) size = length;
		if (bytes != nullptr && size > length) return RaiseError(context, "rlLoadVertexBufferElement: size exceeds buffer length");
		return IntrinsicResult(Value((int)rlLoadVertexBufferElement(bytes, size, dynamic)));
	});
	raylibModule.SetValue("rlLoadVertexBufferElement", i.GetFunc());

	// dataSize <= 0 means the whole RawData; offset is in bytes
	i = Intrinsic::Create("");
	i.AddParam("bufferId");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.AddParam("offset", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int bufferId = (unsigned int)context.GetArg(0).IntValue();
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(1), &length);
		int dataSize = context.GetArg(2).IntValue();
		int offset = context.GetArg(3).IntValue();
		if (bytes == nullptr) return RaiseError(context, "rlUpdateVertexBuffer: data must be a RawData");
		if (dataSize <= 0) dataSize = length;
		if (dataSize > length) return RaiseError(context, "rlUpdateVertexBuffer: dataSize exceeds data length");
		rlUpdateVertexBuffer(bufferId, bytes, dataSize, offset);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUpdateVertexBuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("data");
	i.AddParam("dataSize", Value::zero);
	i.AddParam("offset", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(1), &length);
		int dataSize = context.GetArg(2).IntValue();
		int offset = context.GetArg(3).IntValue();
		if (bytes == nullptr) return RaiseError(context, "rlUpdateVertexBufferElements: data must be a RawData");
		if (dataSize <= 0) dataSize = length;
		if (dataSize > length) return RaiseError(context, "rlUpdateVertexBufferElements: dataSize exceeds data length");
		rlUpdateVertexBufferElements(id, bytes, dataSize, offset);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUpdateVertexBufferElements", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("vaoId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vaoId = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadVertexArray(vaoId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadVertexArray", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("vboId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vboId = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadVertexBuffer(vboId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadVertexBuffer", i.GetFunc());

	// stride and offset are in bytes
	i = Intrinsic::Create("");
	i.AddParam("index");
	i.AddParam("compSize");
	i.AddParam("type", Value(RL_FLOAT));
	i.AddParam("normalized", Value::zero);
	i.AddParam("stride", Value::zero);
	i.AddParam("offset", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		int compSize = context.GetArg(1).IntValue();
		int type = context.GetArg(2).IntValue();
		bool normalized = context.GetArg(3).IntValue() != 0;
		int stride = context.GetArg(4).IntValue();
		int offset = context.GetArg(5).IntValue();
		rlSetVertexAttribute(index, compSize, type, normalized, stride, offset);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetVertexAttribute", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("index");
	i.AddParam("divisor");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		int divisor = context.GetArg(1).IntValue();
		rlSetVertexAttributeDivisor(index, divisor);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetVertexAttributeDivisor", i.GetFunc());

	// value: a number, vector map, list of numbers, or RawData of floats.
	// count <= 0 means the number of components in attribType.
	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("value");
	i.AddParam("attribType", Value(RL_SHADER_ATTRIB_FLOAT));
	i.AddParam("count", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		int attribType = context.GetArg(2).IntValue();
		int count = context.GetArg(3).IntValue();
		if (attribType < RL_SHADER_ATTRIB_FLOAT || attribType > RL_SHADER_ATTRIB_VEC4) {
			return RaiseError(context, "rlSetVertexAttributeDefault: unknown attribType");
		}
		int components = attribType - RL_SHADER_ATTRIB_FLOAT + 1;
		if (count <= 0) count = components;
		// Pack as a float uniform of the same width (VEC2..VEC4 line up)
		int uniformType = RL_SHADER_UNIFORM_FLOAT + (components - 1);
		int uniformCount = 1;
		std::vector<unsigned char> storage;
		const void* data = PackUniformValue(context.GetArg(1), uniformType, uniformCount, storage);
		int length;
		if (RawDataBuffer(context.GetArg(1), &length) != nullptr && length < components * (int)sizeof(float)) {
			return RaiseError(context, "rlSetVertexAttributeDefault: RawData too small");
		}
		rlSetVertexAttributeDefault(locIndex, data, attribType, count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetVertexAttributeDefault", i.GetFunc());

	// Draws with RL_TRIANGLES; offset and count are in vertices
	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		rlDrawVertexArray(offset, count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArray", i.GetFunc());

	// Draws indexed RL_TRIANGLES from the bound element buffer, whose indices
	// must be 16-bit (ushort).  offset is in indices; buffer is a byte offset
	// into the element buffer (usually 0).
	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.AddParam("buffer", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		intptr_t buffer = (intptr_t)context.GetArg(2).IntValue();
		rlDrawVertexArrayElements(offset, count, (const void*)buffer);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArrayElements", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.AddParam("instances", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		int instances = context.GetArg(2).IntValue();
		rlDrawVertexArrayInstanced(offset, count, instances);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArrayInstanced", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.AddParam("buffer", Value::zero);
	i.AddParam("instances", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		intptr_t buffer = (intptr_t)context.GetArg(2).IntValue();
		int instances = context.GetArg(3).IntValue();
		rlDrawVertexArrayElementsInstanced(offset, count, (const void*)buffer, instances);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArrayElementsInstanced", i.GetFunc());

	// Texture management (rlgl).  Pixel data is given and returned as RawData.

	// data may be null for an empty texture
	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("format", Value(RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));
	i.AddParam("mipmapCount", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(0), &length);
		int width = context.GetArg(1).IntValue();
		int height = context.GetArg(2).IntValue();
		int format = context.GetArg(3).IntValue();
		int mipmapCount = context.GetArg(4).IntValue();
		if (width < 1 || height < 1 || mipmapCount < 1) return IntrinsicResult(Value::zero);
		if (bytes != nullptr && length < TextureDataSize(width, height, format, mipmapCount)) {
			return RaiseError(context, "rlLoadTexture: data too small for the given size, format and mipmaps");
		}
		return IntrinsicResult(Value((int)rlLoadTexture(bytes, width, height, format, mipmapCount)));
	});
	raylibModule.SetValue("rlLoadTexture", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("useRenderBuffer", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		bool useRenderBuffer = context.GetArg(2).IntValue() != 0;
		return IntrinsicResult(Value((int)rlLoadTextureDepth(width, height, useRenderBuffer)));
	});
	raylibModule.SetValue("rlLoadTextureDepth", i.GetFunc());

	// data holds all 6 faces (+X, -X, +Y, -Y, +Z, -Z) for each mipmap level,
	// or null for an empty cubemap
	i = Intrinsic::Create("");
	i.AddParam("data");
	i.AddParam("size");
	i.AddParam("format", Value(RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));
	i.AddParam("mipmapCount", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(0), &length);
		int size = context.GetArg(1).IntValue();
		int format = context.GetArg(2).IntValue();
		int mipmapCount = context.GetArg(3).IntValue();
		if (size < 1 || mipmapCount < 1) return IntrinsicResult(Value::zero);
		if (bytes != nullptr && length < CubemapDataSize(size, format, mipmapCount)) {
			return RaiseError(context, "rlLoadTextureCubemap: data too small for the given size, format and mipmaps");
		}
		return IntrinsicResult(Value((int)rlLoadTextureCubemap(bytes, size, format, mipmapCount)));
	});
	raylibModule.SetValue("rlLoadTextureCubemap", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("offsetX");
	i.AddParam("offsetY");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("format");
	i.AddParam("data");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		int offsetX = context.GetArg(1).IntValue();
		int offsetY = context.GetArg(2).IntValue();
		int width = context.GetArg(3).IntValue();
		int height = context.GetArg(4).IntValue();
		int format = context.GetArg(5).IntValue();
		int length;
		unsigned char* bytes = RawDataBuffer(context.GetArg(6), &length);
		if (bytes == nullptr) return RaiseError(context, "rlUpdateTexture: data must be a RawData");
		if (width < 1 || height < 1) return IntrinsicResult::Null;
		if (length < GetPixelDataSize(width, height, format)) {
			return RaiseError(context, "rlUpdateTexture: data too small for the given size and format");
		}
		rlUpdateTexture(id, offsetX, offsetY, width, height, format, bytes);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUpdateTexture", i.GetFunc());

	// Returns a map with glInternalFormat, glFormat and glType
	i = Intrinsic::Create("");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		int format = context.GetArg(0).IntValue();
		unsigned int glInternalFormat = 0, glFormat = 0, glType = 0;
		rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
		ValueDict map;
		map.SetValue(String("glInternalFormat"), Value((int)glInternalFormat));
		map.SetValue(String("glFormat"), Value((int)glFormat));
		map.SetValue(String("glType"), Value((int)glType));
		return IntrinsicResult(DynamicMap(map));
	});
	raylibModule.SetValue("rlGetGlTextureFormats", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadTexture(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadTexture", i.GetFunc());

	// Returns the resulting number of mipmap levels
	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("format", Value(RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		int width = context.GetArg(1).IntValue();
		int height = context.GetArg(2).IntValue();
		int format = context.GetArg(3).IntValue();
		int mipmaps = 1;
		rlGenTextureMipmaps(id, width, height, format, &mipmaps);
		return IntrinsicResult(Value(mipmaps));
	});
	raylibModule.SetValue("rlGenTextureMipmaps", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("width");
	i.AddParam("height");
	i.AddParam("format", Value(RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		int width = context.GetArg(1).IntValue();
		int height = context.GetArg(2).IntValue();
		int format = context.GetArg(3).IntValue();
		if (width < 1 || height < 1) return IntrinsicResult::Null;
		void* pixels = rlReadTexturePixels(id, width, height, format);
		if (pixels == nullptr) return IntrinsicResult::Null;
		int size = GetPixelDataSize(width, height, format);
		return IntrinsicResult(RawDataToValue(new BinaryData((unsigned char*)pixels, size, true)));
	});
	raylibModule.SetValue("rlReadTexturePixels", i.GetFunc());

	// Returns RGBA (R8G8B8A8) pixels, top row first
	i = Intrinsic::Create("");
	i.AddParam("width");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		int height = context.GetArg(1).IntValue();
		if (width < 1 || height < 1) return IntrinsicResult::Null;
		unsigned char* pixels = rlReadScreenPixels(width, height);
		if (pixels == nullptr) return IntrinsicResult::Null;
		return IntrinsicResult(RawDataToValue(new BinaryData(pixels, width * height * 4, true)));
	});
	raylibModule.SetValue("rlReadScreenPixels", i.GetFunc());

	// Framebuffer management (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlLoadFramebuffer()));
	});
	raylibModule.SetValue("rlLoadFramebuffer", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("fboId");
	i.AddParam("texId");
	i.AddParam("attachType", Value(RL_ATTACHMENT_COLOR_CHANNEL0));
	i.AddParam("texType", Value(RL_ATTACHMENT_TEXTURE2D));
	i.AddParam("mipLevel", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int fboId = (unsigned int)context.GetArg(0).IntValue();
		unsigned int texId = (unsigned int)context.GetArg(1).IntValue();
		int attachType = context.GetArg(2).IntValue();
		int texType = context.GetArg(3).IntValue();
		int mipLevel = context.GetArg(4).IntValue();
		rlFramebufferAttach(fboId, texId, attachType, texType, mipLevel);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlFramebufferAttach", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(rlFramebufferComplete(id));
	});
	raylibModule.SetValue("rlFramebufferComplete", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadFramebuffer(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadFramebuffer", i.GetFunc());

	// Shader management (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("code");
	i.AddParam("type", Value(RL_FRAGMENT_SHADER));
	i.set_Code(INTRINSIC_LAMBDA {
		String code = context.GetArg(0).ToString();
		int type = context.GetArg(1).IntValue();
		return IntrinsicResult(Value((int)rlLoadShader(code.c_str(), type)));
	});
	raylibModule.SetValue("rlLoadShader", i.GetFunc());

	// Either code may be empty (or null) to use rlgl's default shader for that stage
	i = Intrinsic::Create("");
	i.AddParam("vsCode", String());
	i.AddParam("fsCode", String());
	i.set_Code(INTRINSIC_LAMBDA {
		String vsCode = context.GetArg(0).ToString();
		String fsCode = context.GetArg(1).ToString();
		const char* vsPtr = vsCode.LengthB() > 0 ? vsCode.c_str() : nullptr;
		const char* fsPtr = fsCode.LengthB() > 0 ? fsCode.c_str() : nullptr;
		return IntrinsicResult(Value((int)rlLoadShaderProgram(vsPtr, fsPtr)));
	});
	raylibModule.SetValue("rlLoadShaderProgram", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("vsId");
	i.AddParam("fsId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vsId = (unsigned int)context.GetArg(0).IntValue();
		unsigned int fsId = (unsigned int)context.GetArg(1).IntValue();
		return IntrinsicResult(Value((int)rlLoadShaderProgramEx(vsId, fsId)));
	});
	raylibModule.SetValue("rlLoadShaderProgramEx", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadShader(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadShader", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadShaderProgram(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadShaderProgram", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("uniformName");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		String uniformName = context.GetArg(1).ToString();
		return IntrinsicResult(Value(rlGetLocationUniform(id, uniformName.c_str())));
	});
	raylibModule.SetValue("rlGetLocationUniform", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("attribName");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		String attribName = context.GetArg(1).ToString();
		return IntrinsicResult(Value(rlGetLocationAttrib(id, attribName.c_str())));
	});
	raylibModule.SetValue("rlGetLocationAttrib", i.GetFunc());

	// Sets a uniform of the shader enabled with rlEnableShader.  value may be a
	// number, vector map, list (flat or of vectors), or RawData; count <= 0
	// infers it from the value.
	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("value");
	i.AddParam("uniformType", Value(RL_SHADER_UNIFORM_FLOAT));
	i.AddParam("count", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		Value value = context.GetArg(1);
		int uniformType = context.GetArg(2).IntValue();
		int count = context.GetArg(3).IntValue();
		std::vector<unsigned char> storage;
		const void* data = PackUniformValue(value, uniformType, count, storage);
		if (data == nullptr) return RaiseError(context, "rlSetUniform: unknown uniformType");
		int length;
		if (RawDataBuffer(value, &length) != nullptr) {
			int components = (uniformType == RL_SHADER_UNIFORM_SAMPLER2D) ? 1 : (uniformType % 4) + 1;
			if (length < count * components * 4) return RaiseError(context, "rlSetUniform: RawData too small for count");
		}
		rlSetUniform(locIndex, data, uniformType, count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniform", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("mat");
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		Matrix mat = ValueToMatrix(context.GetArg(1));
		rlSetUniformMatrix(locIndex, mat);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniformMatrix", i.GetFunc());

	// mats is a list of matrices
	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("mats");
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		Value matsVal = context.GetArg(1);
		if (matsVal.Type() != ValueType::List) return RaiseError(context, "rlSetUniformMatrices: mats must be a list of matrices");
		ValueList list = matsVal.GetList();
		std::vector<Matrix> mats;
		for (int j = 0; j < list.Count(); j++) {
			Matrix m = ValueToMatrix(list[j]);
#ifdef PLATFORM_WEB
			// rlgl uploads these transposed on GL 3.3 but not on ES2 (WebGL
			// can't transpose), so pre-transpose here for the same result.
			m = MatrixTranspose(m);
#endif
			mats.push_back(m);
		}
		if (mats.empty()) return IntrinsicResult::Null;
		rlSetUniformMatrices(locIndex, mats.data(), (int)mats.size());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniformMatrices", i.GetFunc());

	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("textureId");
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		unsigned int textureId = (unsigned int)context.GetArg(1).IntValue();
		rlSetUniformSampler(locIndex, textureId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniformSampler", i.GetFunc());

	// Sets the shader the render batch draws with.  locs is a list of up to
	// RL_MAX_SHADER_LOCATIONS locations (missing ones are -1), or null for the
	// default shader's.
	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("locs");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		Value locsVal = context.GetArg(1);
		int* locs = rlGetShaderLocsDefault();
		if (locsVal.Type() == ValueType::List) {
			ValueList list = locsVal.GetList();
			for (int j = 0; j < RL_MAX_SHADER_LOCATIONS; j++) {
				shaderLocs[j] = (j < list.Count()) ? list[j].IntValue() : -1;
			}
			locs = shaderLocs;
		}
		rlSetShader(id, locs);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetShader", i.GetFunc());
}
