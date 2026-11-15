#include "LinearStretch.h"

#include <algorithm>
#include <string.h>

using namespace Sexy;

LinearStretch::LinearStretch()
{
	mSrcWidth = 0;
	mSrcHeight = 0;
	mRenderScale = 0;
}

// theNumerator / theDenominator (both above 0) as a 32-bit float holds it, rounded to nearest (ties to even) or toward zero:
// *theMantissa / 2^*theExponent, *theMantissa of 24 bits
static void FloatMantissa(uint64_t theNumerator, uint64_t theDenominator, bool theRoundNearest, uint64_t* theMantissa, int* theExponent)
{
	// theNumerator / theDenominator = aMantissa / 2^anExponent with aMantissa in [2^23, 2^24)
	int anExponent = 0;
	while (theNumerator < (theDenominator << 23))
	{
		theNumerator <<= 1;
		anExponent++;
	}
	while (theNumerator >= (theDenominator << 24))
	{
		theDenominator <<= 1;
		anExponent--;
	}
	uint64_t aMantissa = theNumerator / theDenominator;
	const uint64_t aRest = theNumerator - aMantissa * theDenominator;
	if (theRoundNearest && ((aRest * 2 > theDenominator) || ((aRest * 2 == theDenominator) && ((aMantissa & 1) != 0))))
	{
		aMantissa++;
		if (aMantissa == ((uint64_t) 1 << 24))
		{
			aMantissa >>= 1;
			anExponent--;
		}
	}
	*theMantissa = aMantissa;
	*theExponent = anExponent;
}

void LinearStretch::MakeTaps(int theSrcSize, int theRenderScale, int theOutSize, int theOtherSize, std::vector<Tap>& theTaps)
{
	// the screen texture's texels: the frame's pixels, each repeated theRenderScale times
	const int64_t aTexels = (int64_t) theSrcSize * theRenderScale;
	theTaps.resize(theOutSize);
	if (theOutSize <= 0)
		return;
	// the texture coordinate's step from one pixel to the next as the card's triangle setup makes it: the rect's other side
	// times 1 / (width * height), that a 32-bit float rounded to nearest, the product rounded toward zero; aStep /
	// 2^aStepExponent, aStep of 24 bits. (The card's reciprocal is itself one unit of the last place off at some sizes, so
	// a few seams there differ by one level: docs/MEMORY.md.)
	uint64_t aReciprocal;
	int aReciprocalExponent;
	FloatMantissa(1, (uint64_t) theOutSize * (uint64_t) std::max(1, theOtherSize), true, &aReciprocal, &aReciprocalExponent);
	uint64_t aStep;
	int aStepExponent;
	FloatMantissa((uint64_t) std::max(1, theOtherSize) * aReciprocal, 1, false, &aStep, &aStepExponent);
	aStepExponent += aReciprocalExponent;
	for (int i = 0; i < theOutSize; i++)
	{
		// the texture coordinate of the pixel's centre, (i + 0.5) / theOutSize, truncated to 21 fractional bits
		const int64_t aCoord = (int64_t) (((uint64_t) (2 * i + 1) * aStep) >> (aStepExponent - 21 + 1));
		// the position in texels less the texel centre's half, rounded (half up) to 8 fractional bits: the texel and the
		// second texel's weight
		const int64_t aPos = ((aCoord * aTexels + 4096) >> 13) - 128;
		int64_t aT0 = (aPos >= 0) ? (aPos >> 8) : -((255 - aPos) >> 8);
		const int aWeight = (int) (aPos - aT0 * 256);
		int64_t aT1 = aT0 + 1;
		// the edges clamped
		aT0 = std::max<int64_t>(0, std::min<int64_t>(aTexels - 1, aT0));
		aT1 = std::max<int64_t>(0, std::min<int64_t>(aTexels - 1, aT1));
		Tap& t = theTaps[i];
		t.mA = (int) (aT0 / theRenderScale);
		t.mB = (int) (aT1 / theRenderScale);
		t.mWeight = (t.mA == t.mB) ? 0 : aWeight;
		if (t.mWeight == 0)
			t.mB = t.mA;
	}
}

void LinearStretch::Setup(int theSrcWidth, int theSrcHeight, int theRenderScale, int theOutWidth, int theOutHeight)
{
	mSrcWidth = theSrcWidth;
	mSrcHeight = theSrcHeight;
	mRenderScale = std::max(1, theRenderScale);
	MakeTaps(theSrcWidth, mRenderScale, std::max(0, theOutWidth), theOutHeight, mCols);
	MakeTaps(theSrcHeight, mRenderScale, std::max(0, theOutHeight), theOutWidth, mRows);
	// the columns in runs of the same taps: a frame pixel's copies are one value, worked out once a row
	mColSpans.clear();
	mColSpanOf.resize(mCols.size());
	for (int x = 0; x < (int) mCols.size(); x++)
	{
		const Tap& t = mCols[x];
		if ((!mColSpans.empty()) && (mColSpans.back().mTap.mA == t.mA) && (mColSpans.back().mTap.mB == t.mB) &&
			(mColSpans.back().mTap.mWeight == t.mWeight))
			mColSpans.back().mCount++;
		else
		{
			Span aSpan = { x, 1, t };
			mColSpans.push_back(aSpan);
		}
		mColSpanOf[x] = (int) mColSpans.size() - 1;
	}
}

bool LinearStretch::IsSetFor(int theSrcWidth, int theSrcHeight, int theRenderScale, int theOutWidth, int theOutHeight) const
{
	return (mSrcWidth == theSrcWidth) && (mSrcHeight == theSrcHeight) && (mRenderScale == theRenderScale) &&
		((int) mCols.size() == theOutWidth) && ((int) mRows.size() == theOutHeight);
}

// one channel's level from the weighted sum of its texels (the weights summing to 256): the card keeps the sum to 12 bits
// (times 257/4096, rounding half up from 1/32 of a step) and brings that to 8 bits rounding half down. A table of every
// sum (0 to 255 * 256), made once.
static const uint8_t* LevelTable()
{
	static uint8_t sTable[255 * 256 + 1];
	static bool sMade = false;
	if (!sMade)
	{
		for (uint32_t s = 0; s <= 255 * 256; s++)
			sTable[s] = (uint8_t) (((((s * 257 + 128) >> 12) * 255) + 2047) >> 12);
		sMade = true;
	}
	return sTable;
}

// two pixels blended, the second at theWeight (1 to 255)
static inline uint32_t Blend2(const uint8_t* theLevel, uint32_t theA, uint32_t theB, uint32_t theWeight)
{
	const uint32_t wa = 256 - theWeight;
	return (uint32_t) theLevel[(theA & 255) * wa + (theB & 255) * theWeight] |
		((uint32_t) theLevel[((theA >> 8) & 255) * wa + ((theB >> 8) & 255) * theWeight] << 8) |
		((uint32_t) theLevel[((theA >> 16) & 255) * wa + ((theB >> 16) & 255) * theWeight] << 16);
}

// four pixels blended as the card blends them where both axes do: the product of the two weights rounded (half up) to 8
// bits, the other three weights from it, so that they sum to 256
static inline uint32_t Blend4(const uint8_t* theLevel, uint32_t p00, uint32_t p10, uint32_t p01, uint32_t p11, uint32_t wx, uint32_t wy)
{
	const uint32_t w11 = (wx * wy + 128) >> 8;
	const uint32_t w10 = wx - w11;
	const uint32_t w01 = wy - w11;
	const uint32_t w00 = 256 - wx - wy + w11;
	uint32_t aPixel = 0;
	for (int aShift = 0; aShift <= 16; aShift += 8)
		aPixel |= (uint32_t) theLevel[((p00 >> aShift) & 255) * w00 + ((p10 >> aShift) & 255) * w10 + ((p01 >> aShift) & 255) * w01 +
			((p11 >> aShift) & 255) * w11] << aShift;
	return aPixel;
}

// theCount pixels of theValue
static inline uint32_t* Fill(uint32_t* theOut, uint32_t theValue, int theCount)
{
	for (int i = 0; i < theCount; i++)
		theOut[i] = theValue;
	return theOut + theCount;
}

void LinearStretch::Draw(const uint32_t* thePixels, int thePitch, int theX0, int theY0, int theX1, int theY1, uint32_t* theOut,
	int theOutPitch) const
{
	theX0 = std::max(0, theX0);
	theY0 = std::max(0, theY0);
	theX1 = std::min((int) mCols.size(), theX1);
	theY1 = std::min((int) mRows.size(), theY1);
	if ((theX0 >= theX1) || (theY0 >= theY1))
		return;
	const uint8_t* aLevel = LevelTable();
	// the column runs of theX0 to theX1, the first and last cut to the range
	const int aFirstSpan = mColSpanOf[theX0];
	const int aLastSpan = mColSpanOf[theX1 - 1];
	const int aSpanCount = aLastSpan - aFirstSpan + 1;
	const Span* aSpans = &mColSpans[aFirstSpan];
	const int aHeadCount = std::min(aSpans[0].mFirst + aSpans[0].mCount, theX1) - theX0;
	const int aTailCount = (aSpanCount > 1) ? (theX1 - aSpans[aSpanCount - 1].mFirst) : aHeadCount;
	const int aWidth = theX1 - theX0;
	const Tap* aLastRow = NULL;
	const uint32_t* aLastOut = NULL;
	for (int y = theY0; y < theY1; y++)
	{
		const Tap& r = mRows[y];
		uint32_t* aOut = theOut + (size_t) (y - theY0) * theOutPitch;
		// a row with the same taps as the one above is the same row
		if ((aLastRow != NULL) && (aLastRow->mA == r.mA) && (aLastRow->mB == r.mB) && (aLastRow->mWeight == r.mWeight))
		{
			memcpy(aOut, aLastOut, (size_t) aWidth * 4);
			aLastOut = aOut;
			continue;
		}
		aLastRow = &r;
		aLastOut = aOut;
		const uint32_t* aRowA = (const uint32_t*) ((const uint8_t*) thePixels + (size_t) r.mA * thePitch);
		const uint32_t* aRowB = (const uint32_t*) ((const uint8_t*) thePixels + (size_t) r.mB * thePitch);
		const uint32_t wy = (uint32_t) r.mWeight;
		for (int s = 0; s < aSpanCount; s++)
		{
			const Tap& c = aSpans[s].mTap;
			const int aCount = (s == 0) ? aHeadCount : ((s == aSpanCount - 1) ? aTailCount : aSpans[s].mCount);
			uint32_t aValue;
			if (wy == 0)
			{
				// one row of the frame: only the columns' seams blend
				aValue = (c.mWeight == 0) ? (aRowA[c.mA] & 0xFFFFFF) : Blend2(aLevel, aRowA[c.mA], aRowA[c.mB], (uint32_t) c.mWeight);
			}
			else if (c.mWeight == 0)
			{
				// two rows of the frame, one column
				aValue = Blend2(aLevel, aRowA[c.mA], aRowB[c.mA], wy);
			}
			else
				aValue = Blend4(aLevel, aRowA[c.mA], aRowA[c.mB], aRowB[c.mA], aRowB[c.mB], (uint32_t) c.mWeight, wy);
			aOut = Fill(aOut, aValue, aCount);
		}
	}
}

void LinearStretch::OutputRange(const std::vector<Tap>& theTaps, int theSrc0, int theSrc1, int* theOut0, int* theOut1)
{
	// the taps rise with the output, so the outputs reading the range are one run
	int aFirst = -1, aLast = -1;
	for (int i = 0; i < (int) theTaps.size(); i++)
	{
		const Tap& t = theTaps[i];
		if ((t.mB >= theSrc0) && (t.mA < theSrc1))
		{
			if (aFirst < 0)
				aFirst = i;
			aLast = i;
		}
		else if (aFirst >= 0)
			break;
	}
	*theOut0 = (aFirst < 0) ? 0 : aFirst;
	*theOut1 = (aFirst < 0) ? 0 : aLast + 1;
}

void LinearStretch::OutputColumns(int theSrc0, int theSrc1, int* theOut0, int* theOut1) const
{
	OutputRange(mCols, theSrc0, theSrc1, theOut0, theOut1);
}

void LinearStretch::OutputRows(int theSrc0, int theSrc1, int* theOut0, int* theOut1) const
{
	OutputRange(mRows, theSrc0, theSrc1, theOut0, theOut1);
}
