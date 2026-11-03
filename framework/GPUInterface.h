#ifndef __GPUINTERFACE_H__
#define __GPUINTERFACE_H__

// port: the GPU display path (draft 3 with its review's edits; GPUInterface is built and tested, GPU2D and GPUDisplay
// are not written yet). It stands where PopCap's framework has its
// D3DInterface: a Direct3D 9 device as the games drive it, carried out through OpenGL ES 3.0 / WebGL2 (natively the
// driver's ES context, which rundesk's private desktop gives too, else GL 3.3 core). Only games that opt in use it
// (GPUDisplay, below); every other game keeps DDInterface and its SDL_Renderer exactly as they are, and
// Is3DAccelerated keeps its meaning for them.
//
// Three layers:
// - GPUInterface (here): the D3D9 device. One state machine for 2D and 3D: render states, texture stages, samplers,
//   transforms, lights, a material, vertex and pixel shaders bound separately, primitives, render targets, Clear,
//   StretchRect, read-back. Nothing of any one game, and no framework 2D logic.
// - GPU2D (GPU2D.h, drafted after this one): the framework's D3D layer above the device, which is generic framework code
//   in the original, not game code: the 2D draws, the draw-mode layer (slots 0x114-0x13c), the filter cache, the private
//   clip scissor, the texture pieces, the dynamic vertex buffer that batches 2D quads. See "GPU2D" at the end.
// - A game-side adapter for what only a game has (Twist: games/bejeweledtwist/src/TwistD3D.h): DrawPrimitiveEx and its
//   mesh listener, the .p3d loader and its bump-texture callback, the shader table and its fixed-function fallback
//   ladder, the slots' shader-argument rule for constants (below). The adapter owns the meshes it loads and the
//   textures their callback gives them (shared images from the resource manager, released with the mesh).
//
// Where D3D9 and GL differ, the GL implementation converts in one place (GPUInterfaceGL.cpp, one unit test each in tools/gputest):
// - pixel centres: D3D9's are on integers, GL's on half-integers. Every vertex path (pre-transformed XYZRHW,
//   fixed-function, game vertex shaders) gets clip x += w/W and the y term below, with W and H the viewport's size (the
//   signs are fixed by the half-pixel test, not by this comment).
//   The framework's own per-draw offsets (the policy's) stay as the original has them; nothing adds a literal GL -0.5.
// - orientation: every texture, render target and read-back is stored top row first and clip y is negated
//   (y_gl = -y + w/H), so GL's window y equals D3D's: glViewport and glScissor take D3D's rectangles as they are. Only
//   the present quad to the default framebuffer flips. With the y flip, CULLMODE CW/CCW map to the opposite GL face.
// - depth: the games' projections give z in [0,1]; z_gl = 2z - w. XYZRHW vertices take w = 1/rhw.
// - colour: straight alpha as in D3D9 (the blend factors as set; the same blend on alpha; nothing premultiplied, unlike
//   DDInterface). MemoryImage's ARGB words are swizzled to RGBA on the CPU at upload (GLES3 and WebGL2 have no BGRA),
//   D3DCOLOR vertex colours in the vertex stage. Dithering off. Fragment stages use highp.
// - textures in 16-bit formats (R5G6B5, A4R4G4B4) are quantised as D3D9 stores them, truncating, when the policy's
//   format rule picks them; the GL texture may be RGBA8 holding the quantised values.
// - the screen is X8R8G8B8: its target reads alpha 1 when copied, sampled or read back; the canvas has alpha off.
//   A render target's format is the caller's, from the policy's format rule (0x61c070): an opaque target (flag 0x10,
//   made opaque by SetImageMode(false, false) first: Twist's 3D-mode screen, the visualizers' targets, Hyperspace's
//   picture, slot 0xa0's) is X8R8G8B8, a GL RGB8 texture that samples and reads back alpha 1; one with alpha or
//   transparency is A8R8G8B8: UnderDialogWidget's two (0x7567e7) are made by DDImage::Create (0x409ef0), which
//   leaves them without bits, and CommitBits (0x42e270) calls an image with neither bits nor a palette transparent.
// - one texture level: MIPFILTER is recorded and ignored, TEXTURE_MAX_LEVEL 0.
// - fixed function: texture coordinates of two components expand to (u, v, 1, 0) before a texture matrix (COUNT2), so
//   its translation is row 2; D3D9's lighting equations (a point light's range cut-off and attenuation, LOCALVIEWER,
//   normals normalised only with NORMALIZENORMALS, the material sources); lit colours clamp per vertex before
//   interpolation. Alpha test is a discard after the pixel stage, game pixel shaders included, then blending.
// - Clear honours the viewport and an enabled D3D scissor, as D3D9's does, and ignores the write masks (it forces them
//   on while it clears: WorldGraphicsListener clears depth with z-write off). GPU2D clips in software and never sets
//   the scissor.
// - sampler state belongs to the stage: one GL sampler object per stage.
// - one depth buffer of the original's format (Twist: verify D16 or D24X8 in its CreateDevice) stays bound across
//   SetRenderTarget, and SetRenderTarget resets the viewport to the whole target. (If WebGL2 rejects attachments of
//   mixed sizes, each target size gets its own and the sharing holds for screen-sized targets only: checked first.)

#include "Common.h"
#include "Color.h"
#include "Rect.h"

#include <string>
#include <vector>

namespace Sexy
{

class Image;
class MemoryImage;
class GLContext;

// A 4x4 matrix as Direct3D lays it out (row vectors, v * M). Uploaded untransposed: GLSL's M * v then gives v * M.
struct GPUMatrix
{
	float m[4][4];
	static GPUMatrix Identity();
	GPUMatrix operator*(const GPUMatrix& theOther) const;
};

struct GPUVector3
{
	float x, y, z;
};

// D3DCOLORVALUE: floats, unclamped, as D3D9's materials and lights take them
struct GPUColorValue
{
	float r, g, b, a;
};

// D3DLIGHT9
struct GPULight
{
	int mType;  // 1 point, 2 spot, 3 directional (D3DLIGHTTYPE)
	GPUColorValue mDiffuse, mSpecular, mAmbient;
	GPUVector3 mPosition, mDirection;
	float mRange, mFalloff, mAttenuation0, mAttenuation1, mAttenuation2, mTheta, mPhi;
};

// D3DMATERIAL9
struct GPUMaterial
{
	GPUColorValue mDiffuse, mAmbient, mSpecular, mEmissive;
	float mPower;
};

// The D3D9 enum values the games pass, unchanged. A state the implementation does not carry out is recorded and logged
// once, so the games' use of it shows.
enum GPURenderState
{
	GPURS_ZENABLE = 7, GPURS_FILLMODE = 8, GPURS_ZWRITEENABLE = 14, GPURS_ALPHATESTENABLE = 15, GPURS_LASTPIXEL = 16, GPURS_SRCBLEND = 19,
	GPURS_DESTBLEND = 20, GPURS_CULLMODE = 22, GPURS_ZFUNC = 23, GPURS_ALPHAREF = 24, GPURS_ALPHAFUNC = 25,
	GPURS_DITHERENABLE = 26, GPURS_ALPHABLENDENABLE = 27, GPURS_SPECULARENABLE = 29, GPURS_TEXTUREFACTOR = 60,
	GPURS_LIGHTING = 137, GPURS_AMBIENT = 139, GPURS_COLORVERTEX = 141, GPURS_LOCALVIEWER = 142,
	GPURS_NORMALIZENORMALS = 143, GPURS_DIFFUSEMATERIALSOURCE = 145, GPURS_SPECULARMATERIALSOURCE = 146,
	GPURS_AMBIENTMATERIALSOURCE = 147, GPURS_EMISSIVEMATERIALSOURCE = 148, GPURS_SCISSORTESTENABLE = 174
};

enum GPUTextureStageState
{
	GPUTSS_COLOROP = 1, GPUTSS_COLORARG1 = 2, GPUTSS_COLORARG2 = 3, GPUTSS_ALPHAOP = 4, GPUTSS_ALPHAARG1 = 5,
	GPUTSS_ALPHAARG2 = 6, GPUTSS_TEXCOORDINDEX = 11, GPUTSS_TEXTURETRANSFORMFLAGS = 24, GPUTSS_COLORARG0 = 26,
	GPUTSS_ALPHAARG0 = 27, GPUTSS_RESULTARG = 28
};

enum GPUSamplerState
{
	GPUSAMP_ADDRESSU = 1, GPUSAMP_ADDRESSV = 2, GPUSAMP_MAGFILTER = 5, GPUSAMP_MINFILTER = 6, GPUSAMP_MIPFILTER = 7
};

enum GPUTransformType
{
	GPUTS_VIEW = 2, GPUTS_PROJECTION = 3, GPUTS_TEXTURE0 = 16, GPUTS_WORLD = 256
};

enum
{
	GPUFVF_XYZ = 0x2, GPUFVF_XYZRHW = 0x4, GPUFVF_NORMAL = 0x10, GPUFVF_DIFFUSE = 0x40, GPUFVF_SPECULAR = 0x80,
	GPUFVF_TEX1 = 0x100, GPUFVF_TEX2 = 0x200, GPUFVF_TEX3 = 0x300, GPUFVF_TEX4 = 0x400
};

enum GPUPrimitiveType
{
	GPUPT_LINELIST = 2, GPUPT_LINESTRIP = 3,  // GPU2D's DrawLine (0x620cd0)
	GPUPT_TRIANGLELIST = 4, GPUPT_TRIANGLESTRIP = 5, GPUPT_TRIANGLEFAN = 6
};

enum
{
	GPUCLEAR_TARGET = 1, GPUCLEAR_ZBUFFER = 2
};

enum GPUTextureFilter
{
	GPUTEXF_POINT = 1, GPUTEXF_LINEAR = 2
};

// D3DFORMAT values for the texture formats the policy may pick
enum GPUFormat
{
	GPUFMT_A8R8G8B8 = 21, GPUFMT_X8R8G8B8 = 22, GPUFMT_R5G6B5 = 23, GPUFMT_A4R4G4B4 = 26
};

// A state change: what a scene-begin list holds
// How an image's texture is cut (0x61bd00): mColumns x mRows pieces of mPieceWidth x mPieceHeight in the image (the
// right column, bottom row and corner smaller), each piece's texture of its own allocated size (0x61c6e0 computes u, v
// against it), and the image's size over the inner piece's (texdata+0x3c, +0x40). A render target, or an image with
// flag 0x20 or 0x40, is one piece of its own size, max U and V 1.
struct GPUPiece
{
	int mWidth, mHeight;
};

struct GPUPieceLayout
{
	int mPieceWidth = 0, mPieceHeight = 0;
	int mColumns = 0, mRows = 0;
	std::vector<GPUPiece> mPieces;
	float mMaxU = 1.0f, mMaxV = 1.0f;
};

struct GPUStateSet
{
	enum Kind { RENDER, STAGE, SAMPLER } mKind;
	int mStage;
	int mType;
	unsigned long mValue;
};

// How one framework generation's original drew, stored its textures and answered the game. No defaults: a game that
// opts in takes a named preset (or builds its own from one), so nothing silently mixes generations. Twist() holds the
// values read from Twist's D3D9 body; SAF13() (SAF 1.3 on D3D7, for Bookworm Adventures and later games) is filled in
// when the first such game opts in, from its own export.
struct GPUPolicy
{
	// The scene begins lazily, at the first PreDraw of a frame (Twist's slot 0x1c, 0x61a7d0; EndScene is 0x24,
	// 0x6225a0). It then rebinds the base target, applies this list once, and GPU2D clears its filter cache.
	// Twist's list: ALPHABLENDENABLE 1, SRCBLEND SRCALPHA, DESTBLEND INVSRCALPHA, LIGHTING 0, ALPHATESTENABLE 0,
	// ALPHAREF 0, ALPHAFUNC GREATER; sampler 0 MIN, MAG and MIP POINT, ADDRESSU and ADDRESSV CLAMP; DITHERENABLE 0,
	// SPECULARENABLE 0, ZWRITEENABLE 0, ZENABLE 0, CULLMODE NONE, FILLMODE SOLID; stage 0 colour and alpha
	// MODULATE(TEXTURE, DIFFUSE), stage 1 colour and alpha DISABLE. (Its debug render modes 1-4, picked by a value of the
	// app that normal play leaves 0, are not carried.)
	std::vector<GPUStateSet> mSceneBegin;

	// Texture formats (Twist: 0x61c070, with the formats the device accepts, probed by 0x61a520 into the mask: bit 1
	// A8R8G8B8, 2 A4R4G4B4, 4 R5G6B5; Twist's probe has no P8, so its paletted path never runs). TextureFormat is the rule
	// in 0x61c070's order: an image with neither alpha nor transparency is R5G6B5 when the device takes it, unless it has
	// flag 8 (UseA8R8G8B8) or is a target (flag 0x10) on a display that is not R5G6B5; such a target is X8R8G8B8. Then an
	// image still A8R8G8B8 is A4R4G4B4 when it has flag 4 (UseA4R4G4B4) and the device takes it, or when the device does
	// not take A8R8G8B8. So an opaque image asking for A4R4G4B4 is R5G6B5.
	unsigned long mAcceptedFormats;
	bool mDisplayIs565;
	GPUFormat TextureFormat(bool theHasAlpha, bool theHasTrans, unsigned long theFlags) const;

	// Texture pieces (Twist: 0x61bd00 with 0x61b970, the sizes from the device caps, 0x6192d0). Twist on a card that
	// takes non-power-of-two textures (every card the port runs on; the policy fixes that answer): a piece is the image's
	// own size, at least mMinPieceSize, at most mMaxPieceSize (the caps' size, capped at 4096 by the original), a piece
	// wider than 512 and less than half as tall is cut to 512 wide unless the image's flags have 0x01 or 0x10, and the
	// aspect ratio is kept within the caps' limit. Use64By64Subdivisions (flag 0x02) gives 64 x 64. SAF 1.3: power-of-two
	// pieces, 1024 at most, one duplicated column and row of padding. Render targets are never cut: their texture is
	// exactly their size (0x621c60).
	bool mPowerOfTwoPieces;
	int mMinPieceSize;
	int mMaxPieceSize;
	int mWidePieceLimit;
	int mMaxAspectRatio;
	// 0x61b970's size for one piece (theIsEdge for the right column, bottom row and corner), and 0x61bd00's layout
	void PieceSize(int& theWidth, int& theHeight, bool theIsEdge, unsigned long theFlags) const;
	GPUPieceLayout Pieces(int theWidth, int theHeight, unsigned long theFlags) const;

	// The 2D draws' own offsets in screen space, after the transform stack. Twist's agree with SAF 1.3's: the Blt
	// vertex builder (0x61ca30) and FillRect (slot 0x68, 0x620ee0) -0.5; BltTransformed (slot 0x60, 0x620af0, building
	// in 0x61d700) -0.5, or 0 when centred; DrawTriangle (slot 0x6c, 0x621170), DrawTrianglesTex (slot 0x74, 0x622140,
	// building in 0x61e310) and FillPoly (slot 0x80, 0x6214a0) none; DrawLine (slot 0x64, 0x620cd0) none on its two
	// ends. DrawLine builds a third vertex at the end plus mLineOffset in x and y, and SAF 1.3 draws the three as a
	// strip of two lines; Twist passes a count of 1 (0x620ec9, drawn by 0x624710 as count + 1 vertices), so only the
	// two ends are drawn (mLineEndPixel false). Either way the end pixel is lit: D3DRS_LASTPIXEL is TRUE by default
	// and neither sets it, and the device carries that state.
	float mBltOffset, mFillRectOffset, mTransformedOffset, mTransformedCentredOffset, mTrianglesOffset, mLineOffset;
	bool mLineEndPixel;

	// D3D7's DirectDraw short-cut (an exact, unfiltered copy for opaque, white, normal, untransformed blits): SAF 1.3 only
	bool mDirectDrawCopies;

	// The device's answers the game can branch on. Fixed values, the same for the GL and the null renderer, so a
	// headless run takes the branches and random draws the real one takes: the caps (GetCaps; Twist's slot 0x04,
	// 0x61eec0, takes an argument: 0 gives 0x19, plus 2 for ps_2_0 (read at 0x756a92 and by Hyperspace), 4 for vs_2_0
	// (read by the constant slots), 0x20 cube maps and 0x40 volume maps, so 0x7f on a modern card; 1 gives
	// MaxTextureBlendStages, read by RainbowCubeFixedFunctionShader, 0x70f389), ValidateDevice's result for each shader
	// (0x30, whose answer the fixed-function ladder saves under <name>_fallback), the accepted formats. A cube texture
	// (flag 0x20: Twist's IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE, resources.xml:816) is carried, sampled by a stage's
	// texture coordinate as a direction; volume textures (0x40) are not: a request for one is logged.
	unsigned long mCaps;
	unsigned long mMaxTextureBlendStages;
	bool mValidateDevice;

	// D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES (0x6192d0 stores it in DAT_00e0c9b6): every card the port stands for has
	// it, so StretchRect (slot 0xac, 0x621dc0) always takes the device branch and its quad branch is never needed
	bool mCanStretchRectFromTextures;

	// The depth buffer's bits (D3DFMT_D16 or D24X8)
	int mDepthBits;

	static GPUPolicy Twist();
	static GPUPolicy SAF13();
};

// A vertex or pixel shader made from a game's GLSL ES 3.00 source (0 is the fixed-function vertex stage or the texture
// stage combiners). Sources are kept, so programs can be made again after a lost context.
//
// The stages meet through one fixed set of names, so any vertex stage pairs with any pixel stage:
//   in the vertex stage, attributes  aPosition (vec4), aNormal, aDiffuse, aSpecular (vec4, already RGBA), aTex0-aTex3;
//   a vertex shader writes           oPos (D3D's clip-space position) and vDiffuse, vSpecular, vTex0-vTex3 (vec4: the
//                                    D3D colour and texture registers oD0, oD1, oT0-oT3);
//   a pixel shader reads those and writes oC0;
//   uniforms                         uVS[64] and uPS[32] (vec4 arrays: the D3D constant registers of each bank),
//                                    uSampler0-uSampler3.
// The implementation declares all of these around a game's source (which has its own main) and adds the conventions
// above in its own main: the position fix-up from oPos to gl_Position, alpha test on oC0. It makes one program per
// (vertex stage, pixel stage) pair on first use. The fixed-function and combiner stages are
// generated per normalised state key; every key a run uses can be listed, saved and compiled ahead next time
// (Precompile), since a link stalls tens of ms on ANGLE.
//
// The constant banks are the device's, not a program's: they are kept on the CPU and uploaded to whichever program
// draws (the registers that changed since that program last drew). D3D9's bool and int constants have no counterpart:
// Twist's shaders use no b or i registers, so the adapter treats its bool and int slots as no-ops.
typedef int GPUVertexShader;
typedef int GPUPixelShader;

// A static vertex or index buffer (the .p3d meshes' vertices and triangles). Its CPU copy is kept for a lost context.
typedef int GPUBuffer;

class GPUInterface
{
public:
	// The GL implementation, on the context GPUDisplay made (current); null when the context cannot do what it needs
	static GPUInterface* Create(GLContext* theContext, const GPUPolicy& thePolicy, int theWidth, int theHeight);
	// One that accepts every call, draws nothing and answers as the policy says (headless runs: the verifier, the wasm
	// under Node). A run that asked for 3D mode and got no context stops with an error rather than taking it silently,
	// since 3D mode changes the games' logic and random draws, not only their drawing. IsNull is for GPUDisplay only;
	// nothing the game branches on may come from it.
	static GPUInterface* CreateNull(const GPUPolicy& thePolicy, int theWidth, int theHeight);
	virtual ~GPUInterface() {}

	virtual bool IsNull() = 0;
	virtual const GPUPolicy& GetPolicy() = 0;

	// The device. A resize remakes the screen target and depth buffer.
	virtual void Resize(int theWidth, int theHeight) = 0;
	// The context was lost (webglcontextlost: preventDefault) and given back: programs are remade from their sources,
	// buffers from their CPU copies, targets are made again empty and every texture is dropped; the caller marks every
	// widget dirty, as SDL_RENDER_TARGETS_RESET does.
	virtual void ContextLost() = 0;
	virtual void ContextRestored() = 0;

	// The scene, as the policy describes it: BeginSceneIfNeeded is the original's PreDraw. It returns false when the app
	// is minimised (Twist's app+0x4d8, SetSceneAllowed) and otherwise true; at a scene's start it rebinds the base
	// target (the screen) and applies mSceneBegin. Only PreDraw's callers call it: here SetRenderTarget, SetPixelShader,
	// SetVertexShader and StretchRect (0x621890, 0x622dd0, 0x622ea0, 0x621dc0); GPU2D's 2D draws, DrawTrianglesTex and
	// FillPoly, and the adapter's DrawPrimitiveEx, before they draw. Clear, the state setters, SetTexture and the
	// primitives never do (DrawObject's mesh path draws without one), so a state set before the frame's first PreDraw is
	// overwritten by the list, as in the original. EndScene [0x24] ends the scene and unbinds both shaders.
	virtual bool BeginSceneIfNeeded() = 0;
	virtual void EndScene() = 0;
	virtual void SetSceneAllowed(bool theAllowed) = 0;  // GPUDisplay: false while the app is minimised
	virtual int GetSceneCount() = 0;                    // GPU2D clears its filter cache when this moves

	// Every call takes effect at once, in call order; the device keeps no batch and never flushes. GPU2D owns the
	// original's quad batch and flushes it exactly where the original does (0x624830: a changed render, stage or sampler
	// state, texture, target or shader, DrawPrimitiveEx, DrawObject, ClearDepth, a draw-mode slot whose value changes,
	// EndScene, a full batch; not Clear, the constant slots, SetTransform, lights, material or read-back), so
	// under_dialog's three passes draw with the constants the original's do and a Clear after batched quads clears
	// underneath them.

	// Render targets [0x44, 0x94, 0x98]: an image the GPU draws into and samples like any other, exactly its own size,
	// in the format the caller gives (GPUPolicy::TextureFormat with the target's flag 0x10). SetRenderTarget (0x621990)
	// for an image returns true once it is a target, even when PreDraw fails and the target is left as it was; for null
	// (the screen) it returns false when PreDraw fails. Setting the bound target again changes nothing (the viewport
	// stays). The visualizers skip their blocks on false (0x6822a5, 0x689b0d, 0x689be3).
	virtual bool CreateRenderTarget(Image* theImage, GPUFormat theFormat) = 0;
	virtual bool SetRenderTarget(Image* theImage) = 0;
	virtual Image* GetRenderTarget() = 0;
	// The screen's colour buffer and a render target's of the screen's own size change places (the swap at the end of
	// Twist's slot 0xa0, 0x621ae0, which exchanges the screen image's surfaces with a target's): what was drawn to the
	// screen so far is the image's to sample, and drawing to the screen goes on in the image's old buffer. Calling it
	// again with the same image swaps them back. False (nothing swapped) for an image that is not such a target.
	virtual bool SwapScreen(Image* theImage) = 0;
	// The device's StretchRect (slot 0xac's device branch, which is the only one taken: mCanStretchRectFromTextures): a
	// copy from the screen or a target into a target with D3D's filter (Twist always passes LINEAR), no shader, the
	// scissor off for the blit; the source must be a target, else nothing happens. It calls PreDraw first. The screen is
	// always copied before it is sampled: WebGL2 rejects drawing from the target being drawn to.
	virtual void StretchRect(Image* theSource, const Rect& theSrcRect, Image* theDest, const Rect& theDestRect,
		GPUTextureFilter theFilter) = 0;
	// [0x9c, 0x124]: theFlags of GPUCLEAR_*; the colour is D3DCOLOR (ARGB), z in [0,1]
	virtual void Clear(unsigned long theFlags, unsigned long theColor, float theZ) = 0;
	virtual void SetViewport(const Rect& theRect, float theMinZ, float theMaxZ) = 0;
	virtual void SetScissorRect(const Rect& theRect) = 0;  // D3D state, used with GPURS_SCISSORTESTENABLE

	// Pixel ownership. An ordinary MemoryImage's pixels live in its bits: its texture is made on first use and made
	// again (or updated with texSubImage, for images rewritten every frame) when mBitsChangedCount moves. A target's and
	// the screen's pixels live on the GPU. The original's MemoryImage::GetBits (0x42ecf0) recovers an image that has a
	// texture but no bits through its read-back (slot 0x48), and render targets (purged by 0x61e7d0) are such images:
	// RecoverBits is that read-back, for gMemoryImageRecoverBits (MemoryImage.h). It and ReadBack (screenshots, tests)
	// log every use, so a game path that needs one shows. When a target's mBitsChangedCount moves, its next use as a
	// texture or target takes its bits as 0x61c070's target branch does: an A8R8G8B8 target is filled from its bits; an
	// X8R8G8B8 one is not (0x61b5a0 has no case for it), which is logged and leaves it as it was. ImageDeleted unbinds
	// the image from every stage and as the target.
	virtual bool ReadBack(Image* theImage, MemoryImage* theDest) = 0;  // null image: the screen
	// The frame to the window (D3D's Present): the screen scaled linearly into theDestRect of the drawable (top-left
	// origin, in pixels), black around it; then PresentImage draws an image's source rectangle over it (the custom cursor,
	// blended normally, linear). Neither touches the device's own state, target or viewport. The caller swaps.
	virtual void Present(const Rect& theDestRect, int theDrawableWidth, int theDrawableHeight) = 0;
	virtual void PresentImage(Image* theImage, const Rect& theSrcRect, float theX, float theY, float theWidth,
		float theHeight, int theDrawableWidth, int theDrawableHeight) = 0;
	virtual bool RecoverBits(MemoryImage* theImage) = 0;  // fills theImage->mBits from its texture; false without one
	virtual void BitsChanged(Image* theImage) = 0;
	virtual void ImageDeleted(Image* theImage) = 0;

	// The device's answers (GPUPolicy): theWhich 0 the caps word, 1 MaxTextureBlendStages [0x04]
	virtual unsigned long GetCaps(int theWhich) = 0;

	// State [0xc0 render, 0xbc stage, 0xc4 sampler, 0xc8-0xf0, 0x100-0x108]
	virtual void SetRenderState(int theState, unsigned long theValue) = 0;
	virtual void SetTextureStageState(int theStage, int theType, unsigned long theValue) = 0;
	virtual void SetSamplerState(int theStage, int theType, unsigned long theValue) = 0;
	virtual void SetTexture(int theStage, Image* theImage) = 0;  // binds piece 0, as SAF's SetCurTexture
	// Slot 0x44 (0x61e7d0): makes or refreshes the image's texture pieces and answers their layout; null when the
	// original's would fail (no texture format). Each piece's texture is its allocated size; 0x61b5a0's upload copies
	// the image's part and duplicates its last column and row into the padding (SAF 1.3's rightPad and bottomPad). The
	// original then purges the bits of a render target on every call and of a nobits image once: that is GPUDisplay's,
	// through a purge that frees bits only where gMemoryImageRecoverBits can bring them back.
	virtual const GPUPieceLayout* PrepareTexture(Image* theImage) = 0;
	virtual void SetTexturePiece(int theStage, Image* theImage, int thePiece) = 0;  // GPU2D's slot 0xd0
	virtual void SetTransform(int theType, const GPUMatrix& theMatrix) = 0;
	virtual void SetLight(int theIndex, const GPULight& theLight) = 0;
	virtual void LightEnable(int theIndex, bool theEnable) = 0;
	virtual void SetMaterial(const GPUMaterial& theMaterial) = 0;

	// Shaders [0xd8 pixel, 0xdc vertex; the constants 0x100-0x108]. The device's calls take no shader argument; the
	// slots' rule (0x623b80, 0x623ad0, 0x623a20: a constant is set only when the shader the game names is the bound
	// one, else nothing happens) is the adapter's.
	virtual GPUVertexShader CreateVertexShader(const std::string& theName, const char* theSource) = 0;
	virtual GPUPixelShader CreatePixelShader(const std::string& theName, const char* theSource) = 0;
	virtual void SetVertexShader(GPUVertexShader theShader) = 0;
	virtual void SetPixelShader(GPUPixelShader theShader) = 0;
	virtual GPUVertexShader GetVertexShader() = 0;
	virtual GPUPixelShader GetPixelShader() = 0;
	virtual void SetVertexShaderConstantF(int theRegister, const float* theValues, int theCount) = 0;
	virtual void SetPixelShaderConstantF(int theRegister, const float* theValues, int theCount) = 0;
	// The program keys this run has used, to be saved; and the ones a game gives back at its start
	virtual std::vector<std::string> GetUsedProgramKeys() = 0;
	virtual void Precompile(const std::vector<std::string>& theKeys) = 0;

	// Primitives. User-pointer vertices of any FVF above go through a streaming buffer (bufferSubData with orphaning;
	// WebGL2 has no client arrays) [0x7c]. Buffers hold the meshes [0x38, 0x10c].
	virtual void DrawPrimitiveUP(GPUPrimitiveType theType, int thePrimitiveCount, unsigned long theFVF,
		const void* theVertices, int theStride) = 0;
	virtual GPUBuffer CreateVertexBuffer(unsigned long theFVF, const void* theVertices, int theCount, int theStride) = 0;
	virtual GPUBuffer CreateIndexBuffer(const unsigned short* theIndices, int theCount) = 0;
	virtual void DeleteBuffer(GPUBuffer theBuffer) = 0;
	virtual void DrawIndexedPrimitive(GPUPrimitiveType theType, GPUBuffer theVertices, GPUBuffer theIndices,
		int theStartIndex, int thePrimitiveCount) = 0;
};

}  // namespace Sexy

// GPU2D (framework/GPU2D.h, the next draft): the generic framework layer above the device. What it must carry, as read
// from Twist's D3D9 body (SAF 1.3 where Twist's agrees):
//
// - The draw-mode layer, slots 0x114-0x13c (0x624be0-0x624c70), each caching its value and, only when the new value
//   differs, flushing the quad batch (0x624830) and applying the depth and alpha-test states (0x13c): 0x114 the depth
//   test mode (field 0xd4), 0x118 the depth-write mode (0xd5), 0x11c the alpha reference (0xd6), 0x120 two floats (0xd7,
//   0xd8; the constructor 0x618da0 sets 0.25 and 0.5) that are the z of every 2D vertex: 0xd7 when 0xd5 != 0, else 0xd8
//   (the vertex builders 0x61ca30, 0x61d700, 0x61e310, 0x620cd0, 0x620ee0, 0x621170, 0x6214a0). 0x13c (0x61fbf0): with
//   0xd4 and 0xd5 both 0, alpha test, z and z-write off, ALPHAREF 0, ZFUNC ALWAYS. Otherwise alpha test on; z-write on
//   when 0xd5 != 0; 0xd4 = 0: z on when 0xd5 != 0, ZFUNC ALWAYS; 0xd4 = 2: z on, LESSEQUAL; any other 0xd4: z on,
//   GREATEREQUAL; ALPHAREF 0xd6 when 0xd5 != 0, else 0. 0x138 (0x61fb90) applies a draw's blend from its draw mode and
//   0xd5: with 0xd5 = 1, SRCBLEND ZERO and DESTBLEND ONE (the colour is left as it is: a depth-only pass); else normal
//   SRCALPHA/INVSRCALPHA, additive SRCALPHA/ONE. Every 2D draw, DrawTrianglesTex, the adapter's DrawPrimitiveEx,
//   StretchRect's quad branch and DrawObject call 0x138, so this layer and its state are GPU2D's and the adapter calls
//   it. The scene begin resets device states but not these caches, as in the original.
// - The filter cache (Twist's DAT_00e13bf5, SAF 1.3's gLinearFilter): 0x61c9b0 writes stage 0's MIN, MAG and MIP only
//   when the draw's filter differs from the cache; the game's own SetSamplerState bypasses it, so a point-filtered Blt
//   after the game set stage 0 LINEAR (Board, 0x6caa13) stays linear, as in the original. The scene begin clears it.
//   Under a non-empty transform stack every draw is linear (Twist 0x620af0; SAF 1.3 D3DInterface.cpp:1977): Board's
//   DrawAll pushes the board's scale, so every board draw is linear.
// - Each 2D draw sets exactly what its body sets (the filter, the blend through 0x138, the batch) and inherits the rest:
//   a pixel shader, textures in stages 1-3, stage states, TEXTUREFACTOR, alpha test or a stage-0 WRAP left by game code
//   apply to it as in the original. Filter per draw as SAF 1.3: BltF always linear, an integer Blt never, StretchBlt
//   !fastStretch, BltMatrix and DrawTrianglesTex their blend argument (DDInterface's IsFractional is not carried).
// - The quad batch (the original's dynamic vertex buffer) with the original's flush points, listed above
//   GPUInterface's render targets. The per-draw offsets are the policy's (above). The batch holds triangle lists and
//   DrawLine's line strips (0x624880's second argument, 4 or 3; a line strip flushes and is drawn at once).
// - The format rule of the policy with flag 8 (UseA8R8G8B8, the a8r8g8b8 resource attribute set at 0x469ca0: twelve of
//   Twist's images, seven of them visualizer backgrounds such as IMAGE_MOLECULE_BG); images with flag 0x20 or 0x40 take
//   one piece of their own size (0x61bd00's first branch), and Twist duplicates one column and one row into a piece's
//   padding (0x61b5a0) as SAF 1.3 does.
// - Clipping in software, as the original does it (SAF's PointClipper: 0x614f40 and the others, GPU2D.h), not by a
//   scissor: BltTransformed's pieces and FillPoly clip their polygons, so nothing on the 2D path touches the scissor
//   and no clipped draw can cut the next mesh.
// - The pieces and formats of the policy. Twist's DrawTrianglesTex draws one-piece images only, scaling u and v by
//   max U and V, and draws nothing for a cut image (SAF 1.3 clips its triangles per piece).
// - Routing, as Twist's DDImage overrides do it. Check3D (0x404a70) sends a draw to the D3D path only in 3D mode and
//   only when the destination image has flag 0x10 (the 3D-mode screen and the render targets), and then answers with
//   SetRenderTarget(image) (slot 0x98). On false the draw is not skipped: it falls through to the software path on
//   the image's bits (DDImage::FillRect, 0x404ec0, and the others), whose BitsChanged reaches an A8R8G8B8 target at
//   its next use and not an X8R8G8B8 one. In practice false is the screen while minimised. DDImage overrides
//   PolyFill3D (to FillPoly), FillRect, DrawLine, DrawLineAA, FillScanLinesWithCoverage and the Blts. ClearRect is
//   not overridden: MemoryImage's (0x42f0e0) zeroes the rectangle in GetBits() and calls BitsChanged, so on a
//   purged target it reads the target back (RecoverBits), and the cleared rectangle shows only on an A8R8G8B8
//   target. FillScanLines is Image's (0x423e50): one FillRect a span, so its spans are D3D FillRects on a 3D
//   destination. Images the original draws in software stay in software.
//
// GPUDisplay (after GPU2D, with two preconditions of its own):
// - Forwarding: an Image draw goes to GPU2D only in 3D mode and only when its destination has flag 0x10, as the
//   original's Check3D (above, in GPU2D's routing); every other draw stays in software, so Twist's 2D mode and its
//   frozen baseline are untouched by this path.
// - 2D mode in an opted-in game (decided, option (a), staged): the mode is fixed for a run, since the original restarts
//   on a 3D change (0x69ed33), so there is no live handover. GPUDisplay serves 3D runs first, and 2D runs keep today's
//   DDInterface. Later 2D mode moves to the framework's software 2D path (queued), the screen's bits uploaded once a
//   frame and presented by the GL quad, compared with the original in 2D mode by shotdiff, not with today's
//   SDL_Renderer screen. An in-process restart must not leave the web canvas on WebGL1.
// - DDInterface first gets a behaviour-neutral refactor on main (its renderer-free geometry in one helper both displays
//   use: letterboxing and the presentation rect, WindowToLogical and LogicalToWindow, HiDPI, the cursor's placement,
//   resize and full screen; the methods SexyAppBase calls made virtual), checked with Bejeweled's suite; its spec goes
//   to the coordinator first. Render scale stays 1 on the D3D9 path.
// - platform/GLContext: the window with SDL_WINDOW_OPENGL, ES 3.0 / WebGL2 (-sMAX_WEBGL_VERSION=2 for the games that opt
//   in), else GL 3.3 core with a VAO bound and the shaders' version line rewritten. No multisampling is asked for;
//   drivers that force it are harmless because the frame is drawn into an FBO and presented by a quad.
//
// The unit test (tools/check.sh gpu): on the real GPU under rundesk (its private desktop gives SDL GL 3.3 core and an
// ES 3.0 context on the GTX 1080, with correct read-back, 2026-11-02), one test per convention: orientation, the
// half-pixel rule, the z range, ARGB order, straight alpha, cull winding, Clear under a D3D scissor, depth Clear with z-write off, StretchRect's filters, read-back of a target, the constant banks across two
// programs, a lost and restored context, the null renderer's answers equal to the real one's. Exact pixels where the
// result is exact (integer-aligned point-sampled quads at texel centres, alpha 0 and 255, exact z), +-1 per channel for
// blends and filters. A Create that returns null fails the test.

#endif
