// port(deviation): see DeluxeCover.h
#include "DeluxeCover.h"

#include <cmath>

#include "MemoryImage.h"

using namespace Sexy;

namespace
{
	// the word runs down to the right: its centre line passes (theCentreX, theCentreY) and falls theSlope pixels for each one across
	struct Band
	{
		double mCentreX;
		double mCentreY;
		double mSlope;
		double mAbove;		// how far above the line the word's top reaches
		double mBelow;		// how far below it the word and its glow go
		int mLeft;
		int mRight;
	};

	bool InBand(const Band& theBand, int theX, int theY)
	{
		if (theX < theBand.mLeft || theX >= theBand.mRight)
			return false;
		double aOffset = theY - (theBand.mCentreY + theBand.mSlope * (theX - theBand.mCentreX));
		return aOffset >= -theBand.mAbove && aOffset <= theBand.mBelow;
	}
}

// the title's own glow and letters: the band's pixels become see-through
void Sexy::CoverDeluxeOnLoadingTitle(Image* theImage)
{
	MemoryImage* anImage = dynamic_cast<MemoryImage*>(theImage);
	if (anImage == nullptr || anImage->mWidth != 800 || anImage->mHeight != 480)
		return;
	const Band aBand = { 682.0, 332.0, 0.434, 22.0, 40.0, 660, 800 };
	ulong* aBits = anImage->GetBits();
	for (int aY = 0; aY < anImage->mHeight; aY++)
		for (int aX = 0; aX < anImage->mWidth; aX++)
			if (InBand(aBand, aX, aY))
				aBits[aY * anImage->mWidth + aX] = 0;
	anImage->BitsChanged();
}

// the menu's picture: below the line that follows the bottom of the "Heads" lettering, each row between the green just left of the
// word and the green just right of it is filled with the colours between the two
void Sexy::CoverDeluxeOnMainMenu(Image* theImage)
{
	MemoryImage* anImage = dynamic_cast<MemoryImage*>(theImage);
	if (anImage == nullptr || anImage->mWidth != 800 || anImage->mHeight != 600)
		return;
	const int aLeft = 672, aRight = 786;
	static const int aTop[][2] = { { 672, 184 }, { 690, 186 }, { 705, 185 }, { 712, 186 }, { 716, 194 }, { 725, 197 }, { 742, 204 }, { 772, 222 }, { 786, 230 } };
	const int aTopCount = sizeof(aTop) / sizeof(aTop[0]);
	ulong* aBits = anImage->GetBits();
	const int aWidth = anImage->mWidth;
	for (int aX = aLeft; aX < aRight; aX++)
	{
		double aTopY = 300.0;
		for (int i = 0; i + 1 < aTopCount; i++)
			if (aTop[i][0] <= aX && aX <= aTop[i + 1][0])
				aTopY = aTop[i][1] + (double)(aTop[i + 1][1] - aTop[i][1]) * (aX - aTop[i][0]) / (aTop[i + 1][0] - aTop[i][0]);
		int aFirstY = (int)floor(aTopY + 0.5);
		int aLastY = (int)floor(205.0 + 0.475 * (aX - 690) + 34.0 + 0.5);
		double aT = (double)(aX - aLeft + 1) / (double)(aRight - aLeft + 1);
		for (int aY = aFirstY; aY <= aLastY; aY++)
		{
			ulong aLeftPixel = aBits[aY * aWidth + aLeft - 1];
			ulong aRightPixel = aBits[aY * aWidth + aRight];
			ulong aPixel = 0xff000000;
			for (int aShift = 0; aShift <= 16; aShift += 8)
			{
				int aFrom = (int)((aLeftPixel >> aShift) & 0xff);
				int aTo = (int)((aRightPixel >> aShift) & 0xff);
				aPixel |= (ulong)(int)(aFrom + (aTo - aFrom) * aT) << aShift;
			}
			aBits[aY * aWidth + aX] = aPixel;
		}
	}
	anImage->BitsChanged();
}
