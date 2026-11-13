#include "Banner.h"
#include "Board.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "ImageFont.h"

#include <math.h>

using namespace Sexy;

// 0x41c810
Banner::Banner(Board* theBoard, ImageFont* theFont, const std::string& theText, int y, int theDelay)
{
	mBoard = theBoard;
	mDelay = theDelay;
	mFont = theFont;
	mText = theText;
	mY = y - theFont->GetDescent();
	mWidth = theFont->StringWidth(mText);
	mNode = theBoard->mBanners.Append(this);

	// Each letter starts 2 sizes further out than the one before it, so they arrive one after another
	int aScale = 9;
	for (unsigned int i = 0; i < mText.size(); i++)
	{
		mScales.push_back((double)aScale);
		mWobbles.push_back(1.0);
		mWobbleAngles.push_back(0.0);
		aScale += 2;
	}
}

// 0x41c5f0
void Banner::Update()
{
	if (mDelay != 0)
	{
		mDelay--;
		return;
	}

	for (unsigned int i = 0; i < mText.size(); i++)
	{
		mScales[i] = mScales[i] - 0.3;
		if (mScales[i] <= 8.0)
		{
			mWobbles[i] = mWobbles[i] - 0.02;
			if (mWobbles[i] < 0.0)
				mWobbles[i] = 0.0;
			if (mScales[i] < 0.0)
				mWobbleAngles[i] = mWobbleAngles[i] + 0.3;
		}
	}

	if (mScales[mText.size() - 1] < -80.0)
		mBoard->mBanners.Remove(mNode);	// deletes this
}

// 0x41cae0
void Banner::Draw(Graphics* g)
{
	mPenX = -(mWidth / 2);
	g->SetColorizeImages(true);
	g->SetFastStretch(!mBoard->mApp->Is3DAccelerated());

	for (unsigned int i = 0; i < mText.size(); i++)
	{
		if (mScales[i] <= 8.0)
		{
			// Fading in as it arrives, out from size -70
			double aFade = mScales[i] * 32.0;
			if (aFade < 0.0)
				aFade = 0.0;
			if (mScales[i] <= -70.0)
			{
				aFade = (mScales[i] + 70.0) * -50.0;
				if (aFade > 255.0)
					aFade = 255.0;
			}
			g->SetColor(Color(255, 255, 255, (int)(255.0 - aFade)));

			double aScale = mScales[i];
			if (aScale < 1.0)
				aScale = 1.0;

			// The letter's cel in the font's main layer. The original indexes the character map, and the cels with
			// the map's value, as signed chars; the banners' texts are plain ASCII, where the two agree.
			FontLayer* aLayer = mFont->mFontData->mFontLayerMap["MAIN"];
			Rect aSrcRect = aLayer->mCharData[mFont->mFontData->mCharMap[(unsigned char)mText[i]]].mImageRect;
			Image* anImage = mFont->mFontData->mFontLayerMap["MAIN"]->mImage;

			// A bigger letter is nearer: its place is pushed out from the screen's middle (240) as well as grown
			double aSize = cos(mWobbleAngles[i]) * mWobbles[i] + aScale;
			double aDepth = aScale * 0.25 + 0.75;
			int aHalfWidth = aSrcRect.mWidth / 2;
			int aCentreX = (int)(mPenX * aDepth) + aHalfWidth;
			int aX = (int)(aCentreX - aHalfWidth * aSize);
			int aHalfHeight = aSrcRect.mHeight / 2;
			int aCentreY = (int)((mY - 240) * aDepth + 240.0) + aHalfHeight;
			int aY = (int)(aCentreY - aHalfHeight * aSize);
			g->DrawImage(anImage, Rect(aX, aY, (int)(aSrcRect.mWidth * aSize), (int)(aSrcRect.mHeight * aSize)), aSrcRect);
		}

		if (i < mText.size() - 1)
		{
			int aWidth = mFont->CharWidthKern(mText[i], mText[i + 1]);
			if (aWidth <= 0)
				aWidth = mFont->CharWidth(mText[i]);
			mPenX += aWidth;
		}
		else
		{
			mPenX += mFont->CharWidth(mText[i]);
		}
	}

	g->SetColorizeImages(false);
}
