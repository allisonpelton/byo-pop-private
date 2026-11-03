#include "Blit.h"

#include <cstdlib>

namespace jb
{
namespace blit
{

// 0x451178
uint8_t* Malloc32(int theWidth, int theHeight, int* thePitch)
{
	if (thePitch != nullptr)
	{
		if (theWidth < 1)
			*thePitch = 0;
		else
		{
			if (0 < theHeight)
			{
				int aPitch = (theWidth * 4 + 0x1f) & ~0x1f;
				*thePitch = aPitch;
				return (uint8_t*)malloc((size_t)aPitch * theHeight);
			}
			*thePitch = 0;
		}
	}
	return nullptr;
}

// 0x4511fc
uint8_t* Malloc24(int theWidth, int theHeight, int* thePitch)
{
	if (thePitch != nullptr)
	{
		if (theWidth < 1)
			*thePitch = 0;
		else
		{
			if (0 < theHeight)
			{
				int aPitch = (theWidth * 3 + 0x1f) & ~0x1f;
				*thePitch = aPitch;
				return (uint8_t*)malloc((size_t)aPitch * theHeight);
			}
			*thePitch = 0;
		}
	}
	return nullptr;
}

// 0x455974
void Free(uint8_t* thePixels)
{
	free(thePixels);
}

// 0x4556e0
void PremultiplyRow(const uint8_t* theSrc, uint8_t* theDst, int theWidth)
{
	int i = 0;
	if (0 < theWidth * 4)
	{
		do
		{
			theDst[i] = (uint8_t)Div255((uint32_t)theSrc[i + 3] * theSrc[i]);
			theDst[i + 1] = (uint8_t)Div255((uint32_t)theSrc[i + 3] * theSrc[i + 1]);
			theDst[i + 2] = (uint8_t)Div255((uint32_t)theSrc[i + 2] * theSrc[i + 3]);
			theDst[i + 3] = theSrc[i + 3];
			i = i + 4;
		} while (i < theWidth * 4);
	}
}

// 0x455780
int Premultiply(uint8_t* thePixels, int thePitch, int theWidth, int theHeight)
{
	if (thePixels == nullptr)
		return -8;
	if (0 < theWidth && 0 < theHeight)
	{
		if (thePitch < 1)
			return -14;
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				PremultiplyRow(thePixels, thePixels, theWidth);
				thePixels = thePixels + thePitch;
				y = y + 1;
			} while (y < theHeight);
		}
		return 0;
	}
	return -6;
}

// 0x454400
void OverRow(const uint8_t* theSrc, const uint8_t* theUnder, uint8_t* theDst, int theWidth, bool theSaturate)
{
	int i = 0;
	if (!theSaturate)
	{
		if (0 < theWidth * 4)
		{
			do
			{
				uint32_t anAlpha = theSrc[i + 3];
				uint32_t anUnderAlpha = theUnder[i + 3];
				int c = 0;
				do
				{
					uint32_t aSrc = theSrc[i + c] * anAlpha;
					uint32_t anUnder = Div255(theUnder[i + c] * anUnderAlpha);
					anUnder = anUnder * (0xff - anAlpha);
					theDst[i + c] = (uint8_t)(Div255(aSrc) + Div255(anUnder));
					c = c + 1;
				} while (c < 3);
				uint32_t aRest = anUnderAlpha * (0xff - anAlpha);
				theDst[i + 3] = (uint8_t)(anAlpha + Div255(aRest));
				i = i + 4;
			} while (i < theWidth * 4);
		}
	}
	else if (0 < theWidth * 4)
	{
		do
		{
			uint32_t anAlpha = theSrc[i + 3];
			uint32_t anUnderAlpha = theUnder[i + 3];
			int c = 0;
			do
			{
				uint32_t anUnder = theUnder[i + c] * (0xff - anAlpha);
				int aSum = (int)(theSrc[i + c] + Div255(anUnder));
				theDst[i + c] = 0xff < aSum ? 0xff : (uint8_t)aSum;
				c = c + 1;
			} while (c < 3);
			uint32_t aRest = anUnderAlpha * (0xff - anAlpha);
			theDst[i + 3] = (uint8_t)(anAlpha + Div255(aRest));
			i = i + 4;
		} while (i < theWidth * 4);
	}
}

// 0x455338
int Blend(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theUnder, int theUnderPitch, uint8_t* theDst,
	int theDstPitch, int theWidth, int theHeight, int theMode)
{
	if (theSrc == nullptr || theUnder == nullptr || theDst == nullptr)
		return -8;
	if (theWidth < 1 || theHeight < 1)
		return -6;
	if (0 < theSrcPitch && 0 < theUnderPitch && 0 < theDstPitch)
	{
		if (theMode != 6)
			return -50;
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				OverRow(theSrc, theUnder, theDst, theWidth, true);
				theSrc = theSrc + theSrcPitch;
				y = y + 1;
				theUnder = theUnder + theUnderPitch;
				theDst = theDst + theDstPitch;
			} while (y < theHeight);
		}
		return 0;
	}
	return -14;
}

// 0x4557f4 (the original copies three pixels a step while 16 bytes remain; the bytes are the same)
int CopyRgb(const uint8_t* theSrc, int theSrcPitch, uint8_t* theDst, int theDstPitch, int theWidth,
	int theHeight)
{
	int aBytes = theWidth * 4;
	if (theSrc == nullptr || theDst == nullptr)
		return -8;
	if (0 < theWidth && 0 < theHeight)
	{
		if (0 < theSrcPitch && 0 < theDstPitch)
		{
			if (theSrcPitch == theDstPitch && theSrcPitch == aBytes)
			{
				aBytes = aBytes * theHeight;
				theHeight = 1;
			}
			int y = 0;
			if (0 < theHeight)
			{
				do
				{
					int i = 0;
					if (0 < aBytes)
					{
						do
						{
							theDst[i] = theSrc[i];
							theDst[i + 1] = theSrc[i + 1];
							theDst[i + 2] = theSrc[i + 2];
							i = i + 4;
						} while (i < aBytes);
					}
					theSrc = theSrc + theSrcPitch;
					y = y + 1;
					theDst = theDst + theDstPitch;
				} while (y < theHeight);
			}
			return 0;
		}
		return -14;
	}
	return -6;
}

// 0x50e17c: theBytes bytes a row, each multiplied by theValue
static int MulCSame(const uint8_t* theSrc, int theSrcPitch, uint8_t theValue, uint8_t* theDst, int theDstPitch,
	int theBytes, int theHeight)
{
	if (theSrc == nullptr || theDst == nullptr)
		return -8;
	if (0 < theSrcPitch && 0 < theDstPitch)
	{
		if (0 < theBytes && 0 < theHeight)
		{
			int y = 0;
			if (0 < theHeight)
			{
				do
				{
					int i = 0;
					if (0 < theBytes)
					{
						do
						{
							theDst[i] = (uint8_t)Div255((uint32_t)theSrc[i] * theValue);
							i = i + 1;
						} while (i < theBytes);
					}
					theSrc = theSrc + theSrcPitch;
					y = y + 1;
					theDst = theDst + theDstPitch;
				} while (y < theHeight);
			}
			return 0;
		}
		return -6;
	}
	return -14;
}

// 0x451c58
int MulC(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theValue, uint8_t* theDst, int theDstPitch,
	int theWidth, int theHeight)
{
	if (theSrc == nullptr || theValue == nullptr || theDst == nullptr)
		return -8;
	if (theSrcPitch < 1 || theDstPitch < 1)
		return -14;
	if (0 < theWidth && 0 < theHeight)
	{
		uint8_t aValue = theValue[0];
		if (aValue == theValue[1] && aValue == theValue[2] && aValue == theValue[3])
			return MulCSame(theSrc, theSrcPitch, aValue, theDst, theDstPitch, theWidth * 4, theHeight);
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				int i = 0;
				if (0 < theWidth * 4)
				{
					do
					{
						theDst[i] = (uint8_t)Div255((uint32_t)theValue[0] * theSrc[i]);
						theDst[i + 1] = (uint8_t)Div255((uint32_t)theValue[1] * theSrc[i + 1]);
						theDst[i + 2] = (uint8_t)Div255((uint32_t)theValue[2] * theSrc[i + 2]);
						theDst[i + 3] = (uint8_t)Div255((uint32_t)theValue[3] * theSrc[i + 3]);
						i = i + 4;
					} while (i < theWidth * 4);
				}
				theSrc = theSrc + theSrcPitch;
				y = y + 1;
				theDst = theDst + theDstPitch;
			} while (y < theHeight);
		}
		return 0;
	}
	return -6;
}

// 0x50e2f0
int MulCRgb(const uint8_t* theSrc, int theSrcPitch, const uint8_t* theValue, uint8_t* theDst, int theDstPitch,
	int theWidth, int theHeight)
{
	if (theSrc == nullptr || theValue == nullptr || theDst == nullptr)
		return -8;
	if (theSrcPitch < 1 || theDstPitch < 1)
		return -14;
	if (0 < theWidth && 0 < theHeight)
	{
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				int i = 0;
				if (0 < theWidth * 4)
				{
					do
					{
						theDst[i] = (uint8_t)Div255((uint32_t)theValue[0] * theSrc[i]);
						theDst[i + 1] = (uint8_t)Div255((uint32_t)theValue[1] * theSrc[i + 1]);
						theDst[i + 2] = (uint8_t)Div255((uint32_t)theValue[2] * theSrc[i + 2]);
						i = i + 4;
					} while (i < theWidth * 4);
				}
				theSrc = theSrc + theSrcPitch;
				y = y + 1;
				theDst = theDst + theDstPitch;
			} while (y < theHeight);
		}
		return 0;
	}
	return -6;
}

// 0x451df8 (scale 0)
int AddCRgb(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight)
{
	if (theValue == nullptr || theDst == nullptr)
		return -8;
	if (0 < theHeight && 0 < theWidth)
	{
		if (theDstPitch < 1)
			return -14;
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				if (0 < theWidth * 4)
				{
					uint8_t aB = theValue[0];
					uint8_t aG = theValue[1];
					uint8_t aR = theValue[2];
					uint8_t* p = theDst;
					do
					{
						uint32_t b = (uint32_t)p[0] + aB;
						uint32_t g = (uint32_t)p[1] + aG;
						uint32_t r = (uint32_t)p[2] + aR;
						if (0xfe < b)
							b = 0xff;
						p[0] = (uint8_t)b;
						if (0xfe < g)
							g = 0xff;
						p[1] = (uint8_t)g;
						if (0xfe < r)
							r = 0xff;
						p[2] = (uint8_t)r;
						p = p + 4;
					} while (p < theDst + theWidth * 4);
				}
				theDst = theDst + theDstPitch;
				y = y + 1;
			} while (y < theHeight);
		}
		return 0;
	}
	return -6;
}

// 0x451ae0 (the original sets four pixels a step while 20 bytes remain; the bytes are the same)
int SetRgb(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight)
{
	if (theDst == nullptr || theValue == nullptr)
		return -8;
	if (0 < theWidth && 0 < theHeight)
	{
		if (theDstPitch < 1)
			return -14;
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				int i = 0;
				if (0 < theWidth * 4)
				{
					do
					{
						theDst[i] = theValue[0];
						theDst[i + 1] = theValue[1];
						theDst[i + 2] = theValue[2];
						i = i + 4;
					} while (i < theWidth * 4);
				}
				theDst = theDst + theDstPitch;
				y = y + 1;
			} while (y < theHeight);
		}
		return 0;
	}
	return -6;
}

// 0x45194c (three pixels a step in the original while 16 bytes remain)
int Set4(const uint8_t* theValue, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight)
{
	if (theDst == nullptr || theValue == nullptr)
		return -8;
	if (0 < theWidth && 0 < theHeight)
	{
		if (theDstPitch < 1)
			return -14;
		int y = 0;
		if (0 < theHeight)
		{
			do
			{
				int i = 0;
				if (0 < theWidth * 4)
				{
					do
					{
						theDst[i] = theValue[0];
						theDst[i + 1] = theValue[1];
						theDst[i + 2] = theValue[2];
						theDst[i + 3] = theValue[3];
						i = i + 4;
					} while (i < theWidth * 4);
				}
				theDst = theDst + theDstPitch;
				y = y + 1;
			} while (y < theHeight);
		}
		return 0;
	}
	return -6;
}

// 0x4534a0 with its last argument 0
static void AlphaCompCRow(const uint8_t* theSrc, uint32_t theSrcAlpha, const uint8_t* theUnder,
	uint32_t theUnderAlpha, uint8_t* theDst, int theBytes)
{
	int i = 0;
	if (0 < theBytes)
	{
		do
		{
			uint32_t aSrc = (uint32_t)theSrc[i] * theSrcAlpha;
			uint32_t anUnder = Div255((uint32_t)theUnder[i] * theUnderAlpha);
			anUnder = (0xff - theSrcAlpha) * anUnder;
			theDst[i] = (uint8_t)(Div255(aSrc) + Div255(anUnder));
			i = i + 1;
		} while (i < theBytes);
	}
}

// 0x453ef8 (mode 0)
int AlphaCompC(const uint8_t* theSrc, int theSrcPitch, int theSrcAlpha, const uint8_t* theUnder,
	int theUnderPitch, int theUnderAlpha, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight)
{
	if (theSrc == nullptr || theUnder == nullptr || theDst == nullptr)
		return -8;
	if (theWidth < 1 || theHeight < 1)
		return -6;
	if (0 < theSrcPitch && 0 < theUnderPitch && 0 < theDstPitch)
	{
		// the alphas are bytes; (0xff - a) >> 31 is never set for them
		uint32_t aSrcAlpha = (uint8_t)theSrcAlpha;
		uint32_t anUnderAlpha = (uint8_t)theUnderAlpha;
		int y = 0;
		do
		{
			AlphaCompCRow(theSrc, aSrcAlpha, theUnder, anUnderAlpha, theDst, theWidth * 4);
			theSrc = theSrc + theSrcPitch;
			y = y + 1;
			theUnder = theUnder + theUnderPitch;
			theDst = theDst + theDstPitch;
		} while (y < theHeight);
		return 0;
	}
	return -14;
}

// 0x4517f8 with interpolation 1, which goes to the general resize 0x4f8bf8. The source pixel of each destination
// pixel here is the destination position divided by the factor; that mapping is not yet checked against 0x4f8bf8
// (see docs/DEVIATIONS.md).
void ResizeNearest(const uint8_t* theSrc, int theSrcWidth, int theSrcHeight, int theSrcPitch, int theSrcLeft,
	int theSrcTop, int theSrcRight, int theSrcBottom, uint8_t* theDst, int theDstPitch, int theDstWidth,
	int theDstHeight, double theFactorX, double theFactorY)
{
	(void)theSrcWidth;
	(void)theSrcHeight;
	if (theDstWidth < 1 || theDstHeight < 1 || theFactorX <= 0 || theFactorY <= 0)
		return;
	for (int y = 0; y < theDstHeight; y++)
	{
		int sy = theSrcTop + (int)(y / theFactorY);
		if (theSrcBottom <= sy)
			sy = theSrcBottom - 1;
		const uint32_t* aRow = (const uint32_t*)(theSrc + (size_t)sy * theSrcPitch);
		uint32_t* anOut = (uint32_t*)(theDst + (size_t)y * theDstPitch);
		for (int x = 0; x < theDstWidth; x++)
		{
			int sx = theSrcLeft + (int)(x / theFactorX);
			if (theSrcRight <= sx)
				sx = theSrcRight - 1;
			anOut[x] = aRow[sx];
		}
	}
}

} // namespace blit
} // namespace jb
