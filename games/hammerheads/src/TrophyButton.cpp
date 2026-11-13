#include "TrophyButton.h"

#include "Application.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

namespace
{

// How fast the flash dies down for each frame of time
const double FLASH_SPEED = 0.01;

// How much of a trophy's shape the unwon ones keep: the alpha of each of its pixels is this much, and its colour is black
const double SHADOW_ALPHA = 0.1;

}

// 0x45fb20
TrophyButton::TrophyButton(int theId, ButtonListener* theListener, int theTrophy, bool theEarned, int theWidth, int theHeight) :
	DialogButton(nullptr, theId, theListener)
{
	mFlash = 0.0;
	mFlashStart = 0.0;
	mTrophy = theTrophy;
	mEarned = theEarned;
	mShadowImage = nullptr;

	if (!theEarned)
	{
		mShadowImage = new MemoryImage(gApp);
		mShadowImage->Create(theWidth, theHeight);
		{
			Graphics aG(mShadowImage);
			aG.DrawImageCel(IMAGE_TROPHIES, Rect(0, 0, theWidth, theHeight), mTrophy);
		}

		ulong* aBits = mShadowImage->GetBits();
		for (int aPixel = 0; aPixel < mShadowImage->mWidth * mShadowImage->mHeight; aPixel++)
		{
			int anAlpha = (aBits[aPixel] >> 24) & 0xff;
			aBits[aPixel] = ToInt(anAlpha * SHADOW_ALPHA) << 24;
		}
		mShadowImage->BitsChanged();
	}

	Widget::Resize(0, 0, theWidth, theHeight);
}

// port: the original's destructor leaves the shape picture behind (a leak); this one deletes it
TrophyButton::~TrophyButton()
{
	delete mShadowImage;
}

// 0x45fb00
void TrophyButton::StartFlash(double theTime)
{
	mFlash = theTime;
	mFlashStart = theTime;
}

// 0x45fae0
void TrophyButton::UpdateF(float theFrac)
{
	mFlash = mFlash - theFrac * FLASH_SPEED;
}

// 0x45fd10: a trophy that has been won, and, added over itself, a light that is the mouse being over it, or the flash
void TrophyButton::Draw(Graphics* g)
{
	Graphics aG(*g);

	if (!mEarned)
	{
		aG.DrawImage(mShadowImage, 0, 0);
		return;
	}

	aG.DrawImageCel(IMAGE_TROPHIES, Rect(0, 0, mWidth, mHeight), mTrophy);
	if (mFlash > 0.0)
	{
		aG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		aG.SetColorizeImages(true);
		int aLight = ToInt(mFlash / mFlashStart * 255.0);
		aG.SetColor(Color(aLight, aLight, aLight));
	}
	else
	{
		if (!mIsOver)
			return;
		aG.SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	}
	aG.DrawImageCel(IMAGE_TROPHIES, Rect(0, 0, mWidth, mHeight), mTrophy);
}
