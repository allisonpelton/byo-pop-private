// The board's update: one step of everything that moves (see Board.h).

#include "Board.h"
#include "AlchemyApp.h"
#include "Piece.h"
#include "Wizard.h"
#include "Effects.h"
#include "SidePanel.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "Common.h"
#include "PortCompat.h"
#include "Compat2004.h"
#include <math.h>
#include "GameText.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

using namespace Sexy;

// The order in which the pixels of a newly lit tile light up (the original keeps it as a table, 0x4de400): 45 groups of
// pixels that appear together (the 2704 pixels of a tile), all but the last ended by -1 and the last directly by -2, so
// the dissolve takes 45 steps; from the middle outward, with a bell of group sizes. The table is read from the player's
// own Alchemy.exe (GameText.h, not in the repository); without it this one stands in, made the same way from a fixed seed.
static const std::vector<int>& LightOrder()
{
	static std::vector<int> aOrder;
	if (!aOrder.empty())
		return aOrder;

	// the original's own table, from the player's Alchemy.exe
	aOrder = GameTable(0x4de400);
	if (!aOrder.empty())
		return aOrder;

	const int aSize = Board::CELL;
	const int aGroups = 45;
	const int aTotal = aSize * aSize;

	std::vector<std::pair<int, int> > aKeys;
	unsigned int aSeed = 12345;
	for (int y = 0; y < aSize; y++)
		for (int x = 0; x < aSize; x++)
		{
			int aDX = x - aSize / 2;
			int aDY = y - aSize / 2;
			aSeed = aSeed * 1664525u + 1013904223u;
			int aJitter = (int) ((aSeed >> 16) % 90);
			aKeys.push_back(std::make_pair(aDX * aDX + aDY * aDY + aJitter, y * aSize + x));
		}
	std::sort(aKeys.begin(), aKeys.end());

	std::vector<double> aWeights;
	double aWeightSum = 0;
	for (int i = 0; i < aGroups; i++)
	{
		double aSin = sin(3.14159265358979 * (i + 0.5) / aGroups);
		aWeights.push_back(aSin * aSin);
		aWeightSum += aSin * aSin;
	}
	std::vector<int> aSizes;
	int aSum = 0;
	for (int i = 0; i < aGroups; i++)
	{
		int aGroupSize = (int) (aWeights[i] / aWeightSum * aTotal);
		if (aGroupSize < 1)
			aGroupSize = 1;
		aSizes.push_back(aGroupSize);
		aSum += aGroupSize;
	}
	aSizes[aGroups / 2] += aTotal - aSum;

	int aNext = 0;
	for (int i = 0; i < aGroups; i++)
	{
		for (int j = 0; j < aSizes[i]; j++)
			aOrder.push_back(aKeys[aNext++].second);
		aOrder.push_back(i < aGroups - 1 ? -1 : -2);
	}
	return aOrder;
}

// 0x406f2a
void Board::Update()
{
	if (mPause > 0 || mTrialPause > 0)
		return;

	Widget::Update();

	// the test hook's input: "Move: n " plays the n-th place the piece fits (row by row), 0 is the forge
	if (mState == STATE_PLAYING && mCurPiece != NULL)
	{
		size_t aPos = mScript.find("Move: ");
		if (aPos != std::string::npos)
		{
			size_t aEnd = mScript.find(" ", aPos + 6);
			if (aEnd != std::string::npos)
			{
				int aMove = atoi(mScript.substr(aPos + 6, aEnd - aPos - 6).c_str());
				if (aMove == 0)
					UseForgeClicked();
				else
				{
					int aCount = 0;
					for (int aRow = 0; aRow < ROWS; aRow++)
						for (int aCol = 0; aCol < COLS; aCol++)
							if (mValid[aRow][aCol] && ++aCount == aMove)
							{
								mCurPiece->mCol = aCol;
								mCurPiece->mRow = aRow;
								PlaceCurrentPiece();
							}
				}
				mScript.erase(0, aEnd + 1);
			}
		}
	}

	// the forge is nearly full
	if (!mTimeTrial && mState != STATE_GAME_OVER && mProgress > 2.3 && mUpdateCnt - mLastWarning > 0x95)
	{
		mApp->PlaySample(SOUND_WARNING);
		mLastWarning = mUpdateCnt;
	}

	// the wizard: speaks when he is in, goes when he is out
	if (mHelpShown)
	{
		if (mHelpBox->mFadeDir == 1 && mHelpBox->mAlpha == 1.0)
		{
			// 0x407130: the game's end speaks with track 2 and its sound, anything else (a cleared board) with track 1
			if (mState == STATE_GAME_OVER || mState == STATE_ENDING)
			{
				mHelpBox->Talk(2);
				mApp->PlaySample(SOUND_GAMEOVER);
			}
			else
			{
				mHelpBox->Talk(1);
				mApp->PlaySample(SOUND_LEVELCOMPLETE);
			}
			mHelpBox->mFadeDir = 0;
		}
		if (mHelpBox->mFadeDir == -1 && mHelpBox->mAlpha == 0.0)
		{
			mWidgetManager->RemoveWidget(mHelpBox);
			mHelpBox->mFadeDir = 0;
			mHelpShown = false;
		}
	}

	// the sparks of a bomb
	for (std::list<BombSpark>::iterator anItr = mSparks.begin(); anItr != mSparks.end();)
	{
		BombSpark& aSpark = *anItr;
		aSpark.mX += aSpark.mVX;
		aSpark.mY += aSpark.mVY;
		aSpark.mVY += 0.1;
		if (mUpdateCnt % 4 == 0)
		{
			bool isBig = (CrtRand() % 2) == 0;
			mApp->mEffects->AddSparkle((int) (mX + aSpark.mX), (int) (mY + aSpark.mY), true, isBig);
		}
		aSpark.mLife--;
		if (aSpark.mLife < 1)
			anItr = mSparks.erase(anItr);
		else
			++anItr;
	}

	// time trial: the forge fills by itself
	if (mTimeTrial && mState == STATE_PLAYING)
	{
		mProgress = mProgRate * 0.01 + mProgress;
		mProgRate = mProgDecay * 0.01 + mProgRate;
		if (mProgress < 3.0)
		{
			mFullCount = 0;
			mApp->mSidePanel->SetFlashing(false);
		}
		else
		{
			mApp->mSidePanel->SetFlashing(true);
			mFullCount++;
			if (mFullCount > 0x95)
			{
				RecordForge(true);
				GameOver();
			}
		}
	}

	// the tile that was lit dissolves in
	if (mLightOn)
	{
		mLightMix += mLightMixStep;
		if (mLightMix < 1.0)
		{
			if (mLightMix < 0.0)
				mLightMix = 0.0;
		}
		else
		{
			mLightMix = 1.0;
			mLightMixStep = -mLightMixStep;
		}

		ulong* aSrc = mLightLit->GetBits();
		ulong* aDst = mLightOut->GetBits();
		const std::vector<int>& anOrder = LightOrder();
		if (!mLightFading)
		{
			while (anOrder[mLightPos] >= 0)
				mLightPos++;
			if (anOrder[mLightPos] == -2)
				mLightFading = true;
			else
				mLightPos++;
		}
		else
		{
			mLightFade++;
			if (mLightFade == 5)
				mLightOn = false;
		}

		// the newest groups are brightest and fade back to the picture of the lit tile
		int anIdx = mLightPos - 2;
		int aGroup = 5 - mLightFade;
		int anAdd = aGroup * 0x32;
		while (aGroup >= 0 && anIdx >= 0)
		{
			while (anIdx >= 0)
			{
				int aPix = anOrder[anIdx];
				anIdx--;
				if (aPix < 0)
					break;

				uint32_t aColor = (uint32_t) aSrc[aPix];
				int aBlue = (int) (aColor & 0xff) + anAdd;
				int aGreen = (int) ((aColor >> 8) & 0xff) + anAdd;
				int aRed = (int) ((aColor >> 16) & 0xff) + anAdd;
				if (aRed > 0xff)
					aRed = 0xff;
				if (aGreen > 0xff)
					aGreen = 0xff;
				if (aBlue > 0xff)
					aBlue = 0xff;
				aDst[aPix] = 0xff000000u | (aRed << 16) | (aGreen << 8) | aBlue;
			}
			aGroup--;
			anAdd -= 0x32;
		}
		mLightOut->BitsChanged();
		MarkDirty();
	}

	// sparkles over the level bar after a board was cleared
	if (mSparkle > 0)
	{
		if (mUpdateCnt % 8 == 0)
		{
			int aY = CrtRand() % 0x12 + 0xe;
			int aX = CrtRand() % 0x12 + 0xb1 + ((mBoardsCleared - 1) % 7) * 0x45;
			mApp->mEffects->AddSparkle(aX, aY, false, true);
		}
		mSparkle--;
	}

	// the explosion of a bomb
	if (mBombOn && mUpdateCnt % 4 == 0)
	{
		mBombFrame++;
		if (mBombFrame > 0xe)
			mBombOn = false;
		MarkDirty();
	}

	// the fuse of a bomb to place
	if (mCurPiece != NULL && mCurPiece->mBomb && mUpdateCnt % 8 == 0)
	{
		mCurPiece->mShape = (mCurPiece->mShape + 1) % 2;
		MarkDirty();
	}

	if (mState == STATE_CLEARING)
	{
		// the full row and column flash, then go
		mAnimPct += mAnimDir * 2;
		if (mAnimPct < 100)
		{
			if (mAnimPct < 0 && !CheckAllLit())
			{
				mState = STATE_PLAYING;
				if (mTutorial)
					AdvanceTutorial();
			}
		}
		else
		{
			mAnimDir = -1;
			mAnimPct = 100;
			if (mClearCol != -1)
				for (int aRow = 0; aRow < ROWS; aRow++)
				{
					delete mGrid[aRow][mClearCol];
					mGrid[aRow][mClearCol] = NULL;
					RedrawCell(mClearCol, aRow);
				}
			if (mClearRow != -1)
				for (int aCol = 0; aCol < COLS; aCol++)
				{
					delete mGrid[mClearRow][aCol];
					mGrid[mClearRow][aCol] = NULL;
					RedrawCell(aCol, mClearRow);
				}
			ComputeValid();
		}
		MarkDirty();
	}
	else if (mState == STATE_WON)
	{
		// the pieces go three at a time; when none is left the statistics show
		MarkDirty();
		mAnimPct += mAnimDir * 10;
		if (mAnimPct < 100)
		{
			if (mAnimPct < 0)
			{
				mAnimPct = 0;
				mAnimDir = 1;
				std::vector<std::pair<int, int> > aLeft;
				for (int aRow = 0; aRow < ROWS; aRow++)
					for (int aCol = 0; aCol < COLS; aCol++)
					{
						mFlash[aRow][aCol] = false;
						if (mGrid[aRow][aCol] != NULL)
							aLeft.push_back(std::make_pair(aCol, aRow));
					}

				if (aLeft.empty())
					BoardCleared();
				else
				{
					for (int i = 0; i < 3; i++)
					{
						if (aLeft.empty())
							continue;
						size_t aPick = (size_t) CrtRand() % aLeft.size();
						std::pair<int, int> aCell = aLeft[aPick];
						aLeft[aPick] = aLeft.back();
						aLeft.pop_back();
						mFlash[aCell.second][aCell.first] = true;
					}
				}
			}
		}
		else
		{
			mAnimDir = -1;
			mAnimPct = 100;
			for (int aRow = 0; aRow < ROWS; aRow++)
				for (int aCol = 0; aCol < COLS; aCol++)
					if (mFlash[aRow][aCol])
					{
						delete mGrid[aRow][aCol];
						mGrid[aRow][aCol] = NULL;
						RedrawCell(aCol, aRow);
					}
		}
	}
	else if (mState == STATE_NEXT_BOARD)
	{
		mNextBoardFade += 0.015;
		if (mNextBoardFade >= 1.0)
		{
			mApp->PlaySample(SOUND_GEMCHANGE);
			mBoardsCleared++;
			SetDifficulty();
			mSparkle = 0x96;
			if (mCurPiece != NULL)
				mCurPiece->mFollowMouse = true;
			mHoverX = -1;
			mHoverY = -1;
			mState = STATE_PLAYING;
			RedrawAllCells();
		}
		MarkDirty();
	}

	if (mState == STATE_ENDING)
	{
		// the pieces shrink away
		bool isEmpty = true;
		for (int aRow = 0; aRow < ROWS; aRow++)
			for (int aCol = 0; aCol < COLS; aCol++)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == NULL)
					continue;
				isEmpty = false;
				if (mUpdateCnt % 5 == 0)
					aPiece->mShrink++;
				if (aPiece->mShrink > 0x13)
				{
					delete aPiece;
					mGrid[aRow][aCol] = NULL;
				}
			}
		if (isEmpty)
			mState = STATE_GAME_OVER;
		MarkDirty();
	}
	else if (mState == STATE_GAME_OVER)
	{
		if (mOverFade < 1.0)
		{
			mOverFade += 0.01;
			if (mOverFade >= 1.0)
				mOverFade = 1.0;
			MarkDirty();
		}
		if (mOverTimer > 0)
		{
			mOverTimer--;
			if (mOverTimer == 0)
				mApp->ShowHighScores();
		}
	}

	// the discarded piece flies to the forge and melts there
	if (mFxPiece != NULL)
	{
		MarkDirty();
		mFxPiece->mFlyX += mFxPiece->mFlyVX;
		mFxPiece->mFlyY += mFxPiece->mFlyVY;
		bool hasArrived = (mFxPiece->mFlyVX <= 0.0 && mFxPiece->mFlyX <= 39.0) ||
			(mFxPiece->mFlyVX >= 0.0 && mFxPiece->mFlyX >= 37.0);
		if (hasArrived)
		{
			mFxPiece->mSpeed = 0;
			mFxPiece->mSparkleAcc = 0;
			if (mFxPiece->mShrink < 10000)
			{
				// each update the picture goes a little darker and redder, some pixels slide down, and everything is
				// blurred sideways
				const int aSize = 50;
				const int aNum = aSize * aSize;
				ulong* aBits = mFxImage->GetBits();
				std::vector<int> aOrder(aNum);
				for (int i = 0; i < aNum; i++)
				{
					uint32_t aColor = (uint32_t) aBits[i];
					if ((aColor & 0xff) > 2)
						aColor -= 2;
					if ((aColor & 0xff00) > 0x200)
						aColor -= 0x200;
					if ((aColor & 0xff0000) < 0xf80000)
						aColor += 0x40000;
					aBits[i] = aColor;
					aOrder[i] = i;
				}
				for (int i = 0; i < aNum; i++)
					std::swap(aOrder[i], aOrder[CrtRand() % aNum]);

				int aCount = mFxPiece->mShrink;
				if (aCount > aNum - 1)
					aCount = aNum;
				for (int i = 0; i < aCount; i++)
				{
					int aPix = aOrder[i];
					if (aPix < aSize)
						aBits[aPix] = 0;
					else
						aBits[aPix] = aBits[aPix - aSize];
				}

				std::vector<uint32_t> aBlurred(aNum);
				for (int aRow = 0; aRow < aSize; aRow++)
					for (int aCol = 0; aCol < aSize; aCol++)
					{
						uint32_t aCur = (uint32_t) aBits[aRow * aSize + aCol];
						uint32_t aBlue = (aCur & 0xff) * 0xf000;
						uint32_t aGreen = ((aCur >> 8) & 0xff) * 0xf000;
						uint32_t aRed = ((aCur >> 16) & 0xff) * 0xf000;
						uint32_t anAlpha = (aCur >> 24) * 0xeff0;
						if (aCol > 1)
						{
							uint32_t aLeft = (uint32_t) aBits[aRow * aSize + aCol - 1];
							aBlue += (aLeft & 0xff) * 0x800;
							aGreen += ((aLeft >> 8) & 0xff) * 0x800;
							aRed += ((aLeft >> 16) & 0xff) * 0x800;
							anAlpha += (aLeft >> 24) << 11;
						}
						if (aCol < aSize - 1)
						{
							uint32_t aRight = (uint32_t) aBits[aRow * aSize + aCol + 1];
							aBlue += (aRight & 0xff) * 0x800;
							aGreen += ((aRight >> 8) & 0xff) * 0x800;
							aRed += ((aRight >> 16) & 0xff) * 0x800;
							anAlpha += (aRight >> 24) << 11;
						}
						aBlurred[aRow * aSize + aCol] = ((aGreen >> 8) & 0xffffff00u) | (aRed & 0xffff00ffu) |
							((anAlpha & 0xffff0000u) << 8) | (aBlue >> 16);
					}
				for (int i = 0; i < aNum; i++)
					aBits[i] = aBlurred[i];
				mFxImage->BitsChanged();

				mFxPiece->mShrink += 0x28;
				mFxPiece->mFlyX = 38.0;   // 0x407c3f: rdata 0x4e36d0 (the arrival test above uses 39.0, 0x4e3a78)
				mFxPiece->mFlyY = (double) (mFxPiece->mShrink / 200 + 0x140);
				mFxPiece->mFlyVX = 0;
				mFxPiece->mFlyVY = 0;
			}
			else
			{
				mWidgetManager->RemoveWidget(mFxPiece);
				delete mFxPiece;
				mFxPiece = NULL;
			}
		}
		if (mFxPiece != NULL)
			mFxPiece->Move((int) mFxPiece->mFlyX, (int) mFxPiece->mFlyY);
	}

	// nobody touched anything for 200 updates: a hint
	if (mPristine && mUpdateCnt == 200)
		ShowHint();

	// the hint blinks
	if (mHintTimer > 0)
	{
		mHintTimer--;
		if (mHintTimer % 0x1e == 0)
		{
			mHintOn = !mHintOn;
			MarkDirty();
		}
	}

	// the next piece comes after the discarded one has gone
	if (mNewPieceDelay > 0)
	{
		mNewPieceDelay--;
		if (mNewPieceDelay == 0)
		{
			if (mTutorial && mTutStep == 6)
				AdvanceTutorial();
			else
				NewCurrentPiece();
			MouseMove(mMouseX, mMouseY);
			MarkDirty();
		}
	}
}
