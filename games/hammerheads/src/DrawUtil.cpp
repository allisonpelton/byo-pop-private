#include "DrawUtil.h"

#include <cmath>

#include "Color.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"

using namespace Sexy;

// 0x405000
void Sexy::DrawImageCelF(Graphics* g, Image* theImageStrip, double theX, double theY, int theCelCol, int theCelRow)
{
	if (theCelRow < 0 || theCelCol < 0 || theCelRow >= theImageStrip->mNumRows || theCelCol >= theImageStrip->mNumCols)
		return;

	int aCelWidth = theImageStrip->mWidth / theImageStrip->mNumCols;
	int aCelHeight = theImageStrip->mHeight / theImageStrip->mNumRows;
	Rect aSrcRect(aCelWidth * theCelCol, aCelHeight * theCelRow, aCelWidth, aCelHeight);
	g->DrawImageF(theImageStrip, (float)theX, (float)theY, aSrcRect);
}

// 0x4048d0
void Sexy::DrawImageLit(Graphics* g, Image* theImage, int theX, int theY, bool theHover)
{
	g->DrawImage(theImage, theX, theY);
	if (theHover)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(50, 50, 50));
		g->DrawImage(theImage, theX, theY);
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x405080
void Sexy::DrawCelRotated(Graphics* g, Image* theImageStrip, int theX, int theY, int theCelCol, int theCelRow, double theRotation)
{
	if (theCelRow < 0 || theCelCol < 0 || theCelRow >= theImageStrip->mNumRows || theCelCol >= theImageStrip->mNumCols)
		return;

	int aCelWidth = theImageStrip->mWidth / theImageStrip->mNumCols;
	int aCelHeight = theImageStrip->mHeight / theImageStrip->mNumRows;
	Rect aSrcRect(aCelWidth * theCelCol, aCelHeight * theCelRow, aCelWidth, aCelHeight);
	g->DrawImageRotated(theImageStrip, theX, theY, theRotation, &aSrcRect);
}

// 0x405100: a hovered cel is drawn once more over itself, added and lit by a grey of 50
void Sexy::DrawCelRotatedLit(Graphics* g, Image* theImageStrip, int theX, int theY, int theCelCol, int theCelRow, bool theHover,
	double theRotation)
{
	DrawCelRotated(g, theImageStrip, theX, theY, theCelCol, theCelRow, theRotation);
	if (theHover)
	{
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		g->SetColor(Color(50, 50, 50));
		DrawCelRotated(g, theImageStrip, theX, theY, theCelCol, theCelRow, theRotation);
		g->SetColorizeImages(false);
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
}

// 0x4052a0: a bar made of two pictures, an empty one drawn in full and a full one drawn over it as far as the fill reaches. The
// full picture has theMargin pixels at each end that the fill does not count; the last theFadeWidth columns of the filled
// part fade out one by one.
void Sexy::DrawFillBar(Graphics* g, Image* theEmptyImage, Image* theFullImage, int theX, int theY, double theFill,
	int theMargin, int theFadeWidth)
{
	g->DrawImage(theEmptyImage, theX, theY);
	if (!(theFill < 1.0))
	{
		g->DrawImage(theFullImage, theX, theY);
		return;
	}
	if (!(0.0 < theFill))
		return;

	int aFilledWidth = (int)((double)(theFullImage->mWidth - theMargin * 2) * theFill);
	int aColumn = aFilledWidth + theMargin;
	g->DrawImage(theFullImage, theX, theY, Rect(0, 0, aColumn, theFullImage->mHeight));
	if (theFadeWidth > 0)
	{
		g->SetColorizeImages(true);
		for (int i = 0; i < theFadeWidth; i++, aColumn++)
		{
			if (aColumn < theFullImage->mWidth)
			{
				g->SetColor(Color(255, 255, 255, (int)(cos(((double)i * 1.5707963267948966) / (double)theFadeWidth) * 255.0)));
				g->DrawImage(theFullImage, i + aFilledWidth + theX + theMargin, theY, Rect(aColumn, 0, 1, theFullImage->mHeight));
			}
		}
		g->SetColorizeImages(false);
	}
}

// 0x4051b0
bool Sexy::MaskHitsImage(const double* theImagePos, const int* theXY, const std::vector<Point>& theMask, Image* theImage,
	int theCelRow, int theCelCol)
{
	int aCelWidth = theImage->GetCelWidth();
	int aCelHeight = theImage->GetCelHeight();
	for (size_t i = 0; i < theMask.size(); i++)
	{
		int aX = (theMask[i].mX - ToInt(theImagePos[0])) + theXY[0];
		int aY = (theMask[i].mY - ToInt(theImagePos[1])) + theXY[1];
		if (aX > -1 && aX < aCelWidth && aY > -1 && aY < aCelHeight)
		{
			const uint32_t* aBits = static_cast<MemoryImage*>(theImage)->GetBits();
			if ((aBits[theImage->mWidth * (theCelRow * aCelHeight + aY) + theCelCol * aCelWidth + aX] & 0xff000000) != 0)
				return true;
		}
	}
	return false;
}
