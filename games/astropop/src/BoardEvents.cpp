// Board: the handlers of the events that the bricks fire (the original keeps them in the vtable of its listener part at
// +0x58, one slot for each event; the port has one OnEvent that switches). docs/GAME_NOTES.md section 4b has the table.
// The handlers of the big events are in their own files (BoardArrive.cpp: a brick reaches its place; BoardGone.cpp: a
// brick's death is over).

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Font.h"
#include "SmallObjects.h"
#include "Res.h"
#include <set>

using namespace Sexy;

// The Board's listener part gets the events it listened to in its constructor. The source of each is a brick.
// (The cases 0x10 and 0x11 are the counter handlers 0x433cc0 and 0x433cd0.)
void Board::OnEvent(int theEvent, EventListener* theSource, int theArg)
{
	Brick* aBrick = static_cast<Brick*>(theSource);
	switch (theEvent)
	{
	case 5:		OnBrickArrived(aBrick); break;
	case 6:		OnBrickGone(aBrick); break;
	case 7:		OnBrickLanded(aBrick); break;
	case 8:		OnBrickFaded(aBrick); break;
	case 0xb:	OnBrickBounced(aBrick); break;
	case 0xc:	OnBrickFinished(aBrick); break;
	case 0xd:	OnColumnNuke(aBrick); break;
	case 0xe:	OnPowerBrickDone(aBrick); break;
	case 0xf:	OnMutatorSpread(aBrick); break;
	case 0x10:	mSupportedBrickCount++; break;                         // 0x433cc0 (INC [ECX+0x1a04]: ECX is the listener part, Board+0x64)
	case 0x11:	mSupportedBrickCount--; break;                         // 0x433cd0
	case 0x12:	OnHyperBrick(aBrick); break;
	case 0x13:	OnBoosterEnd(aBrick); break;
	case 0x1a:	OnBrickPopped(aBrick); break;
	case 0x1b:	OnBrickSettled(aBrick); break;
	case 0x1c:	OnFreezeBomb(aBrick); break;
	case 0x1f:	OnKillAllDone(aBrick, theArg); break;
	case 0x26:	OnBlast(aBrick, theArg); break;
	case 0x27:	OnColourBrickDie(aBrick, theArg); break;
	case 0x29:	OnPieceBrickHit(aBrick); break;
	}
}

// (inlined in the handlers of the original) the hint box of the cell under the pointer (the third of the Board's boxes)
// goes when the brick that it points at is the one that has been hit
// port: inlined in the original's event handlers
void Board::HideCellHint(Brick* theBrick)
{
	int aCol = CellColumn(theBrick->mX);
	int aRow = CellRow(theBrick->mY);
	HintBox* aBox = mHintBoxes[2];
	if (aBox->mShown && aBox->m60 == aCol && aBox->m64 == aRow)
		aBox->SetShown(false);
}

// 0x434320: one more set in a row. The chain count (mChainCount) goes up, the ping rises with it, and the best chain of the
// game and of the profile are kept
void Board::AdvanceChain()
{
	if (mBonusRoundOn)
		return;

	int anOld = mChainCount;
	mChainCount = anOld + 1;
	if (mChainCount > 1)
	{
		int aPitch = anOld - 1;
		if (aPitch > 9)
			aPitch = 10;
		mApp->PlayComboPing(aPitch);
	}
	int aChain = mChainCount;
	mComboPayDelay = 100;
	if (mLongestChain < aChain && !mBonusRoundOn)
		mLongestChain = aChain;
	if (mBestChain < aChain && !mBonusRoundOn && !mBonusWarpIn)
	{
		mBestChain = aChain;
		int aBest = 0;
		mApp->mUser->GetInt(0x5d, &aBest);
		if (aBest < mBestChain)
			mApp->mUser->SetInt(0x5d, mBestChain);
	}
	aChain = mChainCount;
	if (aChain > 3 && mHardStage <= mStageNumber && mGameType != 2)
		mBoosterDue = true;
	if (aChain > 2)
	{
		mPhase = 10;
		mPhaseDelay = PickSpawnDelay();
		aChain = mChainCount;
		mFlurryTimer = aChain * 100;
	}
	if (aChain == 1)
		mTimeBonusSets++;
	else if (aChain == 2)
	{
		mComboColor++;
		if (mComboColor > 2)
			mComboColor = 0;
	}
	if (aChain > 1 && (mComboChain <= aChain || mComboTimeLeft < 1))
	{
		mComboChain = aChain;
		mComboTimeLeft = 300;
		if (aChain > 5)
		{
			mComboTextVisible = true;
			mComboTimeLeft = 600;
		}
	}
	if (mTimeBonusSets == 1)
	{
		mTimeBonusRunning = true;
		mTimeBonusUpdates = 0;
	}
}

// 0x437830: a field that is nearly empty (no column of more than five bricks, no more than 25 in all, from the second
// row down) starts a bonus
void Board::CheckSparseField()
{
	if (!(mHighestStack < 6 && mExtraRowsDue < 1 && mFiredGroups.empty() && mFallingGroups.empty() && mFreezeUpdatesLeft < 1))
		return;

	int aTotal = 0;
	for (int aCol = 0; aCol < 10; aCol++)
	{
		int aCount = 0;
		for (unsigned int aRow = 1; !mFieldBricks.empty() && aRow < mFieldBricks.size(); aRow++)
		{
			if (mFieldBricks[aRow][aCol] != NULL)
			{
				aCount++;
				aTotal++;
				if (aCount > 5)
					return;
				if (aTotal > 0x19)
					return;
			}
		}
	}
	mExtraRowDelay = 100;
	mExtraRowsDue = (aTotal < 11 ? 1 : 0) + 1;
}

// 0x4588d0: the brick at (theCol, theRow) has been hit; the idle bricks round it go with it (left, right, below, above;
// with the upgrades of the weapon of the mode also the two above-diagonals and the second above). Whole steel only
// cracks. The bricks that went are returned; the plain ones among them hit their neighbours too (DamageNeighbours).
std::vector<CellRef> Board::BlastNeighbours(int theCol, int theRow, Brick* theBrick)
{
	std::vector<CellRef> aNeighbours;

	bool aMode0 = mMode == 0 && mPowerupActive == 1;
	bool aMode1 = mMode == 1 && mPowerupActive == 1;
	bool aMode2 = mMode == 2 && mPowerupActive == 1 && mWeaponPower >= 5;

	int aKind = theBrick->GetEffectiveType();
	bool aMarked = HasMarkOfType(aKind) && theBrick->mMutated != 1 && !theBrick->mSpecial;

	// the filter of the modes 1 and 2 that every neighbour has to pass
#define PASSES(c, r) \
	(!HasCell((c), (r), mConnectedCells) && \
		((aMode2 && !HasCell((c), (r), mTurbotMarkedCells)) || (aMode1 && !HasCell3((c), (r), mAtomicMarkedBricks)) || (!aMode2 && !aMode1)))

	CellRef aCell;
	Brick* aOther;
	if (theCol - 1 >= 0 && (aOther = mFieldBricks[theRow][theCol - 1]) != NULL && aOther->IsIdle() && PASSES(theCol - 1, theRow))
	{
		aCell.mCol = theCol - 1;
		aCell.mRow = theRow;
		aNeighbours.push_back(aCell);
	}
	if (theCol + 1 >= 0 && theCol + 1 < 10 && (aOther = mFieldBricks[theRow][theCol + 1]) != NULL && aOther->IsIdle() &&
		PASSES(theCol + 1, theRow))
	{
		aCell.mCol = theCol + 1;
		aCell.mRow = theRow;
		aNeighbours.push_back(aCell);
	}
	if (theRow != -1 && theRow + 1 >= 0 && theRow + 1 < 0x14 && (aOther = mFieldBricks[theRow + 1][theCol]) != NULL &&
		aOther->IsIdle() && PASSES(theCol, theRow + 1))
	{
		aCell.mCol = theCol;
		aCell.mRow = theRow + 1;
		aNeighbours.push_back(aCell);
	}
	if (((aMode0 && mWeaponPower > 1) || aMarked || aMode2) && theRow != 1 && theRow - 1 >= 0 &&
		(aOther = mFieldBricks[theRow - 1][theCol]) != NULL && aOther->IsIdle() && PASSES(theCol, theRow - 1))
	{
		aCell.mCol = theCol;
		aCell.mRow = theRow - 1;
		aNeighbours.push_back(aCell);
	}
	if (aMode0)
	{
		if (mWeaponPower > 2 && theCol - 1 >= 0 && theRow - 1 > 0 && (aOther = mFieldBricks[theRow - 1][theCol - 1]) != NULL &&
			aOther->IsIdle() && !HasCell(theCol - 1, theRow - 1, mConnectedCells))
		{
			aCell.mCol = theCol - 1;
			aCell.mRow = theRow - 1;
			aNeighbours.push_back(aCell);
		}
		if (mWeaponPower > 3 && theCol + 1 >= 0 && theCol + 1 < 10 && theRow - 1 > 0 &&
			(aOther = mFieldBricks[theRow - 1][theCol + 1]) != NULL && aOther->IsIdle() && !HasCell(theCol + 1, theRow - 1, mConnectedCells))
		{
			aCell.mCol = theCol + 1;
			aCell.mRow = theRow - 1;
			aNeighbours.push_back(aCell);
		}
		if (mWeaponPower > 4 && theRow - 2 > 0 && (aOther = mFieldBricks[theRow - 2][theCol]) != NULL && aOther->IsIdle() &&
			!HasCell(theCol, theRow - 2, mConnectedCells))
		{
			aCell.mCol = theCol;
			aCell.mRow = theRow - 2;
			aNeighbours.push_back(aCell);
		}
	}
#undef PASSES

	std::vector<CellRef> aGone;
	if (aNeighbours.empty())
		return aGone;

	std::vector<CellRef> aKept;
	if (mMode == 1 && mPowerupActive == 1)
	{
		for (unsigned int i = 0; i < mAtomicMarkedBricks.size(); i++)
		{
			CellRef aKeptCell;
			aKeptCell.mCol = mAtomicMarkedBricks[i].mCol;
			aKeptCell.mRow = mAtomicMarkedBricks[i].mRow;
			aKept.push_back(aKeptCell);
		}
	}

	std::vector<CellRef> aDamaged;
	for (unsigned int i = 0; i < aNeighbours.size(); i++)
	{
		Brick* aBrick = mFieldBricks[aNeighbours[i].mRow][aNeighbours[i].mCol];
		if (aBrick->mMutated == 1 || aBrick->mType != 9 || ((SpecialBrick*) aBrick)->mStage == 1)
		{
			if (aKept.empty() || !HasCell(aNeighbours[i].mCol, aNeighbours[i].mRow, aKept))
			{
				if (aBrick->mMutated != 1 && aBrick->mType >= 0 && aBrick->mType < 6 && !aBrick->mSpecial)
					aDamaged.push_back(aNeighbours[i]);
				aGone.push_back(aNeighbours[i]);
			}
		}
		else
			((SpecialBrick*) aBrick)->Crack(true);
	}

	for (unsigned int i = 0; i < aGone.size(); i++)
	{
		Brick* aBrick = mFieldBricks[aGone[i].mRow][aGone[i].mCol];
		if (aBrick != NULL)
		{
			aBrick->AdvanceParticleCount();
			RemoveBrick(NULL, aGone[i].mCol, aGone[i].mRow, false);
		}
	}
	DamageNeighbours(aDamaged);
	return aGone;
}

// 0x44bac0 event 0xc: the brick is out of the game; it leaves the lists of the Board and waits to be deleted
void Board::OnBrickFinished(Brick* theBrick)
{
	for (unsigned int i = 0; i < mFadedBricks.size(); i++)
	{
		if (mFadedBricks[i] == theBrick)
		{
			mFadedBricks.erase(mFadedBricks.begin() + i);
			break;
		}
	}
	for (unsigned int i = 0; i < mTargeted.size(); i++)
	{
		if (mTargeted[i] == theBrick)
		{
			mTargeted.erase(mTargeted.begin() + i);
			break;
		}
	}
	mDoomedBricks.insert(theBrick);
}

// 0x450f10 event 8
void Board::OnBrickFaded(Brick* theBrick)
{
	for (std::list<Brick*>::iterator i = mGoneBricks.begin(); i != mGoneBricks.end(); ++i)
	{
		if (*i == theBrick)
		{
			mGoneBricks.erase(i);
			break;
		}
	}
	mFadedBricks.push_back(theBrick);
}

// 0x438e10 event 0x1b (and the end of 0x45d7a0): when no brick of the field is still to mutate or mutating, the counter
// of the next mutation starts (2000)
static bool NoMutatingBrick(const std::vector<std::vector<Brick*> >& theField)
{
	for (unsigned int aRow = 0; !theField.empty() && aRow < theField.size(); aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = theField[aRow][aCol];
			if (aBrick != NULL && (aBrick->mMutated == 0 || aBrick->mMutated == 1))
				return false;
		}
	}
	return true;
}

void Board::OnBrickSettled(Brick* theBrick)
{
	(void) theBrick;
	if (NoMutatingBrick(mFieldBricks))
		mMutatorCooldown = 2000;
}

// 0x43b910 event 0x1c: the freeze bomb and the super-charger count as a set
void Board::OnFreezeBomb(Brick* theBrick)
{
	HideCellHint(theBrick);
	if (theBrick->mMutated != 1 && (theBrick->mType == 8 || theBrick->mType == 10))
		AdvanceChain();
}

// 0x45d7a0 event 0x1a: a brick has been popped. One that blasts its neighbours does it, from the cell it was removed from
void Board::OnBrickPopped(Brick* theBrick)
{
	HideCellHint(theBrick);
	if (theBrick->mBlastsOnPop)
	{
		unsigned int anIndex = 0;
		for (std::list<Brick*>::iterator i = mTakenBricks.begin(); i != mTakenBricks.end(); ++i, anIndex++)
		{
			if (*i == theBrick)
			{
				BlastNeighbours(mTakenCells[anIndex].mCol, mTakenCells[anIndex].mRow, theBrick);
				return;
			}
		}
	}
	if (NoMutatingBrick(mFieldBricks))
		mMutatorCooldown = 2000;
}

// 0x45d6f0 event 0x27: a plain colour brick dies; a brick that blasts, or one of a colour that has a mark, blasts its
// neighbours from the cell it was removed from
void Board::OnColourBrickDie(Brick* theBrick, int theType)
{
	if (theBrick->mMutated == 1)
		return;
	if (!theBrick->mBlastsOnPop && !HasMarkOfType(theType))
		return;
	unsigned int anIndex = 0;
	for (std::list<Brick*>::iterator i = mTakenBricks.begin(); i != mTakenBricks.end(); ++i, anIndex++)
	{
		if (*i == theBrick)
		{
			BlastNeighbours(mTakenCells[anIndex].mCol, mTakenCells[anIndex].mRow, theBrick);
			return;
		}
	}
}

// 0x451380 event 0x1f: a kill-all brick of a colour is done. The colour goes into the list of the colours to kill
void Board::OnKillAllDone(Brick* theBrick, int theType)
{
	mKillAllTypes.push_back(theType);
	OnBrickGone(theBrick);
	HideCellHint(theBrick);
	if (!static_cast<KillAllBrick*>(theBrick)->mChainCounted)
		AdvanceChain();
}

// 0x451430 event 0x29: a brick of the falling piece is hit. It leaves the piece (mHeldBricks), the cell it was in is remembered
// and the brick dies (the original does all this even for a brick that is not in the piece: then it is cell -1, -1 and
// no brick)
void Board::OnPieceBrickHit(Brick* theBrick)
{
	CellRef aCell;
	aCell.mCol = -1;
	aCell.mRow = -1;
	Brick* aFound = NULL;
	for (std::list<Brick*>::iterator i = mHeldBricks.begin(); i != mHeldBricks.end(); ++i)
	{
		if (*i == theBrick)
		{
			Brick* aBrick = *i;
			aCell.mCol = CellColumn(aBrick->mX);
			aCell.mRow = CellRow(aBrick->mY);
			aFound = aBrick;
			mHeldBricks.erase(i);
			break;
		}
	}
	mHeldBricksNotLanded--;
	mTakenCells.push_back(aCell);
	mTakenBricks.push_back(aFound);
	mTakenBrickCount++;
	FireTo(aFound, 3);
}

// 0x458770 event 0xb: a brick has finished its bounce. When the whole top row is at rest the pause that began with the
// bounces is over (mNewRowBouncing), the bricks of the column no longer announce their bounce, and the tutorial's first hint wakes
void Board::OnBrickBounced(Brick* theBrick)
{
	mRowTimer = 0;
	bool aAllStill = true;
	for (int aCol = 0; aCol < 10; aCol++)
	{
		Brick* aTop = mFieldBricks[0][aCol];
		// (the original does not look whether the cell has a brick)
		if (aTop != NULL && (aTop->mState != 0 || aTop->mBouncing || aTop->mBounceY != 0.0f))
		{
			aAllStill = false;
			break;
		}
	}
	if (aAllStill)
	{
		mNewRowBouncing = false;
		int aCol = CellColumn(theBrick->mX);
		for (unsigned int aRow = 0; !mFieldBricks.empty() && aRow < mFieldBricks.size() && mFieldBricks[aRow][aCol] != NULL; aRow++)
			mFieldBricks[aRow][aCol]->mBounceFire = false;
		if (!mHintSpots.empty())
			mHintSpots[0]->mActive = true;
	}
	if (mMode == 3 && mPowerupActive == 1 && mLastPointerX > 0xa5 && mLastPointerX < 0x25e)
		HoverCell((mLastPointerX - 0xa6) / 0x2c, CellRow((float) mLastPointerY), true);
}

// 0x45e920 event 0xe: a power brick is done. When nothing moves and no move is left the level is counted out
void Board::OnPowerBrickDone(Brick* theBrick)
{
	(void) theBrick;
	if (mBonusRoundOn && mKillAllTypes.empty() && !mBricksRevealing && !mStartBannerDoneInBonus && mBannerTimeLeft == 0 && mTakenBrickCount == 0)
	{
		bool aFlag = false;
		if (CountMovingBricks(false) == 0 && mFiredGroups.empty() && mFallingGroups.empty() && !AnyMoveLeft(&aFlag))
		{
			if (!aFlag)
			{
				mBannerTimeLeft = 400;
				BuildBanner(GT(0x56568c), mBannerTimeLeft, FONT_NEURO30);
				mApp->PlaySample(SOUND_NO_MORE_MOVES);
			}
			StartClearCount();
		}
	}
}

// 0x45d910 event 0xf: the brick at (col, row) has finished a mutator's countdown: one of its neighbours (up, down, left,
// right, starting from a random one) that is settled and idle, and not a booster, starts its own
void Board::OnMutatorSpread(Brick* theBrick)
{
	int aFoundCol = 500000000;
	int aFoundRow = 500000000;
	int aCount = 0;
	int aMinCol = 500000000;
	int aMaxCol = -500000000;

	for (int aCol = 0; aCol < 10; aCol++)
	{
		for (int aRow = 0; aRow < 0x14; aRow++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick == NULL)
				break;
			if (aBrick == theBrick)
			{
				aFoundRow = aRow;
				aFoundCol = aCol;
			}
			if (aBrick->mMutated == 0 || aBrick->mMutated == 1)
			{
				aCount++;
				if (aCol < aMinCol)
					aMinCol = aCol;
				if (aMaxCol < aCol)
					aMaxCol = aCol;
			}
			if (mMaxMutators <= aCount)
				return;
		}
	}

	if (aFoundRow == 0 || (aFoundCol == 500000000 && aFoundRow == 500000000))
		return;

	// the start direction; the original finds no direction when the random number is negative
	int aStart = ((int) AstroRand() % 500000000) % 4;
	if (aStart < 0)
		return;

	for (int n = 0; n < 4; n++)
	{
		int aDir = (aStart + n) % 4;
		int aCol = aFoundCol;
		int aRow = aFoundRow;
		Brick* aBrick = NULL;
		switch (aDir)
		{
		case 0:
			if (aFoundRow != 1 && aFoundRow - 1 >= 0)
				aBrick = mFieldBricks[aFoundRow - 1][aFoundCol];
			aRow = aFoundRow - 1;
			break;
		case 1:
			if (aFoundRow + 1 < 0x13)
				aBrick = mFieldBricks[aFoundRow + 1][aFoundCol];
			aRow = aFoundRow + 1;
			break;
		case 2:
			if (aFoundCol - 1 >= 0)
				aBrick = mFieldBricks[aFoundRow][aFoundCol - 1];
			aCol = aFoundCol - 1;
			break;
		case 3:
			if (aFoundCol + 1 < 10)
				aBrick = mFieldBricks[aFoundRow][aFoundCol + 1];
			aCol = aFoundCol + 1;
			break;
		}
		if (aBrick == NULL || !aBrick->IsIdle())
			continue;
		if (aBrick->mMutated == 0 || aBrick->mMutated == 1)
			continue;
		if (aBrick->mType == 0xe)
			continue;
		if (aDir == 2 && !(aMaxCol - aMinCol < 2 || aMinCol != aFoundCol))
			continue;
		if (aDir == 3 && !(aMaxCol - aMinCol < 2 || aMaxCol != aFoundCol))
			continue;

		if (aCol == 500000000 && aRow == 500000000)
			return;
		mFieldBricks[aRow][aCol]->StartMutatorCountdown();

		// the field may be stuck now
		if (!mBonusRoundOn || !mKillAllTypes.empty() || mBricksRevealing || mStartBannerDoneInBonus || mBannerTimeLeft != 0 || mTakenBrickCount != 0 || !mFiredGroups.empty() || !mFallingGroups.empty())
			return;
		bool aFlag = false;
		if (AnyMoveLeft(&aFlag))
			return;
		if (!aFlag)
		{
			mBannerTimeLeft = 400;
			BuildBanner(GT(0x56568c), mBannerTimeLeft, FONT_NEURO30);
			mApp->PlaySample(SOUND_NO_MORE_MOVES);
		}
		StartClearCount();
		return;
	}
}

// 0x44bba0 event 0x13: a booster has run out. Its brick, if it is in the field, is replaced by a new random brick in the
// same place (the booster's time is cut to one update when it is not to be replaced: the one-store setter 0x402d10)
void Board::OnBoosterEnd(Brick* theBrick)
{
	bool aKeep = theBrick->mState != 0 || theBrick->mBouncing || theBrick->mBounceY != 0.0f ||
		((mMode == 1 || mMode == 2) && mPowerupActive == 1);

	bool aFound = false;
	int aKind = theBrick->GetEffectiveType();
	for (unsigned int aRow = 0; !mFieldBricks.empty() && aRow < mFieldBricks.size() && !aFound; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL && aBrick->GetEffectiveType() == aKind && aBrick == theBrick)
			{
				aFound = true;
				break;
			}
		}
	}

	HideCellHint(theBrick);

	if (!aFound || aKeep)
	{
		if (mFallingGroups.size() != 0 || mFiredGroups.size() != 0 || aKeep)
			static_cast<PowerBrick*>(theBrick)->mBoosterTime = 1;
		return;
	}

	mDoomedBricks.insert(theBrick);
	theBrick->mGone = true;
	int aRow = CellRow(theBrick->mY);
	int aCol = CellColumn(theBrick->mX);
	Brick* aNew = CreateBrick(((int) AstroRand() % 500000000) % (mExtraColors + 4));
	mFieldBricks[aRow][aCol] = aNew;
	aNew->SetParticleX(theBrick->mX);
	aNew->SetParticleY(theBrick->mY);
	int aNewKind = aNew->GetEffectiveType();
	if (HasMarkOfType(aNewKind))
	{
		FireTo(aNew, 0x20, aNewKind);
		if (GetMarkCount(mFieldBricks[aRow][aCol]->GetEffectiveType()) < 0xff)
			FireTo(aNew, 0x25, GetMarkCount(mFieldBricks[aRow][aCol]->GetEffectiveType()));
	}
}

// 0x452e70 event 0x12: a booster (0xe) starts. It counts as a set, and unless the game is in the state that ends the
// level the meter of the bonus gets 15% of its size (or the stage's clock gets a 20th), a text says so, and the meter's
// own animation starts
void Board::OnHyperBrick(Brick* theBrick)
{
	AdvanceChain();
	HideCellHint(theBrick);

	if (!mBonusRoundOn)
	{
		int aMax = mBricksToPass;
		mStageBricksCleared += (int) ((float) aMax * 0.15f);
		if (aMax < mStageBricksCleared)
			mStageBricksCleared = aMax;
		mStageMeterAnim.Reset();
		mStageMeterAnim.SetStopped(false);
		mApp->PlaySample(SOUND_BRICK_MILESTONE);

		int aY = (int) (444.0f - mBrickometerFill * 444.0f);
		std::string aText = GT(0x5652e4);
		int aWidth = FONT_DIALOG_BOX_BTN->StringWidth(aText);
		FloatText* aFloat = new FloatText(aText, FONT_DIALOG_BOX_BTN, (float) (600 - aWidth), (float) aY, true);
		mFloatTexts.push_back(aFloat);
		aFloat = mFloatTexts.back();
		aFloat->mNoFade = true;
		aFloat->mAlphaDelay = 200;

		mBrickometerFill = (float) mStageBricksCleared / (float) mBricksToPass;
		if (!(mBrickometerFill <= 1.0f))
			mBrickometerFill = 1.0f;
		float aFill = mBrickometerFill * 444.0f;
		float aTop = 444.0f - aFill;
		mBrickometerRiseDistance = (int) ((float) aY - aTop);
		mBrickometerRiseSteps = 0x17;
		mBrickometerRiseStep = 0;
		mBrickometerRiseTop = (int) aTop;
		mBoosterFlameY = (int) (492.0f - aFill);
	}
	else
	{
		int anOld = mBonusTimeLeft;
		int aWas = (int) (444.0f - (float) anOld * 0.00016666666488163173f * 444.0f);
		mBonusTimeLeft = anOld + 300;
		if (mBonusTimeLeft >= 6000)
			mBonusTimeLeft = 6000;
		int aNow = (int) (444.0f - (float) mBonusTimeLeft * 0.00016666666488163173f * 444.0f);
		float aFill = mBrickometerFill * 444.0f;
		mBrickometerRiseDistance = aWas - aNow;
		mBrickometerRiseSteps = 0x17;
		mBrickometerRiseStep = 0;
		mBrickometerRiseTop = aNow;
		mBoosterFlameY = (int) (492.0f - aFill);
	}

	mBoosterAnim.SetDelay(1);
	mBoosterAnim.SetNumFrames(0x1f);
	mBoosterAnim.mPingPong = true;
	mBoosterAnim.Reset();
	mBoosterAnim.SetStopped(false);
	mBoosterAnim.mMaxLoops = 0x14;
	mBoosterAnim.mLoopCount = 0;
	static_cast<PowerBrick*>(theBrick)->mBoosterY = mBoosterFlameY;
}
