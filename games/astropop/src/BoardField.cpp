// Board: the field of bricks (19 rows of 10 in mFieldBricks) and the lists of bricks that hang off the Board: emptying them,
// filling the field from a level's layout, and the questions about the falling piece.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Res.h"

using namespace Sexy;

// 0x433030: do two bricks' types count as the same colour? A mutator counts as 0xc; stone (7), mutators (0xc) and
// steel (9) never match anything.
bool Board::TypesMatch(Brick* theBrick, int theType)
{
	int aType;
	if (theBrick->mMutated == 1)
		aType = 0xc;
	else
		aType = theBrick->mType;
	if (aType == theType && aType != 7 && aType != 0xc && aType != 9 && theType != 7 && theType != 0xc && theType != 9)
		return true;
	return false;
}

// 0x436b30: is the first brick of the falling piece of this type (a mutator counts as 0xc)?
bool Board::FirstPieceIsType(int theType)
{
	if (mHeldBricks.empty())
		return false;
	Brick* aBrick = mHeldBricks.front();
	if (aBrick->mMutated == 1)
		return theType == 0xc;
	return aBrick->mType == theType;
}

// 0x439510: does the brick flash already?
bool Board::IsTargeted(Brick* theBrick)
{
	for (unsigned int i = 0; i < mTargeted.size(); i++)
	{
		if (mTargeted[i] == theBrick)
			return true;
	}
	return false;
}

// 0x44e010: every brick the Board holds (the field, the falling piece, the groups, the lists at +0x88, +0xa0, +0x108
// and the one at +0x354) goes into the set of the bricks to delete, the lists are emptied, and the set is deleted
void Board::ClearField()
{
	for (unsigned int aRow = 0; aRow < mFieldBricks.size(); aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL)
				mDoomedBricks.insert(aBrick);
			mFieldBricks[aRow][aCol] = NULL;
		}
	}

	for (std::list<Brick*>::iterator i = mHeldBricks.begin(); i != mHeldBricks.end(); ++i)
		mDoomedBricks.insert(*i);
	mHeldBricks.clear();

	for (std::list<Brick*>::iterator i = mGoneBricks.begin(); i != mGoneBricks.end(); ++i)
		mDoomedBricks.insert(*i);
	mGoneBricks.clear();

	for (std::list<Brick*>::iterator i = mTakenBricks.begin(); i != mTakenBricks.end(); ++i)
		mDoomedBricks.insert(*i);
	mTakenBricks.clear();

	mTakenCells.clear();

	for (std::list<std::list<Brick*> >::iterator i = mFiredGroups.begin(); i != mFiredGroups.end(); ++i)
	{
		for (std::list<Brick*>::iterator j = i->begin(); j != i->end(); ++j)
			mDoomedBricks.insert(*j);
	}
	mFiredGroups.clear();

	for (std::list<std::list<Brick*> >::iterator i = mFallingGroups.begin(); i != mFallingGroups.end(); ++i)
	{
		for (std::list<Brick*>::iterator j = i->begin(); j != i->end(); ++j)
			mDoomedBricks.insert(*j);
	}
	mFallingGroups.clear();

	for (unsigned int i = 0; i < mFadedBricks.size(); i++)
		mDoomedBricks.insert(mFadedBricks[i]);
	mFadedBricks.clear();

	mTargeted.clear();

	if (mHandKindBrick != NULL)
		mDoomedBricks.insert(mHandKindBrick);
	mHandKindBrick = NULL;

	DeleteDoomedBricks();
}

// 0x44e350: the field is cleared and filled from the layout at +0x334 (a brick type for each cell, -1 for none); a
// steel brick is cracked at once when the saved game says so (the profile's score table holds the saved field's flags)
void Board::BuildField()
{
	unsigned int aRow = 0;
	mDemoWaitLeft = 0;
	mDemoStepIndex = 0;
	mDemoInputActive = false;
	mDemoAddedRowIndex = 0;
	mPlayStarted = true;
	ClearField();
	int aY = 4;
	while (aRow < mDemoLayout.size())
	{
		int aCol = 0;
		int aX = 0xa6;
		do
		{
			Brick* aBrick = NULL;
			switch (mDemoLayout[aRow][aCol])
			{
			case 0:
				aBrick = CreateBrick(0);
				break;
			case 1:
				aBrick = CreateBrick(1);
				break;
			case 2:
				aBrick = CreateBrick(2);
				break;
			case 3:
				aBrick = CreateBrick(3);
				break;
			case 4:
				aBrick = CreateBrick(4);
				break;
			case 5:
				aBrick = CreateBrick(5);
				break;
			case 6:
				aBrick = CreateSpecialBrick(6);
				break;
			case 7:
				aBrick = CreateSpecialBrick(7);
				break;
			case 8:
				aBrick = CreateFuelCell(500, 5);
				break;
			case 9:
			{
				SpecialBrick* aSteel = CreateSpecialBrick(9);
				aBrick = aSteel;
				if (mApp->mUser->mScores[aRow][aCol].mAux == 1)
					aSteel->Crack(false);
				break;
			}
			case 10:
				aBrick = CreatePowerBrick(10);
				break;
			case 0xb:
				aBrick = CreateSpecialBrick(0xb);
				break;
			case 0xc:
				aBrick = CreateMutator();
				if (aBrick != NULL)
					FireTo(aBrick, 0x28, 1);
				break;
			case 0xd:
				aBrick = CreatePowerBrick(0xd);
				break;
			case 0xe:
				aBrick = CreatePowerBrick(0xe);
				break;
			default:
				aBrick = NULL;
				break;
			}

			if (aBrick != NULL)
			{
				aBrick->SetParticleX((float)aX);
				int aBrickY = aY;
				if (4 < aY)
					aBrickY = aY + 5;
				aBrick->SetParticleY((float)aBrickY);
			}
			mFieldBricks[aRow][aCol] = aBrick;
			aX = aX + 0x2c;
			aCol++;
		} while (aX < 0x25e);
		aRow++;
		aY = aY + 0x19;
	}
}
