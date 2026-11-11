// Board: the update of the bricks. Every update the bricks of the field and of the falling lists step once
// (UpdateBricks); when a kill has been asked for (the list of kinds at +0xf8) the idle bricks that match are removed, the
// bricks next to them are hit, and what loses its support falls. When nothing is left to do the game looks for a move and
// ends the level of a field that is cleared.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Font.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "SmallObjects.h"
#include "Res.h"

using namespace Sexy;

// 0x450d70: take the brick at (theCol, theRow) out of the field. It stops being a target of the piece in hand, a whole
// steel brick (type 9) cracks when theCrack is set, the cell is remembered for the bricks above to fall, the brick goes to
// the list that deletes it (+0xa0) and the Board tells its listeners (event 3, to the brick only).
void Board::RemoveBrick(Brick* theBrick, int theCol, int theRow, bool theCrack)
{
	// (the original reads the brick from the cell again; the argument is not used)
	(void) theBrick;
	Brick* aBrick = mFieldBricks[theRow][theCol];

	for (unsigned int i = 0; i < mTargeted.size(); i++)
	{
		if (mTargeted[i] == aBrick)
		{
			mTargeted.erase(mTargeted.begin() + i);
			break;
		}
	}

	if (theCrack && aBrick->mMutated != 1 && aBrick->mType == 9 && ((SpecialBrick*) aBrick)->mStage == 2)
		((SpecialBrick*) aBrick)->Crack(true);

	CellRef aCell = {theCol, theRow};
	mTakenCells.push_back(aCell);
	mTakenBricks.push_back(aBrick);
	m1994++;
	mFieldBricks[theRow][theCol] = NULL;
	FireTo(aBrick, 3);
}

// 0x4515f0: the brick at (theCol, theRow) is hit when a neighbour (right, left, above, below, the first that fits) is of
// the kind that is being killed. A whole steel brick only cracks (false); any other brick is removed with the sound of an
// explosion, panned by its column (true).
bool Board::HitByNeighbour(int theCol, int theRow, int theKind)
{
	bool aHit = false;
	if (theCol + 1 < 10 && mFieldBricks[theRow][theCol + 1] != NULL && TypesMatch(mFieldBricks[theRow][theCol + 1], theKind))
		aHit = true;
	else if (theCol - 1 >= 0 && mFieldBricks[theRow][theCol - 1] != NULL && TypesMatch(mFieldBricks[theRow][theCol - 1], theKind))
		aHit = true;
	else if (theRow - 1 >= 0 && mFieldBricks[theRow - 1][theCol] != NULL && TypesMatch(mFieldBricks[theRow - 1][theCol], theKind))
		aHit = true;
	else if (theRow + 1 < 0x13 && mFieldBricks[theRow + 1][theCol] != NULL && TypesMatch(mFieldBricks[theRow + 1][theCol], theKind))
		aHit = true;
	if (!aHit)
		return false;

	Brick* aBrick = mFieldBricks[theRow][theCol];
	if (aBrick->mMutated != 1 && aBrick->mType == 9 && ((SpecialBrick*) aBrick)->mStage == 2)
	{
		((SpecialBrick*) aBrick)->Crack(true);
		return false;
	}
	RemoveBrick(aBrick, theCol, theRow, false);
	mApp->PlaySampleAt(SOUND_EXPLODE1, theCol);
	return true;
}

// 0x458xxx (inside 0x4582d0, four times): what a removed brick does to one neighbour. An idle neighbour of a kind that
// dies with its neighbours (6, 7, 8, 10, 0xb .. 0xe, and the plain colours) is removed; a steel brick (9) cracks once,
// the second time it is removed. theCracked holds the steel bricks that have cracked in this pass.
void Board::DamageNeighbour(int theCol, int theRow, std::vector<CellRef>& theCracked)
{
	Brick* aBrick = mFieldBricks[theRow][theCol];
	int aKind = (aBrick->mMutated != 1) ? aBrick->mType : 0xc;
	if (!aBrick->IsIdle())
		return;

	if (aKind == 9)
	{
		if (!HasCell(theCol, theRow, theCracked))
		{
			if (((SpecialBrick*) aBrick)->mStage < 2)
				RemoveBrick(aBrick, theCol, theRow, false);
			else
			{
				((SpecialBrick*) aBrick)->Crack(true);
				CellRef aCell = {theCol, theRow};
				theCracked.push_back(aCell);
			}
		}
		return;
	}

	if (aKind == 6 || aKind == 7 || aKind == 0xb || aKind == 8 || aKind == 10 || aKind == 0xe || aKind == 0xd ||
		aKind == 0xc || aBrick->mMutated == 0 || aBrick->mMutated == 1)
		RemoveBrick(aBrick, theCol, theRow, false);
}

// 0x4582d0: for every cell that was emptied (theKilled) the bricks above, below, left and right of it (in that order)
// are hit by DamageNeighbour; the top row (row 0) is never hit from below.
void Board::DamageNeighbours(const std::vector<CellRef>& theKilled)
{
	std::vector<CellRef> aCracked;
	for (unsigned int i = 0; i < theKilled.size(); i++)
	{
		int aCol = theKilled[i].mCol;
		int aRow = theKilled[i].mRow;

		int aUp = aRow - 1;
		if (aUp > 0 && mFieldBricks[aUp][aCol] != NULL)
			DamageNeighbour(aCol, aUp, aCracked);

		int aDown = aRow + 1;
		if (aDown < 0x13 && mFieldBricks[aDown][aCol] != NULL)
			DamageNeighbour(aCol, aDown, aCracked);

		int aLeft = aCol - 1;
		if (aLeft >= 0 && mFieldBricks[aRow][aLeft] != NULL)
			DamageNeighbour(aLeft, aRow, aCracked);

		int aRight = aCol + 1;
		if (aRight < 10 && mFieldBricks[aRow][aRight] != NULL)
			DamageNeighbour(aRight, aRow, aCracked);
	}
}

// 0x433df0: the banner of a text ("No More Moves"): the text in the font on a new image (mBannerTextImage) and a second, blank image
// of the same width and 0x30 pixels taller (mBannerWaveImage); it stays for theTime updates (+0x1b80 counts it, +0x1b84 keeps the
// start).
void Board::BuildBanner(const std::string& theText, int theTime, Font* theFont)
{
	delete mBannerTextImage;
	delete mBannerWaveImage;

	mBannerTextImage = new MemoryImage(mApp);
	mBannerTextImage->Create(theFont->StringWidth(theText), theFont->GetHeight());
	mBannerTextImage->SetImageMode(true, true);

	mBannerWaveImage = new MemoryImage(mApp);
	mBannerWaveImage->Create(mBannerTextImage->GetWidth(), mBannerTextImage->GetHeight() + 0x30);
	mBannerWaveImage->SetImageMode(true, true);

	Graphics g(mBannerTextImage);
	g.SetColor(Color(0xff, 0xff, 0xff));
	g.SetFont(theFont);
	g.DrawString(theText, 0, theFont->GetAscent());

	m1b80 = theTime;
	m1b84 = theTime;
	m944 = 0;
}

// 0x4545b0: the field has been cleared: a bonus of the fraction of the stage that is left (+0x940, up to 100000, in
// steps of 50) is paid with a text that moves to the middle of the screen.
void Board::ClearBonus()
{
	int aBonus = (int) (m940 * 10000.0f);
	if (aBonus > 100000)
		aBonus = 100000;
	mApp->PlaySample(SOUND_PERFECT);
	aBonus = aBonus - aBonus % 50;

	std::string aText = StrFormat(GTC(0x5652f8), aBonus);
	mFieldsCleared++;
	int aWidth = FONT_POINTS->StringWidth(aText);
	FloatText* aFloat = new FloatText(aText, FONT_POINTS, (float) (0x182 - aWidth / 2), 250.0f, 300, 10.0f);
	mFloatTexts.push_back(aFloat);
	AddScore(aBonus, false);
	m1b60 = 0;
	m1b68 = 0xaf;
	m1b64 = 1;
}

// 0x45a750: nothing more can be done in a cleared or stuck field: the bricks left are counted (rows 1 down, and the
// piece in hand) for the count-down that pays them (+0x1b94 the count, +0x1b9c what it is worth: a twentieth of the
// level's bonus score +0x960 for each brick, at most all of it).
void Board::StartClearCount()
{
	m916 = true;
	m90f = false;
	m1b88 = 0;
	m1b8c = 0;
	if (mHandKindBrick != NULL)
	{
		mDoomedBricks.insert(mHandKindBrick);
		mHandKindBrick = NULL;
	}
	mTargeted.clear();
	m19fc = 0;
	Fire(0x28, 1);
	mSpecialBannerTexts.clear();

	for (unsigned int aRow = 1; aRow < mFieldBricks.size(); aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			if (mFieldBricks[aRow][aCol] != NULL)
				m1b94++;
		}
	}
	m1b94 += (int) mHeldBricks.size();

	int aStep = (int) ((float) m960 * 0.05f);
	m1b9c = m1b94 * aStep;
	if (m1b9c > m960)
		m1b9c = m960;
	m1ba0 = 0;
	m1ba8 = 0;
	m1bb0 = 0;
	m1b90 = 0x80;
	m1bb4 = aStep;
	m1ba4 = (m1b94 == 0) ? m960 : 0;
	SaveGameState();
	gFlag5A5100 = false;
}

// 0x46e9b0: one update of every brick. First the bricks of the field step (rows 0 to 18). If a kill is asked for (the
// kinds in +0xf8) and nothing is falling, every idle brick that matches is removed (and one that is next to a match when
// it is of a kind that dies with its neighbours), the neighbours of the removed bricks are hit, and what loses support
// falls (DropUnsupported). Then the stage, the game-over and the "no more moves" checks run; the lists of falling bricks
// step last.
void Board::UpdateBricks()
{
	for (int aRow = 0; aRow < 0x13; aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			if (mFieldBricks[aRow][aCol] != NULL)
				mFieldBricks[aRow][aCol]->Update();
		}
	}

	if (!mKillAllTypes.empty() && mFiredGroups.empty() && mFallingGroups.empty() && mTakenBricks.empty())
	{
		bool aRemoved = false;
		std::vector<CellRef> aCells;
		for (int aRow = 1; aRow < 0x13; aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick == NULL || !aBrick->IsIdle())
					continue;

				for (unsigned int i = 0; i < mKillAllTypes.size(); i++)
				{
					if (TypesMatch(aBrick, mKillAllTypes[i]))
					{
						CellRef aCell = {aCol, aRow};
						aCells.push_back(aCell);
						RemoveBrick(aBrick, aCol, aRow, false);
						aRemoved = true;
						break;
					}
					if (aBrick->mMutated != 1 && (aBrick->mType == 6 || aBrick->mType == 7 || aBrick->mType == 9 ||
						aBrick->mType == 0xb) && HitByNeighbour(aCol, aRow, mKillAllTypes[i]))
						break;
				}
			}
		}

		DamageNeighbours(aCells);
		DropUnsupported(mTakenCells);

		if (m1980 > 1 && mTakenCells.empty() && mFallingGroups.empty())
		{
			if (m1980 > 2 && (mField193C > 1 || m1a60 == 2))
				m8ee = true;
			if (m19fc > 0x3c && !AnyMoveLeft(NULL))
			{
				if (m19fc > 0x3c)
					m19fc = 0x3c;
				else
					m19d8 = (int) ((float) m19cc + m930 * 20.0f);
			}
		}

		mKillAllTypes.clear();

		if (!aRemoved)
		{
			bool aFlag = false;
			if (m910)
			{
				if (!m912 && !m911 && m1b7c == 0 && m1994 == 0 && CountMovingBricks(false) == 0 && mFiredGroups.empty() &&
					mFallingGroups.empty() && !AnyMoveLeft(&aFlag))
				{
					if (!aFlag)
					{
						m1b7c = 400;
						BuildBanner(GT(0x56568c), m1b7c, FONT_NEURO30);
						mApp->PlaySample(SOUND_NO_MORE_MOVES);
					}
					StartClearCount();
				}
			}
			else if (mFiredGroups.empty() && mFallingGroups.empty() && mTakenBricks.empty() && mKillAllTypes.empty() && CountMovingBricks(false) == 0)
			{
				m8fb = true;
				for (unsigned int aRow = 1; aRow < mFieldBricks.size(); aRow++)
				{
					int aCol = 0;
					while (aCol < 10 && mFieldBricks[aRow][aCol] == NULL)
						aCol++;
					if (aCol < 10)
					{
						m8fb = false;
						break;
					}
				}
				if (m8fb)
					ClearBonus();
			}
		}
	}

	for (std::list<Brick*>::iterator i = mHeldBricks.begin(); i != mHeldBricks.end(); ++i)
		(*i)->Update();

	unsigned int aGroups = (unsigned int) mFiredGroups.size();
	for (std::list<std::list<Brick*> >::iterator aGroup = mFiredGroups.begin(); aGroup != mFiredGroups.end(); )
	{
		unsigned int aCount = (unsigned int) aGroup->size();
		std::list<Brick*>::iterator aBrick = aGroup->begin();
		while (aBrick != aGroup->end())
		{
			(*aBrick)->Update();
			if (mFiredGroups.size() != aGroups)
				goto DoneGroups;
			if (aGroup->size() != aCount)
				break;
			++aBrick;
		}
		if (mFiredGroups.size() != aGroups || aGroup->size() != aCount)
			break;
		++aGroup;
	}
DoneGroups:

	{
		bool aFirst = true;
		std::list<Brick*>::iterator aPrev = mGoneBricks.end();
		std::list<Brick*>::iterator i = mGoneBricks.begin();
		while (i != mGoneBricks.end())
		{
			if ((*i)->Update())
			{
				// the brick has taken itself out of the list: go on from the one before it
				if (aFirst)
				{
					i = mGoneBricks.begin();
					if (i == mGoneBricks.end())
						break;
				}
				else
					i = aPrev;
			}
			aPrev = i;
			++i;
			aFirst = false;
		}
	}

	for (std::list<std::list<Brick*> >::iterator aGroup = mFallingGroups.begin(); aGroup != mFallingGroups.end(); ++aGroup)
	{
		for (std::list<Brick*>::iterator aBrick = aGroup->begin(); aBrick != aGroup->end(); ++aBrick)
			(*aBrick)->Update();
	}

	for (std::list<Brick*>::iterator i = mTakenBricks.begin(); i != mTakenBricks.end(); )
	{
		(*i)->Update();
		if (mTakenBricks.empty())
			break;
		++i;
	}

	for (unsigned int i = 0; i < mFadedBricks.size(); i++)
		mFadedBricks[i]->Update();
}

// 0x438a30: is the brick in one of the falling groups at +0x94?
bool Board::IsInFallingGroup94(Brick* theBrick)
{
	for (std::list<std::list<Brick*> >::iterator aGroup = mFallingGroups.begin(); aGroup != mFallingGroups.end(); ++aGroup)
	{
		for (std::list<Brick*>::iterator aBrick = aGroup->begin(); aBrick != aGroup->end(); ++aBrick)
		{
			if (*aBrick == theBrick)
				return true;
		}
	}
	return false;
}

// 0x4389c0: is the brick in one of these groups?
bool Board::IsInGroups(Brick* theBrick, const std::vector<std::list<Brick*> >& theGroups)
{
	for (unsigned int i = 0; i < theGroups.size(); i++)
	{
		for (std::list<Brick*>::const_iterator aBrick = theGroups[i].begin(); aBrick != theGroups[i].end(); ++aBrick)
		{
			if (*aBrick == theBrick)
				return true;
		}
	}
	return false;
}

// 0x46cc20: the bricks that have lost their support fall. For each emptied cell (theCells, sorted first) that has an idle
// brick above it (or is at the top, or in row 1 of a game of the second kind), the bricks below it in the column are taken
// out of the field in groups, a gap ending a group: each gets the height it will fall to (mTargetY, from the cell on,
// 25 apart; the top row's cell counts as row 1). The groups then go into the list of falling groups at +0x94, before the
// first group in the same column that starts higher.
void Board::DropUnsupported(std::vector<CellRef>& theCells)
{
	if (theCells.empty())
		return;

	SortCells(theCells);

	std::vector<std::list<Brick*> > aGroups;
	for (unsigned int i = 0; i < theCells.size(); i++)
	{
		int aCol = theCells[i].mCol;
		int aRow = theCells[i].mRow;
		std::list<Brick*> aList;

		bool aGo = true;
		if (aRow != 0 && !(m910 && aRow == 1))
		{
			Brick* anAbove = mFieldBricks[aRow - 1][aCol];
			aGo = (anAbove != NULL && anAbove->IsIdle());
		}

		if (aGo && aRow + 1 < 0x13)
		{
			int aY = aRow * 0x19 + 9;
			for (int aBelow = aRow + 1; aBelow < 0x13; aBelow++)
			{
				Brick* aBrick = mFieldBricks[aBelow][aCol];
				if (aBrick != NULL)
				{
					if (aBrick->IsIdle() && !IsInFallingGroup94(aBrick) && !IsInGroups(aBrick, aGroups))
					{
						if (aY == 9)
							aY += 0x19;
						aBrick->mTargetY = (float) aY;
						aY += 0x19;
						aList.push_back(aBrick);
						mFieldBricks[aBelow][aCol] = NULL;
					}
				}
				else if (!aList.empty())
				{
					aGroups.push_back(aList);
					aList.clear();
				}
			}
			if (!aList.empty())
				aGroups.push_back(aList);
		}
	}

	for (unsigned int i = 0; i < aGroups.size(); i++)
	{
		int aGroupX = (int) aGroups[i].front()->mX;
		int aGroupY = (int) aGroups[i].front()->mY;
		std::list<std::list<Brick*> >::iterator aPos;
		for (aPos = mFallingGroups.begin(); aPos != mFallingGroups.end(); ++aPos)
		{
			if ((int) aPos->front()->mX == aGroupX && aGroupY > (int) aPos->front()->mY)
				break;
		}
		mFallingGroups.insert(aPos, aGroups[i]);
	}
}
