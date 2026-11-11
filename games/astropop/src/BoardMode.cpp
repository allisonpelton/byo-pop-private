// Board: the upkeep of the bonus states of the field (UpdateSpecialMode, 0x46f010): the scrolling texts, the meter that
// runs down, the explosions of mode 1 and the beams and wave of mode 2.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "Particle.h"
#include "Res.h"

using namespace Sexy;

// 0x4335e0: the field waits to be cleared
void Board::BeginClearWait()
{
	mFreezeUpdatesLeft = 1000;
	Fire(0x28, 1);
	mClearWaitStarted = true;
}

// 0x46f010 (4339 bytes)
void Board::UpdateSpecialMode()
{
	if (mPowerupActive == 1)
	{
		mBonusBannerHue = (mBonusBannerHue + 4) & 0xff;
		for (unsigned int i = 0; i < mBonusBannerTexts.size(); i++)
		{
			mBonusBannerTexts[i].mX++;
			if (mBonusBannerTexts[i].mX >= mBonusBannerTexts[i].mFont->StringWidth(mBonusBannerTexts[i].mText) + 0x29e)
			{
				int aMin = 500000000;
				for (unsigned int j = 0; j < mBonusBannerTexts.size(); j++)
				{
					if (mBonusBannerTexts[j].mX < aMin)
						aMin = mBonusBannerTexts[j].mX;
				}
				mBonusBannerTexts[i].mX = aMin - mBonusBannerTexts[i].mFont->StringWidth(mBonusBannerTexts[i].mText) - 0x1e;
			}
		}
	}

	if (mPowerupActive == 0)
	{
		// no bonus runs: the meter fills while the field is quiet
		if (!mBonusRoundOn && !mBonusWarpIn && mPlayStarted && m1bd8 < m1bdc && !mApp->mTutorialOn)
		{
			if (m1bd8 < m1bdc)
				m1bd8 = m1be4 + m1bd8;
			if (m1bd8 < m1bdc)
				return;
			m1bd8 = m1bdc;
			mMeterFullAnim.Reset();
			mMeterFullAnim.SetStopped(false);
			mApp->PlaySample(SOUND_POWERUP);
			return;
		}
		if (mBonusRoundOn)
			return;
		if (m1bd8 != m1bdc)
			return;
		if (mComboTimeLeft != 0)
			return;
		if (mApp->mTutorialOn)
			return;
		if (!mPlayStarted)
			return;
		mPsychicUsed = false;
		mLaserUsed = false;
		mMeterFullAnim.Update();
		return;
	}

	// a bonus runs: the meter runs down
	if (mMeterPaused)
		return;
	if (mMode == 0 && !mLaserUsed)
		return;
	if (mMode == 3 && !mPsychicUsed)
		return;
	m1bd8 = m1bd8 - m1be0;
	if (mShipShimmerY == 0)
	{
		mShipShimmerHeight += 2;
		if (mShipShimmerHeight > 0x34)
		{
			mShipShimmerY = 1;
			mShipShimmerHeight = 0x33;
		}
	}
	else
	{
		mShipShimmerY += 2;
		if (mShipShimmerY < 0x34)
			mShipShimmerHeight = 0x34 - mShipShimmerY;
		else
		{
			mShipShimmerY = 0;
			mShipShimmerHeight = 0;
		}
	}

	if (m1bd8 <= 0.0f && mMode != 1)
	{
		// the bonus is over
		mScreenShakeX = 0;
		m1bd8 = 0;
		mPowerupActive = 0;
		mMeterFullAnim.SetStopped(true);
		mTurbotPowerupAnim.SetStopped(true);
		mApp->PlaySample(SOUND_POWERUP_WORN_OFF);
		if (mMode == 3)
		{
			for (unsigned int i = 0; i < mHoverCells.size(); i++)
			{
				Brick* aBrick = mFieldBricks[mHoverCells[i].mRow][mHoverCells[i].mCol];
				if (aBrick != NULL)
					aBrick->StopFlash();
			}
			std::vector<CellRef>().swap(mHoverCells);
		}
		if (mMode == 2 && !mClearWaitStarted && mTakenBrickCount == 0 && mFiredGroups.empty() && CountMovingBricks(false) == 0)
			BeginClearWait();
		if (!mSirenAnim.mStopped)
			mPhase = 8;
		mPhaseDelay = PickSpawnDelay();
	}

	if (mMode == 1 && !mAtomicMarkedBricks.empty())
	{
		// mode 1: the marked bricks blow up one after the other
		int aNumAnims = (int) mAtomicExplosions.size();
		bool aMake = false;
		bool aFinal = false;
		if (aNumAnims < 3)
		{
			if (mUpdateCnt % 0x28 == 0)
				aMake = true;
		}
		else if (aNumAnims <= 6)
		{
			if (mUpdateCnt % 0x14 == 0)
				aMake = true;
		}
		else if (aNumAnims == 7 && mUpdateCnt % 0x14 == 0)
			aFinal = true;

		if (aMake)
		{
			ExplosionAnim* anAnim = new ExplosionAnim();
			unsigned int aPick = (unsigned int) (((int) AstroRand() % 500000000)) % (unsigned int) mAtomicMarkedBricks.size();
			const CellRef3& aCell = mAtomicMarkedBricks[aPick];
			float aX = (float) (aCell.mCol * 0x2c + 0xa6);
			anAnim->mX = aX;
			if (480.0f < aX)
				anAnim->mX = 480.0f;
			int aY = (aCell.mRow < 1) ? 4 : aCell.mRow * 0x19 + 9;
			anAnim->mY = (float) aY;
			anAnim->SetNumFrames(IMAGE_ATOMIC_EXPLOSION->mNumCols);
			anAnim->SetDelay(aNumAnims < 3 ? 6 : 3);
			int aSpin = ((int) AstroRand() % 500000000) % 0x32;
			anAnim->mStep = 6.283f / (float) aSpin;
			anAnim->mPingPong = false;
			anAnim->mHoldLast = true;
			anAnim->SetStopped(false);
			mAtomicExplosions.push_back(anAnim);
			mApp->PlaySampleAt(SOUND_EXPLODE1, ((int) anAnim->mX - 0xa6) / 0x2c);
		}
		else if (aFinal)
		{
			ExplosionAnim* anAnim = new ExplosionAnim();
			AstroRand();
			anAnim->mX = 300.0f;
			anAnim->mY = 100.0f;
			int aSpin = ((int) AstroRand() % 500000000) % 0x32;
			anAnim->mStep = 6.283f / (float) aSpin;
			anAnim->SetNumFrames(IMAGE_ATOMIC_EXPLOSION->mNumCols);
			anAnim->SetDelay(0xb);
			anAnim->mPingPong = false;
			anAnim->mHoldLast = true;
			anAnim->SetStopped(false);
			mAtomicFlashAlpha = 0;
			mOverlay.SetFill(255, 255, 255, 0);
			SetOverlayHidden(false);
			mAtomicExplosions.push_back(anAnim);
			mApp->PlaySample(SOUND_SLOWDOWN);
		}

		// (x87 at 24 bits, every result a float: the share of the meter that is left, a quarter of the updates, at most 255)
		float aRate = 1.0f / m1be0;
		int aLeft = (int) ((aRate * m1bdc - aRate * m1bd8) * 0.25f);
		if (aLeft > 0xff)
			aLeft = 0xff;
		Fire(0x21, aLeft);
		if (mUpdateCnt % 10 == 0)
		{
			m1b24++;
			if (m1b24 >= IMAGE_SPROCKET_SUPA_OVERLAY->mNumCols)
				m1b24 = 0;
			Fire(0x22, m1b24);
		}

		if (mAtomicBlastOver)
		{
			// the meter has run out: the bricks that were marked go (the original also filled a list of their cells
			// that nothing read)
			mAtomicBlastOver = false;
			for (unsigned int i = 0; i < mAtomicMarkedBricks.size(); i++)
			{
				int aCol = mAtomicMarkedBricks[i].mCol;
				int aRow = mAtomicMarkedBricks[i].mRow;
				Brick* aBrick = mAtomicMarkedBricks[i].mBrick;
				if (aBrick->IsIdle())
				{
					aBrick->StopFade();
					aBrick->StopShake();
					RemoveBrick(aBrick, aCol, aRow, true);
				}
			}
			mPowerupActive = 0;
			mScreenShakeX = 0;
			m1bd8 = 0;
			DropUnsupported(mTakenCells);
			mAtomicWeaponState = -1;
			std::vector<CellRef3>().swap(mAtomicMarkedBricks);
			if (!mSirenAnim.mStopped)
				mPhase = 8;
			for (unsigned int aRowIndex = 1; aRowIndex < mFieldBricks.size(); aRowIndex++)
			{
				for (int aColIndex = 0; aColIndex < 10; aColIndex++)
				{
					Brick* aBrick = mFieldBricks[aRowIndex][aColIndex];
					if (aBrick != NULL)
						aBrick->mSprocketFading = false;
				}
			}
			mPhaseDelay = PickSpawnDelay();
			Fire(0x28, 0);
			Fire(0x2a, 0);
		}

		if (mAtomicSymbolTimer >= 0)
		{
			mAtomicSymbolTimer++;
			if (mAtomicSymbolTimer > 0x95)
			{
				mAtomicSymbolTimer = -5;
				if (!mAtomicExplosions.empty())
					mAtomicExplosions[mAtomicExplosions.size() - 1]->SetStopped(false);
				mApp->PlaySample(SOUND_EXPLOSION);
			}
		}
		return;
	}

	if (mMode != 2)
		return;

	// mode 2: beams and a wave sweep over the field
	mTurbotPowerupAnim.Update();
	if (mPowerupActive == 1)
		mScreenShakeX = ((int) AstroRand() % 500000000) % 0xb - 5;
	{
		float aRate = 1.0f / m1be0;
		int aLeft = (int) ((aRate * m1bdc - aRate * m1bd8) * 0.25f);
		if (aLeft > 0xff)
			aLeft = 0xff;
		Fire(0x23, aLeft);
	}
	if (mUpdateCnt % 10 == 0)
	{
		m1b50++;
		if (m1b50 >= IMAGE_TURBOT_SUPA_OVERLAY->mNumCols)
			m1b50 = 0;
		Fire(0x24, m1b50);
	}
	if (mTurbotPowerupAlpha < 0xff)
	{
		mTurbotPowerupAlpha += 2;
		if (mTurbotPowerupAlpha > 0xfe)
			mTurbotPowerupAlpha = 0xff;
	}
	if (m1b58 > 0 && mUpdateCnt % m1b58 == 0)
	{
		m1b58 = 0xf - mTurbotFlashAlpha / 10;
		if (mTurbotBeams.size() < 6)
		{
			LaserRec aBeam;
			aBeam.mAngle = 0;
			aBeam.mLife = 0xff;
			aBeam.mDecay = ((int) AstroRand() % 500000000) % 3 + 4;
			switch (((int) AstroRand() % 500000000) % 7)
			{
			case 0: aBeam.mAngle = 0.0f; break;
			case 1: aBeam.mAngle = 30.0f; break;
			case 2: aBeam.mAngle = 60.0f; break;
			case 3: aBeam.mAngle = 90.0f; break;
			case 4: aBeam.mAngle = 120.0f; break;
			case 5: aBeam.mAngle = 150.0f; break;
			case 6: aBeam.mAngle = 180.0f; break;
			}
			mTurbotBeams.push_back(aBeam);
		}
	}
	for (int i = 0; i < (int) mTurbotBeams.size(); i++)
	{
		mTurbotBeams[i].mLife -= mTurbotBeams[i].mDecay;
		if (mTurbotBeams[i].mLife < 1)
		{
			mTurbotBeams.erase(mTurbotBeams.begin() + i);
			i--;
		}
	}
	if (mTurbotFlashAlpha < 100 && m1b4c != 3)
	{
		int aOldPhase = m1b4c;
		mTurbotFlashAlpha++;
		if (mTurbotFlashAlpha >= 100)
		{
			if (aOldPhase == 0)
				m1b4c = 2;
			else if (aOldPhase == 1)
				m1b4c = 3;
		}
	}

	bool aDone = true;      // false: leave through the end of the function
	if (m1b4c == 3)
	{
		if (m1b54 < 100)
		{
			m1b54++;
			if (m1b54 >= 100)
			{
				mTurbotFlashAlpha = 0xff;
				mTurbotPowerupAnim.SetStopped(true);
			}
			aDone = false;
		}
		else
		{
			mTurbotWaveY -= 10;
			if (mTurbotFlashAlpha >= 0)
			{
				mTurbotFlashAlpha -= 2;
				if (mTurbotFlashAlpha < 0)
					mTurbotFlashAlpha = 0;
			}
			if (!mTurbotMarkedCells.empty() && mTurbotWaveY <= 0xf0 - IMAGE_TURBOT_WAVE->mHeight / 2)
			{
				// the wave reaches the marked bricks
				unsigned int i = 0;
				while (i < mTurbotMarkedCells.size())
				{
					Brick* aBrick = mFieldBricks[mTurbotMarkedCells[i].mRow][mTurbotMarkedCells[i].mCol];
					if (aBrick == NULL)
					{
						i++;
						continue;
					}
					aBrick->StopShake();
					aBrick->StopFade();
					if (mWeaponPower != 2 || aBrick->mMutated == 1 || aBrick->mType != 9 || ((SpecialBrick*) aBrick)->mStage < 2)
					{
						RemoveBrick(aBrick, mTurbotMarkedCells[i].mCol, mTurbotMarkedCells[i].mRow, 2 < mWeaponPower);
						i++;
					}
					else
					{
						((SpecialBrick*) aBrick)->Crack(true);
						mTurbotMarkedCells.erase(mTurbotMarkedCells.begin() + i);
					}
				}
				DropUnsupported(mTakenCells);
				std::vector<CellRef>().swap(mTurbotMarkedCells);
				Fire(0x28, 0);
				Fire(0x2a, 0);
				Fire(0x1e);
			}
			if (!AnyMoveLeft(NULL) && mFreezeUpdatesLeft > 0x3c)
				mFreezeUpdatesLeft = 0x3c;
		}
	}
	else if (!mTurbotMarkedCells.empty())
		aDone = false;

	if (aDone && mTurbotWaveY <= -IMAGE_TURBOT_WAVE->mHeight && mTurbotMarkedCells.empty())
	{
		mTurbotWaveY = -0x10000;
		mTurbotFlashAlpha = -1;
		if (1.0f < m1bd8)
			m1bd8 = 1.0f;
		SetOverlayHidden(true);
	}
	if (mTurbotFlashAlpha < 0)
	{
		SetOverlayHidden(true);
		return;
	}
	mOverlay.SetFill(255, 255, 255, mTurbotFlashAlpha);
}
