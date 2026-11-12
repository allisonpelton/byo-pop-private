// BoardCheats.cpp: the cheat words that the board takes from the keyboard (Sexy::Board::KeyChar 0x59d5df), and the colour
// changes that three of them make to every image.
//   KeyChar 0x59d5df, TransformAllImages 0x59b592, TransformImage 0x595c2b, the pixel functions 0x595a69-0x595ba3.
//
// The last 24 letters typed are kept; a word anywhere in them is taken (and the letters are forgotten):
//   OLDSKOOL (the 2D drawing of the board on and off), COLORSWAP (the colours of the gems turned one step round),
//   XMAS (a snowy... other backdrop and falling flakes of light on and off), SLOMO (the timed games slow down; counts as a
//   cheat), BLACKANDWHITE, AMBERSCREEN, GREENSCREEN (every image is turned into that colour), NAUSEA (the board sways),
//   STARFIELD and NETWORK (a field of stars or of linked dots behind everything, on and off), NOFRAME (no frame drawn) and
//   NORMAL (all of that off again).
// The space bar pauses a timed game.

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "StarField.h"
#include "CustomResourceManager.h"
#include "MemoryImage.h"
#include "DDInterface.h"
#include "Color.h"
#include <ctype.h>
#include <vector>

using namespace Sexy;

extern Color* const gGemColors;

// 0x595a69: the three colour bytes turned one place (the gems' own colours turn the same way)
static void PixelsRotate(ulong* thePixels, int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		unsigned int aPixel = thePixels[i];
		thePixels[i] = (aPixel >> 16 & 0xff) | ((aPixel & 0xffff) << 8) | (aPixel & 0xff000000);
	}
}

// port: the brightness of a pixel (the weights of green, red and blue are 0x99, 0x4c and 0x1a of 0xff)
static unsigned int PixelBrightness(unsigned int thePixel)
{
	return ((thePixel >> 8 & 0xff) * 0x99) / 0xff + ((thePixel >> 16 & 0xff) * 0x4c) / 0xff + ((thePixel & 0xff) * 0x1a) / 0xff;
}

// 0x595aa0
static void PixelsGray(ulong* thePixels, int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		unsigned int aPixel = thePixels[i];
		unsigned int aBrightness = PixelBrightness(aPixel);
		thePixels[i] = (aBrightness << 16) | (aBrightness << 8) | (aPixel & 0xff000000) | aBrightness;
	}
}

// 0x595b15
static void PixelsAmber(ulong* thePixels, int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		unsigned int aPixel = thePixels[i];
		unsigned int aBrightness = PixelBrightness(aPixel) | 7;
		thePixels[i] = (((aBrightness * 0xff) / 0xff << 8 | (aBrightness * 0xbe) / 0xff) << 8) | (aPixel & 0xff000000);
	}
}

// 0x595ba3
static void PixelsGreen(ulong* thePixels, int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		unsigned int aPixel = thePixels[i];
		unsigned int aBrightness = PixelBrightness(aPixel) | 7;
		thePixels[i] = ((aBrightness * 0xff) / 0xff << 8) | (aBrightness * 0x16) / 0xff | (aPixel & 0xff000000);
	}
}

// 0x595c2b: every pixel of an image goes through theFunction
static void TransformImage(Image* theImage, void (*theFunction)(ulong*, int))
{
	MemoryImage* anImage = (MemoryImage*)theImage;
	theFunction(anImage->GetBits(), anImage->mWidth * anImage->mHeight);
	anImage->BitsChanged();
}

// 0x59b592: every image that the program holds goes through theFunction, but the screen, the cursors, the reload screen
// and the backdrop that is on the board
void Board::TransformAllImages(void (*theFunction)(ulong*, int))
{
	mImagesChanged = true;
	std::vector<MemoryImage*> anImages(mApp->mMemoryImageSet.begin(), mApp->mMemoryImageSet.end());
	for (size_t i = 0; i < anImages.size(); i++)
	{
		MemoryImage* anImage = anImages[i];
		if (anImage != (MemoryImage*)mApp->mDDInterface->mScreenImage &&
			anImage != (MemoryImage*)mApp->mDDInterface->mCursorImage &&
			anImage != mBackdropImage && anImage != IMAGE_RELOAD_BKG && anImage != IMAGE_RELOAD_OVERLAY &&
			anImage != IMAGE_RELOAD_OVERLAY_LIT && anImage != IMAGE_CURSOR_POINTER && anImage != IMAGE_CURSOR_HAND &&
			anImage != IMAGE_CURSOR_DRAGGING && anImage != IMAGE_CURSOR_TEXT)
			TransformImage(anImage, theFunction);
	}
}

// port: the star field or the dots: theNetwork is the second kind. The same word again takes it away; the other word changes it.
void Board::ToggleStarField(bool theNetwork)
{
	if (mCheatStars != NULL)
	{
		if (mCheatStars->mNetwork == theNetwork)
		{
			delete mCheatStars;
			mCheatStars = NULL;
			return;
		}
		delete mCheatStars;
	}
	mCheatStars = new StarField();
	mCheatStars->Init(theNetwork ? 100 : 1000, theNetwork);
}

// 0x59d5df
void Board::KeyChar(SexyChar theChar)
{
	mCheatBuffer += (char)toupper((unsigned char)theChar);
	if (mCheatBuffer.size() > 0x18)
		mCheatBuffer.erase(0, 1);

	const std::string::size_type NONE = std::string::npos;
	bool aMatched = true;
	if (mCheatBuffer.find("OLDSKOOL") != NONE)
	{
		mOldSkool = !mOldSkool;
		MarkDirtyFull();
	}
	else if (mCheatBuffer.find("COLORSWAP") != NONE)
	{
		for (int i = 0; i < 7; i++)
		{
			TransformImage(GetImageById(IMAGE_GEM0_ID + i), PixelsRotate);
			for (int j = 0; j < 6; j++)
				TransformImage(mApp->mShrunkenGems[i][j], PixelsRotate);
			Color& aColor = gGemColors[i];
			int aRed = aColor.mRed;
			aColor.mRed = aColor.mGreen;
			aColor.mGreen = aColor.mBlue;
			aColor.mBlue = aRed;
		}
		TransformImage(IMAGE_OLD_GEMS, PixelsRotate);
		if (mApp->mBombifiedStillStrip != NULL)
			TransformImage(mApp->mBombifiedStillStrip, PixelsRotate);
		mImagesChanged = true;
	}
	else if (mCheatBuffer.find("XMAS") != NONE)
	{
		mXmas = !mXmas;
		if (mXmas)
		{
			delete mBackdropImage;    // (the original leaves it behind)
			CustomResourceManager* aResourceManager = (CustomResourceManager*)mApp->mResourceManager;
			mBackdropImage = (MemoryImage*)aResourceManager->GetCachedImage("images\\backdrops\\backdrop02");
			mBackdropImage->mPurgeBits = true;
			mBackdropImage->PurgeBits();
		}
	}
	else if (mCheatBuffer.find("SLOMO") != NONE)
	{
		if (mIsAction)
			mCheated = true;
		mSlowMotion = !mSlowMotion;
	}
	else if (mCheatBuffer.find("BLACKANDWHITE") != NONE)
		TransformAllImages(PixelsGray);
	else if (mCheatBuffer.find("AMBERSCREEN") != NONE)
		TransformAllImages(PixelsAmber);
	else if (mCheatBuffer.find("GREENSCREEN") != NONE)
		TransformAllImages(PixelsGreen);
	else if (mCheatBuffer.find("NAUSEA") != NONE)
		mNausea = !mNausea;
	else if (mCheatBuffer.find("STARFIELD") != NONE)
		ToggleStarField(false);
	else if (mCheatBuffer.find("NETWORK") != NONE)
		ToggleStarField(true);
	else if (mCheatBuffer.find("NOFRAME") != NONE)
	{
		mRedrawBackground = true;
		mNoFrame = !mNoFrame;
		MarkDirtyFull();
	}
	else if (mCheatBuffer.find("NORMAL") != NONE)
	{
		if (!mImagesChanged)
		{
			mOldSkool = false;
			mXmas = false;
			mSlowMotion = false;
			mNausea = false;
			mNoFrame = false;
			delete mCheatStars;
			mCheatStars = NULL;
		}
		else
		{
			mApp->DoReloading(mApp->mIsWindowed, mApp->Is3DAccelerated());
		}
	}
	else
		aMatched = false;

	if (aMatched)
		mCheatBuffer = "";

	if (mIsAction && mScreenState == SCREEN_SHOWN && mHyperspace == NULL && theChar == ' ')
	{
		if (mPauseCount < 1)
			Pause(true);
		else
			Unpause(true);
	}
}
