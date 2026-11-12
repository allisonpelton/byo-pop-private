// port(mod): mod "colour-blind", the tile symbols (TileSymbols.h). Not part of the original game.

#include "TileSymbols.h"
#include "Board.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Point.h"
#include <algorithm>
#include <cmath>
#include <stdint.h>
#include <vector>

using namespace Sexy;

// The symbols are images made once: drawn at SYMBOL_SCALE times their size with SYMBOL_SAMPLES^2 samples a
// pixel, kept as HD pixels (sharp at any render scale, see framework/MemoryImage.h) over 1x pixels averaged
// from them
static const int SYMBOL_SCALE = 4;
static const int SYMBOL_SAMPLES = 4;
static const int SMALL_SIZE = 20;
static const int LARGE_SIZE = 22;

// The shape of each type as points around (0, 0), in a box about 2 * radius across
static void ShapePoints(int theType, double theRadius, std::vector<FPoint>* thePoints)
{
	const double PI = 3.14159265358979;
	switch (theType)
	{
	case TILE_GREEN:        // a circle
		for (int i = 0; i < 32; i++)
			thePoints->push_back(FPoint(cos(i * PI / 16) * theRadius, sin(i * PI / 16) * theRadius));
		break;
	case TILE_GOLD:         // a square
	{
		double r = theRadius * 0.85;
		thePoints->push_back(FPoint(-r, -r));
		thePoints->push_back(FPoint(r, -r));
		thePoints->push_back(FPoint(r, r));
		thePoints->push_back(FPoint(-r, r));
		break;
	}
	case TILE_SAPPHIRE:     // a rhombus
		thePoints->push_back(FPoint(0, -theRadius * 1.1));
		thePoints->push_back(FPoint(theRadius * 0.8, 0));
		thePoints->push_back(FPoint(0, theRadius * 1.1));
		thePoints->push_back(FPoint(-theRadius * 0.8, 0));
		break;
	case TILE_DIAMOND:      // a five-pointed star
		for (int i = 0; i < 10; i++)
		{
			double r = (i % 2 == 0) ? theRadius * 1.15 : theRadius * 0.5;
			double a = -PI / 2 + i * PI / 5;
			thePoints->push_back(FPoint(cos(a) * r, sin(a) * r));
		}
		break;
	case TILE_FIRE:         // a triangle, point up
		thePoints->push_back(FPoint(0, -theRadius * 1.1));
		thePoints->push_back(FPoint(theRadius * 1.05, theRadius * 0.8));
		thePoints->push_back(FPoint(-theRadius * 1.05, theRadius * 0.8));
		break;
	}
}

// thePoints moved outward from the centre by theGrow (the outline), then placed at theCX, theCY
static std::vector<FPoint> Placed(const std::vector<FPoint>& thePoints, double theCX, double theCY, double theGrow)
{
	std::vector<FPoint> aPoints;
	for (size_t i = 0; i < thePoints.size(); i++)
	{
		double aLength = sqrt(thePoints[i].mX * thePoints[i].mX + thePoints[i].mY * thePoints[i].mY);
		double aScale = (aLength > 0) ? (aLength + theGrow) / aLength : 1.0;
		aPoints.push_back(FPoint(theCX + thePoints[i].mX * aScale, theCY + thePoints[i].mY * aScale));
	}
	return aPoints;
}

// Even-odd rule
static bool Inside(const std::vector<FPoint>& thePolygon, double theX, double theY)
{
	bool isInside = false;
	for (size_t i = 0, j = thePolygon.size() - 1; i < thePolygon.size(); j = i++)
	{
		const FPoint& a = thePolygon[i];
		const FPoint& b = thePolygon[j];
		if (((a.mY > theY) != (b.mY > theY)) && (theX < (b.mX - a.mX) * (theY - a.mY) / (b.mY - a.mY) + a.mX))
			isInside = !isInside;
	}
	return isInside;
}

static MemoryImage* MakeSymbol(SexyAppBase* theApp, int theType, bool isLarge)
{
	std::vector<FPoint> aShape;
	ShapePoints(theType, isLarge ? 6.5 : 5.5, &aShape);
	if (aShape.empty())
		return NULL;

	int aSize = isLarge ? LARGE_SIZE : SMALL_SIZE;
	double aCentre = aSize / 2.0;
	std::vector<FPoint> anOutline = Placed(aShape, aCentre, aCentre, 2.0);
	std::vector<FPoint> aFill = Placed(aShape, aCentre, aCentre, 0.0);

	// The HD pixels: the dark plate (large), the black outline and the white shape, each by its coverage
	int aHDSize = aSize * SYMBOL_SCALE;
	std::vector<uint32_t> aHDBits((size_t)aHDSize * aHDSize);
	for (int y = 0; y < aHDSize; y++)
	{
		for (int x = 0; x < aHDSize; x++)
		{
			int anOutlineCount = 0;
			int aFillCount = 0;
			for (int sy = 0; sy < SYMBOL_SAMPLES; sy++)
			{
				for (int sx = 0; sx < SYMBOL_SAMPLES; sx++)
				{
					double aX = (x + (sx + 0.5) / SYMBOL_SAMPLES) / SYMBOL_SCALE;
					double aY = (y + (sy + 0.5) / SYMBOL_SAMPLES) / SYMBOL_SCALE;
					if (Inside(anOutline, aX, aY))
						anOutlineCount++;
					if (Inside(aFill, aX, aY))
						aFillCount++;
				}
			}
			double aSamples = SYMBOL_SAMPLES * SYMBOL_SAMPLES;
			double aPlate = (isLarge && (x >= SYMBOL_SCALE) && (y >= SYMBOL_SCALE) && (x < aHDSize - SYMBOL_SCALE) &&
				(y < aHDSize - SYMBOL_SCALE)) ? 140.0 / 255.0 : 0.0;
			double anInk = anOutlineCount / aSamples;   // outline or shape: the outline's area holds the shape
			double aWhite = aFillCount / aSamples;
			// the ink (white over black) over the plate (black)
			double anAlpha = anInk + aPlate * (1.0 - anInk);
			double aGrey = (anAlpha > 0) ? (aWhite * 255.0) / anAlpha : 0.0;
			int g = (int)floor(std::min(255.0, aGrey) + 0.5);
			int a = (int)floor(anAlpha * 255.0 + 0.5);
			aHDBits[(size_t)y * aHDSize + x] = ((uint32_t)a << 24) | ((uint32_t)g << 16) | ((uint32_t)g << 8) | (uint32_t)g;
		}
	}

	// The 1x pixels: each the average of its HD block (colour weighted by alpha)
	MemoryImage* anImage = new MemoryImage(theApp);
	anImage->Create(aSize, aSize);
	ulong* aBits = anImage->GetBits();
	for (int y = 0; y < aSize; y++)
	{
		for (int x = 0; x < aSize; x++)
		{
			double anAlphaSum = 0;
			double aGreySum = 0;
			for (int dy = 0; dy < SYMBOL_SCALE; dy++)
			{
				for (int dx = 0; dx < SYMBOL_SCALE; dx++)
				{
					uint32_t aPixel = aHDBits[(size_t)(y * SYMBOL_SCALE + dy) * aHDSize + x * SYMBOL_SCALE + dx];
					double anAlpha = (aPixel >> 24) / 255.0;
					anAlphaSum += anAlpha;
					aGreySum += anAlpha * (aPixel & 0xff);
				}
			}
			int a = (int)floor(anAlphaSum * 255.0 / (SYMBOL_SCALE * SYMBOL_SCALE) + 0.5);
			int g = (anAlphaSum > 0) ? (int)floor(aGreySum / anAlphaSum + 0.5) : 0;
			aBits[y * aSize + x] = ((ulong)a << 24) | ((ulong)g << 16) | ((ulong)g << 8) | (ulong)g;
		}
	}
	anImage->BitsChanged();
	anImage->SetHDBits(&aHDBits[0], aHDSize, aHDSize, SYMBOL_SCALE);
	return anImage;
}

void Sexy::DrawTileSymbol(Graphics* g, SexyAppBase* theApp, int theX, int theY, int theType, bool isLarge)
{
	static MemoryImage* gSymbols[NUM_TILE_TYPES][2] = {};
	static bool gMade[NUM_TILE_TYPES][2] = {};
	if ((theType < 0) || (theType >= NUM_TILE_TYPES))
		return;
	int aSizeIdx = isLarge ? 1 : 0;
	if (!gMade[theType][aSizeIdx])
	{
		gSymbols[theType][aSizeIdx] = MakeSymbol(theApp, theType, isLarge);
		gMade[theType][aSizeIdx] = true;
	}
	MemoryImage* anImage = gSymbols[theType][aSizeIdx];
	if (anImage != NULL)
		g->DrawImage(anImage, theX + 1, theY + 1);
}
