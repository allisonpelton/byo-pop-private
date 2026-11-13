#include "PowerUpDrops.h"
#include "Board.h"
#include "FloatingText.h"
#include "GameText.h"
#include "HeavyWeaponApp.h"
#include "PlayTipDialog.h"
#include "Sprite.h"

using namespace Sexy;

// 0x52ee68 (a copy at 0x52bb18, which the board reads): 0x30 bytes an entry, the name, the top level at +0x20 and the
// level factor at +0x28
const PowerUpInfo Sexy::gPowerUpInfo[HeavyWeaponApp::NUM_POWER_UPS] =
{
	{ GTA(0x0052ee68), 3, 0.0 },
	{ GTA(0x0052ee98), 3, 0.0 },
	{ GTA(0x0052eec8), 10000, 1.0 },
	{ GTA(0x0052eef8), 10000, 1.0 },
	{ GTA(0x0052ef28), 10000, 1.0 },
	{ GTA(0x0052ef58), 4, 0.5 }
};

// The special weapons' names (0x118 bytes an entry at 0x52ef88; the armory's texts follow each name)
static const unsigned int WEAPON_NAMES[HeavyWeaponApp::NUM_WEAPONS] =
{
	GTA(0x0052ef88), GTA(0x0052f0a0), GTA(0x0052f1b8), GTA(0x0052f2d0), GTA(0x0052f3e8), GTA(0x0052f500)
};

// 0x42c170
PowerUpDrops::PowerUpDrops(Board* theBoard)
{
	mBoard = theBoard;
	mApp = theBoard->mApp;
}

// 0x42c1b0
void PowerUpDrops::Add(double x, double y, double theVelX, double theVelY, int theType)
{
	PowerUpDrop* aDrop = new PowerUpDrop;
	mDrops.Append(aDrop);
	aDrop->mX = x;
	aDrop->mY = y;
	aDrop->mVelX = -(x * 0.00625);
	aDrop->mVelY = theVelY;
	aDrop->mType = theType;
}

// 0x42c230
void PowerUpDrops::Draw(Graphics* g)
{
	for (mDrops.mCurrent = mDrops.mHead; mDrops.mCurrent->mNext != NULL; )
	{
		mDrops.mCurrent = mDrops.mCurrent->mNext;
		PowerUpDrop* aDrop = mDrops.Current();
		mBoard->mApp->mSprites[IMAGE_POWERUPS]->Draw(g, (int)aDrop->mX, (int)aDrop->mY, true, aDrop->mType, 0, false);
	}
}

// 0x42c2a0
void PowerUpDrops::Clear()
{
	mDrops.Clear();
}

// 0x42c2b0
void PowerUpDrops::Update()
{
	for (mDrops.mCurrent = mDrops.mHead; mDrops.mCurrent->mNext != NULL; )
	{
		mDrops.mCurrent = mDrops.mCurrent->mNext;
		PowerUpDrop* aDrop = mDrops.Current();

		aDrop->mX = aDrop->mX + aDrop->mVelX;
		double aVelX = aDrop->mVelX * 0.99;
		aDrop->mVelX = aVelX;
		if (aDrop->mY >= 455.0)
		{
			// On the ground: it rolls back with the ground while the tank drives
			if (mBoard->mTankDeadTime == 0)
			{
				aDrop->mVelX = aVelX - 0.003;
				if (aDrop->mVelX < -1.0)
					aDrop->mVelX = -1.0;
			}
			aDrop->mY = 455.0;
		}
		else
		{
			aDrop->mY = aDrop->mVelY + aDrop->mY;
			aDrop->mVelY = aDrop->mVelY + 0.07;
			if (aDrop->mVelY > 3.0)
				aDrop->mVelY = 3.0;
		}

		Sprite* aSprite = mBoard->mApp->mSprites[IMAGE_POWERUPS];
		if ((double)(-320 - aSprite->mCelWidth) > aDrop->mX)
		{
			mDrops.RemoveCurrent();
			continue;
		}

		if (mBoard->mTankDeadTime != 0)
			continue;
		if (!aSprite->Collides((int)aDrop->mX, (int)aDrop->mY, aDrop->mType, 0, false, mBoard->mApp->mSprites[IMAGE_TANK],
				(int)mBoard->mTankX, mBoard->mTankY, (int)mBoard->mTreadFrame, 0, false, true))
			continue;

		int aPan = (int)(mBoard->mTankX * 3.0);	// the sounds' pan
		if (aDrop->mType >= FIRST_WEAPON_TYPE)
		{
			int& aLevel = mBoard->mApp->mWeapons[aDrop->mType - FIRST_WEAPON_TYPE];
			if (aLevel < MAX_WEAPON_LEVEL)
			{
				aLevel++;
				new FloatingText(mBoard, GT(WEAPON_NAMES[aDrop->mType - FIRST_WEAPON_TYPE]), (int)mBoard->mTankX,
					mBoard->mTankY - 13, true, Color(0));
				mBoard->mApp->PlaySample(SOUND_UPGRADE, aPan);
			}
			mDrops.RemoveCurrent();
			continue;
		}

		int& aLevel = mBoard->mApp->mPowerUps[aDrop->mType];
		if (aLevel < gPowerUpInfo[aDrop->mType].mMax)
		{
			aLevel++;
			new FloatingText(mBoard, GT(gPowerUpInfo[aDrop->mType].mName), (int)mBoard->mTankX, mBoard->mTankY - 13, true,
				Color(0));
		}

		// The first nuke shows how to fire one
		if (aDrop->mType == HeavyWeaponApp::POWERUP_NUKES && (mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_TIPS) == 0 &&
			(mApp->mTipsShown & HeavyWeaponApp::TIP_NUKE) == 0 && mApp->mHelpScreen == NULL && !mBoard->mIsSurvival)
		{
			mApp->AddDialog(PlayTipDialog::DIALOG_ID, new PlayTipDialog(mApp, GT(0x0052f628)));
			mApp->mTipsShown |= HeavyWeaponApp::TIP_NUKE;
		}

		int aType = aDrop->mType;
		if (aType == HeavyWeaponApp::POWERUP_GUN_POWER || aType == HeavyWeaponApp::POWERUP_FIRE_RATE ||
			aType == HeavyWeaponApp::POWERUP_SPREAD)
		{
			mBoard->mApp->PlaySample(SOUND_GUNPOWERUP, aPan);
		}
		else if (aType == HeavyWeaponApp::POWERUP_SHIELD)
		{
			mBoard->mApp->PlaySample(SOUND_SHIELDUP, aPan);
			mBoard->mShieldFlash = 255;
		}
		else if (aType == HeavyWeaponApp::POWERUP_SPEED)
		{
			mBoard->mApp->PlaySample(SOUND_SPEEDUP, aPan);
		}
		else
		{
			mBoard->mApp->PlaySample(SOUND_POWERUP, aPan);
		}
		mDrops.RemoveCurrent();
	}
}
