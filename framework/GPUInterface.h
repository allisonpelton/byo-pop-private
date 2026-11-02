#ifndef __GPUINTERFACE_H__
#define __GPUINTERFACE_H__

// port: the GPU display path (draft 3, for review; not built yet). It stands where PopCap's framework has its
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
// Where D3D9 and GL differ, the GL implementation converts in one place (GPUConventions.h, one unit test each):
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
//   Render targets are A8R8G8B8 (the back buffer's X8R8G8B8 becomes A8R8G8B8 for a texture, 0x621c60).
// - one texture level: MIPFILTER is recorded and ignored, TEXTURE_MAX_LEVEL 0.
// - fixed function: texture coordinates of two components expand to (u, v, 1, 0) before a texture matrix (COUNT2), so
//   its translation is row 2; D3D9's lighting equations (a point light's range cut-off and attenuation, LOCALVIEWER,
//   normals normalised only with NORMALIZENORMALS, the material sources); lit colours clamp per vertex before
//   interpolation. Alpha test is a discard after the pixel stage, game pixel shaders included, then blending.
// - Clear honours the viewport and an enabled D3D scissor, as D3D9's does, and ignores the write masks (it forces them
//   on while it clears: WorldGraphicsListener clears depth with z-write off). GPU2D's private clip scissor is not D3D
//   state and is never on during a Clear, StretchRect or any primitive that is not a clipped 2D draw.
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
	GPURS_ZENABLE = 7, GPURS_FILLMODE = 8, GPURS_ZWRITEENABLE = 14, GPURS_ALPHATESTENABLE = 15, GPURS_SRCBLEND = 19,
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
	// The scene begins lazily: at the first draw of a frame, at SetRenderTarget and at SetPixelShader (Twist's PreDraw,
	// slot 0x1c, 0x61a7d0; EndScene is 0x24, 0x6225a0). It then applies this list once, and clears GPU2D's filter cache.
	// Twist's list: ALPHABLENDENABLE 1, SRCBLEND SRCALPHA, DESTBLEND INVSRCALPHA, LIGHTING 0, ALPHATESTENABLE 0,
	// ALPHAREF 0, ALPHAFUNC GREATER; sampler 0 MIN, MAG and MIP POINT, ADDRESSU and ADDRESSV CLAMP; DITHERENABLE 0,
	// SPECULARENABLE 0, ZWRITEENABLE 0, ZENABLE 0, CULLMODE NONE, FILLMODE SOLID; stage 0 colour and alpha
	// MODULATE(TEXTURE, DIFFUSE), stage 1 colour and alpha DISABLE. (Its debug render modes 1-4, picked by a value of the
	// app that normal play leaves 0, are not carried.)
	std::vector<GPUStateSet> mSceneBegin;

	// Texture formats (Twist: 0x61c070, with the formats the device accepts, probed by 0x61a520): A8R8G8B8; R5G6B5 for an
	// image with neither alpha nor transparency that is not a render target; A4R4G4B4 for an image that asks for it
	// (UseA4R4G4B4). Twist's probe has no P8, so its paletted path never runs.
	bool mOpaqueAs565;

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

	// The 2D draws' own offsets in screen space, after the transform stack (SAF 1.3: Blt and FillRect -0.5,
	// BltTransformed -0.5 but a centred BltMatrix 0, DrawTrianglesTex none, DrawLine +0.5; Twist: read from its draw
	// bodies before GPU2D is written)
	float mBltOffset, mFillRectOffset, mTransformedOffset, mTransformedCentredOffset, mTrianglesOffset, mLineOffset;

	// D3D7's DirectDraw short-cut (an exact, unfiltered copy for opaque, white, normal, untransformed blits): SAF 1.3 only
	bool mDirectDrawCopies;

	// The device's answers the game can branch on. Fixed values, the same for the GL and the null renderer, so a
	// headless run takes the branches and random draws the real one takes: the caps (Twist's slot 0x04: bit 2 for
	// ps_2_0, read at 0x756a92 and by Hyperspace; bit 4, read by the constant slots), ValidateDevice's result for each
	// shader (0x30, whose answer the fixed-function ladder saves under <name>_fallback), the accepted formats.
	unsigned long mCaps;
	bool mValidateDevice;

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

	// The scene, as the policy describes it: BeginSceneIfNeeded applies mSceneBegin once per scene and returns false
	// where the original's PreDraw fails (the app minimised: Twist's app+0x4d8). GPU2D and every draw call it, as the
	// original's draws, SetRenderTarget and SetPixelShader call PreDraw. EndScene ends the frame's scene [0x24].
	virtual bool BeginSceneIfNeeded() = 0;
	virtual void EndScene() = 0;
	virtual void SetSceneAllowed(bool theAllowed) = 0;  // GPUDisplay: false while the app is minimised
	virtual int GetSceneCount() = 0;                    // GPU2D clears its filter cache when this moves

	// Draws execute in call order. GPU2D's batch of quads (the original's dynamic vertex buffer) is flushed through
	// DrawPrimitiveUP before any state change, upload, texture deletion, read-back or target change, as the original
	// flushes (0x624830) before its draw-mode slots.

	// Render targets [0x44, 0x98]: an image the GPU draws into and samples like any other, exactly its own size.
	// SetRenderTarget(null) is the screen; it returns false only where the original's does (BeginSceneIfNeeded failing),
	// and the visualizers skip their blocks then (0x6822a5, 0x689b0d, 0x689be3).
	virtual bool CreateRenderTarget(Image* theImage) = 0;
	virtual bool SetRenderTarget(Image* theImage) = 0;
	virtual Image* GetRenderTarget() = 0;
	// The device's StretchRect: a copy from the screen or a target into a target with D3D's filter, no shader, the
	// scissor off for the blit. (Slot 0xac, 0x621dc0, has a second branch that draws a quad through the bound pixel
	// shader when the source has a texture container: that is GPU2D's, and GPU2D takes the device branch for the screen
	// on purpose, since D3D9's back buffer has no texture container though the port's screen FBO does.) The screen is
	// always copied before it is sampled: WebGL2 rejects drawing from the target being drawn to.
	virtual void StretchRect(Image* theSource, const Rect& theSrcRect, Image* theDest, const Rect& theDestRect,
		GPUTextureFilter theFilter) = 0;
	// [0x9c, 0x124]: theFlags of GPUCLEAR_*; the colour is D3DCOLOR (ARGB), z in [0,1]
	virtual void Clear(unsigned long theFlags, unsigned long theColor, float theZ) = 0;
	virtual void SetViewport(const Rect& theRect, float theMinZ, float theMaxZ) = 0;
	virtual void SetScissorRect(const Rect& theRect) = 0;  // D3D state, used with GPURS_SCISSORTESTENABLE

	// Pixel ownership. An ordinary MemoryImage's pixels live in its bits: its texture is made on first use and made
	// again (or updated with texSubImage, for images rewritten every frame) when mBitsChangedCount moves. A target's and
	// the screen's pixels live on the GPU, and in normal play the original never reads them back to the CPU (no
	// normal-path call of its read-back, 0x48). ReadBack is for screenshots and tests, and logs every use, so a game
	// path that needs one shows. ImageDeleted unbinds the image from every stage and as the target.
	virtual bool ReadBack(Image* theImage, MemoryImage* theDest) = 0;  // null image: the screen
	virtual void BitsChanged(Image* theImage) = 0;
	virtual void ImageDeleted(Image* theImage) = 0;

	// State [0xbc render, 0xc0 stage, 0xc4 sampler, 0xc8-0xf0, 0x100-0x108]
	virtual void SetRenderState(int theState, unsigned long theValue) = 0;
	virtual void SetTextureStageState(int theStage, int theType, unsigned long theValue) = 0;
	virtual void SetSamplerState(int theStage, int theType, unsigned long theValue) = 0;
	virtual void SetTexture(int theStage, Image* theImage) = 0;  // binds piece 0, as SAF's SetCurTexture
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
// - The draw-mode layer, slots 0x114-0x13c, each flushing the quad batch first (0x624830) and caching its value:
//   0x114 the depth test mode (field 0xd4), 0x118 the depth-write mode (0xd5), 0x11c the alpha reference (0xd6), 0x120
//   two floats (0xd7, 0xd8; what they feed: verify), each then applying the depth and alpha-test states (0x13c,
//   0x61fbf0: with both modes 0, alpha test, z and z-write off and ZFUNC ALWAYS; else alpha test on, z-write by 0xd5, z
//   on with ZFUNC LESSEQUAL or by 0xd4, ALPHAREF 0xd6). 0x138 (0x61fb90) applies a draw's blend from its draw mode and
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
// - The private clip scissor: set for each draw that clips in the original and off for everything else (Clear,
//   StretchRect, DrawTrianglesTex unless Twist's 0x74 clips, the adapter's primitives, DrawObject), so a clipped dialog
//   draw cannot cut the next mesh.
// - The pieces and formats of the policy; DrawTrianglesTex clips its triangles per piece and scales coordinates by the
//   piece's used size over its texture size.
// - ClearRect and the scan-line fills on the screen or a target: before GPU2D is written, Twist's DDImage overrides are
//   read to see whether they go to the device (then they are GPU2D draws on GPU-owned pixels) or to the bits (then the
//   rule is a read-back, which the original would need too and the logging shows). Images the original draws in
//   software stay in software.
//
// GPUDisplay (after GPU2D, with two preconditions of its own):
// - Forwarding: an Image draw goes to GPU2D only in 3D mode and only when its destination is the screen or a render
//   target, as the original's Check3D; every other draw stays in software, so Twist's 2D mode and its frozen baseline
//   are untouched by this path.
// - 2D mode in an opted-in game, which has one GL context (a canvas never changes context type, and SDL_Renderer
//   cannot share WebGL2's): the plan to decide before GPUDisplay is written is either (a) the screen drawn in software in
//   2D mode, as the original's DirectDraw 2D mode did, its bits uploaded once a frame and presented by the GL quad (then
//   the 2D frozen baseline is checked against the present SDL_Renderer screen first, and any difference explained), or
//   (b) SDL_Renderer's GLES2 back end made on the same WebGL2/ES3 context and GPUInterface saving and restoring the GL
//   state around it. Across a 2D/3D switch the screen's pixels go with it: up to the screen target on 2D->3D, read back
//   once (logged) on 3D->2D.
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
// half-pixel rule, the z range, ARGB order, straight alpha, cull winding, Clear under a D3D scissor and the private one
// off, depth Clear with z-write off, StretchRect's filters, read-back of a target, the constant banks across two
// programs, a lost and restored context, the null renderer's answers equal to the real one's. Exact pixels where the
// result is exact (integer-aligned point-sampled quads at texel centres, alpha 0 and 255, exact z), +-1 per channel for
// blends and filters. A Create that returns null fails the test.

#endif
