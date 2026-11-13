#include "TrophyAppearButton.h"

#include "Application.h"
#include "Creature.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

namespace
{

// The trophy falls in during the first unit of the time (it starts this far above its place), turning three turns and a half; after
// that it swells by this much of its size during the next
const double FALL_START = 0.1;
const double FALL_HEIGHT = 300.0;
const double FALL_TURN = 10.0;
const double SWELL_END = 0.9;
const double SWELL_SIZE = 0.75;

// The blue fades in from a tenth of the time to nine tenths of it
const double FADE_START = 0.1;
const double FADE_END = 0.9;

// Where the heading is
const int HEADING_Y = 0x28;

}

// 0x45f5e0
TrophyAppearButton::TrophyAppearButton(int theId, ButtonListener* theListener, int theTrophy) :
	TrophyPageButton(theId, theListener)
{
	mTrophy = theTrophy;
	mBlend = true;
	mHasAlpha = true;
	gApp->PlaySample(SOUND_STING_MEDAL_AWARDED);
}

// 0x45f590
void TrophyAppearButton::Start(int theTrophy, bool theBlend)
{
	mTime = 0.0;
	mTrophy = theTrophy;
	mClickProgress = 0.0;
	mHasAlpha = true;
	mBlend = theBlend;
	gApp->PlaySample(SOUND_STING_MEDAL_AWARDED);
}

// 0x45f6a0
void TrophyAppearButton::Draw(Graphics* g)
{
	Graphics aG(*g);

	int aCelWidth = IMAGE_TROPHIES->GetCelWidth();
	int aCelHeight = IMAGE_TROPHIES->GetCelHeight();
	int aHalfWidth = mWidth / 2;
	int aHalfHeight = mHeight / 2;

	double aFade = Ease(mTime <= 1.0 ? mTime : 1.0, FADE_START, FADE_END);
	DrawTrophyBackdrop(&aG, mTime, aFade, mBlend);

	if (mTime < 1.0)
	{
		// the trophy comes down from above, turning
		double aFall = Ease(mTime, FALL_START, 1.0);
		double aTurn = (1.0 - mTime) * FALL_TURN;
		int aY = ToInt((double) (aHalfHeight - aCelHeight / 2) - (1.0 - aFall) * FALL_HEIGHT);
		DrawCelRotated(&aG, IMAGE_TROPHIES, aHalfWidth - aCelWidth / 2, aY, mTrophy, 0, aTurn);
	}
	else
	{
		// it swells; the cel is a square, the original puts its top at half the width above the middle
		double aSwellTime = mTime - 1.0 < 1.0 ? mTime - 1.0 : 1.0;
		double aScale = Ease(aSwellTime, 0.0, SWELL_END) * SWELL_SIZE + 1.0;
		int aWidth = ToInt(aCelWidth * aScale);
		int aHeight = ToInt(aCelHeight * aScale);
		aG.DrawImageCel(IMAGE_TROPHIES, Rect(aHalfWidth - aWidth / 2, aHalfHeight - aWidth / 2, aWidth, aHeight), mTrophy);
	}

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

	aG.SetColor(Color(255, 255, 255, 255));
	aG.SetFont(FONT_PTBANANASPLIT24);
	aG.DrawString("You have been awarded a trophy!", gApp->mWidth / 2 - aG.StringWidth("You have been awarded a trophy!") / 2, HEADING_Y);

	if (gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		aG.DrawImage(IMAGE_TOUGH_COOKIE_STAMP, mWidth - IMAGE_TOUGH_COOKIE_STAMP->mWidth, mHeight - IMAGE_TOUGH_COOKIE_STAMP->mHeight);
}
