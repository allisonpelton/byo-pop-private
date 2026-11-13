#include "SupplyCopter.h"
#include "Board.h"
#include "FloatingText.h"
#include "GameText.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "PlayTipDialog.h"
#include "PowerUpDrops.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Sprite.h"

#include <stdlib.h>

using namespace Sexy;

namespace
{
	const int FRIENDLY_LEVEL_CAP = 14;			// the level its health is worked out from stops here
	const int SURVIVAL_LEVEL_TIME = 12000;		// updates of survival time that count as a level
	const int SKY_BAND_HEIGHT = 180;			// it flies in the sky's upper 180 pixels below the top 50
	const int SKY_TOP = 50;
	const int EDGE_X = 520;						// it comes in and leaves at this distance from the middle
	const double FADE_PER_PIXEL = 0.005;		// its sound fades this much a pixel outside the field
	const int FIELD_EDGE_X = 320;
	const int DROP_RANGE = 5;					// the drop is made within this many pixels of its drop place
	const int DROP_SPREAD = 200;				// the drop places lie in a band of this many pixels
	const int DEATH_POINTS = -1000;				// killing it costs this in a campaign
	const int DEATH_TIME_PENALTY = 6000;		// and this many updates in survival (the "-0:30" text)
}

// 0x437450: it comes in from a random side at a random height, with 100 to 1000 health; it holds no craft table entry
SupplyCopter::SupplyCopter(Board* theBoard, int thePowerUp) : Enemy(theBoard)
{
	mPowerUp = thePowerUp;
	mHasDropped = false;

	int aLevel;
	if (!mBoard->mIsSurvival)
		aLevel = mApp->mLevel;
	else
		aLevel = mBoard->mLevelTime / SURVIVAL_LEVEL_TIME;
	if (aLevel > FRIENDLY_LEVEL_CAP)
		aLevel = FRIENDLY_LEVEL_CAP;
	mHealth = (double)((aLevel * 5 + 50) * 2);
	mMaxHealth = mHealth;

	mSprite = mApp->mSprites[IMAGE_PUPCOPTER];
	mRotorFrame = 0.0;
	int aHeight = GetCelHeight();
	int aRand = Rand();
	mVelY = 0.0;
	mY = (double)(aRand % (SKY_BAND_HEIGHT - aHeight) + SKY_TOP + GetCelHeight() / 2);
	mMirrored = Rand() % 2 != 0;
	mPoints = -1500;
	int aDropRand = Rand();
	mDropOffset = aDropRand % DROP_SPREAD - DROP_SPREAD / 2;
	mFlash = 0;
	mDamageStage = 0;
	mIsFriendly = true;
	mBoard->mFriendlyCount++;
	mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]--;
	mBoard->mEnemiesSpawned--;
	mTipShown = false;
	if (!mMirrored)
	{
		mX = -(double)EDGE_X;
		mVelX = (double)(Rand() % 50) * 0.01 + 2.0;
	}
	else
	{
		mX = (double)EDGE_X;
		mVelX = -((double)(Rand() % 50) * 0.01 + 2.0);
	}
	// the original adds entry 0's index, which the craft table leaves at 0
	mBoard->mLevelEndBlocks += mApp->mCraft[0].mIndex;

	mSound = mApp->mSoundManager->GetSoundInstance(SOUND_PUPCOPTER);
	if (mSound != NULL)
	{
		mSound->SetPan((int)(mX * 3.0));
		mSound->SetVolume(0.0);
		mSound->Play(true, true);
	}
}

// 0x437680
SupplyCopter::~SupplyCopter()
{
	mBoard->mLevelEndBlocks -= mApp->mCraft[0].mIndex;
	if (mSound != NULL)
		mSound->Release();
}

// 0x4376f0
void SupplyCopter::UpdateF(float theFrac)
{
	mX = (double)theFrac * mVelX + mX;
}

// 0x437710: the helicopter, its two rotors (at the art's places for either way it faces), and the "don't shoot" sign
// over it while it flashes
void SupplyCopter::Draw(Graphics* g)
{
	Enemy::Draw(g);
	Sprite* aRotor = mApp->mSprites[IMAGE_PUPROTOR];
	if (!mMirrored)
	{
		aRotor->Draw(g, (int)(mX - 47.0), (int)(mY - 24.0), true, (int)mRotorFrame, 0, false);
		aRotor->Draw(g, (int)(mX + 27.0), (int)(mY - 18.0), true, (int)mRotorFrame, 0, false);
	}
	else
	{
		aRotor->Draw(g, (int)(mX - 27.0), (int)(mY - 18.0), true, (int)mRotorFrame, 0, false);
		aRotor->Draw(g, (int)(mX + 47.0), (int)(mY - 24.0), true, (int)mRotorFrame, 0, false);
	}
	if (mFlash != 0)
	{
		g->SetColor(Color(255, 255, 255, mFlash));
		g->SetColorizeImages(true);
		mApp->mSprites[IMAGE_DONTSHOOT]->Draw(g, (int)mX, (int)mY, true, 0, 0, false);
		g->SetColorizeImages(false);
	}
}

// 0x437890
void SupplyCopter::SilenceSounds()
{
	if (mSound != NULL)
		mSound->SetVolume(0.0);
}

// 0x4378b0
void SupplyCopter::RestoreSounds()
{
	if (mSound != NULL)
		SetSoundVolume();
}

// port: the volume is full over the field and fades out over the 200 pixels outside it (RestoreSounds, and the
// second half of Update's sound step)
void SupplyCopter::SetSoundVolume()
{
	int aDistance = abs((int)mX);
	if (aDistance > FIELD_EDGE_X)
		mSound->SetVolume(((double)EDGE_X - (double)aDistance) * FADE_PER_PIXEL);
	else
		mSound->SetVolume(1.0);
}

// 0x437940: the rotors turn and the sign fades; at its drop place it drops the power-up; when the tank hits it (the
// update after) the sign shows, it makes a damage sound at each quarter of its health lost, and the first time
// it shows the play tip; it leaves 520 pixels out
void SupplyCopter::Update()
{
	mRotorFrame += 0.5;
	if (7.0 <= mRotorFrame)
		mRotorFrame -= 7.0;
	if (mFlash > 0)
		mFlash -= FLASH_FADE;
	if (mFlash < 0)
		mFlash = 0;

	if (abs((int)(mX - (double)mDropOffset)) < DROP_RANGE && !mHasDropped && mApp->mHelpScreen == NULL)
	{
		mBoard->mPowerUpDrops->Add(mX, mY, mVelX, mVelY, mPowerUp);
		mHasDropped = true;
	}

	if (mSound != NULL)
	{
		mSound->SetPan((int)(mX * 3.0));
		SetSoundVolume();
	}

	if (mHitFlash == 255)
	{
		mFlash = 255;
		bool aPlayDamage = false;
		if (mMaxHealth * 0.75 > mHealth && mDamageStage == 0)
		{
			mApp->PlaySample(SOUND_FRIENDLY1, (int)(mX * 3.0));
			aPlayDamage = true;
		}
		else if (mMaxHealth * 0.5 > mHealth && mDamageStage == 1)
		{
			mApp->PlaySample(SOUND_FRIENDLY2, (int)(mX * 3.0));
			aPlayDamage = true;
		}
		else if (mMaxHealth * 0.25 > mHealth && mDamageStage == 2)
		{
			mApp->PlaySample(SOUND_FRIENDLY3, (int)(mX * 3.0));
			aPlayDamage = true;
		}
		if (aPlayDamage)
			mDamageStage++;

		if (!mTipShown && (mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_TIPS) == 0 &&
			(mApp->mTipsShown & HeavyWeaponApp::TIP_FRIENDLY) == 0 && mApp->mHelpScreen == NULL && !mBoard->mIsSurvival)
		{
			mApp->AddDialog(PlayTipDialog::DIALOG_ID, new PlayTipDialog(mBoard->mApp, GT(0x005305e0)));
			mApp->mTipsShown |= HeavyWeaponApp::TIP_FRIENDLY;
		}
		mTipShown = true;
	}

	Enemy::Update();
	if (mX < -(double)EDGE_X || (double)EDGE_X < mX)
	{
		mBoard->mFriendlyCount--;
		Remove();
	}
}

// 0x437c50: shooting it costs points, or in survival 30 seconds of time
void SupplyCopter::Killed()
{
	Enemy::Killed();
	Enemy::Explode();
	if (!mBoard->mIsSurvival)
	{
		mBoard->AddScore(DEATH_POINTS, (int)mX, (int)mY);
	}
	else
	{
		new FloatingText(mBoard, GT(0x00530648), (int)mX, (int)mY, false, Color(255, 0, 0));
		mBoard->mTimePenalty += DEATH_TIME_PENALTY;
		if (mBoard->mLevelTime - mBoard->mTimePenalty < 0)
			mBoard->mTimePenalty = mBoard->mLevelTime;
	}
	mApp->PlaySample(SOUND_FRIENDLYDIE, (int)(mX * 3.0));
	mBoard->mFriendlyCount--;
}
