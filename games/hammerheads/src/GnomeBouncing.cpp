#include "GnomeBouncing.h"

#include "Application.h"
#include "Coins.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Image.h"
#include "Res.h"
#include "Stats.h"

using namespace Sexy;

// 0x4365d0 (it draws one number, for the side it comes from)
GnomeBouncing::GnomeBouncing() :
	SimpleObject(0x80),
	mFrame(0.0),
	mSpeedX(0.0),
	mSpeedY(0.0),
	mFloorY(570),
	mUnusedInt(0),
	mRotation(0.0),
	mRotationSpeed(0.0),
	mHit(false),
	mCount(0),
	mCalled(false),
	mHoldCount(0)
{
	if (Rand() % 2 == 0)
	{
		mX = (double)(-(IMAGE_GNOME_BOUNCING->mWidth / IMAGE_GNOME_BOUNCING->mNumCols));
		mY = 100.0;
		mSpeedX = 350.0;
	}
	else
	{
		mX = (double)gApp->mWidth;
		mY = 100.0;
		mSpeedX = -350.0;
	}
	mSpeedY = 0.0;
	UpdateBounds();
}

// 0x436270
GnomeBouncing::~GnomeBouncing()
{
}

// 0x436770
GnomeBouncing* Sexy::MakeGnomeBouncing(const std::string& theName)
{
	GnomeBouncing* aBouncing = new GnomeBouncing();
	aBouncing->SetName(theName);
	return aBouncing;
}

// the rectangle of a cel at the jester's position (the same few lines end the constructor and the update)
void GnomeBouncing::UpdateBounds()
{
	Image* anImage = IMAGE_GNOME_BOUNCING;
	mBounds.mX = ToInt(mX);
	mBounds.mY = ToInt(mY);
	mBounds.mWidth = anImage->mWidth / anImage->mNumCols;
	mBounds.mHeight = anImage->mHeight / anImage->mNumRows;
}

// 0x436280: the cel for the jester's height: cel 5 when its bottom is 200 or more above the floor; below that, going up the
// cel is the height over 40 (rounded down), going down it is 15 less the height over 50 less 6
int GnomeBouncing::GetCel() const
{
	int aCelHeight = IMAGE_GNOME_BOUNCING->mHeight / IMAGE_GNOME_BOUNCING->mNumRows;
	double aAboveFloor = (double)mFloorY - ((double)aCelHeight + mY);
	if (0.0 <= aAboveFloor)
	{
		if (200.0 <= aAboveFloor)
			return 5;
	}
	else
		aAboveFloor = 0.0;

	if (mSpeedY <= 0.0)
		return ToInt(aAboveFloor * 0.025);
	return 15 - ToInt(aAboveFloor * 0.02 + 6.0);
}

// 0x4367e0
void GnomeBouncing::Update(float theFrac)
{
	Object::Update(theFrac);
	double aStepX = ((double)theFrac * mSpeedX) * 0.01;
	mX = aStepX + mX;
	double aStepY = ((double)theFrac * mSpeedY) * 0.01;
	mY = aStepY + mY;
	UpdateBounds();

	// the feet are below the floor on the way down: it bounces, and the bounce is heard if the jester is on the screen
	int aCelHeight = IMAGE_GNOME_BOUNCING->mHeight / IMAGE_GNOME_BOUNCING->mNumRows;
	if ((double)mFloorY < (double)aCelHeight + mY && 0.0 < mSpeedY)
	{
		mSpeedY = mSpeedY * -1.0;
		if (-(double)mBounds.mWidth <= mX && mX < (double)gApp->mWidth && -(double)mBounds.mHeight <= mY &&
			mY < (double)gApp->mHeight)
			gApp->PlaySample(SOUND_BOUNCING_JESTER_BOUNCE);
	}

	// it has gone off the side it was hopping to, or off the top or the bottom
	if (((0.0 < mSpeedX && (double)gApp->mWidth < mX) || (mSpeedX < 0.0 && mX < -(double)mBounds.mWidth) ||
		 mY < -(double)mBounds.mHeight || (double)gApp->mHeight < mY) &&
		(!mHit || 19 < mCount) && mHoldCount < 1)
		Kill();
}

// 0x436be0
void GnomeBouncing::FixedUpdate()
{
	Object::FixedUpdate();

	mFrame = mFrame + 0.3;
	mSpeedY = mSpeedY + 18.0;
	mRotation = mRotation + mRotationSpeed;
	mRotationSpeed = mRotationSpeed * 0.98;

	if (mTickCount % 8 == 0 && mHit && mCount < 20)
		DropCoin();

	if (!mCalled && 20.0 < mX && mX < (double)((gApp->mWidth - mBounds.mWidth) - 20))
	{
		gApp->PlaySample(SOUND_VOICE_BOUNCING_YOOHOO);
		mCalled = true;
	}
}

// 0x436ab0: a coin falls out of the jester into the counter
void GnomeBouncing::DropCoin()
{
	GetCoins()->AddCoins(1);
	int aCoinWidth = IMAGE_COIN->GetCelWidth();
	int aCoinHeight = IMAGE_COIN->GetCelHeight();
	double aCounter[2];
	GetCoinsPos(aCounter);
	MakeFloaterCoin(mX - (double)(aCoinWidth / 2), mY - (double)(aCoinHeight / 2), aCounter[0], aCounter[1], 0.0, 0.0, 0, 0, 0, 0);
	mCount = mCount + 1;
	GetStats()->AddInt(STAT_COINS_COLLECTED, 1);
	GetStats()->SetInt(STAT_LEVEL_OF_LAST_COIN, gApp->mGameScreen->mLevel);
	gApp->PlaySample(SOUND_COIN_SINGLE_PICKED);
}

// 0x436310: the tiny heart on its chest is drawn as the aviator's is
void GnomeBouncing::Draw(Graphics* g)
{
	Object::Draw(g);
	if (mDead)
		return;

	Graphics aG(*g);
	int aCel = GetCel();
	if (mRotation == 0.0 ||
		(0.0 <= mX && mX <= (double)(gApp->mWidth - IMAGE_GNOME_BOUNCING->GetCelWidth()) && 0.0 <= mY &&
		 mY <= (double)(gApp->mHeight - IMAGE_GNOME_BOUNCING->GetCelHeight())))
	{
		DrawCelRotatedLit(&aG, IMAGE_GNOME_BOUNCING, ToInt(mX), ToInt(mY), aCel, 0, mHover, mRotation);
		aG.DrawImage(IMAGE_TINYHEART, ToInt(mX + 15.0), ToInt(mY + 127.0));
	}
}

// 0x4369f0: a solid pixel of the cel that is being shown, under any point of the hammer's mask
bool GnomeBouncing::Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight)
{
	int aLeft = mBounds.mX - theWidth;
	int aTop = mBounds.mY - theHeight;
	if (aLeft <= theX && theX < mBounds.mWidth + theWidth * 2 + aLeft && aTop <= theY &&
		theY < mBounds.mHeight + theHeight * 2 + aTop)
	{
		int aCel = GetCel();
		double aPos[2] = { mX, mY };
		int aXY[2] = { theX, theY };
		if (MaskHitsImage(aPos, aXY, theMask, IMAGE_GNOME_BOUNCING, 0, aCel))
			return true;
	}
	return false;
}

// 0x436cd0: the jester is thrown up and goes twice as fast, to either side, and spins to either side
void GnomeBouncing::Strike(double theX, double theY)
{
	mSpeedY = -700.0;
	mHoldCount = mHoldCount - 1;
	mSpeedX = mSpeedX + mSpeedX;
	if (Rand() % 2 == 0)
		mSpeedX = mSpeedX * -1.0;
	mRotationSpeed = (double)Rand() * 4.656612875245797e-10 * 0.05 + 0.1;
	if (Rand() % 2 == 0)
		mRotationSpeed = mRotationSpeed * -1.0;
	mHit = true;
	gApp->PlaySample(SOUND_HAMMER_HIT_METAL1);
	gApp->PlaySample(SOUND_VOICE_BOUNCING_AI);
	MakeFloater(FLOATER_STRIKE, theX + 45.0, theY + 20.0, nullptr, 0, "");
}

// 0x436e40
void GnomeBouncing::SaveState(BouncingState& theState) const
{
	theState.mName = mName;
	theState.mActive = mActive;
	theState.mFrame = mFrame;
	theState.mSpeedX = mSpeedX;
	theState.mSpeedY = mSpeedY;
	theState.mFloorY = mFloorY;
	theState.mUnusedInt = mUnusedInt;
	theState.mBounds = mBounds;
	theState.mX = mX;
	theState.mY = mY;
	theState.mRotation = mRotation;
	theState.mRotationSpeed = mRotationSpeed;
	theState.mHit = mHit;
	theState.mCount = mCount;
	theState.mCalled = mCalled;
	theState.mHoldCount = mHoldCount;
}

// 0x436510
void GnomeBouncing::LoadState(const BouncingState& theState)
{
	mFrame = theState.mFrame;
	mSpeedX = theState.mSpeedX;
	mSpeedY = theState.mSpeedY;
	mFloorY = theState.mFloorY;
	mUnusedInt = theState.mUnusedInt;
	mBounds = theState.mBounds;
	mX = theState.mX;
	mY = theState.mY;
	mRotation = theState.mRotation;
	mRotationSpeed = theState.mRotationSpeed;
	mHit = theState.mHit;
	mCount = theState.mCount;
	mCalled = theState.mCalled;
	mHoldCount = theState.mHoldCount;
	mActive = theState.mActive;
}

// 0x4364a0
void Sexy::DrawBouncingIntro(Graphics* g, double theX, double theY, int theCel)
{
	Rect aSrcRect = IMAGE_GNOME_BOUNCING->GetCelRect(theCel, 0);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->DrawImage(IMAGE_GNOME_BOUNCING, ToInt(theX - 49.0), ToInt(theY - 191.0), aSrcRect);
}
