// Board: making the row of bricks that appears at the top (FillTopRow), the rules that say which kinds may stand next to
// each other, and the checks of the height of the stacks that follow every new row.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Particle.h"
#include "Res.h"
#include "Image.h"
#include <math.h>
#include <stdlib.h>

using namespace Sexy;

// the order of the rules in SpawnRules
enum
{
	RULE_STONE = 0,         // kind 7
	RULE_STEEL = 1,         // kind 9
	RULE_KILLALL = 2,       // the plain colours (generic)
	RULE_BOMB = 3,          // kind 6
	RULE_FREEZE = 4,        // kind 10
	RULE_ROWNUKE = 5,       // kind 0xb
	RULE_MUTATOR = 6,       // kind 0xc
	RULE_COLNUKE = 7        // kind 0xd
};

// 0x439560: the number of bricks stacked in a column (the row of the first gap)
int Board::ColumnDepth(int theCol)
{
	int aRow = 0;
	while (aRow < 20 && mFieldBricks[aRow][theCol] != NULL)
		aRow++;
	return aRow;
}

// 0x439350: a killall brick of one of the four colours
Brick* Board::CreateKillAll(int theColor)
{
	Brick* aBrick = new KillAllBrick(this, mBus, 0x19);
	aBrick->SetType(theColor);
	mSpecialCooldowns[0] = 2000;
	return aBrick;
}

// 0x439040: may a brick of this kind stand in the column theCol of the row theRefRow, as far as the bricks in the row
// theRow go? Bricks too near of the same kind (or of the kinds that exclude each other) say no. With theAnyDistance every
// distance counts as too near (the rows above and below are then looked at by the caller).
bool Board::RowAllows(int theKind, int theRow, int theRefRow, int theCol, bool theAnyDistance)
{
	SpawnRules& aRules = mSpawnRules[0];
	int aGapPlain = aRules.mRule[RULE_KILLALL].mNearSame;
	int aGapBomb = aRules.mRule[RULE_BOMB].mNearSame;
	int aGapStone = aRules.mRule[RULE_STONE].mNearSame;
	int aGapSteel = aRules.mRule[RULE_STEEL].mNearSame;
	int aGapRowNuke = aRules.mRule[RULE_ROWNUKE].mNearSame;
	int aGapMutator = aRules.mRule[RULE_MUTATOR].mNearSame;
	int aGapColNuke = aRules.mRule[RULE_COLNUKE].mNearSame;
	int aMutators = 0;
	if (theAnyDistance)
	{
		aGapRowNuke = 0x10000;
		aGapSteel = 0x10000;
		aGapMutator = 0x10000;
		aGapStone = 0x10000;
		aGapBomb = 0x10000;
		aGapPlain = 0x10000;
	}

	bool aColoured = theKind <= m1a28 + 3;
	int aDistance = theCol;
	for (int aCol = 0; aCol < 10; aCol++, aDistance--)
	{
		Brick* aBrick = mFieldBricks[theRow][aCol];
		if (aBrick == NULL)
			continue;
		if (theRefRow == theRow && aCol == theCol)
			continue;
		if (!aBrick->IsIdle())
			continue;

		if (theKind == 0xc && (aBrick->mMutated == 1 || aBrick->mType == 0xc))
			aMutators++;

		int anAbs = abs(aDistance);
		if (aColoured && aBrick->mSpecial && aBrick->mMutated != 1 && anAbs <= aGapPlain)
			return false;

		if (theKind == 6)
		{
			if (aBrick->mMutated != 1 && aBrick->mType == 6 && anAbs <= aGapBomb)
				return false;
		}
		else if (theKind == 7)
		{
			if (aBrick->mMutated != 1 && (aBrick->mType == 7 || aBrick->mType == 9) && anAbs <= aGapStone)
				return false;
		}
		else if (theKind == 10)
		{
			if (aBrick->mMutated != 1 && aBrick->mType == 10)
				return false;
		}
		else if (theKind == 9)
		{
			if (aBrick->mMutated != 1 && (aBrick->mType == 7 || aBrick->mType == 9) && anAbs <= aGapSteel)
				return false;
		}
		else if (theKind == 0xb)
		{
			if (aBrick->mMutated != 1 && aBrick->mType == 0xb && anAbs <= aGapRowNuke)
				return false;
		}
		else if (theKind == 0xc)
		{
			if ((aBrick->mMutated == 1 || aBrick->mType == 0xc) && anAbs <= aGapMutator)
				return false;
		}
		else if (theKind == 0xd)
		{
			if (aBrick->mMutated != 1 && aBrick->mType == 0xd && anAbs <= aGapColNuke)
				return false;
		}
	}

	if (theKind == 0xc && aMutators >= m1ac4)
		return false;
	return true;
}

// 0x43d4c0: may a brick of this kind be made at this place of the new row? A kind is refused when a brick of the same
// special kind is anywhere in the field already (the coloured kinds), or when the rules about distance in the row, and in
// the rows above and below, say it stands too near another of its kind.
bool Board::CanPlaceKind(int theKind, int theRow, int theCol)
{
	if (theKind == 0xc && m1b5c > 0)
		return false;

	if (theKind <= m1a28 + 3)
	{
		for (unsigned int aRow = 0; aRow < mFieldBricks.size(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick != NULL && aBrick->mSpecial && aBrick->mMutated != 1 && aBrick->mType == theKind)
					return false;
			}
		}
	}

	if (!RowAllows(theKind, theRow, theRow, theCol, false))
		return false;

	SpawnRules& aRules = mSpawnRules[0];
	int aGap = aRules.mRule[RULE_KILLALL].mNearRow;
	switch (theKind)
	{
	case 7:		aGap = aRules.mRule[RULE_STONE].mNearRow;		break;
	case 6:		aGap = aRules.mRule[RULE_BOMB].mNearRow;		break;
	case 10:	aGap = aRules.mRule[RULE_FREEZE].mNearRow;		break;
	case 9:		aGap = aRules.mRule[RULE_STEEL].mNearRow;		break;
	case 0xb:	aGap = aRules.mRule[RULE_ROWNUKE].mNearRow;		break;
	case 0xc:	aGap = aRules.mRule[RULE_MUTATOR].mNearRow;		break;
	case 0xd:	aGap = aRules.mRule[RULE_COLNUKE].mNearRow;		break;
	}

	int aRowDistance = theRow;
	for (int aRow = 0; aRow < 0x13; aRow++, aRowDistance--)
	{
		if (aRow != theRow)
		{
			if (!RowAllows(theKind, aRow, theRow, theCol, true) && abs(aRowDistance) <= aGap)
				return false;
		}
	}
	return true;
}

// 0x450a30: gathers (into mConnectedCells) the cells that are joined to this one and hold a brick of the kind, going right, left, up
// and down in turn but never back the way it came. It stops when theLimit cells are gathered (0xffffffff: no limit),
// leaves out the cells in theSkip, and takes the top row as a target only when theIncludeTop is set.
void Board::CollectConnected(int theKind, int theCol, int theRow, unsigned int theLimit, bool theIncludeTop, const std::vector<CellRef>* theSkip, int thePrevCol, int thePrevRow)
{
	if (theLimit != 0xffffffff && theLimit <= (unsigned int) mConnectedCells.size())
		return;
	if (theSkip != NULL && HasCell(theCol, theRow, *theSkip))
		return;

	Brick* aBrick = mFieldBricks[theRow][theCol];
	if (aBrick == NULL || !TypesMatch(aBrick, theKind) || !aBrick->IsIdle())
		return;

	if (thePrevRow == -1 && thePrevCol == -1)
	{
		thePrevCol = theCol;
		thePrevRow = theRow;
	}

	for (unsigned int i = 0; i < mConnectedCells.size(); i++)
	{
		if (mConnectedCells[i].mCol == theCol && mConnectedCells[i].mRow == theRow)
			return;
	}
	CellRef aCell;
	aCell.mCol = theCol;
	aCell.mRow = theRow;
	mConnectedCells.push_back(aCell);

	if (theCol + 1 < 10 && thePrevCol <= theCol)
		CollectConnected(theKind, theCol + 1, theRow, theLimit, theIncludeTop, theSkip, theCol, theRow);
	if (theCol - 1 >= 0 && theCol <= thePrevCol)
		CollectConnected(theKind, theCol - 1, theRow, theLimit, theIncludeTop, theSkip, theCol, theRow);
	if ((theIncludeTop ? -1 : 0) < theRow - 1 && theRow <= thePrevRow)
		CollectConnected(theKind, theCol, theRow - 1, theLimit, theIncludeTop, theSkip, theCol, theRow);
	if (theRow + 1 < 0x13 && thePrevRow <= theRow)
		CollectConnected(theKind, theCol, theRow + 1, theLimit, theIncludeTop, theSkip, theCol, theRow);
}

// 0x43d630: after a new row: the height of the stacks is looked at. The highest stack and the sum of all are kept for the
// danger display (m930, m934); a stack of 14 or more starts the siren; bricks of columns with room again stop shaking.
// Returns the height of the highest stack.
int Board::CheckStackHeight()
{
	float aMax = 0.0f;
	int aSum = 0;
	for (int aCol = 0; aCol < 10; aCol++)
	{
		int aDepth = ColumnDepth(aCol);
		aSum = (int) ((double) aSum + (double) aDepth);
		if ((double) aDepth > (double) aMax)
			aMax = (float) aDepth;
	}
	UpdateKlaxon();

	int aHighest = (int) aMax;
	if (aHighest >= 0xe && (double) m970 != (double) aMax && !m910 && (!m8eb || m8ec) && !m8ed && !m8ef && !m8f4 &&
		!m8ec)
	{
		int aValue = 2 - ((int) (18.0 - (double) aMax - 4.0)) * 3;
		if (m1aa4 < 0)
			aValue = -aValue;
		m1aa4 = aValue;
		if (mSirenAnim.mStopped)
		{
			mApp->PlaySample(SOUND_BRICK_WARNING1);
			mSirenAnim.Reset();
			mSirenAnim.SetStopped(false);
			m1ab8 = 8;
			m1abc = 0;
			m1ac0 = PickSpawnDelay();
			mApp->PlaySong(0x23);
			m1aa8 = 0;
			m1a98 = mUpdateCnt + 0x14;
		}
		if ((double) aMax == 14.0)
			m1a7c = -1;
	}
	m970 = aHighest;

	float aTenth = (float) aSum * 0.1f;
	float aDanger = aTenth + fabsf(aMax - aTenth) * 0.5f;
	m930 = aDanger;
	if (aDanger > m934)
		m934 = aDanger;

	for (int aCol = 0; aCol < 10; aCol++)
	{
		Brick* aTop = mFieldBricks[0][aCol];
		if (aTop != NULL && aTop->mShake && ColumnDepth(aCol) + 2 < 0x12)
		{
			for (int aRow = 0; aRow < 0x14; aRow++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick == NULL)
					break;
				aBrick->StopShake();
			}
		}
	}

	if (aMax < 13.0f && m1a7c == -1)
		m1a7c = 300 / abs((int) (15.0 - (double) aMax));
	return aHighest;
}

// 0x43d400: when the stack in the column of the ship is too high for the piece in hand (modes 0 and 3, or when no mode 1
// or 2 play is on), the column shakes and the warning of m909 starts (it counts down m97c); with room again both end.
void Board::CheckShipColumn()
{
	if (mMode == 0 || mMode == 3 || m1a34 == 0)
	{
		int aCol = (m198c - 0xa6) / 0x2c;
		int aDepth = ColumnDepth(aCol);
		if (aDepth * 0x19 + 0x1d < (int) mHeldBricks.size() * -0x19 + 0x1a7)
		{
			m909 = false;
			m97c = 0;
		}
		else
		{
			for (int aRow = 0; aRow < 0x14; aRow++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick != NULL)
					aBrick->StartShake();
			}
			if (m97c == 0)
			{
				m909 = true;
				m97c = 400;
				return;
			}
		}
	}
}

// 0x44fd60: a tutorial hint about a kind of brick (the kinds 0-5 are the colours, 6-0xd the special ones, 0xe the booster,
// -1 the fuel). It is made only in a mode 1 game and only when the hint has not been seen: the profile settings say so
// (setting 0x13 turns the hints on, setting <id> is the hint's own switch, <id> + 1 whether it was shown). A hint that
// is not to be shown is counted as seen; a shown one is kept in mHintSpots with a ring around the brick and the four
// rectangles that darken the rest of the screen.
void Board::ShowHint(int theKind, int theCol, int theRow, bool theFlag)
{
	if (m910 || m917 || m1a60 != 1)
		return;

	HintSpot* aHint = new HintSpot();
	switch (theKind)
	{
	case 0: case 1: case 2: case 3: case 4: case 5:
		aHint->mBg = mHintPanels[8];
		aHint->mBg->mOverPic = GetImageById(theKind + (mApp->mColorblind ? 0x227 : 0x221));
		aHint->mId = 0x22;
		break;
	case 6:		aHint->mBg = mHintPanels[2];		aHint->mId = 0x24;	break;
	case 7:		aHint->mBg = mHintPanels[3];		aHint->mId = 0x14;	break;
	case 8:		aHint->mBg = mHintPanels[1];		aHint->mId = 0x18;	break;
	case 9:		aHint->mBg = mHintPanels[5];		aHint->mId = 0x16;	break;
	case 10:	aHint->mBg = mHintPanels[6];		aHint->mId = 0x1a;	break;
	case 0xb:	aHint->mBg = mHintPanels[4];		aHint->mId = 0x1c;	break;
	case 0xc:	aHint->mBg = mHintPanels[7];		aHint->mId = 0x1e;	break;
	case 0xd:	aHint->mBg = mHintPanels[9];		aHint->mId = 0x20;	break;
	case 0xe:	aHint->mBg = mHintPanels[10];		aHint->mId = 0x26;	break;
	case -1:	aHint->mBg = mHintPanels[11];		aHint->mId = 0x28;	break;
	}

	for (unsigned int i = 0; i < mHintSpots.size(); i++)
	{
		if (mHintSpots[i]->mId == aHint->mId)
		{
			delete aHint;
			return;
		}
	}

	int aHintsOn = 1;
	mApp->mUser->GetInt(0x13, &aHintsOn);
	int aOwnSwitch = 1;
	mApp->mUser->GetInt(aHint->mId, &aOwnSwitch);
	if (aHintsOn == 0 || aOwnSwitch == 0)
	{
		mApp->mUser->SetInt(aHint->mId + 1, 1);
		mApp->mUser->Save();
	}
	else
	{
		int aShown = 0;
		mApp->mUser->GetInt(aHint->mId + 1, &aShown);
		if (aShown != 1)
		{
			aOwnSwitch = 1;
			mApp->mUser->GetInt(aHint->mId, &aOwnSwitch);
			if (aOwnSwitch != 0)
			{
				aHint->mArrowX = (theCol * 0x2c - IMAGE_TH_ARROWHEAD->mWidth / 2) + 0xbc;
				int aY = (theRow < 1) ? 4 : theRow * 0x19 + 9;
				aHint->mArrowY = aY + 0x14;
				aHint->mRingX = (theCol * 0x2c - IMAGE_TH_RING->mWidth / 2) + 0xbc;
				aY = (theRow < 1) ? 4 : theRow * 0x19 + 9;
				aHint->mRingY = (aY - IMAGE_TH_RING->mHeight / 2) + 0xc;

				int aRingWidth = IMAGE_TH_RING->mWidth;
				int aRingHeight = IMAGE_TH_RING->mHeight;
				aHint->mHole.push_back(Rect(0, 0, aHint->mRingX, 0x1e0));
				int aRightX = aRingWidth + aHint->mRingX;
				aHint->mHole.push_back(Rect(aRightX, 0, 0x280 - aRightX, 0x1e0));
				aHint->mHole.push_back(Rect(aHint->mRingX, 0, aRingWidth, aHint->mRingY));
				int aBelowY = aRingHeight + aHint->mRingY;
				aHint->mHole.push_back(Rect(aHint->mRingX, aBelowY, aRingWidth, 0x1e0 - aBelowY));
				aHint->mActive = theFlag;
				aHint->mArrowTop = (0xb4 - IMAGE_TH_ARROWHEAD->mHeight) - aHint->mArrowY;
				mHintSpots.push_back(aHint);
				return;
			}
		}
	}
	delete aHint;
}

// 0x451720: makes a row of ten bricks, in row theRow of the field. theFromTop: the row is the new top row, its bricks start
// above the screen and bounce in (else they stand in place at once, as when the field is built). A row comes from the
// script (m984, row m1930) while there is one, else it is drawn: the share of each kind comes from the rates of the
// phase of the game (the third of the level reached) and from what is allowed (CanPlaceKind). theSpecials allows the
// special kinds, thePowers lets the rows after a special brick draw at once from the rates of the special kinds, and
// theTutorial plays the fixed order of the tutorial (the stage mField193C decides what the sixth brick is). Afterwards the
// stacks are checked: a stack that reaches the ship ends the game.
void Board::FillTopRow(bool theSpecials, bool theTutorial, bool theFromTop, bool thePowers, int theRow)
{
	int aCol = 0;
	bool aTutorial = true;
	if (!theTutorial || mField193C > 0x15 || m1a60 != 1)
		aTutorial = false;

	m8fb = false;
	int aPhase = 2;
	if (m1a60 == 2)
		aPhase = 0;
	else
	{
		float aRatio = (float) m1a14 / (float) m1a18;
		if (!(aRatio <= 0.3333333333333333))
		{
			if (!(aRatio <= 0.6666666666666666))
			{
				aPhase = 2;
				if (mField193C >= m1bbc)
				{
					if (m1b10 < 2)
						m1b10 = 2;
					else if (m1b10 == 2)
						m1b10 = 3;
				}
			}
			else
			{
				aPhase = 1;
				if (mField193C >= m1bbc)
					m1b10 = (m1b10 == 0) * 2 - 1;
			}
		}
		else
			aPhase = 0;
	}

	if (theFromTop)
		theRow = 0;

	if (m1930 != -1)
	{
		// the script's row (a brick of code 0 stays empty; the original then moves the empty place and crashes)
		int anX = 0xa6;
		for (aCol = 0; anX < 0x25e; aCol++, anX += 0x2c)
		{
			int aCode = m984.mCode[m1930][aCol];
			std::vector<Brick*>& aRow = mFieldBricks[theRow];
			switch (aCode)
			{
			case 0:
				aRow[aCol] = NULL;
				break;
			case 1: case 2: case 3: case 4:
			{
				int aKind = aCode - 1;
				aRow[aCol] = CreateBrick(aKind);
				if (HasMarkOfType(aKind))
				{
					FireTo(aRow[aCol], 0x20, aKind);
					if (GetMarkCount(aKind) < 0xff)
						FireTo(aRow[aCol], 0x25, GetMarkCount(aKind));
				}
				break;
			}
			case 5:
				aRow[aCol] = CreateSpecialBrick(7);
				break;
			case 6: case 7: case 8: case 9:
				aRow[aCol] = CreateKillAll(aCode - 6);
				break;
			case 10:
				aRow[aCol] = CreateSpecialBrick(6);
				break;
			}

			Brick* aBrick = aRow[aCol];
			if (aBrick != NULL)
			{
				aBrick->SetParticleX((float) anX);
				if (!theFromTop)
				{
					int aY = theRow * 0x19 + 4;
					if (theRow > 0)
						aY = theRow * 0x19 + 9;
					aBrick->SetParticleY((float) aY);
				}
				else
				{
					aBrick->SetParticleY(-21.0f);
					aBrick->StartBounce();
				}
			}
		}

		m1930++;
		bool aMore = false;
		if (m1930 < 100)
		{
			for (int i = 0; i < 10; i++)
			{
				if (m984.mCode[m1930][i] != 0)
				{
					aMore = true;
					break;
				}
			}
		}
		if (!aMore)
			m1930 = -1;
	}
	else
	{
		int aFuelCol = -1;
		int aPowerCol = -1;
		bool aMadeLast = false;
		if (m8ee)
			aFuelCol = ((int) AstroRand() % 500000000) % 10;
		else
		{
			if (m1b10 == aPhase && m1b10 > 0)
				aPowerCol = ((int) AstroRand() % 500000000) % 10;
			else if (m90e)
			{
				unsigned int aRand = AstroRand();
				m90e = false;
				aPowerCol = ((int) aRand % 500000000) % 10;
			}
		}

		for (aCol = 0; aCol < 10; aCol++)
		{
			bool aShowHint = true;
			int aCode = 0;
			int aKind = -1;
			SpawnRules& aRules = mSpawnRules[0];

			if (!thePowers || aMadeLast)
			{
				if (theSpecials)
				{
					double aDraw1 = (double) ((int) AstroRand() % 500000000);
					if (!(m1bec * 5e8f < aDraw1))
					{
						double aDraw2 = (double) ((int) AstroRand() % 500000000);
						float aStone = aRules.mRule[RULE_STONE].mRates[aPhase].mRate * 5e8f;
						if (aDraw2 <= aStone)
							aCode = 2;
						else
						{
							float aSteel = aRules.mRule[RULE_STEEL].mRates[aPhase].mRate * 5e8f;
							if (!(aDraw2 > aSteel + aStone))
								aCode = 3;
						}
					}
				}
			}
			else
			{
				double aDraw1 = (double) ((int) AstroRand() % 500000000);
				float aPlainShare = m1be8 * 5e8f;
				if (aDraw1 > aPlainShare)
				{
					if (theSpecials && !(aDraw1 > (m1bec + m1be8) * 5e8f))
					{
						double aDraw2 = (double) ((int) AstroRand() % 500000000);
						float aStone = aRules.mRule[RULE_STONE].mRates[aPhase].mRate * 5e8f;
						float aSteel = aRules.mRule[RULE_STEEL].mRates[aPhase].mRate * 5e8f;
						float aMutator = aRules.mRule[RULE_MUTATOR].mRates[aPhase].mRate * 5e8f;
						if (aDraw2 <= aStone)
							aCode = 2;
						else if (aDraw2 <= aSteel + aStone)
							aCode = 3;
						else if (aDraw2 <= (aMutator + aSteel) + aStone)
							aCode = 7;
					}
				}
				else
				{
					float aDraw = (float) (double) ((int) AstroRand() % 500000000);
					float aKillAll = aRules.mRule[RULE_KILLALL].mRates[aPhase].mRate * 5e8f;
					float aBomb = aRules.mRule[RULE_BOMB].mRates[aPhase].mRate * 5e8f;
					float aFreeze = aRules.mRule[RULE_FREEZE].mRates[aPhase].mRate * 5e8f;
					float aRowNuke = aRules.mRule[RULE_ROWNUKE].mRates[aPhase].mRate * 5e8f;
					float aColNuke = aRules.mRule[RULE_COLNUKE].mRates[aPhase].mRate * 5e8f;
					float aSum1 = aBomb + aKillAll;
					float aSum2 = (aFreeze + aBomb) + aKillAll;
					float aSum3 = ((aRowNuke + aFreeze) + aBomb) + aKillAll;
					float aSum4 = (((aColNuke + aRowNuke) + aFreeze) + aBomb) + aKillAll;
					if (aDraw <= aKillAll && mSpecialCooldowns[0] == 0)
						aCode = 1;
					else if (aDraw > aKillAll && !(aSum1 < aDraw) && aBomb > 0.0f && mSpecialCooldowns[1] == 0)
						aCode = 5;
					else if (aSum1 < aDraw && !(aSum2 < aDraw) && aFreeze > 0.0f && mSpecialCooldowns[3] == 0)
						aCode = 4;
					else if (aSum2 < aDraw && !(aSum3 < aDraw) && aRowNuke > 0.0f && mSpecialCooldowns[2] == 0)
						aCode = 6;
					else if (aSum3 < aDraw && !(aSum4 < aDraw) && aColNuke > 0.0f && mSpecialCooldowns[4] == 0)
						aCode = 8;
				}
			}

			aMadeLast = false;
			switch (aCode)
			{
			case 1:		aKind = 0;		break;
			case 5:		aKind = 6;		break;
			case 2:		aKind = 7;		break;
			case 3:		aKind = 9;		break;
			case 4:		aKind = 10;		break;
			case 6:		aKind = 0xb;	break;
			case 7:		aKind = 0xc;	break;
			case 8:		aKind = 0xd;	break;
			default:	aKind = -1;		break;
			}
			if (aKind != -1)
			{
				if (aCode == 1)
					aKind = ((int) AstroRand() % 500000000) % (m1a28 + 4);
				if (!CanPlaceKind(aKind, theRow, aCol))
					aCode = 0;
			}

			bool aFuel = aFuelCol != -1 && aCol == aFuelCol;
			bool aPower = false;
			bool aPlaceFuel = false;
			if (aTutorial && aCol == 5)
			{
				aTutorial = false;
				if (mField193C == 2 || mField193C == 3)
					aPlaceFuel = true;
				else if (mField193C == 4 || mField193C == 5)
					aPower = true;
				else if (mField193C == 6 || mField193C == 7)
					aCode = 5;
				else if (mField193C == 8 || mField193C == 9)
					aCode = 2;
				else if (mField193C == 10 || mField193C == 11)
					aCode = 6;
				else if (mField193C == 12 || mField193C == 13)
					aCode = 3;
				else if (mField193C == 14 || mField193C == 15)
					aCode = 4;
				else if (mField193C == 16 || mField193C == 17)
					aCode = 7;
				else if (mField193C == 18 || mField193C == 19)
				{
					aCode = 1;
					aKind = ((int) AstroRand() % 500000000) % (m1a28 + 4);
				}
				else if (mField193C == 20 || mField193C == 21)
					aCode = 8;
			}

			int aHintRow = theFromTop ? 0 : theRow;
			int aHintKind = 0;
			if (aPlaceFuel || aFuel)
			{
				mFieldBricks[theRow][aCol] = CreateFuelCell(500, 5);
				aMadeLast = true;
				m8ee = false;
				aCode = -1;
				ShowHint(8, aCol, aHintRow, !theFromTop);
			}
			else
			{
				if (aPowerCol == aCol || aPower)
				{
					mFieldBricks[theRow][aCol] = CreatePowerBrick(0xe);
					aMadeLast = true;
					aCode = -1;
					ShowHint(0xe, aCol, aHintRow, !theFromTop);
					aShowHint = false;
				}
				if (aPowerCol != aCol && !aPower)
				{
					bool aMade = true;
					if (aCode == 1)
					{
						mFieldBricks[theRow][aCol] = CreateKillAll(aKind);
						aMadeLast = true;
						aHintKind = aKind;
						if (aKind == -1)
							aMade = false;
					}
					else if (aCode == 5)
					{
						mFieldBricks[theRow][aCol] = CreateSpecialBrick(6);
						aHintKind = 6;
						aMadeLast = true;
					}
					else if (aCode == 2)
					{
						mFieldBricks[theRow][aCol] = CreateSpecialBrick(7);
						aHintKind = 7;
						aMadeLast = false;
						if (aTutorial && (mField193C == 6 || mField193C == 7))
							aTutorial = false;
					}
					else if (aCode == 3)
					{
						mFieldBricks[theRow][aCol] = CreateSpecialBrick(9);
						aHintKind = 9;
						aMadeLast = false;
						if (aTutorial && (mField193C == 10 || mField193C == 11))
							aTutorial = false;
					}
					else if (aCode == 4)
					{
						mFieldBricks[theRow][aCol] = CreatePowerBrick(10);
						aHintKind = 10;
						aMadeLast = true;
					}
					else if (aCode == 6)
					{
						mFieldBricks[theRow][aCol] = CreateSpecialBrick(0xb);
						aHintKind = 0xb;
						aMadeLast = true;
					}
					else if (aCode == 7)
					{
						mFieldBricks[theRow][aCol] = CreateMutator();
						aHintKind = 0xc;
						aMadeLast = false;
						if (!m8fc)
							FireTo(mFieldBricks[theRow][aCol], 0x28, 1);
						if (aTutorial && (mField193C == 0xe || mField193C == 0xf))
							aTutorial = false;
					}
					else if (aCode == 8)
					{
						mFieldBricks[theRow][aCol] = CreatePowerBrick(0xd);
						aHintKind = 0xd;
						aMadeLast = true;
					}
					else
					{
						if (aCode == 0)
							mFieldBricks[theRow][aCol] = CreateBrick(((int) AstroRand() % 500000000) % (m1a28 + 4));
						aMade = false;
					}
					if (aMade && aShowHint)
						ShowHint(aHintKind, aCol, aHintRow, !theFromTop);
				}
			}

			Brick* aBrick = mFieldBricks[theRow][aCol];
			aBrick->SetParticleX((float) (aCol * 0x2c + 0xa6));
			if (!theFromTop)
			{
				int aY = theRow * 0x19 + 4;
				if (theRow > 0)
					aY = theRow * 0x19 + 9;
				aBrick->SetParticleY((float) aY);
			}
			else
				aBrick->SetParticleY(-21.0f);

			if (aCode == 0)
			{
				// a plain brick that would join too many of its colour changes its colour (the next one) until it does not
				int aType = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
				for (;;)
				{
					CollectConnected(aType, aCol, theRow, (unsigned int) m1a54, true, NULL, -1, -1);
					if ((unsigned int) mConnectedCells.size() < (unsigned int) m1a54)
						break;
					aBrick = mFieldBricks[theRow][aCol];
					aType = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
					aType++;
					if (aType > m1a28 + 3)
						aType = 0;
					aBrick->SetType(aType);
					std::vector<CellRef>().swap(mConnectedCells);
				}
				std::vector<CellRef>().swap(mConnectedCells);
			}

			aBrick = mFieldBricks[theRow][aCol];
			if (aBrick != NULL)
			{
				int aKey = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
				if (HasMarkOfType(aKey) && (!aBrick->mSpecial || aBrick->mMutated == 1))
				{
					FireTo(aBrick, 0x20, aKey);
					aBrick = mFieldBricks[theRow][aCol];
					aKey = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
					int aCount = GetMarkCount(aKey);
					if (aCount < 0xff)
						FireTo(aBrick, 0x25, GetMarkCount(aKey));
				}
			}
			if (theFromTop)
				mFieldBricks[theRow][aCol]->StartBounce();
		}
	}

	int aHighest = CheckStackHeight();
	if (aHighest < 0x13)
	{
		for (aCol = 0; aCol < 10; aCol++)
		{
			if ((float) ColumnDepth(aCol) > 15.0f)
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
		CheckShipColumn();
	}
	else if (m1a60 != 0)
	{
		GameOver();
		return;
	}
}
