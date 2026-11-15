#include "ResurrectButton.h"

#include "Color.h"
#include "Common.h"
#include "DrawUtil.h"
#include "Font.h"
#include "Graphics.h"
#include "Hearts.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

namespace
{

// "Start on Level n": the line's place in the button, and how much lower it is while the button is pressed
const int LABEL_Y = 0xd3;
const int LABEL_PRESSED_DROP = 5;

// How much the card fades in and out by an update
const double FADE_IN_SPEED = 8.0;
const double FADE_OUT_SPEED = 10.0;
const double FADE_MAX = 255.0;

// The card: the shift of its picture, then the place of the hammer (and the flame beside it), the coins and the hearts
const int CARD_SHIFT_X = 3;
const int HAMMER_Y = 0x22;
const int FLAME_Y = 0x1b;
const int COIN_Y = 0x3e;
const int COIN_TEXT_Y = 0x4a;
const int COIN_GAP = 5;
const int HEARTS_Y = 0xc;
const int HEARTS_TICK = 0x19;
const double HEARTS_MIN_HEALTH = 0.2;

// 0x57 on in the exe's table of pictures: the hammers, by level
Image* HammerImage(int theLevel)
{
	switch (theLevel)
	{
	case 0: return IMAGE_TINYHAMMER1;
	case 1: return IMAGE_TINYHAMMER2;
	case 2: return IMAGE_TINYHAMMER3;
	default: return IMAGE_TINYHAMMER4;
	}
}

}

// 0x45d040
ResurrectButton::ResurrectButton(Image* theComponentImage, int theId, ButtonListener* theListener) :
	NcButton(theComponentImage, theId, theListener)
{
	mHealth = 0.0;
	mLevel = 0;
	mHearts = 0;
	mFade = 0.0;
	mCoins = 0;
	mHammerLevel = 0;
	mOnFire = false;
}

// 0x45cff0
void ResurrectButton::SetShown(int theLevel, double theHealth, int theHearts, int theCoins, int theHammerLevel, bool theOnFire)
{
	mHealth = theHealth;
	mLevel = theLevel;
	mHearts = theHearts;
	mCoins = theCoins;
	mHammerLevel = theHammerLevel;
	mOnFire = theOnFire;
}

// 0x45cf60
void ResurrectButton::Update()
{
	ButtonWidget::Update();

	double aFade;
	if (!mIsOver && !mIsDown)
	{
		aFade = mFade - FADE_OUT_SPEED;
		if (aFade < 0.0)
			aFade = 0.0;
	}
	else
	{
		aFade = mFade + FADE_IN_SPEED;
		if (FADE_MAX < aFade)
			aFade = FADE_MAX;
	}

	if (aFade != mFade)
	{
		MarkDirty();
		mFade = aFade;
	}
}

// 0x45d0c0: the photo and the line under it (the dialog button's own drawing, not the NcButton's), then the card
void ResurrectButton::Draw(Graphics* g)
{
	DialogButton::Draw(g);

	g->SetFont(FONT_EUROFURENCE14);
	g->SetColor(Color(0, 0, 0, 255));
	int aLevel = mLevel;
	if (aLevel == 0)
		aLevel = 1;
	int aDrop = (mIsDown && mIsOver) ? LABEL_PRESSED_DROP : 0;
	WriteCenteredLine(g, LABEL_Y + aDrop, StrFormat("Start on Level %d", aLevel));

	if (!(0.0 < mFade))
		return;

	int anAlpha = ToInt(mFade);
	g->SetColor(Color(255, 255, 255, anAlpha));
	g->SetColorizeImages(true);
	g->DrawImage(IMAGE_RESURRECTPOPUP, mWidth / 2 - IMAGE_RESURRECTPOPUP->mWidth / 2 + CARD_SHIFT_X, 0);

	// the hammer, and the flame at its side if it is on fire, together in the middle
	std::string aCoins = StrFormat("%d", mCoins);
	int aFlameWidth = mOnFire ? IMAGE_TINYFLAME->mWidth : 0;
	Image* aHammer = HammerImage(mHammerLevel);
	int aHammerX = mWidth / 2 - (aHammer->mWidth + aFlameWidth) / 2;
	g->DrawImage(aHammer, aHammerX, HAMMER_Y);
	if (mOnFire)
		g->DrawImage(IMAGE_TINYFLAME, aHammerX + aHammer->mWidth, FLAME_Y);

	// the coin and the number of coins, together in the middle (the number is measured in the font the line above was written in)
	int aCoinsWidth = g->GetFont()->StringWidth(aCoins) + IMAGE_TINYCOIN->mWidth + COIN_GAP;
	int aCoinX = mWidth / 2 - aCoinsWidth / 2;
	g->DrawImage(IMAGE_TINYCOIN, aCoinX, COIN_Y);
	g->SetFont(FONT_ARIAL10B);
	g->SetColor(Color(0, 0, 0, anAlpha));
	g->DrawString(aCoins, aCoinX + IMAGE_TINYCOIN->mWidth + COIN_GAP, COIN_TEXT_Y);

	// the hearts of the point, at least a fifth of one full
	g->SetColor(Color(255, 255, 255, anAlpha));
	int aHeartCelWidth = IMAGE_TINYHEARTCOMPONENTS->GetCelWidth();
	int aHeartsX = mWidth / 2 - (aHeartCelWidth * mHearts) / 2;
	double aHealth = HEARTS_MIN_HEALTH <= mHealth ? mHealth : HEARTS_MIN_HEALTH;
	DrawHeartRow(g, mHearts, aHealth, aHeartsX, HEARTS_Y, HEARTS_TICK, IMAGE_TINYHEARTCOMPONENTS, 0);
}
