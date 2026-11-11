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
	return (uint16_t)(BlendComponent(f->rMask, theSrc, theDst, theAlpha) |
		BlendComponent(f->gMask, theSrc, theDst, theAlpha) | BlendComponent(f->bMask, theSrc, theDst, theAlpha));
}

// the additive blitters: each component's sum, at most its mask
inline uint16_t Add(const PixelFormat* f, uint32_t theSrc, uint32_t theDst)
{
	uint32_t r = (theSrc & f->rMask) + (f->rMask & theDst);
	if (f->rMask < r)
		r = f->rMask;
	uint32_t g = (f->gMask & theSrc) + (f->gMask & theDst);
	if (f->gMask < g)
		g = f->gMask;
	uint32_t b = (f->bMask & theSrc) + (f->bMask & theDst);
	if (f->bMask < b)
		b = f->bMask;
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

inline const uint8_t* MaskRow(const uint8_t* theMask, int thePitch, int sx, int sy, int mx, int my)
{
	return theMask + (sy + my) * thePitch + sx + mx;
}

// An RLE mask's runs: the alpha bytes are padded to 4 as an address (the original's {0, 3, 2, 1}[p & 3] + p)
inline const uint32_t* AlignRun(const uint8_t* p)
{
	return (const uint32_t*)(((uintptr_t)p + 3) & ~(uintptr_t)3);
}

} // namespace

// 0x42fe50
void Copy8(uint8_t* dst, int dx, int dy, int dpitch, int, const uint8_t* src, int sx, int sy, int spitch, int,
	int theWidth, int theHeight)
{
	uint8_t* d = dst + dy * dpitch + dx;
	const uint8_t* s = src + sy * spitch + sx;
	if (theWidth != 0 && theHeight != 0)
	{
		do
		{
			memcpy(d, s, theWidth);
			s += spitch;
			d += dpitch;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x42fee0
void Copy8Key(uint8_t* dst, int dx, int dy, int dpitch, int, const uint8_t* src, int sx, int sy, int spitch, int,
	int theWidth, int theHeight)
{
	uint8_t* d = dst + dy * dpitch + dx;
	const uint8_t* s = src + sy * spitch + sx;
	if (theWidth != 0)
	{
		do
		{
			for (int i = 0; i < theWidth; i++)
				if (s[i] != 0)
					d[i] = s[i];
			s += spitch;
			d += dpitch;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x42ffb0
void Copy16(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, int sx, int sy, int w, int theHeight)
{
	const uint8_t* s = src + spitch * sy + sx * 2;
	uint8_t* d = dst + dpitch * dy + dx * 2;
	if (0 < theHeight)
	{
		do
		{
			memcpy(d, s, w * 2);
			s += spitch;
			d += dpitch;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x430020
void Copy16Mask(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, int sx, int sy,
	const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	const uint16_t* s = SrcRow16(src, spitch, sx, sy);
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = s[i];
		s += spitch / 2;
		d += dpitch / 2;
		m += mpitch;
	}
}

// 0x430260
void Fill16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy, const uint8_t* mask,
	int mpitch, int mx, int my, int theWidth, int theHeight, uint16_t colour)
{
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint8_t a = m[i];
			if (a != 0)
				d[i] = a == 0xff ? colour : Blend(fmt, colour, d[i], a);
		}
		d += dpitch / 2;
		m += mpitch;
	}
}

// 0x430870. At alpha 0xff it fills through the mask (0x430260) and then blends the same over it again, as the
// original does (it does not return after the first pass).
void Fill16MaskAlpha(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, int sx, int sy,
	const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight, uint16_t colour, uint8_t alpha)
{
	if (alpha == 0xff)
		Fill16Mask(dst, dpitch, fmt, dx, dy, sx, sy, mask, mpitch, mx, my, theWidth, theHeight, colour);
	uint16_t* aDstPixel = Row16(dst, dpitch, dx, dy);
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint32_t a = (uint32_t)m[i] * alpha >> 8;
			if ((uint8_t)a != 0)
				aDstPixel[i] = Blend(fmt, colour, aDstPixel[i], a);
		}
		m += mpitch;
		aDstPixel += dpitch / 2;
	}
}

// 0x430ef0
void Blend16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	const uint16_t* s = SrcRow16(src, spitch, sx, sy);
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = Blend(fmt, s[i], d[i], m[i]);
		s += spitch / 2;
		m += mpitch;
		d += dpitch / 2;
	}
}

// 0x4315a0
void Alpha16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	const uint16_t* aSrcPixel = SrcRow16(src, spitch, sx, sy);
	uint16_t* aDstPixel = Row16(dst, dpitch, dx, dy);
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint8_t a = m[i];
			if (a != 0)
				aDstPixel[i] = a == 0xff ? aSrcPixel[i] : Blend(fmt, aSrcPixel[i], aDstPixel[i], a);
		}
		aSrcPixel += spitch / 2;
		m += mpitch;
		aDstPixel += dpitch / 2;
	}
}

// 0x431d00. At alpha 0xff it draws Alpha16Mask (0x4315a0) and then blends over it again, as the original does.
void Alpha16MaskFade(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src,
	int spitch, int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight, int alpha)
{
	if (alpha == 0xff)
		Alpha16Mask(dst, dpitch, fmt, dx, dy, src, spitch, sx, sy, mask, mpitch, mx, my, theWidth, theHeight);
	const uint16_t* aSrcPixel = SrcRow16(src, spitch, sx, sy);
	uint16_t* aDstPixel = Row16(dst, dpitch, dx, dy);
	const uint8_t* aMaskPixel = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint32_t a = (uint32_t)aMaskPixel[i] * (uint32_t)alpha >> 8;
			if ((uint8_t)a != 0)
				aDstPixel[i] = Blend(fmt, aSrcPixel[i], aDstPixel[i], a & 0xff);
		}
		aSrcPixel += spitch / 2;
		aDstPixel += dpitch / 2;
		aMaskPixel += mpitch;
	}
}

// 0x432440
void Add16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	const uint16_t* s = SrcRow16(src, spitch, sx, sy);
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = Add(fmt, s[i], d[i]);
		s += spitch / 2;
		d += dpitch / 2;
		m += mpitch;
	}
}

namespace
{

// 0x432970 and 0x434a80 (the body both share). The image's corner is found from its centre (x, y) at the angle
// of its half diagonal; each row then steps along (cos a, -sin a) and each row's start along (cos(a+270),
// -sin(a+270)), both before the pixel is plotted. A pixel lands at its rounded position and also on the row
// above when its y truncates there, which fills the gaps a rotation leaves.
template <typename PlotFn>
void RotateBody(int dstStep, int spitchStep, int theWidth, int theHeight, const uint8_t* mask, int mpitch, int mx, int my, int x,
	int y, int angle, const int* clip, bool theFloatAngle, PlotFn thePlot)
{
	int aHalfW = theWidth / 2;
	int aHalfH = theHeight / 2;
	int x0 = x - aHalfW;
	int y0 = y - aHalfH;
	const uint8_t* aMaskRow = mask + my * mpitch + mx;
	int aDeg;
	if (theFloatAngle) // 0x42d590 rounds the arc tangent and the angle in degrees to floats
		aDeg = (int)(float)(((double)(float)std::atan2((double)(y - y0), (double)(x - x0)) * 180.0) / 3.1415927);
	else
		aDeg = (int)((std::atan2((double)(y - y0), (double)(x - x0)) * 180.0) / 3.1415927);
	double aCornerAngle = (double)(angle - aDeg);
	double dxc = std::fabs((double)x0 - (double)x);
	double dyc = std::fabs((double)y0 - (double)y);
	double r = std::sqrt(dyc * dyc + dxc * dxc);
	int a0 = (int)aCornerAngle;
	double aRowX = (double)x - (double)CosDeg(a0) * r;
	double aRowY = (double)SinDeg(a0) * r + (double)y;
	double aCos = (double)CosDeg(angle);
	double aSin = -(double)SinDeg(angle);
	double aRowCos = (double)CosDeg(angle + 270);
	double aRowSin = -(double)SinDeg(angle + 270);
	int aCol = 0;
	int aSrcRow = 0;
	int aSrc = 0;
	const uint8_t* aMaskPixel = aMaskRow;
	for (int aRows = theHeight; aRows > 0; aRows--)
	{
		double cx = aRowX;
		double cy = aRowY;
		for (int aCols = theWidth; aCols > 0; aCols--)
		{
			cx = cx + aCos;
			cy = cy + aSin;
			if (*aMaskPixel != 0)
			{
				int px = (int)(cx + 0.5);
				int py = (int)(cy + 0.5);
				int py2 = (int)cy;
				if (clip[0] <= px && px <= clip[2] && clip[1] <= py && py <= clip[3])
				{
					int anAt = py * dstStep + px;
					thePlot(anAt, aSrc);
					if (py != py2 && clip[1] <= py2)
						thePlot(anAt - dstStep, aSrc);
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
				aMaskRow += mpitch;
				aMaskPixel = aMaskRow;
				aSrcRow += spitchStep;
				aSrc = aSrcRow;
				aCol = 0;
			}
		}
		aRowX += aRowCos;
		aRowY += aRowSin;
	}
}

} // namespace

void Rotate16(uint8_t* dst, int dpitch, const uint8_t* src, int spitch, int w, int h, const uint8_t* mask,
	int mpitch, int mx, int my, int x, int y, int angle, const int* clip)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* s = (const uint16_t*)src;
	RotateBody(dpitch / 2, spitch / 2, w, h, mask, mpitch, mx, my, x, y, angle, clip, false,
		[&](int theAt, int theSrc) { d[theAt] = s[theSrc]; });
}

void PalRotate16(uint8_t* dst, int dpitch, const uint8_t* src, int spitch, const Palette* pal, int w, int h,
	const uint8_t* mask, int mpitch, int mx, int my, int x, int y, int angle, const int* clip)
{
	uint16_t* d = (uint16_t*)dst;
	RotateBody(dpitch / 2, spitch, w, h, mask, mpitch, mx, my, x, y, angle, clip, false,
		[&](int theAt, int theSrc) { d[theAt] = pal->p16[src[theSrc]]; });
}

// 0x42d590: the source is its own mask (0 transparent)
void Rotate8(uint8_t* dst, int dpitch, const uint8_t* src, int spitch, int w, int h, int x, int y, int angle,
	const int* clip)
{
	RotateBody(dpitch, spitch, w, h, src, spitch, 0, 0, x, y, angle, clip, true,
		[&](int theAt, int theSrc) { dst[theAt] = src[theSrc]; });
}

// 0x432be0
void Pal16(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, const Palette* pal, int sx,
	int sy, int theWidth, int theHeight)
{
	const uint8_t* s = src + spitch * sy + sx;
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint16_t* p = pal->p16;
	if (0 < theHeight)
	{
		do
		{
			for (int i = 0; i < theWidth; i++)
				d[i] = p[s[i]];
			s += spitch;
			d += dpitch / 2;
			theHeight--;
		} while (theHeight != 0);
	}
}

// 0x432c70
void Pal16Mask(uint8_t* dst, int dpitch, int dx, int dy, const uint8_t* src, int spitch, const Palette* pal, int sx,
	int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint16_t* p = pal->p16;
	const uint8_t* s = src + spitch * sy + sx;
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				d[i] = p[s[i]];
		s += spitch;
		d += dpitch / 2;
		m += mpitch;
	}
}

// 0x432ee0
void PalBlend16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	uint16_t* aDstPixel = Row16(dst, dpitch, dx, dy);
	const uint8_t* s = src + spitch * sy + sx;
	const uint16_t* p = pal->p16;
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
			if (m[i] != 0)
				aDstPixel[i] = Blend(fmt, p[s[i]], aDstPixel[i], m[i]);
		m += mpitch;
		s += spitch;
		aDstPixel += dpitch / 2;
	}
}

// 0x4335c0
void PalAlpha16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight)
{
	uint16_t* aDstPixel = Row16(dst, dpitch, dx, dy);
	const uint8_t* aSrcPixel = src + spitch * sy + sx;
	const uint16_t* p = pal->p16;
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint8_t a = m[i];
			if (a != 0)
				aDstPixel[i] = a == 0xff ? p[aSrcPixel[i]] : Blend(fmt, p[aSrcPixel[i]], aDstPixel[i], a);
		}
		m += mpitch;
		aSrcPixel += spitch;
		aDstPixel += dpitch / 2;
	}
}

// 0x433db0. At alpha 0xff it draws PalAlpha16Mask (0x4335c0) and then blends over it again, as the original does.
void PalAlpha16MaskFade(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src,
	int spitch, const Palette* pal, int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int theWidth, int theHeight,
	int alpha)
{
	if (alpha == 0xff)
		PalAlpha16Mask(dst, dpitch, fmt, dx, dy, src, spitch, pal, sx, sy, mask, mpitch, mx, my, theWidth, theHeight);
	uint16_t* aDstPixel = Row16(dst, dpitch, dx, dy);
	const uint8_t* aSrcPixel = src + spitch * sy + sx;
	const uint16_t* p = pal->p16;
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	for (; 0 < theHeight; theHeight--)
	{
		for (int i = 0; i < theWidth; i++)
		{
			uint32_t a = (uint32_t)m[i] * (uint32_t)alpha >> 8;
			if ((uint8_t)a != 0)
				aDstPixel[i] = Blend(fmt, p[aSrcPixel[i]], aDstPixel[i], a & 0xff);
		}
		m += mpitch;
		aSrcPixel += spitch;
		aDstPixel += dpitch / 2;
	}
}

// 0x434520
void PalAdd16Mask(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, int sx, int sy, const uint8_t* mask, int mpitch, int mx, int my, int w, int h)
{
	uint16_t* d = Row16(dst, dpitch, dx, dy);
	const uint16_t* p = pal->p16;
	const uint8_t* m = MaskRow(mask, mpitch, sx, sy, mx, my);
	const uint8_t* s = src + spitch * sy + sx;
	for (; 0 < h; h--)
	{
		for (int i = 0; i < w; i++)
			if (m[i] != 0)
				d[i] = Add(fmt, p[s[i]], d[i]);
		s += spitch;
		d += dpitch / 2;
		m += mpitch;
	}
}

namespace
{

// The RLE blitters' walk (0x434d00 and the others): per row, from its first pixel to its width, runs of skip,
// n blended pixels (theBlended with each one's alpha) and the solid ones (theSolid)
template <typename BlendedFn, typename SolidFn>
void RleWalk(int dpitch, int spitchStep, const uint8_t* rle, BlendedFn theBlended, SolidFn theSolid, int theDst)
{
	const int* aHead = (const int*)rle;
	int aWidth = aHead[1];
	int aRows = aHead[2];
	const uint32_t* aRun = (const uint32_t*)(rle + 0xc);
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
		aRowDst += dpitch / 2;
		aRowSrc += spitchStep;
	}
}

} // namespace

// 0x434d00
void Rle16Copy(uint8_t* dst, int dpitch, const PixelFormat*, int dx, int dy, const uint8_t* src, int spitch,
	const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* s = (const uint16_t*)src;
	RleWalk(dpitch, spitch / 2, rle, [&](int theD, int theS, uint8_t) { d[theD] = s[theS]; },
		[&](int theD, int theS) { d[theD] = s[theS]; }, dy * (dpitch / 2) + dx);
}

// 0x434e20
void Rle16Fill(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* rle, uint16_t colour)
{
	uint16_t* d = (uint16_t*)dst;
	RleWalk(dpitch, 0, rle, [&](int theD, int, uint8_t a) { d[theD] = Blend(fmt, colour, d[theD], a); },
		[&](int theD, int) { d[theD] = colour; }, dy * (dpitch / 2) + dx);
}

// 0x434fe0. At alpha 0xff it fills (0x434e20) and then blends over it again, as the original does.
void Rle16FillAlpha(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* rle,
	uint16_t colour, uint32_t alpha)
{
	if (alpha == 0xff)
		Rle16Fill(dst, dpitch, fmt, dx, dy, rle, colour);
	uint16_t* aDstPixel = (uint16_t*)dst;
	RleWalk(dpitch, 0, rle,
		[&](int theD, int, uint8_t a)
		{
			uint32_t aa = a * alpha;
			if ((uint8_t)(aa >> 8) != 0)
				aDstPixel[theD] = Blend(fmt, colour, aDstPixel[theD], aa >> 8 & 0xff);
		},
		[&](int theD, int)
		{
			if ((uint8_t)alpha != 0)
				aDstPixel[theD] = Blend(fmt, colour, aDstPixel[theD], alpha & 0xff);
		},
		dy * (dpitch / 2) + dx);
}

// 0x435270: the solid pixels are blended at 0xff, not copied
void Rle16Blend(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* s = (const uint16_t*)src;
	RleWalk(dpitch, spitch / 2, rle, [&](int theD, int theS, uint8_t a) { d[theD] = Blend(fmt, s[theS], d[theD], a); },
		[&](int theD, int theS) { d[theD] = Blend(fmt, s[theS], d[theD], 0xff); }, dy * (dpitch / 2) + dx);
}

// 0x435500
void Rle16Alpha(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* s = (const uint16_t*)src;
	RleWalk(dpitch, spitch / 2, rle, [&](int theD, int theS, uint8_t a) { d[theD] = Blend(fmt, s[theS], d[theD], a); },
		[&](int theD, int theS) { d[theD] = s[theS]; }, dy * (dpitch / 2) + dx);
}

// 0x435700. At alpha 0xff it draws Rle16Alpha (0x435500) and then blends over it again, as the original does.
void Rle16AlphaFade(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const uint8_t* rle, uint32_t alpha)
{
	if (alpha == 0xff)
		Rle16Alpha(dst, dpitch, fmt, dx, dy, src, spitch, rle);
	uint16_t* aDstPixel = (uint16_t*)dst;
	const uint16_t* aSrcPixel = (const uint16_t*)src;
	RleWalk(dpitch, spitch / 2, rle,
		[&](int theD, int theS, uint8_t a)
		{
			uint32_t aa = a * alpha;
			if ((uint8_t)(aa >> 8) != 0)
				aDstPixel[theD] = Blend(fmt, aSrcPixel[theS], aDstPixel[theD], aa >> 8 & 0xff);
		},
		[&](int theD, int theS)
		{
			if ((uint8_t)alpha != 0)
				aDstPixel[theD] = Blend(fmt, aSrcPixel[theS], aDstPixel[theD], alpha & 0xff);
		},
		dy * (dpitch / 2) + dx);
}

// 0x4359e0
void Rle16Add(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* s = (const uint16_t*)src;
	RleWalk(dpitch, spitch / 2, rle, [&](int theD, int theS, uint8_t) { d[theD] = Add(fmt, s[theS], d[theD]); },
		[&](int theD, int theS) { d[theD] = Add(fmt, s[theS], d[theD]); }, dy * (dpitch / 2) + dx);
}

// 0x435bf0
void RlePal16Copy(uint8_t* dst, int dpitch, const PixelFormat*, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* p = pal->p16;
	RleWalk(dpitch, spitch, rle, [&](int theD, int theS, uint8_t) { d[theD] = p[src[theS]]; },
		[&](int theD, int theS) { d[theD] = p[src[theS]]; }, dy * (dpitch / 2) + dx);
}

// 0x435d10: the solid pixels are blended at 0xff, not copied
void RlePal16Blend(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* p = pal->p16;
	RleWalk(dpitch, spitch, rle, [&](int theD, int theS, uint8_t a) { d[theD] = Blend(fmt, p[src[theS]], d[theD], a); },
		[&](int theD, int theS) { d[theD] = Blend(fmt, p[src[theS]], d[theD], 0xff); }, dy * (dpitch / 2) + dx);
}

// 0x435fb0
void RlePal16Alpha(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* p = pal->p16;
	RleWalk(dpitch, spitch, rle, [&](int theD, int theS, uint8_t a) { d[theD] = Blend(fmt, p[src[theS]], d[theD], a); },
		[&](int theD, int theS) { d[theD] = p[src[theS]]; }, dy * (dpitch / 2) + dx);
}

// 0x4361b0. At alpha 0xff it draws RlePal16Alpha (0x435fb0) and then blends over it again, as the original does.
void RlePal16AlphaFade(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src,
	int spitch, const Palette* pal, const uint8_t* rle, uint32_t alpha)
{
	if (alpha == 0xff)
		RlePal16Alpha(dst, dpitch, fmt, dx, dy, src, spitch, pal, rle);
	uint16_t* aDstPixel = (uint16_t*)dst;
	const uint16_t* aPalette = pal->p16;
	RleWalk(dpitch, spitch, rle,
		[&](int theD, int theS, uint8_t a)
		{
			uint32_t aa = a * alpha;
			if ((uint8_t)(aa >> 8) != 0)
				aDstPixel[theD] = Blend(fmt, aPalette[src[theS]], aDstPixel[theD], aa >> 8 & 0xff);
		},
		[&](int theD, int theS)
		{
			if ((uint8_t)alpha != 0)
				aDstPixel[theD] = Blend(fmt, aPalette[src[theS]], aDstPixel[theD], alpha & 0xff);
		},
		dy * (dpitch / 2) + dx);
}

// 0x436490
void RlePal16Add(uint8_t* dst, int dpitch, const PixelFormat* fmt, int dx, int dy, const uint8_t* src, int spitch,
	const Palette* pal, const uint8_t* rle)
{
	uint16_t* d = (uint16_t*)dst;
	const uint16_t* p = pal->p16;
	RleWalk(dpitch, spitch, rle, [&](int theD, int theS, uint8_t) { d[theD] = Add(fmt, p[src[theS]], d[theD]); },
		[&](int theD, int theS) { d[theD] = Add(fmt, p[src[theS]], d[theD]); }, dy * (dpitch / 2) + dx);
}

} // namespace r64
