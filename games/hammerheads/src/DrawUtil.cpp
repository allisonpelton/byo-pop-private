#include "DrawUtil.h"

#include "Graphics.h"
#include "Image.h"

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
