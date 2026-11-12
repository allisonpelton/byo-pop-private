// BoardWhirl.cpp: the clearing of gems, the warp between levels and the hints (Sexy::Board::UpdateClearing,
// EndWhirlpool, UpdateWhirlpool, TryShowHint; 0x5aab8a-0x5ab4d6).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "GalaxyParser.h"
#include "PuzzleStateOverlay.h"
#include "EffectOverlay.h"
#include "Hyperspace.h"
#include "StarField.h"
#include "Common.h"
#include "WidgetManager.h"
#include <stdlib.h>
#include <algorithm>
#include <utility>

using namespace Sexy;

extern float gCosTable[];
extern float gSinTable[];

// 0x5aab8a: the gems that are being cleared fade out (6 steps, one every 3 updates); when all are gone the board refills
void Board::UpdateClearing()
{
	bool aDone = true;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL && aPiece->mInSet)
			{
				if (mUpdateCnt % 3 == 0)
				{
					aPiece->mShrinkStep++;
					if (aPiece->mShrinkStep >= 6)
					{
						mGemsCleared++;
						if (aPiece == mSelectedPiece)
							mSelectedPiece = NULL;
						delete aPiece;
						mBoard[aRow][aCol] = NULL;
					}
					MarkDirtyFull();
				}
				aDone = false;
			}
		}
	}
	UpdateFalling();
	if (aDone)
		DoneClearing();
}

// 0x5aac20: the warp (or the galaxy hop) is over: the next level starts
void Board::EndWhirlpool()
{
	bool aIs3D = mApp->Is3DAccelerated();
	mFadeIn = 1.0f;
	mRedrawBackground = true;
	if (!mIsPuzzle)
	{
		mLevel++;
		mNextLevel++;
		if (mIsEndless)
			AdvanceSweep();
		if (!mCheated)
		{
			int aMode = mApp->mGameMode;
			if (aMode < 4)
			{
				int& aBest = mApp->mProfile->mModeLevel[aMode];
				if (aBest <= mLevel)
					aBest = mLevel;
			}
		}
		CreateFontGlowImages();
		SetupLevelData();
		mLevelGoalScore = mLevelGoalPoints + mScore;
		mLevelStartScore = mScore;

		// the special gems of the old board are kept: they appear again on random squares of the new one
		int aFlameCount = 0;
		int aHyperCount = 0;
		std::vector<std::pair<int, int> > aCells;
		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aCol = 0; aCol < 8; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if (aPiece != NULL)
				{
					if (!aPiece->mFlame)
					{
						if (aPiece->mType == 9)
							aHyperCount++;
					}
					else
						aFlameCount++;
				}
				aCells.push_back(std::make_pair(aCol, aRow));
			}
		}
		NewBoard();
		for (int i = 0; i < aFlameCount; i++)
		{
			unsigned int aPick = (unsigned int)Rand() % (unsigned int)aCells.size();
			Piece* aPiece = mBoard[aCells[aPick].second][aCells[aPick].first];
			aCells.erase(aCells.begin() + aPick);
			aPiece->mFlame = true;
		}
		for (int i = 0; i < aHyperCount; i++)
		{
			unsigned int aPick = (unsigned int)Rand() % (unsigned int)aCells.size();
			Piece* aPiece = mBoard[aCells[aPick].second][aCells[aPick].first];
			aCells.erase(aCells.begin() + aPick);
			aPiece->mType = 9;
		}
	}
	else if (!mIsSecret)
	{
		if (!mFinale)
		{
			SetupLevelData();
			mPuzzleOverlay->Setup(1);
			SelectPuzzlePod(mPuzzleOverlay->mSelectedPod);
		}
	}
	else
	{
		mLevel++;
		mNextLevel++;
		mSecretRun = 0;
		mLevelValue = mLevelValue + 0.5;
		LoadNextSecretPuzzle();
	}
	mClearBacklog = true;
	if (!mFinale && !mIsPuzzle && aIs3D)
		mApp->PlaySample(SOUND_GET_READY);
	if (mIsAction)
		mDrainGrace = 200;
	mWarpFade = 0.0;
	mBlackHoleAngle = 0.0;
	mBlackHoleSpin = 0.0;
	mInterfaceRestore = 0.0f;
	if (!mFinale)
	{
		mState = 0xd;
		if (!aIs3D)
		{
			EndInterfaceRestore();
			if (!mIsPuzzle)
			{
				mDropDelay = 0x96;
				mLevelTextCountdown = 0x50;
			}
			mFadeIn = 1.0f;
			Show();
		}
	}
	else
	{
		delete mCheatStars;
		mCheatStars = NULL;
		mEffectOverlay->InitFace();
		mState = 0x12;
		HideNow();
	}
}

// port: the next backdrop after a warp: the next one of the list, or a random one when the profile asks for that
void Board::PickNextBackdrop()
{
	if (!mApp->mProfile->mRandomBackdrops)
		mBackdropNum = (mBackdropNum + 1U) % (unsigned int)mApp->mBackdrops.size();
	else
		mBackdropNum = (unsigned int)Rand() % (unsigned int)mApp->mBackdrops.size();
}

// 0x5aafae: the whirlpool of the warp: the mesh of points spirals inward and the levels change underneath
void Board::UpdateWhirlpool()
{
	bool anEnd = false;
	float aFade = (float)((double)mFadeIn - 0.014999999664723873);
	mFadeIn = aFade;
	if (aFade < 0.0f)
		mFadeIn = 0.0f;

	if (mHyperspace != NULL)
	{
		if (mHyperspace->mDone)
		{
			mWidgetManager->RemoveWidget(mHyperspace);
			delete mHyperspace;
			mHyperspace = NULL;
			anEnd = true;
		}
		else if (mHyperspace->mReady)
		{
			if (mHyperspace->mPhase != 2)
				return;
			mHyperspace->mPhase = 3;
			ProcessLevelImages();
			if (!mIsPuzzle)
				PickNextBackdrop();
			else if (!mIsSecret)
				mPlanetNum++;
			else
				mPlanetNum = (mPlanetNum + 1) % 5;
			StartPrepareLevelImages();
			return;
		}
	}
	if (mHyperspaceDelay > 0)
	{
		mHyperspaceDelay--;
		if (mHyperspaceDelay == 0)
			DoHyperspace();
	}
	mWarpFade = mWarpFade + 0.02;
	if (1.0 < mWarpFade)
		mWarpFade = 1.0;
	mWhirlRadiusScale = (float)((double)mWhirlRadiusScale + 0.0010000000474974513);
	if (mWhirlStrengthDelay < 1)
		mWhirlStrength = (float)((double)mWhirlStrength + 0.009999999776482582);
	else
		mWhirlStrengthDelay--;
	if (mInterfaceFlyDelay < 1)
	{
		mInterfaceFlyRamp.IncInVal(0.05);
		mInterfaceFly = mInterfaceFlyRamp.GetOutVal() * 0.01 + mInterfaceFly;
		if (1.0 < mInterfaceFly)
			mInterfaceFly = 1.0;
	}
	else
		mInterfaceFlyDelay--;

	// the mesh: the 46 by 46 points inside the border of the 48 by 48 mesh; each turns around and drifts inward, the faster the
	// farther it is from the edge of the mesh
	const float aTwoPi = 6.283180236816406f;
	for (int r = 1, s = 46; s > 0; r++, s--)
	{
		WhirlPoint* aPoint = &mWhirlMesh[r][1];
		for (int i = 1, j = 46; j > 0; i++, j--, aPoint++)
		{
			int aDist = std::min(std::min(i, j), std::min(r, s));
			if (mWhirlStrengthDelay == 0)
			{
				float aStep = (float)((double)aDist * 0.35 * (double)mWhirlStrength);
				float aRadius = (float)((double)aPoint->mRadius - (double)aStep);
				aPoint->mRadius = aRadius;
				if (!(0.0f <= aRadius))
					aPoint->mRadius = 0.0f;
				float aTurn = (float)((double)aDist * 0.001 * (double)mWhirlStrength);
				aPoint->mAngle = aPoint->mAngle + aTurn;
			}
			if (0.0f <= aPoint->mAngle)
			{
				if (aTwoPi < aPoint->mAngle)
					aPoint->mAngle = aPoint->mAngle - aTwoPi;
			}
			else
				aPoint->mAngle = aPoint->mAngle + aTwoPi;
			int anIndex = (int)((double)aPoint->mAngle * 4096.0 / 6.28318);
			anIndex &= 0x80000fff;
			if (anIndex < 0)
				anIndex = ((anIndex - 1) | 0xfffff000) + 1;
			float aX = gCosTable[anIndex] * aPoint->mRadius * mWhirlRadiusScale;
			aPoint->mX = (float)((double)aX + 512.0);
			float aY = gSinTable[anIndex] * aPoint->mRadius * mWhirlRadiusScale;
			aPoint->mY = (float)((double)aY + 384.0);
		}
	}
	if (anEnd)
		EndWhirlpool();
	MarkDirtyFull();
}

// 0x5ab385
void Board::TryShowHint()
{
	if (mIsPlanetIntro || mState == 0x14)
		return;
	if (mIsPuzzle && mIsSecret && mHintsUsed == 0)
		mScore = mScoreSnapshot;
	if (mPauseCount != 0)
		return;

	KillTutorialButtons();
	if (mState == 0x11)
	{
		if (!mSolutionValid)
			DoNoticeDialog("No Hints", "There are no hints available for this puzzle.");
		else
		{
			UndoUntilSolvable();
			if (!mIsPlanetIntro)
			{
				if (mApp->mProfile->WantsHint(4))
					mMoreHintsDelay = 0x19;
			}
		}
	}
	else if (mState == STATE_IDLE)
	{
		if (!mIsPuzzle)
			ShowHint();
		else
			ShowPuzzleHint();
	}
}
