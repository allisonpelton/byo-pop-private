// The game's utilities (0x420570..0x420ff0). Float expressions are worked in double, as the original's x87 code
// works them at 53 bits, and rounded to float where it stores them.

#include "WBGame.h"
#include "R64Sys.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace wb
{

namespace
{

int32_t gRandSeed = 0x058deac0;        // 0x460a64
const int32_t RAND_A = 0x5eb0a82f;     // 0x460a68
const int32_t RAND_R = 0x429eaf9d;     // 0x460a6c
const int32_t RAND_Q = 2;              // 0x460a70

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

void ParseHex4(const char* theText, int* theRed, int* theGreen, int* theBlue, int* theAlpha)
{
	char aPairs[4][3] = {{theText[0], theText[1], 0}, {theText[2], theText[3], 0}, {theText[4], theText[5], 0},
		{theText[6], theText[7], 0}};
	unsigned v = 0;
	std::sscanf(aPairs[0], "%x", &v);
	*theRed = (int)v;
	std::sscanf(aPairs[1], "%x", &v);
	*theGreen = (int)v;
	std::sscanf(aPairs[2], "%x", &v);
	*theBlue = (int)v;
	std::sscanf(aPairs[3], "%x", &v);
	*theAlpha = (int)v;
}

int gRandCalls = 0;   // (the port's: how many draws so far, for lining the port up with the original)

// 0x420770: Schrage's step with the original's constants (q = 2), in 32-bit wrapping arithmetic. The seed is divided
// unsigned (a div, not an idiv), so a seed with the top bit set gives a different step than the signed one would.
void Rand()
{
	gRandCalls++;
	uint32_t aSeed = (uint32_t)gRandSeed;
	gRandSeed = (int32_t)((aSeed % (uint32_t)RAND_Q) * (uint32_t)RAND_A - (aSeed / (uint32_t)RAND_Q) * (uint32_t)RAND_R);
}

void SeedRand(int theSeed)
{
	gRandSeed = theSeed;}

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
	double aDx = (double)b[0] - a[0];
	double aDy = (double)b[1] - a[1];
	double aDz = (double)b[2] - a[2];
	return (float)std::sqrt(aDz * aDz + aDx * aDx + aDy * aDy);
}

float SegmentDist3(const float* thePoint, const float* theStart, const float* theEnd)
{
	float aLen = Dist3(theEnd, theStart);
	float t = (float)((((double)theEnd[2] - theStart[2]) * ((double)thePoint[2] - theStart[2]) + ((double)thePoint[0] - theStart[0]) * ((double)theEnd[0] - theStart[0]) +
		((double)theEnd[1] - theStart[1]) * ((double)thePoint[1] - theStart[1])) / ((double)aLen * aLen));
	if (0.0f <= t && t <= 1.0f)
	{
		float q[3];
		q[0] = (float)(theStart[0] + (double)t * ((double)theEnd[0] - theStart[0]));
		q[1] = (float)(((double)theEnd[1] - theStart[1]) * t + theStart[1]);
		q[2] = (float)(((double)theEnd[2] - theStart[2]) * t + theStart[2]);
		return Dist3(thePoint, q);
	}
	float aD1 = Dist3(thePoint, theStart);
	float aD2 = Dist3(thePoint, theEnd);
	if (aD1 < aD2)
		theEnd = theStart;
	return Dist3(thePoint, theEnd);
}

float SegmentDist(float thePx, float thePy, float theAx, float theAy, float theBx, float theBy)
{
	float p[3] = {thePx, thePy, 0.0f};
	float a[3] = {theAx, theAy, 0.0f};
	float b[3] = {theBx, theBy, 0.0f};
	return SegmentDist3(p, a, b);
}

void ColourShift(uint32_t* thePixel, const PixelFormat* theFmt, char theRed, char theGreen, char theBlue,
	uint8_t theMulR, uint8_t theMulG, uint8_t theMulB, int theSetR, int theSetG, int theSetB, int theShare)
{
	uint32_t aPixelValue = *thePixel;
	uint8_t aRed = (uint8_t)(((theFmt->mRMask & aPixelValue) >> (theFmt->mRShift & 0x1f)) << (theFmt->mRLoss & 0x1f));
	uint8_t aGreen = (uint8_t)(((theFmt->mGMask & aPixelValue) >> (theFmt->mGShift & 0x1f)) << (theFmt->mGLoss & 0x1f));
	uint8_t aBlue = (uint8_t)(((theFmt->mBMask & aPixelValue) >> (theFmt->mBShift & 0x1f)) << (theFmt->mBLoss & 0x1f));
	bool aChanged = false;
	bool aRedChanged = false;
	uint32_t aB = aBlue;
	uint32_t aTop = (uint32_t)(uint8_t)theRed;
	uint8_t aR = aRed;
	if (theRed != 0 && (int)((aGreen * 100) / theShare) < (int)aRed && (int)((aBlue * 100) / theShare) < (int)aRed)
	{
		aChanged = true;
		aRedChanged = true;
		aTop = aRed;
		aR = (uint8_t)(((uint32_t)aBlue + aGreen) / 2);
	}
	uint8_t aG = aGreen;
	if (theGreen != 0)
	{
		aChanged = aRedChanged;
		if ((int)((aBlue * 100) / theShare) < (int)aGreen && (int)((aRed * 100) / theShare) < (int)aGreen)
		{
			aTop = aGreen;
			aG = (uint8_t)(((uint32_t)aRed + aBlue) / 2);
			aChanged = true;
		}
	}
	if (theBlue != 0 && (int)((aGreen * 100) / theShare) < (int)aBlue && (int)((aRed * 100) / theShare) < (int)aBlue)
	{
		aB = ((uint32_t)aRed + aGreen) / 2;
		aTop = aBlue;
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
	*thePixel = (aB >> (theFmt->mBLoss & 0x1f)) << (theFmt->mBShift & 0x1f) |
		(uint32_t)(aG >> (theFmt->mGLoss & 0x1f)) << (theFmt->mGShift & 0x1f) |
		(uint32_t)(aR >> (theFmt->mRLoss & 0x1f)) << (theFmt->mRShift & 0x1f);
}

// The original parses the swap's colours and reads every pixel of the image, then keeps nothing: it changes
// nothing
void SwapColours(Image* theImage, const char* theSwap)
{
	(void)theImage;
	(void)theSwap;
}

double CircleOverlap(float theX1, float theY1, float theR1, float theX2, float theY2, float theR2)
{
	double v = ((double)theR1 + theR2) * ((double)theR1 + theR2) - (((double)theY1 - theY2) * ((double)theY1 - theY2) +
		((double)theX1 - theX2) * ((double)theX1 - theX2));
	if (v < 0.0)
		return 0.0;
	return v;
}

int BoxOverlap(float theX1, float theY1, float theW1, float theH1, float theX2, float theY2, float theW2, float theH2)
{
	float aHalf = (float)((double)theW2 * 0.5 + (double)theW1 * 0.5);
	if (std::fabs((double)theX1 - theX2) <= aHalf)
	{
		aHalf = (float)((double)theH1 * 0.5 + (double)theH2 * 0.5);
		if (std::fabs((double)theY1 - theY2) <= aHalf)
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
	if (theImage->mHasMask != 0)
	{
		aCopy->mHasMask = 1;
		Image* aMask = new Image();
		aCopy->mMask = aMask;
		Image_Copy(aMask, theImage->mMask);
	}
	Fill8(theImage->mBits, theImage->mPitch, 0, 0, theImage->mW + -1, theImage->mH + -1, 0);
	Fill8(theImage->mMask->mBits, theImage->mMask->mPitch, 0, 0, theImage->mW + -1, theImage->mH + -1, 0);
	int aCx = theImage->mW / 2;
	int aCy = theImage->mH / 2;
	bool aTurn = true;
	if (theAngle == 0x5a)
		Image_BlitRotated(aCopy, theImage->mBits, theImage->mPitch, &theImage->mFmt, aCx, aCy, 0x5a, theImage->mClip);
	else if (theAngle == 0xb4)
	{
		if ((theImage->mH / 2) * 2 == theImage->mH)
			aCy = aCy + -1;
		Image_BlitRotated(aCopy, theImage->mBits, theImage->mPitch, &theImage->mFmt, aCx, aCy, 0xb4, theImage->mClip);
	}
	else if (theAngle == 0x10e)
	{
		if ((theImage->mW / 2) * 2 == theImage->mW)
			aCx = aCx + -1;
		Image_BlitRotated(aCopy, theImage->mBits, theImage->mPitch, &theImage->mFmt, aCx, aCy, 0x10e, theImage->mClip);
	}
	else
		aTurn = false;
	if (aTurn)
	{
		Image* aMask = theImage->mMask;
		Image_BlitRotated(aCopy->mMask, aMask->mBits, aMask->mPitch, &aMask->mFmt, aCx, aCy, theAngle, aMask->mClip);
	}
	if (aCopy->mHasMask != 0 && aCopy->mMask != nullptr)
	{
		Image_Destroy(aCopy->mMask);
		delete aCopy->mMask;
	}
	Image_Destroy(aCopy);
	delete aCopy;
}

} // namespace wb
