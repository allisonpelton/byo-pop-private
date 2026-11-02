#ifndef __GPU2D_H__
#define __GPU2D_H__

// port: the framework's 2D drawing on the GPU path (draft 1, under review; GPU2D.cpp implements it but the pieces). It is the layer
// of the original's D3D interface that sits above the device (framework/GPUInterface.h): the draw-mode layer, the
// filter cache, the transform stack, the quad batch, texture pieces, software clipping and the 2D draws themselves.
// It is read from Bejeweled Twist's D3D9Interface (vtable 0x8195f4), with PopCap's released SAF 1.2 D3DInterface as
// the guide to its shape; where they differ, Twist's export wins, and the differences are listed with each draw.
// Generic framework code: a game opts in through GPUDisplay and gives its GPUPolicy.
//
// Coordinates are the destination's pixels: the screen (logical size, render scale 1) or a render target. Every draw
// assumes the caller has bound its destination and that the destination answered yes (GPUDisplay's Check3D).
//
// The vertex is the original's: x, y, z, rhw 1, diffuse D3DCOLOR, specular 0, u, v (FVF 0x1c4, 32 bytes). Its z is
// the draw-mode layer's (mDepthZ[0] when the depth-write mode is set, else mDepthZ[1]).

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

	// The draw-mode layer, slots 0x114-0x13c (0x624be0-0x624c70). Each setter caches its value and, only when it
	// changes, flushes the batch and applies the depth and alpha-test states (0x13c, 0x61fbf0): with the depth-test and
	// depth-write modes both 0, alpha test, z and z-write off, ALPHAREF 0, ZFUNC ALWAYS; otherwise alpha test on, z-write
	// on with the write mode, z on with test mode 0 only when writing (ZFUNC ALWAYS), test mode 2 LESSEQUAL, any other
	// GREATEREQUAL; ALPHAREF the reference when writing, else 0. The scene's start resets device states, not these caches.
	void SetDepthTestMode(int theMode);    // 0x114, field 0xd4
	void SetDepthWriteMode(int theMode);   // 0x118, field 0xd5
	void SetAlphaRef(int theRef);          // 0x11c, field 0xd6
	void SetDepthZ(float theWriteZ, float theOtherZ);  // 0x120, fields 0xd7 and 0xd8 (0.25 and 0.5 at construction)
	// 0x138 (0x61fb90): the blend for a draw. Depth-write mode 1: ZERO / ONE (a depth-only pass). Otherwise normal
	// SRCALPHA / INVSRCALPHA, additive SRCALPHA / ONE. Every 2D draw calls it, and so do the adapter's DrawPrimitiveEx,
	// StretchRect's quad branch and DrawObject.
	void ApplyDrawMode(int theDrawMode);

	// The batch (the original's dynamic vertex buffer, 0x624880 / 0x624830; 600 vertices, field 0xd1). Triangle
	// lists, strips and fans are kept as a triangle list and drawn in one call at a flush; a line strip flushes and is
	// drawn at once. Flush points are the original's: a changed render, stage or sampler state, texture, target or
	// shader, a draw-mode setter whose value changes, DrawPrimitiveEx, DrawObject, ClearDepth, EndScene, a full batch.
	// Clear, the constant slots, SetTransform, lights, the material and read-back do not flush. The original flushes
	// only inside a scene (field 0xbc, set by PreDraw); every batched draw comes after a PreDraw, so the batch is only
	// ever filled inside one.
	void Flush();
	int GetDrawCallCount() const { return mDrawCalls; }   // fields 0x348 and 0x34c, the original's statistics
	int GetPrimitiveCount() const { return mPrimitives; }

	// The state setters (slots 0xc0, 0xbc, 0xc4, 0xd0: 0x622a60, 0x6229f0, 0x622ad0): each compares with the value it
	// last set and, only when it differs, flushes the batch and sets it on the device. The adapter's game calls and the
	// draws' own go through them, so the cache always holds the device's state; a state never set here is unknown and
	// is always set (the original's cache answers a default for it: to be checked if a game relies on it).
	void SetRenderState(int theState, unsigned long theValue);
	void SetTextureStageState(int theStage, int theType, unsigned long theValue);
	void SetSamplerState(int theStage, int theType, unsigned long theValue);
	void SetTexture(int theStage, Image* theImage);
	void ImageDeleted(Image* theImage);  // forgets theImage as a bound texture

	// PreDraw (slot 0x1c, 0x61a7d0): the device's BeginSceneIfNeeded. At a scene's start the device applies the policy's
	// list, which this records in the state cache, and the filter cache is cleared. EndScene flushes first.
	bool PreDraw();
	void EndScene();

	// The screen's size (the original's this[10], this[11]): a BltTransformed clip rectangle of exactly the screen is
	// not clipped against, whatever the destination.
	void SetScreenSize(int theWidth, int theHeight) { mScreenWidth = theWidth; mScreenHeight = theHeight; }

	// The filter cache (Twist's DAT_00e13bf5, SAF's gLinearFilter): a draw writes stage 0's MAG, MIN and MIP filters
	// (0x61c9b0: in that order, LINEAR or POINT) only when its filter differs from the cache; the game's own
	// SetSamplerState bypasses it. Cleared when the device's scene count moves.
	void SetLinearFilter(bool theLinear);

	// The transform stack (0x620af0's this[2], this[3]). With a non-empty stack every Blt becomes a BltTransformed and
	// every draw is linear.
	void PushTransform(const SexyMatrix3& theTransform, bool theConcatenate = true);
	void PopTransform();

	// The draws, each with the original's own calls and nothing else: PreDraw (the device's BeginSceneIfNeeded) first,
	// returning on false; the image's texture; ApplyDrawMode; the filter; the vertices into the batch. A draw sets
	// nothing more, so a pixel shader, textures in stages 1-3, stage states, TEXTUREFACTOR, alpha test or a stage-0 WRAP
	// left by game code apply to it, as in the original.
	//
	// Blt (slot 0x4c, 0x620770, building in 0x61ca30): with a non-empty stack it is BltF. Otherwise point-filtered;
	// over every texture piece the source rectangle covers, SetTexture(0, piece), then a strip TL, TR, BL, BR at the
	// piece's place - 0.5 with the piece's u, v. The original's region mesh for sparse one-piece images (image+0x6c,
	// 0x436710) is not carried: it skips only transparent texels, which the 2D blends and alpha test leave unchanged.
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
	// BltTransformed (0x60, 0x620af0, building in 0x61d700). The filter is the argument with an empty stack, else
	// linear. With an empty stack the matrix, x, y and centre go to the builder as given; with a non-empty one and x =
	// y = 0, the stack's top times the matrix; otherwise translate(-w/2, -h/2) when centred, the matrix, translate(x, y)
	// and the stack's top are composed and x = y = 0, centre off (the order to be checked at 0x620b20-0x620c60). The
	// builder walks the pieces as Blt does; corners TL, TR, BL, BR from (0, 0), or (-w/2, -h/2) when centred; position
	// M * p - c + (x, y) with c 0.5, or 0 when centred (SAF: start -w/2 + 0.5, always -0.5; equal for translations).
	// With a clip rectangle that is not the whole destination, a piece's quad with any vertex outside it is clipped in
	// software (below) as TL, TR, BR, BL and drawn as a fan; otherwise a strip.
	void BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode,
		const Rect& theSrcRect, const SexyMatrix3& theTransform, bool theLinear, float theX, float theY, bool theCenter);
	// FillRect (0x68, 0x620ee0): no filter, SetTexture(0, null), a strip TL, TR, BL, BR at the rectangle - 0.5; with a
	// stack, M * (corner - 0.5) - 0.5. No clip.
	void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	// DrawLine (0x64, 0x620cd0): SetTexture(0, null), a line between the two ends (as floats, through M with a stack),
	// flushing the batch and drawn at once. GPUPolicy::mLineEndPixel: Twist draws two vertices, SAF 1.3 a strip of three
	// with the third at the end + mLineOffset.
	void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor,
		int theDrawMode);
	// DrawTriangle (0x6c, 0x621170): vertex colours (a vertex's own when non-zero, else the draw's), M with a stack,
	// SetTexture(0, null), one triangle into the batch.
	void DrawTriangle(const TriVertex& theP1, const TriVertex& theP2, const TriVertex& theP3, const Color& theColor,
		int theDrawMode);
	// DrawTrianglesTex (0x74, 0x622140, building in 0x61e310): the filter is the blend argument. One-piece images only
	// (max U and V at most 1): a cut image draws nothing, as in Twist (SAF clips per piece). SetTexture(0, piece 0),
	// the batch flushed, then the triangles drawn at once, 100 at a time, each as (v0, v2, v1): x + tx, y + ty through M
	// with a stack, no 0.5; colour as DrawTriangle; u, v times the image's max U, V.
	void DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles,
		const Color& theColor, int theDrawMode, float theTX, float theTY, bool theBlend);
	// FillPoly (0x80, 0x6214a0): nothing for fewer than 3 points; SetTexture(0, null); (point + t) as floats through M,
	// no 0.5; a fan into the batch, or with a clip rectangle clipped in software first.
	void FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor,
		int theDrawMode, int theTX, int theTY);

	// Software clipping (0x614f40, 0x614fd0, 0x6152c0, 0x6154f0, interpolating in 0x60f910, drawing in 0x61d540): SAF's
	// PointClipper, Sutherland-Hodgman against x < left, y < top, x >= right, y >= bottom in that order, each pass taking
	// (last, first) and then (i, i + 1). x, y, u, v interpolate linearly; z, rhw and specular are the first vertex's;
	// colour interpolates per channel only when the two differ, ROUNDED (SAF truncates). The result is drawn as a fan of
	// n - 2 when n > 2. There is no scissor on the 2D path, so the device's D3D scissor stays the game's.

	// The draws' own hooks (this+0x10 for a draw's vertices and clip rectangle, this+0x14 in the batch): not carried
	// unless Twist sets them (to be checked).

private:
	enum { BATCH_SIZE = 600 };
	typedef std::vector<GPU2DVertex> VertexList;

	GPUInterface* mDevice;
	GPUPolicy mPolicy;
	int mDepthTestMode, mDepthWriteMode, mAlphaRef;
	float mDepthZ[2];
	VertexList mBatch;
	int mDrawCalls, mPrimitives;
	int mFilter;  // -1 unknown, 0 point, 1 linear
	int mSceneCount;
	int mScreenWidth, mScreenHeight;
	std::vector<SexyMatrix3> mTransformStack;
	std::map<long long, unsigned long> mStates;  // (kind, stage, type) -> the value last set
	Image* mTextures[8];
	bool mTexturesKnown[8];

	void SetCachedState(int theKind, int theStage, int theType, unsigned long theValue);
	void ApplyDepthStates();  // 0x13c, 0x61fbf0
	float VertexZ() const { return (mDepthWriteMode != 0) ? mDepthZ[0] : mDepthZ[1]; }
	void AddPrimitives(GPUPrimitiveType theType, int theCount, const GPU2DVertex* theVertices);  // 0x624880
	void DrawNow(GPUPrimitiveType theType, int theCount, const GPU2DVertex* theVertices);       // 0x624710
	void DrawClipped(const Rect& theClipRect, const VertexList& theList);                        // 0x61d540
	void BltPieces(Image* theImage, const Rect& theSrcRect, unsigned long theColor, const SexyMatrix3* theMatrix,
		float theX, float theY, bool theCenter, const Rect* theClipRect);
};

}  // namespace Sexy

// Open points for the review:
// - Texture pieces. GPUInterfaceGL keeps one texture per image at its exact size; the original cuts an image wider than
//   512 and less than half as tall into 512-wide pieces, pads images under 8 pixels to 8, and computes u, v against the
//   piece's allocated size (0x61bd00, 0x61b970, 0x61c6e0). Proposal: the device carries the pieces. GPUInterface gets
//   PrepareTexture(MemoryImage*) returning the piece layout (columns, rows, each piece's allocated size, max U and V;
//   slot 0x44's job) and SetTexturePiece(stage, image, index); SetTexture(stage, image) stays piece 0. The upload
//   duplicates one column and row into a piece's padding (0x61b5a0), as SAF does.
// - Palettes: the original sets a texture palette per piece (D3D SetCurrentTexturePalette) when the image has one; the
//   policy accepts no P8, so it never does on the cards the port runs on. Not carried.
// - Images with flag 0x20 or 0x40 take one piece of their own size and 0x61c6e0 returns no texture for them, so Blt
//   draws them untextured: to be checked against what the game does with such images before it is carried.
// - The adapter (TwistD3D.h) calls ApplyDrawMode, Flush and the draw-mode setters, and GPUDisplay owns one GPU2D.

#endif
