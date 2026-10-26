#include "Sprite.h"
#include "FossilizedApp.h"
#include "DDImage.h"
#include "SexyAppBase.h"
#include "SoundManager.h"
#include <cstring>

using namespace Sexy;

static Graphics*	gDrawGraphics = NULL;	// 0x5304dc
static Point		gDrawOffset;			// 0x5304e0

// 0x4360f0
void Sexy::SetDrawGraphics(Graphics* g)
{
	gDrawGraphics = g;
}

// 0x436100
Graphics* Sexy::GetDrawGraphics()
{
	return gDrawGraphics;
}

// 0x436110
void Sexy::SetDrawOffset(const Point& theOffset)
{
	gDrawOffset = theOffset;
}

// 0x436130
Point Sexy::GetDrawOffset()
{
	return gDrawOffset;
}

// 0x436150
void Sexy::AddDrawOffset(const Point& theOffset)
{
	gDrawOffset.mX += theOffset.mX;
	gDrawOffset.mY += theOffset.mY;
}

// 0x437210
Pic::Pic(SexyAppBase* theApp, const std::string& thePath, bool palletize)
{
	mApp = theApp;
	mImage = NULL;
	mWidth = 0;
	mHeight = 0;
	Load(thePath, palletize);
	mShared = false;
}

// 0x4370a0
void Pic::Load(const std::string& thePath, bool palletize)
{
	mImage = mApp->GetImage(thePath);
	if (mImage == NULL)
	{
		mApp->Popup("Failed to load image:" + thePath);
		mApp->mLoadingFailed = true;
		return;
	}
	if (palletize)
		mImage->Palletize();
	mWidth = mImage->mWidth;
	mHeight = mImage->mHeight;
}

// 0x435ee0
Pic::Pic(SexyAppBase* theApp, const unsigned int* theBits, int theWidth, int theHeight)
{
	mApp = theApp;
	mImage = new DDImage(theApp->mDDInterface);
	mImage->Create(theWidth, theHeight);
	mImage->SetImageMode(false, true);
	memcpy(mImage->GetBits(), theBits, (size_t) theWidth * theHeight * 4);
	mImage->BitsChanged();
	mWidth = theWidth;
	mHeight = theHeight;
	mShared = false;
}

// 0x435fc0
Pic::Pic(const Pic& theOther)
{
	mImage = NULL;
	mApp = theOther.mApp;
	mWidth = theOther.mWidth;
	mHeight = theOther.mHeight;
	mShared = false;
	mImage = new DDImage(mApp->mDDInterface);
	mImage->Create(mWidth, mHeight);
	mImage->mHasTrans = theOther.mImage->mHasTrans;
	mImage->mHasAlpha = theOther.mImage->mHasAlpha;
	memcpy(mImage->GetBits(), theOther.mImage->GetBits(), (size_t) mWidth * mHeight * 4);
	mImage->BitsChanged();
}

// 0x4360c0
Pic::~Pic()
{
	if (!mShared && mImage != NULL)
	{
		delete mImage;
		mImage = NULL;
	}
}

// 0x436170
void Pic::Draw(int theX, int theY)
{
	gDrawGraphics->DrawImage(mImage, gDrawOffset.mX + theX, gDrawOffset.mY + theY);
}

// 0x436a40
void Pic::DrawLine(int theX1, int theY1, int theX2, int theY2)
{
	int aX = theX1;
	int aY = theY1;
	int aDX = theX2 - theX1;
	int aDY = theY2 - theY1;
	int aStepX = 1;
	int aStepY = 1;
	if (aDX < 0)
	{
		aDX = -aDX;
		aStepX = -1;
	}
	if (aDY < 0)
	{
		aDY = -aDY;
		aStepY = -1;
	}
	int aLong, aShort, aStraightX, aStraightY;
	if (aDX < aDY)
	{
		aStraightX = 0;
		aStraightY = aStepY;
		aShort = aDX;
		aLong = aDY;
	}
	else
	{
		aStraightX = aStepX;
		aStraightY = 0;
		aShort = aDY;
		aLong = aDX;
	}
	int aDiagonalInc = (aShort - aLong) * 2;
	int anError = aShort * 2 - aLong;
	Draw(aX - (mWidth >> 1), aY - (mHeight >> 1));
	int aCount = aLong;
	if (0 < aCount)
	{
		do
		{
			int anInc = aDiagonalInc;
			int aMoveX = aStepX;
			int aMoveY = aStepY;
			if (anError < 0)
			{
				anInc = aShort * 2;
				aMoveX = aStraightX;
				aMoveY = aStraightY;
			}
			aY += aMoveY;
			aX += aMoveX;
			anError += anInc;
			Draw(aX - (mWidth >> 1), aY - (mHeight >> 1));
			aCount--;
		} while (aCount != 0);
	}
}

// 0x436b50: the original clips the destination to the Graphics' clip rectangle itself and moves the source's top down
// by the rows it cut; the port's stretched DrawImage clips the same rectangle.
void Pic::DrawStretched(int theX1, int theY1, int theX2, int theY2, bool fast)
{
	Graphics* g = gDrawGraphics;
	g->SetFastStretch(fast);
	g->DrawImage(mImage, Rect(gDrawOffset.mX + theX1, gDrawOffset.mY + theY1, theX2 - theX1, theY2 - theY1),
		Rect(0, 0, mWidth, mHeight));
	g->SetFastStretch(false);
}

// 0x4361a0
void Pic::MultiplyAlpha(Pic* thePic, int theX, int theY)
{
	unsigned int* aDestRow = GetBits() + (mWidth * theY + theX);
	unsigned int* aSrc = thePic->GetBits();
	int aRow = 0;
	if (0 < thePic->mHeight)
	{
		do
		{
			unsigned int* aDest = aDestRow;
			int aCol = 0;
			if (0 < thePic->mWidth)
			{
				do
				{
					unsigned int aPixel = *aDest;
					unsigned int anAlpha = ((*aSrc >> 24) * (aPixel >> 24)) << 16;
					*aDest = (aPixel & 0xffffff) | (anAlpha & 0xff000000);
					aSrc++;
					aCol++;
					aDest++;
				} while (aCol < thePic->mWidth);
			}
			aDestRow += mWidth;
			aRow++;
		} while (aRow < thePic->mHeight);
	}
	BitsChanged();
}

// 0x436800: only ever called with all three channels (0xff); the alpha is the OR of the mask's channels
void Pic::AlphaFromColour(Pic* theMask, int theChannels)
{
	unsigned char* aDest = (unsigned char*) GetBits();
	unsigned char* aSrc = (unsigned char*) theMask->GetBits();
	unsigned char aRed = (theChannels & 1) ? 0xff : 0;
	unsigned char aGreen = (theChannels & 2) ? 0xff : 0;
	unsigned char aBlue = (theChannels & 4) ? 0xff : 0;
	int i = 0;
	if (0 < mHeight * mWidth)
	{
		do
		{
			aDest[3] = (aSrc[1] & aGreen) | (aSrc[2] & aRed) | (aSrc[0] & aBlue);
			aDest += 4;
			aSrc += 4;
			i++;
		} while (i < mHeight * mWidth);
	}
	BitsChanged();
}

// 0x436cb0
Cell::Cell(Pic* thePic, int theCellWidth, int theCellHeight, int theIndex)
{
	mImage = thePic->mImage;
	if (theCellWidth == 0)
		theCellWidth = 1;
	mWidth = theCellWidth;
	mHeight = theCellHeight;
	int aColumns = thePic->mWidth / theCellWidth;
	mSrc = Rect((theIndex % aColumns) * theCellWidth, (theIndex / aColumns) * theCellHeight, theCellWidth,
		theCellHeight);
}

// 0x436d40
Cell::Cell(Pic* thePic, int theX1, int theY1, int theX2, int theY2)
{
	mImage = thePic->mImage;
	mSrc = Rect(theX1, theY1, theX2 - theX1, theY2 - theY1);
	mWidth = (theX2 - theX1) + 1;
	mHeight = (theY2 - theY1) + 1;
}

// 0x4365a0
void Cell::Draw(int theX, int theY)
{
	gDrawGraphics->DrawImage(mImage, gDrawOffset.mX + theX, gDrawOffset.mY + theY, mSrc);
}

// 0x436d90
void Cell::DrawPart(int theX, int theY, int theSrcX, int theSrcY, int theWidth, int theHeight)
{
	Rect aSrc(mSrc.mX + theSrcX, mSrc.mY + theSrcY, theWidth, theHeight);
	gDrawGraphics->DrawImage(mImage, gDrawOffset.mX + theX, gDrawOffset.mY + theY, aSrc);
}

// 0x436df0: the original clips the destination itself (see Pic::DrawStretched); fast stretch on
void Cell::DrawStretched(int theX1, int theY1, int theX2, int theY2)
{
	Graphics* g = gDrawGraphics;
	g->SetFastStretch(true);
	g->DrawImage(mImage, Rect(gDrawOffset.mX + theX1, gDrawOffset.mY + theY1, theX2 - theX1, theY2 - theY1), mSrc);
	g->SetFastStretch(false);
}

// 0x437250
PicFont::PicFont(SexyAppBase* theApp, const std::string& thePath, const char* theCharset, int theSpaceWidth,
	bool palletize) : Pic(theApp, thePath, palletize)
{
	mSpaceWidth = theSpaceWidth;
	for (int i = 0; i < 256; i++)
	{
		mGlyphs[i] = NULL;
		mAdvance[i] = 0;
	}
	Cut(theCharset);
}

// 0x4365d0
PicFont::~PicFont()
{
	for (int i = 0; i < 256; i++)
	{
		delete mGlyphs[i];
		mGlyphs[i] = NULL;
	}
}

// 0x436f70: each glyph starts at the next column with an opaque pixel and ends before the next column without one
void PicFont::Cut(const char* theCharset)
{
	if (theCharset == NULL)
		return;
	unsigned int* aBits = GetBits();
	int aCol = 0;
	const unsigned char* aChar = (const unsigned char*) theCharset;
	while (*aChar != 0)
	{
		int aStart = aCol;
		bool found = false;
		while (aCol < mWidth)
		{
			for (int y = 0; y < mHeight; y++)
			{
				if ((aBits[y * mWidth + aCol] & 0xff000000) != 0)
				{
					found = true;
					break;
				}
			}
			if (found)
				break;
			aCol++;
		}
		aStart = aCol;
		if (aCol >= mWidth)
			return;
		while (aCol < mWidth)
		{
			bool opaque = false;
			for (int y = 0; y < mHeight; y++)
			{
				if ((aBits[y * mWidth + aCol] >> 24) != 0)
				{
					opaque = true;
					break;
				}
			}
			if (!opaque)
				break;
			aCol++;
		}
		mGlyphs[*aChar] = new Cell(this, aStart, 0, aCol, mHeight);
		mAdvance[*aChar] = mGlyphs[*aChar]->mWidth;
		aChar++;
	}
}

// 0x436650
void PicFont::AdjustAdvances(int theDelta)
{
	for (int i = 0; i < 256; i++)
		mAdvance[i] += theDelta;
}

// 0x436670
void PicFont::Draw(const char* theText, int theX, int theY)
{
	if (theText == NULL)
		return;
	for (const char* p = theText; *p != 0; p++)
	{
		char aChar = *p;
		if (aChar > 0)
		{
			if (aChar == ' ')
				theX += mSpaceWidth;
			else if (mGlyphs[(unsigned char) aChar] != NULL)
			{
				mGlyphs[(unsigned char) aChar]->Draw(theX, theY);
				theX += mAdvance[(unsigned char) aChar];
			}
		}
	}
}

// 0x4366d0
int PicFont::Width(const char* theText)
{
	int aWidth = 0;
	if (theText == NULL)
		return aWidth;
	for (const char* p = theText; *p != 0; p++)
	{
		char aChar = *p;
		if (aChar == ' ')
			aWidth += mSpaceWidth;
		else if (mGlyphs[(unsigned char) aChar] != NULL)
			aWidth += mAdvance[(unsigned char) aChar];
	}
	return aWidth;
}

// 0x4372e0
MaskedPic::MaskedPic(SexyAppBase* theApp, const std::string& thePath) : Pic(theApp, thePath, false)
{
	mMask = new Pic(theApp, thePath + "_", false);
	mImage->SetImageMode(false, true);
	AlphaFromColour(mMask, 0xff);
}

// 0x436720
MaskedPic::MaskedPic(const MaskedPic& theOther) : Pic(theOther)
{
	mMask = new Pic(*theOther.mMask);
}

// 0x4367a0
MaskedPic::~MaskedPic()
{
	delete mMask;
}

// 0x435b50
GameSound::GameSound(SexyAppBase* theApp, const std::string& thePath)
{
	mApp = theApp;
	mId = 0;
	if (((FossilizedApp*) theApp)->mSoundInitialized)
	{
		static int sNextId = 0;		// 0x5304d8
		mId = sNextId;
		sNextId++;
		if (!mApp->mSoundManager->LoadSound(mId, thePath))
		{
			mApp->Popup("Failed to load sound: " + thePath);
			mApp->mLoadingFailed = true;
		}
	}
}

// 0x435b10
void GameSound::Play()
{
	if (((FossilizedApp*) mApp)->mSoundInitialized)
		mApp->PlaySample(mId);
}
