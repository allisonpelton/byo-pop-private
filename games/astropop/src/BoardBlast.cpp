// Board: the handlers of the power bricks that take bricks out of the field by their place: the column nuke (event 0xd),
// the blast of a radial bomb or a row nuke (event 0x26), and the text that tells what a brick that has just fallen does
// (event 7).

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Font.h"
#include "SmallObjects.h"
#include "Res.h"

using namespace Sexy;

// 0x45cae0 event 7: a brick has fallen to the floor. The ones that are aimed (radial bomb, row and column nuke) have the
// bricks they would hit flash again; the kinds that have a line of help get it as a text that scrolls across the
// bottom of the field
void Board::OnBrickLanded(Brick* theBrick)
{
	mHeldBricksNotLanded--;

	std::string aText;
	int aKind = theBrick->GetEffectiveType();
	switch (aKind)
	{
	case 6:		aText = GT(0x565660); break;
	case 8:		aText = GT(0x56559c); break;
	case 10:	aText = GT(0x565628); break;
	case 0xb:	aText = GT(0x565600); break;
	case 0xd:	aText = GT(0x5655d4); break;
	case 0xe:	aText = GT(0x565568); break;
	default:	break;
	}

	if (aKind == 0xb || aKind == 0xd || aKind == 6)
	{
		for (unsigned int i = 0; i < mTargeted.size(); i++)
			mTargeted[i]->StopFlash();
		mTargeted.clear();
		HighlightTargets();
	}

	if (!aText.empty())
	{
		mSpecialBannerTexts.clear();
		Font* aFont = FONT_DIALOG_BOX;
		bool aDone = false;
		int anX = -0x1e - aFont->StringWidth(aText);
		do
		{
			if (anX >= aFont->StringWidth(aText) + 0x29e)
				aDone = true;
			DrawnText aDrawn;
			aDrawn.mX = anX;
			aDrawn.mY = 0x1cc;
			aDrawn.mText = aText;
			aDrawn.mFont = aFont;
			mSpecialBannerTexts.push_back(aDrawn);
			anX = anX + 0x1e + aFont->StringWidth(aText);
		}
		while (!aDone);
	}

	if (mHeldBricksNotLanded == 0)
	{
		mApp->PlaySampleAt(SOUND_BLOCK_HIT_SHIP, (mShipX - 0xa6) / 0x2c);
		if (mHeldBricksNotLanded == 0)
			mShipBounceY = 0.0f;
	}
}

// The bricks of theCells (of the field) go: each is taken out, with the particles of theImage, and the Board tells
// them to die afterwards, all at once (event 3). Used by the nukes and blasts, which take their bricks together.
void Board::RemoveCells(const std::vector<CellRef>& theCells, Image* theImage)
{
	size_t anOldCount = mTakenBricks.size();
	for (unsigned int i = 0; i < theCells.size(); i++)
	{
		Brick* aBrick = mFieldBricks[theCells[i].mRow][theCells[i].mCol];
		aBrick->SetParticleImages(theImage);
		mTakenCells.push_back(theCells[i]);
		mTakenBricks.push_back(aBrick);
		mFieldBricks[theCells[i].mRow][theCells[i].mCol] = NULL;
		mTakenBrickCount++;
	}
	std::vector<Brick*> aNew;
	size_t n = 0;
	for (std::list<Brick*>::iterator i = mTakenBricks.begin(); i != mTakenBricks.end(); ++i, n++)
	{
		if (n >= anOldCount)
			aNew.push_back(*i);
	}
	for (unsigned int i = 0; i < aNew.size(); i++)
		FireTo(aNew[i], 3);
}

// 0x453250 event 0xd: a column nuke goes off: every brick of its column from the bottom up (the idle ones that are not
// already taken) goes with it, whole steel cracks as it goes
void Board::OnColumnNuke(Brick* theBrick)
{
	std::vector<CellRef> aCells;

	AdvanceChain();
	HideCellHint(theBrick);

	bool aMode2 = mMode == 2 && mPowerupActive == 1;
	bool aMode1 = mMode == 1 && mPowerupActive == 1;

	unsigned int anIndex = 0;
	std::list<Brick*>::iterator i = mTakenBricks.begin();
	while (i != mTakenBricks.end() && *i != theBrick)
	{
		++i;
		anIndex++;
	}
	if (i == mTakenBricks.end())
		return;

	int aCol = mTakenCells[anIndex].mCol;
	for (int aRow = 0x13; aRow > 0; aRow--)
	{
		if (mFieldBricks[aRow][aCol] != NULL && !HasCell(aCol, aRow, mConnectedCells) &&
			((aMode2 && !HasCell(aCol, aRow, mTurbotMarkedCells)) || (aMode1 && !HasCell3(aCol, aRow, mAtomicMarkedBricks)) || (!aMode2 && !aMode1)))
		{
			CellRef aCell;
			aCell.mCol = aCol;
			aCell.mRow = aRow;
			aCells.push_back(aCell);
		}
	}

	size_t anOldCount = mTakenBricks.size();
	for (unsigned int k = 0; k < aCells.size(); k++)
	{
		Brick* aBrick = mFieldBricks[aCells[k].mRow][aCells[k].mCol];
		if (aBrick->mMutated != 1 && aBrick->mType == 9 && ((SpecialBrick*) aBrick)->mStage == 2)
			((SpecialBrick*) aBrick)->Crack(true);
		aBrick->SetParticleImages(IMAGE_PARTICLE_FIREBALL);
		mTakenCells.push_back(aCells[k]);
		mTakenBricks.push_back(aBrick);
		mFieldBricks[aCells[k].mRow][aCells[k].mCol] = NULL;
		mTakenBrickCount++;
	}
	std::vector<Brick*> aNew;
	size_t n = 0;
	for (std::list<Brick*>::iterator j = mTakenBricks.begin(); j != mTakenBricks.end(); ++j, n++)
	{
		if (n >= anOldCount)
			aNew.push_back(*j);
	}
	for (unsigned int k = 0; k < aNew.size(); k++)
		FireTo(aNew[k], 3);
}

// 0x45ce40 event 0x26: a special brick (stone 0, steel -1, radial bomb 1, row nuke 19) has died. The stone and the steel
// blast their neighbours like any brick that blasts. The radial bomb takes the 3 x 3 cells round its own (rows 1 to 18),
// the row nuke the whole row it was in.
void Board::OnBlast(Brick* theBrick, int theArg)
{
	std::vector<CellRef> aCells;

	bool aMode2 = mMode == 2 && mPowerupActive == 1;
	bool aMode1 = mMode == 1 && mPowerupActive == 1;

	HideCellHint(theBrick);

	// a cell that the modes 1 and 2 leave alone is not taken
#define PASSES(c, r) \
	(((aMode2 && !HasCell((c), (r), mTurbotMarkedCells)) || (aMode1 && !HasCell3((c), (r), mAtomicMarkedBricks))) || (!aMode2 && !aMode1))

	unsigned int anIndex = 0;
	std::list<Brick*>::iterator i = mTakenBricks.begin();

	if (theArg < 1 && theBrick->mBlastsOnPop)
	{
		while (i != mTakenBricks.end() && *i != theBrick)
		{
			++i;
			anIndex++;
		}
		if (i == mTakenBricks.end())
			return;
		BlastNeighbours(mTakenCells[anIndex].mCol, mTakenCells[anIndex].mRow, theBrick);
		return;
	}

	if (theArg != 0x13)
	{
		if (theArg != 1)
			return;
		AdvanceChain();
		while (i != mTakenBricks.end() && *i != theBrick)
		{
			++i;
			anIndex++;
		}
		if (i == mTakenBricks.end())
			return;

		int aCol = mTakenCells[anIndex].mCol;
		int aRow = mTakenCells[anIndex].mRow;
		int aFirstRow = aRow - 1;
		if (aFirstRow < 1)
			aFirstRow = 1;
		int aFirstCol = aCol - 1;
		if (aFirstCol < 0)
			aFirstCol = 0;
		if (aFirstRow > 0x12)
			return;
		for (int aNearRow = aFirstRow; aNearRow <= aRow + 1 && aNearRow < 0x13; aNearRow++)
		{
			for (int aNearCol = aFirstCol; aNearCol < 10 && aNearCol <= aCol + 1; aNearCol++)
			{
				Brick* aBrick;
				if ((aNearCol != aCol || aNearRow != aRow) && PASSES(aNearCol, aNearRow) && (aBrick = mFieldBricks[aNearRow][aNearCol]) != NULL)
				{
					if (aBrick->mMutated != 1 && aBrick->mType == 9 && ((SpecialBrick*) aBrick)->mStage == 2)
						((SpecialBrick*) aBrick)->Crack(true);
					if (aBrick->IsIdle() && !HasCell(aNearCol, aNearRow, mConnectedCells))
					{
						CellRef aCell;
						aCell.mCol = aNearCol;
						aCell.mRow = aNearRow;
						aCells.push_back(aCell);
					}
				}
			}
		}
		if (aCells.empty())
			return;
		RemoveCells(aCells, IMAGE_PARTICLE_RADIAL);
		return;
	}

	// the row nuke: the row of the brick, every idle brick of it that is not taken already (the original looks at every
	// entry of the list that is the brick, and there is one)
	AdvanceChain();
	for (; i != mTakenBricks.end(); ++i, anIndex++)
	{
		if (*i == theBrick)
		{
			int aRow = mTakenCells[anIndex].mRow;
			for (int c = 0; c < 10; c++)
			{
				Brick* aBrick = mFieldBricks[aRow][c];
				if (aBrick != NULL && aBrick->IsIdle() && aBrick != theBrick && !HasCell(c, aRow, mConnectedCells) && PASSES(c, aRow))
				{
					if (aBrick->mMutated != 1 && aBrick->mType == 9 && ((SpecialBrick*) aBrick)->mStage == 2)
						((SpecialBrick*) aBrick)->Crack(true);
					CellRef aCell;
					aCell.mCol = c;
					aCell.mRow = aRow;
					aCells.push_back(aCell);
				}
			}
		}
	}
	RemoveCells(aCells, IMAGE_PARTICLE_LIGHTNING);
#undef PASSES
}
