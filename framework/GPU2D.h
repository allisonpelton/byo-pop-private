#ifndef __GPU2D_H__
#define __GPU2D_H__

// port: the framework's 2D drawing on the GPU path (draft 2, review 1's edits). It is the layer of the original's D3D
// interface that sits above the device (framework/GPUInterface.h): the slots' compare-first state cache, the draw-mode
// layer, the filter cache, the transform stack, the batch, software clipping and the 2D draws themselves. It is read
// from Bejeweled Twist's D3D9Interface (vtable 0x8195f4), with PopCap's released SAF 1.3 D3DInterface as the guide to
// its shape; where they differ, Twist's export wins, and the differences are listed with each draw. Generic framework
// code: a game opts in through GPUDisplay and gives its GPUPolicy.
//
// Coordinates are the destination's pixels: the screen (logical size, render scale 1) or a render target. Every draw
// assumes the caller has bound its destination and that the destination answered yes (GPUDisplay's Check3D).
//
// The vertex is the original's: x, y, z, rhw 1, diffuse D3DCOLOR, specular 0, u, v (FVF 0x1c4, 32 bytes). Its z is
// the draw-mode layer's (mDepthZ[0] when the depth-write mode is set, else mDepthZ[1]). Its colour is the Color's
// fields as every builder packs them (0x620cd0, 0x620ee0, 0x6214a0, 0x61d700): the low byte of red, green and blue and
// alpha << 24, so out-of-range values wrap (alpha 256 is 0, -1 is 255), they are not clamped.

#include "Common.h"
#include "Color.h"
#include "GPUInterface.h"
#include "Rect.h"
#include "SexyMatrix.h"

namespace Sexy
{

class Image;
class MemoryImage;
class TriVertex;

struct GPU2DVertex
{
	float mX, mY, mZ, mRHW;
	unsigned long mDiffuse;
	unsigned long mSpecular;
	float mU, mV;
};

class GPU2D
{
public:
	GPU2D(GPUInterface* theDevice, const GPUPolicy& thePolicy);
	~GPU2D();

	// The slots' state cache. Each wrapper compares with the value it last set and, only when it differs, flushes the
	// batch and calls the device; an equal value makes no flush and no device call. Render state 0xc0 (0x622a60), stage
	// state 0xbc (0x6229f0), sampler state 0xc4 (0x622ad0): a map per kind whose unset value is 0xffffffff (0x623380).
	// Texture 0xd0 (0x622c70): a cache per stage at this+0x158, null at construction, binding a texture piece. Texture
	// 0xd4 (0x622cc0): null binds null through 0xd0; an image is prepared (slot 0x44, the device's PrepareTexture) and
	// its piece 0 bound through 0xd0; a cube or volume texture (image flags 0x20, 0x40) is logged and binds nothing.
	// Target, shaders: flush when the value changes. The draws' own calls and the adapter's game calls all go through
	// these, so the cache always holds the device's state and a batched quad never draws with the next draw's texture.
	void SetRenderState(int theState, unsigned long theValue);
	void SetTextureStageState(int theStage, int theType, unsigned long theValue);
	void SetSamplerState(int theStage, int theType, unsigned long theValue);
	unsigned long GetSamplerState(int theStage, int theType) const;  // 0x623400: the cached value, 0xffffffff unset
	void SetTexturePiece(int theStage, Image* theImage, int thePiece);  // 0xd0; null image unbinds
	void SetTexture(int theStage, Image* theImage);                     // 0xd4
	bool SetRenderTarget(Image* theImage);
	void SetVertexShader(GPUVertexShader theShader);
	void SetPixelShader(GPUPixelShader theShader);
	void ImageDeleted(Image* theImage);  // forgets theImage in the texture cache

	// PreDraw (slot 0x1c, 0x61a7d0): the device's BeginSceneIfNeeded. At a scene's start the device applies the policy's
	// list, which this records in the state cache, and the filter cache is set to 0 (POINT), as 0x61a7d0 sets
	// DAT_00e13bf5 = 0. EndScene (0x6225a0) flushes first.
	bool PreDraw();
	void EndScene();

	// The screen's size (the original's this[10], this[11], from DDInterface +0x80, +0x84 by 0x61a570): a BltTransformed
	// clip rectangle of exactly (0, 0, width, height) is not clipped against, whatever the destination.
	void SetScreenSize(int theWidth, int theHeight) { mScreenWidth = theWidth; mScreenHeight = theHeight; }

	// The draw-mode layer, slots 0x114-0x13c (0x624be0-0x624c70). Each setter compares, and only when the value changes
	// flushes the batch, stores it and applies the depth and alpha-test states (0x13c, 0x61fbf0): with the depth-test
	// and depth-write modes both 0, ALPHATESTENABLE 0, ALPHAREF 0, ZENABLE 0, ZWRITEENABLE 0, ZFUNC ALWAYS; otherwise
	// ALPHATESTENABLE 1, ZWRITEENABLE with the write mode, ZENABLE with the write mode when the test mode is 0 (ZFUNC
	// ALWAYS) or 1 with LESSEQUAL for test mode 2 and GREATEREQUAL for any other, then ALPHAREF the reference when
	// writing, else 0. The scene's start resets device states, not these fields.
	void SetDepthTestMode(int theMode);    // 0x114, field 0xd4
	void SetDepthWriteMode(int theMode);   // 0x118, field 0xd5
	void SetAlphaRef(int theRef);          // 0x11c, field 0xd6
	void SetDepthZ(float theWriteZ, float theOtherZ);  // 0x120, fields 0xd7 and 0xd8 (0.25 and 0.5 at construction)
	// 0x138 (0x61fb90): the blend for a draw. Depth-write mode 1: ZERO / ONE (a depth-only pass). Otherwise normal
	// SRCALPHA / INVSRCALPHA, additive SRCALPHA / ONE. Every 2D draw calls it, and so do the adapter's DrawPrimitiveEx
	// and DrawObject.
	void ApplyDrawMode(int theDrawMode);

	// The batch (the original's dynamic vertex buffer, 0x624880 / 0x624830): 600 vertices (0x618da0: field 0xd1), always
	// on. Adding a triangle list flushes first whenever fewer than 3 slots are left; a strip or fan flushes first when
	// fewer than 3n are left, and one of more than 600 vertices as triangles is drawn at once in its own type. A strip
	// becomes (n-2, n-1, new) without D3D's odd-triangle swap, a fan (0, n-1, new). After adding, the batch flushes when
	// count + 3 > 600. Any other type (DrawLine's line strip) flushes and is drawn at once. A flush draws one triangle
	// list, and happens only inside a scene (field 0xbc, set by PreDraw, cleared by EndScene); the wrappers above,
	// DrawPrimitiveEx, DrawObject, ClearDepth and EndScene flush. Clear, the constant slots, SetTransform, lights, the
	// material and read-back do not.
	void Flush();
	int GetDrawCallCount() const { return mDrawCalls; }   // fields 0x348 and 0x34c, the original's statistics
	int GetPrimitiveCount() const { return mPrimitives; }

	// The filter cache (Twist's DAT_00e13bf5, SAF's gLinearFilter): a draw writes stage 0's MAG, MIN and MIP filters
	// (0x61c9b0: in that order, LINEAR or POINT, through the sampler cache) only when its filter differs from this
	// cache; the game's own SetSamplerState bypasses it, so a point-filtered Blt after the game set stage 0 LINEAR
	// (Board, 0x6caa13) stays linear, as in the original.
	void SetLinearFilter(bool theLinear);

	// The transform stack (0x620af0's this[2], this[3]). Under a non-empty stack a Blt becomes a BltF, and the Blt
	// family through BltTransformed is linear; DrawTrianglesTex keeps its blend argument and the untextured draws set no
	// filter.
	void PushTransform(const SexyMatrix3& theTransform, bool theConcatenate = true);
	void PopTransform();
	// The top of the stack (null when empty): the adapter's DrawPrimitiveEx moves pre-transformed vertices through it
	const SexyMatrix3* GetTransform() const { return mTransformStack.empty() ? NULL : &mTransformStack.back(); }

	// The draws, each with the original's own calls and nothing else: PreDraw first, returning on false; the image's
	// texture (PrepareTexture, returning on false); ApplyDrawMode; the filter; the vertices into the batch. A draw sets
	// nothing more, so a pixel shader, textures in stages 1-3, stage states, TEXTUREFACTOR, alpha test or a stage-0 WRAP
	// left by game code apply to it, as in the original. Against SAF 1.3 the differences are: the vertex orders (1.3 is
	// TL, BL, TR, BR), the z of 0.25 / 0.5 (1.3 uses 0), skipping the clip for the screen's rectangle, the rounded
	// colours of the clipper, DrawTriangle as a list, and DrawTrianglesTex (below).
	//
	// Blt (slot 0x4c, 0x620770, building in 0x61ca30): with a non-empty stack it is BltF. Otherwise point-filtered;
	// over every texture piece the source rectangle covers, SetTexturePiece(0, piece), then a strip TL, TR, BL, BR at
	// the piece's place - 0.5 with the piece's u, v. The original's region mesh for one-piece images with a region list
	// (image+0x6c, chosen by screen coverage in 0x436710) is not carried yet: see the open points.
	void Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	// BltF (0x50, 0x6208e0): translate(x, y), BltTransformed linear.
	void BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
		const Color& theColor, int theDrawMode);
	// BltMirror (0x54, 0x620830): translate(-srcWidth, 0), scale(-1, 1), translate(x, y), BltTransformed.
	void BltMirror(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
		const Color& theColor, int theDrawMode, bool theLinear);
	// StretchBlt (0x58, 0x620950): scale(destW / srcW, destH / srcH), mirrored on request, translate(dest),
	// BltTransformed with linear = !fastStretch.
	void StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect,
		const Color& theColor, int theDrawMode, bool theFastStretch, bool theMirror);
	// BltRotated (0x5c, 0x620a30): translate(-cx, -cy), rotate, translate(x + cx, y + cy), BltTransformed linear.
	void BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect,
		const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	// BltTransformed (0x60, 0x620af0, building in 0x61d700), as SAF 1.3's. The filter is the argument with an empty
	// stack, else linear. With an empty stack, or with x = y = 0 exactly (0x620b4f), the matrix is M (Top * M under a
	// stack) and x, y and the centre flag pass through; otherwise it is Top * T(x, y) * M * T(-w/2, -h/2) (the last only
	// when centred; 0x620bbb-0x620c87) and x = y = 0 with the centre flag off. The builder walks the pieces as Blt does;
	// corners TL, TR, BL, BR from (0, 0), or (-w/2, -h/2) when centred; position M * p + (x, y) + offset, the offset
	// -0.5, or 0 when centred. With a clip rectangle that is not the screen's, a piece's quad with any vertex outside it
	// (inside is left <= x < right, 0x61db8b) is clipped in software (below) as TL, TR, BR, BL; otherwise a strip.
	void BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode,
		const Rect& theSrcRect, const SexyMatrix3& theTransform, bool theLinear, float theX, float theY, bool theCenter);
	// FillRect (0x68, 0x620ee0): no filter, SetTexture(0, null), a strip TL, TR, BL, BR at the rectangle - 0.5; with a
	// stack, M * (corner - 0.5) - 0.5, the corners written in SAF's TL, BL, TR, BR. No clip.
	void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	// DrawLine (0x64, 0x620cd0; DrawLineAA comes here too): SetTexture(0, null), a line between the two ends (as floats,
	// through M with a stack), flushing the batch and drawn at once. GPUPolicy::mLineEndPixel: Twist passes two
	// vertices, SAF 1.3 a strip of three with the third at the end + mLineOffset. The end pixel is lit: D3DRS_LASTPIXEL
	// is TRUE by default and Twist never sets it (the device carries the state). No Twist code draws a line.
	void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor,
		int theDrawMode);
	// DrawTriangle (0x6c, 0x621170): vertex colours (a vertex's own when non-zero, else the draw's), M with a stack,
	// SetTexture(0, null), one triangle as a list into the batch (1.3 draws a strip).
	void DrawTriangle(const TriVertex& theP1, const TriVertex& theP2, const TriVertex& theP3, const Color& theColor,
		int theDrawMode);
	// DrawTriangleTex (0x70, 0x6220b0): one triangle through DrawTrianglesTex with tx = the blend argument as a float,
	// ty 0 and blend true, as Twist's call passes them (SAF's own slip of a bool into the float tx argument).
	void DrawTriangleTex(const TriVertex& theP1, const TriVertex& theP2, const TriVertex& theP3, const Color& theColor,
		int theDrawMode, Image* theTexture, bool theBlend);
	// DrawTrianglesTex (0x74, 0x622140, building in 0x61e310): the filter is the blend argument, stack or not. One-piece
	// images only (max U and V at most 1): a cut image draws nothing (1.3 clips per piece). SetTexturePiece(0, piece 0),
	// the batch flushed, then the triangles drawn at once, 100 at a time, each as (v0, v2, v1) (1.3 keeps the order and
	// has no batch): x + tx, y + ty through M with a stack, no 0.5; colour as DrawTriangle; u, v times max U, V.
	void DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
		const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend);
	// DrawTrianglesTexStrip (0x78, 0x6221d0): the strip as triangles (i, i+1, i+2) without the odd swap, through
	// DrawTrianglesTex 100 triangles at a time.
	void DrawTrianglesTexStrip(Image* theTexture, const TriVertex theVertices[], int theNumTriangles,
		const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend);
	// FillPoly (0x80, 0x6214a0): nothing for fewer than 3 points; SetTexture(0, null); (point + t) as floats through M,
	// no 0.5, every vertex the draw's colour (0x6215a6); a fan into the batch, or with a clip rectangle clipped first.
	void FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor,
		int theDrawMode, int theTX, int theTY);

	// Software clipping (0x614f40, 0x614fd0, 0x6152c0, 0x6154f0, interpolating in 0x60f910, drawing in 0x61d540): SAF's
	// PointClipper, Sutherland-Hodgman against x < left, y < top, x >= right, y >= bottom in that order, the edges the
	// clip rectangle's ints as floats, each pass taking (last, first) and then (i, i + 1). x, y, u, v interpolate
	// linearly; z, rhw and specular are the first vertex's; colour interpolates per channel only when the two differ,
	// rounded by x87 fistp (to nearest, ties to even; SAF 1.3 truncates). The result is drawn as a fan of n - 2 when
	// n > 2. Nothing on the 2D path sets the scissor (no SCISSORTESTENABLE or SetScissorRect in Twist).

private:
	enum { BATCH_SIZE = 600, STAGES = 8 };
	typedef std::vector<GPU2DVertex> VertexList;

	GPUInterface* mDevice;
	GPUPolicy mPolicy;
	int mDepthTestMode, mDepthWriteMode, mAlphaRef;
	float mDepthZ[2];
	VertexList mBatch;
	int mDrawCalls, mPrimitives;
	int mFilter;  // 0 point, 1 linear
	int mSceneCount;
	int mScreenWidth, mScreenHeight;
	std::vector<SexyMatrix3> mTransformStack;
	std::map<long long, unsigned long> mStates;  // (kind, stage, type) -> the value last set; absent: 0xffffffff
	Image* mTextureImages[STAGES];  // this+0x158: the bound piece, as (image, piece)
	int mTexturePieces[STAGES];

	void SetCachedState(int theKind, int theStage, int theType, unsigned long theValue);
	const GPUPieceLayout* PrepareImage(Image* theImage);  // slot 0x44 for a draw; null for a cube or volume texture
	void ApplyDepthStates();  // 0x13c, 0x61fbf0
	float VertexZ() const { return (mDepthWriteMode != 0) ? mDepthZ[0] : mDepthZ[1]; }
	void AddPrimitives(GPUPrimitiveType theType, int theCount, const GPU2DVertex* theVertices);  // 0x624880
	void DrawNow(GPUPrimitiveType theType, int theCount, const GPU2DVertex* theVertices);       // 0x624710
	void DrawClipped(const Rect& theClipRect, const VertexList& theList);                        // 0x61d540
	void BltPieces(Image* theImage, const GPUPieceLayout& theLayout, const Rect& theSrcRect, unsigned long theColor,
		const SexyMatrix3* theMatrix, float theX, float theY, bool theCenter, const Rect* theClipRect);
};

}  // namespace Sexy

// Settled by review 1: P8 is not carried (0x61c070's palette branch needs accepted-format bit 8, which 0x61a520 never
// sets); the hooks at this+0x10 and +0x14 are never set (0x4bdb40 zeroes them) and are not carried; cube and volume
// textures (flags 0x20, 0x40; Twist's one, IMAGE_CUBEMAP_6X1_VIZBUCKYSHARDS_CUBE, is never read) are logged and drawn
// as nothing.
//
// For GPUDisplay, outside this layer: Check3D's callers that are not draws (Graphics's constructor through 0x404a40,
// which stores the answer at Graphics+0x4c; 0x421d50; LockSurface 0x404aa0; UnlockSurface 0x404b10) rebind the target
// through SetRenderTarget, which flushes. FillScanLinesWithCoverage's false path (0x41e600) locks a DirectDraw surface.
// DDImage::Blt's DirectDraw short-cut in 3D mode (0x412c30-0x412db9: a source with a DirectDraw surface, empty stack,
// mode 0, white, opaque) flushes and blits: logged. A nobits image (image+0x54) is purged once after its first texture,
// so GetBits returns the read-back of its quantised texture: logged.
//
// Open:
// - The region mesh (Blt and BltTransformed's one-piece images with a region list at image+0x6c, without flag 0x80 at
//   image+0x44; a level of detail chosen by screen coverage, 0.05 / 0.005 / 0.001). Nearly every image has one:
//   CommitBits (0x42e270) calls MemoryImage slot 36 (0x436530) unless byte +0x53 is set (nothing sets it), which builds
//   the list at +0x6c for an image with alpha or transparency (and +0x7c for every image) through slot 35 (0x435a30),
//   for images of at least 32x32 without flags 0x90: grids of 8, 16, 32... pixels marking the cells with any texel of
//   non-zero alpha. Pixel-identical to the quad wherever a texel of alpha 0 leaves the destination unchanged: the
//   point-filtered Blt under the default stages, both blends, and the depth pass (ALPHAFUNC GREATER rejects alpha 0).
//   Under linear filtering the quad lights a faint fringe in a skipped cell next to a drawn one unless the cells keep a
//   one-texel margin (0x436050 / 0x436240, not read yet); game stage states that make alpha-0 texels visible differ too.
// - IMAGE_DEFUSER_ALERTTRIMTEXTURE (review 1 asked whether it is drawn under WRAP): its global (0x85dcb8) is read
//   only by its load (0x7461fb-0x746210) and the resource table (0x850088), and its id (546) appears nowhere as a
//   constant, so no code draws it.

#endif
