#include "LinearStretch.h"

#include <algorithm>
#include <string.h>

// SSE2 for the blends where the compiler targets it (the native build: framework/CMakeLists.txt's -msse2); otherwise the
// same arithmetic one pixel at a time
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
#define LINEARSTRETCH_SSE2 1
#include <emmintrin.h>
#endif

using namespace Sexy;

LinearStretch::LinearStretch()
{
	mSrcWidth = 0;
	mSrcHeight = 0;
	mRenderScale = 0;
	mSeamsAdjacent = true;
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
	// each column's first pixel (a copy column's value), and the seams, where two pixels blend
	mColSource.resize(mCols.size());
	mSeams.clear();
	mSeamWeights.clear();
	mSeamFrom.resize(mCols.size() + 1);
	mSeamsAdjacent = true;
	for (int x = 0; x < (int) mCols.size(); x++)
	{
		const Tap& t = mCols[x];
		mColSource[x] = t.mA;
		mSeamFrom[x] = (int) mSeams.size();
		if (t.mWeight != 0)
		{
			Seam aSeam = { x, t.mA, t.mB, t.mWeight };
			mSeams.push_back(aSeam);
			mSeamWeights.push_back((uint64_t) t.mWeight * 0x0001000100010001ULL);
			// the second texel is the first's neighbour, so a seam's second pixel is the next one (the edges clamp to one pixel,
			// which makes no seam)
			if (t.mB != t.mA + 1)
				mSeamsAdjacent = false;
		}
	}
	mSeamFrom[mCols.size()] = (int) mSeams.size();
	mBlendedRow.assign((size_t) std::max(1, theSrcWidth), 0);
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

#ifdef LINEARSTRETCH_SSE2
// LevelTable's arithmetic in eight 16-bit lanes at once. ((s * 257 + 128) >> 12) is (s + ((s + 128) >> 8)) >> 4, and
// ((q * 255 + 2047) >> 12) is (q + ((2047 - q) >> 8)) >> 4 with the inner shift arithmetic (it may be negative), so no
// lane leaves 16 bits for any sum up to 255 * 256 (CheckLevels compares every one with the table).
static inline __m128i Levels(__m128i theSums)
{
	const __m128i aQ = _mm_srli_epi16(_mm_add_epi16(theSums, _mm_srli_epi16(_mm_add_epi16(theSums, _mm_set1_epi16(128)), 8)), 4);
	return _mm_srli_epi16(_mm_add_epi16(aQ, _mm_srai_epi16(_mm_sub_epi16(_mm_set1_epi16(2047), aQ), 8)), 4);
}

// sixteen sums (four pixels' channels, in the low and the high lanes) levelled and packed as four pixels, the top byte 0
static inline __m128i PackPixels(__m128i theLow, __m128i theHigh)
{
	return _mm_and_si128(_mm_packus_epi16(Levels(theLow), Levels(theHigh)), _mm_set1_epi32(0xFFFFFF));
}

// two pixels' channels weighted, each weight over its pixel's four lanes: theA * (256 - theW) + theB * theW. Every product
// and sum stays below 2^16 (at most 255 * 256), so the low halves of the 16-bit products are exact.
static inline __m128i Sums2(__m128i theA, __m128i theB, __m128i theW)
{
	return _mm_add_epi16(_mm_mullo_epi16(theA, _mm_sub_epi16(_mm_set1_epi16(256), theW)), _mm_mullo_epi16(theB, theW));
}

// the same for the four pixels of a two-axis blend, the weights made from theWx and theWy as Blend4 makes them
static inline __m128i Sums4(__m128i p00, __m128i p10, __m128i p01, __m128i p11, __m128i theWx, __m128i theWy)
{
	const __m128i w11 = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(theWx, theWy), _mm_set1_epi16(128)), 8);
	const __m128i w10 = _mm_sub_epi16(theWx, w11);
	const __m128i w01 = _mm_sub_epi16(theWy, w11);
	const __m128i w00 = _mm_add_epi16(_mm_sub_epi16(_mm_sub_epi16(_mm_set1_epi16(256), theWx), theWy), w11);
	return _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(p00, w00), _mm_mullo_epi16(p10, w10)),
		_mm_add_epi16(_mm_mullo_epi16(p01, w01), _mm_mullo_epi16(p11, w11)));
}

// four seams' pixels from theRow, each seam's two read at once (its second is the first's right neighbour): the first
// pixels in *theFirst, the second in *theSecond, seam 0 lowest
template <class SeamType>
static inline void Pairs(const uint32_t* theRow, const SeamType* theSeams, __m128i* theFirst, __m128i* theSecond)
{
	// [first 0, second 0, first 1, second 1] and the same for seams 2 and 3
	const __m128i a01 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*) (theRow + theSeams[0].mA)),
		_mm_loadl_epi64((const __m128i*) (theRow + theSeams[1].mA)));
	const __m128i a23 = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*) (theRow + theSeams[2].mA)),
		_mm_loadl_epi64((const __m128i*) (theRow + theSeams[3].mA)));
	// [first 0, first 2, second 0, second 2] and [first 1, first 3, second 1, second 3]
	const __m128i t0 = _mm_unpacklo_epi32(a01, a23);
	const __m128i t1 = _mm_unpackhi_epi32(a01, a23);
	*theFirst = _mm_unpacklo_epi32(t0, t1);
	*theSecond = _mm_unpackhi_epi32(t0, t1);
}
#endif

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
	const int aWidth = theX1 - theX0;
	const int* aSource = &mColSource[theX0];
	const int aSeamCount = mSeamFrom[theX1] - mSeamFrom[theX0];
	const Seam* aSeams = (aSeamCount > 0) ? &mSeams[mSeamFrom[theX0]] : NULL;
	// the frame's columns the range reads (the taps rise with the output)
	const int aSrc0 = mCols[theX0].mA;
	const int aSrc1 = mCols[theX1 - 1].mB + 1;
	uint32_t* aBlended = &mBlendedRow[0];
#ifdef LINEARSTRETCH_SSE2
	const __m128i aZero = _mm_setzero_si128();
#endif
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
		const int wy = r.mWeight;
		// the copy columns' values: one row of the frame, or between two rows each column of the first blended with the
		// second's, worked out once for each of the frame's columns the range reads
		const uint32_t* aCopies = aRowA;
		if (wy != 0)
		{
			int i = aSrc0;
#ifdef LINEARSTRETCH_SSE2
			const __m128i aWy = _mm_set1_epi16((short) wy);
			for (; i + 4 <= aSrc1; i += 4)
			{
				const __m128i a = _mm_loadu_si128((const __m128i*) (aRowA + i));
				const __m128i b = _mm_loadu_si128((const __m128i*) (aRowB + i));
				_mm_storeu_si128((__m128i*) (aBlended + i), PackPixels(Sums2(_mm_unpacklo_epi8(a, aZero), _mm_unpacklo_epi8(b, aZero), aWy),
					Sums2(_mm_unpackhi_epi8(a, aZero), _mm_unpackhi_epi8(b, aZero), aWy)));
			}
#endif
			for (; i < aSrc1; i++)
				aBlended[i] = Blend2(aLevel, aRowA[i], aRowB[i], (uint32_t) wy);
			aCopies = aBlended;
		}
		if (aCopies == aRowA)
		{
			for (int i = 0; i < aWidth; i++)
				aOut[i] = aRowA[aSource[i]] & 0xFFFFFF;
		}
		else
		{
			for (int i = 0; i < aWidth; i++)
				aOut[i] = aBlended[aSource[i]];
		}
		// the seams over them, four at a time
		int k = 0;
#ifdef LINEARSTRETCH_SSE2
		const __m128i aWy = _mm_set1_epi16((short) wy);
		const uint64_t* aWeights = (aSeamCount > 0) ? &mSeamWeights[mSeamFrom[theX0]] : NULL;
		for (; mSeamsAdjacent && (k + 4 <= aSeamCount); k += 4)
		{
			const Seam* s = aSeams + k;
			const __m128i aWxLow = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*) (aWeights + k)),
				_mm_loadl_epi64((const __m128i*) (aWeights + k + 1)));
			const __m128i aWxHigh = _mm_unpacklo_epi64(_mm_loadl_epi64((const __m128i*) (aWeights + k + 2)),
				_mm_loadl_epi64((const __m128i*) (aWeights + k + 3)));
			__m128i a0, a1;
			Pairs(aRowA, s, &a0, &a1);
			__m128i aPixels;
			if (wy == 0)
				aPixels = PackPixels(Sums2(_mm_unpacklo_epi8(a0, aZero), _mm_unpacklo_epi8(a1, aZero), aWxLow),
					Sums2(_mm_unpackhi_epi8(a0, aZero), _mm_unpackhi_epi8(a1, aZero), aWxHigh));
			else
			{
				__m128i b0, b1;
				Pairs(aRowB, s, &b0, &b1);
				aPixels = PackPixels(Sums4(_mm_unpacklo_epi8(a0, aZero), _mm_unpacklo_epi8(a1, aZero), _mm_unpacklo_epi8(b0, aZero),
						_mm_unpacklo_epi8(b1, aZero), aWxLow, aWy),
					Sums4(_mm_unpackhi_epi8(a0, aZero), _mm_unpackhi_epi8(a1, aZero), _mm_unpackhi_epi8(b0, aZero),
						_mm_unpackhi_epi8(b1, aZero), aWxHigh, aWy));
			}
			uint32_t aValues[4];
			_mm_storeu_si128((__m128i*) aValues, aPixels);
			aOut[s[0].mX - theX0] = aValues[0];
			aOut[s[1].mX - theX0] = aValues[1];
			aOut[s[2].mX - theX0] = aValues[2];
			aOut[s[3].mX - theX0] = aValues[3];
		}
#endif
		for (; k < aSeamCount; k++)
		{
			const Seam& s = aSeams[k];
			aOut[s.mX - theX0] = (wy == 0) ? Blend2(aLevel, aRowA[s.mA], aRowA[s.mB], (uint32_t) s.mWeight) :
				Blend4(aLevel, aRowA[s.mA], aRowA[s.mB], aRowB[s.mA], aRowB[s.mB], (uint32_t) s.mWeight, (uint32_t) wy);
		}
	}
}

void LinearStretch::DrawReference(const uint32_t* thePixels, int thePitch, int theX0, int theY0, int theX1, int theY1,
	uint32_t* theOut, int theOutPitch) const
{
	theX0 = std::max(0, theX0);
	theY0 = std::max(0, theY0);
	theX1 = std::min((int) mCols.size(), theX1);
	theY1 = std::min((int) mRows.size(), theY1);
	const uint8_t* aLevel = LevelTable();
	for (int y = theY0; y < theY1; y++)
	{
		const Tap& r = mRows[y];
		const uint32_t* aRowA = (const uint32_t*) ((const uint8_t*) thePixels + (size_t) r.mA * thePitch);
		const uint32_t* aRowB = (const uint32_t*) ((const uint8_t*) thePixels + (size_t) r.mB * thePitch);
		uint32_t* aOut = theOut + (size_t) (y - theY0) * theOutPitch;
		for (int x = theX0; x < theX1; x++)
		{
			const Tap& c = mCols[x];
			uint32_t aValue;
			if (r.mWeight == 0)
				aValue = (c.mWeight == 0) ? (aRowA[c.mA] & 0xFFFFFF) : Blend2(aLevel, aRowA[c.mA], aRowA[c.mB], (uint32_t) c.mWeight);
			else if (c.mWeight == 0)
				aValue = Blend2(aLevel, aRowA[c.mA], aRowB[c.mA], (uint32_t) r.mWeight);
			else
				aValue = Blend4(aLevel, aRowA[c.mA], aRowA[c.mB], aRowB[c.mA], aRowB[c.mB], (uint32_t) c.mWeight, (uint32_t) r.mWeight);
			aOut[x - theX0] = aValue;
		}
	}
}

int LinearStretch::CheckLevels()
{
	int aMisses = 0;
#ifdef LINEARSTRETCH_SSE2
	const uint8_t* aLevel = LevelTable();
	for (uint32_t s = 0; s <= 255 * 256; s += 8)
	{
		uint16_t aSums[8];
		uint16_t aLevels[8];
		for (int i = 0; i < 8; i++)
			aSums[i] = (uint16_t) std::min<uint32_t>(s + i, 255 * 256);
		_mm_storeu_si128((__m128i*) aLevels, Levels(_mm_loadu_si128((const __m128i*) aSums)));
		for (int i = 0; i < 8; i++)
			if (aLevels[i] != aLevel[aSums[i]])
				aMisses++;
	}
#endif
	return aMisses;
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
