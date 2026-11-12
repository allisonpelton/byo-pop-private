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
			if (aPiece != NULL && aPiece->m9C)
			{
				if (mUpdateCnt % 3 == 0)
				{
					aPiece->mA0++;
					if (aPiece->mA0 >= 6)
					{
						m15e44++;
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
	m15f69 = true;
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
		m15eec = m15ef0 + mScore;
		m15ee8 = mScore;

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
					if (!aPiece->m70)
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
			aPiece->m70 = true;
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
		if (!m191c)
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
	m1751 = true;
	if (!m191c && !mIsPuzzle && aIs3D)
		mApp->PlaySample(SOUND_GET_READY);
	if (mIsAction)
		m15f88 = 200;
	m15f90 = 0.0;
	m15f98 = 0.0;
	m15fa0 = 0.0;
	m15e04 = 0.0f;
	if (!m191c)
	{
		mState = 0xd;
		if (!aIs3D)
		{
			EndInterfaceRestore();
			if (!mIsPuzzle)
			{
				mDropDelay = 0x96;
				m1768 = 0x50;
			}
			mFadeIn = 1.0f;
			Show();
		}
	}
	else
	{
		delete m165bc;
		m165bc = NULL;
		mEffectOverlay->InitFace();
		mState = 0x12;
		HideNow();
	}
}

// the next backdrop after a warp: the next one of the list, or a random one when the profile asks for that
void Board::PickNextBackdrop()
{
	if (!mApp->mProfile->mFlag78)
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
				m15f6c++;
			else
				m15f6c = (m15f6c + 1) % 5;
			StartPrepareLevelImages();
			return;
		}
	}
	if (m15df0 > 0)
	{
		m15df0--;
		if (m15df0 == 0)
			DoHyperspace();
	}
	m15f90 = m15f90 + 0.02;
	if (1.0 < m15f90)
		m15f90 = 1.0;
	m15da8 = (float)((double)m15da8 + 0.0010000000474974513);
	if (m15da0 < 1)
		m15d9c = (float)((double)m15d9c + 0.009999999776482582);
	else
		m15da0--;
	if (m15da4 < 1)
	{
		m15db0.IncInVal(0.05);
		m15de0 = m15db0.GetOutVal() * 0.01 + m15de0;
		if (1.0 < m15de0)
			m15de0 = 1.0;
	}
	else
		m15da4--;

	// the mesh: the 46 by 46 points inside the border of the 48 by 48 mesh; each turns around and drifts inward, the faster the
	// farther it is from the edge of the mesh
	const float aTwoPi = 6.283180236816406f;
	for (int r = 1, s = 46; s > 0; r++, s--)
	{
		WhirlPoint* aPoint = &mWhirlMesh[r][1];
		for (int i = 1, j = 46; j > 0; i++, j--, aPoint++)
		{
			int aDist = std::min(std::min(i, j), std::min(r, s));
			if (m15da0 == 0)
			{
				float aStep = (float)((double)aDist * 0.35 * (double)m15d9c);
				float aRadius = (float)((double)aPoint->mRadius - (double)aStep);
				aPoint->mRadius = aRadius;
				if (!(0.0f <= aRadius))
					aPoint->mRadius = 0.0f;
				float aTurn = (float)((double)aDist * 0.001 * (double)m15d9c);
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
			float aX = gCosTable[anIndex] * aPoint->mRadius * m15da8;
			aPoint->mX = (float)((double)aX + 512.0);
			float aY = gSinTable[anIndex] * aPoint->mRadius * m15da8;
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
	if (mIsPuzzle && mIsSecret && m1918 == 0)
		mScore = m1774;
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
					m1910 = 0x19;
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
