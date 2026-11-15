#include "TrophyCloseUpButton.h"

#include "Application.h"
#include "Creature.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

namespace
{

// The blue fades in from a tenth of the time to nine tenths of it
const double FADE_START = 0.1;
const double FADE_END = 0.9;

// The trophy grows with the fade, from half its size to one and three quarters of it
const double GROW_RATE = 1.25;
const double GROW_START = 0.5;

}

// 0x4600a0
TrophyCloseUpButton::TrophyCloseUpButton(int theId, ButtonListener* theListener) :
	TrophyPageButton(theId, theListener)
{
	mFromX = 0;
	mFromY = 0;
	mHasAlpha = true;
}

// 0x460050
void TrophyCloseUpButton::Setup(int theTrophy, int theFromX, int theFromY)
{
	mTrophy = theTrophy;
	mFromX = theFromX;
	mFromY = theFromY;
}

// 0x460080
void TrophyCloseUpButton::Restart()
{
	mHasAlpha = true;
	mTime = 0.0;
	mClickProgress = 0.0;
}

// 0x460300
void TrophyCloseUpButton::Draw(Graphics* g)
{
	Graphics aG(*g);

	double aFade = Ease(mTime <= 1.0 ? mTime : 1.0, FADE_START, FADE_END);
	DrawTrophyBackdrop(&aG, mTime, aFade, true);

	// the trophy grows and moves from the shelf to the middle of the page
	double aScale = aFade * GROW_RATE + GROW_START;
	int aWidth = ToInt(IMAGE_TROPHIES->GetCelWidth() * aScale);
	int aHeight = ToInt(IMAGE_TROPHIES->GetCelHeight() * aScale);
	int aX = ToInt(((double) (mWidth - aWidth) * 0.5 - (double) mFromX) * aFade + (double) mFromX);
	int aY = ToInt(((double) (mHeight - aHeight) * 0.5 - (double) mFromY) * aFade + (double) mFromY);
	aG.DrawImageCel(IMAGE_TROPHIES, Rect(aX, aY, aWidth, aHeight), mTrophy);

	int anOffsetX = 0;
	int anOffsetY = 0;
	if (IsButtonDown() && mClickProgress == 1.0)
	{
		anOffsetX = 3;
		anOffsetY = 2;
	}
	aG.SetColorizeImages(true);
	aG.SetColor(Color(255, 255, 255, ToInt(aFade * 255.0)));
	DrawTrophyInfo(&aG, anOffsetX, anOffsetY);
	aG.SetColorizeImages(false);
}
