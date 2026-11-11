// Retro64's lines and rectangles (0x42d1f0..0x42dd00, and the 16-bit drawers 0x441da0..0x4422e0): Draw_SetTarget names
// the surface, Draw_Line draws on it clipped, each pixel blended toward the colour by alpha/256; the others draw on the
// surface they are given. The original also has 24- and 32-bit drawers (0x442370.., 0x442870..); the port's screen is
// 16-bit, so only those are translated, and the dispatchers do nothing at other depths.

#include "R64Draw.h"

#include <cstring>

namespace r64
{

namespace
{

uint8_t* gTargetBits = nullptr;   // 0x71ddc4
int gTargetPitch = 0;             // 0x71ddc8
[[maybe_unused]] int gTargetW = 0;  // 0x71ddcc  (no line reads these two)
[[maybe_unused]] int gTargetH = 0;  // 0x71ddd0
int gTargetClip[4] = {0, 0, 0, 0}; // 0x71ddd4  left, top, right, bottom (inclusive)
PixelFormat gTargetFmt = {};      // 0x70a4f4

// 0x42d250: Cohen-Sutherland's outcode (1 below, 2 above, 4 right, 8 left)
int OutCode(double x, double y, double theLeft, double theRight, double theTop, double theBottom)
{
	int aCode = 0;
	if (y > theBottom)
		aCode = 1;
	else if (y < theTop)
		aCode = 2;
	if (x > theRight)
		return aCode | 4;
	if (x < theLeft)
		aCode = aCode | 8;
	return aCode;
}

// 0x42d2b0: 0 when the line misses the clip; the ends move onto it, cut to whole pixels
int ClipLine(int* theX0, int* theY0, int* theX1, int* theY1, const int* theClip)
{
	double anAx0 = *theX0;
	double anAy0 = *theY0;
	double anAx1 = *theX1;
	double anAy1 = *theY1;
	double aLeft = theClip[0];
	double aRight = theClip[2];
	double aTop = theClip[1];
	double aBottom = theClip[3];
	int aCode0 = OutCode(anAx0, anAy0, aLeft, aRight, aTop, aBottom);
	int aCode1 = OutCode(anAx1, anAy1, aLeft, aRight, aTop, aBottom);
	for (;;)
	{
		if (aCode0 == 0 && aCode1 == 0)
		{
			*theX0 = (int)anAx0;
			*theY0 = (int)anAy0;
			*theX1 = (int)anAx1;
			*theY1 = (int)anAy1;
			return 1;
		}
		if ((aCode0 & aCode1) != 0)
			return 0;
		int aCode = aCode0 != 0 ? aCode0 : aCode1;
		double x;
		double y;
		if ((aCode & 1) != 0)
		{
			x = (anAx1 - anAx0) * (aBottom - anAy0) / (anAy1 - anAy0) + anAx0;
			y = aBottom;
		}
		else if ((aCode & 2) != 0)
		{
			x = (anAx1 - anAx0) * (aTop - anAy0) / (anAy1 - anAy0) + anAx0;
			y = aTop;
		}
		else if ((aCode & 4) != 0)
		{
			y = (aRight - anAx0) * (anAy1 - anAy0) / (anAx1 - anAx0) + anAy0;
			x = aRight;
		}
		else
		{
			y = (aLeft - anAx0) * (anAy1 - anAy0) / (anAx1 - anAx0) + anAy0;
			x = aLeft;
		}
		if (aCode == aCode0)
		{
			anAx0 = x;
			anAy0 = y;
			aCode0 = OutCode(anAx0, anAy0, aLeft, aRight, aTop, aBottom);
		}
		else
		{
			anAx1 = x;
			anAy1 = y;
			aCode1 = OutCode(anAx1, anAy1, aLeft, aRight, aTop, aBottom);
		}
	}
}

// A component of a 16-bit pixel moved theAlpha/256 of the way to theTo (the original's sums wrap as uint32)
inline uint32_t Blend(uint32_t thePixel, uint32_t theTo, uint32_t theMask, uint32_t theAlpha)
{
	return ((theTo - (thePixel & theMask)) * theAlpha >> 8) + (thePixel & theMask) & theMask;
}

// 0x441da0: the rectangle (inclusive) in the colour
void Fill16(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int theLeft, int theTop, int theRight,
	int theBottom, uint8_t theR, uint8_t theG, uint8_t theB)
{
	uint16_t aColour = (uint16_t)((uint32_t)(theB >> theFmt->mBLoss) << theFmt->mBShift |
		(uint32_t)(theG >> theFmt->mGLoss) << theFmt->mGShift | (uint32_t)(theR >> theFmt->mRLoss) << theFmt->mRShift);
	uint32_t aWidth = (theRight - theLeft) + 1;
	uint8_t* aRow = theBits + thePitch * theTop + theLeft * 2;
	if (theR == 0 && theG == 0 && theB == 0)
	{
		if (theTop <= theBottom)
		{
			int aCount = (theBottom - theTop) + 1;
			do
			{
				std::memset(aRow, 0, aWidth * 2);
				aRow = aRow + thePitch;
				aCount = aCount - 1;
			} while (aCount != 0);
			return;
		}
	}
	else if (theTop <= theBottom)
	{
		int aCount = (theBottom - theTop) + 1;
		do
		{
			if (theLeft <= theRight)
			{
				uint16_t* p = (uint16_t*)aRow;
				for (uint32_t i = aWidth >> 1; i != 0; i--)
				{
					p[0] = aColour;
					p[1] = aColour;
					p = p + 2;
				}
				for (uint32_t i = (aWidth & 1) != 0; i != 0; i--)
				{
					*p = aColour;
					p = p + 1;
				}
			}
			aRow = aRow + thePitch;
			aCount = aCount - 1;
		} while (aCount != 0);
	}
}

// 0x441eb0: the rectangle (inclusive) blended toward the colour by theAlpha/256
void FillBlend16(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int l, int t, int r, int b, uint8_t theR,
	uint8_t theG, uint8_t theB, uint8_t theAlpha)
{
	uint16_t* aRow = (uint16_t*)(theBits + thePitch * t + l * 2);
	uint32_t aR = (uint32_t)(theR >> theFmt->mRLoss) << theFmt->mRShift;
	int aWidth = (r - l) + 1;
	int aCount = (b - t) + 1;
	uint32_t aB = (uint32_t)(theB >> theFmt->mBLoss) << theFmt->mBShift;
	if (0 < aCount)
	{
		do
		{
			if (0 < aWidth)
			{
				uint16_t* p = aRow;
				int n = aWidth;
				do
				{
					uint32_t c = *p;
					*p = (uint16_t)(Blend(c, (uint32_t)(theG >> theFmt->mGLoss) << theFmt->mGShift, theFmt->mGMask,
						theAlpha) |
						Blend(c, aR, theFmt->mRMask, theAlpha) | Blend(c, aB, theFmt->mBMask, theAlpha));
					n = n - 1;
					p = p + 1;
				} while (n != 0);
			}
			aRow = aRow + thePitch / 2;
			aCount = aCount - 1;
		} while (aCount != 0);
	}
}

// 0x442000: FillBlend16 on the part inside theClip
void FillBlend16Clipped(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, const int* theClip, int l,
	int theTop, int theRight, int theBottom, uint8_t theR, uint8_t theG, uint8_t theB, uint8_t theAlpha)
{
	if (l <= theClip[2] && theClip[0] <= theRight && theTop <= theClip[3] && theClip[1] <= theBottom)
	{
		int aLeft = theClip[0];
		if (aLeft <= l)
			aLeft = l;
		if (theClip[2] < theRight)
			theRight = theClip[2];
		if (theTop < theClip[1])
			theTop = theClip[1];
		if (theClip[3] < theBottom)
			theBottom = theClip[3];
		FillBlend16(theBits, thePitch, theFmt, aLeft, theTop, theRight, theBottom, theR, theG, theB, theAlpha);
	}
}

// 0x442090: max(|dx|, |dy|) + 1 pixels, stepping along the longer axis
void Line16(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int theX0, int theY0, int theX1, int theY1,
	uint8_t theRed, uint8_t theGreen, uint8_t theBlue, uint8_t theAlpha)
{
	int aRow = thePitch / 2;
	uint32_t aR = (uint32_t)(theRed >> theFmt->mRLoss) << theFmt->mRShift;
	uint32_t aG = (uint32_t)(theGreen >> theFmt->mGLoss) << theFmt->mGShift;
	int anErr = 0;
	uint32_t aB = (uint32_t)(theBlue >> theFmt->mBLoss) << theFmt->mBShift;
	uint16_t* aPixel = (uint16_t*)(theBits + thePitch * theY0 + theX0 * 2);
	int aDx = theX1 - theX0;
	int aDy = theY1 - theY0;
	int aStep;
	if (aDx < 0)
	{
		aDx = -aDx;
		aStep = -1;
	}
	else
		aStep = 1;
	if (aDy < 0)
	{
		aRow = -aRow;
		aDy = -aDy;
	}
	if (aDy < aDx)
	{
		for (int i = 0; i <= aDx; i++)
		{
			uint32_t c = *aPixel;
			anErr = anErr + aDy;
			*aPixel = (uint16_t)(Blend(c, aG, theFmt->mGMask, theAlpha) | Blend(c, aR, theFmt->mRMask, theAlpha) |
				Blend(c, aB, theFmt->mBMask, theAlpha));
			if (aDx < anErr)
			{
				anErr = anErr - aDx;
				aPixel = aPixel + aRow;
			}
			aPixel = aPixel + aStep;
		}
	}
	else
	{
		for (int i = 0; i <= aDy; i++)
		{
			uint32_t c = *aPixel;
			anErr = anErr + aDx;
			*aPixel = (uint16_t)(Blend(c, aG, theFmt->mGMask, theAlpha) | Blend(c, aR, theFmt->mRMask, theAlpha) |
				Blend(c, aB, theFmt->mBMask, theAlpha));
			if (0 < anErr)
			{
				anErr = anErr - aDy;
				aPixel = aPixel + aStep;
			}
			aPixel = aPixel + aRow;
		}
	}
}

// 0x4422e0
void Line16Clipped(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, const int* theClip, int theX0, int theY0,
	int theX1, int theY1, int r, int g, int b, int theAlpha)
{
	if (ClipLine(&theX0, &theY0, &theX1, &theY1, theClip) != 0)
		Line16(theBits, thePitch, theFmt, theX0, theY0, theX1, theY1, (uint8_t)r, (uint8_t)g, (uint8_t)b,
			(uint8_t)theAlpha);
}

} // namespace

void Draw_SetTarget(uint8_t* theBits, int thePitch, int theW, int theH, const PixelFormat* theFmt)
{
	gTargetBits = theBits;
	gTargetPitch = thePitch;
	gTargetW = theW;
	gTargetH = theH;
	gTargetClip[3] = theH - 1;
	gTargetClip[2] = theW - 1;
	gTargetClip[1] = 0;
	gTargetClip[0] = 0;
	if (theFmt != nullptr)
		gTargetFmt = *theFmt;
}

void Draw_Line(int theX0, int theY0, int theX1, int theY1, int r, int g, int b, int theAlpha)
{
	if (gTargetFmt.mBpp == 16)
		Line16Clipped(gTargetBits, gTargetPitch, &gTargetFmt, gTargetClip, theX0, theY0, theX1, theY1, r, g, b,
			theAlpha);
}

void Draw_Fill(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int l, int t, int r, int b, int theR,
	int theG, int theB)
{
	if (theFmt->mBpp == 0x10)
		Fill16(theBits, thePitch, theFmt, l, t, r, b, (uint8_t)theR, (uint8_t)theG, (uint8_t)theB);
}

void Draw_FillBlend(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int l, int t, int r, int b, int theR,
	int theG, int theB, int theAlpha)
{
	if (theFmt->mBpp == 0x10)
		FillBlend16(theBits, thePitch, theFmt, l, t, r, b, (uint8_t)theR, (uint8_t)theG, (uint8_t)theB,
			(uint8_t)theAlpha);
}

void Draw_FillBlendTarget(int l, int t, int r, int b, int theR, int theG, int theB, int theAlpha)
{
	if (gTargetFmt.mBpp == 0x10)
		FillBlend16(gTargetBits, gTargetPitch, &gTargetFmt, l, t, r, b, (uint8_t)theR, (uint8_t)theG, (uint8_t)theB,
			(uint8_t)theAlpha);
}

void Draw_FillBlendClipped(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, const int* theClip, int l, int t,
	int theRight, int theBottom, int theR, int theG, int theB, int theAlpha)
{
	if (theFmt->mBpp == 0x10)
		FillBlend16Clipped(theBits, thePitch, theFmt, theClip, l, t, theRight, theBottom, (uint8_t)theR, (uint8_t)theG,
			(uint8_t)theB, (uint8_t)theAlpha);
}

void Draw_LineOn(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int theX0, int theY0, int theX1, int theY1,
	int r, int g, int b, int theAlpha)
{
	if (theFmt->mBpp == 0x10)
		Line16(theBits, thePitch, theFmt, theX0, theY0, theX1, theY1, (uint8_t)r, (uint8_t)g, (uint8_t)b,
			(uint8_t)theAlpha);
}

} // namespace r64
