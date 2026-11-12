// BoardMatch.cpp: finding the sets of three or more identical gems, the swap of two gems and the lightning of the
// hypercube (Sexy::Board::FindSets, UpdateExplodeDelays, SwapPieces, UpdateSwapping, UpdateLightning).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include <math.h>
#include <stdlib.h>
#include <algorithm>

using namespace Sexy;

extern Color* const gPointColors;

// 0x5a7ec1: looks for rows and columns of three or more identical gems and marks them for clearing (flame and
// hyper cubes are made here); returns whether it found any
bool Board::FindSets()
{
	if (mSetsInMove == 0)
	{
		mMoveScore = 0;
	}
	if (mInUndoPreview)
		return false;

	int anOldScore = mScore;
	bool aMulti = false;            // a gem became a flame gem by crossing sets
	bool aMadeHypercube = false;
	int aNumSets = 0;
	int aSumXAll = 0;
	int aMaxSet = 0;
	std::vector<std::pair<int, int> > anExplodeList;    // (col, row) of the flame gems in the sets

	for (int aPass = 0; aPass < 2; aPass++)
	{
		for (int aLine = 0; aLine < 8; aLine++)
		{
			int aRunType = -1;
			int aRunLen = 0;
			int aStartRow = 0;
			int aStartCol = 0;
			for (int aPos = 0; aPos < 8; aPos++)
			{
				int aCol = (aPass == 0) ? aLine : aPos;
				int aRow = (aPass == 0) ? aPos : aLine;
				Piece* aPiece = mBoard[aRow][aCol];
				int aType = -1;
				bool anEnd;
				if (aPiece == NULL)
					anEnd = true;
				else
				{
					// a hyper cube made this update still matches as the gem it was
					aType = (aPiece->mHypercubeTick == mUpdateCnt) ? aPiece->mTypeBeforeHypercube : aPiece->mType;
					if (aType != aRunType || aType > 6)
						anEnd = true;
					else
					{
						aRunLen++;
						anEnd = (aPos == 7);
					}
				}

				if (anEnd)
				{
					if (aRunLen > 2)
					{
						aNumSets++;
						int aSumCol = 0, aSumRow = 0, aSumN = 0, aBest = 0;
						int aSumX = 0, aSumY = 0, aFlames = 0;
						if (aStartRow <= aRow)
						{
							for (int r = aStartRow; r <= aRow; r++)
							{
								if (aStartCol <= aCol)
								{
									for (int c = aStartCol; c <= aCol; c++)
									{
										Piece* aSetPiece = mBoard[r][c];
										if (aSetPiece != NULL && aSetPiece->mType == aRunType)
										{
											if (aSetPiece->mFlame && aSetPiece->mFlameTick != mUpdateCnt)
											{
												// a flame gem in the set explodes
												aFlames++;
												anExplodeList.push_back(std::pair<int, int>(aSetPiece->mCol, aSetPiece->mRow));
												aSetPiece->mFlame = false;
											}
											int aTick = aSetPiece->mMoveTick;
											int aPrevBest = aBest;
											if (aPrevBest < aTick)
											{
												aBest = aTick;
												aSumCol = 0;
												aSumRow = 0;
												aSumN = 0;
											}
											if (aPrevBest <= aTick)
											{
												aSumCol += aSetPiece->mCol;
												aSumRow += aSetPiece->mRow;
												aSumN++;
											}
											aSumX += (int)aSetPiece->mX;
											aSumY += (int)aSetPiece->mY;
											if (!aSetPiece->mInSet || (mIsClassic && mIsSecret))
											{
												if (aSetPiece->mFlameTick != mUpdateCnt)
												{
													aSetPiece->mShrinkStep = 0;
													aSetPiece->mInSet = true;
												}
											}
											else
											{
												// the gem is in two sets: it becomes a flame gem
												aSetPiece->mInSet = false;
												aSetPiece->mFlame = true;
												aSetPiece->mFlameTick = mUpdateCnt;
												mPowerGemsMade++;
												aMulti = true;
												mFlameFromCross = true;
											}
										}
									}
								}
							}
						}

						// the gem that becomes the special one: the middle of the latest moved gems of the set
						int aAvgCol = ((mCrazy ? (aSumN - 1) : 0) + aSumCol) / aSumN;
						int aAvgRow = aSumRow / aSumN;
						Piece* aTarget = mBoard[aAvgRow][aAvgCol];
						if (!mIsClassic || !mIsSecret)
						{
							if (aRunLen < 5)
							{
								if (aRunLen > 3)
								{
									aTarget->mInSet = false;
									aTarget->mFlame = true;
									aTarget->mFlameTick = mUpdateCnt;
									mPowerGemsMade++;
									aMulti = true;
									mFlameFromCross = false;
								}
							}
							else
							{
								bool aNear = false;
								for (int i = 0; i < (int)anExplodeList.size(); i++)
								{
									if (abs(anExplodeList[i].first - aTarget->mCol) < 2 &&
										abs(anExplodeList[i].second - aTarget->mRow) < 2)
										aNear = true;
								}
								if (!aNear)
								{
									aTarget->mInSet = false;
									aTarget->mTypeBeforeHypercube = aTarget->mType;
									aTarget->mType = 9;
									aTarget->mHypercubeTick = mUpdateCnt;
									mHyperCubesMade++;
									aMadeHypercube = true;
								}
							}
						}

						int aAvgX = aSumX / aRunLen;
						aSumXAll += aAvgX;
						int aAvgY = aSumY / aRunLen;
						if (!mIsPuzzle || (mIsSecret && mHintsUsed == 0))
						{
							mSetsInMove++;
							int aBonus = mApp->GetInteger("PowerBonus", 25) * aFlames;
							if (!mIsFinity)
							{
								aBonus += 10;
								switch (mSetsInMove)
								{
								case 1: break;
								case 2: aBonus += 10; break;
								case 3: aBonus += 20; break;
								case 4: aBonus += 30; break;
								case 5: aBonus += 50; break;
								case 6: aBonus += 70; break;
								case 7: aBonus += 100; break;
								case 8: aBonus += 150; break;
								default: aBonus += 200;
								}
								if (aRunLen > 3)
									aBonus += (aRunLen - 3) * 10;
								if (aRunLen > 5)
									aBonus += (aRunLen - 5) * 10;
							}
							int aPoints = (int)(aBonus * mLevelValue);
							if (aPoints > 0)
								DoPoints(aPoints, aAvgX + 42, aAvgY + 34, gPointColors[aRunType]);
							if (mBigMoveTimer != -1)
							{
								double aValue = ((double)mMoveScore / mLevelValue) * 3.0;
								if (aValue < 300.0)
									aValue = 300.0;
								else if (aValue >= 1200.0)
									aValue = 1200.0;
								mBigMoveTimer = (int)aValue;
							}
							if (aMaxSet < aRunLen)
								aMaxSet = aRunLen;
						}
					}
					aStartRow = aRow;
					aStartCol = aCol;
					aRunType = aType;
					aRunLen = 1;
				}
			}
		}
	}

	mChainCount = 0;
	for (int i = 0; i < (int)anExplodeList.size(); i++)
	{
		if (i > 0)
			mChainCount++;
		ExplodeAt(anExplodeList[i].first, anExplodeList[i].second);
	}

	if (aNumSets > 0)
	{
		int aPan = GetPanPosition(aSumXAll / aNumSets + 42);
		mHintCountdown = 0;
		int aSound = SOUND_MULTISHOT;
		if (!aMulti)
		{
			aSound = SOUND_TRANSFER;
			if (aNumSets > 1)
				aSound = SOUND_TRANSFER_BIG;
		}
		mApp->PlaySample(aSound, aPan);
		if (aMadeHypercube)
			mApp->PlaySample(SOUND_HYPERGEM_CREATE, aPan);
		int aCombo = mCascadeCount;
		if (aCombo > 5)
			aCombo = 5;
		mApp->PlaySample(aCombo + SOUND_COMBO_1, aPan);
	}

	mMoveScore += mScore - anOldScore;
	int aMoveScore = mMoveScore;
	if (!mIsPuzzle)
	{
		int aThreshold2 = (int)(mLevelValue * 125.0);
		int aThreshold3 = (int)(mLevelValue * 275.0);
		int aThreshold1 = (int)(mLevelValue * 60.0);
		if (aThreshold1 <= aMoveScore && mCheerTier < 1)
			mCheerTier = 1;
		if (aThreshold2 <= aMoveScore && mCheerTier < 2)
			mCheerTier = 2;
		if (aThreshold3 <= aMoveScore && mCheerTier < 3)
			mCheerTier = 3;
	}
	return aNumSets > 0;
}

// 0x5a8551: counts down the delays of the gems that explode in a chain; true when none is waiting
bool Board::UpdateExplodeDelays()
{
	bool aDone = true;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL && aPiece->mExplodeDelay > 0)
			{
				int aLeft = --aPiece->mExplodeDelay;
				if (aLeft == 0)
				{
					ExplodeAt(aPiece->mCol, aPiece->mRow);
					if (mState == STATE_NEW)
						DoneClearing();
				}
				else
					aDone = false;
			}
		}
	}
	return aDone;
}

// 0x5a85bc: starts the swap of two gems (the move of a hyper cube is the electric one)
void Board::SwapPieces(Piece* theFirst, Piece* theSecond)
{
	mHyperSwap = false;
	int aColDist = abs(theFirst->mCol - theSecond->mCol);
	int aRowDist = abs(theFirst->mRow - theSecond->mRow);
	if (aColDist + aRowDist != 1)
	{
		mApp->PlaySample(SOUND_BAD);
		return;
	}

	bool aNormal = true;
	if (!mInUndoPreview)
	{
		Piece* aHyper = theFirst;
		Piece* anOther = theSecond;
		bool aIsHyper = true;
		if (theFirst->mType != 9)
		{
			if (theSecond->mType != 9)
				aIsHyper = false;
			else
			{
				aHyper = theSecond;
				anOther = theFirst;
			}
		}
		if (aIsHyper)
		{
			aNormal = false;
			mHintCountdown = 0;
			mApp->PlaySample(SOUND_ELECTRO_START);
			DoneClearing();
			SolutionCommitMove(mSolutionMove);
			mLightningColor = anOther->mType;
			anOther->mLightningProgress = 0;
			anOther->mLightningHit = true;
			aHyper->mHyperFade = 0;
			aHyper->mHyperRole = 1;
			aHyper->mHyperDelay = 0x23;
			mHyperGlowDelay = 0xf;
			mHyperGlowX = (int)anOther->mX;
			mHyperGlowY = (int)anOther->mY;
			mHyperGlowPhase = 0;
			anOther->mHyperFade = 0;
			anOther->mHyperRole = 2;
			anOther->mHyperDelay = 0xf;
			mHyperSwap = true;
		}
	}
	theFirst->mHintTime = 0;
	theSecond->mHintTime = 0;
	mSwapTime = 0;
	mSwapPiece[0] = theFirst;
	mSwapPiece[1] = theSecond;
	mSwapCenterX = (int)((double)theSecond->mX + (double)theFirst->mX) / 2;
	mSwapCenterY = (int)((double)theFirst->mY + (double)theSecond->mY) / 2;
	theFirst->mSwapOffsetX = (int)theFirst->mX - mSwapCenterX;
	theFirst->mSwapOffsetY = (int)theFirst->mY - mSwapCenterY;
	theFirst->mSwapDX = theSecond->mCol - theFirst->mCol;
	theFirst->mSwapDY = theSecond->mRow - theFirst->mRow;
	theSecond->mSwapOffsetX = (int)theSecond->mX - mSwapCenterX;
	theSecond->mSwapOffsetY = (int)theSecond->mY - mSwapCenterY;
	theSecond->mSwapDX = theFirst->mCol - theSecond->mCol;
	theSecond->mSwapDY = theFirst->mRow - theSecond->mRow;
	theFirst->mMoveTick = mUpdateCnt;
	theSecond->mMoveTick = mUpdateCnt;
	mState = STATE_SWAPPING;
	if (!mHyperSwap && !WillBeValidSwap())
		mState = STATE_SWAP_BACK;
}

// 0x5a888f: the two gems slide past each other; at the end the swap is committed and checked
void Board::UpdateSwapping()
{
	MarkDirtyFull();
	mCountBombs = true;
	if (!mHyperSwap)
		mSwapTime += 5;
	else
		mSwapTime += 4;
	if (mState == STATE_SWAPPING)
	{
		float aFade = (float)(1.0 - (double)mSwapTime / 180.0);
		if (!(aFade >= 0.0f))
			aFade = 0.0f;
		mSwapFade = aFade;
	}
	if (mHyperSwap && mSwapTime >= 0x5a && mLightning.empty())
		AddLightning(0, 0, 0, 0);
	if (!mLightning.empty())
	{
		Lightning* aLightning = mLightning[0];
		float aX1 = mSwapPiece[0]->mX + 42.0f;
		float aY1 = mSwapPiece[0]->mY + 42.0f;
		aLightning->mPoint[0][0] = aX1;
		aLightning->mPoint[0][1] = aY1;
		aLightning->mPoint[0][2] = aX1;
		aLightning->mPoint[0][3] = aY1;
		float aX2 = mSwapPiece[1]->mX + 42.0f;
		float aY2 = mSwapPiece[1]->mY + 42.0f;
		aLightning->mPoint[7][0] = aX2;
		aLightning->mPoint[7][1] = aY2;
		aLightning->mPoint[7][2] = aX2;
		aLightning->mPoint[7][3] = aY2;
	}

	if (mSwapTime == 180)
	{
		CommitSwap();
		if (!mIsHyper || mHintCountdown > 100)
		{
			mCascadeCount = 0;
			mSetsInMove = 0;
		}
		if (!mHyperSwap)
		{
			if (!FindSets())
			{
				if (mState == STATE_SWAP_BACK && !mFreeMode && !mInUndoPreview)
				{
					// no set: the gems slide back
					Piece* aFirst = mSwapPiece[0];
					aFirst->mSwapOffsetX = -aFirst->mSwapOffsetX;
					aFirst->mSwapOffsetY = -aFirst->mSwapOffsetY;
					aFirst->mSwapDX = -aFirst->mSwapDX;
					aFirst->mSwapDY = -aFirst->mSwapDY;
					Piece* aSecond = mSwapPiece[1];
					if (aSecond != NULL)
					{
						aSecond->mSwapOffsetX = -aSecond->mSwapOffsetX;
						aSecond->mSwapOffsetY = -aSecond->mSwapOffsetY;
						aSecond->mSwapDX = -aSecond->mSwapDX;
						aSecond->mSwapDY = -aSecond->mSwapDY;
					}
					mSwapTime = 0;
					if (mApp->mProfile->WantsHint(0))
					{
						mApp->DoDialog(DIALOG_MESSAGE, true, "Illegal Move",
							"Each swap must create a row of three or more identical gems.", "OK", 3);
					}
					mApp->PlaySample(SOUND_BAD);
					PopUndo();
					mState = STATE_COMMIT_SWAP;
				}
				else
				{
					mSwapPiece[0] = NULL;
					mSwapPiece[1] = NULL;
					mState = STATE_IDLE;
				}
			}
			else
			{
				SolutionCommitMove(mSolutionMove);
				mSwapPiece[0] = NULL;
				mSwapPiece[1] = NULL;
				mState = STATE_CLEARING;
				mNoMoveYet = false;
			}
		}
		else
		{
			mState = STATE_HYPER_SWAP_DONE;
			mSwapPiece[0] = NULL;
			mSwapPiece[1] = NULL;
		}
	}
	else
	{
		double aFactor = cos((double)mSwapTime * 3.14159 / 180.0);
		Piece* aFirst = mSwapPiece[0];
		aFirst->mX = (float)(int)((double)aFirst->mSwapOffsetX * aFactor + (double)mSwapCenterX);
		aFirst->mY = (float)(int)((double)aFirst->mSwapOffsetY * aFactor + (double)mSwapCenterY);
		Piece* aSecond = mSwapPiece[1];
		if (aSecond != NULL)
		{
			aSecond->mX = (float)(int)((double)aSecond->mSwapOffsetX * aFactor + (double)mSwapCenterX);
			aSecond->mY = (float)(int)(aFactor * (double)aSecond->mSwapOffsetY + (double)mSwapCenterY);
		}
	}
}

// 0x5a8c66: the lightning of a hyper cube jumps from gem to gem of the chosen colour; each gem it reaches explodes
void Board::UpdateLightning()
{
	bool a3D = mApp->Is3DAccelerated();
	int aHitCount = 0;          // gems the lightning has reached and not yet exploded
	int aNewHitCount = 0;       // the ones it reached just now
	int anUnhitCount = 0;       // gems of the colour it has not reached
	bool aHyperActive = false;
	bool aClearing = false;
	Piece* anUnhit[64];
	Piece* aNewHit[64];
	Piece* aHit[64];

	UpdateExplodeDelays();
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;
			if (aPiece->mExplodeDelay > 0)
				aClearing = true;
			if (aPiece->mHyperRole == 1)
				aHyperActive = true;
			if (!aPiece->mLightningHit)
			{
				if (aPiece->mType == mLightningColor)
					anUnhit[anUnhitCount++] = aPiece;
			}
			else
			{
				float aProgress = (float)((double)aPiece->mLightningProgress + 0.015);
				aPiece->mLightningProgress = aProgress;
				if (aProgress <= 1.0f)
				{
					if (aProgress < 0.04f)
						aNewHit[aNewHitCount++] = aPiece;
					aHit[aHitCount++] = aPiece;
				}
				else
				{
					if (!aPiece->mFlame)
					{
						int aBonus = (int)(mApp->GetInteger("ElectNormalBonus", 20) * mLevelValue);
						DoPoints(aBonus, (int)(42.0 + aPiece->mX), (int)(aPiece->mY + 42.0 - 8.0),
							gPointColors[aPiece->mType]);
						SmallExplodeAt(aCol, aRow);
					}
					else
					{
						int aBonus = (int)(mApp->GetInteger("ElectPowerBonus", 50) * mLevelValue);
						DoPoints(aBonus, (int)(42.0 + aPiece->mX), (int)(aPiece->mY + 42.0 - 8.0),
							gPointColors[aPiece->mType]);
						mChainCount = 0;
						ExplodeAt(aCol, aRow);
					}
					mApp->PlaySample(SOUND_ELECTRO_EXPLODE);
				}
			}
		}
	}

	// the lightning reaches one more gem (at random, and not every update once it is running)
	bool aSkip = (anUnhitCount < 1) ||
		(!mLightning.empty() && Rand() % (20 / (aHitCount + 1) + 5) != 0);
	if (!aSkip)
	{
		Piece* aTarget = NULL;
		Piece* aFrom = NULL;
		bool aRandomTarget = false;
		if (aNewHitCount < 1)
		{
			if (aHitCount > 0)
				aFrom = aHit[Rand() % aHitCount];
			else
				aRandomTarget = true;
		}
		else
			aFrom = aNewHit[Rand() % aNewHitCount];
		if (aFrom == NULL)
			aRandomTarget = true;

		if (aRandomTarget)
			aTarget = anUnhit[Rand() % anUnhitCount];
		else
		{
			// (the original compares each gem's distance with 0x7fffffff, so the last one wins)
			for (int i = 0; i < anUnhitCount; i++)
				aTarget = anUnhit[i];
			AddLightning((int)aTarget->mX + 42, (int)aTarget->mY + 42, (int)aFrom->mX + 42, (int)aFrom->mY + 42);
			mApp->PlaySample(SOUND_ELECTRO_PATH);
		}
		aTarget->mLightningHit = true;
	}

	// the lightning's bolts flicker and fade
	for (int i = 0; i < (int)mLightning.size(); i++)
	{
		Lightning* aLightning = mLightning[i];
		float aProgress = (float)((double)aLightning->mProgress + 0.012);
		aLightning->mProgress = aProgress;
		if (aProgress <= 1.0f)
		{
			float aFade = (float)(1.0 - (1.0 - (double)aProgress) * 3.0);
			if (aFade < 0.0f)
				aFade = 0.0f;
			if (mUpdateCnt % 4 == 0)
			{
				float aX0 = aLightning->mPoint[0][0];
				float aY0 = aLightning->mPoint[0][1];
				float aX7 = aLightning->mPoint[7][0];
				float aY7 = aLightning->mPoint[7][1];
				for (int k = 0; k < 8; k++)
				{
					float aT = (float)((double)k / 7.0);
					float aProfile = (float)(1.0 - (double)fabsf((float)(1.0 - ((double)aT + (double)aT))));
					double anInv = 1.0 - (double)aT;
					float aR1 = GetRandFloat();
					float aPX = (float)((double)aX0 * (double)(float)anInv + (double)aT * (double)aX7 +
						((double)aLightning->mSwingX * (double)aFade + (double)aR1 * 24.0) * (double)aProfile);
					float aR2 = GetRandFloat();
					float aPY = (float)((double)aY0 * (double)(float)anInv + (double)aT * (double)aY7 +
						((double)aLightning->mSwingY * (double)aFade + (double)aR2 * 24.0) * (double)aProfile);
					float* aPoint = aLightning->mPoint[k];
					if (k == 0 || k == 7)
					{
						aPoint[0] = aPX;
						aPoint[1] = aPY;
						aPoint[2] = aPX;
						aPoint[3] = aPY;
					}
					else
					{
						float aJitter = a3D ? 18.0f : 12.0f;
						aPoint[0] = GetRandFloat() * aJitter + aPX;
						aPoint[1] = GetRandFloat() * aJitter + aPY;
						aPoint[2] = GetRandFloat() * aJitter + aPX;
						aPoint[3] = GetRandFloat() * aJitter + aPY;
					}
				}
			}
		}
		else
		{
			delete aLightning;
			mLightning.erase(mLightning.begin() + i);
			i--;
		}
	}

	if (!aClearing && aHitCount == 0 && anUnhitCount == 0 && mLightning.empty() && !aHyperActive)
	{
		for (int aRow = 0; aRow < 8; aRow++)
			for (int aCol = 0; aCol < 8; aCol++)
				if (mBoard[aRow][aCol] != NULL)
					mBoard[aRow][aCol]->mVY = 0;
		mDropDelay = 0x32;
		DoneClearing();
		mState = STATE_NEW;
	}
}
