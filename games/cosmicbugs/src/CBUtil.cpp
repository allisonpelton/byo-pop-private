// The game's utilities (0x420090..0x420b10). Float expressions are worked in double, as the original's x87 code
// works them at 53 bits, and rounded to float where it stores them.

#include "CBGame.h"
#include "R64Sys.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace cb
{

namespace
{

int32_t gRandSeed = 0x058deac0;        // 0x460a64
const int32_t kRandA = 0x5eb0a82f;     // 0x460a68
const int32_t kRandR = 0x429eaf9d;     // 0x460a6c
const int32_t kRandQ = 2;              // 0x460a70

} // namespace

uint8_t* ReadWholeFile(const char* thePath, size_t* theSize)
{
	std::vector<uint8_t> aData;
	if (!Sys_ReadFile(thePath, aData))
		return nullptr;
	*theSize = aData.size();
	uint8_t* aBuf = (uint8_t*)std::malloc(aData.size() + 1);
	if (!aData.empty())
		std::memcpy(aBuf, aData.data(), aData.size());
	aBuf[aData.size()] = 0;
	return aBuf;
}

void ParseHex4(const char* theText, int* a, int* b, int* c, int* d)
{
	char aPairs[4][3] = {{theText[0], theText[1], 0}, {theText[2], theText[3], 0}, {theText[4], theText[5], 0},
		{theText[6], theText[7], 0}};
	unsigned v = 0;
	std::sscanf(aPairs[0], "%x", &v);
	*a = (int)v;
	std::sscanf(aPairs[1], "%x", &v);
	*b = (int)v;
	std::sscanf(aPairs[2], "%x", &v);
	*c = (int)v;
	std::sscanf(aPairs[3], "%x", &v);
	*d = (int)v;
}

void ParseHex3(const char* theText, int* a, int* b, int* c)
{
	char aPairs[3][3] = {{theText[0], theText[1], 0}, {theText[2], theText[3], 0}, {theText[4], theText[5], 0}};
	unsigned v = 0;
	std::sscanf(aPairs[0], "%x", &v);
	*a = (int)v;
	std::sscanf(aPairs[1], "%x", &v);
	*b = (int)v;
	std::sscanf(aPairs[2], "%x", &v);
	*c = (int)v;
}

// Schrage's step with the original's constants (q = 2), in 32-bit wrapping arithmetic
void Rand()
{
	gRandSeed = (int32_t)((uint32_t)(gRandSeed % kRandQ) * (uint32_t)kRandA -
		(uint32_t)(gRandSeed / kRandQ) * (uint32_t)kRandR);
}

void SeedRand(int theSeed)
{
	gRandSeed = theSeed;
}

uint32_t RandMod(uint32_t theRange)
{
	Rand();
	return (uint32_t)gRandSeed % theRange;
}

int RandRange(int theLow, int theHigh)
{
	Rand();
	return (int)((uint32_t)gRandSeed % ((uint32_t)(theHigh - theLow) + 1u) + (uint32_t)theLow);
}

int RandValue()
{
	return gRandSeed;
}

float Dist3(const float* a, const float* b)
{
	double dx = (double)b[0] - a[0];
	double dy = (double)b[1] - a[1];
	double dz = (double)b[2] - a[2];
	return (float)std::sqrt(dz * dz + dx * dx + dy * dy);
}

float SegmentDist3(const float* p, const float* a, const float* b)
{
	float aLen = Dist3(b, a);
	float t = (float)((((double)b[2] - a[2]) * ((double)p[2] - a[2]) + ((double)p[0] - a[0]) * ((double)b[0] - a[0]) +
		((double)b[1] - a[1]) * ((double)p[1] - a[1])) / ((double)aLen * aLen));
	if (0.0f <= t && t <= 1.0f)
	{
		float q[3];
		q[0] = (float)(a[0] + (double)t * ((double)b[0] - a[0]));
		q[1] = (float)(((double)b[1] - a[1]) * t + a[1]);
		q[2] = (float)(((double)b[2] - a[2]) * t + a[2]);
		return Dist3(p, q);
	}
	float d1 = Dist3(p, a);
	float d2 = Dist3(p, b);
	if (d1 < d2)
		b = a;
	return Dist3(p, b);
}

float SegmentDist(float px, float py, float ax, float ay, float bx, float by)
{
	float p[3] = {px, py, 0.0f};
	float a[3] = {ax, ay, 0.0f};
	float b[3] = {bx, by, 0.0f};
	return SegmentDist3(p, a, b);
}

void ColourShift(uint32_t* thePixel, const PixelFormat* theFmt, char theRed, char theGreen, char theBlue,
	uint8_t theMulR, uint8_t theMulG, uint8_t theMulB, int theSetR, int theSetG, int theSetB, int theShare)
{
	uint32_t v = *thePixel;
	uint8_t r = (uint8_t)(((theFmt->rMask & v) >> (theFmt->rShift & 0x1f)) << (theFmt->rLoss & 0x1f));
	uint8_t g = (uint8_t)(((theFmt->gMask & v) >> (theFmt->gShift & 0x1f)) << (theFmt->gLoss & 0x1f));
	uint8_t b = (uint8_t)(((theFmt->bMask & v) >> (theFmt->bShift & 0x1f)) << (theFmt->bLoss & 0x1f));
	bool aChanged = false;
	bool aRedChanged = false;
	uint32_t aB = b;
	uint32_t aTop = (uint32_t)(uint8_t)theRed;
	uint8_t aR = r;
	if (theRed != 0 && (int)((g * 100) / theShare) < (int)r && (int)((b * 100) / theShare) < (int)r)
	{
		aChanged = true;
		aRedChanged = true;
		aTop = r;
		aR = (uint8_t)(((uint32_t)b + g) / 2);
	}
	uint8_t aG = g;
	if (theGreen != 0)
	{
		aChanged = aRedChanged;
		if ((int)((b * 100) / theShare) < (int)g && (int)((r * 100) / theShare) < (int)g)
		{
			aTop = g;
			aG = (uint8_t)(((uint32_t)r + b) / 2);
			aChanged = true;
		}
	}
	if (theBlue != 0 && (int)((g * 100) / theShare) < (int)b && (int)((r * 100) / theShare) < (int)b)
	{
		aB = ((uint32_t)r + g) / 2;
		aTop = b;
	}
	else if (!aChanged)
		return;
	uint8_t aTopByte = (uint8_t)aTop;
	if (theSetR != 0)
		aR = aTopByte;
	if (theSetG != 0)
		aG = aTopByte;
	if (theSetB != 0)
		aB = aTop & 0xff;
	if (theMulR != 0)
	{
		int t = (int)((uint32_t)aR * theMulR) >> 7;
		aR = aTopByte;
		if (t <= (int)aTop)
			aR = (uint8_t)t;
	}
	if (theMulG != 0)
	{
		int t = (int)((uint32_t)aG * theMulG) >> 7;
		aG = aTopByte;
		if (t <= (int)aTop)
			aG = (uint8_t)t;
	}
	if (theMulB != 0)
	{
		uint32_t t = (uint32_t)((int)(aB * theMulB) >> 7);
		if ((int)t <= (int)aTop)
			aTop = t;
		aB = aTop & 0xff;
	}
	*thePixel = (aB >> (theFmt->bLoss & 0x1f)) << (theFmt->bShift & 0x1f) |
		(uint32_t)(aG >> (theFmt->gLoss & 0x1f)) << (theFmt->gShift & 0x1f) |
		(uint32_t)(aR >> (theFmt->rLoss & 0x1f)) << (theFmt->rShift & 0x1f);
}

// The original parses the swap's colours and reads every pixel of the image, then keeps nothing: it changes
// nothing
void SwapColours(Image* theImage, const char* theSwap)
{
	(void)theImage;
	(void)theSwap;
}

double CircleOverlap(float x1, float y1, float r1, float x2, float y2, float r2)
{
	double v = ((double)r1 + r2) * ((double)r1 + r2) - (((double)y1 - y2) * ((double)y1 - y2) +
		((double)x1 - x2) * ((double)x1 - x2));
	if (v < 0.0)
		return 0.0;
	return v;
}

int BoxOverlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2)
{
	float aHalf = (float)((double)w2 * 0.5 + (double)w1 * 0.5);
	if (std::fabs((double)x1 - x2) <= aHalf)
	{
		aHalf = (float)((double)h1 * 0.5 + (double)h2 * 0.5);
		if (std::fabs((double)y1 - y2) <= aHalf)
			return 1;
	}
	return 0;
}

// 90, 180 and 270 turn the frame about its middle (an even size's middle one pixel back for 180 and 270); the fill
// that clears it first is 8-bit, so it clears the first w bytes of each row whatever the depth
void RotateImage(Image* theImage, int theAngle)
{
	Image* aCopy = new Image();
	Image_Copy(aCopy, theImage);
	if (theImage->hasMask != 0)
	{
		aCopy->hasMask = 1;
		Image* aMask = new Image();
		aCopy->mask = aMask;
		Image_Copy(aMask, theImage->mask);
	}
	Fill8(theImage->bits, theImage->pitch, 0, 0, theImage->w + -1, theImage->h + -1, 0);
	Fill8(theImage->mask->bits, theImage->mask->pitch, 0, 0, theImage->w + -1, theImage->h + -1, 0);
	int cx = theImage->w / 2;
	int cy = theImage->h / 2;
	bool aTurn = true;
	if (theAngle == 0x5a)
		Image_BlitRotated(aCopy, theImage->bits, theImage->pitch, &theImage->fmt, cx, cy, 0x5a, theImage->clip);
	else if (theAngle == 0xb4)
	{
		if ((theImage->h / 2) * 2 == theImage->h)
			cy = cy + -1;
		Image_BlitRotated(aCopy, theImage->bits, theImage->pitch, &theImage->fmt, cx, cy, 0xb4, theImage->clip);
	}
	else if (theAngle == 0x10e)
	{
		if ((theImage->w / 2) * 2 == theImage->w)
			cx = cx + -1;
		Image_BlitRotated(aCopy, theImage->bits, theImage->pitch, &theImage->fmt, cx, cy, 0x10e, theImage->clip);
	}
	else
		aTurn = false;
	if (aTurn)
	{
		Image* aMask = theImage->mask;
		Image_BlitRotated(aCopy->mask, aMask->bits, aMask->pitch, &aMask->fmt, cx, cy, theAngle, aMask->clip);
	}
	if (aCopy->hasMask != 0 && aCopy->mask != nullptr)
	{
		Image_Destroy(aCopy->mask);
		delete aCopy->mask;
	}
	Image_Destroy(aCopy);
	delete aCopy;
}

void Log(const char* theFormat, ...)
{
	(void)theFormat;
}

} // namespace cb
