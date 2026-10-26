// PGSprite (see PGSprite.h).

#include "PGSprite.h"
#include "HFileSystem.h"

namespace pg
{

PGSprite::PGSprite()
{
	mRefCount = 0;
	mSurface = nullptr;
	mDisplay = nullptr;
	HStrncpy(mName, "(unnamed)", 0x40);
}

// port: the original only noted a sprite still holding its surface; here the surface goes with the sprite
PGSprite::~PGSprite()
{
	if (mSurface != nullptr && mDisplay != nullptr)
		mDisplay->freeSurface(mSurface);
	mSurface = nullptr;
}

PGSprite* PGSprite::clone()
{
	PGSprite* aSprite = new PGSprite();
	aSprite->mDisplay = mDisplay;
	aSprite->mSurface = mDisplay->copySurface(mSurface);
	return aSprite;
}

void PGSprite::drawAt(const PGPoint* theAt)
{
	if (mSurface != nullptr)
	{
		int h = height();
		PGRect aDest = {theAt->x, theAt->y, width() + theAt->x, h + theAt->y};
		PGRect aSrc = {0, 0, width(), height()};
		mDisplay->blit(&aDest, &aSrc, mSurface);
	}
}

void PGSprite::blitEx(const PGRect* theDest, const PGRect* theSrc, int s1, int s2, int s3, int s4, int d1, int d2,
	int d3, int d4, int theOp)
{
	if (mSurface != nullptr)
		mDisplay->blitEx(theDest, theSrc, mSurface, s1, s2, s3, s4, d1, d2, d3, d4, theOp);
}

void PGSprite::blit(const PGRect* theDest, const PGRect* theSrc)
{
	if (mSurface != nullptr)
		mDisplay->blit(theDest, theSrc, mSurface);
}

bool PGSprite::create(HDisplay* theDisplay, const HImage& theImage)
{
	if (mSurface == nullptr)
	{
		mDisplay = theDisplay;
		mSurface = theDisplay->createSurface(theImage);
	}
	mRefCount++;
	return true;
}

bool PGSprite::load(HDisplay* theDisplay, const HFileSystem* theFileSystem, const char* theName)
{
	if (mSurface != nullptr)
	{
		mRefCount++;
		return true;
	}
	mDisplay = theDisplay;
	if (theFileSystem == nullptr)
	{
		PGMessageBox(0x31, "Error", "The read file system was not initialized.");
		mSurface = nullptr;
		return false;
	}
	HImage anImage;
	if (!LoadImage(theFileSystem, theName, anImage))
		return false;
	mSurface = theDisplay->createSurface(anImage);
	HStrncpy(mName, theName, 0x40);
	mRefCount++;
	return true;
}

void PGSprite::release()
{
	mRefCount--;
	if (mRefCount < 1 && mSurface != nullptr)
	{
		mDisplay->freeSurface(mSurface);
		mSurface = nullptr;
	}
}

int PGSprite::width() const
{
	if (mSurface == nullptr)
		return 0;
	return mSurface->mWidth;
}

int PGSprite::height() const
{
	if (mSurface == nullptr)
		return 0;
	return mSurface->mHeight;
}

void PGSprite::getRect(PGRect* theRect) const
{
	theRect->left = 0;
	theRect->top = 0;
	theRect->right = width();
	theRect->bottom = height();
}

} // namespace pg
