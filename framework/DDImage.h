#ifndef __DDIMAGE_H__
#define __DDIMAGE_H__

// Portable DDImage: a MemoryImage that can be the screen (drawn through DDInterface's GPU path) and
// that can carry an HD pixel set used only for its GPU texture.
//
// HD images: a mod may provide "name@2x.png". The image keeps its logical mWidth/mHeight (so all
// game-side source rectangles keep working), mBits holds a 1x copy for CPU code, and mHDBits holds
// the full-resolution pixels uploaded to the texture. If game code modifies the 1x bits later
// (BitsChanged), the HD pixels are discarded and the texture follows mBits.

#include "MemoryImage.h"

namespace Sexy
{

class DDInterface;

class DDImage : public MemoryImage
{
public:
	DDInterface* mDDInterface;
	bool mIsScreen;
	bool mVideoMemory;
	bool mWantDDSurface;
	bool mDrawToBits;

	// The HD pixels (mHDBits, SetHDBits, ...) are part of MemoryImage.

private:
	void Init();
	void RemoveTexture();

public:
	DDImage();
	DDImage(DDInterface* theDDInterface);
	virtual ~DDImage();

	static bool Check3D(Image* theImage);
	static bool Check3D(DDImage* theImage);

	// DirectDraw-era API kept for game code; surfaces don't exist in the port.
	bool GenerateDDSurface() { return true; }
	void DeleteDDSurface() {}
	bool LockSurface() { return true; }
	bool UnlockSurface() { return true; }
	void SetVideoMemory(bool wantVideoMemory) { mVideoMemory = wantVideoMemory; }
	void SetSurface(void*) {}

	virtual bool PolyFill3D(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty, bool convex);
	virtual void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	virtual void DrawRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	virtual void ClearRect(const Rect& theRect);
	virtual void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);
	virtual void DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);
	virtual void FillScanLines(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode);
	virtual void FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight);
	virtual void Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	virtual void BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode);
	virtual void BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	virtual void StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch);
	virtual void BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend);
	virtual void BltTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect& theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend);
	virtual void BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	virtual void StretchBltMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch);

	virtual void SetBits(ulong* theBits, int theWidth, int theHeight, bool commitBits = true);
	virtual ulong* GetBits();
	virtual void BitsChanged();
	virtual void Create(int theWidth, int theHeight);
	virtual void PurgeBits();
	virtual void Delete3DBuffers();
	virtual void DeleteNativeData();
};

}

#endif
