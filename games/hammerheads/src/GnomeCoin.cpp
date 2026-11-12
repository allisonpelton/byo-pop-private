#include "GnomeCoin.h"

#include "Application.h"
#include "ChallengeMeter.h"
#include "Coins.h"
#include "Cubby.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Gameplay.h"
#include "Graphics.h"
#include "Res.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x438a30
GnomeCoin::GnomeCoin(int theKind) :
	Creature(theKind),
	mFrame(0.0),
	mFrameSpeed(0.55),
	mUnusedInt(0),
	mUnusedDouble(0.0),
	mDrawX(0.0),
	mDrawY(0.0),
	mCoinKind(theKind),
	mHintShown(false)
{
	// in a challenge the coin sits on the platform and makes no sound; elsewhere it jumps up with one
	if (gApp->IsChallengeOpen())
	{
		mHeight = 0.0;
		return;
	}
	gApp->PlaySample(mCoinKind == KIND_COIN ? SOUND_COIN_SINGLE_APPEAR : SOUND_COIN_THREE_APPEAR);
}

// 0x438d30 (it looks the hint's floater up and calls a function that does nothing)
GnomeCoin::~GnomeCoin()
{
}

// 0x438e20
void GnomeCoin::Update(const double thePos[2])
{
	Creature::Update(thePos);

	mDrawX = thePos[0] - mAnchorX;
	mDrawY = (thePos[1] - mAnchorY) - mHeight;
	mFrame = mFrame + mFrameSpeed;
	double aSpeed = mFrameSpeed - 0.001;
	if (aSpeed < 0.15)
		aSpeed = 0.15;
	mFrameSpeed = aSpeed;

	// the coin that appears first in the first level puts up the hint to pick coins up
	if (mCubby != nullptr && gApp->mGameScreen->mLevel == 1 && !mHintShown && !GetGameplay()->mFlagCC &&
		gApp->mGameMode != GAME_MODE_TOUGH_COOKIE)
	{
		if (mCubby->mName == "class Sexy::Cubby_00001")
			mHintShown = true;
		if (mHintShown)
		{
			Floater* aHint = MakeFloater(FLOATER_HINT_PICK, 400.0, 216.0, nullptr, 0, "");
			mHintName = aHint->mName;
			GetGameplay()->mFlagCC = true;
		}
	}
}

// 0x4382c0: a single coin, or three of them
void GnomeCoin::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	Graphics aG(*g);
	double aTopLeftX = theX - mAnchorX;
	double aTopLeftY = (theY - mAnchorY) - mHeight;
	int aCol = (int)mFrame % 20;

	if (mCoinKind == KIND_COIN)
	{
		DrawShadow(&aG, theX, theY);
		int aFlash = FlashAlpha(theFlashTime);
		DrawBody(&aG, aTopLeftX, aTopLeftY, aCol, 0, aFlash, 1.0);
	}
	else if (mCoinKind == KIND_BIG_COIN)
	{
		DrawShadow(&aG, theX - 20.0, theY - 5.0);
		DrawShadow(&aG, theX + 20.0, theY - 5.0);
		DrawShadow(&aG, theX, theY + 10.0);
		DrawBody(&aG, aTopLeftX - 20.0, aTopLeftY - 5.0, aCol, 0, FlashAlpha(theFlashTime), 1.0);
		DrawBody(&aG, aTopLeftX + 20.0, aTopLeftY - 5.0, aCol, 0, FlashAlpha(theFlashTime), 1.0);
		DrawBody(&aG, aTopLeftX, aTopLeftY + 10.0, aCol, 0, FlashAlpha(theFlashTime), 1.0);
	}
}

// 0x4384f0: three coins are three places to hit, one above and two below
bool GnomeCoin::PixelHit(const double thePos[2], int theX, int theY)
{
	if (mCoinKind == KIND_COIN)
		return Creature::PixelHit(thePos, theX, theY);
	if (mCoinKind == KIND_BIG_COIN)
		return Creature::PixelHit(thePos, theX, theY - 10) || Creature::PixelHit(thePos, theX + 20, theY + 5) ||
			Creature::PixelHit(thePos, theX - 20, theY + 5);
	return false;
}

// 0x4388f0
bool GnomeCoin::MaskHit(const double thePos[2], int theX, int theY, const std::vector<Point>& theMask, int theRectX, int theRectY,
	int theRectW, int theRectH)
{
	if (mCoinKind == KIND_COIN)
		return Creature::MaskHit(thePos, theX, theY, theMask, theRectX, theRectY, theRectW, theRectH);
	if (mCoinKind == KIND_BIG_COIN)
		return Creature::MaskHit(thePos, theX, theY - 10, theMask, theRectX, theRectY - 10, theRectW, theRectH) ||
			Creature::MaskHit(thePos, theX + 20, theY + 5, theMask, theRectX + 20, theRectY + 5, theRectW, theRectH) ||
			Creature::MaskHit(thePos, theX - 20, theY + 5, theMask, theRectX - 20, theRectY + 5, theRectW, theRectH);
	return false;
}

// 0x4385d0
bool GnomeCoin::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit)
	{
		Rect aClip = mCubby->GetEffectRect();
		GetStats()->SetInt(STAT_LEVEL_OF_LAST_COIN, gApp->mGameScreen->mLevel);

		if (mCoinKind == KIND_COIN)
		{
			double aCounter[2];
			GetCoinsPos(aCounter);
			GetCoins()->AddCoins(1);
			GetStats()->AddInt(STAT_COINS_COLLECTED, 1);
			MakeFloaterCoin(mDrawX, mDrawY, aCounter[0], aCounter[1], mFrame, mExposed, aClip.mX, aClip.mY, aClip.mWidth,
				aClip.mHeight);
			gApp->PlaySample(SOUND_COIN_SINGLE_PICKED);
			return true;
		}
		if (mCoinKind == KIND_BIG_COIN)
		{
			// in a challenge the coins count towards the challenge instead of the player's coins
			double aDest[2];
			if (gApp->IsChallengeOpen())
				GetNextCellPos(aDest);
			else
			{
				GetCoinsPos(aDest);
				GetCoins()->AddCoins(3);
			}
			GetStats()->AddInt(STAT_COINS_COLLECTED, 3);
			MakeFloaterCoin(mDrawX - 20.0, mDrawY - 5.0, aDest[0], aDest[1], mFrame, mExposed, aClip.mX, aClip.mY,
				aClip.mWidth, aClip.mHeight);
			MakeFloaterCoin(mDrawX + 20.0, mDrawY - 5.0, aDest[0], aDest[1], mFrame, mExposed, aClip.mX, aClip.mY,
				aClip.mWidth, aClip.mHeight);
			MakeFloaterCoin(mDrawX, mDrawY + 10.0, aDest[0], aDest[1], mFrame, mExposed, aClip.mX, aClip.mY,
				aClip.mWidth, aClip.mHeight);
			gApp->PlaySample(SOUND_COIN_THREE_PICKED);
		}
	}
	return aHit;
}

// 0x438580: a coin that freezes is moved off the frames where it shows only its edge
void GnomeCoin::SetFrozen(bool theFrozen)
{
	Creature::SetFrozen(theFrozen);
	if (theFrozen)
	{
		int aCol = (int)mFrame % 20;
		if (aCol == 5 || aCol == 15)
			mFrame = mFrame + 1.0;
	}
}

// 0x438b30
std::string GnomeCoin::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mUnusedInt);
	aList.Add(mUnusedDouble);
	aList.Add(mDrawX);
	aList.Add(mDrawY);
	aList.AddInt(mCoinKind);
	aList.AddBool(mHintShown);
	return aList.ToString();
}

// 0x438c40
void GnomeCoin::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mUnusedInt = aList.NextInt();
	mUnusedDouble = aList.NextDouble();
	mDrawX = aList.NextDouble();
	mDrawY = aList.NextDouble();
	mCoinKind = aList.NextInt();
	mHintShown = aList.NextBool();
}
