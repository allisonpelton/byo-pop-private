#include "Sprite.h"
#include "HeavyWeaponApp.h"

#include "DDImage.h"
#include "Graphics.h"

#include <cstdlib>

using namespace Sexy;

// 0x42d4e0
Sprite::Sprite(HeavyWeaponApp* theApp)
{
	mApp = theApp;
	mImage = NULL;
	mAlphaMask = NULL;
	// port: the original leaves the cel sizes unset until Load (a sprite whose image failed to load is never drawn)
	mCelWidth = 0;
	mCelHeight = 0;
	mColumns = 0;
	mRows = 0;
}

// 0x42d670
Sprite::~Sprite()
{
	delete mImage;
	free(mAlphaMask);
}

// 0x42d500: a new image replaces the old one (the level's backgrounds are loaded again for every level)
bool Sprite::Load(const std::string& theFileName, int theColumns, int theRows)
{
	if (mImage != NULL)
	{
		delete mImage;
		// port: the original frees the mask and keeps the pointer, so a reloaded sprite's next GetAlphaMask would return
		// freed memory; no reloaded image (the level's backgrounds) ever has a mask made, so clearing it changes nothing
		free(mAlphaMask);
		mAlphaMask = NULL;
	}
	mImage = mApp->GetImage(theFileName);
	if (mImage == NULL)
		return false;

	mCelWidth = mImage->GetWidth() / theColumns;
	mColumns = theColumns;
	mRows = theRows;
	mCelHeight = mImage->GetHeight() / theRows;
	return true;
}

// 0x42d580: the image's alpha, one byte a pixel, made the first time it is asked for
unsigned char* Sprite::GetAlphaMask()
{
	if (mAlphaMask == NULL)
	{
		ulong* aBits = mImage->GetBits();
		int aSize = mImage->GetHeight() * mImage->GetWidth();
		mAlphaMask = (unsigned char*) malloc(aSize);
		for (int i = 0; i < aSize; i++)
			mAlphaMask[i] = (unsigned char) (aBits[i] >> 24);
	}
	return mAlphaMask;
}

// 0x42d6b0: the cel at theColumn, theRow, its top left at x, y or its centre there
void Sprite::Draw(Graphics* g, int x, int y, bool isCentered, int theColumn, int theRow, bool isMirrored)
{
	Rect aSrcRect(mCelWidth * theColumn, mCelHeight * theRow, mCelWidth, mCelHeight);
	if (isCentered)
		g->DrawImageMirror(mImage, x - mCelWidth / 2, y - mCelHeight / 2, aSrcRect, isMirrored);
	else
		g->DrawImageMirror(mImage, x, y, aSrcRect, isMirrored);
}

// 0x42d5e0
Rect Sexy::SpriteIntersection(const Rect& theRect, const Rect& theOther)
{
	int aLeft = (theRect.mX > theOther.mX) ? theRect.mX : theOther.mX;
	int aRight = (theOther.mX + theOther.mWidth < theRect.mX + theRect.mWidth) ? theOther.mX + theOther.mWidth :
		theRect.mX + theRect.mWidth;
	int aTop = (theRect.mY > theOther.mY) ? theRect.mY : theOther.mY;
	int aBottom = (theOther.mY + theOther.mHeight < theRect.mY + theRect.mHeight) ? theOther.mY + theOther.mHeight :
		theRect.mY + theRect.mHeight;
	if ((aRight - aLeft >= 0) && (aBottom - aTop >= 0))
		return Rect(aLeft, aTop, aRight - aLeft, aBottom - aTop);
	return Rect(0, 0, 0, 0);
}

// 0x42d750: whether two cels overlap where both have a pixel of nonzero alpha. A mirrored sprite is read mirrored
// across its whole image, not its cel (every sprite the game mirrors has one column).
bool Sprite::Collides(int x, int y, int theColumn, int theRow, bool isMirrored, Sprite* theOther, int theOtherX,
	int theOtherY, int theOtherColumn, int theOtherRow, bool isOtherMirrored, bool isCentered)
{
	if (isCentered)
	{
		x -= mCelWidth / 2;
		y -= mCelHeight / 2;
		theOtherX -= theOther->mCelWidth / 2;
		theOtherY -= theOther->mCelHeight / 2;
	}

	Rect aRect(x, y, mCelWidth, mCelHeight);
	Rect anOtherRect(theOtherX, theOtherY, theOther->mCelWidth, theOther->mCelHeight);
	if ((anOtherRect.mWidth + theOtherX <= x) || (anOtherRect.mHeight + theOtherY <= y) ||
		(theOtherX >= mCelWidth + x) || (theOtherY >= mCelHeight + y))
		return false;

	Rect anOverlap = SpriteIntersection(aRect, anOtherRect);
	int aSrcX = mCelWidth * theColumn - x + anOverlap.mX;
	int aSrcY = mCelHeight * theRow - y + anOverlap.mY;
	int anOtherSrcX = theOther->mCelWidth * theOtherColumn - theOtherX + anOverlap.mX;
	int anOtherSrcY = theOther->mCelHeight * theOtherRow - theOtherY + anOverlap.mY;
	unsigned char* aMask = GetAlphaMask();
	unsigned char* anOtherMask = theOther->GetAlphaMask();

	int aRowOffset = aSrcY - anOtherSrcY;
	for (int aRow = 0; aRow < anOverlap.mHeight; aRow++, anOtherSrcY++)
	{
		for (int aColumn = 0; aColumn < anOverlap.mWidth; aColumn++)
		{
			int aWidth = mImage->GetWidth();
			unsigned char anAlpha = isMirrored ?
				aMask[aWidth * (aRowOffset + anOtherSrcY) - aColumn - aSrcX + aWidth - 1] :
				aMask[aWidth * (aRowOffset + anOtherSrcY) + aColumn + aSrcX];
			int anOtherWidth = theOther->mImage->GetWidth();
			unsigned char anOtherAlpha = isOtherMirrored ?
				anOtherMask[anOtherWidth * anOtherSrcY - aColumn - anOtherSrcX + anOtherWidth - 1] :
				anOtherMask[anOtherWidth * anOtherSrcY + aColumn + anOtherSrcX];
			if ((anAlpha != 0) && (anOtherAlpha != 0))
				return true;
		}
	}
	return false;
}
