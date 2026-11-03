#include "Character.h"
#include "PixelusApp.h"
#include "Widget.h"

using namespace Sexy;

namespace
{
	struct Step
	{
		int		mPose;			// -1 ends the sequence
		int		mTime;
	};

	// 0x5105e0: five sequences of at most 50 steps, 8 bytes each
	const Step gAnimations[Character::NUM_ANIMATIONS][Character::ANIMATION_ENTRIES] =
	{
		{ { 2, 10 }, { 0, 30 }, { 2, 10 }, { 3, 100 }, { -1, 0 } },
		{ { 2, 10 }, { 1, 30 }, { -1, 0 } },
		{ { 3, 20 }, { 1, 30 }, { -1, 0 } },
		{ { 2, 8 }, { 3, 20 }, { 2, 8 }, { 1, 100 }, { -1, 0 } },
		{ { 0, 30 }, { 1, 30 }, { 2, 10 }, { -1, 0 } }
	};
}

// 0x42c4a0: mode 0 and 1 name the tutorial's first two hints; mode 2 picks one of the six that follow at random. Any
// other mode (the temple passes the picture of the map behind it) leaves the first one as it was in memory: the original
// reads it uninitialised, the port starts it at the first of the random ones.
SpeechBubble::SpeechBubble(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode)
{
	mApp = theApp;
	mGod = theGod;
	mOwner = theOwner;
	mTip = 3;
	if (theMode == 0)
		mTip = 1;
	else if (theMode == 1)
		mTip = 2;
	else if (theMode == 2)
		mTip = theApp->AppRandNext() % 6 + 3;
	int aRand = theApp->AppRandNext();
	mNextTip = aRand % 6 + 3;
	if (mNextTip == mTip)
		mNextTip = (aRand % 6 + 1) % 6 + 3;
	mPhase = 0;
	mPeriod = 1000;
	mDelay = 100;
	mAlpha = -1;
	mLeaving = false;
}

// 0x42c590
void SpeechBubble::Update()
{
	if (mDelay != 0)
	{
		mDelay--;
		return;
	}
	if (mTip == -1)
		return;

	mPhase += mLeaving ? 2 : 1;
	if (mPeriod < mPhase)
	{
		if (!mLeaving)
		{
			mTip = mNextTip;
			int aRand = mApp->AppRandNext();
			mNextTip = aRand % 6 + 3;
			if (mNextTip == mTip)
				mNextTip = (aRand % 6 + 1) % 6 + 3;
			mDelay = 500;
		}
		else
		{
			mTip = 0;
			mDelay = 0;
			mLeaving = false;
		}
		mPhase = 0;
	}
	int anAlpha;
	if (mPhase < 0x80)
		anAlpha = mPhase * 2;
	else if (mPeriod - mPhase < 0x80)
		anAlpha = (mPeriod - mPhase) * 2;
	else
		anAlpha = 0xff;
	if (anAlpha != mAlpha)
	{
		mAlpha = anAlpha;
		if (mOwner != NULL)
			mOwner->MarkDirty();
	}
}

// 0x42c550: the text goes now
void SpeechBubble::Hurry()
{
	int aPeriod = mPeriod;
	mLeaving = true;
	if (mDelay != 0)
	{
		mDelay = 0;
		mPhase = aPeriod;
		return;
	}
	if (mPhase < aPeriod - 0x80)
	{
		if (mPhase < 0x81)
			mPhase = aPeriod - mPhase;
		else
			mPhase = aPeriod - 0x80;
	}
}

// 0x42e110
Character::Character(PixelusApp* theApp, int theGod, Widget* theOwner, int theMode)
{
	mApp = theApp;
	mTick = 0;
	mAngle = 0;
	mSpeed = 0.0008;
	mGod = theGod;
	mAnimation = 0;
	mAnimTick = 0;
	mAnimEntry = 0;
	mPose = 2;
	mFade = 0xff;
	mPrevPose = 2;
	mBlink = -1;
	mBlinkAngle = 1.5;
	mBubble = theOwner != NULL ? new SpeechBubble(theApp, theGod, theOwner, theMode) : NULL;
}

Character::~Character()
{
	delete mBubble;
}

// 0x42df40
void Character::Update(Widget* theWidget)
{
	double aSpeed = mAngle + mSpeed;
	mTick++;
	mAngle = aSpeed;
	mSpeed = mSpeed - aSpeed * 0.0004;
	mAngle = mAngle * 0.99;
	if (mApp->AppRandNext() % 300 == 0)
		mSpeed = mSpeed + 0.0005;

	if (mAnimation != NO_ANIMATION)
	{
		mAnimTick++;
		if (gAnimations[mAnimation][mAnimEntry].mTime * 2 <= mAnimTick)
		{
			mAnimEntry++;
			mAnimTick = 0;
			int aPose = gAnimations[mAnimation][mAnimEntry].mPose;
			if (aPose == -1)
			{
				mAnimation = NO_ANIMATION;
				aPose = 2;
			}
			if (aPose != mPose)
			{
				mPrevPose = mPose;
				mPose = aPose;
				mFade = 0;
			}
		}
	}

	if (mFade < 0xff)
	{
		mFade += 0x18;
		if (mFade > 0xfe)
			mFade = 0xff;
	}

	int aRand = mApp->RandNext();
	if (aRand % 250 == 0 && mAnimation == NO_ANIMATION)
	{
		aRand = mApp->RandNext();
		mAnimEntry = 0;
		mAnimTick = 0;
		mAnimation = aRand % 5;
		int aPose = gAnimations[mAnimation][0].mPose;
		if (aPose != mPose)
		{
			mPrevPose = mPose;
			mPose = aPose;
			mFade = 0;
		}
	}

	if (theWidget != NULL)
		theWidget->MarkDirty();

	if (mGod == 9)
	{
		if (mBlink == -1)
		{
			if (mApp->RandNext() % 400 == 0)
			{
				mBlink = 0;
				if ((mApp->RandNext() & 1) == 0)
					mBlinkAngle = (mApp->RandNext() % 200) * 0.001 + 1.4;
				else
					mBlinkAngle = 0.10000000149011612 - (mApp->RandNext() % 200) * 0.001;
			}
		}
		else
		{
			mBlink++;
			if (mBlink > 100)
				mBlink = -1;
		}
	}

	if (mBubble != NULL)
		mBubble->Update();
}

// 0x42de40: a click on the figure hurries its bubble
void Character::Poke()
{
	if (mBubble != NULL)
		mBubble->Hurry();
}
