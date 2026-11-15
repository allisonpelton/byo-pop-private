#include "ImageFilter.h"

#include <cmath>

using namespace engine;

namespace
{
const float BYTE_TO_UNIT = 0.003921569f;  // 1 / 255
const float UNIT_TO_BYTE = 255.0f;        // 255, the constant at 0x503118 (pizzafrenzy)
const float HUE_SECTOR = 60.0f;
const float HUE_CIRCLE = 360.0f;

// 0x47b130 (pizzafrenzy): a colour's hue (in degrees, -1 for black), saturation and value
void rgbToHsv(uint8_t theRed, uint8_t theGreen, uint8_t theBlue, float* theHue, float* theSaturation, float* theValue)
{
	float aRed = (float)theRed * BYTE_TO_UNIT;
	float aGreen = (float)theGreen * BYTE_TO_UNIT;
	float aBlue = (float)theBlue * BYTE_TO_UNIT;
	float aLowest = aRed;
	if (aGreen < aRed)
		aLowest = aGreen;
	if (aBlue < aLowest)
		aLowest = aBlue;
	float aHighest = aRed;
	if (aRed < aGreen)
		aHighest = aGreen;
	if (aHighest < aBlue)
		aHighest = aBlue;
	*theValue = aHighest;
	float aRange = aHighest - aLowest;
	if (aHighest == 0.0f)
	{
		*theSaturation = 0.0f;
		*theHue = -1.0f;
		return;
	}
	*theSaturation = aRange / aHighest;
	if (aRange == 0.0f)
	{
		*theHue = 0.0f;
		return;
	}
	float aHue;
	if (aRed == aHighest)
		aHue = (aGreen - aBlue) / aRange;
	else if (aGreen == aHighest)
		aHue = (aBlue - aRed) / aRange + 2.0f;
	else
		aHue = (aRed - aGreen) / aRange + 4.0f;
	*theHue = aHue * HUE_SECTOR;
	if (*theHue < 0.0f)
		*theHue = *theHue + HUE_CIRCLE;
}

// 0x47b2d0 (pizzafrenzy)
void hsvToRgb(float theHue, float theSaturation, float theValue, uint8_t* theRed, uint8_t* theGreen, uint8_t* theBlue)
{
	if (theSaturation == 0.0f)
	{
		uint8_t aGrey = (uint8_t)(int)(theValue * UNIT_TO_BYTE);
		*theRed = aGrey;
		*theGreen = aGrey;
		*theBlue = aGrey;
		return;
	}
	float aSector = theHue * (1.0f / HUE_SECTOR);
	int aSectorIndex = (int)std::floor((double)aSector);
	float aFraction = aSector - (float)aSectorIndex;
	float aP = (1.0f - theSaturation) * theValue;
	float aQ = (1.0f - theSaturation * aFraction) * theValue;
	float aT = (1.0f - (1.0f - aFraction) * theSaturation) * theValue;
	float aRed, aGreen, aBlue;
	switch (aSectorIndex)
	{
	case 0: aRed = theValue; aGreen = aT; aBlue = aP; break;
	case 1: aRed = aQ; aGreen = theValue; aBlue = aP; break;
	case 2: aRed = aP; aGreen = theValue; aBlue = aT; break;
	case 3: aRed = aP; aGreen = aQ; aBlue = theValue; break;
	case 4: aRed = aT; aGreen = aP; aBlue = theValue; break;
	default: aRed = theValue; aGreen = aP; aBlue = aQ; break;
	}
	*theRed = (uint8_t)(int)(aRed * UNIT_TO_BYTE);
	*theGreen = (uint8_t)(int)(aGreen * UNIT_TO_BYTE);
	*theBlue = (uint8_t)(int)(aBlue * UNIT_TO_BYTE);
}
}

// 0x47b110 (pizzafrenzy)
void HsvShiftFilter::setShift(float theHue, float theSaturation, float theValue)
{
	mHue = theHue;
	mSaturation = theSaturation;
	mValue = theValue;
}

// 0x47b550 (pizzafrenzy)
void HsvShiftFilter::filter(uint8_t theRed, uint8_t theGreen, uint8_t theBlue, uint8_t theAlpha, uint8_t* theNewRed,
                            uint8_t* theNewGreen, uint8_t* theNewBlue, uint8_t* theNewAlpha) const
{
	float aHue, aSaturation, aValue;
	rgbToHsv(theRed, theGreen, theBlue, &aHue, &aSaturation, &aValue);
	aSaturation = aSaturation + mSaturation;
	aValue = aValue + mValue;
	for (aHue = aHue + mHue; HUE_CIRCLE < aHue; aHue = aHue - HUE_CIRCLE)
	{
	}
	for (; aHue < 0.0f; aHue = aHue + HUE_CIRCLE)
	{
	}
	if (aSaturation < 0.0f)
		aSaturation = 0.0f;
	else if (1.0f < aSaturation)
		aSaturation = 1.0f;
	if (aValue < 0.0f)
		aValue = 0.0f;
	else if (1.0f < aValue)
		aValue = 1.0f;
	hsvToRgb(aHue, aSaturation, aValue, theNewRed, theNewGreen, theNewBlue);
	*theNewAlpha = theAlpha;
}
