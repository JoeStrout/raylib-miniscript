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

using namespace MiniScript;

// Defined in RCore.cpp
const void* PackUniformValue(Value value, int uniformType, int& count, std::vector<unsigned char>& storage);
String AdaptShaderCode(String code);

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

// Custom render batches are held in a map's resource handle (unloaded by
// rlUnloadRenderBatch, like the other raylib types).  We track the active one,
// since rlgl offers no way to ask, so that unloading it can first switch back
// to the default batch.  rlgl keeps a pointer into its resource box, so the
// active batch's handle is pinned against collection until it is replaced.
static rlRenderBatch* activeRenderBatch = nullptr;
static PinnedResource activeRenderBatchHandle;

static rlRenderBatch* ValueToRenderBatch(Value value) {
	return ResourcePtr<rlRenderBatch>(value);
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
	raylibModule.SetValue("rlSetBlendFactors", WithNote(i,
		"Set blending mode factor and equation"));

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
	raylibModule.SetValue("rlSetBlendFactorsSeparate", WithNote(i,
		"Set blending mode factor and equation separately for RGB and alpha"));

	// Matrix operations (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("mode");
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlMatrixMode(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlMatrixMode", WithNote(i,
		"Choose the current matrix to be transformed (RL_MODELVIEW or RL_PROJECTION)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlPushMatrix();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlPushMatrix", WithNote(i,
		"Push the current matrix onto the matrix stack"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlPopMatrix();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlPopMatrix", WithNote(i,
		"Pop the most recently pushed matrix from the matrix stack"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlLoadIdentity();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlLoadIdentity", WithNote(i,
		"Reset current matrix to identity matrix"));

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
	raylibModule.SetValue("rlTranslatef", WithNote(i,
		"Multiply the current matrix by a translation matrix"));

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
	raylibModule.SetValue("rlRotatef", WithNote(i,
		"Multiply the current matrix by a rotation of angle degrees about the axis (x, y, z)"));

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
	raylibModule.SetValue("rlScalef", WithNote(i,
		"Multiply the current matrix by a scaling matrix"));

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
	raylibModule.SetValue("rlMultMatrixf", WithNote(i,
		"Multiply the current matrix by another matrix, given as a list of 16 numbers in "
		"column-major (OpenGL) order; missing entries are 0"));

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
	raylibModule.SetValue("rlFrustum", WithNote(i,
		"Multiply the current matrix by a perspective matrix generated by parameters"));

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
	raylibModule.SetValue("rlOrtho", WithNote(i,
		"Multiply the current matrix by an orthographic matrix generated by parameters"));

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
	raylibModule.SetValue("rlViewport", WithNote(i,
		"Set the viewport area (transformation from normalized device coordinates to window "
		"coordinates)"));

	i = Intrinsic::Create("");
	i.AddParam("nearPlane");
	i.AddParam("farPlane");
	i.set_Code(INTRINSIC_LAMBDA {
		double nearPlane = context.GetArg(0).FloatValue();
		double farPlane = context.GetArg(1).FloatValue();
		rlSetClipPlanes(nearPlane, farPlane);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetClipPlanes", WithNote(i,
		"Set the near and far clip plane distances"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetCullDistanceNear()));
	});
	raylibModule.SetValue("rlGetCullDistanceNear", WithNote(i,
		"Get the near clip plane distance"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetCullDistanceFar()));
	});
	raylibModule.SetValue("rlGetCullDistanceFar", WithNote(i,
		"Get the far clip plane distance"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((double)rlGetActiveFramebuffer()));
	});
	raylibModule.SetValue("rlGetActiveFramebuffer", WithNote(i,
		"Get the id of the framebuffer (fbo) currently bound for drawing, such as a render "
		"texture's; 0 for the default framebuffer (the screen)"));

	// Render state toggles (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableBackfaceCulling();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableBackfaceCulling", WithNote(i,
		"Enable backface culling"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableBackfaceCulling();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableBackfaceCulling", WithNote(i,
		"Disable backface culling"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableDepthTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableDepthTest", WithNote(i,
		"Enable depth testing"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableDepthTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableDepthTest", WithNote(i,
		"Disable depth testing"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableDepthMask();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableDepthMask", WithNote(i,
		"Enable writing to the depth buffer"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableDepthMask();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableDepthMask", WithNote(i,
		"Disable writing to the depth buffer"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableWireMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableWireMode", WithNote(i,
		"Enable wire mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableWireMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableWireMode", WithNote(i,
		"Disable wire mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableSmoothLines();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableSmoothLines", WithNote(i,
		"Enable line antialiasing"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableSmoothLines();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableSmoothLines", WithNote(i,
		"Disable line antialiasing"));

	i = Intrinsic::Create("");
	i.AddParam("width", Value(1));
	i.set_Code(INTRINSIC_LAMBDA {
		float width = context.GetArg(0).FloatValue();
		rlSetLineWidth(width);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetLineWidth", WithNote(i,
		"Set the width of drawn lines"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetLineWidth()));
	});
	raylibModule.SetValue("rlGetLineWidth", WithNote(i,
		"Get the line drawing width"));

	// Render batch (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDrawRenderBatchActive();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawRenderBatchActive", WithNote(i,
		"Draw (flush) the active render batch now"));

	// Get/set matrices (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixModelview()));
	});
	raylibModule.SetValue("rlGetMatrixModelview", WithNote(i,
		"Get internal modelview matrix"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixProjection()));
	});
	raylibModule.SetValue("rlGetMatrixProjection", WithNote(i,
		"Get internal projection matrix"));

	i = Intrinsic::Create("");
	i.AddParam("proj");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix proj = ValueToMatrix(context.GetArg(0));
		rlSetMatrixProjection(proj);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixProjection", WithNote(i,
		"Set a custom projection matrix (replaces internal projection matrix)"));

	i = Intrinsic::Create("");
	i.AddParam("view");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix view = ValueToMatrix(context.GetArg(0));
		rlSetMatrixModelview(view);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixModelview", WithNote(i,
		"Set a custom modelview matrix (replaces internal modelview matrix)"));


	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(MatrixToValue(rlGetMatrixTransform()));
	});
	raylibModule.SetValue("rlGetMatrixTransform", WithNote(i,
		"Get internal accumulated transform matrix"));

	i = Intrinsic::Create("");
	i.AddParam("eye", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int eye = context.GetArg(0).IntValue();
		return IntrinsicResult(MatrixToValue(rlGetMatrixProjectionStereo(eye)));
	});
	raylibModule.SetValue("rlGetMatrixProjectionStereo", WithNote(i,
		"Get internal projection matrix for stereo render (selected eye)"));

	i = Intrinsic::Create("");
	i.AddParam("eye", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int eye = context.GetArg(0).IntValue();
		return IntrinsicResult(MatrixToValue(rlGetMatrixViewOffsetStereo(eye)));
	});
	raylibModule.SetValue("rlGetMatrixViewOffsetStereo", WithNote(i,
		"Get internal view offset matrix for stereo render (selected eye)"));

	i = Intrinsic::Create("");
	i.AddParam("right");
	i.AddParam("left");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix right = ValueToMatrix(context.GetArg(0));
		Matrix left = ValueToMatrix(context.GetArg(1));
		rlSetMatrixProjectionStereo(right, left);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixProjectionStereo", WithNote(i,
		"Set eyes projection matrices for stereo rendering"));

	i = Intrinsic::Create("");
	i.AddParam("right");
	i.AddParam("left");
	i.set_Code(INTRINSIC_LAMBDA {
		Matrix right = ValueToMatrix(context.GetArg(0));
		Matrix left = ValueToMatrix(context.GetArg(1));
		rlSetMatrixViewOffsetStereo(right, left);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetMatrixViewOffsetStereo", WithNote(i,
		"Set eyes view offsets matrices for stereo rendering"));

	// Vertex level operations (rlgl immediate mode)

	i = Intrinsic::Create("");
	i.AddParam("mode", Value(RL_TRIANGLES));
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlBegin(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlBegin", WithNote(i,
		"Begin defining vertices, organized as mode (RL_LINES, RL_TRIANGLES or RL_QUADS); finish "
		"with rlEnd"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnd();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnd", WithNote(i,
		"Finish defining vertices begun with rlBegin"));

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int x = context.GetArg(0).IntValue();
		int y = context.GetArg(1).IntValue();
		rlVertex2i(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlVertex2i", WithNote(i,
		"Add one vertex at (x, y), using the current color, texture coordinate and normal"));

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		rlVertex2f(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlVertex2f", WithNote(i,
		"Add one vertex at (x, y), using the current color, texture coordinate and normal"));

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
	raylibModule.SetValue("rlVertex3f", WithNote(i,
		"Add one vertex at (x, y, z), using the current color, texture coordinate and normal"));

	i = Intrinsic::Create("");
	i.AddParam("x", Value::zero);
	i.AddParam("y", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		float x = context.GetArg(0).FloatValue();
		float y = context.GetArg(1).FloatValue();
		rlTexCoord2f(x, y);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlTexCoord2f", WithNote(i,
		"Set the texture coordinate for the following vertices"));

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
	raylibModule.SetValue("rlNormal3f", WithNote(i,
		"Set the normal for the following vertices"));

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
	raylibModule.SetValue("rlColor4ub", WithNote(i,
		"Set the color (components 0-255) for the following vertices"));

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
	raylibModule.SetValue("rlColor3f", WithNote(i,
		"Set the color (r, g, b as x, y, z, each 0-1; alpha 1) for the following vertices"));

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
	raylibModule.SetValue("rlColor4f", WithNote(i,
		"Set the color (r, g, b, a as x, y, z, w, each 0-1) for the following vertices"));

	i = Intrinsic::Create("");
	i.AddParam("id", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlSetTexture(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetTexture", WithNote(i,
		"Set the texture id used by the following vertices (0 for none)"));

	i = Intrinsic::Create("");
	i.AddParam("vCount");
	i.set_Code(INTRINSIC_LAMBDA {
		int vCount = context.GetArg(0).IntValue();
		return IntrinsicResult(rlCheckRenderBatchLimit(vCount));
	});
	raylibModule.SetValue("rlCheckRenderBatchLimit", WithNote(i,
		"Check whether adding vCount vertices would overflow the active render batch, and if so "
		"draw (flush) it; returns true if it was flushed"));

	// More render state (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableColorBlend();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableColorBlend", WithNote(i,
		"Enable color blending"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableColorBlend();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableColorBlend", WithNote(i,
		"Disable color blending"));

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
	raylibModule.SetValue("rlColorMask", WithNote(i,
		"Choose which color channels drawing writes to (true to write r, g, b and a)"));

	i = Intrinsic::Create("");
	i.AddParam("mode", Value(RL_CULL_FACE_BACK));
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlSetCullFace(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetCullFace", WithNote(i,
		"Set which faces are culled (RL_CULL_FACE_BACK or RL_CULL_FACE_FRONT)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableScissorTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableScissorTest", WithNote(i,
		"Enable the scissor test"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableScissorTest();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableScissorTest", WithNote(i,
		"Disable the scissor test"));

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
	raylibModule.SetValue("rlScissor", WithNote(i,
		"Set the scissor rectangle, in framebuffer pixels from the bottom left (used while the "
		"scissor test is enabled)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnablePointMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnablePointMode", WithNote(i,
		"Enable point mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisablePointMode();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisablePointMode", WithNote(i,
		"Disable point mode"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlEnableStereoRender();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableStereoRender", WithNote(i,
		"Enable stereo rendering"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableStereoRender();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableStereoRender", WithNote(i,
		"Disable stereo rendering"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(rlIsStereoRenderEnabled());
	});
	raylibModule.SetValue("rlIsStereoRenderEnabled", WithNote(i,
		"Check if stereo render is enabled"));

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
	raylibModule.SetValue("rlClearColor", WithNote(i,
		"Set the color (components 0-255) used to clear the screen"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlClearScreenBuffers();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlClearScreenBuffers", WithNote(i,
		"Clear used screen buffers (color and depth)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlCheckErrors();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlCheckErrors", WithNote(i,
		"Check and log OpenGL error codes"));

	i = Intrinsic::Create("");
	i.AddParam("mode", Value(RL_BLEND_ALPHA));
	i.set_Code(INTRINSIC_LAMBDA {
		int mode = context.GetArg(0).IntValue();
		rlSetBlendMode(mode);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetBlendMode", WithNote(i,
		"Set the blend mode (e.g. RL_BLEND_ALPHA)"));

	// Quick and dirty cube/quad buffers load->draw->unload

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlLoadDrawCube();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlLoadDrawCube", WithNote(i,
		"Load and draw a cube in normalized device coordinates"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlLoadDrawQuad();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlLoadDrawQuad", WithNote(i,
		"Load and draw a quad in normalized device coordinates"));

	// rlgl queries

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetVersion()));
	});
	raylibModule.SetValue("rlGetVersion", WithNote(i,
		"Get the current OpenGL version, as an RL_OPENGL_ constant (e.g. RL_OPENGL_33)"));

	i = Intrinsic::Create("");
	i.AddParam("width");
	i.set_Code(INTRINSIC_LAMBDA {
		int width = context.GetArg(0).IntValue();
		rlSetFramebufferWidth(width);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetFramebufferWidth", WithNote(i,
		"Set current framebuffer width"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetFramebufferWidth()));
	});
	raylibModule.SetValue("rlGetFramebufferWidth", WithNote(i,
		"Get default framebuffer width"));

	i = Intrinsic::Create("");
	i.AddParam("height");
	i.set_Code(INTRINSIC_LAMBDA {
		int height = context.GetArg(0).IntValue();
		rlSetFramebufferHeight(height);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetFramebufferHeight", WithNote(i,
		"Set current framebuffer height"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetFramebufferHeight()));
	});
	raylibModule.SetValue("rlGetFramebufferHeight", WithNote(i,
		"Get default framebuffer height"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlGetTextureIdDefault()));
	});
	raylibModule.SetValue("rlGetTextureIdDefault", WithNote(i,
		"Get the id of rlgl's default texture (1x1 white)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlGetShaderIdDefault()));
	});
	raylibModule.SetValue("rlGetShaderIdDefault", WithNote(i,
		"Get the id of rlgl's default shader"));

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
	raylibModule.SetValue("rlGetShaderLocsDefault", WithNote(i,
		"Get a list of the default shader's locations (a copy; changing it has no effect)"));

	i = Intrinsic::Create("");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int format = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(Value(rlGetPixelFormatName(format)));
	});
	raylibModule.SetValue("rlGetPixelFormatName", WithNote(i,
		"Get the name of a PIXELFORMAT_ constant (e.g. \"R8G8B8A8\")"));

	// Texture state (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("slot", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int slot = context.GetArg(0).IntValue();
		rlActiveTextureSlot(slot);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlActiveTextureSlot", WithNote(i,
		"Select and activate a texture slot"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableTexture(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableTexture", WithNote(i,
		"Enable texture"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableTexture();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableTexture", WithNote(i,
		"Disable texture"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableTextureCubemap(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableTextureCubemap", WithNote(i,
		"Enable texture cubemap"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableTextureCubemap();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableTextureCubemap", WithNote(i,
		"Disable texture cubemap"));

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
	raylibModule.SetValue("rlTextureParameters", WithNote(i,
		"Set texture parameters (wrap mode/filter mode)"));

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
	raylibModule.SetValue("rlCubemapParameters", WithNote(i,
		"Set cubemap parameters (wrap mode/filter mode)"));

	// Shader and framebuffer state (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableShader(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableShader", WithNote(i,
		"Enable shader program"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableShader();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableShader", WithNote(i,
		"Disable shader program"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableFramebuffer(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableFramebuffer", WithNote(i,
		"Enable rendering to texture (fbo)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableFramebuffer();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableFramebuffer", WithNote(i,
		"Disable rendering to texture"));

	i = Intrinsic::Create("");
	i.AddParam("target");
	i.AddParam("framebuffer");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int target = (unsigned int)context.GetArg(0).IntValue();
		unsigned int framebuffer = (unsigned int)context.GetArg(1).IntValue();
		rlBindFramebuffer(target, framebuffer);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlBindFramebuffer", WithNote(i,
		"Bind framebuffer object (fbo)"));

	// Copies the read framebuffer to the draw framebuffer (see rlBindFramebuffer
	// with RL_READ_FRAMEBUFFER / RL_DRAW_FRAMEBUFFER).  bufferMask is a GL mask:
	// 0x4000 = color (the default), 0x0100 = depth, 0x0400 = stencil.
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
	raylibModule.SetValue("rlBlitFramebuffer", WithNote(i,
		"Copy a rectangle of the read framebuffer to the draw framebuffer (see rlBindFramebuffer); "
		"bufferMask: 0x4000 color (default), 0x0100 depth, 0x0400 stencil"));

	// Activates `count` color attachments (up to 8) for multiple render
	// targets.
	i = Intrinsic::Create("");
	i.AddParam("count", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int count = context.GetArg(0).IntValue();
		rlActiveDrawBuffers(count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlActiveDrawBuffers", WithNote(i,
		"Activate count color attachments (up to 8) as draw buffers, for multiple render targets; "
		"one is active by default"));

	// Custom render batches (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("numBuffers", Value(RL_DEFAULT_BATCH_BUFFERS));
#ifdef PLATFORM_WEB
	i.AddParam("bufferElements", Value(2048));	// rlgl's default for ES2/ES3
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
		map.SetValue(CACHED_KEY("_handle"), NewResourceHandle(batch));
		map.SetValue(CACHED_KEY("bufferCount"), Value(batch.bufferCount));
		return IntrinsicResult(DynamicMap(map));
	});
	raylibModule.SetValue("rlLoadRenderBatch", WithNote(i,
		"Create a custom render batch with numBuffers buffers of bufferElements quads each; returns "
		"a batch map, or null on failure.  Unload with rlUnloadRenderBatch"));

	i = Intrinsic::Create("");
	i.AddParam("batch");
	i.set_Code(INTRINSIC_LAMBDA {
		Value batchValue = context.GetArg(0);
		rlRenderBatch* batch = ValueToRenderBatch(batchValue);
		if (batch == nullptr) return IntrinsicResult::Null;
		if (batch == activeRenderBatch) {
			rlSetRenderBatchActive(nullptr);
			activeRenderBatch = nullptr;
			activeRenderBatchHandle.Clear();
		}
		rlRenderBatch unloaded;
		TakeResource(batchValue, &unloaded);
		rlUnloadRenderBatch(unloaded);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadRenderBatch", WithNote(i,
		"Unload a render batch made by rlLoadRenderBatch (switching back to the default batch if it "
		"was active)"));

	i = Intrinsic::Create("");
	i.AddParam("batch");
	i.set_Code(INTRINSIC_LAMBDA {
		rlRenderBatch* batch = ValueToRenderBatch(context.GetArg(0));
		if (batch == nullptr) return IntrinsicResult::Null;
		rlDrawRenderBatch(batch);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawRenderBatch", WithNote(i,
		"Draw a render batch; this resets it and moves on to its next buffer (if it has several)"));

	// batch = null goes back to rlgl's default batch
	i = Intrinsic::Create("");
	i.AddParam("batch");
	i.set_Code(INTRINSIC_LAMBDA {
		rlRenderBatch* batch = ValueToRenderBatch(context.GetArg(0));
		rlSetRenderBatchActive(batch);
		activeRenderBatch = batch;
		if (batch) activeRenderBatchHandle.Set(context.GetArg(0));
		else activeRenderBatchHandle.Clear();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetRenderBatchActive", WithNote(i,
		"Set the render batch rlgl draws into (null for the default batch)"));

	// Vertex buffer state (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("vaoId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vaoId = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(rlEnableVertexArray(vaoId));
	});
	raylibModule.SetValue("rlEnableVertexArray", WithNote(i,
		"Enable (bind) a vertex array object (VAO); returns true if VAOs are supported"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableVertexArray();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexArray", WithNote(i,
		"Disable vertex array object (VAO)"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableVertexBuffer(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableVertexBuffer", WithNote(i,
		"Enable vertex buffer (VBO)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableVertexBuffer();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexBuffer", WithNote(i,
		"Disable vertex buffer (VBO)"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlEnableVertexBufferElement(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableVertexBufferElement", WithNote(i,
		"Enable vertex buffer element (VBO element)"));

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		rlDisableVertexBufferElement();
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexBufferElement", WithNote(i,
		"Disable vertex buffer element (VBO element)"));

	i = Intrinsic::Create("");
	i.AddParam("index");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		rlEnableVertexAttribute(index);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlEnableVertexAttribute", WithNote(i,
		"Enable vertex attribute index"));

	i = Intrinsic::Create("");
	i.AddParam("index");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		rlDisableVertexAttribute(index);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDisableVertexAttribute", WithNote(i,
		"Disable vertex attribute index"));

	// Vertex buffer management (rlgl).  Buffer data is given as RawData.

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlLoadVertexArray()));
	});
	raylibModule.SetValue("rlLoadVertexArray", WithNote(i,
		"Create a vertex array object (VAO), if supported; returns its id, or 0"));

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
	raylibModule.SetValue("rlLoadVertexBuffer", WithNote(i,
		"Create a vertex buffer object (VBO) from a RawData and return its id.  buffer may be null to "
		"allocate size bytes without filling them; size <= 0 uses the whole RawData.  dynamic hints "
		"that the data will be updated often"));

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
	raylibModule.SetValue("rlLoadVertexBufferElement", WithNote(i,
		"Create an element (index) buffer from a RawData and return its id.  buffer may be null to "
		"allocate size bytes without filling them; size <= 0 uses the whole RawData.  dynamic hints "
		"that the data will be updated often"));

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
	raylibModule.SetValue("rlUpdateVertexBuffer", WithNote(i,
		"Update a vertex buffer with data from a RawData, starting offset bytes into the buffer; "
		"dataSize (in bytes) <= 0 uses the whole RawData"));

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
	raylibModule.SetValue("rlUpdateVertexBufferElements", WithNote(i,
		"Update an element (index) buffer with data from a RawData, starting offset bytes into the "
		"buffer; dataSize (in bytes) <= 0 uses the whole RawData"));

	i = Intrinsic::Create("");
	i.AddParam("vaoId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vaoId = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadVertexArray(vaoId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadVertexArray", WithNote(i,
		"Unload vertex array object (VAO)"));

	i = Intrinsic::Create("");
	i.AddParam("vboId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vboId = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadVertexBuffer(vboId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadVertexBuffer", WithNote(i,
		"Unload vertex buffer (VBO)"));

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
	raylibModule.SetValue("rlSetVertexAttribute", WithNote(i,
		"Describe vertex attribute index of the bound vertex buffer: compSize components of type "
		"(e.g. RL_FLOAT), whether normalized, and stride and offset in bytes"));

	i = Intrinsic::Create("");
	i.AddParam("index");
	i.AddParam("divisor");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int index = (unsigned int)context.GetArg(0).IntValue();
		int divisor = context.GetArg(1).IntValue();
		rlSetVertexAttributeDivisor(index, divisor);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetVertexAttributeDivisor", WithNote(i,
		"Set vertex attribute divisor (for instanced drawing)"));

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
	raylibModule.SetValue("rlSetVertexAttributeDefault", WithNote(i,
		"Set the default value of a vertex attribute (used when no buffer feeds it); value is a "
		"number, vector map, list of numbers, or RawData of floats; count <= 0 uses the number of "
		"components in attribType"));

	// Draws with RL_TRIANGLES; offset and count are in vertices
	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		if (offset < 0 || count < 0) return RaiseError(context, "rlDrawVertexArray: offset and count must be >= 0");
		rlDrawVertexArray(offset, count);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArray", WithNote(i,
		"Draw count vertices of the bound vertex array as triangles, starting at vertex offset"));

	// Draws indexed RL_TRIANGLES from the bound element buffer, whose indices
	// must be 16-bit (ushort).  offset is in indices; buffer is a byte offset
	// into the element buffer, and must be 0: raylib takes it as a pointer, and
	// script-supplied numbers must never become addresses.
	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.AddParam("buffer", Value::zero);
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		if (context.GetArg(2).IntValue() != 0) return RaiseError(context, "rlDrawVertexArrayElements: buffer must be 0");
		if (offset < 0 || count < 0) return RaiseError(context, "rlDrawVertexArrayElements: offset and count must be >= 0");
		rlDrawVertexArrayElements(offset, count, nullptr);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArrayElements", WithNote(i,
		"Draw count indices of the bound element buffer (16-bit indices) as triangles, starting at "
		"index offset; buffer must be 0"));

	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.AddParam("instances", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		int instances = context.GetArg(2).IntValue();
		if (offset < 0 || count < 0 || instances < 0) return RaiseError(context, "rlDrawVertexArrayInstanced: offset, count and instances must be >= 0");
		rlDrawVertexArrayInstanced(offset, count, instances);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArrayInstanced", WithNote(i,
		"Draw count vertices of the bound vertex array as triangles, starting at vertex offset, "
		"instances times"));

	i = Intrinsic::Create("");
	i.AddParam("offset", Value::zero);
	i.AddParam("count");
	i.AddParam("buffer", Value::zero);
	i.AddParam("instances", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		int offset = context.GetArg(0).IntValue();
		int count = context.GetArg(1).IntValue();
		int instances = context.GetArg(3).IntValue();
		if (context.GetArg(2).IntValue() != 0) return RaiseError(context, "rlDrawVertexArrayElementsInstanced: buffer must be 0");
		if (offset < 0 || count < 0 || instances < 0) return RaiseError(context, "rlDrawVertexArrayElementsInstanced: offset, count and instances must be >= 0");
		rlDrawVertexArrayElementsInstanced(offset, count, nullptr, instances);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlDrawVertexArrayElementsInstanced", WithNote(i,
		"Draw count indices of the bound element buffer (16-bit indices) as triangles, starting at "
		"index offset, instances times; buffer must be 0"));

	// Texture management (rlgl).  Pixel data is given and returned as RawData.

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
	raylibModule.SetValue("rlLoadTexture", WithNote(i,
		"Create a width x height texture in the given pixel format from data (a RawData holding each "
		"mipmap level in turn, or null for an empty texture); returns its id, or 0 on failure"));

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
	raylibModule.SetValue("rlLoadTextureDepth", WithNote(i,
		"Create a depth texture (or, if useRenderBuffer, a depth renderbuffer) to attach to a "
		"framebuffer; returns its id"));

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
	raylibModule.SetValue("rlLoadTextureCubemap", WithNote(i,
		"Create a cubemap texture with size x size faces; data is a RawData holding, for each mipmap "
		"level, all 6 faces (+X, -X, +Y, -Y, +Z, -Z), or null for an empty cubemap.  Returns its id, "
		"or 0 on failure"));

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
	raylibModule.SetValue("rlUpdateTexture", WithNote(i,
		"Update a width x height region of a loaded texture, at offsetX, offsetY, with pixel data "
		"from a RawData; format should match the texture's own format (this can't be checked)"));

	// Returns a map with glInternalFormat, glFormat and glType
	i = Intrinsic::Create("");
	i.AddParam("format");
	i.set_Code(INTRINSIC_LAMBDA {
		int format = context.GetArg(0).IntValue();
		unsigned int glInternalFormat = 0, glFormat = 0, glType = 0;
		rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
		ValueDict map;
		map.SetValue(CACHED_KEY("glInternalFormat"), Value((int)glInternalFormat));
		map.SetValue(CACHED_KEY("glFormat"), Value((int)glFormat));
		map.SetValue(CACHED_KEY("glType"), Value((int)glType));
		return IntrinsicResult(DynamicMap(map));
	});
	raylibModule.SetValue("rlGetGlTextureFormats", WithNote(i,
		"Get the OpenGL formats and data type for a raylib pixel format, as a map with "
		"glInternalFormat, glFormat and glType"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadTexture(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadTexture", WithNote(i,
		"Unload texture from GPU memory"));

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
	raylibModule.SetValue("rlGenTextureMipmaps", WithNote(i,
		"Generate mipmaps for a texture on the GPU; returns the resulting number of mipmap levels"));

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
	raylibModule.SetValue("rlReadTexturePixels", WithNote(i,
		"Read a texture's pixel data as a RawData; null on failure or if width or height < 1"));

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
	raylibModule.SetValue("rlReadScreenPixels", WithNote(i,
		"Read a width x height region from the bottom left of the current framebuffer (normally the "
		"whole screen) as a RawData of RGBA bytes, top row first; null if width or height < 1"));

	// Framebuffer management (rlgl)

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value((int)rlLoadFramebuffer()));
	});
	raylibModule.SetValue("rlLoadFramebuffer", WithNote(i,
		"Create an empty framebuffer object; returns its id"));

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
	raylibModule.SetValue("rlFramebufferAttach", WithNote(i,
		"Attach a texture or renderbuffer to a framebuffer object.  attachType: an RL_ATTACHMENT_ "
		"constant (COLOR_CHANNEL0-7, DEPTH or STENCIL); texType: RL_ATTACHMENT_TEXTURE2D, "
		"RL_ATTACHMENT_RENDERBUFFER, or an RL_ATTACHMENT_CUBEMAP_ side"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		return IntrinsicResult(rlFramebufferComplete(id));
	});
	raylibModule.SetValue("rlFramebufferComplete", WithNote(i,
		"Check whether a framebuffer is complete (ready to draw to)"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadFramebuffer(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadFramebuffer", WithNote(i,
		"Unload a framebuffer from GPU memory; its depth texture or renderbuffer (if any) is also "
		"deleted, but color attachments are not"));

	// Shader management (rlgl)

	i = Intrinsic::Create("");
	i.AddParam("code");
	i.AddParam("type", Value(RL_FRAGMENT_SHADER));
	i.set_Code(INTRINSIC_LAMBDA {
		String code = AdaptShaderCode(context.GetArg(0).ToString());
		int type = context.GetArg(1).IntValue();
		return IntrinsicResult(Value((int)rlLoadShader(code.c_str(), type)));
	});
	raylibModule.SetValue("rlLoadShader", WithNote(i,
		"Compile one shader stage from source (type: RL_VERTEX_SHADER, RL_FRAGMENT_SHADER or "
		"RL_COMPUTE_SHADER); returns its id.  On web, a desktop GLSL #version line is adapted for "
		"WebGL"));

	i = Intrinsic::Create("");
	i.AddParam("vsCode", String());
	i.AddParam("fsCode", String());
	i.set_Code(INTRINSIC_LAMBDA {
		String vsCode = AdaptShaderCode(context.GetArg(0).ToString());
		String fsCode = AdaptShaderCode(context.GetArg(1).ToString());
		const char* vsPtr = vsCode.LengthB() > 0 ? vsCode.c_str() : nullptr;
		const char* fsPtr = fsCode.LengthB() > 0 ? fsCode.c_str() : nullptr;
		return IntrinsicResult(Value((int)rlLoadShaderProgram(vsPtr, fsPtr)));
	});
	raylibModule.SetValue("rlLoadShaderProgram", WithNote(i,
		"Compile and link a shader program from vertex and fragment shader source; either may be empty "
		"(or null) to use rlgl's default shader for that stage.  Returns the program id, or 0 on "
		"failure.  On web, a desktop GLSL #version line is adapted for WebGL"));

	i = Intrinsic::Create("");
	i.AddParam("vsId");
	i.AddParam("fsId");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int vsId = (unsigned int)context.GetArg(0).IntValue();
		unsigned int fsId = (unsigned int)context.GetArg(1).IntValue();
		return IntrinsicResult(Value((int)rlLoadShaderProgramEx(vsId, fsId)));
	});
	raylibModule.SetValue("rlLoadShaderProgramEx", WithNote(i,
		"Link a shader program from already-compiled vertex and fragment shader ids; returns its id"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadShader(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadShader", WithNote(i,
		"Delete a compiled shader stage (from rlLoadShader)"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		rlUnloadShaderProgram(id);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlUnloadShaderProgram", WithNote(i,
		"Unload shader program"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("uniformName");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		String uniformName = context.GetArg(1).ToString();
		return IntrinsicResult(Value(rlGetLocationUniform(id, uniformName.c_str())));
	});
	raylibModule.SetValue("rlGetLocationUniform", WithNote(i,
		"Get the location of a uniform in shader program id; -1 if not found"));

	i = Intrinsic::Create("");
	i.AddParam("id");
	i.AddParam("attribName");
	i.set_Code(INTRINSIC_LAMBDA {
		unsigned int id = (unsigned int)context.GetArg(0).IntValue();
		String attribName = context.GetArg(1).ToString();
		return IntrinsicResult(Value(rlGetLocationAttrib(id, attribName.c_str())));
	});
	raylibModule.SetValue("rlGetLocationAttrib", WithNote(i,
		"Get the location of a vertex attribute in shader program id; -1 if not found"));

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
	raylibModule.SetValue("rlSetUniform", WithNote(i,
		"Set a uniform of the shader enabled with rlEnableShader; value may be a number, vector map, "
		"list (flat or of vectors), or RawData; count <= 0 infers it from value"));

	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("mat");
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		Matrix mat = ValueToMatrix(context.GetArg(1));
		rlSetUniformMatrix(locIndex, mat);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniformMatrix", WithNote(i,
		"Set a matrix uniform of the shader enabled with rlEnableShader"));

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
			// rlgl uploads these transposed on GL 3.3 but not on ES (WebGL
			// can't transpose), so pre-transpose here for the same result.
			m = MatrixTranspose(m);
#endif
			mats.push_back(m);
		}
		if (mats.empty()) return IntrinsicResult::Null;
		rlSetUniformMatrices(locIndex, mats.data(), (int)mats.size());
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniformMatrices", WithNote(i,
		"Set a matrix-array uniform of the shader enabled with rlEnableShader from a list of "
		"matrices"));

	i = Intrinsic::Create("");
	i.AddParam("locIndex");
	i.AddParam("textureId");
	i.set_Code(INTRINSIC_LAMBDA {
		int locIndex = context.GetArg(0).IntValue();
		unsigned int textureId = (unsigned int)context.GetArg(1).IntValue();
		rlSetUniformSampler(locIndex, textureId);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetUniformSampler", WithNote(i,
		"Set a sampler uniform of the shader enabled with rlEnableShader to a texture id"));

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
	raylibModule.SetValue("rlSetShader", WithNote(i,
		"Set the shader the render batch draws with: program id, and locs as a list of locations "
		"(missing ones are -1) or null for the default shader's"));
}
