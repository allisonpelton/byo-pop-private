#include "PixelFx.h"

#include <cmath>

namespace engine
{

namespace
{

// 0x56685c (feedingfrenzy2): a byte's share of 1, a float
const float BYTE_TO_UNIT = 0.003921569f;
// 0x56691c (feedingfrenzy2): a degree's share of a sextant, a float
const float DEGREES_TO_SEXTANTS = 0.016666668f;
const double FULL_TURN = 360.0;     // 0x566920 (feedingfrenzy2)
const double SEXTANT_DEGREES = 60.0; // 0x566924 (feedingfrenzy2)
const double BYTE_MAX = 255.0;      // 0x566848 (feedingfrenzy2)

// The x87's value truncated to a whole number (_ftol2, or fistp under a truncating control word), its low byte kept
uint8_t truncatedByte(double theValue)
{
	return (uint8_t)(int)theValue;
}

// 0x46d360 and 0x428090 (feedingfrenzy2): the least and the most of two floats, the first kept on a tie
float leastOf(float theFirst, float theSecond)
{
	return theFirst <= theSecond ? theFirst : theSecond;
}

float mostOf(float theFirst, float theSecond)
{
	return theFirst < theSecond ? theSecond : theFirst;
}

// The hue brought into 0 to 360 by whole turns, at the x87's precision
double wrapHue(double theHue)
{
	while (FULL_TURN < theHue)
		theHue = theHue - FULL_TURN;
	while (theHue < 0.0)
		theHue = theHue + FULL_TURN;
	return theHue;
}

// 0x43f520 (feedingfrenzy2), then _ftol2: the hue's sextant, and the hue in sextants kept as a float
int sextantOf(double theHue, float& theSextants)
{
	theSextants = (float)(theHue * (double)DEGREES_TO_SEXTANTS);
	return (int)(float)floor((double)theSextants);
}

} // namespace

void ColorFx::shadePixel(int theColumn, int theRow, PixelColor& thePixel) const
{
	if (mMode == COLOR_FX_BLEND)
	{
		double aKeep = 1.0 - (double)mColor.a;
		double anAlpha = (double)thePixel.mAlpha;
		thePixel.mRed = truncatedByte(thePixel.mRed * aKeep + (double)mColor.r * anAlpha * (double)mColor.a);
		thePixel.mGreen = truncatedByte(thePixel.mGreen * aKeep + (double)mColor.g * anAlpha * (double)mColor.a);
		thePixel.mBlue = truncatedByte((double)mColor.b * anAlpha * (double)mColor.a + thePixel.mBlue * aKeep);
		return;
	}
	if (mMode == COLOR_FX_ADD)
	{
		// 0x509364 (feedingfrenzy2): 0x502b5d holds each sum to 0 to 255
		const float aColor[4] = {mColor.r, mColor.g, mColor.b, mColor.a};
		uint8_t* aChannels[4] = {&thePixel.mRed, &thePixel.mGreen, &thePixel.mBlue, &thePixel.mAlpha};
		for (int i = 0; i < 4; i++)
		{
			int aSum = (int)((double)aColor[i] * BYTE_MAX + (double)*aChannels[i]);
			if (aSum < 0)
				aSum = 0;
			else if (aSum > 255)
				aSum = 255;
			*aChannels[i] = (uint8_t)aSum;
		}
		return;
	}
	if (mMode == COLOR_FX_MULTIPLY)
	{
		thePixel.mRed = truncatedByte((double)thePixel.mRed * (double)mColor.r);
		thePixel.mGreen = truncatedByte((double)thePixel.mGreen * (double)mColor.g);
		thePixel.mBlue = truncatedByte((double)thePixel.mBlue * (double)mColor.b);
		thePixel.mAlpha = truncatedByte((double)thePixel.mAlpha * (double)mColor.a);
	}
}

void HSVShiftFx::shadePixel(int theColumn, int theRow, PixelColor& thePixel) const
{
	if (thePixel.mAlpha == 0)
		return;
	// The alpha's share and its bytes kept as floats; the colours divided by the bytes the x87 holds
	double anAlphaShare = (double)thePixel.mAlpha * (double)BYTE_TO_UNIT;
	float anAlpha = (float)anAlphaShare;
	float anAlphaBytes = (float)(anAlphaShare * BYTE_MAX);
	double anInverse = 1.0 / (anAlphaShare * BYTE_MAX);
	float aRed = (float)((double)thePixel.mRed * anInverse);
	float aGreen = (float)((double)thePixel.mGreen * anInverse);
	float aBlue = (float)((double)thePixel.mBlue * anInverse);
	float aLeast = leastOf(leastOf(aRed, aGreen), aBlue);
	double aMost = (double)mostOf(mostOf(aRed, aGreen), aBlue);
	double aSpread = aMost - (double)aLeast;

	double aHue;
	double aSaturation;
	if (aMost == 0.0)
	{
		aHue = -1.0;
		aSaturation = 0.0;
	}
	else
	{
		aSaturation = aSpread / aMost;
		aHue = 0.0;
		if (aSpread != 0.0)
		{
			if ((double)aRed == aMost)
				aHue = ((double)aGreen - (double)aBlue) / aSpread;
			else if ((double)aGreen == aMost)
				aHue = ((double)aBlue - (double)aRed) / aSpread + 2.0;
			else
				aHue = ((double)aRed - (double)aGreen) / aSpread + 4.0;
			aHue = aHue * SEXTANT_DEGREES;
			if (aHue < 0.0)
				aHue = aHue + FULL_TURN;
		}
	}

	double aNewSaturation = (double)mSaturation + aSaturation;
	float aSaturationKept = (float)aNewSaturation;
	double aNewValue = (double)mValue + aMost;
	aHue = wrapHue((double)mHue + aHue);
	if (aNewSaturation < 0.0)
	{
		aSaturationKept = 0.0f;
		aNewSaturation = 0.0;
	}
	else if (1.0 < aNewSaturation)
	{
		aSaturationKept = 1.0f;
		aNewSaturation = 1.0;
	}
	if (aNewValue < 0.0)
		aNewValue = 0.0;
	else if (1.0 < aNewValue)
		aNewValue = 1.0;
	float aValueKept = (float)aNewValue;

	if (aNewSaturation == 0.0)
	{
		uint8_t aGrey = truncatedByte(aNewValue * (double)anAlpha * BYTE_MAX);
		thePixel.mRed = aGrey;
		thePixel.mGreen = aGrey;
		thePixel.mBlue = aGrey;
		return;
	}
	float aSextants;
	int aSextant = sextantOf(aHue, aSextants);
	double aFraction = (double)aSextants - (double)(float)aSextant;
	double aLeastValue = (1.0 - (double)aSaturationKept) * (double)aValueKept;
	double aFalling = (1.0 - aFraction * (double)aSaturationKept) * (double)aValueKept;
	double aRising = (1.0 - (1.0 - aFraction) * (double)aSaturationKept) * (double)aValueKept;
	double aNewRed = (double)aValueKept;
	double aNewGreen = aLeastValue;
	double aNewBlue = aFalling;
	switch (aSextant)
	{
	case 1:
		aNewRed = aFalling;
		aNewGreen = (double)aValueKept;
		aNewBlue = aLeastValue;
		break;
	case 2:
		aNewRed = aLeastValue;
		aNewGreen = (double)aValueKept;
		aNewBlue = aRising;
		break;
	case 3:
		aNewRed = aLeastValue;
		aNewGreen = aFalling;
		aNewBlue = (double)aValueKept;
		break;
	case 4:
		aNewRed = aRising;
		aNewGreen = aLeastValue;
		aNewBlue = (double)aValueKept;
		break;
	}
	thePixel.mRed = truncatedByte((double)anAlphaBytes * aNewRed);
	thePixel.mGreen = truncatedByte(aNewGreen * (double)anAlphaBytes);
	thePixel.mBlue = truncatedByte(aNewBlue * (double)anAlphaBytes);
}

void RainbowShiftFx::shadePixel(int theColumn, int theRow, PixelColor& thePixel) const
{
	float aRed = (float)((double)thePixel.mRed * (double)BYTE_TO_UNIT);
	float aGreen = (float)((double)thePixel.mGreen * (double)BYTE_TO_UNIT);
	float aBlue = (float)((double)thePixel.mBlue * (double)BYTE_TO_UNIT);
	float aLeast = leastOf(leastOf(aRed, aGreen), aBlue);
	float aMost = mostOf(mostOf(aRed, aGreen), aBlue);
	double aSaturation = 0.0;
	if ((double)aMost != 0.0)
		aSaturation = ((double)aMost - (double)aLeast) / (double)aMost;
	float aSaturationKept = (float)aSaturation;
	double aHue = wrapHue((double)theColumn * (double)mHuePerColumn + (double)mHue);
	if (aSaturation == 0.0)
	{
		uint8_t aGrey = truncatedByte((double)aMost * BYTE_MAX);
		thePixel.mRed = aGrey;
		thePixel.mGreen = aGrey;
		thePixel.mBlue = aGrey;
		return;
	}
	float aSextants;
	int aSextant = sextantOf(aHue, aSextants);
	double aFraction = (double)aSextants - (double)(float)aSextant;
	double aLeastValue = (1.0 - (double)aSaturationKept) * (double)aMost;
	double aFalling = (1.0 - aFraction * (double)aSaturationKept) * (double)aMost;
	double aRising = (1.0 - (1.0 - aFraction) * (double)aSaturationKept) * (double)aMost;
	// Sextant 5, and any other
	double aNewRed = (double)aMost;
	double aNewGreen = aLeastValue;
	double aNewBlue = aFalling;
	switch (aSextant)
	{
	case 0:
		aNewGreen = aRising;
		aNewBlue = aLeastValue;
		break;
	case 1:
		aNewRed = aFalling;
		aNewGreen = (double)aMost;
		aNewBlue = aLeastValue;
		break;
	case 2:
		aNewRed = aLeastValue;
		aNewGreen = (double)aMost;
		aNewBlue = aRising;
		break;
	case 3:
		aNewRed = aLeastValue;
		aNewGreen = aFalling;
		aNewBlue = (double)aMost;
		break;
	case 4:
		aNewRed = aRising;
		aNewGreen = aLeastValue;
		aNewBlue = (double)aMost;
		break;
	}
	thePixel.mRed = truncatedByte(aNewRed * BYTE_MAX);
	thePixel.mGreen = truncatedByte(aNewGreen * BYTE_MAX);
	thePixel.mBlue = truncatedByte(aNewBlue * BYTE_MAX);
}

} // namespace engine
