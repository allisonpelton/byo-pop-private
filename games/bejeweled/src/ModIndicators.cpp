// port: the signs that the game runs with mods (ModIndicators.h).

#include "ModIndicators.h"

#include "MemoryImage.h"
#include "ImageFont.h"

#include <cmath>
#include <stdint.h>
#include <vector>

using namespace Sexy;

namespace
{

// The edge of the font's strokes by distance from the stroke, in 1x pixels: white fill, a soft light
// edge, then a dark outline that fades out. Sampled from overfont2's own G.
const int kEdgeGray[] = { 254, 215, 38, 85, 130 };
const int kEdgeAlpha[] = { 255, 255, 250, 161, 62 };
const int kEdgeSteps = 5;

// Draws the crossbar into the glyph at theX, theY (theWidth x theHeight pixels, theScale pixels per 1x
// pixel) of an ARGB buffer. The bar runs from inside the arc into the top of the G's spur; the
// proportions are measured on overfont2's 39x50 G.
void DrawCrossbar(uint32_t* theBits, int theStride, int theX, int theY, int theWidth, int theHeight, int theScale)
{
	int aLeft = (int) (theWidth * 18.0 / 39.0 + 0.5);
	int aRight = (int) (theWidth * 34.0 / 39.0 + 0.5) - 1;
	int aTop = (int) (theHeight * 21.0 / 50.0 + 0.5);
	int aBottom = (int) (theHeight * 26.0 / 50.0 + 0.5) - 1;

	for (int y = 0; y < theHeight; y++)
	{
		for (int x = 0; x < theWidth; x++)
		{
			int dx = std::max(std::max(aLeft - x, x - aRight), 0);
			int dy = std::max(std::max(aTop - y, y - aBottom), 0);
			int aStep = (int) (std::sqrt((double) (dx * dx + dy * dy)) / theScale + 0.5);
			if (aStep >= kEdgeSteps)
				continue;

			uint32_t& aPixel = theBits[(theY + y) * theStride + theX + x];
			int anAlpha = (int) (aPixel >> 24);
			int aGray = (int) ((aPixel >> 16) & 0xFF);
			if ((anAlpha >= 250) && (aGray >= 200))
				continue;  // the letter's own fill stays
			if ((aStep <= 1) || (kEdgeAlpha[aStep] > anAlpha))
			{
				uint32_t g = (uint32_t) kEdgeGray[aStep];
				aPixel = ((uint32_t) kEdgeAlpha[aStep] << 24) | (g << 16) | (g << 8) | g;
			}
		}
	}
}

}

void Sexy::AddModsCrossbarToG(ImageFont* theFont)
{
	FontData* aData = theFont->mFontData;
	if ((aData == NULL) || (aData->mFontLayerList.empty()))
		return;
	FontLayer& aLayer = aData->mFontLayerList.front();
	MemoryImage* anImage = (MemoryImage*) aLayer.mImage;
	const Rect& aRect = aLayer.mCharData[(unsigned char) 'G'].mImageRect;
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

	DrawCrossbar((uint32_t*) anImage->GetBits(), anImage->mWidth, aRect.mX, aRect.mY, aRect.mWidth, aRect.mHeight, 1);
	anImage->BitsChanged();

	if ((!aHDBits.empty()) && ((aRect.mX + aRect.mWidth) * aHDScale <= aHDWidth) && ((aRect.mY + aRect.mHeight) * aHDScale <= aHDHeight))
	{
		DrawCrossbar(&aHDBits[0], aHDWidth, aRect.mX * aHDScale, aRect.mY * aHDScale, aRect.mWidth * aHDScale, aRect.mHeight * aHDScale, aHDScale);
		anImage->SetHDBits(&aHDBits[0], aHDWidth, aHDHeight, aHDScale);
	}
}
