#include "DXDrawContext.h"

#include "Surface.h"

#include <cmath>

namespace engine
{

namespace
{

// A part of a surface as the blitters work on it (the lock structure's +0x00, +0x04, +0x10 and +0x14): its size, the
// pixels from one row to the next, and its first pixel
struct PixelRect
{
	int mWidth;
	int mHeight;
	int mPitch;
	uint32_t* mBits;
};

// The surface's slot 0x04: a rectangle of it locked
PixelRect lockRect(Surface* theSurface, const RectI& theRect)
{
	PixelRect aLock;
	aLock.mWidth = theRect.right - theRect.left;
	aLock.mHeight = theRect.bottom - theRect.top;
	aLock.mPitch = theSurface->getWidth();
	aLock.mBits = theSurface->getPixels() + theRect.top * aLock.mPitch + theRect.left;
	return aLock;
}

// And its slot 0x08: the whole surface
PixelRect lockAll(Surface* theSurface)
{
	return lockRect(theSurface, RectI(0, 0, theSurface->getWidth(), theSurface->getHeight()));
}

bool isEmpty(const RectI& theRect)
{
	return theRect.left >= theRect.right || theRect.top >= theRect.bottom;
}

// 0x434bf0 (feedingfrenzy2)
RectI intersection(const RectI& a, const RectI& b)
{
	return RectI(maxOf(a.left, b.left), maxOf(a.top, b.top), minOf(a.right, b.right), minOf(a.bottom, b.bottom));
}

// A value times 255 as a byte, the x87 truncating the product it computes at double precision (_ftol2, 0x4c3b16
// (feedingfrenzy2)); a value above 1 keeps the low byte
uint32_t toByte(float theValue)
{
	return (uint32_t)(int)((double)theValue * 255.0) & 0xff;
}

// The state's colour as the blitters take it, 0xAARRGGBB
uint32_t colorBytes(const Color& theColor)
{
	return toByte(theColor.a) << 24 | toByte(theColor.r) << 16 | toByte(theColor.g) << 8 | toByte(theColor.b);
}

// 0x48ca50 (feedingfrenzy2): an opacity byte as the blitters scale by it, 255 made 0x101 so that (c * 0x101) >> 8
// leaves c whole
int alphaScale(uint32_t theAlpha)
{
	return theAlpha == 0xff ? 0x101 : (int)theAlpha;
}

// The blend every blitter ends with (pmullw, paddw 0x80, the high byte added back, paddusb): each byte of the target
// times the inverse alpha, divided by 255 rounded, plus the source's byte, held at 255
uint32_t blendOver(uint32_t theTarget, uint32_t theSource, uint32_t theInverseAlpha)
{
	uint32_t aResult = 0;
	for (int aShift = 0; aShift < 32; aShift += 8)
	{
		uint32_t aProduct = ((theTarget >> aShift) & 0xff) * theInverseAlpha + 0x80;
		uint32_t aByte = ((theSource >> aShift) & 0xff) + ((aProduct + (aProduct >> 8)) >> 8);
		aResult |= (aByte > 0xff ? 0xff : aByte) << aShift;
	}
	return aResult;
}

// Each byte times the scale, shifted down (pmullw and psrlw)
uint32_t scaleBytes(uint32_t thePixel, int theScale)
{
	uint32_t aResult = 0;
	for (int aShift = 0; aShift < 32; aShift += 8)
		aResult |= ((((thePixel >> aShift) & 0xff) * (uint32_t)theScale >> 8) & 0xff) << aShift;
	return aResult;
}

// How the fills, outlines, lines and circles put the state's colour on a pixel. At full opacity (0x4234e0, 0x463180,
// 0x513a87, 0x475590, 0x51b52d (feedingfrenzy2)) an opaque colour is copied and a translucent one blended; below it
// (0x48dc10, 0x513763, 0x498f60, 0x51b48c (feedingfrenzy2)) the colour's bytes are scaled by the opacity first, and
// only an opaque colour at an opacity byte of 255 is copied
struct ColorPlot
{
	uint32_t mColor;
	bool mScaled;
	int mScale;

	ColorPlot(uint32_t theColor) : mColor(theColor), mScaled(false), mScale(0x101) {}
	ColorPlot(uint32_t theColor, int theScale) : mColor(theColor), mScaled(true), mScale(theScale) {}

	void plot(uint32_t* thePixel) const
	{
		uint32_t anAlpha = mColor >> 24;
		if (!mScaled)
		{
			if (anAlpha == 0xff)
				*thePixel = mColor;
			else if (anAlpha != 0)
				*thePixel = blendOver(*thePixel, mColor, 0xff - anAlpha);
			return;
		}
		if (anAlpha == 0xff && mScale == 0x101)
			*thePixel = mColor;
		else if (anAlpha != 0 && mScale > 0)
		{
			uint32_t aScaled = scaleBytes(mColor, mScale);
			*thePixel = blendOver(*thePixel, aScaled, 0xff - (aScaled >> 24));
		}
	}
};

// The state's colour and opacity as the primitives plot them: scaled below full opacity
ColorPlot stateColorPlot(const DrawState& theState, bool theFullOpacity)
{
	uint32_t aColor = colorBytes(theState.mColor);
	if (theFullOpacity)
		return ColorPlot(aColor);
	return ColorPlot(aColor, alphaScale(toByte(theState.mOpacity)));
}

// An image's pixel put on the target's by the draw's blend
uint32_t blendPixel(uint32_t theTarget, uint32_t theSource, int theBlend, int theAlphaScale, uint32_t theColor)
{
	switch (theBlend)
	{
	case 0: // copy
		return theSource;
	case 2: // multiply: port: 0x440910's own arithmetic is for the looks pass; here each colour byte times the
		// state's, divided by 255, then the alpha blend
	{
		uint32_t aTinted = theSource & 0xff000000;
		for (int aShift = 0; aShift < 24; aShift += 8)
			aTinted |= (((theSource >> aShift) & 0xff) * ((theColor >> aShift) & 0xff) / 0xff) << aShift;
		theSource = aTinted;
		break;
	}
	case 3: // add: port: 0x4609d0's arithmetic is for the looks pass; here the state's colour, times the pixel's alpha,
		// added to each colour byte
	{
		uint32_t anAlpha = theSource >> 24;
		uint32_t aSum = theSource & 0xff000000;
		for (int aShift = 0; aShift < 24; aShift += 8)
		{
			uint32_t aByte = ((theSource >> aShift) & 0xff) + ((theColor >> aShift) & 0xff) * anAlpha / 0xff;
			aSum |= (aByte > 0xff ? 0xff : aByte) << aShift;
		}
		theSource = aSum;
		break;
	}
	default:
		break;
	}
	if (theAlphaScale != 0x101)
		theSource = scaleBytes(theSource, theAlphaScale);
	uint32_t anAlpha = theSource >> 24;
	if (anAlpha == 0)
		return theTarget;
	if (anAlpha == 0xff)
		return theSource;
	return blendOver(theTarget, theSource, 0xff - anAlpha);
}

} // namespace

DXDrawContext::DXDrawContext(Surface* theTarget) : mTarget(theTarget)
{
}

DXDrawContext::~DXDrawContext()
{
}

void DXDrawContext::drawImage(Surface* theImage, const RectI& theSource, float x, float y)
{
	const DrawState& aState = back();
	ImageDraw aDraw;
	aDraw.mImage = theImage;
	aDraw.mSource = theSource;
	aDraw.mScaleX = aState.mScale.x;
	aDraw.mScaleY = aState.mScale.y;
	aDraw.mRotation = aState.mRotation;
	aDraw.mX = aState.mPosition.x + x;
	aDraw.mY = aState.mPosition.y + y;
	aDraw.mFilter = 0;
	if (aState.mFilterMode == FILTER_MODE_SNAP)
	{
		// 0x43f520 (feedingfrenzy2): the CRT's floor
		aDraw.mX = (float)floor((double)aDraw.mX);
		aDraw.mY = (float)floor((double)aDraw.mY);
	}
	else if (aState.mFilterMode == FILTER_MODE_SMOOTH)
		aDraw.mFilter = 0xf;
	if (aState.mOpacity == 0.0f)
		return;
	if (aDraw.mScaleX == 0.0f || aDraw.mScaleY == 0.0f)
		return;
	aDraw.mAlphaScale = alphaScale(toByte(aState.mOpacity));
	aDraw.mColor = colorBytes(aState.mColor);

	if (aState.mColorMode == COLOR_MODE_MULTIPLY)
		aDraw.mBlend = IMAGE_BLEND_MULTIPLY;
	else if (aState.mColorMode == COLOR_MODE_PIXEL_FX)
		aDraw.mBlend = IMAGE_BLEND_PIXEL_FX;
	else if (aState.mColorMode == COLOR_MODE_ADD)
		aDraw.mBlend = IMAGE_BLEND_ADD;
	else
	{
		int aBlendMode = theImage->getBlendMode();
		bool aFullOpacity = nearlyEqual(aState.mOpacity, 1.0f);
		if (aBlendMode == 3)
			aDraw.mBlend = IMAGE_BLEND_MODE_3;
		else if (aBlendMode == BLEND_ALPHA || !aFullOpacity)
			aDraw.mBlend = IMAGE_BLEND_ALPHA;
		else if (aBlendMode == 1)
			aDraw.mBlend = IMAGE_BLEND_COLOR_KEY;
		else
			aDraw.mBlend = IMAGE_BLEND_COPY;
		// 0x441810 (feedingfrenzy2) draws at full opacity, 0x442740 (feedingfrenzy2) scales by the opacity byte
		if (aDraw.mBlend == IMAGE_BLEND_ALPHA && aFullOpacity && aBlendMode == BLEND_ALPHA)
			aDraw.mAlphaScale = 0x101;
	}
	if (aDraw.mBlend == IMAGE_BLEND_COPY)
		aDraw.mAlphaScale = 0x101;

	RectI aTargetRect(0, 0, mTarget->getWidth(), mTarget->getHeight());
	if (mClips.empty())
	{
		blitImage(aDraw, aTargetRect);
		return;
	}
	for (const RectI& aRect : getClipRegion())
	{
		RectI aClip = intersection(aRect, aTargetRect);
		blitImage(aDraw, aClip);
	}
}

void DXDrawContext::blitImage(const ImageDraw& theDraw, const RectI& theClip)
{
	if (isEmpty(theClip))
		return;
	if (theDraw.mRotation != 0.0f || theDraw.mBlend == IMAGE_BLEND_PIXEL_FX || theDraw.mBlend == IMAGE_BLEND_COLOR_KEY ||
		theDraw.mBlend == IMAGE_BLEND_MODE_3)
	{
		// port: the rotated blitters (0x459680 (feedingfrenzy2) for an alpha image) and the pixel effect, colour key
		// and mode 3 ones are for the looks pass
		sampleImage(theDraw, theClip);
		return;
	}

	// The unrotated geometry (0x441810 (feedingfrenzy2)): the image's rectangle around its anchor, scaled, placed at
	// the position; the x87 keeps the corners in registers until they are stored as floats
	Vector2f anAnchor = theDraw.mImage->getAnchor();
	if ((theDraw.mFilter & 3) == 0)
	{
		anAnchor.x = (float)(int)anAnchor.x;
		anAnchor.y = (float)(int)anAnchor.y;
	}
	double aSourceLeft = theDraw.mSource.left;
	double aSourceTop = theDraw.mSource.top;
	double aSourceRight = theDraw.mSource.right;
	double aSourceBottom = theDraw.mSource.bottom;
	double aLeft = (aSourceLeft - aSourceLeft + -(double)anAnchor.x) * theDraw.mScaleX;
	double aRight = (aSourceRight - aSourceLeft + -(double)anAnchor.x) * theDraw.mScaleX;
	double aTop = (aSourceTop - aSourceTop + -(double)anAnchor.y) * theDraw.mScaleY;
	double aBottom = (aSourceBottom - aSourceTop + -(double)anAnchor.y) * theDraw.mScaleY;
	if (aRight < aLeft)
	{
		double aSwap = aLeft;
		aLeft = aRight;
		aRight = aSwap;
	}
	if (aBottom < aTop)
	{
		double aSwap = aTop;
		aTop = aBottom;
		aBottom = aSwap;
	}
	float aDestLeft = (float)(aLeft + theDraw.mX);
	float aDestRight = (float)(aRight + theDraw.mX);
	float aDestTop = (float)(aTop + theDraw.mY);
	float aDestBottom = (float)(aBottom + theDraw.mY);
	float aSrcLeft = (float)aSourceLeft;
	float aSrcTop = (float)aSourceTop;
	float aSrcRight = (float)aSourceRight;
	float aSrcBottom = (float)aSourceBottom;

	// Cut to the clip rectangle, the source cut to match (0x441e3f (feedingfrenzy2))
	// port: the original then nudges an unscaled source whose cut edge is not whole (0x441fef-0x442055
	// (feedingfrenzy2)), left for the looks pass
	if (aDestLeft < (float)theClip.left || aDestTop < (float)theClip.top || aDestRight > (float)theClip.right ||
		aDestBottom > (float)theClip.bottom)
	{
		aDestLeft = maxOf((float)theClip.left, aDestLeft);
		aDestRight = minOf((float)theClip.right, aDestRight);
		aDestTop = maxOf((float)theClip.top, aDestTop);
		aDestBottom = minOf((float)theClip.bottom, aDestBottom);
		if (!(aDestLeft < aDestRight) || !(aDestTop < aDestBottom))
			return;
		double anInverseX = 1.0 / theDraw.mScaleX;
		double anInverseY = 1.0 / theDraw.mScaleY;
		double aU0 = (aDestLeft + -(double)theDraw.mX) * anInverseX;
		double aU1 = (aDestRight + -(double)theDraw.mX) * anInverseX;
		double aV0 = (aDestTop + -(double)theDraw.mY) * anInverseY;
		double aV1 = (aDestBottom + -(double)theDraw.mY) * anInverseY;
		if (aU1 < aU0)
		{
			double aSwap = aU0;
			aU0 = aU1;
			aU1 = aSwap;
		}
		if (aV1 < aV0)
		{
			double aSwap = aV0;
			aV0 = aV1;
			aV1 = aSwap;
		}
		aSrcLeft = (float)(aU0 + anAnchor.x + aSourceLeft);
		aSrcRight = (float)(aU1 + anAnchor.x + aSourceLeft);
		aSrcTop = (float)(aV0 + anAnchor.y + aSourceTop);
		aSrcBottom = (float)(aV1 + anAnchor.y + aSourceTop);
	}

	int aFlip = (theDraw.mScaleX < 0.0f ? 1 : 0) | (theDraw.mScaleY < 0.0f ? 2 : 0);
	float aScaleX = fabsf(theDraw.mScaleX);
	float aScaleY = fabsf(theDraw.mScaleY);
	RectI aDest((int)aDestLeft, (int)aDestTop, (int)ceil((double)aDestRight), (int)ceil((double)aDestBottom));
	RectI aSource((int)aSrcLeft, (int)aSrcTop, (int)ceil((double)aSrcRight), (int)ceil((double)aSrcBottom));
	aSource.right = minOf(aSource.right, theDraw.mImage->getWidth());
	aSource.bottom = minOf(aSource.bottom, theDraw.mImage->getHeight());
	if (isEmpty(aDest) || isEmpty(aSource))
		return;
	int aFractionX = (int)((aDestLeft - (float)floor((double)aDestLeft)) * 255.0);
	int aFractionY = (int)((aDestTop - (float)floor((double)aDestTop)) * 255.0);

	if (aScaleX != 1.0f || aScaleY != 1.0f || aFractionX != 0 || aFractionY != 0 || theDraw.mBlend == IMAGE_BLEND_MULTIPLY ||
		theDraw.mBlend == IMAGE_BLEND_ADD)
	{
		// port: the scaled blitters (0x433680 and 0x44e6e0 (feedingfrenzy2)), the sub-pixel one (0x44d660
		// (feedingfrenzy2)) and the multiply and add ones are for the looks pass
		sampleImage(theDraw, theClip);
		return;
	}

	// 0x444ec0 (feedingfrenzy2) and the copies: the two locked rectangles' common size, the source read backwards
	// along a flipped axis
	PixelRect aTarget = lockRect(mTarget, aDest);
	PixelRect anImage = lockRect(theDraw.mImage, aSource);
	int aWidth = minOf(aTarget.mWidth, anImage.mWidth);
	int aHeight = minOf(aTarget.mHeight, anImage.mHeight);
	for (int aRow = 0; aRow < aHeight; aRow++)
	{
		int aSourceRow = (aFlip & 2) ? anImage.mHeight - 1 - aRow : aRow;
		uint32_t* aTargetPixel = aTarget.mBits + aRow * aTarget.mPitch;
		const uint32_t* aSourceLine = anImage.mBits + aSourceRow * anImage.mPitch;
		for (int aColumn = 0; aColumn < aWidth; aColumn++)
		{
			int aSourceColumn = (aFlip & 1) ? anImage.mWidth - 1 - aColumn : aColumn;
			uint32_t aPixel = aSourceLine[aSourceColumn];
			if (theDraw.mBlend == IMAGE_BLEND_COPY)
				aTargetPixel[aColumn] = aPixel;
			else
				aTargetPixel[aColumn] = blendPixel(aTargetPixel[aColumn], aPixel, 1, theDraw.mAlphaScale, theDraw.mColor);
		}
	}
}

void DXDrawContext::sampleImage(const ImageDraw& theDraw, const RectI& theClip)
{
	Vector2f anAnchor = theDraw.mImage->getAnchor();
	if ((theDraw.mFilter & 3) == 0)
	{
		anAnchor.x = (float)(int)anAnchor.x;
		anAnchor.y = (float)(int)anAnchor.y;
	}
	RectI aSource = theDraw.mSource;
	aSource.right = minOf(aSource.right, theDraw.mImage->getWidth());
	aSource.bottom = minOf(aSource.bottom, theDraw.mImage->getHeight());
	aSource.left = maxOf(aSource.left, 0);
	aSource.top = maxOf(aSource.top, 0);
	if (isEmpty(aSource))
		return;
	double aCos = cos((double)theDraw.mRotation);
	double aSin = sin((double)theDraw.mRotation);

	// The image's corners on the target, for the rectangle to visit
	double aMinX = 1e30, aMinY = 1e30, aMaxX = -1e30, aMaxY = -1e30;
	double aCornersX[2] = { -(double)anAnchor.x, (double)(theDraw.mSource.right - theDraw.mSource.left) - anAnchor.x };
	double aCornersY[2] = { -(double)anAnchor.y, (double)(theDraw.mSource.bottom - theDraw.mSource.top) - anAnchor.y };
	for (double aCornerX : aCornersX)
		for (double aCornerY : aCornersY)
		{
			double aX = aCornerX * theDraw.mScaleX;
			double aY = aCornerY * theDraw.mScaleY;
			double aTargetX = theDraw.mX + aX * aCos - aY * aSin;
			double aTargetY = theDraw.mY + aX * aSin + aY * aCos;
			aMinX = minOf(aMinX, aTargetX);
			aMaxX = maxOf(aMaxX, aTargetX);
			aMinY = minOf(aMinY, aTargetY);
			aMaxY = maxOf(aMaxY, aTargetY);
		}
	RectI aVisit((int)floor(aMinX), (int)floor(aMinY), (int)ceil(aMaxX), (int)ceil(aMaxY));
	aVisit = intersection(aVisit, theClip);
	if (isEmpty(aVisit))
		return;

	int aBlend = theDraw.mBlend == IMAGE_BLEND_COPY ? 0 : theDraw.mBlend == IMAGE_BLEND_MULTIPLY ? 2 :
		theDraw.mBlend == IMAGE_BLEND_ADD ? 3 : 1;
	const uint32_t* aPixels = theDraw.mImage->getPixels();
	int aPitch = theDraw.mImage->getWidth();
	bool aFiltered = theDraw.mFilter != 0;
	uint32_t* aTargetPixels = mTarget->getPixels();
	int aTargetPitch = mTarget->getWidth();
	for (int aRow = aVisit.top; aRow < aVisit.bottom; aRow++)
	{
		for (int aColumn = aVisit.left; aColumn < aVisit.right; aColumn++)
		{
			// The pixel's centre back in the image
			double aX = aColumn + 0.5 - theDraw.mX;
			double aY = aRow + 0.5 - theDraw.mY;
			double aU = (aX * aCos + aY * aSin) / theDraw.mScaleX + anAnchor.x + theDraw.mSource.left;
			double aV = (-aX * aSin + aY * aCos) / theDraw.mScaleY + anAnchor.y + theDraw.mSource.top;
			if (aU < aSource.left || aV < aSource.top || aU >= aSource.right || aV >= aSource.bottom)
				continue;
			uint32_t aPixel;
			if (!aFiltered)
				aPixel = aPixels[(int)aV * aPitch + (int)aU];
			else
			{
				// The four pixels around the point, weighted by how near each is, the image's edge repeated
				double aFloorU = floor(aU - 0.5);
				double aFloorV = floor(aV - 0.5);
				int aWeightX = (int)((aU - 0.5 - aFloorU) * 256.0);
				int aWeightY = (int)((aV - 0.5 - aFloorV) * 256.0);
				int aU0 = clampOf((int)aFloorU, aSource.left, aSource.right - 1);
				int aU1 = clampOf((int)aFloorU + 1, aSource.left, aSource.right - 1);
				int aV0 = clampOf((int)aFloorV, aSource.top, aSource.bottom - 1);
				int aV1 = clampOf((int)aFloorV + 1, aSource.top, aSource.bottom - 1);
				uint32_t aTopLeft = aPixels[aV0 * aPitch + aU0];
				uint32_t aTopRight = aPixels[aV0 * aPitch + aU1];
				uint32_t aBottomLeft = aPixels[aV1 * aPitch + aU0];
				uint32_t aBottomRight = aPixels[aV1 * aPitch + aU1];
				aPixel = 0;
				for (int aShift = 0; aShift < 32; aShift += 8)
				{
					int aTop = (int)((aTopLeft >> aShift) & 0xff) * (256 - aWeightX) + (int)((aTopRight >> aShift) & 0xff) * aWeightX;
					int aBottom = (int)((aBottomLeft >> aShift) & 0xff) * (256 - aWeightX) +
						(int)((aBottomRight >> aShift) & 0xff) * aWeightX;
					int aByte = (aTop * (256 - aWeightY) + aBottom * aWeightY + 0x8000) >> 16;
					aPixel |= (uint32_t)(aByte > 0xff ? 0xff : aByte) << aShift;
				}
			}
			uint32_t& aTarget = aTargetPixels[aRow * aTargetPitch + aColumn];
			aTarget = blendPixel(aTarget, aPixel, aBlend, theDraw.mAlphaScale, theDraw.mColor);
		}
	}
}

void DXDrawContext::drawRect(const RectI& theRect)
{
	const DrawState& aState = back();
	int anOffsetX = (int)aState.mPosition.x;
	int anOffsetY = (int)aState.mPosition.y;
	RectI aRect(theRect.left + anOffsetX, theRect.top + anOffsetY, theRect.right + anOffsetX, theRect.bottom + anOffsetY);
	aRect = intersection(aRect, RectI(0, 0, mTarget->getWidth(), mTarget->getHeight()));
	if (isEmpty(aRect))
		return;
	ColorPlot aPlot = stateColorPlot(aState, aState.mOpacity >= 1.0f);
	PixelRect aLock = lockRect(mTarget, aRect);
	// The rectangle's edges (0x513a87 and 0x513763 (feedingfrenzy2)): the top row, the bottom one, the left column and
	// the right one, the corners twice.
	// port: the original's first blended pixel of each edge goes through a stale pointer (for the top row, into the
	// lock structure's width, which the bottom row then reads), so a translucent outline would corrupt memory; the port
	// blends that pixel where it belongs
	for (int x = 0; x < aLock.mWidth; x++)
		aPlot.plot(aLock.mBits + x);
	for (int x = 0; x < aLock.mWidth; x++)
		aPlot.plot(aLock.mBits + (aLock.mHeight - 1) * aLock.mPitch + x);
	for (int y = 0; y < aLock.mHeight; y++)
		aPlot.plot(aLock.mBits + y * aLock.mPitch);
	for (int y = 0; y < aLock.mHeight; y++)
		aPlot.plot(aLock.mBits + y * aLock.mPitch + aLock.mWidth - 1);
}

void DXDrawContext::fillRect(const RectI& theRect)
{
	const DrawState& aState = back();
	int anOffsetX = (int)aState.mPosition.x;
	int anOffsetY = (int)aState.mPosition.y;
	RectI aRect(theRect.left + anOffsetX, theRect.top + anOffsetY, theRect.right + anOffsetX, theRect.bottom + anOffsetY);
	if (aRect.left < 1)
		aRect.left = 0;
	if (mTarget->getWidth() <= aRect.right)
		aRect.right = mTarget->getWidth();
	if (aRect.top < 1)
		aRect.top = 0;
	if (mTarget->getHeight() <= aRect.bottom)
		aRect.bottom = mTarget->getHeight();
	if (isEmpty(aRect))
		return;
	PixelRect aLock = lockRect(mTarget, aRect);
	// 0x463180 (an opaque colour), 0x4234e0 (a translucent one) and 0x48dc10 (below full opacity) (feedingfrenzy2)
	ColorPlot aPlot = stateColorPlot(aState, nearlyEqual(aState.mOpacity, 1.0f));
	for (int y = 0; y < aLock.mHeight; y++)
		for (int x = 0; x < aLock.mWidth; x++)
			aPlot.plot(aLock.mBits + y * aLock.mPitch + x);
}

void DXDrawContext::drawLine(const Vector2f& theFrom, const Vector2f& theTo)
{
	const DrawState& aState = back();
	Vector2f aFrom(theFrom.x + aState.mPosition.x, theFrom.y + aState.mPosition.y);
	Vector2f aTo(theTo.x + aState.mPosition.x, theTo.y + aState.mPosition.y);
	PixelRect aLock = lockAll(mTarget);
	// Drawn only when it lies wholly on the target
	if (!(maxOf(aFrom.y, aTo.y) < (float)aLock.mHeight) || !(minOf(aFrom.y, aTo.y) >= 0.0f))
		return;
	if (!(maxOf(aFrom.x, aTo.x) < (float)aLock.mWidth) || !(minOf(aFrom.x, aTo.x) >= 0.0f))
		return;
	ColorPlot aPlot = stateColorPlot(aState, !(aState.mOpacity < 1.0f));

	// 0x475590 and 0x498f60 (feedingfrenzy2): Bresenham's line between the truncated ends, the far end left out
	int aDeltaX = (int)((double)aTo.x - (double)aFrom.x);
	int aDoubleX = (aDeltaX > 0 ? aDeltaX : -aDeltaX) * 2;
	int aStepX = aDeltaX > 0 ? 1 : aDeltaX < 0 ? -1 : 0;
	int aDeltaY = (int)((double)aTo.y - (double)aFrom.y);
	int aDoubleY = (aDeltaY > 0 ? aDeltaY : -aDeltaY) * 2;
	int aStepY = aDeltaY > 0 ? 1 : aDeltaY < 0 ? -1 : 0;
	int x = (int)aFrom.x;
	int anEndX = (int)aTo.x;
	int y = (int)aFrom.y;
	int anEndY = (int)aTo.y;
	if (aDoubleX > aDoubleY && aStepX != 0)
	{
		int anError = aDoubleY - (aDoubleX >> 1);
		while (x != anEndX)
		{
			aPlot.plot(aLock.mBits + y * aLock.mPitch + x);
			if (anError >= 0)
			{
				y += aStepY;
				anError -= aDoubleX;
			}
			x += aStepX;
			anError += aDoubleY;
		}
	}
	else if (aStepY != 0)
	{
		int anError = aDoubleX - (aDoubleY >> 1);
		while (y != anEndY)
		{
			aPlot.plot(aLock.mBits + y * aLock.mPitch + x);
			if (anError >= 0)
			{
				x += aStepX;
				anError -= aDoubleY;
			}
			y += aStepY;
			anError += aDoubleX;
		}
	}
}

void DXDrawContext::drawCircle(const Vector2f& theCenter, float theRadius)
{
	const DrawState& aState = back();
	PixelRect aLock = lockAll(mTarget);
	Vector2f aCenter(theCenter.x + aState.mPosition.x, theCenter.y + aState.mPosition.y);
	// Drawn only when it lies wholly on the target
	if (!((double)aCenter.x - theRadius >= 0.0) || !((double)aCenter.x + theRadius <= (double)aLock.mWidth))
		return;
	if (!((double)aCenter.y + theRadius <= (double)aLock.mHeight) || !((double)aCenter.y - theRadius >= 0.0))
		return;
	ColorPlot aPlot = stateColorPlot(aState, aState.mOpacity >= 1.0f);

	// 0x51b52d and 0x51b48c (feedingfrenzy2): the midpoint circle, each step's eight points (0x513226
	// (feedingfrenzy2)) in its order.
	// port: the original lets a point lie on the target's right or bottom edge, one past its last pixel (the next
	// row's first, or past the buffer); the port leaves such a point out
	int aCenterX = (int)aCenter.x;
	int aCenterY = (int)aCenter.y;
	auto plotEight = [&](int x, int y) {
		const int aPoints[8][2] = { { x, y }, { y, x }, { y, -x }, { x, -y }, { -x, -y }, { -y, -x }, { -y, x }, { -x, y } };
		for (const int* aPoint : aPoints)
		{
			int aPixelX = aCenterX + aPoint[0];
			int aPixelY = aCenterY + aPoint[1];
			if (aPixelX >= 0 && aPixelX < aLock.mWidth && aPixelY >= 0 && aPixelY < aLock.mHeight)
				aPlot.plot(aLock.mBits + aPixelY * aLock.mPitch + aPixelX);
		}
	};
	int aRadius = (int)theRadius;
	int x = 0;
	int y = aRadius;
	int aDecision = 1 - aRadius;
	plotEight(x, y);
	while (y > x)
	{
		if (aDecision < 0)
			aDecision += 2 * x + 3;
		else
		{
			aDecision += 2 * (x - y) + 5;
			y--;
		}
		x++;
		plotEight(x, y);
	}
}

void DXDrawContext::darkenByAlpha()
{
	// 0x4335b0 (feedingfrenzy2): each colour byte times (255 - alpha), divided by 255 rounded; the alpha kept
	auto darken = [](const PixelRect& theLock) {
		for (int y = 0; y < theLock.mHeight; y++)
		{
			uint32_t* aPixel = theLock.mBits + y * theLock.mPitch;
			for (int x = 0; x < theLock.mWidth; x++, aPixel++)
			{
				uint32_t anInverse = 0xff - (*aPixel >> 24);
				uint32_t aResult = *aPixel & 0xff000000;
				for (int aShift = 0; aShift < 24; aShift += 8)
				{
					uint32_t aProduct = ((*aPixel >> aShift) & 0xff) * anInverse + 0x80;
					aResult |= (((aProduct + (aProduct >> 8)) >> 8) & 0xff) << aShift;
				}
				*aPixel = aResult;
			}
		}
	};
	RectI aTargetRect(0, 0, mTarget->getWidth(), mTarget->getHeight());
	if (mClips.empty())
	{
		darken(lockAll(mTarget));
		return;
	}
	for (const RectI& aRect : getClipRegion())
	{
		RectI aClip = intersection(aRect, aTargetRect);
		if (!isEmpty(aClip))
			darken(lockRect(mTarget, aClip));
	}
}

void DXDrawContext::clear()
{
	mTarget->fill(back().mColor);
}

} // namespace engine
