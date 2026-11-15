#include "MegaLaserPart.h"
#include "Board.h"
#include "FloatingText.h"
#include "GameText.h"
#include "HeavyWeaponApp.h"
#include "PlayTipDialog.h"
#include "Sprite.h"

#include <math.h>

using namespace Sexy;

// 0x42da00
MegaLaserPart::MegaLaserPart(Board* theBoard, int x, int y)
{
	mBoard = theBoard;
	mApp = theBoard->mApp;
	mFrame = (double)theBoard->mMegaLaserPartsCaught;
	mX = (double)x;
	mVelY = 0.0;
	mY = (double)y;
	mIsCaught = false;
	mVelX = (double)(x / -120);
	mNode = theBoard->mMegaLaserParts.Append(this);
}

// 0x42d9b0
void MegaLaserPart::Draw(Graphics* g)
{
	mBoard->mApp->mSprites[IMAGE_CRATES]->Draw(g, (int)mX, (int)mY, true, (int)mFrame, 0, false);
}

// 0x42db00
void MegaLaserPart::Update()
{
	mX = mX + mVelX;
	double aY = mVelY + mY;
	mY = aY;

	if (mIsCaught)
	{
		// Flying to the meter at the top of the screen, at most 8 pixels an update
		double aDistX = 160.0 - mX;
		mVelX = aDistX;
		double aDistY = 20.0 - aY;
		mVelY = aDistY;
		double aDist = sqrt(pow(aDistY, 2.0) + pow(aDistX, 2.0));
		if (aDist > 8.0)
		{
			double aScale = 8.0 / aDist;
			mVelX = aDistX * aScale;
			mVelY = aDistY * aScale;
		}

		if (mY < 21.0)
		{
			mBoard->mApp->PlaySample(SOUND_MEGAUP1 + mBoard->mMegaLaserPartsCaught, 0);
			mBoard->mMegaLaserPartsCaught++;
			int aCaught = mBoard->mMegaLaserPartsCaught;
			if (aCaught == NUM_PARTS)
			{
				mBoard->mMegaLaserPartsCaught = 0;
				mBoard->mMegaLaserTime = 3000;
				mBoard->mApp->PlaySample(SOUND_V_MEGALASER, 0);
			}
			else
			{
				mBoard->AddScore(aCaught * 1000, (int)mX, (int)mY);
			}
			mBoard->mMegaLaserParts.Remove(mNode);
		}
		return;
	}

	mVelX = mVelX * 0.99;
	mVelY = mVelY + 0.02;
	if (mVelY > 1.5)
		mVelY = 1.5;

	if (mY > 460.0)
	{
		mBoard->mApp->PlaySample(SOUND_STARSMASH, (int)(mX * 3.0));
		mBoard->AddExplosion(mX, mY, 50, 50, mVelX, 0.0, 0.0);
		mBoard->AddCrater((int)mX, 0);
		mBoard->mMegaLaserParts.Remove(mNode);
		return;
	}

	if (mBoard->mTankDeadTime != 0)
		return;
	if (!mBoard->mApp->mSprites[IMAGE_CRATES]->Collides((int)mX, (int)mY, (int)mFrame, 0, false,
			mBoard->mApp->mSprites[IMAGE_TANK], (int)mBoard->mTankX, mBoard->mTankY, (int)mBoard->mTreadFrame, 0, false, true))
		return;

	mBoard->mApp->PlaySample(SOUND_STAR, (int)mX);
	new FloatingText(mBoard, StrFormat(GTC(0x0052f9d0), (mBoard->mMegaLaserPartsCaught + 1) * 25), (int)mBoard->mTankX,
		mBoard->mTankY, true, Color(0));
	mIsCaught = true;

	// The first part caught shows what the parts are for
	if ((mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_TIPS) == 0 && (mApp->mTipsShown & HeavyWeaponApp::TIP_MEGALASER) == 0 &&
		mApp->mHelpScreen == NULL && !mBoard->mIsSurvival)
	{
		mApp->AddDialog(PlayTipDialog::DIALOG_ID, new PlayTipDialog(mApp, GT(0x0052f980)));
		mApp->mTipsShown |= HeavyWeaponApp::TIP_MEGALASER;
	}
}
