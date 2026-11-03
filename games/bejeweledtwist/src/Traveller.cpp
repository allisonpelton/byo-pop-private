// Sexy::Traveller and Sexy::SmokeTraveller (Traveller.h).
#include "Traveller.h"

#include "Board.h"
#include "EffectsManager.h"
#include "Profile.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

Traveller::Traveller()
{
	mImage = IMAGE_EMBER;
}

SmokeTraveller::SmokeTraveller(const Color& theColor, int theFromX, int theFromY, int theToX, int theToY,
	int thePoints, bool theSound)
{
	mSound = theSound;
	mAdditive = true;
	if (gApp->mProfile->mGamesPlayed == 0) {
		// In the first game the smoke hurries once the good twists (stat 0x21 less the bad ones, 0x22) pass 13
		float aHurry = (float)((gApp->mUnknownC74[0x21] - gApp->mUnknownC74[0x22]) - 13) * 0.0625f;
		float aClamped = aHurry;
		if (aHurry <= 0.0f)
			aClamped = 0.0f;
		float aValue = 0.0f;
		if (1.0f <= aClamped)
			aValue = 1.0f;
		else if (0.0f < aHurry)
			aValue = aHurry;
		mHurry = aValue;
	} else {
		mHurry = 1.0f;
	}
	mColor = theColor;
	mFromX = (float)theFromX;
	mFromY = (float)theFromY;
	mToX = (float)theToX;
	mWanderY = mFromY;
	mToY = (float)theToY;
	mWanderDY = 0;
	mProgress = 0;
	mPoints = thePoints;
	mScale = 1.0f;
	mNoPoints = false;
	mBurst = true;
}

SmokeTraveller::~SmokeTraveller()
{
	if (!mNoPoints) {
		gBoard->SetScore(gBoard->mUnknown168 + mPoints, false);
		gBoard->AddChainStep();
	}
	if (mBurst) {
		EffectsManager* aManager = gBoard->mFXManager;
		int aLeft = 30;
		do {
			Effect* anEffect = aManager->AllocEffect(12);
			anEffect->mX = mToX;
			anEffect->mY = mToY;
			anEffect->mColor = Color::White;
			anEffect->mDAlpha = -0.03f;
			anEffect->mDX = (float)RandSigned();
			anEffect->mDY = (float)RandSigned();
			anEffect->mGravity = 0.015f;
			aManager->AddEffect(anEffect);
			aLeft--;
		} while (aLeft != 0);
	}
}

void SmokeTraveller::Update()
{
	if (0.0f < mDelay) {
		mDelay = (float)((double)mDelay - 0.009999999776482582);
		return;
	}
	float anEnd = (float)(((double)mHurry * 0.6499999761581421 + 0.3499999940395355) * (double)mStep + (double)mProgress);
	if (!(1.0f >= anEnd))
		anEnd = 1.0f;
	while (mProgress < anEnd) {
		float aNext = (float)(((double)mHurry * 0.6499999761581421 + 0.3499999940395355) * 0.004999999888241291 +
			(double)mProgress);
		mProgress = aNext;
		if (!(anEnd > aNext))
			mProgress = anEnd;
		EmitSmoke();
		if (std::fabs(mFromY - mWanderY) < 1.0f)
			mWanderY = (float)(RandSigned() * 50.0 + (double)mWanderY);
		float aPull = (float)(((double)mHurry * 0.8999999761581421 + 0.10000000149011612) * 0.10000000149011612);
		float aY = mWanderDY + mFromY;
		mFromY = aY;
		if (mWanderY <= aY)
			mWanderDY = mWanderDY - aPull;
		else
			mWanderDY = mWanderDY + aPull;
	}
	mProgress = anEnd;
	if (1.0f == anEnd) {
		if (mSound)
			gApp->PlaySoundAt(SOUND_CHAINPARTICLE1, Ftol(mX), 0.0f, 0.0f);
		mDone = true;
	}
}

void SmokeTraveller::EmitSmoke()
{
	EffectsManager* aManager = gBoard->mFXManager;
	Effect* anEffect = aManager->AllocEffect(0);
	anEffect->mOverlay = gBoard->mUnknown268C == 1.0f;
	anEffect->mX = (mToX - mFromX) * mProgress + mFromX;
	anEffect->mY = (mToY - mFromY) * mProgress + mFromY;
	anEffect->mAngle = (float)(RandSigned() * 3.1415927410125732);
	anEffect->mDAngle = (float)(RandSigned() * 0.009999999776482582);
	anEffect->mImage = IMAGE_FX_STEAM;
	anEffect->mColor = mColor;
	anEffect->mScale = (float)((double)mScale * 0.25);
	float aRand = (float)RandSigned();
	anEffect->mDScale = (float)((double)std::fabs(aRand) * 0.019999999552965164);
	aRand = (float)RandSigned();
	anEffect->mGravity = (float)(-(double)std::fabs(aRand) * 0.004999999888241291);
	anEffect->mIsAdditive = mAdditive;
	aRand = (float)RandSigned();
	float aFade = (float)((double)std::fabs(aRand) * 0.20000000298023224);
	if (!((double)aFade >= 0.08500000089406967))
		aFade = 0.085f;
	anEffect->mDAlpha = -aFade;
	aManager->AddEffect(anEffect);
	anEffect->mAlpha = (float)((1.0 - (double)mHurry * 0.5) * (double)anEffect->mAlpha);
	anEffect->mDAlpha = (float)(((double)mHurry * 0.5 + 0.5) * (double)anEffect->mDAlpha);
}
