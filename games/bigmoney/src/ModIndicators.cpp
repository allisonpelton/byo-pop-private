#include "ModIndicators.h"

#include "MemoryImage.h"
#include "ImageFont.h"

#include <stdint.h>
#include <vector>

using namespace Sexy;

namespace
{

// The stubs of the "$" are the rows at the top and bottom of the glyph whose opaque run is narrow (the
// vertical stroke sticking out of the S). Each gets a gap down its middle in the glyph's own darkest
// colour, the colour of its outline. theScale is the pixels per 1x pixel of theBits; the rows and the
// gap are found on the 1x alpha so 1x and HD get the same shape.
void SplitStubs(const uint32_t* theAlpha1x, int theStride1x, const Rect& theRect, uint32_t* theBits, int theStride, int theScale)
{
	const int kStubMaxWidth = 8;
	const int kEndRows = 7;

	// The darkest opaque pixel of the glyph (in theBits' resolution) is the outline colour
	uint32_t aDark = 0xFF000000;
	int aDarkSum = 3 * 256;
	for (int y = 0; y < theRect.mHeight * theScale; y++)
	{
		for (int x = 0; x < theRect.mWidth * theScale; x++)
		{
			uint32_t aPixel = theBits[(theRect.mY * theScale + y) * theStride + theRect.mX * theScale + x];
			if ((aPixel >> 24) < 250)
				continue;
			int aSum = (int) ((aPixel >> 16) & 0xFF) + (int) ((aPixel >> 8) & 0xFF) + (int) (aPixel & 0xFF);
			if (aSum < aDarkSum)
			{
				aDarkSum = aSum;
				aDark = aPixel;
			}
		}
	}

	for (int y = 0; y < theRect.mHeight; y++)
	{
		if ((y >= kEndRows) && (y < theRect.mHeight - kEndRows))
			continue;
		int aFirst = -1;
		int aLast = -1;
		for (int x = 0; x < theRect.mWidth; x++)
		{
			uint32_t aPixel = theAlpha1x[(theRect.mY + y) * theStride1x + theRect.mX + x];
			if ((aPixel >> 24) > 128)
			{
				if (aFirst < 0)
					aFirst = x;
				aLast = x;
			}
		}
		int aRunWidth = aLast - aFirst + 1;
		if ((aFirst < 0) || (aRunWidth < 4) || (aRunWidth > kStubMaxWidth))
			continue;
		// a gap one 1x pixel wide (two for a wide stub) in the middle of the stub
		int aGapLeft = aFirst + (aRunWidth - 1) / 2;
		int aGapRight = (aRunWidth >= 6) ? aGapLeft + 1 : aGapLeft;
		for (int sy = 0; sy < theScale; sy++)
		{
			for (int sx = aGapLeft * theScale; sx < (aGapRight + 1) * theScale; sx++)
			{
				uint32_t& aPixel = theBits[((theRect.mY + y) * theScale + sy) * theStride + theRect.mX * theScale + sx];
				if ((aPixel >> 24) > 0)
					aPixel = aDark;
			}
		}
	}
}

}

void Sexy::AddModsDoubleStrokeToDollar(ImageFont* theFont)
{
	FontData* aData = theFont->mFontData;
	if (aData == NULL)
		return;
	// The layer drawn on top (scorefont.txt: "Top"; the "Blur" layer under it stays as it is). Layer
	// names are upper-cased by the descriptor parser.
	FontLayerMap::iterator anItr = aData->mFontLayerMap.find("TOP");
	if ((anItr != aData->mFontLayerMap.end()) && (anItr->second != NULL))
	{
		FontLayer& aLayer = *anItr->second;
		MemoryImage* anImage = (MemoryImage*) aLayer.mImage;
		const Rect& aRect = aLayer.mCharData[(unsigned char) '$'].mImageRect;
		if ((anImage == NULL) || (aRect.mWidth <= 0) || (aRect.mHeight <= 0) ||
			(aRect.mX + aRect.mWidth > anImage->mWidth) || (aRect.mY + aRect.mHeight > anImage->mHeight))
			return;

		// BitsChanged() makes the HD pixels of a mod's HD font stale, so patch a copy of them and put it back
		std::vector<uint32_t> aHDBits;
		int aHDWidth = 0;
		int aHDHeight = 0;
		int aHDScale = 1;
		if (anImage->HasValidHDBits())
		{
			aHDBits = anImage->mHDBits;
			aHDWidth = anImage->mHDWidth;
			aHDHeight = anImage->mHDHeight;
			aHDScale = anImage->mHDScale;
		}

		uint32_t* aBits = (uint32_t*) anImage->GetBits();
		std::vector<uint32_t> anAlpha1x(aBits, aBits + anImage->mWidth * anImage->mHeight);
		if ((!aHDBits.empty()) && ((aRect.mX + aRect.mWidth) * aHDScale <= aHDWidth) && ((aRect.mY + aRect.mHeight) * aHDScale <= aHDHeight))
			SplitStubs(&anAlpha1x[0], anImage->mWidth, aRect, &aHDBits[0], aHDWidth, aHDScale);
		else
			aHDBits.clear();
		SplitStubs(&anAlpha1x[0], anImage->mWidth, aRect, aBits, anImage->mWidth, 1);
		anImage->BitsChanged();
		if (!aHDBits.empty())
			anImage->SetHDBits(&aHDBits[0], aHDWidth, aHDHeight, aHDScale);
	}
}
