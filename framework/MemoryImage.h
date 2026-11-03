#ifndef __MEMORYIMAGE_H__
#define __MEMORYIMAGE_H__

#include "Image.h"

#include <stdint.h>
#include <vector>

#define OPTIMIZE_SOFTWARE_DRAWING
#ifdef OPTIMIZE_SOFTWARE_DRAWING
extern bool gOptimizeSoftwareDrawing;
#endif

namespace Sexy
{

const ulong MEMORYCHECK_ID = 0x4BEEFADE;

class NativeDisplay;
class SexyAppBase;
class MemoryImage;

// port: set by a GPU display (GPUInterface::RecoverBits): GetBits on an image that has a texture but no bits fills
// them from its texture, as the original's GetBits does through its read-back; false (the bits stay zero) when the
// image has none. Null otherwise.
extern bool (*gMemoryImageRecoverBits)(MemoryImage* theImage);

class MemoryImage : public Image
{
public:
	ulong*					mBits;
	int						mBitsChangedCount;
	void*					mD3DData;
	DWORD					mD3DFlags;	// see D3DInterface.h for possible values

	ulong*					mColorTable;	
	uchar*					mColorIndices;
	
	bool					mForcedMode;
	bool					mHasTrans;
	bool					mHasAlpha;
	bool					mIsVolatile;
	bool					mPurgeBits;
	bool					mWantPal;
	
	ulong*					mNativeAlphaData;
	uchar*					mRLAlphaData;
	uchar*					mRLAdditiveData;	

	bool					mBitsChanged;
	SexyAppBase*			mApp;

	// port: HD pixels, an optional copy at mHDScale times the size that becomes the GPU texture (HD art
	// from mods, see DDImage.h). They are valid while mHDBitsChangedCount == mBitsChangedCount.
	// Software drawing keeps them valid when HD pixels are involved: Blt, BltF/BltRotated, StretchBlt and
	// the fills repeat each draw on the HD pixels (BeginHDMirror / EndHDMirror). Any other change to the
	// 1x bits leaves them stale, and the image is then drawn from its 1x bits as before.
	std::vector<uint32_t>	mHDBits;
	int						mHDWidth;
	int						mHDHeight;
	int						mHDScale;
	int						mHDBitsChangedCount;
	bool					mHDMirroring;
	ulong*					mHDNativeAlphaData;	// the blitters' alpha caches for the HD pixels
	uchar*					mHDRLAlphaData;
	uchar*					mHDRLAdditiveData;

private:
	void					Init();
	void					InitHD();
	void					FreeHDCaches();

public:
	virtual void*			GetNativeAlphaData(NativeDisplay *theNative);
	virtual uchar*			GetRLAlphaData();
	virtual uchar*			GetRLAdditiveData(NativeDisplay *theNative);
	virtual void			PurgeBits();
	virtual void			DeleteSWBuffers();
	virtual void			Delete3DBuffers();	
	virtual void			DeleteExtraBuffers();
	virtual void			ReInit();

	virtual void			BitsChanged();
	virtual void			CommitBits();
	
	virtual void			DeleteNativeData();	

	void					NormalBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);
	void					AdditiveBlt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor);

	void					NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	void					AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);

	void					NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);
	void					AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor);

	void					SlowStretchBlt(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect, const Color& theColor, int theDrawMode);
	void					FastStretchBlt(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect, const Color& theColor, int theDrawMode);
	// port: SlowStretchBlt's averaging with the later framework's store (Bejeweled Twist's 0x433ca0), which divides the
	// colour's weight by the new alpha (onto transparent bits it keeps the colour at full strength, with the averaged
	// alpha) where the 2004 one above darkens it by the alpha. DDInterface's software stretch draws with it.
	void					StretchCopy(Image* theImage, const Rect& theDestRect, const FRect& theSrcRect);
	bool					BltRotatedClipHelper(float &theX, float &theY, const Rect &theSrcRect, const Rect &theClipRect, double theRot, FRect &theDestRect, float theRotCenterX, float theRotCenterY);
	bool					StretchBltClipHelper(const Rect &theSrcRect, const Rect &theClipRect, const Rect &theDestRect, FRect &theSrcRectOut, Rect &theDestRectOut);
	bool					StretchBltMirrorClipHelper(const Rect &theSrcRect, const Rect &theClipRect, const Rect &theDestRect, FRect &theSrcRectOut, Rect &theDestRectOut);
	void					BltMatrixHelper(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, void *theSurface, int theBytePitch, int thePixelFormat, bool blend);
	void					BltTrianglesTexHelper(Image *theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect &theClipRect, const Color &theColor, int theDrawMode, void *theSurface, int theBytePitch, int thePixelFormat, float tx, float ty, bool blend);

	void					FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight);


public:
	MemoryImage();
	MemoryImage(SexyAppBase* theApp);
	MemoryImage(const MemoryImage& theMemoryImage);	
	virtual ~MemoryImage();

	virtual void			Clear();
	virtual void			SetBits(ulong* theBits, int theWidth, int theHeight, bool commitBits = true);
	virtual void			Create(int theWidth, int theHeight);
	virtual ulong*			GetBits();	
	
	virtual void			FillRect(const Rect& theRect, const Color& theColor, int theDrawMode);
	virtual void			ClearRect(const Rect& theRect);
	virtual void			DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);
	virtual void			DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode);

	virtual void			Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode);
	virtual void			BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect &theClipRect, const Color& theColor, int theDrawMode);
	virtual void			BltRotated(Image* theImage, float theX, float theY, const Rect &theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	virtual void			StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch);
	virtual void			BltMatrix(Image* theImage, float x, float y, const SexyMatrix3 &theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, bool blend);
	virtual void			BltTrianglesTex(Image *theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect& theClipRect, const Color &theColor, int theDrawMode, float tx, float ty, bool blend);

	virtual void			SetImageMode(bool hasTrans, bool hasAlpha);
	virtual void			SetVolatile(bool isVolatile);	

	virtual bool			Palletize();

	// port: HD pixels (see mHDBits)
	void					SetHDBits(const uint32_t* theBits, int theWidth, int theHeight, int theScale);
	bool					HasHDBits() const;
	bool					HasValidHDBits() const;		// not stale: they can be the texture
	bool					HasMirrorableHDBits() const;	// ... and exactly mHDScale times the size: they can be drawn on
	void					ClearHDBits();
	// Before a software draw from theSource (NULL for a fill): the scale to repeat the draw at on the HD
	// pixels, or 0. When it returns a scale, the caller draws at 1x, then repeats the draw on the HD
	// pixels, then calls EndHDMirror.
	int						BeginHDMirror(Image* theSource);
	void					EndHDMirror();
};

}

#endif //__MEMORYIMAGE_H__