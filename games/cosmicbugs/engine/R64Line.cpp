// Retro64's lines (0x42cd10..0x42d820, and the 16-bit drawer 0x441c50..0x441ea0): Draw_SetTarget names the surface,
// Draw_Line draws on it clipped, each pixel blended toward the colour by alpha/256. The original also has 24- and
// 32-bit drawers (0x4423a0, 0x4428b0); the port's screen is 16-bit, so only that one is translated.

#include "R64Draw.h"

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

// 0x42cd70: Cohen-Sutherland's outcode (1 below, 2 above, 4 right, 8 left)
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

// 0x42cdd0: 0 when the line misses the clip; the ends move onto it, cut to whole pixels
int ClipLine(int* x0, int* y0, int* x1, int* y1, const int* theClip)
{
	double ax0 = *x0;
	double ay0 = *y0;
	double ax1 = *x1;
	double ay1 = *y1;
	double aLeft = theClip[0];
	double aRight = theClip[2];
	double aTop = theClip[1];
	double aBottom = theClip[3];
	int aCode0 = OutCode(ax0, ay0, aLeft, aRight, aTop, aBottom);
	int aCode1 = OutCode(ax1, ay1, aLeft, aRight, aTop, aBottom);
	for (;;)
	{
		if (aCode0 == 0 && aCode1 == 0)
		{
			*x0 = (int)ax0;
			*y0 = (int)ay0;
			*x1 = (int)ax1;
			*y1 = (int)ay1;
			return 1;
		}
		if ((aCode0 & aCode1) != 0)
			return 0;
		int aCode = aCode0 != 0 ? aCode0 : aCode1;
		double x;
		double y;
		if ((aCode & 1) != 0)
		{
			x = (ax1 - ax0) * (aBottom - ay0) / (ay1 - ay0) + ax0;
			y = aBottom;
		}
		else if ((aCode & 2) != 0)
		{
			x = (ax1 - ax0) * (aTop - ay0) / (ay1 - ay0) + ax0;
			y = aTop;
		}
		else if ((aCode & 4) != 0)
		{
			y = (aRight - ax0) * (ay1 - ay0) / (ax1 - ax0) + ay0;
			x = aRight;
		}
		else
		{
			y = (aLeft - ax0) * (ay1 - ay0) / (ax1 - ax0) + ay0;
			x = aLeft;
		}
		if (aCode == aCode0)
		{
			ax0 = x;
			ay0 = y;
			aCode0 = OutCode(ax0, ay0, aLeft, aRight, aTop, aBottom);
		}
		else
		{
			ax1 = x;
			ay1 = y;
			aCode1 = OutCode(ax1, ay1, aLeft, aRight, aTop, aBottom);
		}
	}
}

// A component of a 16-bit pixel moved theAlpha/256 of the way to theTo (the original's sums wrap as uint32)
inline uint32_t Blend(uint32_t thePixel, uint32_t theTo, uint32_t theMask, uint32_t theAlpha)
{
	return ((theTo - (thePixel & theMask)) * theAlpha >> 8) + (thePixel & theMask) & theMask;
}

// 0x441c50: max(|dx|, |dy|) + 1 pixels, stepping along the longer axis
void Line16(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int x0, int y0, int x1, int y1, uint8_t r,
	uint8_t g, uint8_t b, uint8_t theAlpha)
{
	int aRow = thePitch / 2;
	uint32_t aR = (uint32_t)(r >> theFmt->rLoss) << theFmt->rShift;
	uint32_t aG = (uint32_t)(g >> theFmt->gLoss) << theFmt->gShift;
	int anErr = 0;
	uint32_t aB = (uint32_t)(b >> theFmt->bLoss) << theFmt->bShift;
	uint16_t* p = (uint16_t*)(theBits + thePitch * y0 + x0 * 2);
	int dx = x1 - x0;
	int dy = y1 - y0;
	int aStep;
	if (dx < 0)
	{
		dx = -dx;
		aStep = -1;
	}
	else
		aStep = 1;
	if (dy < 0)
	{
		aRow = -aRow;
		dy = -dy;
	}
	if (dy < dx)
	{
		for (int i = 0; i <= dx; i++)
		{
			uint32_t c = *p;
			anErr = anErr + dy;
			*p = (uint16_t)(Blend(c, aG, theFmt->gMask, theAlpha) | Blend(c, aR, theFmt->rMask, theAlpha) |
				Blend(c, aB, theFmt->bMask, theAlpha));
			if (dx < anErr)
			{
				anErr = anErr - dx;
				p = p + aRow;
			}
			p = p + aStep;
		}
	}
	else
	{
		for (int i = 0; i <= dy; i++)
		{
			uint32_t c = *p;
			anErr = anErr + dx;
			*p = (uint16_t)(Blend(c, aG, theFmt->gMask, theAlpha) | Blend(c, aR, theFmt->rMask, theAlpha) |
				Blend(c, aB, theFmt->bMask, theAlpha));
			if (0 < anErr)
			{
				anErr = anErr - dy;
				p = p + aStep;
			}
			p = p + aRow;
		}
	}
}

// 0x441ea0
void Line16Clipped(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, const int* theClip, int x0, int y0,
	int x1, int y1, int r, int g, int b, int theAlpha)
{
	if (ClipLine(&x0, &y0, &x1, &y1, theClip) != 0)
		Line16(theBits, thePitch, theFmt, x0, y0, x1, y1, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)theAlpha);
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

void Draw_Line(int x0, int y0, int x1, int y1, int r, int g, int b, int theAlpha)
{
	if (gTargetFmt.bpp == 16)
		Line16Clipped(gTargetBits, gTargetPitch, &gTargetFmt, gTargetClip, x0, y0, x1, y1, r, g, b, theAlpha);
}

} // namespace r64
