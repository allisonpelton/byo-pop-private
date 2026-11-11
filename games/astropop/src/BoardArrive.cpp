// Board: the handler of event 5 (0x473b20). A brick that was fired up the field has reached the place under the stack.
// The bricks that the player fired in one go are the groups of mFiredGroups; when the first of them arrives all of them are put
// into the field, the sets that they make (the same kind, at least mSetSize together) are taken out, and the Board looks
// at what the arrival did: the sound, the score's bonus, the stack's height, the hint boxes and a field that has no move
// left.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Font.h"
#include "SmallObjects.h"
#include "Res.h"

using namespace Sexy;

// the bricks that flash as targets stop (shared by the placing of a power brick and the end of the arrival)
// port: split out of Board::OnBrickArrived (0x473b20)
static void StopTargetFlashes(std::vector<Brick*>& theTargeted)
{
	for (unsigned int i = 0; i < theTargeted.size(); i++)
		theTargeted[i]->StopFlash();
	theTargeted.clear();
}

// 0x473b20 event 5: a brick has arrived (it has stopped in the field). When nothing else moves the groups that were
// waiting are put in their columns, the sets they make are found and taken, power bricks are placed, and the field is
// looked at again (targets, the stack's height, a move left).
void Board::OnBrickArrived(Brick* theBrick)
{
	std::vector<CellRef> aPlaced;       // the cells the bricks of the groups were put in, in the order of the groups
	bool aFlag = false;                 // "a set was made" (the byte [EBP+0xb] of the original, which also holds the first value of mSupported)

	theBrick->StopShake();
	theBrick->StopFade();

	if (mNewRowBouncing)
	{
		aFlag = theBrick->mSupported;
		theBrick->SetTargetY((int) theBrick->mTargetY);
		if (!aFlag && theBrick->mSupported && theBrick->mSupport == NULL)
		{
			int aRow = 0;
			if ((int) theBrick->mY >= 0x22)
				aRow = ((int) theBrick->mY - 0x22) / 0x19 + 1;
			int aCol = CellColumn(theBrick->mX);
			aRow--;
			while (aRow > 0)
			{
				if (mFieldBricks[aRow][aCol] != NULL)
				{
					theBrick->mSupport = NULL;
					break;
				}
				aRow--;
			}
		}
	}

	if (!mNewRowBouncing && CountMovingBricks(true) == 0)
	{
		if (mTakenBrickCount > 0)
			return;

		// -- 1: a power brick in a group is put into the field and goes off at once
		std::list<std::list<Brick*> >::iterator aGroup = mFiredGroups.begin();
		while (aGroup != mFiredGroups.end())
		{
			if (aGroup->empty())
			{
				aGroup = mFiredGroups.erase(aGroup);
				continue;
			}
			int anX = (int) aGroup->front()->mX;
			int aCol = (anX - 0xa6) / 0x2c;
			int aBottom = FindBottomRow(anX, true);
			for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
			{
				Brick* aBrick = *aBrickIt;
				int aType = aBrick->mType;
				if (aBrick->mMutated != 1 && (aType == 6 || aType == 8 || aType == 10 || aType == 0xe || aType == 0xb || aType == 0xd) &&
					aBrick->mGoesOffOnArrival)
				{
					mFieldBricks[aBottom + 1][aCol] = aBrick;
					FireTo(aBrick, 0);
					StopTargetFlashes(mTargeted);
					HighlightTargets();
					CheckStackHeight();
					CheckShipColumn();
					if (!mPlayStarted)
						Fire(0x2a, 0);
					mPlayStarted = true;
					mStartedFromCheckpoint = false;
					RemoveBrick(aBrick, aCol, aBottom + 1, false);
					DropUnsupported(mTakenCells);
					aGroup->erase(aBrickIt);
					if (aGroup->empty())
						mFiredGroups.erase(aGroup);
					return;
				}
			}
			if (aGroup->empty())
				aGroup = mFiredGroups.erase(aGroup);
			else
				++aGroup;
		}

		// -- 2: the bricks of the groups are put in the field, each group on top of its column
		for (aGroup = mFiredGroups.begin(); aGroup != mFiredGroups.end(); ++aGroup)
		{
			if (aGroup->empty())
				continue;
			int anX = (int) aGroup->front()->mX;
			int aCol = (anX - 0xa6) / 0x2c;
			int aRow = FindBottomRow(anX, true) + 1;
			for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
			{
				mFieldBricks[aRow][aCol] = *aBrickIt;
				CellRef aCell;
				aCell.mCol = aCol;
				aCell.mRow = aRow;
				aPlaced.push_back(aCell);
				FireTo(*aBrickIt, 0);
				aRow++;
			}
			for (int aRowToWobble = 0x12; aRowToWobble > 0; aRowToWobble--)
			{
				if (mFieldBricks[aRowToWobble][aCol] != NULL)
					mFieldBricks[aRowToWobble][aCol]->StartWobble();
			}
		}

		// -- 3: the sets: from each brick that was put down, the bricks of its kind that touch it are gathered
		aFlag = false;
		unsigned int aPlacedIndex = 0;
		for (aGroup = mFiredGroups.begin(); aGroup != mFiredGroups.end(); ++aGroup)
		{
			for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
			{
				int aKind = (*aBrickIt)->GetEffectiveType();
				CellRef aCell = aPlaced[aPlacedIndex];
				if (!HasCell(aCell.mCol, aCell.mRow, mConnectedCells))
				{
					std::vector<CellRef> aBefore = mConnectedCells;
					mConnectedCells.clear();
					CollectConnected(aKind, aCell.mCol, aCell.mRow, 0xffffffff, false, NULL, -1, -1);
					if ((int) mConnectedCells.size() < mSetSize)
						mConnectedCells = aBefore;
					else
					{
						AstroRand();
						aFlag = true;
						AdvanceChain();
						for (unsigned int k = 0; k < mConnectedCells.size(); k++)
						{
							Brick* aBrick = mFieldBricks[mConnectedCells[k].mRow][mConnectedCells[k].mCol];
							if (aBrick->mSpecial && aBrick->mMutated != 1)
								static_cast<KillAllBrick*>(aBrick)->mChainCounted = true;
						}
						mApp->PlaySampleAt(SOUND_EXPLODE1, mConnectedCells[0].mCol);
						for (unsigned int k = 0; k < aBefore.size(); k++)
							mConnectedCells.push_back(aBefore[k]);
					}
				}
				aPlacedIndex++;
			}
		}

		// the tutorial's first hint is dropped when it is the last but one
		if ((mStageNumber == 1 || mTutorialStageForced) && !mDemoPlaying && !aFlag)
		{
			StageBg* aBg = mStageHintPanels[1];
			if (aBg->mNext != NULL && aBg->mNext->mNext == NULL)
			{
				StageBg* aNext = aBg->mNext;
				delete aBg;
				mStageHintPanels[1] = aNext;
				mHintPanels[0] = aNext;
			}
		}

		// -- 4: the bricks of the sets leave the field; they die (event 3), one after the other
		std::vector<Brick*> aTaken;
		for (unsigned int k = 0; k < mConnectedCells.size(); k++)
		{
			aTaken.push_back(mFieldBricks[mConnectedCells[k].mRow][mConnectedCells[k].mCol]);
			mFieldBricks[mConnectedCells[k].mRow][mConnectedCells[k].mCol] = NULL;
		}
		int aLastKind = -1;
		for (unsigned int k = 0; k < mConnectedCells.size(); k++)
		{
			Brick* aBrick = aTaken[k];
			int aKind = aBrick->GetEffectiveType();
			mTakenBricks.push_back(aBrick);
			mTakenCells.push_back(mConnectedCells[k]);
			FireTo(aBrick, 3);
			mTakenBrickCount++;
			aLastKind = aKind;
		}

		if ((mStageNumber == 1 || mTutorialStageForced) && !mDemoPlaying)
		{
			if (mConnectedCells.size() >= (unsigned int) mSetSize)
			{
				StageBg* aBg = mStageHintPanels[1];
				if (aBg->mNext != NULL)
				{
					delete aBg->mNext;
					aBg->mNext = NULL;
				}
				aBg = mStageHintPanels[1];
				aBg->mPanelAlphaStep = -8;
				aBg->mPicAlphaStep = -8;
				if (!mPlayStarted)
					Fire(0x2a, 0);
				mPlayStarted = true;
				mStartedFromCheckpoint = false;
			}
		}

		// -- 5: the sound of what happened (and the bonus of a field that is nearly empty after a failed set)
		if (mConnectedCells.size() < 7)
		{
			if (mConnectedCells.size() < (unsigned int) mSetSize)
			{
				if (!mFiredGroups.empty())
				{
					unsigned int aGroupIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) mFiredGroups.size();
					std::list<std::list<Brick*> >::iterator aPick = mFiredGroups.begin();
					while (aGroupIndex > 0)
					{
						aGroupIndex--;
						++aPick;
					}
					unsigned int aBrickIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) aPick->size();
					std::list<Brick*>::iterator aPickBrick = aPick->begin();
					while (aBrickIndex > 0)
					{
						aBrickIndex--;
						++aPickBrick;
					}
					if (!mStartBannerDoneInBonus)
						mApp->PlaySampleAt(SOUND_BLOCK_HIT_BLOCK, ((int) (*aPickBrick)->mX - 0xa6) / 0x2c);
				}
				if (mChainCount > 1 && mKillAllTypes.empty() && mChainCount > 2 && (mStageNumber > 1 || mGameType == 2))
					mFuelCellDue = true;
				if (mKillAllTypes.empty() && !AnyMoveLeft(NULL))
				{
					if (mFreezeUpdatesLeft < 0x3d)
						mRowTimer = (int) ((float) mRowInterval + mStackDanger * 20.0f);
					else
						mFreezeUpdatesLeft = 0x3c;
				}
			}
			else
			{
				mSetMadeSinceTopRow = true;
				unsigned int anIndex = mConnectedCells.empty() ? 0 : (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) mConnectedCells.size();
				if (!HasMarkOfType(aLastKind))
					mApp->PlaySampleAt(SOUND_MADE_SET, mConnectedCells[anIndex].mCol);
				else
					mApp->PlaySampleAt(SOUND_BUBBLY, mConnectedCells[anIndex].mCol);
			}
		}
		else
		{
			mSetMadeSinceTopRow = true;
			unsigned int anIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) mConnectedCells.size();
			if (!HasMarkOfType(aLastKind))
				mApp->PlaySampleAt(SOUND_MADE_BIG_SET, mConnectedCells[anIndex].mCol);
			else
				mApp->PlaySampleAt(SOUND_BUBBLY, mConnectedCells[anIndex].mCol);
		}

		// -- 6: the bricks next to the set are hit, what has lost its support falls, the groups are done
		if (mConnectedCells.size() >= (unsigned int) mSetSize)
			DamageNeighbours(mConnectedCells);
		DropUnsupported(mTakenCells);
		mConnectedCells.clear();
		mFiredGroups.clear();
		StopTargetFlashes(mTargeted);
		HighlightTargets();
		CheckStackHeight();
		CheckShipColumn();
		if (mMode == 3 && mPowerupActive == 1 && mLastPointerX > 0xa5 && mLastPointerX < 0x25e)
			HoverCell((mLastPointerX - 0xa6) / 0x2c, CellRow((float) mLastPointerY), false);
		if (mMode == 2 && !mClearWaitStarted && mPowerupActive == 0 && mTakenBrickCount == 0 && mFiredGroups.empty() && CountMovingBricks(false) == 0)
			BeginClearWait();
		CheckSparseField();

		// a field with no move left (the out flag of AnyMoveLeft is the byte that held "a set was made")
		if (mBonusRoundOn && mKillAllTypes.empty() && !mBricksRevealing && !mStartBannerDoneInBonus && m1b7c == 0 && mTakenBrickCount == 0 && mFiredGroups.empty() && mFallingGroups.empty() &&
			!AnyMoveLeft(&aFlag))
		{
			if (!aFlag)
			{
				m1b7c = 400;
				BuildBanner(GT(0x56568c), m1b7c, FONT_NEURO30);
				mApp->PlaySample(SOUND_NO_MORE_MOVES);
			}
			StartClearCount();
		}

		// the columns that are as high as the ship shake, the others stop
		for (int aCol = 0; aCol < 10; aCol++)
		{
			if (ColumnDepth(aCol) < 0x10)
			{
				Brick* aTop = mFieldBricks[0][aCol];
				if (aTop != NULL && aTop->mShake)
				{
					for (int aRow = 0; aRow < 0x13; aRow++)
					{
						Brick* aBrick = mFieldBricks[aRow][aCol];
						if (aBrick == NULL)
							break;
						aBrick->StopShake();
					}
				}
			}
			else
			{
				for (int aRow = 0; aRow < 0x13; aRow++)
				{
					Brick* aBrick = mFieldBricks[aRow][aCol];
					if (aBrick == NULL)
						break;
					aBrick->StartShake();
				}
			}
		}

		if (mKillAllTypes.empty() && !AnyMoveLeft(NULL))
		{
			if (mFreezeUpdatesLeft < 0x3d)
				mRowTimer = (int) ((float) mRowInterval + mStackDanger * 20.0f);
			else
				mFreezeUpdatesLeft = 0x3c;
		}
	}

	if (mChainCount > 1 && mChainCount != mLastChainCount && mFiredGroups.empty() && mFallingGroups.empty())
		mLastChainCount = mChainCount;
}
