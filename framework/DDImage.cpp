// Portable DDImage. The screen image (mIsScreen) draws through DDInterface's GPU functions; every
// other DDImage is a plain MemoryImage for drawing purposes (the 1.3 DirectDraw surface paths are
// gone), plus optional HD pixels for its texture. See DDImage.h.

#include "DDImage.h"
#include "DDInterface.h"
#include "Graphics.h"
#include "SexyAppBase.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include <algorithm>
#include <cmath>

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
	// (a GPUDisplay has no SDL renderer but is 3D; a software screen's bits are the screen itself)
	if (mIsScreen && (mDDInterface != NULL) && (!mDDInterface->ScreenOnCPU()) && ((mDDInterface->mRenderer != NULL) || mDDInterface->mIs3D))
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

// port: textures can't be read back, so PurgeBits drops nothing itself (mPurgeBits is still recorded); a picture loaded
// from a file loses its pixels after its upload all the same and makes them again from the file (MemoryImage::DropBits).
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

void DDImage::NoteScreenDrawnTo(const Rect& theRect)
{
	Rect aRect = theRect.Intersection(Rect(0, 0, mWidth, mHeight));
	if ((aRect.mWidth > 0) && (aRect.mHeight > 0))
		mDDInterface->ScreenDrawn(aRect);
}

void DDImage::NoteScreenDrawn(float theMinX, float theMinY, float theMaxX, float theMaxY, const Rect& theClipRect)
{
	if ((!mIsScreen) || (mDDInterface == NULL) || !(theMinX <= theMaxX) || !(theMinY <= theMaxY))
		return;
	// clamped in float, so a bound past an int's range never reaches the conversion
	float aLeft = std::max(std::floor(theMinX) - 1.0f, (float) theClipRect.mX);
	float aTop = std::max(std::floor(theMinY) - 1.0f, (float) theClipRect.mY);
	float aRight = std::min(std::ceil(theMaxX) + 2.0f, (float) (theClipRect.mX + theClipRect.mWidth));
	float aBottom = std::min(std::ceil(theMaxY) + 2.0f, (float) (theClipRect.mY + theClipRect.mHeight));
	if ((aRight > aLeft) && (aBottom > aTop))
		NoteScreenDrawnTo(Rect((int) aLeft, (int) aTop, (int) aRight - (int) aLeft, (int) aBottom - (int) aTop));
}

// the bounds of a matrix blit's corners, as MemoryImage::BltMatrixHelper places them
static void MatrixBltBounds(float x, float y, const SexyMatrix3& theMatrix, const Rect& theSrcRect, float& theMinX, float& theMinY, float& theMaxX, float& theMaxY)
{
	theMinX = 1e30f;
	theMinY = 1e30f;
	theMaxX = -1e30f;
	theMaxY = -1e30f;
	for (int i = 0; i < 4; i++)
	{
		SexyVector3 v((i & 1) ? theSrcRect.mWidth / 2.0f : -theSrcRect.mWidth / 2.0f, (i & 2) ? theSrcRect.mHeight / 2.0f : -theSrcRect.mHeight / 2.0f, 1);
		v = theMatrix * v;
		theMinX = std::min(theMinX, v.x + x - 0.5f);
		theMaxX = std::max(theMaxX, v.x + x - 0.5f);
		theMinY = std::min(theMinY, v.y + y - 0.5f);
		theMaxY = std::max(theMaxY, v.y + y - 0.5f);
	}
}

// the bounds of a triangle blit's vertices
static void TrianglesBounds(const TriVertex theVertices[][3], int theNumTriangles, float tx, float ty, float& theMinX, float& theMinY, float& theMaxX, float& theMaxY)
{
	theMinX = 1e30f;
	theMinY = 1e30f;
	theMaxX = -1e30f;
	theMaxY = -1e30f;
	for (int i = 0; i < theNumTriangles; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			theMinX = std::min(theMinX, theVertices[i][j].x + tx);
			theMaxX = std::max(theMaxX, theVertices[i][j].x + tx);
			theMinY = std::min(theMinY, theVertices[i][j].y + ty);
			theMaxY = std::max(theMaxY, theVertices[i][j].y + ty);
		}
	}
}

// the bounds of scan lines' spans
static Rect SpansBounds(const Span* theSpans, int theSpanCount)
{
	if (theSpanCount <= 0)
		return Rect(0, 0, 0, 0);
	int aLeft = theSpans[0].mX, aTop = theSpans[0].mY;
	int aRight = aLeft + theSpans[0].mWidth, aBottom = aTop + 1;
	for (int i = 1; i < theSpanCount; ++i)
	{
		aLeft = std::min(theSpans[i].mX, aLeft);
		aRight = std::max(theSpans[i].mX + theSpans[i].mWidth, aRight);
		aTop = std::min(theSpans[i].mY, aTop);
		aBottom = std::max(theSpans[i].mY + 1, aBottom);
	}
	return Rect(aLeft, aTop, aRight - aLeft, aBottom - aTop);
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

	NoteScreenDrawn(theRect);
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

	NoteScreenDrawn(theRect);
	MemoryImage::ClearRect(theRect);
}

void DDImage::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode, false);
		return;
	}

	NoteScreenDrawn((float) std::min(theStartX, theEndX), (float) std::min(theStartY, theEndY), (float) std::max(theStartX, theEndX), (float) std::max(theStartY, theEndY), Rect(0, 0, mWidth, mHeight));
	MemoryImage::DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void DDImage::DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode, true);
		return;
	}

	NoteScreenDrawn((float) std::min(theStartX, theEndX), (float) std::min(theStartY, theEndY), (float) std::max(theStartX, theEndX), (float) std::max(theStartY, theEndY), Rect(0, 0, mWidth, mHeight));
	MemoryImage::DrawLineAA(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void DDImage::FillScanLines(Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode)
{
	if (Check3D(this))
	{
		mDDInterface->FillScanLines(theSpans, theSpanCount, theColor, theDrawMode);
		return;
	}

	NoteScreenDrawn(SpansBounds(theSpans, theSpanCount));
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

	NoteScreenDrawn(SpansBounds(theSpans, theSpanCount));
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

	NoteScreenDrawn(Rect(theX, theY, theSrcRect.mWidth, theSrcRect.mHeight));
	MemoryImage::Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
}

void DDImage::BltF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		// the software transforms: a software screen's BltF is its BltRotated at no angle (Twist's 0x412fd0)
		if (mDDInterface->mSoftwareTransforms && (dynamic_cast<MemoryImage*>(theImage) != NULL))
		{
			BltRotated(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, 0, 0, 0);
			return;
		}
		mDDInterface->Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode, &theClipRect);
		return;
	}

	NoteScreenDrawn(theX, theY, theX + theSrcRect.mWidth, theY + theSrcRect.mHeight, theClipRect);
	MemoryImage::BltF(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode);
}

// port: DDInterface::mBltRotated12, the 1.2 framework's software rotation: the branch of 1.2's DDImage::BltRotated that draws
// straight into the locked surface when 3D is off. Big Money's is 0x44ed40, Hammer Heads' its DDImage slot 9 at 0x4a6570 (the
// same function: 16 665 and 16 558 bytes with the same 712 multiplies and shifts, the stack frame and registers chosen
// differently). Bookworm's DDImage slot 8 (0x45acb0) is a variant with fewer branches, not compared. Of the branch's
// variants only the one Big Money draws is here: a 32-bit destination, white and normal, from 32-bit pixels or a palette.
// The others (a colour, additive, a 16-bit surface) and 1.2's MemoryImage::BltRotated (Big Money's 0x43bbf0, Hammer Heads'
// MemoryImage slot 9 at 0x493da0) keep 1.3's MemoryImage::BltRotated.

// 1.2's BltRotatedClipHelper (Big Money's 0x43b720, Hammer Heads' 0x4938d0, both 680 bytes with the same constants): the four
// corners of theSrcRect turned about the centre, in single precision and in its order, and their bounding rectangle within
// theClipRect
static bool BltRotated12Bounds(float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, double theRot,
	float theRotCenterX, float theRotCenterY, FRect& theDestRect)
{
	float aCos = (float) cos(theRot);
	float aSin = (float) sin(theRot);
	float aMinX = 1e+07f;
	float aMaxX = -1e+07f;
	float aMinY = 1e+07f;
	float aMaxY = -1e+07f;
	float aLeft = -theRotCenterX;
	float aTop = -theRotCenterY;
	float aRight = (float) theSrcRect.mWidth - theRotCenterX;
	float aBottom = (float) theSrcRect.mHeight - theRotCenterY;
	// the corners in the original's order: top left, top right, bottom right, bottom left
	const float aCornerX[4] = { aLeft, aRight, aRight, aLeft };
	const float aCornerY[4] = { aTop, aTop, aBottom, aBottom };
	for (int i = 0; i < 4; i++)
	{
		float aX = aCornerX[i] * aCos + aSin * aCornerY[i] + theRotCenterX + theX;
		float aY = ((aCornerY[i] * aCos + theRotCenterY) - aCornerX[i] * aSin) + theY;
		if (aX < aMinX)
			aMinX = aX;
		if (aMaxX < aX)
			aMaxX = aX;
		if (aY < aMinY)
			aMinY = aY;
		if (aMaxY < aY)
			aMaxY = aY;
	}
	FRect aBounds(aMinX, aMinY, aMaxX - aMinX, aMaxY - aMinY);
	theDestRect = aBounds.Intersection(FRect(theClipRect.mX, theClipRect.mY, theClipRect.mWidth, theClipRect.mHeight));
	return (theDestRect.mWidth > 0) && (theDestRect.mHeight > 0);
}

// A source pixel of BltRotated12: 32-bit pixels, or a palette and its indices (Palletize)
struct BltRotated12Source
{
	const ulong* mBits;
	const uchar* mIndices;
	const ulong* mPalette;
	ulong At(int theIndex) const { return (mBits != NULL) ? mBits[theIndex] : mPalette[mIndices[theIndex]]; }
};

// The branch's loop for a 32-bit surface, white and normal (its 32-bit and its palettized source alike): each row's start
// in the source in 16.16, the row cut to the columns whose four neighbours lie inside the source, and each pixel the four
// neighbours weighted by position and alpha, added to the surface's pixel scaled by what is left. The original takes the
// channels' shifts from its surface; the port's is always 8:8:8 at 16, 8 and 0, and keeps the byte above them (the
// original's surface has none).
static void BltRotated12Pixels(MemoryImage* theDest, const BltRotated12Source& theSource, int theSourceWidth, float theX,
	float theY, const Rect& theSrcRect, const FRect& theDestRect, double theRot, float theRotCenterX, float theRotCenterY)
{
	double aCos = cos(theRot);
	double aSin = sin(theRot);
	int aCosLong = (int) (aCos * 65536.0);
	int aSinLong = (int) (aSin * 65536.0);
	int aDestX = (int) theDestRect.mX;
	int aDestY = (int) theDestRect.mY;
	int aLastColumn = (int) (theDestRect.mWidth - 1.0);
	ulong* aDestRow = theDest->GetBits() + aDestY * theDest->mWidth + aDestX;
	int aSrcWidth = theSrcRect.mWidth;
	int aSrcHeight = theSrcRect.mHeight;
	int aSrcOrigin = theSrcRect.mY * theSourceWidth + theSrcRect.mX;

	double aBaseX = (double) aDestX - theRotCenterX - theX;
	double aCosBaseX = aCos * aBaseX;
	double aSinBaseX = aSin * aBaseX;
	for (int aRow = 0; (double) aRow < theDestRect.mHeight; aRow++, aDestRow += theDest->mWidth)
	{
		double aBaseY = (double) (aDestY + aRow) - theRotCenterY - theY;
		int aU = (int) (((aCosBaseX - aSin * aBaseY) + theRotCenterX) * 65536.0);
		int aV = (int) (((aCos * aBaseY + theRotCenterY) + aSinBaseX) * 65536.0);
		int aLeft = 0;
		int aRight = aLastColumn;

		if (aCosLong < 0)
		{
			if ((aU >> 16) < 0)
				continue;
			int anEdge = ((aSrcWidth * 0x10000 - aU) - 0x27fff) / aCosLong;
			if (anEdge >= 0)
				aLeft = anEdge;
			anEdge = (0x7fff - aU) / aCosLong;
			if (anEdge < aRight)
				aRight = anEdge;
		}
		else if (aCosLong == 0)
		{
			if (!((aU >= 0) && (aU < aSrcWidth * 0x10000 - 0x10000)))
				continue;
		}
		else
		{
			if (!((aU >> 16) < aSrcWidth - 1))
				continue;
			int anEdge = (0x10000 - aU) / aCosLong;
			if (anEdge > 0)
				aLeft = anEdge;
			anEdge = ((aSrcWidth * 0x10000 - aU) - 0x17fff) / aCosLong;
			if (anEdge < aRight)
				aRight = anEdge;
		}

		if (aSinLong < 0)
		{
			if ((aV >> 16) < 0)
				continue;
			int anEdge = ((aSrcHeight * 0x10000 - aV) - 0x27fff) / aSinLong;
			if (aLeft < anEdge)
				aLeft = anEdge;
			anEdge = -(aV / aSinLong);
			if (anEdge < aRight)
				aRight = anEdge;
		}
		else if (aSinLong == 0)
		{
			if (!((aV >= 0) && (aV < aSrcHeight * 0x10000 - 0x10000)))
				continue;
		}
		else
		{
			if (!((aV >> 16) < aSrcHeight - 1))
				continue;
			int anEdge = (0x10000 - aV) / aSinLong;
			if (aLeft < anEdge)
				aLeft = anEdge;
			anEdge = ((aSrcHeight * 0x10000 - aV) - 0x17fff) / aSinLong;
			if (anEdge < aRight)
				aRight = anEdge;
		}

		aU += aLeft * aCosLong;
		aV += aLeft * aSinLong;
		ulong* aDest = aDestRow + aLeft;
		for (int aColumn = aLeft; aColumn <= aRight; aColumn++)
		{
			int anIndex = aSrcOrigin + (aV >> 16) * theSourceWidth + (aU >> 16);
			ulong aTopLeft = theSource.At(anIndex);
			ulong aTopRight = theSource.At(anIndex + 1);
			ulong aBottomLeft = theSource.At(anIndex + theSourceWidth);
			ulong aBottomRight = theSource.At(anIndex + theSourceWidth + 1);
			unsigned int aUFactor = (aU & 0xfffe) + 1;
			unsigned int aVFactor = (aV & 0xfffe) + 1;
			unsigned int aBottomRightWeight = (((aVFactor * aUFactor) >> 16) * (aBottomRight >> 24)) >> 16;
			unsigned int aTopRightWeight = ((((0x10000 - aVFactor) * aUFactor) >> 16) * (aTopRight >> 24)) >> 16;
			unsigned int aBottomLeftWeight = ((((0x10000 - aUFactor) * aVFactor) >> 16) * (aBottomLeft >> 24)) >> 16;
			unsigned int aTopLeftWeight = ((((0x10000 - aUFactor) * (0x10000 - aVFactor)) >> 16) * (aTopLeft >> 24)) >> 16;
			unsigned int aRest = 0x100 - aBottomRightWeight - aBottomLeftWeight - aTopRightWeight - aTopLeftWeight;
			ulong aBelow = *aDest;
			ulong aGreen = ((((aBottomRight & 0xff00) * aBottomRightWeight + (aBottomLeft & 0xff00) * aBottomLeftWeight +
				(aTopRight & 0xff00) * aTopRightWeight + (aTopLeft & 0xff00) * aTopLeftWeight) >> 16) << 8) +
				(((aBelow & 0xff00) * aRest >> 8) & 0xff00);
			ulong aBlue = (((aBottomRight & 0xff) * aBottomRightWeight + (aBottomLeft & 0xff) * aBottomLeftWeight +
				(aTopRight & 0xff) * aTopRightWeight + (aTopLeft & 0xff) * aTopLeftWeight) >> 8) +
				(((aBelow & 0xff) * aRest >> 8) & 0xff);
			ulong aRed = ((((aBottomRight & 0xff0000) * aBottomRightWeight + (aBottomLeft & 0xff0000) * aBottomLeftWeight +
				(aTopRight & 0xff0000) * aTopRightWeight + (aTopLeft & 0xff0000) * aTopLeftWeight) >> 24) << 16) +
				(((aBelow & 0xff0000) * aRest >> 8) & 0xff0000);
			*aDest++ = aGreen | aBlue | aRed | (aBelow & 0xff000000);
			aU += aCosLong;
			aV += aSinLong;
		}
	}
}

// Draws as 1.2's branch where it applies (the caller has found no 3D): a destination without alpha or transparency, a
// MemoryImage source drawn white in normal mode, and, the port's own, no HD pixels on either side (the framework mirrors
// those). Returns false where it does not, and otherwise the bounds it drew within (empty when clipped away)
static bool BltRotated12(MemoryImage* theDest, Image* theImage, float theX, float theY, const Rect& theSrcRect,
	const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY,
	FRect& theBounds)
{
	MemoryImage* aSource = dynamic_cast<MemoryImage*>(theImage);
	if ((aSource == NULL) || theDest->mHasAlpha || theDest->mHasTrans || (theDrawMode != Graphics::DRAWMODE_NORMAL) ||
		!(theColor == Color::White) || theDest->HasHDBits() || aSource->HasHDBits())
		return false;
	if (!BltRotated12Bounds(theX, theY, theSrcRect, theClipRect, theRot, theRotCenterX, theRotCenterY, theBounds))
	{
		theBounds = FRect(0, 0, 0, 0);
		return true;
	}

	// pixels dropped after an upload come back first (a palette as a palette); a palette is read as it is, as 1.2 reads
	// mColorTable, since GetBits and ReadBits would turn it into 32-bit pixels for good
	aSource->RestoreBits();
	BltRotated12Source aPixels;
	if (aSource->mColorTable != NULL)
	{
		aPixels.mBits = NULL;
		aPixels.mIndices = aSource->mColorIndices;
		aPixels.mPalette = aSource->mColorTable;
	}
	else
	{
		aPixels.mBits = aSource->ReadBits();
		aPixels.mIndices = NULL;
		aPixels.mPalette = NULL;
	}
	BltRotated12Pixels(theDest, aPixels, aSource->mWidth, theX, theY, theSrcRect, theBounds, theRot, theRotCenterX, theRotCenterY);
	theDest->BitsChanged();
	return true;
}

void DDImage::BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect& theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		if (mDDInterface->mSoftwareTransforms && (dynamic_cast<MemoryImage*>(theImage) != NULL))
		{
			FRect aBounds;
			float aX = theX, aY = theY;
			if (BltRotatedClipHelper(aX, aY, theSrcRect, theClipRect, theRot, aBounds, theRotCenterX, theRotCenterY))
				BltSoftwareTransformed(aBounds.mX, aBounds.mY, aBounds.mX + aBounds.mWidth, aBounds.mY + aBounds.mHeight, theClipRect, theDrawMode,
					[&](MemoryImage& theCopy, int theDX, int theDY, const Rect& theCopyClip)
					{ theCopy.BltRotated(theImage, theX + theDX, theY + theDY, theSrcRect, theCopyClip, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY); });
			return;
		}
		mDDInterface->BltRotated(theImage, theX, theY, theSrcRect, &theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY);
		return;
	}

	// the 1.2 framework's software rotation (DDInterface::mBltRotated12), which 1.2 takes only with 3D off
	FRect aBounds12;
	if ((mDDInterface != NULL) && mDDInterface->mBltRotated12 && (!mDDInterface->mIs3D) &&
		BltRotated12(this, theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY, aBounds12))
	{
		if ((aBounds12.mWidth > 0) && (aBounds12.mHeight > 0))
			NoteScreenDrawn(aBounds12.mX, aBounds12.mY, aBounds12.mX + aBounds12.mWidth, aBounds12.mY + aBounds12.mHeight, theClipRect);
		return;
	}

	if (mIsScreen && (mDDInterface != NULL))
	{
		FRect aBounds;
		float aX = theX, aY = theY;
		if (BltRotatedClipHelper(aX, aY, theSrcRect, theClipRect, theRot, aBounds, theRotCenterX, theRotCenterY))
			NoteScreenDrawn(aBounds.mX, aBounds.mY, aBounds.mX + aBounds.mWidth, aBounds.mY + aBounds.mHeight, theClipRect);
	}
	MemoryImage::BltRotated(theImage, theX, theY, theSrcRect, theClipRect, theColor, theDrawMode, theRot, theRotCenterX, theRotCenterY);
}

// port: DDInterface::mSoftwareTransforms. theDraw draws, moved by (theDX, theDY) and clipped to the area, into the
// top-left of a copy, an area that covers the bounds (rounded out by a pixel) within theClipRect: transparent for a
// normal draw, opaque black for an additive draw, as MemoryImage's additive code adds onto its destination's colours
// and keeps its alpha. The area is then blitted at its place in the draw's mode.
void DDImage::BltSoftwareTransformed(float theMinX, float theMinY, float theMaxX, float theMaxY, const Rect& theClipRect, int theDrawMode, const std::function<void(MemoryImage& theCopy, int theDX, int theDY, const Rect& theCopyClip)>& theDraw)
{
	if (!(theMinX <= theMaxX) || !(theMinY <= theMaxY))
		return;
	// clamped and tested in float, so a bound past an int's range never reaches the conversion
	float aLeftF = std::max(std::floor(theMinX) - 1.0f, (float) theClipRect.mX);
	float aTopF = std::max(std::floor(theMinY) - 1.0f, (float) theClipRect.mY);
	float aRightF = std::min(std::ceil(theMaxX) + 2.0f, (float) (theClipRect.mX + theClipRect.mWidth));
	float aBottomF = std::min(std::ceil(theMaxY) + 2.0f, (float) (theClipRect.mY + theClipRect.mHeight));
	if (!(aRightF > aLeftF) || !(aBottomF > aTopF))
		return;
	int aLeft = (int) aLeftF;
	int aTop = (int) aTopF;
	int aRight = (int) aRightF;
	int aBottom = (int) aBottomF;

	int aWidth = aRight - aLeft;
	int aHeight = aBottom - aTop;
	MemoryImage* aCopy = mDDInterface->GetTransformCopy(aWidth, aHeight);
	ulong* aBits = aCopy->GetBits();
	// one more column and row than drawn are cleared too (where the copy has them), so a filtered blit never takes in
	// an earlier draw's pixels
	int aClearWidth = std::min(aWidth + 1, aCopy->mWidth);
	int aClearHeight = std::min(aHeight + 1, aCopy->mHeight);
	for (int y = 0; y < aClearHeight; y++)
		std::fill(aBits + y * aCopy->mWidth, aBits + y * aCopy->mWidth + aClearWidth, (theDrawMode == Graphics::DRAWMODE_ADDITIVE) ? 0xFF000000 : 0);
	theDraw(*aCopy, -aLeft, -aTop, Rect(0, 0, aWidth, aHeight));
	aCopy->ClearHDBits();  // a source with HD pixels gives the copy some, outside the pool's bound
	aCopy->BitsChanged();
	mDDInterface->Blt(aCopy, (float) aLeft, (float) aTop, Rect(0, 0, aWidth, aHeight), Color::White, theDrawMode, &theClipRect);
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

	// port: with the software stretch (DDInterface::mSoftwareStretch), a stretch into a picture without alpha or
	// transparency is drawn as the original's software DDImage::StretchBlt (0x41a3e0) draws it: the source stretched to
	// the destination's size, then blitted with the draw's colour, mode and clip. MemoryImage::StretchBlt keeps neither
	// colour nor mode, so an additive stretch drew its black background (Bejeweled Twist's picture of the board at a
	// challenge's end)
	NoteScreenDrawn(theDestRect.Intersection(theClipRect));
	MemoryImage* aSource = dynamic_cast<MemoryImage*>(theImage);
	if ((mDDInterface != NULL) && mDDInterface->mSoftwareStretch && (aSource != NULL) && !mHasAlpha && !mHasTrans &&
		(theDestRect.mWidth > 0) && (theDestRect.mHeight > 0))
	{
		MemoryImage* aCopy = mDDInterface->GetStretchCopy(aSource, theSrcRect, theDestRect.mWidth, theDestRect.mHeight, fastStretch);
		if (aCopy != NULL)
		{
			Rect aDest = theDestRect.Intersection(theClipRect);
			if ((aDest.mWidth > 0) && (aDest.mHeight > 0))
				MemoryImage::Blt(aCopy, aDest.mX, aDest.mY, Rect(aDest.mX - theDestRect.mX, aDest.mY - theDestRect.mY, aDest.mWidth, aDest.mHeight), theColor, theDrawMode);
			return;
		}
	}

	MemoryImage::StretchBlt(theImage, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, fastStretch);
}

void DDImage::BltMatrix(Image* theImage, float x, float y, const SexyMatrix3& theMatrix, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, bool blend)
{
	theImage->mDrawn = true;

	if (Check3D(this))
	{
		if (mDDInterface->mSoftwareTransforms && (theDrawMode == Graphics::DRAWMODE_NORMAL) && (dynamic_cast<MemoryImage*>(theImage) != NULL))
		{
			float aMinX, aMinY, aMaxX, aMaxY;
			MatrixBltBounds(x, y, theMatrix, theSrcRect, aMinX, aMinY, aMaxX, aMaxY);
			BltSoftwareTransformed(aMinX, aMinY, aMaxX, aMaxY, theClipRect, theDrawMode,
				[&](MemoryImage& theCopy, int theDX, int theDY, const Rect& theCopyClip)
				{ theCopy.BltMatrix(theImage, x + theDX, y + theDY, theMatrix, theCopyClip, theColor, theDrawMode, theSrcRect, blend); });
			return;
		}
		mDDInterface->BltTransformed(theImage, &theClipRect, theColor, theDrawMode, theSrcRect, theMatrix, blend, x, y, true);
		return;
	}

	if (mIsScreen && (mDDInterface != NULL))
	{
		float aMinX, aMinY, aMaxX, aMaxY;
		MatrixBltBounds(x, y, theMatrix, theSrcRect, aMinX, aMinY, aMaxX, aMaxY);
		NoteScreenDrawn(aMinX, aMinY, aMaxX, aMaxY, theClipRect);
	}
	MemoryImage::BltMatrix(theImage, x, y, theMatrix, theClipRect, theColor, theDrawMode, theSrcRect, blend);
}

void DDImage::BltTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect& theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend)
{
	theTexture->mDrawn = true;

	if (Check3D(this))
	{
		if (mDDInterface->mSoftwareTransforms && (theDrawMode == Graphics::DRAWMODE_NORMAL) && (dynamic_cast<MemoryImage*>(theTexture) != NULL))
		{
			float aMinX, aMinY, aMaxX, aMaxY;
			TrianglesBounds(theVertices, theNumTriangles, tx, ty, aMinX, aMinY, aMaxX, aMaxY);
			BltSoftwareTransformed(aMinX, aMinY, aMaxX, aMaxY, theClipRect, theDrawMode,
				[&](MemoryImage& theCopy, int theDX, int theDY, const Rect& theCopyClip)
				{ theCopy.BltTrianglesTex(theTexture, theVertices, theNumTriangles, theCopyClip, theColor, theDrawMode, tx + theDX, ty + theDY, blend); });
			return;
		}
		mDDInterface->DrawTrianglesTex(theTexture, theVertices, theNumTriangles, &theClipRect, theColor, theDrawMode, tx, ty, blend);
		return;
	}

	if (mIsScreen && (mDDInterface != NULL))
	{
		float aMinX, aMinY, aMaxX, aMaxY;
		TrianglesBounds(theVertices, theNumTriangles, tx, ty, aMinX, aMinY, aMaxX, aMaxY);
		NoteScreenDrawn(aMinX, aMinY, aMaxX, aMaxY, theClipRect);
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

	ulong* aSrcBits = aSrcImage->ReadBits();
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

	NoteScreenDrawn(Rect(theX, theY, theSrcRect.mWidth, theSrcRect.mHeight));
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

	NoteScreenDrawn(theDestRect.Intersection(theClipRect));
	MemoryImage aMirrorImage(mApp);
	if (CreateMirroredImage(theImage, theSrcRect, aMirrorImage))
		MemoryImage::StretchBlt(&aMirrorImage, theDestRect, Rect(0, 0, theSrcRect.mWidth, theSrcRect.mHeight), theClipRect, theColor, theDrawMode, fastStretch);
}
