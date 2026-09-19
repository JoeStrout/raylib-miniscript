//
//  RLgl.cpp
//  raylib-miniscript
//
//  rlgl module intrinsics (low-level OpenGL abstraction layer)
//

#include "RaylibIntrinsics.h"
#include "RaylibTypes.h"
#include "raylib.h"
#include "rlgl.h"
#include "miniscript.h"
#include "macros.h"

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
	i.AddParam("size", Value::one);
	i.set_Code(INTRINSIC_LAMBDA {
		float size = context.GetArg(0).FloatValue();
		rlSetPointSize(size);
		return IntrinsicResult::Null;
	});
	raylibModule.SetValue("rlSetPointSize", i.GetFunc());

	i = Intrinsic::Create("");
	i.set_Code(INTRINSIC_LAMBDA {
		return IntrinsicResult(Value(rlGetPointSize()));
	});
	raylibModule.SetValue("rlGetPointSize", i.GetFunc());

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
}
