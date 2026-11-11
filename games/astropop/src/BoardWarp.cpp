// Board: the warp between two stages and the level-up panel (UpdateWarp, 0x46a810: a long state machine that runs
// instead of the rest of the update) and the streaks of the warp charge-up (0x439650).

#include "Board.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Image.h"
#include "Particle.h"
#include "SoundInstance.h"
#include "ResourceManager.h"
#include "WidgetManager.h"
#include "Res.h"

using namespace Sexy;

// 0x439650: the streaks of the warp charge-up grow and speed up (the 100 are stepped in groups of ten)
void Board::UpdateStreaks()
{
	unsigned int aCount = (unsigned int) mStreaks.size();
	if (aCount > 100)
		aCount = 100;
	for (unsigned int i = 0; i < aCount; i++)
	{
		StreakRec* aStreak = &mStreaks[i];
		aStreak->mMargin += 3;
		aStreak->mY += 3.0f;
		if (aStreak->mMargin > 0x1e0)
			aStreak->mMargin = 0x1e0;
		aStreak->mSpeed += mBonusWarpIn ? 0.06f : 0.03f;
	}
}

// 0x46a810: true ends the update
bool Board::UpdateWarp()
{
	mWarpGateAnim.Update();
	mNewRankAnim.Update();
	if (mUpdateCnt % 5 == 0)
	{
		m1b14++;
		if (m1b14 >= IMAGE_CP_WARP_OVERLAY->mNumCols)
			m1b14 = 0;
	}

	// the level-up panel types its lines and the arms of the upgrade screen slide in
	if ((mWarpFlight || mAtWarpGate) && ((m1b2c != 2 && m1b2c != 4) || mUpgradeDialogDone))
	{
		if (!mWarpGatePicked && mGameType == 1 && mLevelUpPanelY < 0)
		{
			mLevelUpPanelY += 2;
			if (mLevelUpPanelY >= 0)
			{
				mLevelUpPanelY = 2;
				int aBest = 0;
				mApp->mUser->GetInt(0x71, &aBest);
				if (aBest < m1bd4)
				{
					mNewRankDelayOver = false;
					m1b44 = mUpgradeDialogSeen ? 1 : 100;
					mNewRankAnim.SetStopped(false);
					mApp->mUser->SetInt(0x71, m1bd4);
					mApp->mUser->Save();
				}
			}
		}
		if (mLevelUpPanelY >= 0 && mGameType == 1)
		{
			mLevelUpPanelY = 0;
			mClicksBlocked = false;
			if (mLevelUpScanY < 0)
				mLevelUpScanY = 0;
			int aCnt = mUpdateCnt;
			if (aCnt % mLevelUpCursorInterval == 0 && (unsigned int) mLevelUpLineIndex >= (unsigned int) mLevelUpLines.size())
				mLevelUpCursorShown = !mLevelUpCursorShown;
			if (aCnt % mLevelUpTypeInterval == 0 && (unsigned int) mLevelUpLineIndex < (unsigned int) mLevelUpLines.size())
			{
				mApp->PlaySample(SOUND_COMPUTER_TEXT);
				mLevelUpLetterIndex++;
				if ((unsigned int) mLevelUpLetterIndex >= (unsigned int) mLevelUpLines[mLevelUpLineIndex].size())
				{
					mLevelUpLetterIndex = 0;
					mLevelUpLineIndex++;
				}
			}
			if (mUpdateCnt % mLevelUpScanInterval == 0)
			{
				int aPulse = mLevelUpScanHeight;
				if (aPulse < mLevelUpScanMaxHeight && mLevelUpScanY == 0)
					mLevelUpScanHeight = aPulse + 1;
				else
				{
					mLevelUpScanY++;
					if (mLevelUpScanY + aPulse >= 0xbb)
					{
						mLevelUpScanHeight = aPulse - 1;
						if (mLevelUpScanHeight <= 0)
						{
							mLevelUpScanHeight = 0;
							mLevelUpScanY = 0;
						}
					}
				}
			}
		}
		if (m1b2c == 2 || m1b2c == 4)
		{
			mLeftArmX -= 4;
			if (mLeftArmX < -IMAGE_UPGRADE_ARM->mWidth)
				mLeftArmX = -IMAGE_UPGRADE_ARM->mWidth;
			mRightArmX += 4;
			if (mRightArmX > IMAGE_UPGRADE_HEAD->GetCelWidth() + 0x280)
				mRightArmX = IMAGE_UPGRADE_HEAD->GetCelWidth() + 0x280;
		}
	}

	// the panel slides away
	if (mWarpGatePicked || (mLevelUpPanelY < 0 && mLevelUpPanelY > -mLevelUpPanelHeight && !mWarpFlight && !mAtWarpGate))
	{
		mLevelUpPanelY -= 2;
		if (mLevelUpPanelY <= -mLevelUpPanelHeight)
		{
			mLevelUpPanelY = -mLevelUpPanelHeight;
			ShowThemeButtons();
		}
	}

	// the warp stars fall
	for (unsigned int i = 0; i < mWarpStars.size(); i++)
	{
		WarpStar* aStar = mWarpStars[i];
		aStar->mAnim.Update();
		aStar->mY += aStar->mSpeed;
		if ((float) (mHeight + 0x32) < aStar->mY)
			aStar->mY = (float) (((int) AstroRand() % 500000000) % 0x32 - 100);
	}

	if (mWarpCharging)
	{
		// the charge-up: the screen shakes, then the warp starts
		mWarpShakeX = ((int) AstroRand() % 500000000) % 0xb - 5;
		mWarpChargeAlpha += 2;
		if (mWarpChargeAlpha > 0xff)
		{
			mWarpChargeAlpha = 0xff;
			mWarpActive = true;
			mWarpFlight = false;
			mWarpCharging = false;
			if (mApp->mHyperspaceLoop != NULL)
				mApp->mHyperspaceLoop->Play(true, false);
			if (mApp->mEngineLoop != NULL)
				mApp->mEngineLoop->Stop();
			Fire(9);
		}
		if (mWarpChargeAlpha == 0x96)
			mApp->PlaySample(SOUND_WARP);
		UpdateStreaks();
		return false;
	}

	if (!mWarpActive)
		return false;

	if (!mWarpFlight)
	{
		if (!mAtWarpGate && !mWarpGatePicked)
		{
			// the screen fades in white while the field flies away
			if (mUpdateCnt % mWarpFrameInterval == 0 && mOverlayFillColor.mAlpha >= 10 && mShipFrame < 7)
			{
				mShipFrame++;
				if (mShipFrame >= 5)
				{
					mOverlay.SetFill(255, 255, 255, 255);
					for (unsigned int i = 0; i < mFloatTexts.size(); i++)
						delete mFloatTexts[i];
					mFloatTexts.clear();
				}
			}
			mOverlayFillColor.mAlpha++;
			if (mOverlayFillColor.mAlpha > 0x28)
			{
				mOverlayFillColor.mAlpha = 0xff;
				mLevelUpTextShown = false;
				mOverlay.SetFill(mOverlayFillColor.mRed, mOverlayFillColor.mGreen, mOverlayFillColor.mBlue, mOverlayFillColor.mAlpha);
				mWarpFlight = true;
				mShipInWarp = true;
				mShipFrame = 2;
				for (unsigned int i = 0; i < 100 && i < mStreaks.size(); i++)
					mStreaks[i].mFlag28 = false;
				mWarpEngaged = true;
				mApp->PlaySample(SOUND_WARP_ENGAGED);
				ClearField();
				mWarpShipY = -312.0f; // 0xc39c0000 (the asm at 0x46ae88)
				for (int i = 0; i < 15; i++)
				{
					WarpStar* aStar = new WarpStar();
					aStar->mX = 0;
					aStar->mY = 0;
					aStar->mField5C = 0;
					aStar->mSpeed = 0;
					aStar->mImage = NULL;
					if (i < 5)
						aStar->mImage = IMAGE_WARP_FX_LARGE;
					else if (i < 10)
						aStar->mImage = IMAGE_WARP_FX_MED;
					else
						aStar->mImage = IMAGE_WARP_FX_SMALL;
					aStar->mAnim.SetStopped(false);
					aStar->mAnim.SetDelay(7);
					aStar->mAnim.SetNumFrames(aStar->mImage->mNumCols);
					int aDraw = (int) AstroRand() % 500000000;
					if (aDraw % 2 == 0)
						aStar->mX = (float) (((int) AstroRand() % 500000000) % (mWidth - 0x226) + 0x226);
					else
						aStar->mX = (float) (((int) AstroRand() % 500000000) % 0xdc);
					aStar->mY = (float) (((int) AstroRand() % 500000000) % 0x32 - 100);
					aStar->mField5C = 0;
					aStar->mSpeed = (float) (((int) AstroRand() % 500000000) % 0x51) * 0.01f + 0.4f;
					mWarpStars.push_back(aStar);
				}
				if (mBestLevelReached < 4)
					mShipX = IMAGE_SLOW_WARP_GATE->GetCelWidth() / 2 - 0x22 + mSlowGateX;
				else
					mShipX = 0x159;
				if (m1b2c == 2 || m1b2c == 4)
				{
					mWarpShipY = 423.0f;
					mApp->ShowUpgradeDialog();
					mApp->mWidgetManager->BringToFront(&mOverlay);
				}
			}
			mWarpGlowAlpha += mWarpGlowStep;
			if (mWarpGlowAlpha > 0xff)
				mWarpGlowAlpha = 0xff;
			return false;
		}
		else if (!mAtWarpGate)
		{
			if (mWarpGatePicked)
			{
				// the screen fades to white and the next stage begins
				mOverlayFillColor.mRed = 0xff;
				mOverlayFillColor.mGreen = 0xff;
				mOverlayFillColor.mBlue = 0xff;
				mOverlayFillColor.mAlpha += 6;
				if (mOverlayFillColor.mAlpha > 0xff)
				{
					mOverlayFillColor.mAlpha = 0xff;
					mOverlayFillColor.mGreen = 0xff;
					mOverlayFillColor.mBlue = 0xff;
					mOverlayFillColor.mRed = 0xff;
					mWarpActive = false;
					mWarpGatePicked = false;
					if (mApp->mHyperspaceLoop != NULL)
						mApp->mHyperspaceLoop->Stop();
					if (mApp->mEngineLoop != NULL)
						mApp->mEngineLoop->Play(true, false);
					AstroRand();
					int aSceneryId = mStageNumber % 3 + 0x18b;
					std::string aName = GetStringIdById(mSceneryLayer.mI30);
					mApp->mResourceManager->DeleteImage(aName);
					mSceneryLayer.mImage = LoadImageById(mApp->mResourceManager, aSceneryId);
					mSceneryLayer.mI30 = aSceneryId;
					mSceneryLayer.mF24 = 0;
					mSceneryLayer.mF28 = -1.0f;
					mSceneryLayer.mCur[0] = 0;
					mSceneryLayer.mCur[1] = (float) (mSceneryLayer.mImage->mHeight - 0x1e0);
					mSceneryLayer.mCur[2] = 640.0f;
					mSceneryLayer.mCur[3] = 480.0f;
					mSceneryLayer.mNext[0] = 0;
					mSceneryLayer.mNext[1] = 0;
					mSceneryLayer.mNext[2] = 640.0f;
					mSceneryLayer.mNext[3] = 0;
					if (OpenStoryScreen())
						return true;
					mApp->PlaySample(SOUND_GET_READY);
					LoadLevel(true);
					mLevelUpPanelY = -mLevelUpPanelHeight;
				}
				mOverlay.SetFill(mOverlayFillColor.mRed, mOverlayFillColor.mGreen, mOverlayFillColor.mBlue, mOverlayFillColor.mAlpha);
			}
		}
		else
		{
			// the warp has come out: the gate lights up
			mWarpShakeX = ((int) AstroRand() % 500000000) % 4 - 2;
			mWarpGateAlpha += 2;
			if (mWarpGateAlpha > 0xff)
				mWarpGateAlpha = 0xff;
			if (!mPointerOverEasyGate)
			{
				mEasyGateGlow -= 8;
				if (mEasyGateGlow < 0)
					mEasyGateGlow = 0;
			}
			else
			{
				mEasyGateGlow += 8;
				if (mEasyGateGlow > 0xff)
					mEasyGateGlow = 0xff;
			}
			if (!mPointerOverSlowGate)
			{
				mSlowGateGlow -= 8;
				if (mSlowGateGlow < 0)
					mSlowGateGlow = 0;
			}
			else
			{
				mSlowGateGlow += 8;
				if (mSlowGateGlow > 0xff)
					mSlowGateGlow = 0xff;
			}
			if (!mPointerOverFastGate)
			{
				mFastGateGlow -= 8;
				if (mFastGateGlow < 0)
					mFastGateGlow = 0;
			}
			else
			{
				mFastGateGlow += 8;
				if (mFastGateGlow > 0xff)
					mFastGateGlow = 0xff;
			}
			if (m1b2c == 2 || m1b2c == 4)
			{
				if (!mUpgradeDialogDone && !mWarpGatePicked)
				{
					if (mLeftArmX < mLeftArmStopX)
					{
						mLeftArmX += 4;
						if (mLeftArmX >= mLeftArmStopX)
							mLeftArmX = mLeftArmStopX;
					}
					else
						mLeftHeadAnim.Update();
					if (mRightArmX <= mRightArmStopX)
					{
						mRightHeadAnim.Update();
						return false;
					}
					mRightArmX -= 4;
					if (mRightArmX <= mRightArmStopX)
					{
						mRightArmX = mRightArmStopX;
						return false;
					}
				}
			}
		}
	}
	else
	{
		// the flight through the warp: the screen darkens and shakes until the end of the tunnel
		// (x87 at 24 bits: every result is rounded to a float, see GAME_NOTES section 4)
		float aStep = (mWarpShipY < 0.0f) ? 0.0f : mWarpShipY * 0.001f;
		float aNew = (1.5f - aStep) + mWarpShipY;
		mWarpShipY = aNew;
		if (aNew > 423.0f)
			mWarpShipY = 423.0f;
		mWarpShakeX = ((int) AstroRand() % 500000000) % 4 - 2;
		if (mOverlayFillColor.mAlpha > 0)
			mOverlayFillColor.mAlpha--;
		if (mOverlayFillColor.mBlue > 0)
			mOverlayFillColor.mBlue--;
		if (mOverlayFillColor.mGreen > 0)
			mOverlayFillColor.mGreen--;
		mOverlay.SetFill(mOverlayFillColor.mRed, mOverlayFillColor.mGreen, mOverlayFillColor.mBlue, mOverlayFillColor.mAlpha);
		if (mWarpShipY >= 423.0f && mOverlayFillColor.mAlpha == 0 && mOverlayFillColor.mGreen == 0 && mOverlayFillColor.mBlue == 0)
		{
			mWarpShipY = 423.0f;
			mAtWarpGate = true;
			mWarpFlight = false;
			mWarpGateAlpha = 0;
			mFastGateGlow = 0;
			mSlowGateGlow = 0;
			mEasyGateGlow = 0;
			return false;
		}
	}
	return false;
}
