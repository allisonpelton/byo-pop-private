// Portable DDImage. The screen image (mIsScreen) draws through DDInterface's GPU functions; every
// other DDImage is a plain MemoryImage for drawing purposes (the 1.3 DirectDraw surface paths are
// gone), plus optional HD pixels for its texture. See DDImage.h.

#include "DDImage.h"
#include "DDInterface.h"
#include "Graphics.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "TriVertex.h"

using namespace Sexy;

DDImage::DDImage(DDInterface* theDDInterface) :
	MemoryImage(theDDInterface->mApp)
{
	mDDInterface = theDDInterface;
	Init();
}

DDImage::DDImage() :
	MemoryImage(gSexyAppBase)
{
	mDDInterface = gSexyAppBase->mDDInterface;
	Init();
}

DDImage::~DDImage()
{
	RemoveTexture();
}

void DDImage::Init()
{
	mIsScreen = false;
	mVideoMemory = false;
	mWantDDSurface = false;
	mDrawToBits = false;
}

// Frees this image's texture. Images created before the display existed have no mDDInterface.
void DDImage::RemoveTexture()
{
	if (mD3DData == NULL)
		return;

	DDInterface* anInterface = (mDDInterface != NULL) ? mDDInterface : ((mApp != NULL) ? mApp->mDDInterface : NULL);
	if (anInterface != NULL)
		anInterface->RemoveImageData(this);
}

bool DDImage::Check3D(Image* theImage)
{
	DDImage* anImage = dynamic_cast<DDImage*>(theImage);
	return (anImage != NULL) && Check3D(anImage);
}

bool DDImage::Check3D(DDImage* theImage)
{
	return (theImage != NULL) && (theImage->mDDInterface != NULL) && theImage->mDDInterface->CheckDestination(theImage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDImage::SetBits(ulong* theBits, int theWidth, int theHeight, bool commitBits)
{
	ClearHDBits();

	// port: MemoryImage::SetBits ignores its own buffer; count it as a change so the texture follows
	if ((theBits == mBits) && (theBits != NULL))
	{
		BitsChanged();
		if (commitBits)
			CommitBits();
		return;
	}

	MemoryImage::SetBits(theBits, theWidth, theHeight, commitBits);
}

ulong* DDImage::GetBits()
{
	// (a GPUDisplay has no SDL renderer but is 3D)
	if (mIsScreen && (mDDInterface != NULL) && ((mDDInterface->mRenderer != NULL) || mDDInterface->mIs3D))
	{
		// port: the screen lives on the GPU, so it is read back on every request
		if (mBits == NULL)
		{
			mBits = new ulong[mWidth*mHeight + 1];
			mBits[mWidth*mHeight] = MEMORYCHECK_ID;
		}

		if (!mDDInterface->ReadScreen(mBits))
			memset(mBits, 0, mWidth*mHeight*sizeof(ulong));

		return mBits;
	}

	return MemoryImage::GetBits();
}

void DDImage::BitsChanged()
{
	MemoryImage::BitsChanged();

	// port: the 1x bits were modified, so the HD pixels no longer match them (unless the same draw is
	// being repeated on them, see MemoryImage::BeginHDMirror)
	if ((!mHDMirroring) && (!mHDBits.empty()) && (mHDBitsChangedCount != mBitsChangedCount))
		ClearHDBits();
}

void DDImage::Create(int theWidth, int theHeight)
{
	ClearHDBits();
	MemoryImage::Create(theWidth, theHeight);
}

// port: textures can't be read back, so the bits are always kept (mPurgeBits is still recorded).
void DDImage::PurgeBits()
{
	mPurgeBits = true;
}

void DDImage::Delete3DBuffers()
{
	RemoveTexture();
}

void DDImage::DeleteNativeData()
{
	MemoryImage::DeleteNativeData();
	RemoveTexture();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool DDImage::PolyFill3D(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty, bool convex)
{
	if (Check3D(this))
	{
		mDDInterface->FillPoly(theVertices, theNumVertices, theClipRect, theColor, theDrawMode, tx, ty);
		return true;
	}
	else
		return false;
}

void DDImage::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->FillRect(theRect, theColor, theDrawMode, NULL);
		return;
	}

	MemoryImage::FillRect(theRect, theColor, theDrawMode);
}

void DDImage::DrawRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	// four FillRect calls, through this image's FillRect
	MemoryImage::DrawRect(theRect, theColor, theDrawMode);
}

void DDImage::ClearRect(const Rect& theRect)
{
	if (Check3D(this))
	{
		mDDInterface->ClearRect(theRect);
		return;
	}

	MemoryImage::ClearRect(theRect);
}

void DDImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode, false);
		return;
	}

	MemoryImage::DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void DDImage::DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode, true);
		return;
	}

	MemoryImage::DrawLineAA(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void DDImage::FillScanLines(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->FillScanLines(theSpans, theSpanCount, theColor, theDrawMode);
		return;
	}

	MemoryImage::FillScanLines(theSpans, theSpanCount, theColor, theDrawMode);
}

void DDImage::FillScanLinesWithCoverage(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode, const BYTE* theCoverage, int theCoverX, int theCoverY, int theCoverWidth, int theCoverHeight)
{
	if (theSpanCount == 0) return;

	if (Check3D(this))
	{
		// As in 1.3: render the coverage into a temporary image and blit that
		int l = theSpans[0].mX, t = theSpans[0].mY;
		int r = l + theSpans[0].mWidth, b = t;
		for (int i = 1; i < theSpanCount; ++i)
		{
			l = std::min(theSpans[i].mX, l);
			r = std::max(theSpans[i].mX + theSpans[i].mWidth - 1, r);
			t = std::min(theSpans[i].mY, t);
			b = std::max(theSpans[i].mY, b);
		}
		for (int i = 0; i < theSpanCount; ++i)
		{
			theSpans[i].mX -= l;
			theSpans[i].mY -= t;
		}

		MemoryImage aTempImage(mApp);
		aTempImage.Create(r-l+1, b-t+1);
		aTempImage.FillScanLinesWithCoverage(theSpans, theSpanCount, theColor, theDrawMode, theCoverage, theCoverX - l, theCoverY - t, theCoverWidth, theCoverHeight);
		Blt(&aTempImage, l, t, Rect(0, 0, r-l+1, b-t+1), Color::White, theDrawMode);
		mDDInterface->RemoveImageData(&aTempImage);
		return;
	}

	MemoryImage::FillScanLinesWithCoverage(theSpans, theSpanCount, theColor, theDrawMode, theCoverage, theCoverX, theCoverY, theCoverWidth, theCoverHeight);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DDImage::Blt(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		mDDInterface->Blt(theImage, (float) theX, (float) theY, theSrcRect, theColor, theDrawMode, NULL);
		return;
	}

	MemoryImage::Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
}

void DDImage::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		mDDInterface->Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode, &theClipRect);
		return;
	}

	MemoryImage::BltF(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode);
}

void DDImage::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		mDDInterface->BltRotated(theImage, theX, theY, theSrcRect, &theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY);
		return;
	}

	MemoryImage::BltRotated(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY);
}

void DDImage::StretchBlt(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		FRect aDestRect(theDestRect.mX, theDestRect.mY, theDestRect.mWidth, theDestRect.mHeight);
		mDDInterface->StretchBlt(theImage, aDestRect, theSrcRect, &theClipRect, theColor, theDrawMode, fastStretch);
		return;
	}

	MemoryImage::StretchBlt(theImage, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, fastStretch);
}

void DDImage::BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		mDDInterface->BltTransformed(theImage, &theClipRect, theColor, theDrawMode, theSrcRect, theMatrix, blend, x, y, true);
		return;
	}

	MemoryImage::BltMatrix(theImage, x, y, theMatrix, theClipRect, theColor, theDrawMode, theSrcRect, blend);
}

void DDImage::BltTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect& theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend)
{
	theTexture->mDrawn = true;

	if (Check3D(this))
	{
		mDDInterface->DrawTrianglesTex(theTexture, theVertices, theNumTriangles, &theClipRect, theColor, theDrawMode, tx, ty, blend);
		return;
	}

	MemoryImage::BltTrianglesTex(theTexture, theVertices, theNumTriangles, theClipRect, theColor, theDrawMode, tx, ty, blend);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// MemoryImage has no mirrored blits (1.3 drew them on the DirectDraw surface), so offscreen images
// mirror the source rectangle into a temporary image and use the normal software blitters.
static bool CreateMirroredImage(Image* theImage, const Rect& theSrcRect, MemoryImage& theMirrorImage)
{
	MemoryImage* aSrcImage = dynamic_cast<MemoryImage*>(theImage);
	if ((aSrcImage == NULL) || (theSrcRect.mWidth <= 0) || (theSrcRect.mHeight <= 0))
		return false;

	ulong* aSrcBits = aSrcImage->GetBits();
	if (aSrcBits == NULL)
		return false;

	theMirrorImage.Create(theSrcRect.mWidth, theSrcRect.mHeight);
	ulong* aDestBits = theMirrorImage.GetBits();

	for (int y = 0; y < theSrcRect.mHeight; y++)
	{
		ulong* aSrcPixels = aSrcBits + (theSrcRect.mY + y) * aSrcImage->mWidth + theSrcRect.mX + theSrcRect.mWidth - 1;
		ulong* aDestPixels = aDestBits + y * theSrcRect.mWidth;

		for (int x = 0; x < theSrcRect.mWidth; x++)
			*(aDestPixels++) = *(aSrcPixels--);
	}

	theMirrorImage.BitsChanged();
	theMirrorImage.CommitBits();
	return true;
}

void DDImage::BltMirror(Image* theImage, int theX, int theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		mDDInterface->Blt(theImage, (float) theX, (float) theY, theSrcRect, theColor, theDrawMode, NULL, true);
		return;
	}

	MemoryImage aMirrorImage(mApp);
	if (CreateMirroredImage(theImage, theSrcRect, aMirrorImage))
		MemoryImage::Blt(&aMirrorImage, theX, theY, Rect(0, 0, theSrcRect.mWidth, theSrcRect.mHeight), theColor, theDrawMode);
}

void DDImage::StretchBltMirror(Image* theImage, const Rect& theDestRect, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, bool fastStretch)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		FRect aDestRect(theDestRect.mX, theDestRect.mY, theDestRect.mWidth, theDestRect.mHeight);
		mDDInterface->StretchBlt(theImage, aDestRect, theSrcRect, &theClipRect, theColor, theDrawMode, fastStretch, true);
		return;
	}

	MemoryImage aMirrorImage(mApp);
	if (CreateMirroredImage(theImage, theSrcRect, aMirrorImage))
		MemoryImage::StretchBlt(&aMirrorImage, theDestRect, Rect(0, 0, theSrcRect.mWidth, theSrcRect.mHeight), theClipRect, theColor, theDrawMode, fastStretch);
}
