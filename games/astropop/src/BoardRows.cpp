// Board: the new row of bricks. Every so often the whole field moves down one row and a new row appears at the top
// (AddTopRow). The row comes from the demo's script or is made at random (FillTopRow); then the game checks whether
// the field has reached the player's ship.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "Res.h"

using namespace Sexy;

// 0x438a90: is a piece of this kind waiting to drop (the marks at +0x200 are pairs of a kind and a count)?
bool Board::HasMarkOfType(int theType)
{
	for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
	{
		if (mSuperchargeMarks[i].mType == theType && mSuperchargeMarks[i].mCount > 0)
			return true;
	}
	return false;
}

// 0x438af0: the count of the first mark of this kind, 0 for none
int Board::GetMarkCount(int theType)
{
	for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
	{
		if (mSuperchargeMarks[i].mType == theType)
			return mSuperchargeMarks[i].mCount;
	}
	return 0;
}

// 0x437780: of the groups of bricks that fall in a column (the list at +0x94), the brick that is lowest on the screen;
// NULL when none is there. Only the groups whose first brick is in the column count, and a group is looked through
// brick by brick.
Brick* Board::LowestFallingBrick94(int theColumn)
{
	Brick* aLowest = NULL;
	int aX = theColumn * 0x2c + 0xa6;
	for (std::list<std::list<Brick*> >::iterator aGroup = mFallingGroups.begin(); aGroup != mFallingGroups.end(); ++aGroup)
	{
		if (aGroup->empty() || aX != (int) aGroup->front()->mX)
			continue;
		for (std::list<Brick*>::iterator aBrick = aGroup->begin(); aBrick != aGroup->end(); ++aBrick)
		{
			if (aLowest == NULL)
				aLowest = *aBrick;
			else if (aLowest->mY < (*aBrick)->mY)
				aLowest = *aBrick;
		}
	}
	return aLowest;
}

// 0x4376d0: the same for the groups at +0x7c
Brick* Board::LowestFallingBrick7c(int theColumn)
{
	Brick* aLowest = NULL;
	int aX = theColumn * 0x2c + 0xa6;
	for (std::list<std::list<Brick*> >::iterator aGroup = mFiredGroups.begin(); aGroup != mFiredGroups.end(); ++aGroup)
	{
		if (aGroup->empty() || aX != (int) aGroup->front()->mX)
			continue;
		for (std::list<Brick*>::iterator aBrick = aGroup->begin(); aBrick != aGroup->end(); ++aBrick)
		{
			if (aLowest == NULL)
				aLowest = *aBrick;
			else if (aLowest->mY < (*aBrick)->mY)
				aLowest = *aBrick;
		}
	}
	return aLowest;
}

// 0x438c60: is the column at this x full up to the ship? The height the stack would reach (the bottom of the settled
// bricks, or the lowest of the falling ones plus 25) is compared with the limit that the length of the piece in hand
// sets.
bool Board::ColumnReachesShip(int theX)
{
	int aLimit = 0x1a7 - (int) mHeldBricks.size() * 0x19;
	int aTop = -500;
	int aFloor;
	if (FindBottomRow(theX, true) + 1 > 0)
		aFloor = FindBottomRow(theX, true) * 0x19 + 0x22;
	else
		aFloor = 4;

	int aColumn = (theX - 0xa6) / 0x2c;
	int aY94 = -500;
	Brick* aBrick = LowestFallingBrick94(aColumn);
	if (aBrick != NULL)
		aY94 = (int) (aBrick->mY + 25.0f);
	int aY7c = -500;
	aBrick = LowestFallingBrick7c(aColumn);
	if (aBrick != NULL)
		aY7c = (int) (aBrick->mY + 25.0f);

	aTop = aY7c;
	if (aY94 > aY7c)
		aTop = aY94;
	if (aFloor >= aTop)
		aTop = aFloor;
	if (aLimit > aTop)
	{
		if (FindBottomRow(theX, true) < 0x13)
			return false;
	}
	return true;
}

// 0x46e420: a new row of bricks at the top. It is added when something asks for it: while a game is running (not at its
// end), and only if something is in the field or in the air. Every row moves one down and starts to bounce; the new row
// is the demo's next row, or a random one. Afterwards the field is checked against the ship: if it is there the game
// ends, else the targets of the piece in hand are marked again.
void Board::AddTopRow()
{
	if (mGameOver || mLevelUpPhase)
		return;
	if (!(m19c0 != 0 || mWarpActive || mWarpFlight || mAtWarpGate || mWarpGatePicked || mWarpCharging || m1924 != 0 || !mFallingGroups.empty() || !mFiredGroups.empty() ||
		!mTakenBricks.empty() || !mKillAllTypes.empty()))
		return;

	bool aNoRoom = NoRoomLeft();
	if (m1a34 == 1 && IsSettled(true) && aNoRoom)
		return;
	if (mDemoPlaying && (unsigned int) m958 >= mDemoAddedRows.size())
		return;

	for (int aRow = 18; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			if (mFieldBricks[aRow][aCol] != NULL)
			{
				mFieldBricks[aRow + 1][aCol] = mFieldBricks[aRow][aCol];
				mFieldBricks[aRow][aCol] = NULL;
				mFieldBricks[aRow + 1][aCol]->StartBounce();
			}
		}
	}

	if (!mDemoPlaying)
		FillTopRow(true, false, true, true, 0);
	else
	{
		int anX = 0xa6;
		for (int aCol = 0; aCol < 10; aCol++, anX += 0x2c)
		{
			Brick* aBrick = NULL;
			int aType = mDemoAddedRows[m958][aCol];
			switch (aType)
			{
			case 0: case 1: case 2: case 3: case 4: case 5:
				aBrick = CreateBrick(aType);
				break;
			case 6: case 7: case 0xb:
				aBrick = CreateSpecialBrick(aType);
				break;
			case 8:
				aBrick = CreateFuelCell(500, 5);
				break;
			case 9:
				// (the original then reads a score of the profile past the end of its row and cracks the brick when
				// that is 1; see the notes, "Oddities")
				aBrick = CreateSpecialBrick(9);
				break;
			case 10: case 0xd: case 0xe:
				aBrick = CreatePowerBrick(aType);
				break;
			case 0xc:
				aBrick = CreateMutator();
				FireTo(aBrick, 0x28, 1);
				break;
			}
			mFieldBricks[0][aCol] = aBrick;
			if (aBrick != NULL)
			{
				int aMutated = aBrick->mMutated;
				int aKey = (aMutated != 1) ? aBrick->mType : 0xc;
				if (HasMarkOfType(aKey) && (!aBrick->mSpecial || aMutated == 1))
				{
					FireTo(aBrick, 0x20, aKey);
					int aCount = GetMarkCount(aKey);
					if (aCount < 0xff)
						FireTo(aBrick, 0x25, aCount);
				}
				aBrick->SetParticleX((float) anX);
				aBrick->SetParticleY(-21.0f);
				aBrick->StartBounce();
			}
		}
		m958++;
	}

	mNewRowBouncing = true;
	if (!ColumnReachesShip(m198c))
		HighlightTargets();
	else
		GameOver();
	gFlag5A5100 = false;

	if (mRecordingDemo)
	{
		std::vector<int> aTypes;
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = mFieldBricks[0][aCol];
			if (aBrick == NULL)
				aTypes.push_back(-1);
			else if (aBrick->mMutated == 1)
				aTypes.push_back(0xc);
			else
				aTypes.push_back(aBrick->mType);
		}
		mDemoAddedRows.push_back(aTypes);
	}
}
