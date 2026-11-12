#include "GnomeFlying.h"

#include "Application.h"
#include "Coins.h"
#include "DrawUtil.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Hearts.h"
#include "Image.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Stats.h"

using namespace Sexy;

// 0x4398b0 (it draws the direction, then the height, then the speed)
GnomeFlying::GnomeFlying() :
	SimpleObject(0x80),
	mFrame(0.0),
	mSpeedX(0.0),
	mSpeedY(0.0),
	mUnusedInt(0),
	mRotation(0.0),
	mRotationSpeed(0.0),
	mHit(false),
	mLeft(false),
	mCount(0),
	mCalled(false),
	mHoldCount(0),
	mState(STATE_FLYING),
	mEngine(nullptr)
{
	int aSoundId;
	if (Rand() % 2 == 0)
	{
		// from the left, to the right: it starts 1.5 time units' flight before the edge
		int aHeightRand = Rand();
		mX = (double)(-IMAGE_GNOME_AVIATOR->GetCelWidth());
		mY = (double)(aHeightRand % 201 + 100);
		int aSpeedRand = Rand();
		mSpeedX = (double)(aSpeedRand % 251) + 800.0;
		mSpeedY = 0.0;
		mX = mX - mSpeedX * 1.5;
		aSoundId = SOUND_AVIATOR_ENGINE_L2R;
	}
	else
	{
		int aHeightRand = Rand();
		mX = (double)gApp->mWidth;
		mY = (double)(aHeightRand % 201 + 100);
		int aSpeedRand = Rand();
		mSpeedX = (double)(-(aSpeedRand % 251)) - 800.0;
		mSpeedY = 0.0;
		mX = mX - mSpeedX * 1.5;
		aSoundId = SOUND_AVIATOR_ENGINE_R2L;
	}

	mEngine = gApp->mSoundManager->GetSoundInstance(aSoundId);
	if (mEngine != nullptr)
		mEngine->Play(false, false);

	UpdateBounds();
}

// 0x4394d0
GnomeFlying::~GnomeFlying()
{
	if (mEngine != nullptr)
		mEngine->Release();
}

// 0x439b10
GnomeFlying* Sexy::MakeGnomeFlying(const std::string& theName)
{
	GnomeFlying* aFlying = new GnomeFlying();
	aFlying->SetName(theName);
	return aFlying;
}

// the rectangle of a cel at the plane's position (the same few lines end the constructor and the update)
void GnomeFlying::UpdateBounds()
{
	Image* anImage = IMAGE_GNOME_AVIATOR;
	mBounds.mX = ToInt(mX);
	mBounds.mY = ToInt(mY);
	mBounds.mWidth = anImage->mWidth / anImage->mNumCols;
	mBounds.mHeight = anImage->mHeight / anImage->mNumRows;
}

// 0x439b80
void GnomeFlying::Update(float theFrac)
{
	Object::Update(theFrac);
	double aStepX = (double)theFrac * 0.01 * mSpeedX;
	mX = aStepX + mX;
	double aStepY = (double)theFrac * 0.01 * mSpeedY;
	mY = aStepY + mY;
	UpdateBounds();

	// it has gone off the side it was flying to, or off the top or the bottom
	if ((0.0 < mSpeedX && (double)gApp->mWidth < mX) || (mSpeedX < 0.0 && mX < -(double)mBounds.mWidth) ||
		mY < -(double)mBounds.mHeight || (double)gApp->mHeight < mY)
	{
		if (!mHit)
		{
			if (!mLeft)
				mLeft = true;
		}
		else if (mCount < 10)
			return;

		if ((!mLeft || 19 < mCount) && mHoldCount < 1)
			Kill();
	}
}

// 0x43a110
void GnomeFlying::FixedUpdate()
{
	Object::FixedUpdate();

	if (mState == STATE_FLYING)
		mFrame = mFrame + 0.3;
	else if (mState == STATE_HIT && 4.0 < mFrame)
		mState = STATE_DONE;

	mRotation = mRotation + mRotationSpeed;
	mRotationSpeed = mRotationSpeed * 0.98;

	if (mTickCount % 20 == 0 && mHit && mCount < 10)
		DropItem();
	else if (mTickCount % 3 == 0 && mLeft && mCount < 20)
		TakeCoin();

	if (!mCalled && 20.0 < mX && mX < (double)((gApp->mWidth - mBounds.mWidth) - 20))
	{
		gApp->PlaySample(SOUND_VOICE_BOUNCING_YOOHOO);
		mCalled = true;
	}
}

// 0x439db0: the plane takes a coin out of the counter, which flies to it (and when the counter is empty it is done)
void GnomeFlying::TakeCoin()
{
	if (0 < GetStats()->GetInt(STAT_COIN_BALANCE))
	{
		mCount = mCount + 1;
		GetStats()->AddInt(STAT_COIN_BALANCE, -1);

		double aCounter[2];
		GetCoinsPos(aCounter);
		int aHalfHeight = IMAGE_GNOME_AVIATOR->GetCelHeight() / 2;
		int aHalfWidth = IMAGE_GNOME_AVIATOR->GetCelWidth() / 2;
		FloaterCoin* aCoin = MakeFloaterCoin(aCounter[0], aCounter[1], (double)aHalfWidth + mX, (double)aHalfHeight + mY, 0.0,
			0.0, 0, 0, 0, 0);
		aCoin->SetDuration(0.45);
		aCoin->SetKeepSize(true);
		return;
	}
	mCount = 20;
}

// 0x439ec0: the plane that has been hit lets something fall, a heart (that goes to the health row) in turn with a coin (that
// goes to the counter)
void GnomeFlying::DropItem()
{
	if (mCount % 2 == 0)
	{
		GetHearts()->ChangeHealth(HEALTH_GAIN, 0.125);

		double aEnd[2];
		GetHearts()->GetHealthEndPos(aEnd);
		MakeFloaterHeart((double)(IMAGE_GNOME_AVIATOR->GetCelWidth() / 2) + mX, (double)(IMAGE_GNOME_AVIATOR->GetCelHeight() / 2) + mY,
			aEnd[0], aEnd[1], 0.0);

		if (GetStats()->GetInt(STAT_HEARTS_COLLECTED) == 0)
			GetStats()->SetInt(STAT_LEVEL_OF_FIRST_HEART, gApp->mGameScreen->mLevel);
		GetStats()->AddInt(STAT_HEARTS_COLLECTED, 1);
		GetStats()->SetInt(STAT_LEVEL_OF_LAST_HEART, gApp->mGameScreen->mLevel);
	}
	else
	{
		GetCoins()->AddCoins(1);
		int aCoinWidth = IMAGE_COIN->GetCelWidth();
		int aCoinHeight = IMAGE_COIN->GetCelHeight();
		double aCounter[2];
		GetCoinsPos(aCounter);
		MakeFloaterCoin(mX - (double)(aCoinWidth / 2), mY - (double)(aCoinHeight / 2), aCounter[0], aCounter[1], 0.0, 0.0, 0, 0, 0,
			0);
		GetStats()->AddInt(STAT_COINS_COLLECTED, 1);
		GetStats()->SetInt(STAT_LEVEL_OF_LAST_COIN, gApp->mGameScreen->mLevel);
	}
	gApp->PlaySample(SOUND_COIN_SINGLE_PICKED);
	mCount = mCount + 1;
}

// 0x439530: a plane that faces left is the first picture, one that flies to the right the mirrored one; the tiny heart on it is
// the pilot's (it is where the cockpit is, which is on the other side in the mirrored picture)
void GnomeFlying::Draw(Graphics* g)
{
	Object::Draw(g);
	if (mDead)
		return;

	Graphics aG(*g);
	if (mRotation == 0.0 ||
		(0.0 <= mX && mX <= (double)(gApp->mWidth - IMAGE_GNOME_AVIATOR->GetCelWidth()) && 0.0 <= mY &&
		 mY <= (double)(gApp->mHeight - IMAGE_GNOME_AVIATOR->GetCelHeight())))
	{
		int aCol = (int)mFrame % 8;
		if (mSpeedX <= 0.0)
		{
			DrawCelRotatedLit(&aG, IMAGE_GNOME_AVIATOR, ToInt(mX), ToInt(mY), aCol, 0, mHover, mRotation);
			aG.DrawImage(IMAGE_TINYHEART, ToInt(mX + 63.0), ToInt(mY + 77.0));
		}
		else
		{
			DrawCelRotatedLit(&aG, IMAGE_GNOME_AVIATOR_MIRRORED, ToInt(mX), ToInt(mY), aCol, 0, mHover, mRotation);
			aG.DrawImage(IMAGE_TINYHEART, ToInt(mX + 162.0), ToInt(mY + 77.0));
		}
	}
}

// 0x439cf0: the mask of the hammer is put down at the point and a solid pixel of the plane's first cel under any point of it
// is a hit
bool GnomeFlying::Hit(int theX, int theY, const std::vector<Point>& theMask, int theWidth, int theHeight)
{
	int aLeft = mBounds.mX - theWidth;
	int aTop = mBounds.mY - theHeight;
	if (aLeft <= theX && theX < mBounds.mWidth + theWidth * 2 + aLeft && aTop <= theY &&
		theY < mBounds.mHeight + theHeight * 2 + aTop)
	{
		double aPos[2] = { mX, mY };
		int aXY[2] = { theX, theY };
		if (MaskHitsImage(aPos, aXY, theMask, IMAGE_GNOME_AVIATOR, 0, 0))
			return true;
	}
	return false;
}

// 0x43a240: the plane falls, and the engine stops
void GnomeFlying::Strike(double theX, double theY)
{
	mSpeedY = 250.0;
	mHoldCount = mHoldCount - 1;
	if (mState == STATE_FLYING)
	{
		mState = STATE_HIT;
		mFrame = 0.0;
	}
	mHit = true;
	if (mEngine != nullptr)
		mEngine->Stop();
	gApp->PlaySample(SOUND_HAMMER_HIT_METAL1);
	gApp->PlaySample(SOUND_VOICE_BOUNCING_AI);
	gApp->PlaySample(SOUND_AVIATOR_HIT);
	MakeFloater(FLOATER_STRIKE_BIG, theX + 45.0, theY + 20.0, nullptr, 0, "");
}

// 0x439770
void Sexy::DrawFlyingIntro(Graphics* g, double theX, double theY, int theCel)
{
	Rect aSrcRect = IMAGE_GNOME_AVIATOR->GetCelRect(theCel, 0);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->DrawImage(IMAGE_GNOME_AVIATOR, ToInt(theX - 109.0), ToInt(theY - 138.0), aSrcRect);
}

// 0x43a370
void GnomeFlying::SaveState(FlyingState& theState) const
{
	theState.mName = mName;
	theState.mActive = mActive;
	theState.mFrame = mFrame;
	theState.mSpeedX = mSpeedX;
	theState.mSpeedY = mSpeedY;
	theState.mUnusedInt = mUnusedInt;
	theState.mBounds = mBounds;
	theState.mX = mX;
	theState.mY = mY;
	theState.mRotation = mRotation;
	theState.mRotationSpeed = mRotationSpeed;
	theState.mHit = mHit;
	theState.mLeft = mLeft;
	theState.mCount = mCount;
	theState.mCalled = mCalled;
	theState.mHoldCount = mHoldCount;
	theState.mState = mState;
}

// 0x4397e0
void GnomeFlying::LoadState(const FlyingState& theState)
{
	mFrame = theState.mFrame;
	mSpeedX = theState.mSpeedX;
	mSpeedY = theState.mSpeedY;
	mUnusedInt = theState.mUnusedInt;
	mBounds = theState.mBounds;
	mX = theState.mX;
	mY = theState.mY;
	mRotation = theState.mRotation;
	mRotationSpeed = theState.mRotationSpeed;
	mHit = theState.mHit;
	mLeft = theState.mLeft;
	mCount = theState.mCount;
	mCalled = theState.mCalled;
	mHoldCount = theState.mHoldCount;
	mState = theState.mState;
	mActive = theState.mActive;
}
