// The screen of a game: the update (see Board.h).

#include "Board.h"
#include "AtomicaApp.h"
#include "Atomicus.h"
#include "BonusMolecules.h"
#include "Grid.h"
#include "Effects.h"
#include "Swirl.h"

#include <cmath>
#include <vector>

using namespace Sexy;

// 0x433080 / 0x41ffd0 / 0x425c30
void Board::Update()
{
	if (mApp->mB3eac)
		mApp->CloseMenu();
	MarkDirty();	// the original calls the widget's vtable slot 42 here, not Widget::Update
	if (!mApp->mHasFocus || mApp->mMinimized || (mApp->mPauseCount > 0))
		return;

	if ((mF8c <= 10.0f) && mGrid->mB33c)
	{
		mI16c -= 2;
		if (mI16c < 0)
			mI16c = 0;
	}

	// the geiger counter's glow follows the combo
	int aCombo = mGrid->mCombo;
	if (aCombo < 2)
	{
		float aGlow = mGeigerGlow;
		mGeigerGlowStep = -1.0f;
		mGeigerGlow = aGlow - 2.0f;
		if (aGlow - 2.0f < 0.0f)
			mGeigerGlow = 0;
	}
	else
	{
		float aGlow = (float) aCombo * mGeigerGlowStep + mGeigerGlow;
		mGeigerGlow = aGlow;
		if (255.0f <= aGlow)
		{
			mGeigerGlow = 255.0f;
			mGeigerGlowStep = -1.0f;
		}
		if (mGeigerGlow <= 0.0f)
		{
			mGeigerGlowStep = 1.0f;
			mGeigerGlow = 0;
		}
	}

	if (mGrid->mLevelOver)
	{
		if (mGrid->mOverTick < 0x12d)
			mI170 = 0;
		else
		{
			mI170 += 2;
			if (0xff < mI170)
				mI170 = 0xff;
		}
	}
	if ((0 < mMonitorSlide) && ((mMonitorSlide -= 3) < 0))
		mMonitorSlide = 0;

	float aPulse = mPulse;
	mPulse = aPulse + mPulseStep;
	if (255.0f < aPulse + mPulseStep)
	{
		mPulse = 255.0f;
		mPulseStep = -fabsf(mPulseStep);
	}
	if (mPulse < 128.0f)
	{
		mPulse = 128.0f;
		mPulseStep = fabsf(mPulseStep);
	}

	// the bomb button's lamp
	if ((mProgress < 6.0f) || ((mApp->mMode == 0) && (mMeter.mLevel % 5 == 0)))
	{
		mBombPhase = 0;
		mButtonBomb->SetDisabled(true);
		if ((0.0f < mBombFrame) && ((mBombFrame -= 0.25f) < 0.0f))
			mBombFrame = 0;
	}
	else
	{
		if (11.0f <= mBombPhase)
			mBombPhase = 11.0f;
		else
			mBombPhase = mBombPhase + 0.2f;
		mButtonBomb->SetDisabled(false);
		if (mBombFrame == 15.0f)
		{
			mApp->mSndCapClick->Play();
			mAtomicus->Say(0);
		}
		if (mBombFrame < 24.0f)
			mBombFrame = mBombFrame + 0.25f;
	}
	if (mBombOn)
		mButtonBomb->SetDisabled(true);

	mEffects.Reset();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		if (!anEffect->Update())
		{
			mEffects.Remove(anEffect);
			delete anEffect;
		}
		anEffect = mEffects.Next();
	}

	// the game starts after 100 updates (the 50 counts of a half)
	if ((0.0f < mF8c) && ((mF8c -= 0.5f) == 0.0f))
	{
		mGrid->Reset(mNumColorsStart);
		mGrid->mPulse = 100.0f;
		mGrid->mPulseStep = -1.0f;
	}

	// the swirls of the background: the copies the flown atoms circle in aim at the next larger ring; then every swirl is
	// brought up to date (a copy seven times) and a new one made every 95 updates; at the start of a game this goes on until
	// one of them has gone
	{
		std::vector<Swirl*> aCopies = mSwirls.mItems;
		for (size_t i = 0; i < aCopies.size(); i++)
		{
			Swirl* aClone = aCopies[i];
			if (!aClone->mClone)
				continue;
			mSwirls.Reset();
			Swirl* aMain = mSwirls.Next();
			while (aMain != NULL)
			{
				if (aClone->mRadius < aMain->mRadius)
				{
					aClone->mTargetX = aMain->mX;
					aClone->mTargetY = aMain->mY;
					break;
				}
				aMain = mSwirls.Next();
			}
		}
	}
	do
	{
		mSwirls.Reset();
		Swirl* aSwirl = mSwirls.Next();
		while (aSwirl != NULL)
		{
			bool aGone = false;
			if (aSwirl->mClone)
			{
				for (int i = 0; i < 7; i++)
				{
					if (!aSwirl->Update())
					{
						aGone = true;
						break;
					}
				}
			}
			else if (!aSwirl->Update())
				aGone = true;
			if (aGone)
			{
				mSwirls.Remove(aSwirl);
				delete aSwirl;
				mB98 = false;
			}
			aSwirl = mSwirls.Next();
		}
		if ((mI184 < 1) || mB98)
			mI90--;
		if (mI90 < 1)
		{
			mSwirls.Add(new Swirl(this));
			mI90 = 0x5f;
		}
	} while (mB98);
	mAngle = mAngle + 0.1f;
	if (360.0f <= mAngle)
		mAngle = mAngle - 360.0f;

	// the colour flash fades
	for (int i = 0; i < 5; i++)
	{
		if (0 < mFlash.mRed)
			mFlash.mRed--;
		if (0 < mFlash.mGreen)
			mFlash.mGreen--;
		if (0 < mFlash.mBlue)
			mFlash.mBlue--;
	}

	// the geiger needle
	if (mF0ac == 0.0f)
	{
		if (0.0f < mNeedle)
			mNeedle = mNeedle - 1.0f;
	}
	else
	{
		float aNeedle = mNeedleSpeed * 0.33333334f + mNeedle;
		mNeedle = aNeedle;
		if (mF0ac < aNeedle)
		{
			float aSpeed = mNeedleSpeed - 0.22f;
			mNeedleSpeed = aSpeed;
			float aLimit = (float) -mGrid->mCombo;
			if (-0.1f < aLimit)
				aLimit = -0.1f;
			if (aSpeed < aLimit)
				mNeedleSpeed = aLimit;
		}
		if (mNeedle < mF0ac)
		{
			float aSpeed = mNeedleSpeed + 0.22f;
			mNeedleSpeed = aSpeed;
			float aLimit = (float) mGrid->mCombo;
			if (aLimit < 0.1f)
				aLimit = 0.1f;
			if (aLimit < aSpeed)
				mNeedleSpeed = aLimit;
		}
	}
	if (mNeedle < 0.0f)
		mNeedle = 0;
	int aStep = mGrid->mCombo;
	if (aStep < 1)
		aStep = 1;
	mNeedleTick += aStep;
	if (100 < mNeedleTick)
	{
		mNeedleBlink = !mNeedleBlink;
		mNeedleTick -= 100;
	}

	mMeter.Update();
	if (mApp->mRand.Get(0x32, false) == 5)	// 0x41ffd0
		mFlicker = (mFlicker == 0xff) ? 0xdc : 0xff;
	mAtomicus->Update();

	// the level is made: the signs, the fly-away of the atoms, the level-up box, the next level's field
	if (mMeter.mFull)
	{
		mGrid->mSelected = NULL;
		if (mMeter.mOverTick == 10)
		{
			mAtomicus->Say(0x10);
			mNumColorsStart++;
		}
		if (mMeter.mOverTick == 200)
		{
			mGrid->FlyAwayAll();
			if (mGrid->mB4c0 && (mApp->mMode == 0))
				FloatScore(mGrid->mWidth / 2 + mGrid->mX, mGrid->mHeight / 2 - 100 + mGrid->mY, mMeter.mLevel * 100,
					Color(255, 255, 255, 255));
			mI184 = 0;
			mI198 = 0;
		}
		if (mMeter.mOverTick == 0x226)
			mApp->ShowLevelUp();
		if ((800 < mMeter.mOverTick) && (mApp->mLevelUpDialog == NULL))
		{
			mMeter.Reset();
			mGrid->Reset(mNumColorsStart);
		}
		if ((mMeter.mOverTick < 0xb) || (199 < mMeter.mOverTick))
		{
			mI174 -= 2;
			if (mI174 < 0)
				mI174 = 0;
		}
		else
		{
			mI174 += 3;
			if (0xff < mI174)
				mI174 = 0xff;
		}
	}
	else
	{
		mI174 -= 2;
		if (mI174 < 0)
			mI174 = 0;
	}

	// the bomb
	if (mBombOn)
	{
		StepBomb();
		if (mB164)
		{
			mI1c0 += 5;
			if (0xff < mI1c0)
				mI1c0 = 0xff;
			mBombTint.mRed += 2;
			mBombTint.mGreen -= 4;
			mBombTint.mBlue -= 4;
			if (0xff < mBombTint.mRed)
				mBombTint.mRed = 0xff;
			if (mBombTint.mGreen < 0)
				mBombTint.mGreen = 0;
			if (mBombTint.mBlue < 0)
				mBombTint.mBlue = 0;
			SetTint(mBombTint.mRed, mBombTint.mGreen, mBombTint.mBlue);
		}
	}
	else
	{
		mI1c0 -= 2;
		if (mI1c0 < 0)
			mI1c0 = 0;
		mB164 = false;
		mBombTint.mRed++;
		mBombTint.mGreen++;
		mBombTint.mBlue++;
		if (0xff < mBombTint.mRed)
			mBombTint.mRed = 0xff;
		if (0xff < mBombTint.mGreen)
			mBombTint.mGreen = 0xff;
		if (0xff < mBombTint.mBlue)
			mBombTint.mBlue = 0xff;
		SetTint(mBombTint.mRed, mBombTint.mGreen, mBombTint.mBlue);
	}

	mBonus->Update();

	// the freeze
	if (mI184 < 1)
	{
		mI180--;
		if (mI180 < 0)
			mI180 = 0;
		mGlowSwirl = NULL;
	}
	else
		StepFreeze();

	// the air element: the atoms turn green
	if (mI198 < 1)
	{
		mI194++;
		if (0xff < mI194)
			mI194 = 0xff;
		if (0xfe < mI194)
			return;
		mBombTint.mRed++;
		mBombTint.mGreen++;
		mBombTint.mBlue++;
		if (0xff < mBombTint.mRed)
			mBombTint.mRed = 0xff;
		if (0xff < mBombTint.mGreen)
			mBombTint.mGreen = 0xff;
		if (0xff < mBombTint.mBlue)
			mBombTint.mBlue = 0xff;
	}
	else
	{
		StepAir();
		mBombTint.mBlue -= 4;
		mBombTint.mRed -= 4;
		mBombTint.mGreen += 2;
		if (mBombTint.mRed < 0x80)
			mBombTint.mRed = 0x80;
		if (0xff < mBombTint.mGreen)
			mBombTint.mGreen = 0xff;
		if (mBombTint.mBlue < 0x80)
			mBombTint.mBlue = 0x80;
	}
	SetTint(mBombTint.mRed, mBombTint.mGreen, mBombTint.mBlue);
}
