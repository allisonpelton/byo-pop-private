// The graphics' image primitives and effects (0x424df0-0x428730; see Draw.h).

#include "Draw.h"
#include "Graphics.h"
#include "Host.h"
#include "Platform.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace bonnies
{

namespace
{
// The blend of the engine's shapes: each channel (screen * (255 - a) + colour * a) >> 8, and the alpha byte lost
inline uint32_t blend(uint32_t theScreen, uint32_t theR, uint32_t theG, uint32_t theB, uint32_t theA)
{
	uint32_t anInv = 0xff - theA;
	return ((theScreen >> 0x10 & 0xff) * anInv + theR * theA & 0xffff00) << 8 |
		((theScreen >> 8 & 0xff) * anInv + theG * theA & 0xffffff00) | (theScreen & 0xff) * anInv + theB * theA >> 8;
}

inline uint32_t* pixelAt(int theX, int theY)
{
	return (uint32_t*)((uint8_t*)gPixels + gPitch * theY) + theX;
}

// 0x41f930: the distance of two points in whole pixels
inline int intDistance(int theX1, int theY1, int theX2, int theY2)
{
	int dx = theX1 - theX2;
	int dy = theY1 - theY2;
	return ftol(std::sqrt((double)(dy * dy + dx * dx)));
}
}

// 0x424df0
void drawPanel(float theX, float theY, float theW, float theH, int theRadius, uint32_t theFill, uint32_t theInner,
	int theInnerWidth, uint32_t theOuter, int theOuterWidth, float theInX0, float theInY0, float theInX1, float theInY1)
{
	int aBorder = theInnerWidth + theOuterWidth;
	double aW = (double)gWidth;
	float aH = (float)gHeight;
	int ix0 = ftol((double)theInX0 * aW + (double)0.01f);
	int iy0 = ftol((double)aH * theInY0 + (double)0.01f);
	int ix1 = ftol((double)theInX1 * aW + (double)0.01f);
	int iy1 = ftol((double)aH * theInY1 + (double)0.01f);
	int x0 = ftol((double)theX * aW + (double)0.01f);
	int y0 = ftol((double)aH * theY + (double)0.01f);
	int x1 = ftol(((double)theX + theW) * aW + (double)0.01f - 1.0);
	int y1 = ftol(((double)theY + theH) * aH + (double)0.01f - 1.0);
	int r = aBorder + theRadius;
	int cx0 = x0;
	if (x0 < gClipX0)
		cx0 = gClipX0;
	int y = gClipY0;
	if (gClipY0 <= y0)
		y = y0;
	int cx1 = gClipX1;
	if (x1 <= gClipX1)
		cx1 = x1;
	int cy1 = gClipY1;
	if (y1 <= gClipY1)
		cy1 = y1;
	if (cx0 <= cx1 && y <= cy1)
	{
		int aFromTop = y - y0;
		int aFromBottom = y1 - y;
		do
		{
			int aFromRight = x1 - cx0;
			int aFromLeft = cx0 - x0;
			int x = cx0;
			int aVertical = aFromTop;
			if (aFromBottom < aFromTop)
				aVertical = aFromBottom;
			do
			{
				int aHorizontal = aFromRight;
				if (x < x1 + x0 >> 1)
					aHorizontal = aFromLeft;
				int d = aVertical;
				if (aHorizontal < aVertical)
					d = aHorizontal;
				if (d <= r)
				{
					int aCorner = 999;
					int aCx = x1 - r;
					bool aInX = aCx <= x;
					if (!aInX)
					{
						aCx = r + x0;
						aInX = x <= aCx;
					}
					if (aInX)
					{
						int aCy = y1 - r;
						bool aInY = aCy <= y;
						if (!aInY)
						{
							aCy = r + y0;
							aInY = y <= aCy;
						}
						if (aInY)
							aCorner = r - intDistance(x, y, aCx, aCy);
					}
					if (aCorner < d)
						d = aCorner;
				}
				if (-1 < d)
				{
					uint32_t aColor = theFill;
					if (d < aBorder && (x < ix0 || ix1 < x || y < iy0 || iy1 < y))
					{
						aColor = theOuter;
						if (theOuterWidth <= d)
							aColor = theInner;
					}
					uint32_t* p = pixelAt(x, y);
					uint32_t a = aColor >> 0x18;
					if (a == 0xff)
						*p = aColor;
					else
						*p = blend(*p, aColor >> 0x10 & 0xff, aColor >> 8 & 0xff, aColor & 0xff, a);
				}
				x = x + 1;
				aFromRight = aFromRight - 1;
				aFromLeft = aFromLeft + 1;
			} while (x <= cx1);
			y = y + 1;
			aFromTop = aFromTop + 1;
			aFromBottom = aFromBottom - 1;
		} while (y <= cy1);
	}
}

// 0x425130
void drawSweep(float theX, float theY, float theW, float theH, uint32_t theColor)
{
	double aW = (double)gWidth;
	float aH = (float)gHeight;
	int x0 = ftol((double)theX * aW + (double)0.01f);
	int y0 = ftol((double)aH * theY + (double)0.01f);
	int x1 = ftol(((double)theX + theW) * aW + (double)0.01f - 1.0);
	int y1 = ftol(((double)theY + theH) * aH + (double)0.01f - 1.0);
	int aCy = y1 + y0 >> 1;
	int cx0 = x0;
	if (x0 < gClipX0)
		cx0 = gClipX0;
	int y = y0;
	if (y0 < gClipY0)
		y = gClipY0;
	int cx1 = x1;
	if (gClipX1 < x1)
		cx1 = gClipX1;
	int cy1 = y1;
	if (gClipY1 < y1)
		cy1 = gClipY1;
	if (cx0 <= cx1 && y <= cy1 && theColor >> 0x18 != 0)
	{
		int aTime = (int)gLastActiveTime;
		int aDy = y - aCy;
		int aUp = aCy - y;
		float aPhase = (float)((double)(float)(aTime % 0x578) * (double)0.0007142857f * (double)6.28318f);
		uint32_t a = theColor >> 0x18;
		do
		{
			uint32_t* p = pixelAt(cx0, y);
			int aAbsDy = std::abs(aDy);
			int aDx = cx0 - (x1 + x0 >> 1);
			int n = (cx1 - cx0) + 1;
			do
			{
				int aFar = std::abs(aDx);
				if (aFar <= aAbsDy)
					aFar = aAbsDy;
				if (0xf < aFar)
				{
					double anAngle = angleOf((float)aDx, (float)aUp);
					double aTo = anAngle <= (double)aPhase ? (double)aPhase : (double)aPhase + (double)6.28318f;
					double aDiff = aTo - anAngle;
					if (0.0 <= aDiff)
					{
						double aFade = 1.0 - aDiff * (double)0.4420974f;
						if ((double)0.01f <= aFade)
						{
							int k = ftol((double)a * aFade);
							*p = blend(*p, theColor >> 0x10 & 0xff, theColor >> 8 & 0xff, theColor & 0xff, k);
						}
					}
				}
				p = p + 1;
				aDx = aDx + 1;
				n = n - 1;
			} while (n != 0);
			y = y + 1;
			aDy = aDy + 1;
			aUp = aUp - 1;
		} while (y <= cy1);
	}
}

// 0x4253d0
void drawFrameGlow(const Frame* theFrame, float theX, float theY, float theAlpha, int theBright, int theCx, int theCy,
	int theMode)
{
	int x0 = ftol((double)gWidth * theX + (double)0.01f);
	int y0 = ftol((double)gHeight * theY + (double)0.01f);
	int aSize = theFrame->mHeight;
	if (theFrame->mHeight < theFrame->mWidth)
		aSize = theFrame->mWidth;
	bool aClip = x0 < gClipX0 || y0 < gClipY0 || gClipX1 + 1 < aSize + x0 || gClipY1 + 1 < aSize + y0;
	int anAlpha = ftol((double)theAlpha * 255.0 + (double)0.01f);
	if (anAlpha == 0)
		return;
	int x1 = theFrame->mWidth - 1 + x0;
	int y1 = theFrame->mHeight - 1 + y0;
	const Image* anImage = theFrame->mOwner;
	int aBytes = anImage->mBits >> 3;
	int aCx = (x1 + x0 >> 1) + theCx;
	int aCy = (y1 + y0 >> 1) + theCy;
	// the colours of mode 1: three tints the angle sweeps between
	int c10 = 0, c14 = 0, c18 = 0, c1c = 0, c3c = 0, c40 = 0, c44 = 0, c48 = 0, c4c = 0, c50 = 0;
	float aPhase;
	if (theMode != 0)
	{
		int aTime = (int)gLastActiveTime;
		c40 = 0xf8;
		c44 = 0x35;
		c10 = 0xbf;
		c14 = 0x19;
		c18 = 0x39;
		c4c = 0xf8;
		c50 = 0x35;
		c3c = 0xff;
		c1c = 0xff;
		c48 = 0xff;
		aPhase = (float)((double)(aTime % 600) * (double)0.0016666667f * (double)-6.28318f);
	}
	else
	{
		int aTime = (int)gLastActiveTime;
		aPhase = (float)((double)(aTime % 0x708) * (double)0.00055555557f * (double)-6.28318f);
	}
	if (y0 <= y1)
	{
		int aDy = aCy - y0;
		int y = y0;
		do
		{
			const uint8_t* aSrc = (const uint8_t*)anImage->mPixels +
				((theFrame->mY - y0) + y) * anImage->mPitch + theFrame->mX * aBytes;
			if (x0 <= x1)
			{
				int aDx = x0 - aCx;
				int x = x0;
				do
				{
					uint32_t aPixel = *(const uint32_t*)aSrc;
					if ((!aClip || (gClipX0 <= x && gClipY0 <= y && x < gClipX1 + 1 && y < gClipY1 + 1)) &&
						aPixel >> 0x18 != 0)
					{
						uint32_t a = aPixel >> 0x18;
						int aBright = (int)(aPixel & 0xff) + theBright;
						if (0xff < aBright)
							aBright = 0xff;
						if (anAlpha != 0xff)
							a = a * anAlpha >> 8;
						double anAngle = (double)tableAngle(aDx, aDy) + (double)aPhase;
						if (anAngle < 0.0)
							anAngle = anAngle + (double)6.28318f;
						float v = (float)(anAngle * (double)0.4744f);
						uint32_t* p = pixelAt(x, y);
						if (theMode != 0)
						{
							double t = (double)v * (double)0.6666667f;
							int k;
							int aInv;
							uint32_t aWeight;
							uint32_t aR;
							uint32_t aG;
							uint32_t aB;
							if (1.0 <= t)
							{
								k = ftol((t - 1.0) * 255.0);
								aInv = 0xff - k;
								aWeight = (uint32_t)(k * c48 + aInv * c10) >> 8;
								aR = (uint32_t)(aInv * c14) >> 8;
								aG = (uint32_t)(k * c4c + aInv * c18) >> 8;
								aB = (uint32_t)(k * c50 + aInv * c1c) >> 8;
							}
							else
							{
								k = ftol(t * 255.0);
								aInv = 0xff - k;
								aWeight = (uint32_t)(k * c10 + aInv * c3c) >> 8;
								aR = (uint32_t)(k * c14) >> 8;
								aG = (uint32_t)(k * c18 + aInv * c40) >> 8;
								aB = (uint32_t)(k * c1c + aInv * c44) >> 8;
							}
							*p = blend(*p, aR, aG, aB, aWeight);
						}
						else
						{
							double aR;
							float aG;
							float aB;
							if ((double)v < 1.0)
							{
								aR = 1.0 - (double)v;
								aG = v;
								aB = 0.0f;
							}
							else if ((double)v < 2.0)
							{
								aG = (float)(2.0 - (double)v);
								aB = (float)((double)v - 1.0);
								aR = 0.0;
							}
							else
							{
								aG = 0.0f;
								aR = (double)v - 2.0;
								aB = (float)(3.0 - (double)v);
							}
							float aLevel = (float)aBright;
							int r = ftol((double)aLevel * aR);
							int g = ftol((double)aLevel * aG * (double)0.8f);
							int b = ftol((double)aLevel * aB);
							if (a == 0xff)
								*p = ((uint32_t)r << 8 | g) << 8 | b;
							else
								*p = blend(*p, r, g, b, a);
						}
					}
					aSrc += aBytes;
					x = x + 1;
					aDx = aDx + 1;
				} while (x <= x1);
			}
			y = y + 1;
			aDy = aDy - 1;
		} while (y <= y1);
	}
}

// 0x4259b0
void drawFrameTinted(const Frame* theFrame, float theX, float theY, uint32_t theColor)
{
	int x0 = ftol((double)gWidth * theX + (double)0.01f);
	int y0 = ftol((double)gHeight * theY + (double)0.01f);
	int aSize = theFrame->mWidth;
	if (theFrame->mWidth <= theFrame->mHeight)
		aSize = theFrame->mHeight;
	bool aClip = x0 < gClipX0 || y0 < gClipY0 || gClipX1 + 1 < aSize + x0 || gClipY1 + 1 < aSize + y0;
	uint32_t anAlpha = theColor >> 0x18;
	if (anAlpha == 0)
		return;
	int y1 = theFrame->mHeight - 1 + y0;
	bool aRainbow = (theColor & 0xffffff) == 0x123456;
	int x1 = theFrame->mWidth - 1 + x0;
	int aCx = 0;
	int aDy = 0;
	float aPhase = 0.0f;
	uint32_t aRB;
	uint32_t aG;
	if (aRainbow)
	{
		aCx = x1 + x0 >> 1;
		aDy = y1 + y0 >> 1;
		int aTime = (int)gLastActiveTime;
		aPhase = (float)((double)(aTime % 0x708) * (double)0.00055555557f * (double)-6.28318f);
		aG = theColor;
		aRB = 0;
	}
	else
	{
		aG = theColor >> 8 & 0xff;
		aRB = (theColor >> 0x10 & 0xff) * 0x10000 + (theColor & 0xff);
	}
	if (y0 <= y1)
	{
		aDy = aDy - y0;
		const Image* anImage = theFrame->mOwner;
		int y = y0;
		do
		{
			const uint32_t* aSrc = anImage->mPixels +
				(((y - y0) + theFrame->mY) * (anImage->mPitch >> 2) + theFrame->mX);
			uint32_t* p = gPixels + (gPitch4 * y + x0);
			if (x0 <= x1)
			{
				int aDx = x0 - aCx;
				int x = x0;
				do
				{
					if ((!aClip || (gClipX0 <= x && gClipY0 <= y && x < gClipX1 + 1 && y < gClipY1 + 1)) &&
						0xffffff < *aSrc)
					{
						uint32_t a = *aSrc >> 0x18;
						if (anAlpha != 0xff)
							a = a * anAlpha >> 8;
						bool aBlend = true;
						if (aRainbow)
						{
							double anAngle = (double)tableAngle(aDx, aDy) + (double)aPhase;
							if (anAngle < 0.0)
								anAngle = anAngle + (double)6.28318f;
							float v = (float)(anAngle * (double)0.4744f);
							double aR;
							float aGreen;
							float aB;
							if ((double)v < 1.0)
							{
								aR = 1.0 - (double)v;
								aGreen = v;
								aB = 0.0f;
							}
							else if ((double)v < 2.0)
							{
								aGreen = (float)(2.0 - (double)v);
								aB = (float)((double)v - 1.0);
								aR = 0.0;
							}
							else
							{
								aGreen = 0.0f;
								aR = (double)v - 2.0;
								aB = (float)(3.0 - (double)v);
							}
							int r = ftol(aR * 255.0);
							int g = ftol((double)aGreen * 200.0);
							int b = ftol((double)aB * 255.0);
							aRB = (uint32_t)r << 0x10 | b;
							aG = g;
							if (a == 0xff)
							{
								*p = ((uint32_t)r << 8 | g) << 8 | b;
								aBlend = false;
							}
						}
						else if (a == 0xff)
						{
							*p = theColor;
							aBlend = false;
						}
						if (aBlend)
							*p = ((*p & 0xff00ff) * (0xff - a) + a * aRB >> 8 & 0xff00ff) |
								((*p >> 8 & 0xff) * (0xff - a) + a * aG & 0xff00);
					}
					aSrc = aSrc + 1;
					p = p + 1;
					x = x + 1;
					aDx = aDx + 1;
				} while (x <= x1);
			}
			y = y + 1;
			aDy = aDy - 1;
		} while (y <= y1);
	}
}

} // namespace bonnies

namespace bonnies
{

namespace
{
// The blend of the image primitives: red and blue together, green alone, (screen * (255 - a) + pixel * a) >> 8
inline uint32_t blendPacked(uint32_t theScreen, uint32_t thePixel, uint32_t theA)
{
	return ((theScreen >> 8 & 0xff) * (0xff - theA) + (thePixel >> 8 & 0xff) * theA & 0xff00) |
		((theScreen & 0xff00ff) * (0xff - theA) + (thePixel & 0xff00ff) * theA >> 8 & 0xff00ff);
}
}

// 0x426f50
void drawFrame(const Frame* theFrame, float theX, float theY, float theAlpha, int theOpaque)
{
	int x0 = ftol((double)gWidth * theX + (double)0.01f);
	int y0 = ftol((double)gHeight * theY + (double)0.01f);
	int aClipX0 = gClipX0;
	int aSrcY = theFrame->mY;
	int y1 = theFrame->mHeight - 1 + y0;
	int aSrcX = theFrame->mX;
	int x1 = theFrame->mWidth - 1 + x0;
	if (x0 < gClipX0)
	{
		aSrcX = aSrcX + (gClipX0 - x0);
		x0 = gClipX0;
	}
	if (y0 < gClipY0)
	{
		int aCut = gClipY0 - y0;
		y0 = gClipY0;
		aSrcY = aSrcY + aCut;
	}
	int aRight = gClipX1 + 1;
	if (aRight <= x1)
		x1 = gClipX1;
	int aBottom = gClipY1 + 1;
	if (aBottom <= y1)
		y1 = gClipY1;
	int aWidth = (x1 - x0) + 1;
	int aRows = (y1 - y0) + 1;
	if (aWidth < 1)
		return;
	if (aRows < 1)
		return;
	int anAlpha = 0;
	if ((double)theAlpha < 1.0)
	{
		anAlpha = ftol((double)theAlpha * 255.0);
		if (anAlpha == 0)
			return;
	}
	const Image* anImage = theFrame->mOwner;
	if (x0 == 0 && aWidth == gWidth && gClipY0 == 0 && aBottom == gHeight && aClipX0 == 0 && aRight == gWidth &&
		gPitch >> 2 == aWidth)
	{
		if (theOpaque != 0 && 1.0 <= (double)theAlpha)
		{
			// As the original: the rows are copied from the image's first pixel, whatever the frame and the clipping
			std::memcpy((uint8_t*)gPixels + gPitch * y0, anImage->mPixels, (size_t)gPitch * aRows);
			return;
		}
	}
	else if (theOpaque != 0 && anAlpha == 0)
	{
		if (y1 < y0)
			return;
		int aRow = aSrcY - y0;
		do
		{
			const uint8_t* aSrc = (const uint8_t*)anImage->mPixels + (aRow + y0) * anImage->mPitch + aSrcX * 4;
			std::memcpy(pixelAt(x0, y0), aSrc, (size_t)aWidth * 4);
			y0 = y0 + 1;
		} while (y0 <= y1);
		return;
	}
	int aPitch = anImage->mPitch;
	int aRow = 0;
	if (0 < aRows)
	{
		int anOffset = aPitch * aSrcY;
		do
		{
			const uint32_t* aSrc = (const uint32_t*)((const uint8_t*)anImage->mPixels + anOffset + aSrcX * 4);
			uint32_t* p = pixelAt(x0, y0 + aRow);
			int n = aWidth;
			if (anAlpha == 0)
			{
				do
				{
					uint32_t c = *aSrc;
					if (c < 0xff000000)
					{
						if (0xffffff < c)
							*p = blendPacked(*p, c, c >> 0x18);
					}
					else
						*p = c;
					aSrc = aSrc + 1;
					p = p + 1;
					n = n - 1;
				} while (n != 0);
			}
			else
			{
				do
				{
					uint32_t c = *aSrc;
					if (0xffffff < c)
						*p = blendPacked(*p, c, (c >> 0x18) * anAlpha >> 8);
					aSrc = aSrc + 1;
					p = p + 1;
					n = n - 1;
				} while (n != 0);
			}
			aRow = aRow + 1;
			anOffset = anOffset + aPitch;
		} while (aRow < aRows);
	}
}

// 0x427340: theTurn quarter turns (0-3)
void drawFrameTurned(const Frame* theFrame, float theX, float theY, int theTurn)
{
	int x0 = ftol((double)gWidth * theX + (double)0.01f);
	int y0 = ftol((double)gHeight * theY + (double)0.01f);
	int aWidth = theFrame->mWidth;
	int aTurn = theTurn & 3;
	int aHeight = theFrame->mHeight;
	int aSize = aWidth;
	if (aWidth <= aHeight)
		aSize = aHeight;
	bool aClip = x0 < gClipX0 || y0 < gClipY0 || gClipX1 + 1 < x0 + aSize || gClipY1 + 1 < aSize + y0;
	int aLastRow = aHeight - 1;
	const Image* anImage = theFrame->mOwner;
	int aBytes = anImage->mBits >> 3;
	int r = 0;
	if (-1 < aLastRow)
	{
		int aTurnedX = aLastRow + x0;
		do
		{
			const uint8_t* aSrc = (const uint8_t*)anImage->mPixels + theFrame->mX * aBytes +
				(theFrame->mY + r) * anImage->mPitch;
			if (-1 < aWidth - 1)
			{
				int aBack = aWidth - 1 + x0;
				int aShift = y0 - x0;
				int aForward = x0;
				int n = aWidth;
				do
				{
					int y;
					int x;
					if (aTurn == 0)
					{
						y = r + y0;
						x = aForward;
					}
					else if (aTurn == 1)
					{
						y = aShift + aForward;
						x = aTurnedX;
					}
					else if (aTurn == 2)
					{
						y = aTurnedX + aShift;
						x = aBack;
					}
					else
					{
						y = aBack + aShift;
						x = (x0 - y0) + r + y0;
					}
					if (!aClip || (gClipX0 <= x && gClipY0 <= y && x <= gClipX1 && y <= gClipY1))
					{
						uint32_t c = *(const uint32_t*)aSrc;
						uint32_t a = c >> 0x18;
						if (a != 0)
						{
							uint32_t* p = pixelAt(x, y);
							if (a == 0xff)
								*p = c;
							else
								*p = blendPacked(*p, c, a);
						}
					}
					aSrc += aBytes;
					aBack = aBack - 1;
					aForward = aForward + 1;
					n = n - 1;
				} while (n != 0);
			}
			r = r + 1;
			aTurnedX = aTurnedX - 1;
		} while (r <= aLastRow);
	}
}

} // namespace bonnies

namespace bonnies
{

// 0x4275c0
void drawLine(float theX1, float theY1, float theX2, float theY2, uint32_t theColor, int theThickness, int theMinor)
{
	int aHalf = theThickness >> 1;
	float aHalfW = (float)((double)gInvWidth * 0.5);
	double aHalfH = (double)gInvHeight * 0.5;
	if (theY1 == theY2)
	{
		float aMax = theX1 > theX2 ? theX1 : theX2;
		float aMin = theX1 < theX2 ? theX1 : theX2;
		float aThick = (float)theThickness;
		fillRect(aMin, (float)(theY1 - aThick * aHalfH), (float)(aMax - aMin), (float)(aThick * gInvHeight), theColor);
		return;
	}
	if (theX1 == theX2)
	{
		float aMax = theY1 > theY2 ? theY1 : theY2;
		float aMin = theY1 < theY2 ? theY1 : theY2;
		float aThick = (float)theThickness;
		fillRect((float)(theX1 - aThick * aHalfW), aMin, (float)(aThick * gInvWidth), (float)(aMax - aMin), theColor);
		return;
	}
	int x1 = ftol((double)theX1 * gWidth);
	int y1 = ftol((double)gHeight * theY1);
	int x2 = ftol((double)gWidth * theX2);
	int y2 = ftol((double)gHeight * theY2);
	int aFixedX = x1 << 0x10;
	int aFixedY = y1 << 0x10;
	int dx = x2 - x1;
	int dy = y2 - y1;
	bool aClip = x1 < gClipX0 + theThickness || gClipX1 - theThickness + 1 <= x2 ||
		y1 < gClipY0 + theThickness || gClipY1 - theThickness + 1 <= y2;
	uint8_t* aPixels = (uint8_t*)gPixels;
	int aDy = dy < 0 ? -dy : dy;
	int aDx = dx < 0 ? -dx : dx;
	int aCount = (aDx > aDy ? aDx : aDy) + 1;
	if (aDx > aDy && theMinor == 0)
	{
		// Along x: a column of theThickness pixels at each x
		int aStepX = ((dx <= 0) - 1 & 0x20000) - 0x10000;
		int aStepY = (dy << 0x10) / aDx + (dy > 0) * 2 - 1;
		int aY = aFixedY + (aStepY >> 1);
		if (aCount <= 0)
			return;
		do
		{
			int aCol = aFixedX >> 0x10;
			int aRow = (aY >> 0x10) - aHalf;
			uint8_t* p = aPixels + gPitch * aRow + aCol * 4;
			if (aClip)
			{
				for (int n = theThickness; n > 0; n--)
				{
					if (aCol >= gClipX0 && aCol <= gClipX1 && aRow >= gClipY0 && aRow <= gClipY1)
						*(uint32_t*)p = theColor;
					p += gPitch;
					aRow++;
				}
			}
			else
			{
				for (int n = theThickness; n > 0; n--)
				{
					*(uint32_t*)p = theColor;
					p += gPitch;
				}
			}
			aFixedX += aStepX;
			aY += aStepY;
		} while (--aCount != 0);
		return;
	}
	// Along y (and with theMinor, a line mostly along x drawn as aDy + 1 rows): a row of theThickness pixels at each y
	if (theMinor != 0 && aDx > aDy)
		aCount = aDy + 1;
	int aStepY = ((dy <= 0) - 1 & 0x20000) - 0x10000;
	if (aDy == 0)
		return;  // the original divides by zero here
	int aStepX = (dx << 0x10) / aDy + (dx > 0) * 2 - 1;
	int aX = aFixedX + (aStepX >> 1);
	if (aCount <= 0)
		return;
	do
	{
		int aRow = aFixedY >> 0x10;
		int aCol = (aX >> 0x10) - aHalf;
		uint8_t* p = aPixels + gPitch * aRow + aCol * 4;
		if (aClip)
		{
			// The original steps this pointer by one byte, not one pixel
			for (int n = theThickness; n > 0; n--)
			{
				if (aCol >= gClipX0 && aCol <= gClipX1 && aRow >= gClipY0 && aRow <= gClipY1)
					std::memcpy(p, &theColor, 4);
				p++;
				aCol++;
			}
		}
		else
		{
			uint32_t* q = (uint32_t*)p;
			for (int n = theThickness; n != 0; n--)
				*q++ = theColor;
		}
		aX += aStepX;
		aFixedY += aStepY;
	} while (--aCount != 0);
}

// 0x427ab0: darkens the screen towards its corners; theAmount 0 leaves the centre lit, 1 darkens all of it
void drawVignette(float theAmount)
{
	int aCx = gWidth >> 1;
	int aCy = gHeight >> 1;
	int aRadius = intDistance(0, 0, gWidth / 2, gHeight / 2) + 1;
	int anInner = 0;
	if (theAmount > 0.5f)
		anInner = ftol(((double)theAmount - (double)0.5f) * aRadius);
	double aRest = 1.0 - (double)theAmount;
	int anOuter = ftol((1.0 + aRest) * aRadius);
	int aDiv = aRadius < 1 ? 1 : aRadius;
	float aScale = (float)(aRest / ((double)aDiv * (double)(1.0f / 255.0f)));
	int aDist = 0;
	int r = 0;
	if (gHeight <= 0)
		return;
	do
	{
		uint32_t* p = pixelAt(0, r);
		int aWait = 0;
		int c = 0;
		if (gWidth > 0)
		{
			do
			{
				int aLast = aWait--;
				if (aLast <= 0)
				{
					aDist = intDistance(c, r, aCx, aCy) + anInner;
					aWait = 5;
				}
				int v = ftol((double)(anOuter - aDist) * aScale);
				if (v <= 0)
					*p = 0;
				else if (v < 0xff)
				{
					uint32_t aPixel = *p;
					*p = (((aPixel & 0xffff00ff) * v ^ (aPixel & 0xff00) * v) & 0xff0000 ^ (aPixel & 0xff00ff) * v) >> 8;
				}
				p++;
				c++;
			} while (c < gWidth);
		}
		r++;
	} while (r < gHeight);
}

namespace
{
// 0x4818f8: the clip rectangles saved by pushClip; 0x481998 their count
int gClipStack[10][4];
int gClipDepth = 0;
}

// 0x428030: saves the clip rectangle and narrows it to the pixels theX0..theX1, theY0..theY1 (inclusive)
void pushClip(int theX0, int theY0, int theX1, int theY1)
{
	gClipStack[gClipDepth][0] = gClipX0;
	gClipStack[gClipDepth][1] = gClipY0;
	gClipStack[gClipDepth][2] = gClipX1;
	gClipStack[gClipDepth][3] = gClipY1;
	gClipDepth++;
	if (theX0 <= gClipX0)
		theX0 = gClipX0;
	if (theY0 <= gClipY0)
		theY0 = gClipY0;
	if (gClipX1 <= theX1)
		theX1 = gClipX1;
	if (gClipY1 <= theY1)
		theY1 = gClipY1;
	gClipX1 = theX1;
	gClipX0 = (theX0 < 1) - 1 & theX0;
	gClipY0 = (theY0 < 1) - 1 & theY0;
	if (gWidth - 1 <= theX1)
		gClipX1 = gWidth - 1;
	gClipY1 = theY1;
	if (gHeight - 1 <= theY1)
		gClipY1 = gHeight - 1;
}

// 0x4280f0: pushClip of the screen fractions theX0..theX1, theY0..theY1
void pushClip(float theX0, float theY0, float theX1, float theY1)
{
	float aWidth = (float)gWidth;
	int y1 = ftol((double)theY1 * gHeight);
	int x1 = ftol((double)aWidth * theX1);
	int y0 = ftol((double)gHeight * theY0);
	int x0 = ftol((double)aWidth * theX0);
	pushClip(x0, y0, x1, y1);
}

// 0x428150
void popClip()
{
	gClipDepth--;
	gClipX0 = gClipStack[gClipDepth][0];
	gClipY0 = gClipStack[gClipDepth][1];
	gClipX1 = gClipStack[gClipDepth][2];
	gClipY1 = gClipStack[gClipDepth][3];
}

// 0x427c70: a frame theBorder pixels wide around a rectangle, coloured by the angle of each pixel from the centre: from
// theColor1 through theColor2 to theColor3 once round, the start turned by theTurn degrees
void drawAngleFrame(float theX, float theY, float theW, float theH, uint32_t theColor1, uint32_t theColor2,
	uint32_t theColor3, int theBorder, float theTurn)
{
	int x0 = ftol((double)theX * gWidth + (double)0.01f);
	float aHeight = (float)gHeight;
	int y0 = ftol((double)aHeight * theY + (double)0.01f);
	int aW = ftol((double)gWidth * theW + (double)0.01f);
	int aH = ftol((double)aHeight * theH + (double)0.01f);
	bool aClip = x0 < gClipX0 || y0 < gClipY0 || aW + x0 > gClipX1 + 1 || aH + y0 > gClipY1 + 1;
	int y1 = aH + y0 - 1;
	uint32_t a1 = theColor1 >> 0x18, r1 = theColor1 >> 0x10 & 0xff, g1 = theColor1 >> 8 & 0xff, b1 = theColor1 & 0xff;
	uint32_t a2 = theColor2 >> 0x18, r2 = theColor2 >> 0x10 & 0xff, g2 = theColor2 >> 8 & 0xff, b2 = theColor2 & 0xff;
	uint32_t a3 = theColor3 >> 0x18, r3 = theColor3 >> 0x10 & 0xff, g3 = theColor3 >> 8 & 0xff, b3 = theColor3 & 0xff;
	int x1 = aW + x0 - 1;
	int aCx = x0 + x1 >> 1;
	int aCy = y1 + y0 >> 1;
	int anInY0 = y0 + theBorder - 1;
	int anInY1 = y1 - theBorder + 1;
	int anInX0 = x0 + theBorder - 1;
	int anInX1 = x1 - theBorder + 1;
	float aTurn = (float)((double)theTurn * (double)0.0027777778f * (double)-6.28318f);
	int r = y0;
	if (r > y1)
		return;
	int aRelY = aCy - y0;
	do
	{
		int c = x0;
		if (c <= x1)
		{
			int aRelX = x0 - aCx;
			do
			{
				if ((!aClip || (c >= gClipX0 && r >= gClipY0 && c <= gClipX1 && r <= gClipY1)) &&
					(r < anInY0 || r > anInY1 || c < anInX0 || c > anInX1))
				{
					double v = (double)tableAngle(aRelX, aRelY) + aTurn;
					if (v < 0.0)
						v += (double)6.28318f;
					v *= (double)0.3183f;
					uint32_t a, rr, g, b;
					if (v < 1.0)
					{
						uint32_t t = (uint32_t)ftol(v * 255.0);
						uint32_t u = 0xff - t;
						a = t * a2 + u * a1;
						rr = t * r2 + u * r1;
						g = t * g2 + u * g1;
						b = t * b2 + u * b1;
					}
					else
					{
						uint32_t t = (uint32_t)ftol((v - 1.0) * 255.0);
						uint32_t u = 0xff - t;
						a = t * a3 + u * a2;
						rr = t * r3 + u * r2;
						g = t * g3 + u * g2;
						b = t * b3 + u * b2;
					}
					b >>= 8;
					a >>= 8;
					g >>= 8;
					rr >>= 8;
					uint32_t* p = gPixels + (r * gPitch4 + c);
					if (a >= 0xfa)
						*p = (rr << 8 | g) << 8 | b;
					else
						*p = blend(*p, rr, g, b, a);
				}
				c++;
				aRelX++;
			} while (c <= x1);
		}
		r++;
		aRelY--;
	} while (r <= y1);
}

// 0x425d50: theFrame scaled by theScale and centred on theX, theY (its top left corner there with theCorner), faded by
// theAlpha; theTint (its alpha fading too) replaces the frame's colours. Shrunk to 0.65 or less without a tint, each
// pixel averages two of the frame's.
void drawFrameScaled(const Frame* theFrame, float theX, float theY, float theScale, float theAlpha, uint32_t theTint,
	int theCorner)
{
	if (theTint != 0)
	{
		uint32_t aTintAlpha = theTint >> 0x18;
		theTint |= 0xff000000;
		theAlpha = (float)((double)(int)aTintAlpha * (double)(1.0f / 255.0f) * theAlpha);
	}
	if (theAlpha <= 0.0f)
		return;
	int aW = theFrame->mWidth;
	int aH = theFrame->mHeight;
	float aWf = (float)aW;
	float aHf = (float)aH;
	if (aWf <= 0.0f || aHf <= 0.0f || theScale <= 0.001f)
		return;
	double aY;
	if (theCorner != 0)
	{
		theX = (float)((double)aWf * theScale * 0.5 / gWidth + theX);
		aY = (double)aHf * theScale * 0.5 / gHeight + theY;
	}
	else
		aY = theY;
	int aCx = ftol((double)gWidth * theX + 0.5);
	int aCy = ftol((double)gHeight * aY + 0.5);
	int anAlpha = 0;
	if (theAlpha < 1.0f)
	{
		anAlpha = ftol((double)theAlpha * 255.0);
		if (anAlpha == 0)
			return;
	}
	int aDw = ftol((double)aWf * theScale);
	int aDh = ftol((double)aHf * theScale);
	if (aDw <= 0 || aDh == 0)
		return;
	const Image* anImage = theFrame->mOwner;
	int aSrcPitch = anImage->mPitch >> 2;
	int aDstPitch = gPitch >> 2;
	const uint32_t* aSrc = anImage->mPixels + (theFrame->mY * aSrcPitch + theFrame->mX);
	int y0 = aCy - (aDh >> 1);
	int y1 = y0 + aDh - 1;
	int ys = y0;
	if (ys < gClipY0)
		ys = gClipY0;
	if (y1 > gClipY1)
		y1 = gClipY1;
	int x0 = aCx - (aDw >> 1);
	int x1 = x0 + aDw - 1;
	int xs = x0;
	if (xs < gClipX0)
		xs = gClipX0;
	if (x1 > gClipX1)
		x1 = gClipX1;
	bool aSmooth = theScale <= 0.65f;
	int aDivX;
	if (theTint != 0)
	{
		aSmooth = false;
		aDivX = aDw;
	}
	else
		aDivX = aSmooth ? aDw * 2 : aDw;
	int aStepX = (aW << 0x10) / aDivX - 1;
	if (aSmooth)
		aDh *= 2;
	int aStepY = (aH << 0x10) / aDh - 1;
	int aStartX = (xs - x0) * aStepX + 16000;
	int aFy = (ys - y0) * aStepY + 16000;
	int aLastY;
	if (aSmooth)
	{
		if (aStartX + (x1 - xs) * aStepX * 2 >> 0x10 >= aW)
			return;
		aLastY = aFy + (y1 - ys) * aStepY * 2;
	}
	else
	{
		if ((x1 - xs) * aStepX + aStartX >> 0x10 >= aW)
			return;
		aLastY = (y1 - ys) * aStepY + aFy;
	}
	if (aLastY >> 0x10 >= aH)
		return;
	if (ys > y1)
		return;
	const uint32_t* aRow2 = nullptr;
	uint32_t* aDstRow = gPixels + (ys * aDstPitch + xs);
	int aRows = y1 - ys + 1;
	do
	{
		uint32_t* d = aDstRow;
		const uint32_t* aRow = aSrc + (aFy >> 0x10) * aSrcPitch;
		int aFx = aStartX;
		if (aSmooth)
		{
			aFy += aStepY;
			aRow2 = aSrc + (aFy >> 0x10) * aSrcPitch;
		}
		if (xs <= x1)
		{
			int aCount = x1 - xs + 1;
			do
			{
				uint32_t c = aRow[aFx >> 0x10];
				int a = (int)(c >> 0x18);
				if (anAlpha != 0)
					a = a * anAlpha >> 8;
				if (aSmooth)
				{
					aFx += aStepX;
					uint32_t c2 = aRow2[aFx >> 0x10];
					if (a == 0xff)
						*d = ((c2 & 0xff) + (c & 0xff) | (c2 & 0xfe0000) + (c & 0xfe0000) | (c2 & 0xfe00) + (c & 0xfe00)) >> 1;
					else if (a > 0)
					{
						uint32_t u = 0xff - a;
						uint32_t aScreen = *d;
						*d = (((aScreen & 0xff0000) * u + (c & 0xff0000) * a & 0xff0000ff | (aScreen & 0xff) * u + (c & 0xff) * a) >>
							8) | ((aScreen >> 8 & 0xff) * u + (c >> 8 & 0xff) * a & 0xff00);
					}
				}
				else
				{
					if (theTint != 0)
						c = theTint;
					if (a == 0xff)
						*d = c;
					else if (a > 0)
					{
						uint32_t u = 0xff - a;
						uint32_t aScreen = *d;
						*d = ((aScreen >> 8 & 0xff) * u + (c >> 8 & 0xff) * a & 0xff00) |
							((aScreen & 0xff00ff) * u + (c & 0xff00ff) * a >> 8 & 0xff00ff);
					}
				}
				aFx += aStepX;
				d++;
			} while (--aCount != 0);
		}
		aFy += aStepY;
		aDstRow += aDstPitch;
	} while (--aRows != 0);
}

// 0x426290: theFrame turned by theAngle degrees (clockwise), scaled and placed as drawFrameScaled places it; drawn as a
// four-cornered polygon, row by row between its left and right edges
void drawFrameRotated(const Frame* theFrame, float theX, float theY, float theAngle, float theScale, float theAlpha,
	uint32_t theTint, int theCorner)
{
	if (theAngle == 0.0f)
	{
		drawFrameScaled(theFrame, theX, theY, theScale, theAlpha, theTint, theCorner);
		return;
	}
	if (theTint != 0)
	{
		uint32_t aTintAlpha = theTint >> 0x18;
		theTint |= 0xff000000;
		theAlpha = (float)((double)(int)aTintAlpha * (double)(1.0f / 255.0f) * theAlpha);
	}
	if (theAlpha <= 0.0f)
		return;
	const Image* anImage = theFrame->mOwner;
	float anImageW = (float)anImage->mWidth;
	float anImageH = (float)anImage->mHeight;
	float aWf = (float)theFrame->mWidth;
	float aHf = (float)theFrame->mHeight;
	if (aWf <= 0.0f || aHf <= 0.0f || theScale <= 0.0f)
		return;
	double aY;
	if (theCorner != 0)
	{
		theX = (float)((double)aWf * theScale * 0.5 / gWidth + theX);
		aY = (double)aHf * theScale * 0.5 / gHeight + theY;
	}
	else
		aY = theY;
	int aCx = ftol((double)gWidth * theX + (double)0.01f);
	int aCy = ftol((double)gHeight * aY + (double)0.01f);
	int anAlpha = 0;
	if (theAlpha < 1.0f)
	{
		anAlpha = ftol((double)theAlpha * 255.0);
		if (anAlpha == 0)
			return;
	}
	double aTurnD = (double)theAngle * (double)0.0027777778f * (double)6.28318f;
	float aTurn = (float)aTurnD;
	if (aTurnD < 0.0)
	{
		do
			aTurnD += (double)628.318f;
		while (aTurnD < 0.0);
		aTurn = (float)aTurnD;
	}
	aTurn = (float)std::fmod((double)aTurn, (double)6.28318f);
	double aRadius = distance(0.0f, 0.0f, (float)((double)aWf * 0.5), (float)((double)aHf * 0.5)) * theScale;
	double aCorner = std::atan2(-((double)aWf / aHf), 1.0);
	float aNeg = (float)-aCorner;
	float anAngle2 = (float)((double)3.14159f + aCorner);
	float anAngle3 = (float)((double)aNeg + (double)3.14159f);
	double anAngle0 = aCorner + aTurn;
	float anAngle1 = (float)((double)aNeg + aTurn);
	anAngle2 = (float)((double)anAngle2 + aTurn);
	anAngle3 = (float)((double)anAngle3 + aTurn);
	float aCxf = (float)aCx;
	float aCyf = (float)aCy;
	// The corners, clockwise from the top left one when unturned
	float X[4], Y[4];
	X[0] = (float)(std::sin(anAngle0) * aRadius + aCxf);
	Y[0] = (float)(-std::cos(anAngle0) * aRadius + aCyf);
	X[1] = (float)(std::sin((double)anAngle1) * aRadius + aCxf);
	double aY1 = -std::cos((double)anAngle1) * aRadius + aCyf;
	Y[1] = (float)aY1;
	X[2] = (float)(std::sin((double)anAngle2) * aRadius + aCxf);
	Y[2] = (float)(-std::cos((double)anAngle2) * aRadius + aCyf);
	X[3] = (float)(std::sin((double)anAngle3) * aRadius + aCxf);
	Y[3] = (float)(-std::cos((double)anAngle3) * aRadius + aCyf);
	int aTop = aY1 < Y[0] ? 1 : 0;
	if (Y[2] < Y[aTop])
		aTop = 2;
	if (Y[3] < Y[aTop])
		aTop = 3;
	int aBottom = aTop - 2 & 3;
	int aRow = ftol((double)Y[aTop] + 0.5);
	int aLastRow = ftol((double)Y[aBottom] + 0.5);
	// The frame's corners in the image, in the same order
	double aWm = (double)theFrame->mWidth - (double)0.001f;
	double aHm = (double)theFrame->mHeight - (double)0.001f;
	float aFx = (float)theFrame->mX;
	float aFy = (float)theFrame->mY;
	float U[4], V[4];
	U[0] = aFx;
	U[1] = (float)((double)aFx + (double)(float)aWm);
	U[2] = (float)((double)aFx + aWm);
	U[3] = aFx;
	V[0] = aFy;
	V[1] = aFy;
	V[2] = (float)((double)aFy + (double)(float)aHm);
	V[3] = (float)((double)aFy + aHm);
	// The left edge runs from the top corner through the one before it to the bottom one, the right edge through the one
	// after it
	int aLeftFrom = aTop;
	int aLeftTo = aTop - 1 & 3;
	int aRightFrom = aTop;
	int aRightTo = aTop + 1 & 3;
	double aLeftX = X[aTop];
	double aLeftY = Y[aTop];
	float aLeftEndX = X[aLeftTo];
	float aRightX = X[aTop];
	float aRightY = Y[aTop];
	float aRightEndX = X[aRightTo];
	float aRightEndY = Y[aRightTo];
	const uint32_t* aSrc = anImage->mPixels;
	int aSrcPitch = anImage->mPitch >> 2;
	int aDstPitch = gPitch >> 2;
	if (aRow < gClipY0)
		aRow = gClipY0;
	if (aLastRow > gClipY1)
		aLastRow = gClipY1;
	if (aRow > aLastRow)
		return;
	double aLeftDy = (double)Y[aLeftTo] - aLeftY;
	do
	{
		double k = 0.1f > aLeftDy ? (double)0.1f : aLeftDy;
		float aRowF = (float)aRow;
		double t = ((double)aRowF - aLeftY + 0.5) / k;
		float aLeftT = (float)t;
		if (t > 1.0)
		{
			if (aLeftTo == aBottom)
				continue;
			aLeftFrom = aLeftTo;
			aLeftTo = aBottom;
			aLeftX = X[aLeftFrom];
			aLeftY = Y[aLeftFrom];
			aLeftEndX = X[aBottom];
			aLeftDy = (double)Y[aBottom] - aLeftY;
			k = 0.1f > aLeftDy ? (double)0.1f : aLeftDy;
			aLeftT = (float)(((double)aRowF - aLeftY + 0.5) / k);
		}
		double aRightDy = (double)aRightEndY - aRightY;
		k = 0.1f > aRightDy ? (double)0.1f : aRightDy;
		double aRightT = ((double)aRowF - aRightY + 0.5) / k;
		if (aRightT > 1.0)
		{
			if (aRightTo == aBottom)
				continue;
			aRightFrom = aRightTo;
			aRightX = X[aRightFrom];
			aRightY = Y[aRightFrom];
			aRightEndY = Y[aBottom];
			aRightTo = aBottom;
			aRightEndX = X[aBottom];
			aRightDy = (double)aRightEndY - aRightY;
			k = 0.1f > aRightDy ? (double)0.1f : aRightDy;
			aRightT = ((double)aRowF - aRightY + 0.5) / k;
		}
		double aLeftU = 1.0 - aLeftT;
		float u0 = (float)(aLeftT * (double)U[aLeftTo] + aLeftU * U[aLeftFrom]);
		float v0 = (float)(aLeftT * (double)V[aLeftTo] + aLeftU * V[aLeftFrom]);
		int x0 = ftol(aLeftU * aLeftX + aLeftT * (double)aLeftEndX);
		double aRightU = 1.0 - aRightT;
		int x1 = ftol((double)aRightX * aRightU + aRightT * aRightEndX);
		int aCount = x1 - x0 + 1;
		uint32_t* d = gPixels + (aRow * aDstPitch + x0);
		double aCountD = aCount;
		int du = ftol((aRightT * U[aRightTo] + aRightU * U[aRightFrom] - u0) / aCountD * 65536.0);
		int dv = ftol((aRightT * V[aRightTo] + aRightU * V[aRightFrom] - v0) / aCountD * 65536.0);
		int u = ftol((double)u0 * 65536.0);
		int v = ftol((double)v0 * 65536.0);
		if (x0 < 0)
		{
			int n = -x0;
			aCount -= n;
			u += n * du;
			v += n * dv;
			d += n;
		}
		if (x1 >= gClipX1 + 1)
			aCount += gClipX1 - x1;
		auto aPixel = [&](uint32_t* thePixel)
		{
			if (u >= 0 && (float)(u >> 0x10) < anImageW && v >= 0 && (float)(v >> 0x10) < anImageH)
			{
				uint32_t c = aSrc[(v >> 0x10) * aSrcPitch + (u >> 0x10)];
				int a = (int)(c >> 0x18);
				if (anAlpha != 0)
					a = a * anAlpha >> 8;
				if (theTint != 0)
					c = theTint;
				if (a == 0xff)
					*thePixel = c;
				else if (a > 0)
				{
					uint32_t aU = 0xff - a;
					uint32_t aScreen = *thePixel;
					*thePixel = ((aScreen >> 8 & 0xff) * aU + (c >> 8 & 0xff) * a & 0xff00) |
						((aScreen & 0xff00ff) * aU + (c & 0xff00ff) * a >> 8 & 0xff00ff);
				}
			}
			u += du;
			v += dv;
		};
		int i = 0;
		if (aCount >= 4)
		{
			int aGroups = (int)((unsigned)(aCount - 4) >> 2) + 1;
			i = aGroups * 4;
			do
			{
				aPixel(d);
				aPixel(d + 1);
				aPixel(d + 2);
				aPixel(d + 3);
				d += 4;
			} while (--aGroups != 0);
		}
		if (i < aCount)
		{
			int n = aCount - i;
			do
			{
				aPixel(d);
				d++;
			} while (--n != 0);
		}
	} while (++aRow <= aLastRow);
}

} // namespace bonnies
