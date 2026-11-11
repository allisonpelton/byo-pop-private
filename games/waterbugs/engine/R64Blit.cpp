// Retro64's 8- and 16-bit blitters (see R64Blit.h).
//
// From 12 pixels wide the original splits each row at the mask's 4-byte boundaries and takes the middle four mask
// bytes at a time, skipping a group whose four bytes are all 0 (and copying it whole when all are 0xff). Per pixel
// that gives what its loop for narrower rows gives, which is what is translated here: one pixel at a time.
//
// A blend computes each component as ((c & m) - (d & m)) * a >> 8, plus (d & m), masked with m, in unsigned 32-bit
// arithmetic with a wrap below 0, as the original does.

#include "R64Blit.h"
#include "R64.h"

#include <cmath>
#include <cstring>

namespace r64
{

namespace
{

inline uint32_t BlendComponent(uint32_t theMask, uint32_t theSrc, uint32_t theDst, uint32_t theAlpha)
{
	return ((((theMask & theSrc) - (theMask & theDst)) * theAlpha >> 8) + (theDst & theMask)) & theMask;
}

inline uint16_t Blend(const PixelFormat* f, uint32_t theSrc, uint32_t theDst, uint32_t theAlpha)
{
	return (uint16_t)(BlendComponent(f->mRMask, theSrc, theDst, theAlpha) |
		BlendComponent(f->mGMask, theSrc, theDst, theAlpha) | BlendComponent(f->mBMask, theSrc, theDst, theAlpha));
}

// the additive blitters: each component's sum, at most its mask
inline uint16_t Add(const PixelFormat* f, uint32_t theSrc, uint32_t theDst)
{
	uint32_t r = (theSrc & f->mRMask) + (f->mRMask & theDst);
	if (f->mRMask < r)
		r = f->mRMask;
	uint32_t g = (f->mGMask & theSrc) + (f->mGMask & theDst);
	if (f->mGMask < g)
		g = f->mGMask;
	uint32_t b = (f->mBMask & theSrc) + (f->mBMask & theDst);
	if (f->mBMask < b)
		b = f->mBMask;
	return (uint16_t)(b | r | g);
}

inline uint16_t* Row16(uint8_t* theBits, int thePitch, int x, int y)
{
	return (uint16_t*)(theBits + thePitch * y + x * 2);
}

inline const uint16_t* SrcRow16(const uint8_t* theBits, int thePitch, int x, int y)
{
	return (const uint16_t*)(theBits + thePitch * y + x * 2);
}

inline const uint8_t* MaskRow(const uint8_t* theMask, int thePitch, int theSrcX, int theSrcY, int theMaskX,
	int theMaskY)
{
	return theMask + (theSrcY + theMaskY) * thePitch + theSrcX + theMaskX;
}

// An RLE mask's runs: the alpha bytes are padded to 4 as an address (the original's {0, 3, 2, 1}[p & 3] + p)
inline const uint32_t* AlignRun(const uint8_t* p)
{
	return (const uint32_t*)(((uintptr_t)p + 3) & ~(uintptr_t)3);
}

} // namespace

// 0x42fe50
void Copy8(uint8_t* theDst, int theDstX, int theDstY, int theDstPitch, int, const uint8_t* theSrc, int theSrcX,
	int theSrcY, int theSrcPitch, int, int theWidth, int theHeight)
{
	uint8_t* d = theDst + theDstY * theDstPitch + theDstX;
	const uint8_t* s = theSrc + theSrcY * theSrcPitch + theSrcX;
	if (theWidth != 0 && theHeight != 0)
	{
		do
		{
			memcpy(d, s, theWidth);
			s += theSrcPitch;
			d += theDstPitch;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x42fee0
void Copy8Key(uint8_t* theDst, int theDstX, int theDstY, int theDstPitch, int, const uint8_t* theSrc, int theSrcX,
	int theSrcY, int theSrcPitch, int, int theWidth, int theHeight)
{
	uint8_t* d = theDst + theDstY * theDstPitch + theDstX;
	const uint8_t* s = theSrc + theSrcY * theSrcPitch + theSrcX;
	if (theWidth != 0)
	{
		do
		{
			for (int i = 0; i < theWidth; i++)
				if (s[i] != 0)
					d[i] = s[i];
			s += theSrcPitch;
			d += theDstPitch;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x42ffb0
void Copy16(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	int theSrcX, int theSrcY, int theWidth, int theHeight)
{
	const uint8_t* s = theSrc + theSrcPitch * theSrcY + theSrcX * 2;
	uint8_t* d = theDst + theDstPitch * theDstY + theDstX * 2;
	if (0 < theHeight)
	{
		do
		{
			memcpy(d, s, theWidth * 2);
			s += theSrcPitch;
			d += theDstPitch;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x430020
void Copy16Mask(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth,
	int theHeight)
{
	const uint16_t* s = SrcRow16(theSrc, theSrcPitch, theSrcX, theSrcY);
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = s[i];
		s += theSrcPitch / 2;
		d += theDstPitch / 2;
		m += theMaskPitch;
	}
}

// 0x430260
void Fill16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX,
	int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight,
	uint16_t theColour)
{
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint8_t a = m[i];
			if (a != 0)
				d[i] = a == 0xff ? theColour : Blend(theFmt, theColour, d[i], a);
		}
		d += theDstPitch / 2;
		m += theMaskPitch;
	}
}

// 0x430870. At alpha 0xff it fills through the mask (0x430260) and then blends the same over it again, as the
// original does (it does not return after the first pass).
void Fill16MaskAlpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY, int theSrcX,
	int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight,
	uint16_t theColour, uint8_t theAlpha)
{
	if (theAlpha == 0xff)
		Fill16Mask(theDst, theDstPitch, theFmt, theDstX, theDstY, theSrcX, theSrcY, theMask, theMaskPitch, theMaskX,
			theMaskY, theWidth, theHeight, theColour);
	uint16_t* aDstPixel = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint32_t a = (uint32_t)m[i] * theAlpha >> 8;
			if ((uint8_t)a != 0)
				aDstPixel[i] = Blend(theFmt, theColour, aDstPixel[i], a);
		}
		m += theMaskPitch;
		aDstPixel += theDstPitch / 2;
	}
}

// 0x430ef0
void Blend16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch,
	int theMaskX, int theMaskY, int theWidth, int theHeight)
{
	const uint16_t* s = SrcRow16(theSrc, theSrcPitch, theSrcX, theSrcY);
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = Blend(theFmt, s[i], d[i], m[i]);
		s += theSrcPitch / 2;
		m += theMaskPitch;
		d += theDstPitch / 2;
	}
}

// 0x4315a0
void Alpha16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch,
	int theMaskX, int theMaskY, int theWidth, int theHeight)
{
	const uint16_t* aSrcPixel = SrcRow16(theSrc, theSrcPitch, theSrcX, theSrcY);
	uint16_t* aDstPixel = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint8_t a = m[i];
			if (a != 0)
				aDstPixel[i] = a == 0xff ? aSrcPixel[i] : Blend(theFmt, aSrcPixel[i], aDstPixel[i], a);
		}
		aSrcPixel += theSrcPitch / 2;
		m += theMaskPitch;
		aDstPixel += theDstPitch / 2;
	}
}

// 0x431d00. At alpha 0xff it draws Alpha16Mask (0x4315a0) and then blends over it again, as the original does.
void Alpha16MaskFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch,
	int theMaskX, int theMaskY, int theWidth, int theHeight, int theAlpha)
{
	if (theAlpha == 0xff)
		Alpha16Mask(theDst, theDstPitch, theFmt, theDstX, theDstY, theSrc, theSrcPitch, theSrcX, theSrcY, theMask,
			theMaskPitch, theMaskX, theMaskY, theWidth, theHeight);
	const uint16_t* aSrcPixel = SrcRow16(theSrc, theSrcPitch, theSrcX, theSrcY);
	uint16_t* aDstPixel = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* aMaskPixel = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint32_t a = (uint32_t)aMaskPixel[i] * (uint32_t)theAlpha >> 8;
			if ((uint8_t)a != 0)
				aDstPixel[i] = Blend(theFmt, aSrcPixel[i], aDstPixel[i], a & 0xff);
		}
		aSrcPixel += theSrcPitch / 2;
		aDstPixel += theDstPitch / 2;
		aMaskPixel += theMaskPitch;
	}
}

// 0x432440
void Add16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch,
	int theMaskX, int theMaskY, int theWidth, int theHeight)
{
	const uint16_t* s = SrcRow16(theSrc, theSrcPitch, theSrcX, theSrcY);
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = Add(theFmt, s[i], d[i]);
		s += theSrcPitch / 2;
		d += theDstPitch / 2;
		m += theMaskPitch;
	}
}

namespace
{

// 0x432970 and 0x434a80 (the body both share). The image's corner is found from its centre (x, y) at the angle
// of its half diagonal; each row then steps along (cos a, -sin a) and each row's start along (cos(a+270),
// -sin(a+270)), both before the pixel is plotted. A pixel lands at its rounded position and also on the row
// above when its y truncates there, which fills the gaps a rotation leaves.
template <typename PlotFn>
void RotateBody(int theDstStep, int theSpitchStep, int theWidth, int theHeight, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int x, int y, int theAngle, const int* theClip, bool theFloatAngle,
	PlotFn thePlot)
{
	int aHalfW = theWidth / 2;
	int aHalfH = theHeight / 2;
	int aX0 = x - aHalfW;
	int aY0 = y - aHalfH;
	const uint8_t* aMaskRow = theMask + theMaskY * theMaskPitch + theMaskX;
	int aDeg;
	if (theFloatAngle) // 0x42d590 rounds the arc tangent and the angle in degrees to floats
		aDeg = (int)(float)(((double)(float)std::atan2((double)(y - aY0), (double)(x - aX0)) * 180.0) / 3.1415927);
	else
		aDeg = (int)((std::atan2((double)(y - aY0), (double)(x - aX0)) * 180.0) / 3.1415927);
	double aCornerAngle = (double)(theAngle - aDeg);
	double aDxc = std::fabs((double)aX0 - (double)x);
	double aDyc = std::fabs((double)aY0 - (double)y);
	double r = std::sqrt(aDyc * aDyc + aDxc * aDxc);
	int anA0 = (int)aCornerAngle;
	double aRowX = (double)x - (double)CosDeg(anA0) * r;
	double aRowY = (double)SinDeg(anA0) * r + (double)y;
	double aCos = (double)CosDeg(theAngle);
	double aSin = -(double)SinDeg(theAngle);
	double aRowCos = (double)CosDeg(theAngle + 270);
	double aRowSin = -(double)SinDeg(theAngle + 270);
	int aCol = 0;
	int aSrcRow = 0;
	int aSrc = 0;
	const uint8_t* aMaskPixel = aMaskRow;
	for (int aRows = theHeight; aRows > 0; aRows--)
	{
		double aCx = aRowX;
		double aCy = aRowY;
		for (int aCols = theWidth; aCols > 0; aCols--)
		{
			aCx = aCx + aCos;
			aCy = aCy + aSin;
			if (*aMaskPixel != 0)
			{
				int aPx = (int)(aCx + 0.5);
				int aPy = (int)(aCy + 0.5);
				int aPy2 = (int)aCy;
				if (theClip[0] <= aPx && aPx <= theClip[2] && theClip[1] <= aPy && aPy <= theClip[3])
				{
					int anAt = aPy * theDstStep + aPx;
					thePlot(anAt, aSrc);
					if (aPy != aPy2 && theClip[1] <= aPy2)
						thePlot(anAt - theDstStep, aSrc);
				}
			}
			aCol++;
			if (aCol < theWidth)
			{
				aSrc++;
				aMaskPixel++;
			}
			else
			{
				aMaskRow += theMaskPitch;
				aMaskPixel = aMaskRow;
				aSrcRow += theSpitchStep;
				aSrc = aSrcRow;
				aCol = 0;
			}
		}
		aRowX += aRowCos;
		aRowY += aRowSin;
	}
}

} // namespace

void Rotate16(uint8_t* theDst, int theDstPitch, const uint8_t* theSrc, int theSrcPitch, int theWidth, int theHeight,
	const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int x, int y, int theAngle,
	const int* theClip)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* s = (const uint16_t*)theSrc;
	RotateBody(theDstPitch / 2, theSrcPitch / 2, theWidth, theHeight, theMask, theMaskPitch, theMaskX, theMaskY, x, y,
		theAngle, theClip, false, [&](int theAt, int theSrcIndex) { d[theAt] = s[theSrcIndex]; });
}

void PalRotate16(uint8_t* theDst, int theDstPitch, const uint8_t* theSrc, int theSrcPitch, const Palette* thePal,
	int theWidth, int theHeight, const uint8_t* theMask, int theMaskPitch, int theMaskX, int theMaskY, int x, int y,
	int theAngle, const int* theClip)
{
	uint16_t* d = (uint16_t*)theDst;
	RotateBody(theDstPitch / 2, theSrcPitch, theWidth, theHeight, theMask, theMaskPitch, theMaskX, theMaskY, x, y,
		theAngle, theClip, false, [&](int theAt, int theSrcIndex) { d[theAt] = thePal->mP16[theSrc[theSrcIndex]]; });
}

// 0x42d590: the source is its own mask (0 transparent)
void Rotate8(uint8_t* theDst, int theDstPitch, const uint8_t* theSrc, int theSrcPitch, int theWidth, int theHeight,
	int x, int y, int theAngle, const int* theClip)
{
	RotateBody(theDstPitch, theSrcPitch, theWidth, theHeight, theSrc, theSrcPitch, 0, 0, x, y, theAngle, theClip, true,
		[&](int theAt, int theSrcIndex) { theDst[theAt] = theSrc[theSrcIndex]; });
}

// 0x432be0
void Pal16(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, int theSrcX, int theSrcY, int theWidth, int theHeight)
{
	const uint8_t* s = theSrc + theSrcPitch * theSrcY + theSrcX;
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint16_t* p = thePal->mP16;
	if (0 < theHeight)
	{
		do
		{
			for (int i = 0; i < theWidth; i++)
				d[i] = p[s[i]];
			s += theSrcPitch;
			d += theDstPitch / 2;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x432c70
void Pal16Mask(uint8_t* theDst, int theDstPitch, int theDstX, int theDstY, const uint8_t* theSrc, int theSrcPitch,
	const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask, int theMaskPitch, int theMaskX,
	int theMaskY, int theWidth, int theHeight)
{
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint16_t* p = thePal->mP16;
	const uint8_t* s = theSrc + theSrcPitch * theSrcY + theSrcX;
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = p[s[i]];
		s += theSrcPitch;
		d += theDstPitch / 2;
		m += theMaskPitch;
	}
}

// 0x432ee0
void PalBlend16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight)
{
	uint16_t* aDstPixel = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* s = theSrc + theSrcPitch * theSrcY + theSrcX;
	const uint16_t* p = thePal->mP16;
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				aDstPixel[i] = Blend(theFmt, p[s[i]], aDstPixel[i], m[i]);
		m += theMaskPitch;
		s += theSrcPitch;
		aDstPixel += theDstPitch / 2;
	}
}

// 0x4335c0
void PalAlpha16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight)
{
	uint16_t* aDstPixel = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* aSrcPixel = theSrc + theSrcPitch * theSrcY + theSrcX;
	const uint16_t* p = thePal->mP16;
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint8_t a = m[i];
			if (a != 0)
				aDstPixel[i] = a == 0xff ? p[aSrcPixel[i]] : Blend(theFmt, p[aSrcPixel[i]], aDstPixel[i], a);
		}
		m += theMaskPitch;
		aSrcPixel += theSrcPitch;
		aDstPixel += theDstPitch / 2;
	}
}

// 0x433db0. At alpha 0xff it draws PalAlpha16Mask (0x4335c0) and then blends over it again, as the original does.
void PalAlpha16MaskFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight, int theAlpha)
{
	if (theAlpha == 0xff)
		PalAlpha16Mask(theDst, theDstPitch, theFmt, theDstX, theDstY, theSrc, theSrcPitch, thePal, theSrcX, theSrcY,
			theMask, theMaskPitch, theMaskX, theMaskY, theWidth, theHeight);
	uint16_t* aDstPixel = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint8_t* aSrcPixel = theSrc + theSrcPitch * theSrcY + theSrcX;
	const uint16_t* p = thePal->mP16;
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint32_t a = (uint32_t)m[i] * (uint32_t)theAlpha >> 8;
			if ((uint8_t)a != 0)
				aDstPixel[i] = Blend(theFmt, p[aSrcPixel[i]], aDstPixel[i], a & 0xff);
		}
		m += theMaskPitch;
		aSrcPixel += theSrcPitch;
		aDstPixel += theDstPitch / 2;
	}
}

// 0x434520
void PalAdd16Mask(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, int theSrcX, int theSrcY, const uint8_t* theMask,
	int theMaskPitch, int theMaskX, int theMaskY, int theWidth, int theHeight)
{
	uint16_t* d = Row16(theDst, theDstPitch, theDstX, theDstY);
	const uint16_t* p = thePal->mP16;
	const uint8_t* m = MaskRow(theMask, theMaskPitch, theSrcX, theSrcY, theMaskX, theMaskY);
	const uint8_t* s = theSrc + theSrcPitch * theSrcY + theSrcX;
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = Add(theFmt, p[s[i]], d[i]);
		s += theSrcPitch;
		d += theDstPitch / 2;
		m += theMaskPitch;
	}
}

namespace
{

// The RLE blitters' walk (0x434d00 and the others): per row, from its first pixel to its width, runs of skip,
// n blended pixels (theBlended with each one's alpha) and the solid ones (theSolid)
template <typename BlendedFn, typename SolidFn>
void RleWalk(int theDstPitch, int theSpitchStep, const uint8_t* theRle, BlendedFn theBlended, SolidFn theSolid,
	int theDst)
{
	const int* aHead = (const int*)theRle;
	int aWidth = aHead[1];
	int aRows = aHead[2];
	const uint32_t* aRun = (const uint32_t*)(theRle + 0xc);
	int aRowDst = theDst;
	int aRowSrc = 0;
	for (; aRows != 0; aRows--)
	{
		int aDstPos = aRowDst;
		int aSrcPos = aRowSrc;
		int anEnd = aRowDst + aWidth;
		while (aDstPos < anEnd)
		{
			aDstPos += (int)aRun[0];
			aSrcPos += (int)aRun[0];
			uint32_t n = aRun[1];
			const uint8_t* a = (const uint8_t*)(aRun + 2);
			for (uint32_t i = 0; i < n; i++)
				theBlended(aDstPos++, aSrcPos++, a[i]);
			aRun = AlignRun(a + n);
			uint32_t aSolid = *aRun;
			for (uint32_t i = 0; i < aSolid; i++)
				theSolid(aDstPos++, aSrcPos++);
			aRun++;
		}
		aRowDst += theDstPitch / 2;
		aRowSrc += theSpitchStep;
	}
}

} // namespace

// 0x434d00
void Rle16Copy(uint8_t* theDst, int theDstPitch, const PixelFormat*, int theDstX, int theDstY, const uint8_t* theSrc,
	int theSrcPitch, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* s = (const uint16_t*)theSrc;
	RleWalk(theDstPitch, theSrcPitch / 2, theRle, [&](int theD, int theS, uint8_t) { d[theD] = s[theS]; },
		[&](int theD, int theS) { d[theD] = s[theS]; }, theDstY * (theDstPitch / 2) + theDstX);
}

// 0x434e20
void Rle16Fill(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theRle, uint16_t theColour)
{
	uint16_t* d = (uint16_t*)theDst;
	RleWalk(theDstPitch, 0, theRle, [&](int theD, int, uint8_t a) { d[theD] = Blend(theFmt, theColour, d[theD], a); },
		[&](int theD, int) { d[theD] = theColour; }, theDstY * (theDstPitch / 2) + theDstX);
}

// 0x434fe0. At alpha 0xff it fills (0x434e20) and then blends over it again, as the original does.
void Rle16FillAlpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theRle, uint16_t theColour, uint32_t theAlpha)
{
	if (theAlpha == 0xff)
		Rle16Fill(theDst, theDstPitch, theFmt, theDstX, theDstY, theRle, theColour);
	uint16_t* aDstPixel = (uint16_t*)theDst;
	RleWalk(theDstPitch, 0, theRle,
		[&](int theD, int, uint8_t a)
		{
			uint32_t anAa = a * theAlpha;
			if ((uint8_t)(anAa >> 8) != 0)
				aDstPixel[theD] = Blend(theFmt, theColour, aDstPixel[theD], anAa >> 8 & 0xff);
		},
		[&](int theD, int)
		{
			if ((uint8_t)theAlpha != 0)
				aDstPixel[theD] = Blend(theFmt, theColour, aDstPixel[theD], theAlpha & 0xff);
		},
		theDstY * (theDstPitch / 2) + theDstX);
}

// 0x435270: the solid pixels are blended at 0xff, not copied
void Rle16Blend(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* s = (const uint16_t*)theSrc;
	RleWalk(theDstPitch, theSrcPitch / 2, theRle,
		[&](int theD, int theS, uint8_t a) { d[theD] = Blend(theFmt, s[theS], d[theD], a); },
		[&](int theD, int theS) { d[theD] = Blend(theFmt, s[theS], d[theD], 0xff); },
		theDstY * (theDstPitch / 2) + theDstX);
}

// 0x435500
void Rle16Alpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* s = (const uint16_t*)theSrc;
	RleWalk(theDstPitch, theSrcPitch / 2, theRle,
		[&](int theD, int theS, uint8_t a) { d[theD] = Blend(theFmt, s[theS], d[theD], a); },
		[&](int theD, int theS) { d[theD] = s[theS]; }, theDstY * (theDstPitch / 2) + theDstX);
}

// 0x435700. At alpha 0xff it draws Rle16Alpha (0x435500) and then blends over it again, as the original does.
void Rle16AlphaFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const uint8_t* theRle, uint32_t theAlpha)
{
	if (theAlpha == 0xff)
		Rle16Alpha(theDst, theDstPitch, theFmt, theDstX, theDstY, theSrc, theSrcPitch, theRle);
	uint16_t* aDstPixel = (uint16_t*)theDst;
	const uint16_t* aSrcPixel = (const uint16_t*)theSrc;
	RleWalk(theDstPitch, theSrcPitch / 2, theRle,
		[&](int theD, int theS, uint8_t a)
		{
			uint32_t anAa = a * theAlpha;
			if ((uint8_t)(anAa >> 8) != 0)
				aDstPixel[theD] = Blend(theFmt, aSrcPixel[theS], aDstPixel[theD], anAa >> 8 & 0xff);
		},
		[&](int theD, int theS)
		{
			if ((uint8_t)theAlpha != 0)
				aDstPixel[theD] = Blend(theFmt, aSrcPixel[theS], aDstPixel[theD], theAlpha & 0xff);
		},
		theDstY * (theDstPitch / 2) + theDstX);
}

// 0x4359e0
void Rle16Add(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* s = (const uint16_t*)theSrc;
	RleWalk(theDstPitch, theSrcPitch / 2, theRle,
		[&](int theD, int theS, uint8_t) { d[theD] = Add(theFmt, s[theS], d[theD]); },
		[&](int theD, int theS) { d[theD] = Add(theFmt, s[theS], d[theD]); }, theDstY * (theDstPitch / 2) + theDstX);
}

// 0x435bf0
void RlePal16Copy(uint8_t* theDst, int theDstPitch, const PixelFormat*, int theDstX, int theDstY, const uint8_t* theSrc,
	int theSrcPitch, const Palette* thePal, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* p = thePal->mP16;
	RleWalk(theDstPitch, theSrcPitch, theRle, [&](int theD, int theS, uint8_t) { d[theD] = p[theSrc[theS]]; },
		[&](int theD, int theS) { d[theD] = p[theSrc[theS]]; }, theDstY * (theDstPitch / 2) + theDstX);
}

// 0x435d10: the solid pixels are blended at 0xff, not copied
void RlePal16Blend(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* p = thePal->mP16;
	RleWalk(theDstPitch, theSrcPitch, theRle,
		[&](int theD, int theS, uint8_t a) { d[theD] = Blend(theFmt, p[theSrc[theS]], d[theD], a); },
		[&](int theD, int theS) { d[theD] = Blend(theFmt, p[theSrc[theS]], d[theD], 0xff); },
		theDstY * (theDstPitch / 2) + theDstX);
}

// 0x435fb0
void RlePal16Alpha(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* p = thePal->mP16;
	RleWalk(theDstPitch, theSrcPitch, theRle,
		[&](int theD, int theS, uint8_t a) { d[theD] = Blend(theFmt, p[theSrc[theS]], d[theD], a); },
		[&](int theD, int theS) { d[theD] = p[theSrc[theS]]; }, theDstY * (theDstPitch / 2) + theDstX);
}

// 0x4361b0. At alpha 0xff it draws RlePal16Alpha (0x435fb0) and then blends over it again, as the original does.
void RlePal16AlphaFade(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, const uint8_t* theRle, uint32_t theAlpha)
{
	if (theAlpha == 0xff)
		RlePal16Alpha(theDst, theDstPitch, theFmt, theDstX, theDstY, theSrc, theSrcPitch, thePal, theRle);
	uint16_t* aDstPixel = (uint16_t*)theDst;
	const uint16_t* aPalette = thePal->mP16;
	RleWalk(theDstPitch, theSrcPitch, theRle,
		[&](int theD, int theS, uint8_t a)
		{
			uint32_t anAa = a * theAlpha;
			if ((uint8_t)(anAa >> 8) != 0)
				aDstPixel[theD] = Blend(theFmt, aPalette[theSrc[theS]], aDstPixel[theD], anAa >> 8 & 0xff);
		},
		[&](int theD, int theS)
		{
			if ((uint8_t)theAlpha != 0)
				aDstPixel[theD] = Blend(theFmt, aPalette[theSrc[theS]], aDstPixel[theD], theAlpha & 0xff);
		},
		theDstY * (theDstPitch / 2) + theDstX);
}

// 0x436490
void RlePal16Add(uint8_t* theDst, int theDstPitch, const PixelFormat* theFmt, int theDstX, int theDstY,
	const uint8_t* theSrc, int theSrcPitch, const Palette* thePal, const uint8_t* theRle)
{
	uint16_t* d = (uint16_t*)theDst;
	const uint16_t* p = thePal->mP16;
	RleWalk(theDstPitch, theSrcPitch, theRle,
		[&](int theD, int theS, uint8_t) { d[theD] = Add(theFmt, p[theSrc[theS]], d[theD]); },
		[&](int theD, int theS) { d[theD] = Add(theFmt, p[theSrc[theS]], d[theD]); },
		theDstY * (theDstPitch / 2) + theDstX);
}

} // namespace r64
