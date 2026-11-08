#include "Support.h"
#include "AtomicaApp.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "DDImage.h"
#include "SexyAppBase.h"
#include <cstring>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// Rand

GameRand::GameRand() : mJ(0), mK(0x1f), mDrawing(false), mDrawJ(0), mDrawK(0x1f)
{
	Seed(1);
}

// 0x425e30
void GameRand::Seed(unsigned int theSeed)
{
	mState[0] = theSeed & 0x3fffffff;
	mJ = 0;
	mK = 0x1f;
	mState[1] = 1;
	for (int i = 0; i < 0x35; i++)
		mState[i + 2] = (mState[i] + mState[i + 1]) & 0x3fffffff;
	// the drawing generator (the fixes mod) starts from the seed in the same way, from another point of the sequence
	mDrawState[0] = (theSeed ^ 0x2545f491u) & 0x3fffffff;
	mDrawJ = 0;
	mDrawK = 0x1f;
	mDrawState[1] = 1;
	for (int i = 0; i < 0x35; i++)
		mDrawState[i + 2] = (mDrawState[i] + mDrawState[i + 1]) & 0x3fffffff;
}

// 0x425e80
int GameRand::Next()
{
	if (mDrawing)
	{
		unsigned int aDrawn = (mDrawState[mDrawK] + mDrawState[mDrawJ]) & 0x3fffffff;
		mDrawState[mDrawJ] = aDrawn;
		mDrawJ++;
		if (mDrawJ == 0x37)
			mDrawJ = 0;
		mDrawK++;
		if (mDrawK == 0x37)
			mDrawK = 0;
		return (int) aDrawn >> 6;
	}
	unsigned int aValue = (mState[mK] + mState[mJ]) & 0x3fffffff;
	mState[mJ] = aValue;
	mJ++;
	if (mJ == 0x37)
		mJ = 0;
	mK++;
	if (mK == 0x37)
		mK = 0;
	return (int) aValue >> 6;
}

// 0x425f10
int GameRand::Get(int theRange, bool theSigned)
{
	if (theRange == 0)
		return 0;
	int aBound = 2;
	if (2 < theRange)
	{
		do
		{
			aBound = aBound * 2;
		} while (aBound < theRange);
	}
	int aResult = (int) (Next() & (aBound - 1)) % theRange;
	if (theSigned && (Get(2, false) == 1))
		aResult = -aResult;
	return aResult;
}

// ---------------------------------------------------------------------------------------------------------
// Sprite

Sprite::~Sprite()
{
	if (mOwnsImage)
		delete mImage;
}

// 0x429610
void Sprite::SetImage(Image* theImage)
{
	mImage = theImage;
	mWidth = theImage->GetWidth();
	mHeight = theImage->GetHeight();
	mSrc = Rect(0, 0, mWidth, mHeight);
}

unsigned int* Sprite::GetBits()
{
	return (unsigned int*) static_cast<MemoryImage*>(mImage)->GetBits();
}

void Sprite::BitsChanged()
{
	static_cast<MemoryImage*>(mImage)->BitsChanged();
}

// 0x4014c0
void Sprite::Draw(Graphics* g, int theX, int theY)
{
	if (mImage != NULL)
		g->DrawImage(mImage, theX, theY, mSrc);
}

// 0x4014f0
void Sprite::DrawCentered(Graphics* g, int theX, int theY)
{
	if (mImage != NULL)
		g->DrawImage(mImage, theX - (mWidth >> 1), theY - (mHeight >> 1), mSrc);
}

// 0x401570
void Sprite::DrawCenteredF(Graphics* g, float theX, float theY)
{
	if (mImage != NULL)
	{
		int aY = (int) ((theY + 0.5f) - (float) (mHeight >> 1));
		int aX = (int) ((theX + 0.5f) - (float) (mWidth >> 1));
		g->DrawImage(mImage, aX, aY, mSrc);
	}
}

// 0x4296e0
void Sprite::DrawPartial(Graphics* g, int theX, int theY, float theFraction, int theModeX, int theModeY)
{
	Rect aSrc = mSrc;
	if (theModeX == 1)
		aSrc.mWidth = (int) ((float) aSrc.mWidth * theFraction);
	if (theModeY == 1)
		aSrc.mHeight = (int) ((float) aSrc.mHeight * theFraction);
	if (theModeX == -1)
	{
		float aCut = (float) mSrc.mWidth - theFraction * (float) mSrc.mWidth;
		theX = (int) ((float) theX + aCut);
		aSrc.mX = (int) ((float) mSrc.mX + aCut);
		aSrc.mWidth = (int) ((float) aSrc.mWidth * theFraction);
	}
	if (theModeY == -1)
	{
		float aCut = (float) mSrc.mHeight - theFraction * (float) mSrc.mHeight;
		theY = (int) ((float) theY + aCut);
		aSrc.mY = (int) ((float) mSrc.mY + aCut);
		aSrc.mHeight = (int) ((float) aSrc.mHeight * theFraction);
	}
	if (mImage != NULL)
		g->DrawImage(mImage, theX, theY, aSrc);
}

// ---------------------------------------------------------------------------------------------------------
// SpriteMgr

SpriteMgr::~SpriteMgr()
{
	for (size_t i = 0; i < mSprites.size(); i++)
		delete mSprites[i];
}

// 0x429c60
Sprite* SpriteMgr::Load(const std::string& theName)
{
	std::string aPath = "images\\" + theName;
	DDImage* anImage = mApp->GetImage(aPath);
	if (anImage == NULL)
	{
		mApp->Popup("Could not load image: " + aPath);
		mLoadFailed = true;
		return NULL;
	}
	Sprite* aSprite = new Sprite();
	mSprites.push_back(aSprite);
	aSprite->SetImage(anImage);
	return aSprite;
}

// 0x429bc0
Sprite* SpriteMgr::Wrap(Image* theImage)
{
	Sprite* aSprite = new Sprite();
	mSprites.push_back(aSprite);
	aSprite->SetImage(theImage);
	return aSprite;
}

// 0x429970
Sprite* SpriteMgr::SubSprite(Sprite* theSheet, int theX, int theY, int theWidth, int theHeight)
{
	Sprite* aSprite = new Sprite();
	mSprites.push_back(aSprite);
	aSprite->mOwnsImage = false;
	aSprite->mImage = theSheet->mImage;
	aSprite->mSrc = Rect(theX, theY, theWidth, theHeight);
	aSprite->mWidth = theWidth;
	aSprite->mHeight = theHeight;
	return aSprite;
}

// 0x429a10: cell theIndex of a sheet cut in cells of theCellWidth by theCellHeight, row by row
Sprite* SpriteMgr::Cell(Sprite* theSheet, int theCellWidth, int theCellHeight, int theIndex)
{
	Sprite* aSprite = new Sprite();
	mSprites.push_back(aSprite);
	aSprite->mOwnsImage = false;
	aSprite->mImage = theSheet->mImage;
	aSprite->mWidth = theCellWidth;
	aSprite->mHeight = theCellHeight;
	int aColumns = theSheet->mWidth / theCellWidth;
	aSprite->mSrc = Rect((theIndex % aColumns) * theCellWidth, (theIndex / aColumns) * theCellHeight, theCellWidth,
		theCellHeight);
	return aSprite;
}

// 0x429ab0
Sprite* SpriteMgr::Blank(int theWidth, int theHeight)
{
	Sprite* aSprite = new Sprite();
	mSprites.push_back(aSprite);
	MemoryImage* anImage = new MemoryImage(mApp);
	anImage->Create(theWidth, theHeight);
	memset(anImage->GetBits(), 0, (size_t) theWidth * theHeight * 4);
	anImage->SetImageMode(true, true);
	anImage->BitsChanged();
	aSprite->SetImage(anImage);
	return aSprite;
}

// 0x429920
void SpriteMgr::Free(Sprite* theSprite)
{
	for (size_t i = 0; i < mSprites.size(); i++)
	{
		if (mSprites[i] == theSprite)
		{
			mSprites.erase(mSprites.begin() + i);
			delete theSprite;
			return;
		}
	}
}
